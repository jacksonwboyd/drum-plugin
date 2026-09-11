#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PhysicalDrumEngineAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      public juce::FileDragAndDropTarget,
      private juce::Timer
{
public:
    explicit PhysicalDrumEngineAudioProcessorEditor(PhysicalDrumEngineAudioProcessor&);
    ~PhysicalDrumEngineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void setupButton(juce::TextButton& button, const juce::String& text, std::function<void()> action);
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& id, const juce::String& name);
    void selectPad(int index, bool audition = false);
    void refreshPadText();
    void refreshSelectedPadControls();
    void loadSelectedSample();
    void clearSelectedSample();
    void saveKitToChooser();
    void loadKitFromChooser();
    void importKit();
    void applyPreset(int index);
    void applyMidiMap(int index);
    int padAtPosition(int x, int y) const;
    void updateDragTarget(int x, int y);
    void clearDragTarget();
    bool isSupportedSampleFile(const juce::String& path) const;
    void drawPanel(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& titleText);
    void drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float value, const juce::String& label);

    PhysicalDrumEngineAudioProcessor& processor;

    juce::Label windowTitle;
    juce::Label menuBar;
    juce::Label marquee;
    juce::Label lcdTitle;
    juce::Label lcdStatus;
    juce::Label sampleName;
    juce::Label sampleInfo;
    juce::Label selectedPadInfo;

    std::array<juce::TextButton, PhysicalDrumEngineAudioProcessor::numPads> padButtons;
    std::array<juce::Label, PhysicalDrumEngineAudioProcessor::numPads> padLabels;
    std::array<juce::Rectangle<int>, PhysicalDrumEngineAudioProcessor::numPads> padAreas;

    juce::TextButton playButton { "PLAY" };
    juce::TextButton stopButton { "STOP" };
    juce::TextButton pauseButton { "PAUSE" };
    juce::TextButton previousButton { "|<" };
    juce::TextButton nextButton { ">|" };

    juce::TextButton newKitButton { "New Kit" };
    juce::TextButton saveKitButton { "Save Kit" };
    juce::TextButton loadKitButton { "Load Kit" };
    juce::TextButton importButton { "Import..." };

    juce::TextButton loadSampleButton { "LOAD" };
    juce::TextButton clearSampleButton { "CLEAR" };

    juce::ComboBox presetBrowser;
    juce::ComboBox midiMapBox;

    juce::Slider velocitySlider;
    std::array<juce::Slider, 4> sampleKnobs;
    std::array<juce::Label, 4> sampleKnobLabels;
    std::array<juce::Slider, 5> globalKnobs;
    std::array<juce::Label, 5> globalKnobLabels;

    static constexpr std::array<const char*, 5> globalKnobIds = {
        "sampleRate", "output", "filter", "attack", "release"
    };

    static constexpr std::array<const char*, 5> globalKnobNames = {
        "SAMPLE RATE", "OUTPUT", "FILTER", "ATTACK", "RELEASE"
    };

    int selectedPad = 1;
    int dragTargetPad = -1;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhysicalDrumEngineAudioProcessorEditor)
};
