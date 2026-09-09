#include "PluginEditor.h"

namespace
{
const juce::Colour chrome(0xff1b2029);
const juce::Colour chromeLight(0xff5c6675);
const juce::Colour screen(0xff05090b);
const juce::Colour green(0xff86e85a);
const juce::Colour text(0xffe2e6e9);
const juce::Colour muted(0xff9aa2aa);

juce::String valueForParameter(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* p = state.getParameter(id))
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
            return ranged->getNormalisableRange().convertFrom0to1(p->getValue());
    }
    return {};
}
}

PhysicalDrumEngineAudioProcessorEditor::PhysicalDrumEngineAudioProcessorEditor(PhysicalDrumEngineAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1220, 780);
    setResizable(false, false);

    windowTitle.setText("Physical Drum Engine v1.8", juce::dontSendNotification);
    windowTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(17.0f).withStyle("Bold")));
    windowTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(windowTitle);

    menuBar.setText("File     Edit     Kits     Options     Help", juce::dontSendNotification);
    menuBar.setFont(juce::Font(juce::FontOptions{}.withHeight(13.0f)));
    menuBar.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(menuBar);

    marquee.setText("S A M P L E S   F E E L   B E T T E R   H E R E .", juce::dontSendNotification);
    marquee.setJustificationType(juce::Justification::centredRight);
    marquee.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(marquee);

    lcdTitle.setText("PHYSICAL DRUM ENGINE", juce::dontSendNotification);
    lcdTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(18.0f).withStyle("Bold")));
    lcdTitle.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(lcdTitle);

    lcdStatus.setText("V1.8   LOAD. DISTORT. DEGRADE. PLAY.", juce::dontSendNotification);
    lcdStatus.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    lcdStatus.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(lcdStatus);

    setupButton(playButton, "PLAY", [this] { processor.setPaused(false); processor.triggerPadFromUI(selectedPad, (float) velocitySlider.getValue()); });
    setupButton(stopButton, "STOP", [this] { processor.stopAllVoices(); });
    setupButton(pauseButton, "PAUSE", [this] { processor.setPaused(!processor.isPaused()); pauseButton.setButtonText(processor.isPaused() ? "RESUME" : "PAUSE"); });
    setupButton(previousButton, "|<", [this] { selectPad((selectedPad + PhysicalDrumEngineAudioProcessor::numPads - 1) % PhysicalDrumEngineAudioProcessor::numPads); });
    setupButton(nextButton, ">|", [this] { selectPad((selectedPad + 1) % PhysicalDrumEngineAudioProcessor::numPads); });

    setupButton(newKitButton, "New Kit", [this]
    {
        if (juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "New Kit", "Clear the current kit and start fresh?"))
        {
            processor.newKit();
            selectPad(0);
            lcdStatus.setText("NEW KIT CREATED", juce::dontSendNotification);
            refreshPadText();
        }
    });
    setupButton(saveKitButton, "Save Kit", [this] { saveKitToChooser(); });
    setupButton(loadKitButton, "Load Kit", [this] { loadKitFromChooser(); });
    setupButton(importButton, "Import...", [this] { importKit(); });

    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const auto& pad = processor.pads[(size_t) i];
        padLabels[(size_t) i].setText(juce::String::formatted("%s  C%d", pad.name.toRawUTF8(), pad.midiNote), juce::dontSendNotification);
        padLabels[(size_t) i].setJustificationType(juce::Justification::centred);
        padLabels[(size_t) i].setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(padLabels[(size_t) i]);

        setupButton(padButtons[(size_t) i], pad.name, [this, i]
        {
            selectPad(i, true);
        });
    }

    selectedPadInfo.setJustificationType(juce::Justification::centredLeft);
    selectedPadInfo.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(selectedPadInfo);

    setupButton(loadSampleButton, "LOAD", [this] { loadSelectedSample(); });
    setupButton(clearSampleButton, "CLEAR", [this] { clearSelectedSample(); });

    sampleName.setColour(juce::Label::textColourId, green);
    sampleName.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    addAndMakeVisible(sampleName);
    sampleInfo.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(sampleInfo);

    const std::array<const char*, 4> sampleNames = {"TUNE", "START", "END", "LEVEL"};
    for (int i = 0; i < 4; ++i)
    {
        auto& s = sampleKnobs[(size_t) i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        sampleKnobLabels[(size_t) i].setText(sampleNames[(size_t) i], juce::dontSendNotification);
        sampleKnobLabels[(size_t) i].setJustificationType(juce::Justification::centred);
        sampleKnobLabels[(size_t) i].setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(sampleKnobLabels[(size_t) i]);
        addAndMakeVisible(s);
    }

    setupKnob(sampleKnobs[0], sampleKnobLabels[0], "", "TUNE");
    sampleKnobs[0].setRange(-1200.0, 1200.0, 1.0);
    sampleKnobs[0].onValueChange = [this] { processor.pads[(size_t) selectedPad].tuneCents = (float) sampleKnobs[0].getValue(); };
    sampleKnobs[1].setRange(0.0, 0.999, 0.001);
    sampleKnobs[1].onValueChange = [this] { processor.pads[(size_t) selectedPad].startNorm = juce::jlimit(0.0f, 0.999f, (float) sampleKnobs[1].getValue()); if (processor.pads[(size_t) selectedPad].endNorm <= processor.pads[(size_t) selectedPad].startNorm) processor.pads[(size_t) selectedPad].endNorm = juce::jmin(1.0f, processor.pads[(size_t) selectedPad].startNorm + 0.001f); };
    sampleKnobs[2].setRange(0.001, 1.0, 0.001);
    sampleKnobs[2].onValueChange = [this] { processor.pads[(size_t) selectedPad].endNorm = juce::jmax(processor.pads[(size_t) selectedPad].startNorm + 0.001f, (float) sampleKnobs[2].getValue()); };
    sampleKnobs[3].setRange(0.0, 2.0, 0.001);
    sampleKnobs[3].onValueChange = [this] { processor.pads[(size_t) selectedPad].level = (float) sampleKnobs[3].getValue(); };

    const std::array<const char*, 13> names = {"PHYSICALITY", "TRANSIENT", "ATTACK", "BRIGHTNESS", "PITCH", "BODY", "DECAY", "TIMING", "VARIATION", "SAMPLE RATE", "SUSTAIN", "RELEASE", "MIX"};
    for (int i = 0; i < 13; ++i)
        setupKnob(globalKnobs[(size_t) i], globalKnobLabels[(size_t) i], globalKnobIds[(size_t) i], names[(size_t) i]);

    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    volumeSlider.setRange(-18.0, 6.0, 0.1);
    volumeSlider.setValue(processor.apvts.getRawParameterValue("output")->load());
    volumeSlider.onValueChange = [this]
    {
        if (auto* p = processor.apvts.getParameter("output"))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                ranged->setValueNotifyingHost(ranged->getNormalisableRange().convertTo0to1((float) volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);

    velocitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    velocitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    velocitySlider.setRange(0.0, 1.0, 0.001);
    velocitySlider.setValue(1.0);
    addAndMakeVisible(velocitySlider);

    ceilingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ceilingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    ceilingSlider.setRange(-12.0, 0.0, 0.1);
    ceilingSlider.setValue(processor.apvts.getRawParameterValue("ceiling")->load());
    ceilingSlider.onValueChange = [this]
    {
        if (auto* p = processor.apvts.getParameter("ceiling"))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                ranged->setValueNotifyingHost(ranged->getNormalisableRange().convertTo0to1((float) ceilingSlider.getValue()));
    };
    addAndMakeVisible(ceilingSlider);

    setupButton(limiterButton, "ON", [this] { setLimiter(!limiterOn); });

    presetBrowser.addItem("Init", 1);
    presetBrowser.addItem("Lo Fi Kit", 2);
    presetBrowser.addItem("Broken Tape", 3);
    presetBrowser.addItem("Vinyl Room", 4);
    presetBrowser.addItem("Basement", 5);
    presetBrowser.addItem("1998", 6);
    presetBrowser.addItem("Compressed", 7);
    presetBrowser.addItem("Crushed", 8);
    presetBrowser.setSelectedId(1, juce::dontSendNotification);
    presetBrowser.onChange = [this] { applyPreset(presetBrowser.getSelectedId()); };
    addAndMakeVisible(presetBrowser);

    midiMapBox.addItem("General MIDI", 1);
    midiMapBox.addItem("Classic 12 Pad", 2);
    midiMapBox.setSelectedId(1, juce::dontSendNotification);
    midiMapBox.onChange = [this] { applyMidiMap(midiMapBox.getSelectedId()); };
    addAndMakeVisible(midiMapBox);

    refreshPadText();
    refreshSelectedPadControls();
    startTimerHz(30);
}

void PhysicalDrumEngineAudioProcessorEditor::setupButton(juce::TextButton& button, const juce::String& textValue, std::function<void()> action)
{
    button.setButtonText(textValue);
    button.setColour(juce::TextButton::buttonColourId, chrome);
    button.setColour(juce::TextButton::textColourOffId, text);
    button.setColour(juce::TextButton::textColourOnId, green);
    button.onClick = std::move(action);
    addAndMakeVisible(button);
}

void PhysicalDrumEngineAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& id, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18);
    if (id.isNotEmpty())
    {
        if (auto* p = processor.apvts.getParameter(id))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
            {
                const auto range = ranged->getNormalisableRange();
                slider.setRange(range.start, range.end, range.interval);
                slider.setValue(range.convertFrom0to1(ranged->getValue()), juce::dontSendNotification);
                slider.onValueChange = [this, &slider, id]
                {
                    if (auto* p2 = processor.apvts.getParameter(id))
                        if (auto* ranged2 = dynamic_cast<juce::RangedAudioParameter*>(p2))
                            ranged2->setValueNotifyingHost(ranged2->getNormalisableRange().convertTo0to1((float) slider.getValue()));
                };
            }
    }
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(label);
    addAndMakeVisible(slider);
}

