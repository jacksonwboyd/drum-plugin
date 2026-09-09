#include "PluginEditor.h"

namespace
{
const juce::Colour bg          (0xff07111a);
const juce::Colour panel       (0xff0d1b28);
const juce::Colour panel2      (0xff101f2d);
const juce::Colour edge        (0xff6f8398);
const juce::Colour edgeBright  (0xffb9c9d8);
const juce::Colour green       (0xff49ff20);
const juce::Colour greenDim    (0xff1f9f19);
const juce::Colour text        (0xffe7edf2);
const juce::Colour muted       (0xffa7b3bd);
const juce::Colour black       (0xff020507);

class WinampLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    WinampLookAndFeel()
    {
        setColour(juce::TextButton::buttonColourId, panel2);
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff183c19));
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, green);
        setColour(juce::ComboBox::backgroundColourId, black);
        setColour(juce::ComboBox::outlineColourId, edge);
        setColour(juce::ComboBox::textColourId, green);
        setColour(juce::Slider::textBoxTextColourId, green);
        setColour(juce::Slider::textBoxBackgroundColourId, black);
        setColour(juce::Slider::textBoxOutlineColourId, edge);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(down ? juce::Colour(0xff172b3b) : (over ? juce::Colour(0xff1a3043) : panel2));
        g.fillRect(r);
        g.setColour(edgeBright);
        g.drawRect(r, 1.0f);
        g.setColour(juce::Colour(0xff283c50));
        g.drawRect(r.reduced(3.0f), 1.0f);
        if (b.getToggleState())
        {
            g.setColour(green);
            g.drawRect(r.reduced(1.5f), 2.0f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        auto f = juce::Font(juce::FontOptions{}.withHeight(std::min(15.0f, (float)b.getHeight() * 0.55f)).withStyle("Bold"));
        g.setFont(f);
        g.setColour(b.getToggleState() ? green : text);
        g.drawText(b.getButtonText(), b.getLocalBounds().reduced(3), juce::Justification::centred, true);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider& slider) override
    {
        const float size = (float)std::min(w, h) - 4.0f;
        const float cx = x + w * 0.5f;
        const float cy = y + h * 0.43f;
        const float radius = size * 0.38f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        g.setColour(juce::Colour(0xff02070b));
        g.fillEllipse(cx - radius - 4, cy - radius - 4, (radius + 4) * 2, (radius + 4) * 2);
        g.setColour(edge);
        g.drawEllipse(cx - radius - 3, cy - radius - 3, (radius + 3) * 2, (radius + 3) * 2, 1.0f);
        g.setColour(juce::Colour(0xff1c2c3b));
        g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        g.setColour(juce::Colour(0xff536678));
        g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.0f);

        juce::Path arc;
        arc.addCentredArc(cx, cy, radius + 2, radius + 2, 0.0f, startAngle, angle, true);
        g.setColour(green);
        g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path tick;
        tick.startNewSubPath(cx + std::cos(angle) * radius * 0.22f, cy + std::sin(angle) * radius * 0.22f);
        tick.lineTo(cx + std::cos(angle) * radius * 0.78f, cy + std::sin(angle) * radius * 0.78f);
        g.setColour(edgeBright);
        g.strokePath(tick, juce::PathStrokeType(2.0f));

        if (slider.isMouseOver())
        {
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.fillEllipse(cx - radius + 2, cy - radius + 2, (radius - 2) * 2, (radius - 2) * 2);
        }
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        g.setColour(black);
        g.fillRect(0, 0, w, h);
        g.setColour(isButtonDown ? green : edgeBright);
        g.drawRect(juce::Rectangle<float>(0.0f, 0.0f, (float) w, (float) h), 1.0f);
        g.setColour(edge);
        g.drawRect(juce::Rectangle<float>(3.0f, 3.0f, (float) (w - 6), (float) (h - 6)), 1.0f);
        g.setColour(green);
        juce::Path p;
        p.addTriangle((float)buttonX + 5, (float)buttonY + 7, (float)buttonX + buttonW - 5, (float)buttonY + 7, (float)buttonX + buttonW * 0.5f, (float)buttonY + buttonH - 6);
        g.fillPath(p);
    }
};

WinampLookAndFeel winampLaf;

