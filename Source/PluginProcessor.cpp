#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdint>
#include <cstring>

namespace
{
float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }
float param(const juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* p = s.getRawParameterValue(id)) return p->load();
    return 0.0f;
}

bool encodePadSample(const PhysicalDrumEngineAudioProcessor::Pad& pad, juce::String& encoded)
{
    if (!pad.sample || pad.sample->getNumChannels() <= 0 || pad.sample->getNumSamples() <= 0)
        return false;

    const int channels = pad.sample->getNumChannels();
    const int samples = pad.sample->getNumSamples();
    const double sampleRate = pad.sampleRate;

    juce::MemoryBlock raw;
    raw.append(&channels, sizeof(channels));
    raw.append(&samples, sizeof(samples));
    raw.append(&sampleRate, sizeof(sampleRate));

    for (int channel = 0; channel < channels; ++channel)
        raw.append(pad.sample->getReadPointer(channel), (size_t) samples * sizeof(float));

    encoded = raw.toBase64Encoding();
    return !encoded.isEmpty();
}

bool decodePadSample(PhysicalDrumEngineAudioProcessor::Pad& pad, const juce::String& encoded)
{
    if (encoded.isEmpty()) return false;

    juce::MemoryBlock raw;
    if (!raw.fromBase64Encoding(encoded)) return false;

    const size_t headerSize = sizeof(int) + sizeof(int) + sizeof(double);
    if (raw.getSize() < headerSize) return false;

    const auto* bytes = static_cast<const std::uint8_t*>(raw.getData());
    int channels = 0;
    int samples = 0;
    double sampleRate = 44100.0;
    std::memcpy(&channels, bytes, sizeof(channels));
    bytes += sizeof(channels);
    std::memcpy(&samples, bytes, sizeof(samples));
    bytes += sizeof(samples);
    std::memcpy(&sampleRate, bytes, sizeof(sampleRate));
    bytes += sizeof(sampleRate);

    if (channels < 1 || channels > 32 || samples < 1 || samples > 100000000) return false;

    const size_t audioBytes = (size_t) channels * (size_t) samples * sizeof(float);
    if (raw.getSize() - headerSize != audioBytes) return false;

    auto audio = std::make_unique<juce::AudioBuffer<float>>(channels, samples);
    for (int channel = 0; channel < channels; ++channel)
    {
        std::memcpy(audio->getWritePointer(channel), bytes, (size_t) samples * sizeof(float));
        bytes += (size_t) samples * sizeof(float);
    }

    pad.sample = std::move(audio);
    pad.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    return true;
}
}

PhysicalDrumEngineAudioProcessor::PhysicalDrumEngineAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameters())
{
    const char* names[numPads] = {"Kick", "Snare", "Closed Hat", "Open Hat", "Tom 1", "Tom 2", "Tom 3", "Crash", "Ride", "Clap", "Perc 1", "Perc 2"};
    const int notes[numPads] = {36, 38, 42, 46, 45, 43, 41, 49, 51, 39, 37, 40};
    for (int i = 0; i < numPads; ++i)
    {
        pads[i].name = names[i];
        pads[i].midiNote = notes[i];
    }
    formatManager.registerBasicFormats();
    loadFactorySnare();
}

juce::AudioProcessorValueTreeState::ParameterLayout PhysicalDrumEngineAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&](const char* id, const char* name, float min, float max, float def)
    {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(min, max, 0.001f), def));
    };
    // Global kit controls. Defaults are intentionally neutral so an imported
    // sample is reproduced unchanged until the user moves a control.
    add("sampleRate", "Sample Rate", 1000.0f, 44100.0f, 44100.0f);
    add("output", "Output dB", -60.0f, 6.0f, 0.0f);
    add("filter", "Filter", 20.0f, 20000.0f, 20000.0f);
    add("attack", "Attack", 0.0f, 1.0f, 0.0f);
    add("release", "Release", 0.0f, 1.0f, 0.0f);
    return {p.begin(), p.end()};
}