void PhysicalDrumEngineAudioProcessorEditor::selectPad(int index, bool audition)
{
    selectedPad = juce::jlimit(0, PhysicalDrumEngineAudioProcessor::numPads - 1, index);
    refreshSelectedPadControls();
    repaint();
    if (audition)
        processor.triggerPadFromUI(selectedPad, (float) velocitySlider.getValue());
}

void PhysicalDrumEngineAudioProcessorEditor::refreshPadText()
{
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const auto& pad = processor.pads[(size_t) i];
        padButtons[(size_t) i].setButtonText(pad.name);
        padLabels[(size_t) i].setText(juce::String::formatted("%s   %d", pad.sampleFile.existsAsFile() ? pad.sampleFile.getFileNameWithoutExtension().toRawUTF8() : "EMPTY", pad.midiNote), juce::dontSendNotification);
    }
}

void PhysicalDrumEngineAudioProcessorEditor::refreshSelectedPadControls()
{
    const auto& pad = processor.pads[(size_t) selectedPad];
    selectedPadInfo.setText("SELECTED: " + pad.name + "    MIDI " + juce::String(pad.midiNote), juce::dontSendNotification);
    sampleName.setText(pad.sampleFile.existsAsFile() ? pad.sampleFile.getFileName() : "NO SAMPLE LOADED", juce::dontSendNotification);
    if (pad.sample)
        sampleInfo.setText(juce::String((double) pad.sampleRate, 1) + " kHz    " + juce::String(pad.sample->getNumSamples()) + " samples", juce::dontSendNotification);
    else
        sampleInfo.setText("Drag a sample onto this pad or press LOAD", juce::dontSendNotification);

    sampleKnobs[0].setValue(pad.tuneCents, juce::dontSendNotification);
    sampleKnobs[1].setValue(pad.startNorm, juce::dontSendNotification);
    sampleKnobs[2].setValue(pad.endNorm, juce::dontSendNotification);
    sampleKnobs[3].setValue(pad.level, juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::loadSelectedSample()
{
    processor.loadSampleForPadFromChooser(selectedPad);
    lcdStatus.setText("LOADING SAMPLE...", juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::clearSelectedSample()
{
    processor.pads[(size_t) selectedPad].sample.reset();
    processor.pads[(size_t) selectedPad].sampleFile = {};
    processor.stopAllVoices();
    refreshSelectedPadControls();
    refreshPadText();
    lcdStatus.setText("SAMPLE CLEARED", juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::saveKitToChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save Drum Kit", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MyKit.pdk"), "*.pdk");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& c)
        {
            auto file = c.getResult().withFileExtension("pdk");
            if (processor.saveKit(file)) lcdStatus.setText("KIT SAVED: " + file.getFileName(), juce::dontSendNotification);
            else lcdStatus.setText("KIT SAVE FAILED", juce::dontSendNotification);
            fileChooser.reset();
        });
}

void PhysicalDrumEngineAudioProcessorEditor::loadKitFromChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Load Drum Kit", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.pdk");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& c)
        {
            if (processor.loadKit(c.getResult()))
            {
                refreshPadText();
                refreshSelectedPadControls();
                lcdStatus.setText("KIT LOADED: " + c.getResult().getFileName(), juce::dontSendNotification);
            }
            else lcdStatus.setText("KIT LOAD FAILED", juce::dontSendNotification);
            fileChooser.reset();
        });
}