juce::String valueForParameter(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* p = state.getParameter(id))
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
            return juce::String(ranged->getNormalisableRange().convertFrom0to1(p->getValue()), 3);
    return {};
}

void drawBevel(juce::Graphics& g, juce::Rectangle<int> r, bool bright = false)
{
    g.setColour(panel);
    g.fillRect(r);
    g.setColour(bright ? edgeBright : edge);
    g.drawRect(r, 1);
    g.setColour(juce::Colour(0xff23394d));
    g.drawRect(r.reduced(3), 1);
}

void drawSectionTitle(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    g.drawText(title, r.getX() + 10, r.getY() + 4, r.getWidth() - 20, 20, juce::Justification::left, false);
    g.setColour(edge);
    g.drawLine((float)r.getX() + 8, (float)r.getY() + 27, (float)r.getRight() - 8, (float)r.getY() + 27, 1.0f);
}
}

PhysicalDrumEngineAudioProcessorEditor::PhysicalDrumEngineAudioProcessorEditor(PhysicalDrumEngineAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1500, 900);
    setResizable(false, false);
    setLookAndFeel(&winampLaf);

    windowTitle.setText("Physical Drum Engine v1.8", juce::dontSendNotification);
    windowTitle.setVisible(false);
    addAndMakeVisible(windowTitle);

    menuBar.setText("File     Edit     Kits     Options     Help", juce::dontSendNotification);
    menuBar.setFont(juce::Font(juce::FontOptions{}.withHeight(18.0f).withStyle("Bold")));
    menuBar.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(menuBar);

    marquee.setText("SAMPLES FEEL BETTER HERE.", juce::dontSendNotification);
    marquee.setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f).withStyle("Bold")));
    marquee.setJustificationType(juce::Justification::centredRight);
    marquee.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(marquee);

    lcdTitle.setText("PHYSICAL DRUM ENGINE v1.8", juce::dontSendNotification);
    lcdTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(24.0f).withStyle("Bold")));
    lcdTitle.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(lcdTitle);

    lcdStatus.setText("LOAD. DISTORT. DEGRADE. PLAY.", juce::dontSendNotification);
    lcdStatus.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    lcdStatus.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(lcdStatus);

    setupButton(playButton, "PLAY", [this] { processor.setPaused(false); processor.triggerPadFromUI(selectedPad, (float) velocitySlider.getValue()); });
    setupButton(stopButton, "STOP", [this] { processor.stopAllVoices(); });
    setupButton(pauseButton, "PAUSE", [this] { processor.setPaused(!processor.isPaused()); pauseButton.setButtonText(processor.isPaused() ? "RESUME" : "PAUSE"); });
    setupButton(previousButton, "|<", [this] { selectPad((selectedPad + PhysicalDrumEngineAudioProcessor::numPads - 1) % PhysicalDrumEngineAudioProcessor::numPads); });
    setupButton(nextButton, ">|", [this] { selectPad((selectedPad + 1) % PhysicalDrumEngineAudioProcessor::numPads); });

    setupButton(newKitButton, "New Kit", [this] { processor.newKit(); selectPad(0); lcdStatus.setText("NEW KIT CREATED", juce::dontSendNotification); refreshPadText(); refreshSelectedPadControls(); });
    setupButton(saveKitButton, "Save Kit", [this] { saveKitToChooser(); });
    setupButton(loadKitButton, "Load Kit", [this] { loadKitFromChooser(); });
    setupButton(importButton, "Import...", [this] { importKit(); });

    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const auto& pad = processor.pads[(size_t)i];
        padLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        padLabels[(size_t)i].setColour(juce::Label::textColourId, muted);
        padLabels[(size_t)i].setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
        addAndMakeVisible(padLabels[(size_t)i]);
        setupButton(padButtons[(size_t)i], pad.name, [this, i] { selectPad(i, true); });
    }

    selectedPadInfo.setJustificationType(juce::Justification::centredLeft);
    selectedPadInfo.setColour(juce::Label::textColourId, green);
    selectedPadInfo.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    addAndMakeVisible(selectedPadInfo);

    setupButton(loadSampleButton, "LOAD", [this] { loadSelectedSample(); });
    setupButton(clearSampleButton, "CLEAR", [this] { clearSelectedSample(); });
    sampleName.setColour(juce::Label::textColourId, green);
    sampleName.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    addAndMakeVisible(sampleName);
    sampleInfo.setColour(juce::Label::textColourId, muted);
    sampleInfo.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f)));
    addAndMakeVisible(sampleInfo);

    const std::array<const char*, 4> sampleNames = {"TUNE", "START", "END", "LEVEL"};
    for (int i = 0; i < 4; ++i)
    {
        auto& s = sampleKnobs[(size_t)i];
        s.setLookAndFeel(&winampLaf);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 19);
        sampleKnobLabels[(size_t)i].setText(sampleNames[(size_t)i], juce::dontSendNotification);
        sampleKnobLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        sampleKnobLabels[(size_t)i].setColour(juce::Label::textColourId, muted);
        sampleKnobLabels[(size_t)i].setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
        addAndMakeVisible(sampleKnobLabels[(size_t)i]);
        addAndMakeVisible(s);
    }
    sampleKnobs[0].setRange(-1200.0, 1200.0, 1.0);
    sampleKnobs[0].onValueChange = [this] { processor.pads[(size_t)selectedPad].tuneCents = (float)sampleKnobs[0].getValue(); };
    sampleKnobs[1].setRange(0.0, 0.999, 0.001);
    sampleKnobs[1].onValueChange = [this] { auto& pad = processor.pads[(size_t)selectedPad]; pad.startNorm = juce::jlimit(0.0f, 0.999f, (float)sampleKnobs[1].getValue()); if (pad.endNorm <= pad.startNorm) pad.endNorm = juce::jmin(1.0f, pad.startNorm + 0.001f); };
    sampleKnobs[2].setRange(0.001, 1.0, 0.001);
    sampleKnobs[2].onValueChange = [this] { auto& pad = processor.pads[(size_t)selectedPad]; pad.endNorm = juce::jmax(pad.startNorm + 0.001f, (float)sampleKnobs[2].getValue()); };
    sampleKnobs[3].setRange(0.0, 2.0, 0.001);
    sampleKnobs[3].onValueChange = [this] { processor.pads[(size_t)selectedPad].level = (float)sampleKnobs[3].getValue(); };

    const std::array<const char*, 13> names = {"PHYSICALITY", "TRANSIENT", "ATTACK", "BRIGHTNESS", "PITCH", "BODY", "DECAY", "TIMING", "VARIATION", "SAMPLE RATE", "SUSTAIN", "RELEASE", "MIX"};
    for (int i = 0; i < 13; ++i)
        setupKnob(globalKnobs[(size_t)i], globalKnobLabels[(size_t)i], globalKnobIds[(size_t)i], names[(size_t)i]);

    velocitySlider.setLookAndFeel(&winampLaf);
    velocitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    velocitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
    velocitySlider.setRange(0.0, 1.0, 0.001);
    velocitySlider.setValue(1.0);
    addAndMakeVisible(velocitySlider);

    volumeSlider.setLookAndFeel(&winampLaf);
    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
    volumeSlider.setRange(-18.0, 6.0, 0.1);
    volumeSlider.setValue(processor.apvts.getRawParameterValue("output")->load());
    volumeSlider.onValueChange = [this] { if (auto* p = processor.apvts.getParameter("output")) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(p)) r->setValueNotifyingHost(r->getNormalisableRange().convertTo0to1((float)volumeSlider.getValue())); };
    addAndMakeVisible(volumeSlider);

    ceilingSlider.setLookAndFeel(&winampLaf);
    ceilingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ceilingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
    ceilingSlider.setRange(-12.0, 0.0, 0.1);
    ceilingSlider.setValue(processor.apvts.getRawParameterValue("ceiling")->load());
    ceilingSlider.onValueChange = [this] { if (auto* p = processor.apvts.getParameter("ceiling")) if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(p)) r->setValueNotifyingHost(r->getNormalisableRange().convertTo0to1((float)ceilingSlider.getValue())); };
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

