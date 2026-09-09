#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }
float param(const juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* p = s.getRawParameterValue(id)) return p->load();
    return 0.0f;
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
    add("physicality", "Physicality", 0.0f, 1.0f, 0.75f);
    add("transient", "Transient", 0.0f, 1.0f, 0.70f);
    add("attack", "Attack", 0.0f, 1.0f, 0.35f);
    add("brightness", "Brightness", 0.0f, 1.0f, 0.55f);
    add("pitch", "Pitch Response", 0.0f, 1.0f, 0.35f);
    add("body", "Body", 0.0f, 1.5f, 1.0f);
    add("decay", "Decay", 0.0f, 1.0f, 0.55f);
    add("timing", "Timing", 0.0f, 1.0f, 0.20f);
    add("variation", "Hit Variation", 0.0f, 1.0f, 0.30f);
    add("output", "Output dB", -18.0f, 6.0f, 0.0f);
    add("mix", "Dry/Wet", 0.0f, 1.0f, 1.0f);
    add("sampleRate", "Sample Rate", 1000.0f, 44100.0f, 44100.0f);
    add("sustain", "Sustain", 0.0f, 1.0f, 1.0f);
    add("release", "Release", 0.0f, 1.0f, 0.15f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"limiter", 1}, "Limiter", true));
    add("ceiling", "Limiter Ceiling", -12.0f, 0.0f, -0.1f);
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

    const float phy = clamp01(param(apvts, "physicality"));
    const float trans = clamp01(param(apvts, "transient"));
    const float pitch = clamp01(param(apvts, "pitch"));
    const float variation = clamp01(param(apvts, "variation"));
    const float timing = clamp01(param(apvts, "timing"));
    const float brightness = clamp01(param(apvts, "brightness"));
    const float attack = clamp01(param(apvts, "attack"));
    const float decay = clamp01(param(apvts, "decay"));
    const float body = juce::jlimit(0.0f, 1.5f, param(apvts, "body"));
    const float vel = clamp01(velocity);
    const float velCurve = std::pow(std::max(0.001f, vel), 0.82f);
    const float physicalAmount = juce::jlimit(0.0f, 1.0f, 0.20f + 0.80f * phy);
    const float variationAmount = 0.15f + 0.85f * variation;
    auto signedRandom = [&]() { return voice->rng.nextFloat() * 2.0f - 1.0f; };
    const float randomIdentity = signedRandom() * variationAmount * physicalAmount;

    const float sustainFromVelocity = 0.34f + 0.66f * std::pow(vel, 0.72f);
    const float sustainFromDecay = 0.55f + 0.55f * decay;
    const float sustainVariation = 1.0f + randomIdentity * 0.10f;
    voice->maxProgress = juce::jlimit(0.22f, 1.0f, sustainFromVelocity * sustainFromDecay * sustainVariation);

    const float velocityDurationRate = 1.18f - 0.46f * vel;
    const float pitchFromVelocity = (vel - 0.5f) * 110.0f * pitch * (0.45f + 0.90f * phy);
    const float randomPitch = signedRandom() * (18.0f + 42.0f * variation) * physicalAmount;
    const float cents = pitchFromVelocity + randomPitch + pads[padIndex].tuneCents;
    const double pitchRate = std::pow(2.0, cents / 1200.0);

    voice->active = true;
    voice->pad = padIndex;
    const auto& pad = pads[padIndex];
    const int total = pad.sample->getNumSamples();
    voice->pos = juce::jlimit(0.0, std::max(0.0, (double) total - 1.0), pad.startNorm * std::max(1, total - 1));
    voice->rate = velocityDurationRate * pitchRate * (pad.sampleRate / currentSampleRate);
    voice->velocity = vel;
    voice->pitchCents = cents;
    voice->gainJitter = dbToGain(randomIdentity * 2.8f);
    voice->gain = std::pow(std::max(0.001f, vel), 1.20f)
        * (0.45f + 1.15f * trans * (0.35f + 0.65f * phy))
        * voice->gainJitter * pad.trim * pad.level;

    const float attackSeconds = 0.00015f + (1.0f - vel) * (0.002f + 0.022f * attack * (0.45f + 0.85f * phy));
    voice->attack = attackSeconds * (float) currentSampleRate;
    voice->transientBoost = juce::jlimit(0.25f, 2.60f, (0.35f + 2.10f * std::pow(vel, 0.78f)) * (1.0f + randomIdentity * 0.32f));
    voice->brightnessAmount = juce::jlimit(0.02f, 1.0f, brightness * (0.16f + 0.84f * std::pow(vel, 0.70f)) + randomIdentity * 0.10f);

    const float cutoff = juce::jlimit(900.0f, 19500.0f, 900.0f + voice->brightnessAmount * 18600.0f);
    voice->lowpassCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float) currentSampleRate);
    voice->lowpassState = 0.0f;
    voice->decayScale = juce::jlimit(0.30f, 1.70f, 0.55f + 0.95f * vel + 0.28f * decay + randomIdentity * 0.10f);
    voice->sustain = clamp01(param(apvts, "sustain"));
    voice->releaseSamples = (0.002f + 0.25f * param(apvts, "release")) * (float) currentSampleRate;
    voice->age = 0;
    voice->sampleRatePhase = 0.0;
    voice->heldSample = 0.0f;
    voice->hasHeldSample = false;

    if (timing > 0.0f)
        voice->pos += voice->rng.nextFloat() * timing * physicalAmount * 0.006 * currentSampleRate;
}