void PhysicalDrumEngineAudioProcessorEditor::importKit()
{
    loadKitFromChooser();
}

void PhysicalDrumEngineAudioProcessorEditor::applyPreset(int index)
{
    const std::array<float, 9> values = [&]()
    {
        switch (index)
        {
            case 2: return std::array<float, 9>{0.90f,0.55f,0.25f,0.35f,0.45f,1.05f,0.70f,0.35f,0.45f};
            case 3: return std::array<float, 9>{0.95f,0.85f,0.20f,0.20f,0.65f,1.10f,0.65f,0.40f,0.70f};
            case 4: return std::array<float, 9>{0.75f,0.60f,0.25f,0.45f,0.35f,1.00f,0.55f,0.25f,0.35f};
            case 5: return std::array<float, 9>{1.00f,0.80f,0.15f,0.30f,0.55f,1.20f,0.80f,0.30f,0.55f};
            case 6: return std::array<float, 9>{0.70f,0.90f,0.15f,0.65f,0.50f,0.95f,0.50f,0.20f,0.25f};
            case 7: return std::array<float, 9>{0.65f,0.80f,0.20f,0.50f,0.35f,1.00f,0.40f,0.10f,0.20f};
            case 8: return std::array<float, 9>{1.00f,1.00f,0.10f,0.15f,0.80f,1.30f,0.90f,0.45f,0.80f};
            default: return std::array<float, 9>{0.75f,0.70f,0.35f,0.55f,0.35f,1.00f,0.55f,0.20f,0.30f};
        }
    }();
    for (int i = 0; i < 9; ++i)
    {
        if (auto* p = processor.apvts.getParameter(globalKnobIds[(size_t) i]))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                ranged->setValueNotifyingHost(ranged->getNormalisableRange().convertTo0to1(values[(size_t) i]));
        globalKnobs[(size_t) i].setValue(values[(size_t) i], juce::dontSendNotification);
    }
    lcdStatus.setText("PRESET LOADED: " + presetBrowser.getText(), juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::applyMidiMap(int index)
{
    const int gm[PhysicalDrumEngineAudioProcessor::numPads] = {36,38,42,46,45,43,41,49,51,39,37,40};
    const int classic[PhysicalDrumEngineAudioProcessor::numPads] = {36,38,42,46,45,43,41,49,51,39,37,40};
    const auto* map = index == 2 ? classic : gm;
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i) processor.pads[(size_t) i].midiNote = map[i];
    refreshPadText();
    refreshSelectedPadControls();
    lcdStatus.setText("MIDI MAP: " + midiMapBox.getText(), juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::setLimiter(bool enabled)
{
    limiterOn = enabled;
    if (auto* p = processor.apvts.getParameter("limiter")) p->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
    limiterButton.setButtonText(enabled ? "ON" : "OFF");
    lcdStatus.setText(enabled ? "LIMITER ENABLED" : "LIMITER BYPASSED", juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::timerCallback()
{
    repaint();
    refreshPadText();
    refreshSelectedPadControls();
}

void PhysicalDrumEngineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0e12));
    auto outer = getLocalBounds().reduced(8);
    g.setColour(chrome);
    g.fillRect(outer);
    g.setColour(chromeLight);
    g.drawRect(outer, 2);

    g.setColour(juce::Colour(0xff34475e));
    g.fillRect(outer.getX(), outer.getY(), outer.getWidth(), 25);
    g.setColour(text);
    g.drawRect(outer.getX() + 1, outer.getY() + 1, outer.getWidth() - 2, 23);

    auto content = outer.reduced(10);
    content.removeFromTop(34);

    drawPanel(g, {content.getX(), content.getY(), 580, 86}, "PLAYER");
    g.setColour(screen);
    g.fillRect(content.getX() + 12, content.getY() + 29, 270, 46);
    g.setColour(green);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f)));
    g.drawText("00:00", content.getX() + 20, content.getY() + 34, 80, 20, juce::Justification::left);
    g.drawText("■■■■■■■■■■■■■■", content.getX() + 20, content.getY() + 56, 220, 14, juce::Justification::left);

    auto meterPanel = juce::Rectangle<int>(content.getX() + 850, content.getY() + 610, 340, 88);
    drawPanel(g, meterPanel, "OUTPUT");
    drawMeter(g, {meterPanel.getX() + 16, meterPanel.getY() + 34, 230, 16}, processor.getLeftPeak(), "L");
    drawMeter(g, {meterPanel.getX() + 16, meterPanel.getY() + 56, 230, 16}, processor.getRightPeak(), "R");

    auto padStart = juce::Rectangle<int>(content.getX() + 205, content.getY() + 104, 610, 360);
    const int cellW = padStart.getWidth() / 4;
    const int cellH = padStart.getHeight() / 3;
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        auto r = padAreas[(size_t) i];
        g.setColour(i == selectedPad ? juce::Colour(0xff2e632e) : juce::Colour(0xff141a21));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(i == dragTargetPad ? green : (i == selectedPad ? green : chromeLight));
        g.drawRoundedRectangle(r.toFloat(), 4.0f, i == selectedPad ? 2.0f : 1.0f);
    }

    if (dragTargetPad >= 0)
    {
        g.setColour(green);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        g.drawText("DROP SAMPLE", padAreas[(size_t) dragTargetPad], juce::Justification::centredBottom, false);
    }

    auto samplePanel = juce::Rectangle<int>(content.getX() + 825, content.getY() + 104, 365, 270);
    drawPanel(g, samplePanel, "SAMPLE");
    g.setColour(screen);
    auto wave = samplePanel.reduced(12);
    wave.removeFromTop(42);
    wave.removeFromBottom(52);
    g.fillRect(wave);
    if (auto* s = processor.pads[(size_t) selectedPad].sample.get())
    {
        const auto* data = s->getReadPointer(0);
        const int n = s->getNumSamples();
        juce::Path p;
        for (int x = 0; x < wave.getWidth(); ++x)
        {
            const int idx = juce::jlimit(0, n - 1, (int) ((double) x / wave.getWidth() * n));
            const float y = wave.getCentreY() - data[idx] * wave.getHeight() * 0.45f;
            if (x == 0) p.startNewSubPath((float) wave.getX() + x, y); else p.lineTo((float) wave.getX() + x, y);
        }
        g.setColour(green);
        g.strokePath(p, juce::PathStrokeType(1.0f));
    }

    drawPanel(g, {content.getX(), content.getY() + 478, 815, 120}, "GLOBAL EFFECTS");
    drawPanel(g, {content.getX() + 825, content.getY() + 388, 365, 210}, "PRESET / MIDI");
    drawPanel(g, {content.getX(), content.getY() + 610, 815, 88}, "KIT");
    drawPanel(g, {content.getX() + 610, content.getY() + 104, 205, 360}, "VELOCITY / OUTPUT");
}

void PhysicalDrumEngineAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& titleText)
{
    g.setColour(juce::Colour(0xff11161d));
    g.fillRect(r);
    g.setColour(chromeLight);
    g.drawRect(r, 1);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f)));
    g.drawText(titleText, r.getX() + 8, r.getY() + 5, r.getWidth() - 16, 18, juce::Justification::left);
}

void PhysicalDrumEngineAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float value, const juce::String& label)
{
    g.setColour(screen);
    g.fillRect(r);
    const float normalized = juce::jlimit(0.0f, 1.0f, value);
    g.setColour(green);
    g.fillRect(r.withWidth((int) (r.getWidth() * normalized)));
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText(label, r.getX() - 14, r.getY(), 12, r.getHeight(), juce::Justification::centred);
}

void PhysicalDrumEngineAudioProcessorEditor::resized()
{
    auto outer = getLocalBounds().reduced(8);
    windowTitle.setBounds(outer.getX() + 12, outer.getY() + 2, 250, 21);
    menuBar.setBounds(outer.getX() + 270, outer.getY() + 2, 390, 21);
    marquee.setBounds(outer.getRight() - 330, outer.getY() + 2, 310, 21);

    auto content = outer.reduced(10);
    content.removeFromTop(34);

    auto player = juce::Rectangle<int>(content.getX(), content.getY(), 580, 86);
    lcdTitle.setBounds(player.getX() + 300, player.getY() + 30, 265, 22);
    lcdStatus.setBounds(player.getX() + 300, player.getY() + 54, 265, 20);
    int bx = player.getX() + 12;
    for (auto* b : {&playButton, &stopButton, &pauseButton, &previousButton, &nextButton})
    {
        b->setBounds(bx, player.getY() + 5, 76, 22);
        bx += 80;
    }

    auto kit = juce::Rectangle<int>(content.getX(), content.getY() + 610, 815, 88);
    newKitButton.setBounds(kit.getX() + 12, kit.getY() + 34, 110, 25);
    saveKitButton.setBounds(kit.getX() + 130, kit.getY() + 34, 110, 25);
    loadKitButton.setBounds(kit.getX() + 248, kit.getY() + 34, 110, 25);
    importButton.setBounds(kit.getX() + 366, kit.getY() + 34, 110, 25);
    selectedPadInfo.setBounds(kit.getX() + 490, kit.getY() + 26, 310, 18);

    auto padStart = juce::Rectangle<int>(content.getX() + 205, content.getY() + 104, 610, 360);
    const int cellW = padStart.getWidth() / 4;
    const int cellH = padStart.getHeight() / 3;
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const int row = i / 4, col = i % 4;
        auto r = juce::Rectangle<int>(padStart.getX() + col * cellW + 4, padStart.getY() + row * cellH + 4, cellW - 8, cellH - 8);
        padAreas[(size_t) i] = r;
        padButtons[(size_t) i].setBounds(r);
        padLabels[(size_t) i].setBounds(r.getX() + 4, r.getBottom() - 24, r.getWidth() - 8, 18);
        padLabels[(size_t) i].toFront(false);
    }

    auto samplePanel = juce::Rectangle<int>(content.getX() + 825, content.getY() + 104, 365, 270);
    sampleName.setBounds(samplePanel.getX() + 14, samplePanel.getY() + 32, 235, 22);
    sampleInfo.setBounds(samplePanel.getX() + 14, samplePanel.getY() + 55, 235, 20);
    loadSampleButton.setBounds(samplePanel.getRight() - 86, samplePanel.getY() + 32, 72, 25);
    clearSampleButton.setBounds(samplePanel.getRight() - 86, samplePanel.getY() + 62, 72, 25);
    for (int i = 0; i < 4; ++i)
    {
        const int x = samplePanel.getX() + 14 + i * 82;
        sampleKnobLabels[(size_t) i].setBounds(x, samplePanel.getBottom() - 82, 74, 16);
        sampleKnobs[(size_t) i].setBounds(x, samplePanel.getBottom() - 65, 74, 62);
    }

    auto global = juce::Rectangle<int>(content.getX(), content.getY() + 478, 815, 120);
    for (int i = 0; i < 13; ++i)
    {
        const int x = global.getX() + 8 + i * 61;
        globalKnobLabels[(size_t) i].setBounds(x, global.getY() + 24, 58, 24);
        globalKnobs[(size_t) i].setBounds(x, global.getY() + 46, 58, 68);
    }

    auto vo = juce::Rectangle<int>(content.getX() + 610, content.getY() + 104, 205, 360);
    velocitySlider.setBounds(vo.getX() + 52, vo.getY() + 58, 100, 100);
    volumeSlider.setBounds(vo.getX() + 52, vo.getY() + 176, 100, 100);
    ceilingSlider.setBounds(vo.getX() + 52, vo.getY() + 282, 100, 70);
    limiterButton.setBounds(vo.getX() + 10, vo.getY() + 282, 35, 30);

    auto pm = juce::Rectangle<int>(content.getX() + 825, content.getY() + 388, 365, 210);
    presetBrowser.setBounds(pm.getX() + 14, pm.getY() + 36, 337, 28);
    midiMapBox.setBounds(pm.getX() + 14, pm.getY() + 76, 337, 28);
}