PhysicalDrumEngineAudioProcessorEditor::~PhysicalDrumEngineAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    for (auto& s : sampleKnobs) s.setLookAndFeel(nullptr);
    for (auto& s : globalKnobs) s.setLookAndFeel(nullptr);
    velocitySlider.setLookAndFeel(nullptr);
    volumeSlider.setLookAndFeel(nullptr);
    ceilingSlider.setLookAndFeel(nullptr);
}

void PhysicalDrumEngineAudioProcessorEditor::setupButton(juce::TextButton& button, const juce::String& textValue, std::function<void()> action)
{
    button.setLookAndFeel(&winampLaf);
    button.setButtonText(textValue);
    button.onClick = std::move(action);
    addAndMakeVisible(button);
}

void PhysicalDrumEngineAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& id, const juce::String& name)
{
    slider.setLookAndFeel(&winampLaf);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 19);
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
                            ranged2->setValueNotifyingHost(ranged2->getNormalisableRange().convertTo0to1((float)slider.getValue()));
                };
            }
    }
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, muted);
    label.setFont(juce::Font(juce::FontOptions{}.withHeight(8.0f).withStyle("Bold").withHorizontalScale(0.62f)));
    label.setMinimumHorizontalScale(0.55f);
    addAndMakeVisible(label);
    addAndMakeVisible(slider);
}

