#include "PluginEditor.h"

namespace
{
const juce::Colour shellDark       (0xff080d12);
const juce::Colour shellMid        (0xff18232c);
const juce::Colour shellLight      (0xff3c4a55);
const juce::Colour shellEdge       (0xff788691);
const juce::Colour bevelDark       (0xff10171d);
const juce::Colour panelBlack      (0xff020608);
const juce::Colour panelBlue       (0xff0a1219);
const juce::Colour panelBlue2      (0xff111c25);
const juce::Colour green           (0xff72ff38);
const juce::Colour greenDark       (0xff1f9b20);
const juce::Colour amber           (0xffffc400);
const juce::Colour text            (0xffdbe4e9);
const juce::Colour muted           (0xff8997a0);
const juce::Colour black           (0xff010203);

class WinampLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    WinampLookAndFeel()
    {
        setColour(juce::TextButton::buttonColourId, panelBlue2);
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1d401e));
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, green);
        setColour(juce::ComboBox::backgroundColourId, black);
        setColour(juce::ComboBox::outlineColourId, shellEdge);
        setColour(juce::ComboBox::textColourId, green);
        setColour(juce::Slider::textBoxTextColourId, green);
        setColour(juce::Slider::textBoxBackgroundColourId, black);
        setColour(juce::Slider::textBoxOutlineColourId, shellEdge);
    }

    static juce::Font pixelFont(float height, bool bold = false)
    {
        auto options = juce::FontOptions{}.withHeight(height).withTypefaceName("DejaVu Sans Mono");
        if (bold) options = options.withStyle("Bold");
        return juce::Font(options);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        const bool on = b.getToggleState();
        g.setColour(down ? juce::Colour(0xff070c10) : (on ? juce::Colour(0xff153b18) : (over ? juce::Colour(0xff253640) : juce::Colour(0xff17232c))));
        g.fillRect(r);

        // Hard-edged Winamp bevel: bright upper/left, dark lower/right.
        g.setColour(down ? shellLight : shellEdge);
        g.drawLine(r.getX(), r.getY(), r.getRight(), r.getY(), 1.0f);
        g.drawLine(r.getX(), r.getY(), r.getX(), r.getBottom(), 1.0f);
        g.setColour(black);
        g.drawLine(r.getX(), r.getBottom(), r.getRight(), r.getBottom(), 1.0f);
        g.drawLine(r.getRight(), r.getY(), r.getRight(), r.getBottom(), 1.0f);
        g.setColour(juce::Colour(0xff2d3d48));
        g.drawRect(r.reduced(2.0f), 1.0f);
        if (on || down)
        {
            g.setColour(green);
            g.drawRect(r.reduced(2.0f), 1.0f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont(pixelFont(std::min(12.0f, (float)b.getHeight() * 0.48f), true));
        g.setColour(b.getToggleState() ? green : text);
        g.drawText(b.getButtonText(), b.getLocalBounds().reduced(2), juce::Justification::centred, true);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider& slider) override
    {
        const float size = (float)std::min(w, h) - 5.0f;
        const float cx = (float)x + (float)w * 0.5f;
        const float cy = (float)y + (float)h * 0.42f;
        const float radius = size * 0.36f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        g.setColour(black);
        g.fillEllipse(cx - radius - 5.0f, cy - radius - 5.0f, (radius + 5.0f) * 2.0f, (radius + 5.0f) * 2.0f);
        g.setColour(shellEdge);
        g.drawEllipse(cx - radius - 4.0f, cy - radius - 4.0f, (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f, 1.0f);
        g.setColour(juce::Colour(0xff293740));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colour(0xff65747e));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);
        g.setColour(juce::Colour(0xff111a20));
        g.fillEllipse(cx - radius + 3.0f, cy - radius + 3.0f, (radius - 3.0f) * 2.0f, (radius - 3.0f) * 2.0f);

        juce::Path arc;
        arc.addCentredArc(cx, cy, radius + 2.0f, radius + 2.0f, 0.0f, startAngle, angle, true);
        g.setColour(greenDark);
        g.strokePath(arc, juce::PathStrokeType(2.0f));

        juce::Path tick;
        tick.startNewSubPath(cx + std::cos(angle) * radius * 0.20f, cy + std::sin(angle) * radius * 0.20f);
        tick.lineTo(cx + std::cos(angle) * radius * 0.80f, cy + std::sin(angle) * radius * 0.80f);
        g.setColour(green);
        g.strokePath(tick, juce::PathStrokeType(2.0f));

        if (slider.isMouseOver())
        {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.fillEllipse(cx - radius + 3.0f, cy - radius + 3.0f, (radius - 3.0f) * 2.0f, (radius - 3.0f) * 2.0f);
        }
    }

    void drawComboBox(juce::Graphics& g, int w, int h, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        juce::ignoreUnused(box);
        g.setColour(black);
        g.fillRect(0, 0, w, h);
        g.setColour(isButtonDown ? green : shellEdge);
        g.drawRect(juce::Rectangle<float>(0.0f, 0.0f, (float)w, (float)h), 1.0f);
        g.setColour(juce::Colour(0xff283843));
        g.drawRect(juce::Rectangle<float>(2.0f, 2.0f, (float)std::max(0, w - 4), (float)std::max(0, h - 4)), 1.0f);
        g.setColour(green);
        juce::Path p;
        p.addTriangle((float)buttonX + 5.0f, (float)buttonY + 7.0f,
                      (float)buttonX + (float)buttonW - 5.0f, (float)buttonY + 7.0f,
                      (float)buttonX + (float)buttonW * 0.5f, (float)buttonY + (float)buttonH - 6.0f);
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
    const auto rf = r.toFloat();
    g.setColour(shellMid);
    g.fillRect(rf);
    g.setColour(bright ? shellEdge : shellLight);
    g.drawRect(rf, 1.0f);
    g.setColour(black);
    g.drawLine((float)r.getX() + 1.0f, (float)r.getBottom() - 2.0f, (float)r.getRight() - 1.0f, (float)r.getBottom() - 2.0f, 2.0f);
    g.drawLine((float)r.getRight() - 2.0f, (float)r.getY() + 1.0f, (float)r.getRight() - 2.0f, (float)r.getBottom() - 1.0f, 2.0f);
    g.setColour(juce::Colour(0xff273640));
    g.drawRect(r.reduced(3).toFloat(), 1.0f);
}

void drawSectionTitle(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour(juce::Colour(0xff1a2730));
    g.fillRect(r.getX() + 2, r.getY() + 2, r.getWidth() - 4, 27);
    g.setColour(shellEdge);
    g.drawLine((float)r.getX() + 5.0f, (float)r.getY() + 28.0f, (float)r.getRight() - 5.0f, (float)r.getY() + 28.0f, 1.0f);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText(title.toUpperCase(), r.getX() + 9, r.getY() + 5, r.getWidth() - 18, 18, juce::Justification::left, false);
}

void drawScrew(juce::Graphics& g, float x, float y)
{
    g.setColour(juce::Colour(0xff71808a));
    g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    g.setColour(black);
    g.drawLine(x - 2.0f, y - 2.0f, x + 2.0f, y + 2.0f, 1.0f);
}
}

