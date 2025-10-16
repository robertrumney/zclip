#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

class ScopeComponent : public juce::Component, private juce::Timer
{
public:
    ScopeComponent(std::function<void(juce::AudioBuffer<float>&)> reader,
                   std::function<float()> ceilingGetter);
    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;

    std::function<void(juce::AudioBuffer<float>&)> readFn;
    std::function<float()> getCeiling;

    juce::AudioBuffer<float> buf;
    juce::Path leftPath, rightPath;
};