bool PhysicalDrumEngineAudioProcessorEditor::isSupportedSampleFile(const juce::String& path) const
{
    const auto ext = juce::File(path).getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".ogg";
}

int PhysicalDrumEngineAudioProcessorEditor::padAtPosition(int x, int y) const
{
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
        if (padAreas[(size_t) i].contains(x, y)) return i;
    return -1;
}

void PhysicalDrumEngineAudioProcessorEditor::updateDragTarget(int x, int y)
{
    const int target = padAtPosition(x, y);
    if (target != dragTargetPad) { dragTargetPad = target; repaint(); }
}

void PhysicalDrumEngineAudioProcessorEditor::clearDragTarget()
{
    if (dragTargetPad != -1) { dragTargetPad = -1; repaint(); }
}

bool PhysicalDrumEngineAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files) if (isSupportedSampleFile(f)) return true;
    return false;
}

void PhysicalDrumEngineAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    if (isInterestedInFileDrag(files)) updateDragTarget(x, y);
}

void PhysicalDrumEngineAudioProcessorEditor::fileDragMove(const juce::StringArray& files, int x, int y)
{
    if (isInterestedInFileDrag(files)) updateDragTarget(x, y);
}

void PhysicalDrumEngineAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    clearDragTarget();
}

void PhysicalDrumEngineAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    const int target = padAtPosition(x, y);
    if (target < 0 || files.isEmpty()) { clearDragTarget(); return; }
    const juce::File file(files[0]);
    if (isSupportedSampleFile(file.getFullPathName()) && processor.loadSampleForPad(target, file))
    {
        selectPad(target);
        refreshPadText();
        lcdStatus.setText("SAMPLE LOADED: " + file.getFileName(), juce::dontSendNotification);
    }
    clearDragTarget();
}