void PhysicalDrumEngineAudioProcessor::renderVoice(Voice& v, juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!v.active || v.pad < 0 || !pads[v.pad].sample) return;
    auto& pad = pads[v.pad];
    const auto* data = pad.sample->getReadPointer(0);
    const int total = pad.sample->getNumSamples();
    const float phy = clamp01(param(apvts, "physicality"));
    const float transient = clamp01(param(apvts, "transient"));
    const float body = juce::jlimit(0.0f, 1.5f, param(apvts, "body"));
    const float decay = clamp01(param(apvts, "decay"));
    const float mix = clamp01(param(apvts, "mix"));
    const int endSample = juce::jlimit(1, total, (int) std::round(pad.endNorm * (double) total));

    for (int i = 0; i < numSamples; ++i)
    {
        if (paused.load()) break;
        if (v.pos < 0.0) { v.pos += 1.0; ++v.age; continue; }
        const int idx = (int) v.pos;
        if (idx >= endSample) { v.active = false; break; }

        const float requestedSampleRate = juce::jlimit(1000.0f, (float) currentSampleRate, param(apvts, "sampleRate"));
        const double holdInterval = std::max(1.0, currentSampleRate / requestedSampleRate);
        if (!v.hasHeldSample || v.sampleRatePhase <= 0.0)
        {
            const int next = std::min(idx + 1, endSample - 1);
            const float frac = (float) (v.pos - idx);
            v.heldSample = data[idx] + (data[next] - data[idx]) * frac;
            v.hasHeldSample = true;
            v.sampleRatePhase += holdInterval;
        }

        const float raw = v.heldSample;
        const float progress = (float) (idx - (int) (pad.startNorm * total)) / (float) std::max(1, endSample - (int) (pad.startNorm * total));
        const float clampedProgress = juce::jlimit(0.0f, 1.0f, progress);
        if (clampedProgress >= v.maxProgress) { v.active = false; break; }
        const float normalizedLife = clampedProgress / std::max(0.001f, v.maxProgress);
        const float transientShape = std::exp(-normalizedLife * 105.0f);
        const float bodyShape = 0.50f + 0.50f * std::exp(-normalizedLife * 5.0f);
        const float attackEnv = v.age < v.attack ? (float) v.age / std::max(1.0f, v.attack) : 1.0f;
        const float decayExponent = juce::jlimit(0.18f, 5.5f, 0.22f + (1.0f - decay) * 3.8f) / v.decayScale;
        const float tailShape = std::pow(std::max(0.0f, 1.0f - normalizedLife), decayExponent);
        const float velocityTransient = 0.45f + transient * v.transientBoost * transientShape * (0.35f + 1.75f * v.velocity) * (0.45f + 0.95f * phy);
        const float velocityBody = 0.35f + body * (0.25f + 1.35f * v.velocity) * (0.55f + 0.85f * phy * bodyShape);
        v.lowpassState += v.lowpassCoeff * (raw - v.lowpassState);
        const float spectral = v.lowpassState + v.brightnessAmount * (raw - v.lowpassState);
        const float releaseEnv = v.sustain >= 0.999f ? 1.0f : (v.sustain + (1.0f - v.sustain) * std::max(0.0f, 1.0f - normalizedLife));
        const float env = attackEnv * tailShape * releaseEnv;
        const float out = spectral * v.gain * velocityTransient * velocityBody * env * mix;
        buffer.addSample(0, startSample + i, out);
        if (buffer.getNumChannels() > 1) buffer.addSample(1, startSample + i, out);
        v.sampleRatePhase -= 1.0;
        if (v.sampleRatePhase <= 0.0) v.pos += v.rate * holdInterval;
        ++v.age;
    }
}

