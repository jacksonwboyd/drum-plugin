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