PhysicalDrumEngineAudioProcessorEditor::PhysicalDrumEngineAudioProcessorEditor(PhysicalDrumEngineAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1500, 900);
    setResizable(false, false);
    setLookAndFeel(&winampLaf);

    windowTitle.setText("Physical Drum Engine v2.1", juce::dontSendNotification);
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

    lcdTitle.setText("PHYSICAL DRUM ENGINE v2.1", juce::dontSendNotification);
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

    const std::array<const char*, 5> names = {"SAMPLE RATE", "OUTPUT", "FILTER", "ATTACK", "RELEASE"};
    for (int i = 0; i < 5; ++i)
        setupKnob(globalKnobs[(size_t)i], globalKnobLabels[(size_t)i], globalKnobIds[(size_t)i], names[(size_t)i]);

    velocitySlider.setLookAndFeel(&winampLaf);
    velocitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    velocitySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
    velocitySlider.setRange(0.0, 1.0, 0.001);
    velocitySlider.setValue(1.0);
    addAndMakeVisible(velocitySlider);

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
    const std::array<float, 5> values = [&]()
    {
        switch (index)
        {
            case 2: return std::array<float, 5>{22050.0f, -1.0f, 12000.0f, 0.00f, 0.18f};
            case 3: return std::array<float, 5>{11025.0f, -2.0f, 8000.0f, 0.02f, 0.30f};
            case 4: return std::array<float, 5>{44100.0f, 0.0f, 16000.0f, 0.00f, 0.08f};
            case 5: return std::array<float, 5>{16000.0f, -1.5f, 10000.0f, 0.01f, 0.25f};
            case 6: return std::array<float, 5>{30000.0f, 0.0f, 18000.0f, 0.00f, 0.05f};
            case 7: return std::array<float, 5>{8000.0f, -3.0f, 6500.0f, 0.01f, 0.22f};
            case 8: return std::array<float, 5>{3500.0f, -2.5f, 4500.0f, 0.03f, 0.35f};
            default: return std::array<float, 5>{44100.0f, 0.0f, 20000.0f, 0.0f, 0.0f};
        }
    }();
    for (int i = 0; i < 5; ++i)
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