bool PhysicalDrumEngineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PhysicalDrumEngineAudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr;
    clearVoices();
    leftPeak.store(0.0f);
    rightPeak.store(0.0f);
}

void PhysicalDrumEngineAudioProcessor::clearVoices()
{
    for (auto& v : voices) v.active = false;
}

void PhysicalDrumEngineAudioProcessor::stopAllVoices()
{
    clearVoices();
}

int PhysicalDrumEngineAudioProcessor::noteToPad(int note) const
{
    for (int i = 0; i < numPads; ++i)
        if (pads[i].midiNote == note) return i;
    return -1;
}

void PhysicalDrumEngineAudioProcessor::triggerPadFromUI(int padIndex, float velocity)
{
    triggerPad(padIndex, velocity);
}

void PhysicalDrumEngineAudioProcessor::triggerPad(int padIndex, float velocity)
{
    if (paused.load() || padIndex < 0 || padIndex >= numPads || !pads[padIndex].sample) return;

    Voice* voice = nullptr;
    for (auto& candidate : voices)
        if (!candidate.active) { voice = &candidate; break; }
    if (!voice) voice = &voices[0];

    const float vel = clamp01(velocity);
    const auto& pad = pads[padIndex];
    const int total = pad.sample->getNumSamples();
    const double maxPosition = std::max(0.0, (double) total - 1.0);
    const double startPosition = (double) pad.startNorm * (double) std::max(1, total - 1);

    // Velocity response is part of the instrument itself. It is never a user
    // switch and does not depend on the global controls. The imported sample
    // remains untouched until a hit is triggered; the hit velocity determines
    // how softly or forcefully that source is excited.
    //
    // At maximum velocity the velocity stage is neutral: unity gain, no added
    // filtering, no attack smoothing and the complete sample is available.
    // Lower velocities progressively become softer, darker, slower to arrive,
    // and shorter-lived. This preserves the source sample while making MIDI
    // velocity behave like playing a physical drum.
    const float soft = 1.0f - vel;
    const float velocityGain = std::pow(std::max(0.001f, vel), 0.82f);

    // A soft strike has a gentle onset; a hard strike starts immediately.
    const float attackSeconds = soft * (0.0015f + 0.010f * soft);

    // Soft strikes release earlier, while hard strikes expose the full source.
    // The lower bound keeps even very quiet MIDI notes recognizably drum-like.
    const float maxProgress = 0.30f + 0.70f * std::pow(vel, 0.62f);

    voice->active = true;
    voice->pad = padIndex;
    voice->pos = std::clamp(startPosition, 0.0, maxPosition);
    voice->rate = pad.sampleRate / currentSampleRate;
    voice->velocity = vel;
    voice->pitchCents = pad.tuneCents;
    voice->gain = pad.trim * pad.level * velocityGain;
    voice->attack = attackSeconds * (float) currentSampleRate;
    voice->maxProgress = juce::jlimit(0.30f, 1.0f, maxProgress);
    voice->age = 0;
    voice->sampleRatePhase = 0.0;
    voice->heldSample = 0.0f;
    voice->hasHeldSample = false;
    voice->lowpassState = { 0.0f, 0.0f };
    voice->globalFilterState = { 0.0f, 0.0f };
    voice->lowpassCoeff = 1.0f;
    voice->gainJitter = 1.0f;
}