void PhysicalDrumEngineAudioProcessorEditor::selectPad(int index, bool audition)
{
    selectedPad = juce::jlimit(0, PhysicalDrumEngineAudioProcessor::numPads - 1, index);
    refreshSelectedPadControls();
    repaint();
    if (audition) processor.triggerPadFromUI(selectedPad, (float)velocitySlider.getValue());
}

void PhysicalDrumEngineAudioProcessorEditor::refreshPadText()
{
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const auto& pad = processor.pads[(size_t)i];
        padButtons[(size_t)i].setButtonText(pad.name);
        padLabels[(size_t)i].setText(juce::String::formatted("%s   %d", pad.sampleFile.existsAsFile() ? pad.sampleFile.getFileNameWithoutExtension().toRawUTF8() : "EMPTY", pad.midiNote), juce::dontSendNotification);
    }
}

void PhysicalDrumEngineAudioProcessorEditor::refreshSelectedPadControls()
{
    const auto& pad = processor.pads[(size_t)selectedPad];
    selectedPadInfo.setText("Selected: " + pad.name + "   MIDI: " + juce::String(pad.midiNote), juce::dontSendNotification);
    sampleName.setText(pad.sampleFile.existsAsFile() ? pad.sampleFile.getFileName() : "NO SAMPLE LOADED", juce::dontSendNotification);
    if (pad.sample)
        sampleInfo.setText(juce::String((double)pad.sampleRate, 1) + " kHz    " + juce::String(pad.sample->getNumSamples()) + " samples", juce::dontSendNotification);
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
    processor.pads[(size_t)selectedPad].sample.reset();
    processor.pads[(size_t)selectedPad].sampleFile = juce::File();
    processor.stopAllVoices();
    refreshSelectedPadControls();
    refreshPadText();
    lcdStatus.setText("SAMPLE CLEARED", juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::saveKitToChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save Drum Kit", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MyKit.pdk"), "*.pdk");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& c)
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
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& c)
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

void PhysicalDrumEngineAudioProcessorEditor::importKit() { loadKitFromChooser(); }

