#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
class ZClipAudioProcessor : public juce::AudioProcessor{
public:

    float getCeilingDb();

public:
    ZClipAudioProcessor(); ~ZClipAudioProcessor() override = default;
    void prepareToPlay(double, int) override; void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override; bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Z-Clip"; }
    bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; } double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {} const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState apvts;
    void readScope(juce::AudioBuffer<float>& dest);
    float getRecentClipRatio() const noexcept { return recentClipRatio.load(); }
private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int currentOSChoice = 0; size_t currentMaxBlock = 0; double sampleRate = 44100.0;
    float levelEnvL = 0.0f, levelEnvR = 0.0f; float envAttack = 0.001f, envRelease = 0.050f;
    juce::AudioBuffer<float> scopeBuffer {2, 8192};
    juce::AbstractFifo scopeFifo {8192};
    std::atomic<float> recentClipRatio {0.0f};
    void pushToScope(const float* const* data, int numChannels, int numSamples, int stride = 8);
    void ensureOversampler(int osChoice, int numChannels, size_t maxBlock);
    static inline float dbToLin(float dB){ return std::pow(10.0f, dB*0.05f); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZClipAudioProcessor)
};