void PhysicalDrumEngineAudioProcessor::renderVoice(Voice& v, juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!v.active || v.pad < 0 || !pads[v.pad].sample) return;

    auto& pad = pads[v.pad];
    const int total = pad.sample->getNumSamples();
    const int channels = pad.sample->getNumChannels();
    const int endSample = juce::jlimit(1, total, (int) std::round(pad.endNorm * (double) total));

    const float requestedSampleRate = juce::jlimit(1000.0f, (float) currentSampleRate,
                                                    param(apvts, "sampleRate"));
    const double holdInterval = std::max(1.0, currentSampleRate / requestedSampleRate);

    const float filterHz = param(apvts, "filter");
    const bool globalFilterBypassed = filterHz >= 19999.5f;
    const float globalFilterCoeff = globalFilterBypassed ? 1.0f
        : 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
            * juce::jlimit(20.0f, 20000.0f, filterHz) / (float) currentSampleRate);

    const float globalAttack = clamp01(param(apvts, "attack"));
    const float globalRelease = clamp01(param(apvts, "release"));

    for (int i = 0; i < numSamples; ++i)
    {
        if (paused.load()) break;
        if (v.pos < 0.0) { v.pos += 1.0; ++v.age; continue; }

        const int idx = (int) v.pos;
        if (idx >= endSample) { v.active = false; break; }

        const int next = std::min(idx + 1, endSample - 1);
        const float frac = (float) (v.pos - idx);
        const float progress = (float) idx / (float) std::max(1, total - 1);

        // Velocity is an intrinsic physical response, not an optional effect.
        // At velocity 1.0 all of these velocity terms become neutral.
        if (progress > v.maxProgress)
        {
            v.active = false;
            break;
        }

        const float velocity = v.velocity;
        const float soft = 1.0f - velocity;

        // Direct sample path when all user processing is neutral and the hit is
        // hard enough to be neutral. Otherwise interpolate as needed.
        const bool directSample = holdInterval <= 1.000001
            && std::abs(v.rate - 1.0) < 0.000001
            && pad.startNorm <= 0.000001f
            && pad.endNorm >= 0.999999f
            && std::abs(pad.tuneCents) < 0.0001f;

        // Velocity attack: hard hits have no imposed attack envelope. Soft hits
        // ramp in smoothly, giving the impression of a less forceful strike.
        const float velocityAttackEnv = v.attack <= 0.0f ? 1.0f
            : juce::jlimit(0.0f, 1.0f, (float) v.age / std::max(1.0f, v.attack));

        // Global attack is layered on top and is neutral at zero.
        const float globalAttackSamples = globalAttack * 0.100f * (float) currentSampleRate;
        const float globalAttackEnv = globalAttackSamples <= 0.0f ? 1.0f
            : juce::jlimit(0.0f, 1.0f,
                (float) v.age / std::max(1.0f, globalAttackSamples));

        // Global release is neutral at zero. It fades the end of the hit rather
        // than altering the imported source when left untouched.
        const float remaining = (float) std::max(0, endSample - idx);
        const float globalReleaseSamples = globalRelease * 0.250f * (float) currentSampleRate;
        const float globalReleaseEnv = globalReleaseSamples <= 0.0f ? 1.0f
            : juce::jlimit(0.0f, 1.0f,
                remaining / std::max(1.0f, globalReleaseSamples));

        // Intrinsic velocity brightness: hard hits remain completely open;
        // soft hits are progressively low-passed. This is calculated per hit,
        // not stored as a user parameter.
        const float velocityCutoff = 1800.0f + 18200.0f * std::pow(velocity, 0.58f);
        const float velocityCoeff = velocity >= 0.999999f ? 1.0f
            : 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                * velocityCutoff / (float) currentSampleRate);

        for (int outCh = 0; outCh < buffer.getNumChannels(); ++outCh)
        {
            const int srcCh = std::min(outCh, channels - 1);
            const auto* data = pad.sample->getReadPointer(srcCh);

            float raw;
            if (directSample)
                raw = data[idx];
            else
                raw = data[idx] + (data[next] - data[idx]) * frac;

            float out = raw;

            // Intrinsic velocity filtering. At maximum velocity this is a
            // perfect bypass, preserving the imported waveform.
            if (velocity < 0.999999f)
            {
                const size_t stateCh = (size_t) std::min(outCh, 1);
                v.lowpassState[stateCh] += velocityCoeff
                    * (raw - v.lowpassState[stateCh]);
                out = v.lowpassState[stateCh];
            }

            // Optional user filter. Neutral position is a true bypass.
            if (!globalFilterBypassed)
            {
                const size_t stateCh = (size_t) std::min(outCh, 1);
                v.globalFilterState[stateCh] += globalFilterCoeff
                    * (out - v.globalFilterState[stateCh]);
                out = v.globalFilterState[stateCh];
            }

            out *= v.gain * velocityAttackEnv * globalAttackEnv * globalReleaseEnv;
            buffer.addSample(outCh, startSample + i, out);
        }

        if (holdInterval <= 1.000001)
            v.pos += v.rate;
        else
        {
            v.sampleRatePhase -= 1.0;
            if (v.sampleRatePhase <= 0.0)
            {
                v.sampleRatePhase += holdInterval;
                v.pos += v.rate * holdInterval;
            }
        }
        ++v.age;
    }
}

void PhysicalDrumEngineAudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    float l = 0.0f, r = 0.0f;
    if (buffer.getNumChannels() > 0) l = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1) r = buffer.getMagnitude(1, 0, buffer.getNumSamples()); else r = l;
    leftPeak.store(juce::jmax(l, leftPeak.load() * 0.86f));
    rightPeak.store(juce::jmax(r, rightPeak.load() * 0.86f));
}

void PhysicalDrumEngineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    const int totalSamples = buffer.getNumSamples();
    int renderStart = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int eventSample = juce::jlimit(0, totalSamples, metadata.samplePosition);
        if (eventSample > renderStart)
        {
            for (auto& v : voices) renderVoice(v, buffer, renderStart, eventSample - renderStart);
            renderStart = eventSample;
        }
        if (msg.isNoteOn() && msg.getVelocity() > 0.0f)
            triggerPad(noteToPad(msg.getNoteNumber()), msg.getFloatVelocity());
    }
    if (renderStart < totalSamples)
        for (auto& v : voices) renderVoice(v, buffer, renderStart, totalSamples - renderStart);

    buffer.applyGain(dbToGain(param(apvts, "output")));
    updateMeters(buffer);
}

bool PhysicalDrumEngineAudioProcessor::loadSampleForPad(int padIndex, const juce::File& file)
{
    if (padIndex < 0 || padIndex >= numPads || !file.existsAsFile()) return false;
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;
    auto audio = std::make_unique<juce::AudioBuffer<float>>((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(audio.get(), 0, (int) reader->lengthInSamples, 0, true, true);
    pads[padIndex].sample = std::move(audio);
    pads[padIndex].sampleRate = reader->sampleRate;
    pads[padIndex].sampleFile = file;
    // A newly imported sample starts completely neutral. Any previous per-pad
    // edits are cleared so a replacement sample cannot inherit old processing.
    pads[padIndex].trim = 1.0f;
    pads[padIndex].tuneCents = 0.0f;
    pads[padIndex].startNorm = 0.0f;
    pads[padIndex].endNorm = 1.0f;
    pads[padIndex].level = 1.0f;
    return true;
}

void PhysicalDrumEngineAudioProcessor::loadSampleForPadFromChooser(int padIndex)
{
    if (padIndex < 0 || padIndex >= numPads) return;
    sampleChooser = std::make_unique<juce::FileChooser>("Choose a drum sample", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    sampleChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, padIndex](const juce::FileChooser& chooser)
        {
            if (chooser.getURLResult().isLocalFile()) loadSampleForPad(padIndex, chooser.getResult());
            sampleChooser.reset();
        });
}

void PhysicalDrumEngineAudioProcessor::loadFactorySnare()
{
    if (auto* format = formatManager.findFormatForFileExtension("wav"))
    {
        std::unique_ptr<juce::InputStream> stream(new juce::MemoryInputStream(BinaryData::snare_wav, BinaryData::snare_wavSize, false));
        std::unique_ptr<juce::AudioFormatReader> reader(format->createReaderFor(stream.release(), true));
        if (reader)
        {
            auto audio = std::make_unique<juce::AudioBuffer<float>>((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read(audio.get(), 0, (int) reader->lengthInSamples, 0, true, true);
            pads[1].sample = std::move(audio);
            pads[1].sampleRate = reader->sampleRate;
            pads[1].sampleFile = juce::File();
        }
    }
}

void PhysicalDrumEngineAudioProcessor::resetParametersToDefaults()
{
    static constexpr const char* ids[] =
    {
        "sampleRate", "output", "filter", "attack", "release"
    };

    for (const auto* id : ids)
        if (auto* parameter = apvts.getParameter(id))
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
}

void PhysicalDrumEngineAudioProcessor::newKit()
{
    stopAllVoices();
    for (auto& pad : pads)
    {
        pad.sample.reset();
        pad.sampleFile = juce::File();
        pad.trim = 1.0f;
        pad.tuneCents = 0.0f;
        pad.startNorm = 0.0f;
        pad.endNorm = 1.0f;
        pad.level = 1.0f;
    }
    resetParametersToDefaults();
}

bool PhysicalDrumEngineAudioProcessor::saveKit(const juce::File& file)
{
    if (file == juce::File()) return false;
    auto root = std::make_unique<juce::XmlElement>("PHYSICAL_DRUM_KIT");
    root->setAttribute("version", "2.0");
    if (auto params = apvts.copyState().createXml()) root->addChildElement(params.release());
    for (int i = 0; i < numPads; ++i)
    {
        const auto& pad = pads[i];
        auto* node = root->createNewChildElement("PAD");
        node->setAttribute("index", i);
        node->setAttribute("name", pad.name);
        node->setAttribute("note", pad.midiNote);
        node->setAttribute("file", pad.sampleFile.getFullPathName());
        node->setAttribute("sampleRate", pad.sampleRate);
        if (i == 1 && pad.sample && !pad.sampleFile.existsAsFile())
            node->setAttribute("factory", "snare");
        juce::String encodedSample;
        if (encodePadSample(pad, encodedSample))
        {
            auto* sampleNode = node->createNewChildElement("SAMPLE_DATA");
            sampleNode->addTextElement(encodedSample);
        }
        node->setAttribute("trim", pad.trim);
        node->setAttribute("tuneCents", pad.tuneCents);
        node->setAttribute("start", pad.startNorm);
        node->setAttribute("end", pad.endNorm);
        node->setAttribute("level", pad.level);
    }
    return root->writeTo(file);
}

bool PhysicalDrumEngineAudioProcessor::loadKit(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse(file);
    if (!xml || !xml->hasTagName("PHYSICAL_DRUM_KIT")) return false;
    stopAllVoices();
    if (auto* params = xml->getChildByName("PARAMETERS"))
        apvts.replaceState(juce::ValueTree::fromXml(*params));

    for (auto* node : xml->getChildIterator())
    {
        if (!node->hasTagName("PAD")) continue;
        const int index = node->getIntAttribute("index", -1);
        if (index < 0 || index >= numPads) continue;
        auto& pad = pads[index];
        pad.name = node->getStringAttribute("name", pad.name);
        pad.midiNote = node->getIntAttribute("note", pad.midiNote);
        pad.trim = (float) node->getDoubleAttribute("trim", 1.0);
        pad.tuneCents = (float) node->getDoubleAttribute("tuneCents", 0.0);
        pad.startNorm = juce::jlimit(0.0f, 1.0f, (float) node->getDoubleAttribute("start", 0.0));
        pad.endNorm = juce::jlimit(pad.startNorm + 0.001f, 1.0f, (float) node->getDoubleAttribute("end", 1.0));
        pad.level = juce::jlimit(0.0f, 2.0f, (float) node->getDoubleAttribute("level", 1.0));
        const juce::File sample(node->getStringAttribute("file"));
        pad.sample.reset();
        pad.sampleFile = juce::File();
        bool restoredEmbedded = false;
        if (auto* sampleNode = node->getChildByName("SAMPLE_DATA"))
            restoredEmbedded = decodePadSample(pad, sampleNode->getAllSubText());

        if (restoredEmbedded)
        {
            pad.sampleFile = sample;
        }
        else if (sample.existsAsFile())
            loadSampleForPad(index, sample);
        else if (node->getStringAttribute("factory") == "snare")
        {
            if (index == 1) loadFactorySnare();
        }
    }
    return true;
}

void PhysicalDrumEngineAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto root = std::make_unique<juce::XmlElement>("PHYSICAL_DRUM_STATE");
    root->setAttribute("version", "2.0");

    if (auto params = apvts.copyState().createXml())
        root->addChildElement(params.release());

    for (int i = 0; i < numPads; ++i)
    {
        const auto& pad = pads[i];
        auto* node = root->createNewChildElement("PAD");
        node->setAttribute("index", i);
        node->setAttribute("name", pad.name);
        node->setAttribute("note", pad.midiNote);
        node->setAttribute("file", pad.sampleFile.getFullPathName());
        node->setAttribute("sampleRate", pad.sampleRate);
        node->setAttribute("trim", pad.trim);
        node->setAttribute("tuneCents", pad.tuneCents);
        node->setAttribute("start", pad.startNorm);
        node->setAttribute("end", pad.endNorm);
        node->setAttribute("level", pad.level);

        if (i == 1 && pad.sample && !pad.sampleFile.existsAsFile())
            node->setAttribute("factory", "snare");

        juce::String encodedSample;
        if (encodePadSample(pad, encodedSample))
        {
            auto* sampleNode = node->createNewChildElement("SAMPLE_DATA");
            sampleNode->addTextElement(encodedSample);
        }
    }

    copyXmlToBinary(*root, destData);
}

void PhysicalDrumEngineAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        if (xml->hasTagName("PHYSICAL_DRUM_STATE"))
        {
            if (auto* params = xml->getChildByName("PARAMETERS"))
                apvts.replaceState(juce::ValueTree::fromXml(*params));

            stopAllVoices();
            for (auto* node : xml->getChildIterator())
            {
                if (!node->hasTagName("PAD")) continue;
                const int index = node->getIntAttribute("index", -1);
                if (index < 0 || index >= numPads) continue;

                auto& pad = pads[index];
                pad.name = node->getStringAttribute("name", pad.name);
                pad.midiNote = node->getIntAttribute("note", pad.midiNote);
                pad.trim = (float) node->getDoubleAttribute("trim", 1.0);
                pad.tuneCents = (float) node->getDoubleAttribute("tuneCents", 0.0);
                pad.startNorm = juce::jlimit(0.0f, 1.0f, (float) node->getDoubleAttribute("start", 0.0));
                pad.endNorm = juce::jlimit(pad.startNorm + 0.001f, 1.0f, (float) node->getDoubleAttribute("end", 1.0));
                pad.level = juce::jlimit(0.0f, 2.0f, (float) node->getDoubleAttribute("level", 1.0));
                pad.sample.reset();
                pad.sampleFile = juce::File(node->getStringAttribute("file"));
                pad.sampleRate = node->getDoubleAttribute("sampleRate", 44100.0);

                bool restoredEmbedded = false;
                if (auto* sampleNode = node->getChildByName("SAMPLE_DATA"))
                    restoredEmbedded = decodePadSample(pad, sampleNode->getAllSubText());

                if (!restoredEmbedded)
                {
                    const auto sample = pad.sampleFile;
                    if (sample.existsAsFile())
                        loadSampleForPad(index, sample);
                    else if (node->getStringAttribute("factory") == "snare" && index == 1)
                        loadFactorySnare();
                }
            }
        }
        else if (xml->hasTagName(apvts.state.getType()))
        {
            // Backward compatibility with V1.8 states that only stored APVTS parameters.
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }
}

juce::AudioProcessorEditor* PhysicalDrumEngineAudioProcessor::createEditor()
{
    return new PhysicalDrumEngineAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhysicalDrumEngineAudioProcessor();
}
