// Source/PluginProcessor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
using namespace juce;

static inline float dbToLin(float dB) { return std::pow(10.0f, dB * 0.05f); }

ZClipAudioProcessor::ZClipAudioProcessor()
: AudioProcessor(
    BusesProperties()
      .withInput ("Input",  AudioChannelSet::stereo(), true)
      .withOutput("Output", AudioChannelSet::stereo(), true)
  ),
  apvts(*this, nullptr, "PARAMS", createLayout())
{}

void ZClipAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    envAttack = 0.001f;
    envRelease = 0.050f;
    currentOSChoice = (int) apvts.getRawParameterValue("os")->load();
    currentMaxBlock = (size_t) jmax(1, samplesPerBlock);

    ensureOversampler(currentOSChoice, getTotalNumInputChannels(), currentMaxBlock);
    if (oversampler) { oversampler->reset(); oversampler->initProcessing(currentMaxBlock); }

    scopeBuffer.clear();
    scopeFifo.reset();
    recentClipRatio.store(0.0f);
}

void ZClipAudioProcessor::releaseResources(){}

bool ZClipAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const bool same = layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    const bool okOut = layouts.getMainOutputChannelSet() == AudioChannelSet::mono()
                    || layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
    return same && okOut;
}

static inline float saturateTanh(float x, float ceiling, float driveLin)
{
    const float c = jmax(1.0e-6f, ceiling);
    const float n = (x * driveLin) / c;
    return c * std::tanh(n);
}

void ZClipAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals _;
    const int totalCh    = getTotalNumInputChannels();
    const int numSamples = buffer.getNumSamples();

    const int osChoiceNow = (int) apvts.getRawParameterValue("os")->load();
    if (osChoiceNow != currentOSChoice || !oversampler || (size_t) numSamples > currentMaxBlock)
    {
        currentOSChoice = osChoiceNow;
        currentMaxBlock = (size_t) jmax<int>((int) currentMaxBlock, numSamples);
        ensureOversampler(currentOSChoice, totalCh, currentMaxBlock);
        if (oversampler) { oversampler->reset(); oversampler->initProcessing(currentMaxBlock); }
    }

    const float pre      =          dbToLin(apvts.getRawParameterValue("pregain")->load());
    const float ceilLin  = jlimit(0.01f, 1.0f, dbToLin(apvts.getRawParameterValue("ceiling")->load()));
    const float out      =          dbToLin(apvts.getRawParameterValue("output")->load());
    const float mix      =          apvts.getRawParameterValue("mix")->load() * 0.01f;
    const bool  autoMk   =          apvts.getRawParameterValue("automakeup")->load() > 0.5f;

    const bool  upOn     =          apvts.getRawParameterValue("up_enable")->load() > 0.5f;
    const float upDb     =          apvts.getRawParameterValue("up_amount")->load();
    const float upMax    =          dbToLin(upDb);
    const float upKnee   = jlimit(0.0f, 12.0f, apvts.getRawParameterValue("up_knee")->load());

    const bool  ispProtect =       apvts.getRawParameterValue("isp")->load() > 0.5f;
    const float driveDb  =          apvts.getRawParameterValue("drive")->load();
    const float driveLin =          dbToLin(driveDb);

    const float makeup = autoMk ? (1.0f / std::sqrt(jmax(ceilLin, 1.0e-6f))) : 1.0f;
    const bool useSaturation = (driveDb > 0.0f); // Drive=0 → hard clip

    dsp::AudioBlock<float> block(buffer);
    int clippedCount = 0, considered = 0;
    const int osFactor = 1 << currentOSChoice;

    auto processSample = [&](float x, float dry, int ch)->float
    {
        if (upOn)
        {
            float& env = (ch == 0 ? levelEnvL : levelEnvR);
            const float ax   = std::abs(x);
            const float rateMul = (currentOSChoice > 0 ? 0.5f * (float) osFactor : 1.0f);
            const float atk  = std::exp(-1.0f / (rateMul * (float) sampleRate * envAttack));
            const float rel  = std::exp(-1.0f / (rateMul * (float) sampleRate * envRelease));
            env = (ax > env) ? (atk * env + (1.0f - atk) * ax)
                             : (rel * env + (1.0f - rel) * ax);

            const float envForDB = jmax(env, 1.0e-9f);
            const float envDB = 20.0f * std::log10(envForDB / ceilLin);
            float comp = 0.0f;
            if (envDB < 0.0f)
            {
                if (upKnee > 0.0f && envDB > -upKnee) comp = (upMax - 1.0f) * ((0.0f - envDB) / upKnee);
                else                                   comp = (upMax - 1.0f);
            }
            x *= (1.0f + comp);
        }

        const float y = useSaturation ? saturateTanh(x, ceilLin, driveLin)
                                      : jlimit(-ceilLin, ceilLin, x);

        float m = mix * y + (1.0f - mix) * dry;
        considered++;
        if (std::abs(m) > ceilLin) clippedCount++;
        return jlimit(-ceilLin, ceilLin, m);
    };

    if (oversampler && currentOSChoice > 0)
    {
        auto& os = *oversampler;
        auto up  = os.processSamplesUp(block);

        for (int ch = 0; ch < totalCh; ++ch)
        {
            float* d = up.getChannelPointer(ch);
            const int ns  = (int) up.getNumSamples();
            for (int i = 0; i < ns; ++i)
            {
                const float dry = d[i];
                const float x   = dry * pre * makeup;
                d[i] = processSample(x, dry, ch);
            }
            if (ispProtect)
                for (int i = 0; i < ns; ++i) d[i] = jlimit(-ceilLin, ceilLin, d[i]);
        }

        os.processSamplesDown(block);

        if (ispProtect)
            for (int ch = 0; ch < totalCh; ++ch)
            {
                float* w = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i) w[i] = jlimit(-ceilLin, ceilLin, w[i]);
            }
    }
    else
    {
        for (int ch = 0; ch < totalCh; ++ch)
        {
            float* d = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = d[i];
                const float x   = dry * pre * makeup;
                d[i] = processSample(x, dry, ch);
            }
        }
    }

    buffer.applyGain(out);

    for (int ch = totalCh; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    if (considered > 0) recentClipRatio.store((float) clippedCount / (float) considered);

    const float* ch0 = buffer.getReadPointer(0);
    const float* ch1 = (buffer.getNumChannels() > 1) ? buffer.getReadPointer(1) : ch0;
    const float* chData[2] = { ch0, ch1 };
    pushToScope(chData, jmin(2, buffer.getNumChannels()), numSamples, 1);
}

