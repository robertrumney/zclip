// Source/ScopeComponent.cpp
#include "ScopeComponent.h"
using namespace juce;

ScopeComponent::ScopeComponent(std::function<void(AudioBuffer<float>&)> reader,
                               std::function<float()> ceilingGetter)
    : readFn(std::move(reader)), getCeiling(std::move(ceilingGetter))
{
    buf.setSize(2, 2048, false, true, true);
    setOpaque(false);
    startTimerHz(30);
}

void ScopeComponent::timerCallback() { repaint(); }

void ScopeComponent::paint(juce::Graphics& g)
{
    if (readFn) readFn(buf);

    g.fillAll(Colour::fromFloatRGBA(0.07f, 0.08f, 0.10f, 1.0f));

    auto bounds = getLocalBounds().reduced(6);
    auto radius = 12.0f;
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();
    auto midY = (float) bounds.getCentreY();

    Path clip; clip.addRoundedRectangle(bounds.toFloat(), radius);
    g.reduceClipRegion(clip);

    g.setColour(Colour::fromFloatRGBA(0.10f, 0.12f, 0.14f, 1.0f));
    g.fillRoundedRectangle(bounds.toFloat(), radius);

    g.setColour(Colours::black);
    g.drawRoundedRectangle(bounds.toFloat(), radius, 2.0f);

    leftPath.clear(); rightPath.clear();
    const int samples = buf.getNumSamples();
    if (samples <= 0 || w <= 0) return;

    const int step = jmax(1, samples / jmax(1, w));
    auto drawChan = [&](int ch, Path& path)
    {
        float x = (float) bounds.getX();
        bool started = false;
        for (int i = 0; i < samples; i += step)
        {
            const float v = buf.getSample(jmin(ch, buf.getNumChannels()-1), i);
            const float y = midY - v * (h * 0.40f);
            if (!started) { path.startNewSubPath(x, y); started = true; }
            else          { path.lineTo(x, y); }
            x += 1.0f;
            if (x > (float) bounds.getRight()) break;
        }
    };
    drawChan(0, leftPath);
    drawChan(1, rightPath);

    // ceiling line: WHITE
    float ceilDb = getCeiling ? getCeiling() : 0.0f;
    ceilDb = jlimit(-24.0f, 0.0f, ceilDb);
    const float yCeil = midY - Decibels::decibelsToGain(ceilDb) * (h * 0.40f);
    g.setColour(Colours::white.withAlpha(0.95f));
    g.drawLine((float) bounds.getX(), yCeil, (float) bounds.getRight(), yCeil, 2.0f);

    // waveform: yellow
    auto waveCol = Colour::fromRGB(255, 216, 0);
    g.setColour(waveCol.withAlpha(0.25f));
    g.strokePath(leftPath,  PathStrokeType(6.0f, PathStrokeType::beveled, PathStrokeType::rounded));
    g.strokePath(rightPath, PathStrokeType(6.0f, PathStrokeType::beveled, PathStrokeType::rounded));

    g.setColour(waveCol);
    g.strokePath(leftPath,  PathStrokeType(2.0f));
    g.strokePath(rightPath, PathStrokeType(2.0f));
}