void PhysicalDrumEngineAudioProcessor::applyLimiter(juce::AudioBuffer<float>& buffer)
{
    if (param(apvts, "limiter") < 0.5f) return;
    const float ceiling = dbToGain(param(apvts, "ceiling"));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = d[i];
            const float sign = x < 0.0f ? -1.0f : 1.0f;
            d[i] = sign * std::min(std::abs(x), ceiling);
        }
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
    applyLimiter(buffer);
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
    pads[padIndex].startNorm = 0.0f;
    pads[padIndex].endNorm = 1.0f;
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
            pads[1].sampleFile = {};
        }
    }
}

void PhysicalDrumEngineAudioProcessor::resetParametersToDefaults()
{
    auto defaults = createParameters();
    for (const auto& parameter : defaults)
    {
        if (auto* current = apvts.getParameter(parameter->getParameterID().getParamID()))
            current->setValueNotifyingHost(parameter->getDefaultValue());
    }
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
    if (file == juce::File{}) return false;
    auto root = std::make_unique<juce::XmlElement>("PHYSICAL_DRUM_KIT");
    root->setAttribute("version", "1.8");
    if (auto params = apvts.copyState().createXml()) root->addChildElement(params.release());
    for (int i = 0; i < numPads; ++i)
    {
        const auto& pad = pads[i];
        auto* node = root->createNewChildElement("PAD");
        node->setAttribute("index", i);
        node->setAttribute("name", pad.name);
        node->setAttribute("note", pad.midiNote);
        node->setAttribute("file", pad.sampleFile.getFullPathName());
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
        pad.trim = (float) node->getDoubleAttribute("trim", 1.0);
        pad.tuneCents = (float) node->getDoubleAttribute("tuneCents", 0.0);
        pad.startNorm = juce::jlimit(0.0f, 1.0f, (float) node->getDoubleAttribute("start", 0.0));
        pad.endNorm = juce::jlimit(pad.startNorm + 0.001f, 1.0f, (float) node->getDoubleAttribute("end", 1.0));
        pad.level = juce::jlimit(0.0f, 2.0f, (float) node->getDoubleAttribute("level", 1.0));
        const juce::File sample(node->getStringAttribute("file"));
        pad.sample.reset();
        pad.sampleFile = juce::File();
        if (sample.existsAsFile()) loadSampleForPad(index, sample);
    }
    return true;
}

void PhysicalDrumEngineAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}

void PhysicalDrumEngineAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* PhysicalDrumEngineAudioProcessor::createEditor()
{
    return new PhysicalDrumEngineAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhysicalDrumEngineAudioProcessor();
}
