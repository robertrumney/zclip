#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ScopeComponent.h"

class ZClipAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ZClipAudioProcessorEditor(ZClipAudioProcessor&);
    ~ZClipAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ZClipAudioProcessor& p;

    juce::ComboBox os;
    juce::ToggleButton automakeup{"AutoMakeup"}, isp{"TruePeakProtect"}, upEnable{"Upward Compression"};
    juce::Slider pregain, ceiling, drive, softness, upAmount, upKnee, mix, output;
    juce::Label  lOS, lPre, lCeil, lDrive, lSoft, lUpEn, lUpAmt, lUpKnee, lMix, lOut;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aOS;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aAM, aISP, aUpEn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aPre, aCeil, aDrive, aSoft, aUpAmt, aUpKnee, aMix, aOut;

    ScopeComponent scope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZClipAudioProcessorEditor)
};