void PhysicalDrumEngineAudioProcessorEditor::timerCallback()
{
    repaint();
    refreshPadText();
    refreshSelectedPadControls();
}

void PhysicalDrumEngineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);
    auto outer = getLocalBounds().reduced(7);

    // === WINAMP METAL SHELL ===
    g.setColour(shellDark); g.fillRect(outer);
    g.setColour(shellLight); g.drawRect(outer.toFloat(), 2.0f);
    g.setColour(black); g.drawRect(outer.reduced(4).toFloat(), 2.0f);
    g.setColour(juce::Colour(0xff26343e)); g.drawRect(outer.reduced(7).toFloat(), 1.0f);
    drawScrew(g, (float)outer.getX() + 8.0f, (float)outer.getY() + 8.0f);
    drawScrew(g, (float)outer.getRight() - 8.0f, (float)outer.getY() + 8.0f);
    drawScrew(g, (float)outer.getX() + 8.0f, (float)outer.getBottom() - 8.0f);
    drawScrew(g, (float)outer.getRight() - 8.0f, (float)outer.getBottom() - 8.0f);

    // Header: compact early-2000s media-player chrome.
    auto header = outer.removeFromTop(58);
    g.setColour(juce::Colour(0xff1b2832)); g.fillRect(header);
    g.setColour(shellEdge); g.drawRect(header.toFloat(), 1.0f);
    g.setColour(black); g.drawLine((float)header.getX(), (float)header.getBottom()-2.0f, (float)header.getRight(), (float)header.getBottom()-2.0f, 2.0f);

    juce::Path bolt;
    bolt.startNewSubPath((float)header.getX()+18.0f, (float)header.getY()+8.0f);
    bolt.lineTo((float)header.getX()+39.0f, (float)header.getY()+5.0f);
    bolt.lineTo((float)header.getX()+30.0f, (float)header.getY()+23.0f);
    bolt.lineTo((float)header.getX()+47.0f, (float)header.getY()+21.0f);
    bolt.lineTo((float)header.getX()+19.0f, (float)header.getY()+51.0f);
    bolt.lineTo((float)header.getX()+26.0f, (float)header.getY()+30.0f);
    bolt.lineTo((float)header.getX()+11.0f, (float)header.getY()+32.0f);
    bolt.closeSubPath();
    g.setColour(juce::Colours::white); g.fillPath(bolt);
    g.setColour(amber); g.strokePath(bolt, juce::PathStrokeType(1.5f));

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(21.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("WINAMP", header.getX()+57, header.getY()+8, 150, 24, juce::Justification::left);
    g.setColour(green);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("PHYSICAL DRUM ENGINE", header.getX()+205, header.getY()+10, 280, 18, juce::Justification::left);
    g.drawText("[ DIGITAL AUDIO // 12 PAD ]", header.getX()+205, header.getY()+29, 280, 16, juce::Justification::left);
    g.setColour(green); g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("SAMPLES FEEL BETTER HERE.", header.getRight()-255, header.getY()+13, 235, 18, juce::Justification::right);

    // Menu strip (cosmetic only, as in the reference skin).
    auto menu = outer.removeFromTop(26);
    g.setColour(juce::Colour(0xff0b1116)); g.fillRect(menu);
    g.setColour(shellEdge); g.drawRect(menu.toFloat(), 1.0f);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withTypefaceName("DejaVu Sans Mono")));
    g.drawText("File    Edit    Kits    Options    Help", menu.getX()+10, menu.getY()+5, 270, 15, juce::Justification::left);
    g.setColour(green);
    g.drawText("READY", menu.getRight()-75, menu.getY()+5, 62, 15, juce::Justification::right);

    auto body = outer.reduced(9);
    const int leftW = 270;
    const int centerW = 740;
    const int rightW = body.getWidth() - leftW - centerW - 20;
    juce::ignoreUnused(rightW);
    auto left = body.removeFromLeft(leftW);
    body.removeFromLeft(10);
    auto center = body.removeFromLeft(centerW);
    body.removeFromLeft(10);
    auto right = body;

    // === PLAYER ===
    auto player = center.removeFromTop(108);
    drawBevel(g, player, true);
    auto lcd = player.reduced(9);
    g.setColour(panelBlack); g.fillRect(lcd);
    g.setColour(juce::Colour(0xff0c1a10)); g.drawRect(lcd.toFloat(), 1.0f);
    g.setColour(green);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(25.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("00:00", lcd.getX()+12, lcd.getY()+8, 115, 29, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withTypefaceName("DejaVu Sans Mono")));
    g.drawText("TRACK 01", lcd.getX()+13, lcd.getY()+39, 90, 13, juce::Justification::left);
    g.drawText("128 BPM", lcd.getX()+13, lcd.getY()+53, 90, 13, juce::Justification::left);

    const float peak = juce::jlimit(0.0f, 1.0f, std::max(processor.getLeftPeak(), processor.getRightPeak()));
    for (int i = 0; i < 32; ++i)
    {
        const float wave = 0.18f + 0.82f * std::abs(std::sin((float)i * 0.63f + peak * 7.0f));
        const int bh = 4 + (int)(wave * 24.0f * peak);
        g.setColour(i % 4 == 0 ? amber : green);
        g.fillRect(lcd.getX()+120+i*5, lcd.getBottom()-10-bh, 3, bh);
    }

    const int bw = 64;
    int bx = player.getX()+420;
    for (auto* b : {&previousButton,&playButton,&stopButton,&pauseButton,&nextButton})
    {
        b->setBounds(bx, player.getY()+13, bw, 29);
        bx += bw + 5;
    }
    lcdTitle.setBounds(player.getX()+420, player.getY()+48, player.getWidth()-435, 22);
    lcdStatus.setBounds(player.getX()+420, player.getY()+72, player.getWidth()-435, 18);

    // === PAD MATRIX ===
    auto pads = center.removeFromTop(392);
    drawBevel(g, pads);
    drawSectionTitle(g, pads, "DRUM PADS");
    auto padArea = pads.reduced(11); padArea.removeFromTop(28);
    const int gap = 7;
    const int cw = (padArea.getWidth()-gap*3)/4;
    const int ch = (padArea.getHeight()-gap*2)/3;
    for (int i = 0; i < PhysicalDrumEngineAudioProcessor::numPads; ++i)
    {
        const int row=i/4, col=i%4;
        auto r=juce::Rectangle<int>(padArea.getX()+col*(cw+gap), padArea.getY()+row*(ch+gap), cw, ch);
        padAreas[(size_t)i]=r;
        padButtons[(size_t)i].setBounds(r);
        padLabels[(size_t)i].setBounds(r.getX()+4,r.getBottom()-20,r.getWidth()-8,15);
        g.setColour(i==selectedPad ? juce::Colour(0xff143d1a) : juce::Colour(0xff101b22));
        g.fillRect(r.reduced(2));
        g.setColour(i==selectedPad || i==dragTargetPad ? green : shellEdge);
        g.drawRect(r.toFloat().reduced(1.0f), i==selectedPad ? 2.0f : 1.0f);
        g.setColour(juce::Colour(0xff344650));
        g.drawLine((float)r.getX()+5.0f,(float)r.getY()+5.0f,(float)r.getRight()-5.0f,(float)r.getY()+5.0f,1.0f);
        g.setColour(green);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
        g.drawText(juce::String::formatted("%02d", i+1), r.getX()+8, r.getY()+8, 24, 13, juce::Justification::left);
    }

    auto statusStrip = juce::Rectangle<int>(center.getX(), center.getBottom()-56, center.getWidth(), 48);
    drawBevel(g, statusStrip);
    selectedPadInfo.setBounds(statusStrip.getX()+10,statusStrip.getY()+12,statusStrip.getWidth()-20,22);

    // === KITS ===
    auto kitList=left.removeFromTop(405);
    drawBevel(g,kitList,true); drawSectionTitle(g,kitList,"KITS");
    presetBrowser.setBounds(kitList.getX()+10,kitList.getY()+37,kitList.getWidth()-20,kitList.getHeight()-147);
    newKitButton.setBounds(kitList.getX()+10,kitList.getBottom()-100,118,28);
    saveKitButton.setBounds(kitList.getX()+137,kitList.getBottom()-100,118,28);
    loadKitButton.setBounds(kitList.getX()+10,kitList.getBottom()-63,118,28);
    importButton.setBounds(kitList.getX()+137,kitList.getBottom()-63,118,28);

    // === GLOBAL EFFECTS ===
    auto effectsWide = juce::Rectangle<int>(left.getX(), center.getBottom()-147, leftW+10+centerW, 147);
    drawBevel(g,effectsWide); drawSectionTitle(g,effectsWide,"GLOBAL EFFECTS");
    const int gw = effectsWide.getWidth()/5;
    for (int i=0;i<5;++i)
    {
        const int x=effectsWide.getX()+i*gw;
        globalKnobLabels[(size_t)i].setBounds(x+1,effectsWide.getY()+29,gw-2,18);
        globalKnobs[(size_t)i].setBounds(x+2,effectsWide.getY()+45,gw-4,80);
    }

    // === MIDI ===
    auto midi = left.removeFromTop(std::max(120, left.getHeight()));
    drawBevel(g,midi); drawSectionTitle(g,midi,"MIDI");
    midiMapBox.setBounds(midi.getX()+10,midi.getY()+37,midi.getWidth()-20,28);
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("VELOCITY", midi.getX()+10,midi.getY()+76,70,14,juce::Justification::left);
    velocitySlider.setBounds(midi.getRight()-94,midi.getY()+51,78,68);

    // === SAMPLE ===
    auto sample=right.removeFromTop(402);
    drawBevel(g,sample,true); drawSectionTitle(g,sample,"SAMPLE");
    sampleName.setBounds(sample.getX()+12,sample.getY()+35,sample.getWidth()-112,21);
    sampleInfo.setBounds(sample.getX()+12,sample.getY()+57,sample.getWidth()-112,18);
    loadSampleButton.setBounds(sample.getRight()-91,sample.getY()+34,78,28);
    clearSampleButton.setBounds(sample.getRight()-91,sample.getY()+68,78,28);

    auto wave=sample.reduced(12); wave.removeFromTop(86); wave.removeFromBottom(105);
    g.setColour(panelBlack); g.fillRect(wave);
    g.setColour(juce::Colour(0xff16341b));
    for(int yy=wave.getY()+10;yy<wave.getBottom();yy+=10) g.drawHorizontalLine(yy,(float)wave.getX(),(float)wave.getRight());
    g.setColour(juce::Colour(0xff39503f));
    g.drawLine((float)wave.getX(),(float)wave.getCentreY(),(float)wave.getRight(),(float)wave.getCentreY(),1.0f);
    if(auto* s=processor.pads[(size_t)selectedPad].sample.get())
    {
        const auto* data=s->getReadPointer(0); const int n=s->getNumSamples();
        juce::Path pth;
        for(int x=0;x<wave.getWidth();++x)
        {
            int idx=juce::jlimit(0,n-1,(int)((double)x/(double)wave.getWidth()*n));
            float yy=wave.getCentreY()-data[idx]*wave.getHeight()*0.42f;
            if(x==0) pth.startNewSubPath((float)wave.getX()+x,yy); else pth.lineTo((float)wave.getX()+x,yy);
        }
        g.setColour(green); g.strokePath(pth,juce::PathStrokeType(1.1f));
    }
    for(int i=0;i<4;++i)
    {
        const int x=sample.getX()+8+i*(sample.getWidth()-16)/4;
        sampleKnobLabels[(size_t)i].setBounds(x,sample.getBottom()-91,(sample.getWidth()-16)/4-4,17);
        sampleKnobs[(size_t)i].setBounds(x,sample.getBottom()-75,(sample.getWidth()-16)/4-4,70);
    }

    // === OUTPUT ===
    auto out=right;
    drawBevel(g,out); drawSectionTitle(g,out,"OUTPUT");
    const float l=juce::jlimit(0.0f,1.0f,processor.getLeftPeak());
    const float rr=juce::jlimit(0.0f,1.0f,processor.getRightPeak());
    auto meterL=juce::Rectangle<int>(out.getX()+15,out.getY()+47,34,out.getHeight()-73);
    auto meterR=juce::Rectangle<int>(out.getX()+56,out.getY()+47,34,out.getHeight()-73);
    for(auto mr:{meterL,meterR})
    {
        g.setColour(black); g.fillRect(mr);
        g.setColour(shellEdge); g.drawRect(mr.toFloat(),1.0f);
        for(int sy=mr.getY()+4;sy<mr.getBottom();sy+=8){g.setColour(juce::Colour(0xff172229));g.drawHorizontalLine(sy,(float)mr.getX()+1,(float)mr.getRight()-1);}
    }
    g.setColour(green); g.fillRect(meterL.withTop(meterL.getBottom()-(int)(meterL.getHeight()*l)));
    g.setColour(green); g.fillRect(meterR.withTop(meterR.getBottom()-(int)(meterR.getHeight()*rr)));
    g.setColour(muted); g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("L",meterL.getX(),meterL.getBottom()+5,meterL.getWidth(),13,juce::Justification::centred);
    g.drawText("R",meterR.getX(),meterR.getBottom()+5,meterR.getWidth(),13,juce::Justification::centred);

    // Footer status bar.
    auto footer = juce::Rectangle<int>(outer.getX()+1, outer.getBottom()-25, outer.getWidth()-2, 18);
    g.setColour(juce::Colour(0xff0a1116)); g.fillRect(footer);
    g.setColour(shellEdge); g.drawRect(footer.toFloat(),1.0f);
    g.setColour(green); g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withTypefaceName("DejaVu Sans Mono").withStyle("Bold")));
    g.drawText("PHYSICAL DRUM ENGINE v2.1", footer.getX()+7, footer.getY()+2, 240, 13, juce::Justification::left);
    g.setColour(muted);
    g.drawText("WINAMP SKIN // AUDIO READY", footer.getRight()-220, footer.getY()+2, 210, 13, juce::Justification::right);
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