void PhysicalDrumEngineAudioProcessorEditor::applyPreset(int index)
{
    const std::array<float, 13> values = [&]()
    {
        switch (index)
        {
            case 2: return std::array<float, 13>{0.90f,0.55f,0.25f,0.35f,0.45f,1.05f,0.70f,0.35f,0.45f,18000.0f,0.95f,0.18f,1.0f};
            case 3: return std::array<float, 13>{0.95f,0.85f,0.20f,0.20f,0.65f,1.10f,0.65f,0.40f,0.70f,8000.0f,0.90f,0.30f,1.0f};
            case 4: return std::array<float, 13>{0.75f,0.60f,0.25f,0.45f,0.35f,1.00f,0.55f,0.25f,0.35f,22000.0f,1.0f,0.15f,0.95f};
            case 5: return std::array<float, 13>{1.00f,0.80f,0.15f,0.30f,0.55f,1.20f,0.80f,0.30f,0.55f,12000.0f,0.95f,0.35f,1.0f};
            case 6: return std::array<float, 13>{0.70f,0.90f,0.15f,0.65f,0.50f,0.95f,0.50f,0.20f,0.25f,30000.0f,1.0f,0.10f,1.0f};
            case 7: return std::array<float, 13>{0.65f,0.80f,0.20f,0.50f,0.35f,1.00f,0.40f,0.10f,0.20f,6000.0f,0.88f,0.28f,0.90f};
            case 8: return std::array<float, 13>{1.00f,1.00f,0.10f,0.15f,0.80f,1.30f,0.90f,0.45f,0.80f,3500.0f,0.82f,0.40f,1.0f};
            default: return std::array<float, 13>{0.75f,0.70f,0.35f,0.55f,0.35f,1.00f,0.55f,0.20f,0.30f,44100.0f,1.0f,0.15f,1.0f};
        }
    }();
    for (int i = 0; i < 13; ++i)
    {
        if (auto* p = processor.apvts.getParameter(globalKnobIds[(size_t)i]))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                ranged->setValueNotifyingHost(ranged->getNormalisableRange().convertTo0to1(values[(size_t)i]));
        globalKnobs[(size_t)i].setValue(values[(size_t)i], juce::dontSendNotification);
    }
    lcdStatus.setText("PRESET LOADED: " + presetBrowser.getText(), juce::dontSendNotification);
}

void PhysicalDrumEngineAudioProcessorEditor::applyMidiMap(int index)
{
    const int gm[PhysicalDrumEngineAudioProcessor::numPads] = {36,38,42,46,45,43,41,49,51,39,37,40};
    const int classic[PhysicalDrumEngineAudioProcessor::numPads] = {36,37,38,39,40,41,42,43,44,45,46,47};
    const auto* map = index == 2 ? classic : gm;
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i) processor.pads[(size_t)i].midiNote = map[i];
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
    if (auto* p = processor.apvts.getRawParameterValue("limiter"))
    {
        const bool enabled = p->load() >= 0.5f;
        if (enabled != limiterOn)
        {
            limiterOn = enabled;
            limiterButton.setButtonText(enabled ? "ON" : "OFF");
        }
    }
    repaint();
    refreshPadText();
    refreshSelectedPadControls();
}

void PhysicalDrumEngineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff02060a));
    auto outer = getLocalBounds().reduced(8);

    // Winamp-style metal shell
    // JUCE 8-compatible metal shell: layered fills instead of ColourGradient constructor.
    g.setColour(juce::Colour(0xff142331));
    g.fillRect(outer);
    g.setColour(juce::Colour(0xff08131d));
    g.fillRect(outer.reduced(3));
    g.setColour(edgeBright); g.drawRect(outer, 2);
    g.setColour(juce::Colour(0xff24394d)); g.drawRect(outer.reduced(4), 2);

    auto header = outer.removeFromTop(70);
    g.setColour(juce::Colour(0xff08131d)); g.fillRect(header);
    g.setColour(edgeBright); g.drawRect(header, 1);

    // Lightning logo / WINAMP wordmark
    juce::Path bolt;
    bolt.startNewSubPath((float)header.getX()+18, (float)header.getY()+12);
    bolt.lineTo((float)header.getX()+39, (float)header.getY()+8);
    bolt.lineTo((float)header.getX()+30, (float)header.getY()+27);
    bolt.lineTo((float)header.getX()+47, (float)header.getY()+25);
    bolt.lineTo((float)header.getX()+20, (float)header.getY()+57);
    bolt.lineTo((float)header.getX()+27, (float)header.getY()+34);
    bolt.lineTo((float)header.getX()+12, (float)header.getY()+36);
    bolt.closeSubPath();
    g.setColour(juce::Colours::white); g.fillPath(bolt);
    g.setColour(juce::Colour(0xffffc20a)); g.strokePath(bolt, juce::PathStrokeType(2.0f));
    g.setColour(text); g.setFont(juce::Font(juce::FontOptions{}.withHeight(24.0f).withStyle("Bold")));
    g.drawText("WINAMP", header.getX()+55, header.getY()+12, 160, 28, juce::Justification::left);
    g.setColour(green); g.setFont(juce::Font(juce::FontOptions{}.withHeight(13.0f).withStyle("Bold")));
    g.drawText("PHYSICAL DRUM ENGINE", header.getX()+235, header.getY()+13, 290, 22, juce::Justification::left);
    g.setColour(green); g.drawText("SAMPLES FEEL BETTER HERE.", header.getRight()-330, header.getY()+13, 310, 22, juce::Justification::right);

    // Menu strip
    auto menu = outer.removeFromTop(34);
    g.setColour(juce::Colour(0xff101d2a)); g.fillRect(menu);
    g.setColour(edge); g.drawRect(menu, 1);

    auto body = outer.reduced(10);
    const int leftW = 285;
    const int centerW = 735;
    const int rightW = body.getWidth() - leftW - centerW - 20;
    auto left = body.removeFromLeft(leftW);
    body.removeFromLeft(10);
    auto center = body.removeFromLeft(centerW);
    body.removeFromLeft(10);
    auto right = body;

    // Player / transport
    auto player = center.removeFromTop(116);
    drawBevel(g, player, true);
    g.setColour(black); g.fillRect(player.reduced(10));
    g.setColour(green); g.setFont(juce::Font(juce::FontOptions{}.withHeight(27.0f).withStyle("Bold")));
    g.drawText("00:00", player.getX()+18, player.getY()+16, 125, 34, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText("128 BPM", player.getX()+19, player.getY()+51, 70, 16, juce::Justification::left);
    // fake-but-reactive spectrum
    const float peak = juce::jlimit(0.0f, 1.0f, std::max(processor.getLeftPeak(), processor.getRightPeak()));
    for (int i=0;i<22;++i)
    {
        const float wave = 0.25f + 0.75f * std::abs(std::sin((float)i*0.8f + peak*10.0f));
        const int bh = 8 + (int)(wave * 22.0f * peak);
        g.setColour(green);
        g.fillRect(player.getX()+18+i*6, player.getBottom()-16-bh, 4, bh);
    }

    auto tx = player.getX()+175;
    auto tw = player.getWidth()-195;
    const int bw = 72;
    int bx = tx;
    for (auto* b : {&previousButton,&playButton,&stopButton,&pauseButton,&nextButton})
    {
        b->setBounds(bx, player.getY()+15, bw, 32);
        bx += bw + 6;
    }
    lcdTitle.setBounds(tx, player.getY()+55, tw, 26);
    lcdStatus.setBounds(tx, player.getY()+80, tw, 20);

    // Pads
    auto pads = center.removeFromTop(440);
    drawBevel(g, pads);
    drawSectionTitle(g, pads, "DRUM PADS");
    auto padArea = pads.reduced(12); padArea.removeFromTop(28);
    const int gap=8;
    const int cw=(padArea.getWidth()-gap*3)/4;
    const int ch=(padArea.getHeight()-gap*2)/3;
    for (int i=0;i<PhysicalDrumEngineAudioProcessor::numPads;++i)
    {
        const int row=i/4, col=i%4;
        auto r=juce::Rectangle<int>(padArea.getX()+col*(cw+gap),padArea.getY()+row*(ch+gap),cw,ch);
        padAreas[(size_t)i]=r;
        padButtons[(size_t)i].setBounds(r);
        padLabels[(size_t)i].setBounds(r.getX()+5,r.getBottom()-23,r.getWidth()-10,17);
        g.setColour(i==selectedPad ? juce::Colour(0xff143c19) : juce::Colour(0xff0c1824));
        g.fillRect(r.reduced(2));
        g.setColour(i==dragTargetPad || i==selectedPad ? green : edge);
        g.drawRect(r.reduced(1), i==selectedPad ? 2.0f : 1.0f);
        g.setColour(edgeBright.withAlpha(0.35f));
        g.drawLine((float)r.getX()+5,(float)r.getY()+5,(float)r.getRight()-5,(float)r.getY()+5,1.0f);
        g.setColour(muted); g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
        g.drawText(juce::String(i+1), r.getRight()-19, r.getBottom()-18, 12, 12, juce::Justification::right);
    }

    // Selected-pad status strip beneath the pad matrix.
    auto statusStrip = juce::Rectangle<int>(center.getX(), center.getBottom()-64, center.getWidth(), 54);
    drawBevel(g, statusStrip);
    selectedPadInfo.setBounds(statusStrip.getX()+12, statusStrip.getY()+14, statusStrip.getWidth()-24, 24);

    // Left kits + controls
    auto kitList=left.removeFromTop(430); drawBevel(g,kitList); drawSectionTitle(g,kitList,"KITS");
    presetBrowser.setBounds(kitList.getX()+12,kitList.getY()+38,kitList.getWidth()-24,kitList.getHeight()-154);
    newKitButton.setBounds(kitList.getX()+12,kitList.getBottom()-104,122,30);
    saveKitButton.setBounds(kitList.getX()+145,kitList.getBottom()-104,122,30);
    loadKitButton.setBounds(kitList.getX()+12,kitList.getBottom()-64,122,30);
    importButton.setBounds(kitList.getX()+145,kitList.getBottom()-64,122,30);

    // The reference puts the full-width effect strip under the pads.
    auto effectsWide = juce::Rectangle<int>(body.getX() - centerW - 10 - leftW, center.getBottom() - 150, leftW + 10 + centerW, 150);
    drawBevel(g, effectsWide); drawSectionTitle(g, effectsWide, "GLOBAL EFFECTS");
    const int gw = effectsWide.getWidth() / 13;
    for (int i = 0; i < 13; ++i)
    {
        const int x = effectsWide.getX() + i * gw;
        globalKnobLabels[(size_t)i].setBounds(x + 2, effectsWide.getY() + 30, gw - 4, 24);
        globalKnobs[(size_t)i].setBounds(x + 3, effectsWide.getY() + 51, gw - 6, 82);
    }

    auto midi = left.removeFromTop(std::max(120, left.getHeight()));
    drawBevel(g, midi); drawSectionTitle(g, midi, "MIDI");
    midiMapBox.setBounds(midi.getX()+12,midi.getY()+38,midi.getWidth()-24,30);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withStyle("Bold")));
    g.drawText("VELOCITY", midi.getX()+12, midi.getY()+78, 70, 16, juce::Justification::left);
    velocitySlider.setBounds(midi.getRight()-100,midi.getY()+55,80,70);
    volumeSlider.setBounds(midi.getX()+12,midi.getY()+112,1,1);
    ceilingSlider.setBounds(midi.getX()+12,midi.getY()+112,1,1);

    // Right sample panel
    auto sample=right.removeFromTop(420); drawBevel(g,sample,true); drawSectionTitle(g,sample,"SAMPLE");
    sampleName.setBounds(sample.getX()+14,sample.getY()+36,sample.getWidth()-120,24);
    sampleInfo.setBounds(sample.getX()+14,sample.getY()+61,sample.getWidth()-120,20);
    loadSampleButton.setBounds(sample.getRight()-96,sample.getY()+36,82,30);
    clearSampleButton.setBounds(sample.getRight()-96,sample.getY()+72,82,30);
    auto wave=sample.reduced(14); wave.removeFromTop(95); wave.removeFromBottom(112);
    g.setColour(black); g.fillRect(wave);
    if(auto* s=processor.pads[(size_t)selectedPad].sample.get())
    {
        const auto* data=s->getReadPointer(0); const int n=s->getNumSamples();
        juce::Path pth;
        for(int x=0;x<wave.getWidth();++x){int idx=juce::jlimit(0,n-1,(int)((double)x/wave.getWidth()*n)); float yy=wave.getCentreY()-data[idx]*wave.getHeight()*0.45f; if(x==0) pth.startNewSubPath((float)wave.getX()+x,yy); else pth.lineTo((float)wave.getX()+x,yy);}
        g.setColour(green); g.strokePath(pth,juce::PathStrokeType(1.2f));
    }
    for(int i=0;i<4;++i)
    {
        const int x=sample.getX()+10+i*(sample.getWidth()-20)/4;
        sampleKnobLabels[(size_t)i].setBounds(x,sample.getBottom()-98,(sample.getWidth()-20)/4-6,18);
        sampleKnobs[(size_t)i].setBounds(x,sample.getBottom()-80,(sample.getWidth()-20)/4-6,76);
    }

    // Right output meters / limiter
    auto out=right; drawBevel(g,out); drawSectionTitle(g,out,"OUTPUT");
    const float l=juce::jlimit(0.0f,1.0f,processor.getLeftPeak());
    const float r=juce::jlimit(0.0f,1.0f,processor.getRightPeak());
    auto meterL=juce::Rectangle<int>(out.getX()+18,out.getY()+48,42,out.getHeight()-76);
    auto meterR=juce::Rectangle<int>(out.getX()+68,out.getY()+48,42,out.getHeight()-76);
    for(auto mr:{meterL,meterR}) { g.setColour(black); g.fillRect(mr); g.setColour(edge); g.drawRect(mr,1); }
    g.setColour(green); g.fillRect(meterL.withTop(meterL.getBottom()-(int)(meterL.getHeight()*l)));
    g.setColour(green); g.fillRect(meterR.withTop(meterR.getBottom()-(int)(meterR.getHeight()*r)));
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    g.drawText("L",meterL.getX(),meterL.getBottom()+6,meterL.getWidth(),14,juce::Justification::centred);
    g.drawText("R",meterR.getX(),meterR.getBottom()+6,meterR.getWidth(),14,juce::Justification::centred);
    limiterButton.setBounds(out.getRight()-92,out.getY()+48,72,30);
    ceilingSlider.setBounds(out.getRight()-102,out.getY()+90,92,78);
    g.drawText("LIMITER",out.getRight()-102,out.getY()+170,92,16,juce::Justification::centred);
}

void PhysicalDrumEngineAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& titleText)
{
    drawBevel(g,r); drawSectionTitle(g,r,titleText);
}

void PhysicalDrumEngineAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float value, const juce::String& label)
{
    g.setColour(black); g.fillRect(r);
    g.setColour(green); g.fillRect(r.withWidth((int)(r.getWidth()*juce::jlimit(0.0f,1.0f,value))));
    g.setColour(text); g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText(label,r.getX()-14,r.getY(),12,r.getHeight(),juce::Justification::centred);
}

void PhysicalDrumEngineAudioProcessorEditor::resized()
{
    // Component positions are finalized in paint so the skin remains pixel-aligned.
}

bool PhysicalDrumEngineAudioProcessorEditor::isSupportedSampleFile(const juce::String& path) const
{
    const auto ext=juce::File(path).getFileExtension().toLowerCase();
    return ext==".wav" || ext==".aif" || ext==".aiff" || ext==".flac" || ext==".ogg";
}

int PhysicalDrumEngineAudioProcessorEditor::padAtPosition(int x,int y) const
{
    for(int i=0;i<PhysicalDrumEngineAudioProcessor::numPads;++i) if(padAreas[(size_t)i].contains(x,y)) return i;
    return -1;
}

void PhysicalDrumEngineAudioProcessorEditor::updateDragTarget(int x,int y)
{
    const int target=padAtPosition(x,y);
    if(target!=dragTargetPad){dragTargetPad=target;repaint();}
}
void PhysicalDrumEngineAudioProcessorEditor::clearDragTarget(){if(dragTargetPad!=-1){dragTargetPad=-1;repaint();}}
bool PhysicalDrumEngineAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files){for(const auto& f:files) if(isSupportedSampleFile(f)) return true; return false;}
void PhysicalDrumEngineAudioProcessorEditor::fileDragEnter(const juce::StringArray& files,int x,int y){if(isInterestedInFileDrag(files)) updateDragTarget(x,y);}
void PhysicalDrumEngineAudioProcessorEditor::fileDragMove(const juce::StringArray& files,int x,int y){if(isInterestedInFileDrag(files)) updateDragTarget(x,y);}
void PhysicalDrumEngineAudioProcessorEditor::fileDragExit(const juce::StringArray&){clearDragTarget();}
void PhysicalDrumEngineAudioProcessorEditor::filesDropped(const juce::StringArray& files,int x,int y)
{
    const int target=padAtPosition(x,y);
    if(target<0 || files.isEmpty()){clearDragTarget();return;}
    const juce::File file(files[0]);
    if(isSupportedSampleFile(file.getFullPathName()) && processor.loadSampleForPad(target,file))
    {
        selectPad(target); refreshPadText(); lcdStatus.setText("SAMPLE LOADED: "+file.getFileName(),juce::dontSendNotification);
    }
    clearDragTarget();
}