void ZClipAudioProcessor::pushToScope(const float* const* data, int numChannels, int numSamples, int stride)
{
    const int toPush = numSamples / stride;
    int start1=0,size1=0,start2=0,size2=0;
    const int chans = jmin(2, numChannels);
    scopeFifo.prepareToWrite(toPush, start1, size1, start2, size2);
    if (size1 + size2 > 0)
    {
        for (int c = 0; c < chans; ++c)
        {
            if (size1 > 0) for (int i = 0; i < size1; ++i) scopeBuffer.setSample(c, start1 + i, data[c][i * stride]);
            if (size2 > 0) for (int i = 0; i < size2; ++i) scopeBuffer.setSample(c, start2 + i, data[c][(size1 + i) * stride]);
        }
        scopeFifo.finishedWrite(size1 + size2);
    }
}

void ZClipAudioProcessor::readScope(AudioBuffer<float>& dest)
{
    const int want = dest.getNumSamples();
    int start1=0,size1=0,start2=0,size2=0;
    scopeFifo.prepareToRead(want, start1, size1, start2, size2);
    if (size1 + size2 > 0)
    {
        const int chans = jmin(2, dest.getNumChannels());
        for (int c = 0; c < chans; ++c)
        {
            if (size1 > 0) dest.copyFrom(c, 0,     scopeBuffer, c, start1, size1);
            if (size2 > 0) dest.copyFrom(c, size1, scopeBuffer, c, start2, size2);
        }
        scopeFifo.finishedRead(size1 + size2);
        if (size1 + size2 < want) dest.clear(0, size1 + size2, want - (size1 + size2));
    } else dest.clear();
}

void ZClipAudioProcessor::ensureOversampler(int osChoice, int numChannels, size_t maxBlock)
{
    const int factorPow = osChoice;
    if (factorPow <= 0) { oversampler.reset(); return; }
    oversampler.reset(new dsp::Oversampling<float>(
        numChannels, factorPow,
        dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true));
    oversampler->initProcessing(maxBlock);
}

juce::AudioProcessorValueTreeState::ParameterLayout ZClipAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // Mode removed
    p.push_back(std::make_unique<AudioParameterFloat>("pregain","PreGain",NormalisableRange<float>(-24.0f,24.0f,0.01f),0.0f));
    p.push_back(std::make_unique<AudioParameterFloat>("ceiling","Ceiling",NormalisableRange<float>(-18.0f,0.0f,0.01f),0.0f));
    p.push_back(std::make_unique<AudioParameterBool>("automakeup","AutoMakeup",true));

    p.push_back(std::make_unique<AudioParameterBool>("up_enable","UpwardEnable",false));
    p.push_back(std::make_unique<AudioParameterFloat>("up_amount","UpwardAmount",NormalisableRange<float>(0.0f,6.0f,0.01f),0.0f));
    p.push_back(std::make_unique<AudioParameterFloat>("up_knee","UpwardKnee",NormalisableRange<float>(0.0f,12.0f,0.01f),6.0f));

    p.push_back(std::make_unique<AudioParameterFloat>("mix","Mix",NormalisableRange<float>(0.0f,100.0f,0.01f),100.0f));
    p.push_back(std::make_unique<AudioParameterFloat>("output","Output",NormalisableRange<float>(-24.0f,24.0f,0.01f),0.0f));

    p.push_back(std::make_unique<AudioParameterChoice>("os","Oversampling",StringArray{"1x","2x","4x","8x"},2));
    p.push_back(std::make_unique<AudioParameterBool>("isp","TruePeakProtect",true));

    p.push_back(std::make_unique<AudioParameterFloat>("drive","Drive",NormalisableRange<float>(0.0f,12.0f,0.01f),0.0f));

    return { p.begin(), p.end() };
}

void ZClipAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void ZClipAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto t = ValueTree::readFromData(data, (size_t) sizeInBytes);
    if (t.isValid()) apvts.replaceState(t);
}

juce::AudioProcessorEditor* ZClipAudioProcessor::createEditor() { return new ZClipAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZClipAudioProcessor(); }

float ZClipAudioProcessor::getCeilingDb()
{
    if (auto* p = apvts.getRawParameterValue("ceiling")) return p->load();
    return 0.0f;
}
