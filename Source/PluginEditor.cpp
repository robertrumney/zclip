#include "PluginEditor.h"
using namespace juce;

static Colour brandYellow() { return Colour::fromRGB(255, 216, 0); }

static void styleKnob(Slider& s)
{
    s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(Slider::TextBoxBelow, false, 64, 18);
    s.setColour(Slider::rotarySliderFillColourId, brandYellow().withAlpha(0.9f));
    s.setColour(Slider::thumbColourId,            brandYellow());
    s.setColour(Slider::textBoxTextColourId,      Colours::white);
    s.setColour(Slider::textBoxOutlineColourId,   Colours::transparentBlack);
    s.setColour(Slider::trackColourId,            brandYellow().withAlpha(0.35f));
    s.setColour(Slider::backgroundColourId,       Colours::black.withAlpha(0.4f));
}

static void label(Label& L, const String& t)
{
    L.setText(t, dontSendNotification);
    L.setJustificationType(Justification::centred);
    L.setColour(Label::textColourId, Colours::white.withAlpha(0.9f));
}

ZClipAudioProcessorEditor::ZClipAudioProcessorEditor(ZClipAudioProcessor& proc)
    : AudioProcessorEditor(&proc), p(proc),
      scope([&proc](AudioBuffer<float>& dest){ proc.readScope(dest); },
            [&proc]{ return proc.getCeilingDb(); })
{
    setOpaque(true);
    setSize(980, 580);

    os.addItemList(StringArray{ "1x", "2x", "4x", "8x" }, 1);

    styleKnob(pregain); styleKnob(ceiling); styleKnob(drive); styleKnob(softness);
    styleKnob(upAmount); styleKnob(upKnee); styleKnob(mix); styleKnob(output);

    pregain.setRange(-24.0, 24.0, 0.01);
    ceiling.setRange(-18.0, 0.0, 0.01);
    drive.setRange(0.0, 12.0, 0.01);
    softness.setRange(0.0, 1.0, 0.001);
    upAmount.setRange(0.0, 6.0, 0.01);
    upKnee.setRange(0.0, 12.0, 0.01);
    mix.setRange(0.0, 100.0, 0.01);
    output.setRange(-24.0, 24.0, 0.01);

    label(lOS, "Oversampling");
    label(lPre, "PreGain");
    label(lCeil, "Ceiling (dB)");
    label(lDrive, "Drive");
    label(lSoft, "Shape");
    label(lUpEn, "Upward Compression");
    label(lUpAmt, "Upward Gain (dB)");
    label(lUpKnee, "Upward Knee (dB)");
    label(lMix, "Mix (%)");
    label(lOut, "Output");

    addAndMakeVisible(lOS);   addAndMakeVisible(os);

    addAndMakeVisible(automakeup);
    addAndMakeVisible(isp);
    addAndMakeVisible(upEnable);

    addAndMakeVisible(lPre);    addAndMakeVisible(pregain);
    addAndMakeVisible(lCeil);   addAndMakeVisible(ceiling);
    addAndMakeVisible(lDrive);  addAndMakeVisible(drive);
    addAndMakeVisible(lSoft);   addAndMakeVisible(softness);
    addAndMakeVisible(lUpAmt);  addAndMakeVisible(upAmount);
    addAndMakeVisible(lUpKnee); addAndMakeVisible(upKnee);
    addAndMakeVisible(lMix);    addAndMakeVisible(mix);
    addAndMakeVisible(lOut);    addAndMakeVisible(output);

    addAndMakeVisible(scope);

    aOS    = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "os", os);
    aAM    = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "automakeup", automakeup);
    aISP   = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "isp", isp);
    aUpEn  = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "up_enable", upEnable);

    aPre   = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "pregain", pregain);
    aCeil  = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "ceiling", ceiling);
    aDrive = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "drive", drive);
    aSoft  = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "shape", softness);
    aUpAmt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "up_amount", upAmount);
    aUpKnee= std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "up_knee", upKnee);
    aMix   = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "mix", mix);
    aOut   = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "output", output);
}

ZClipAudioProcessorEditor::~ZClipAudioProcessorEditor() {}

void ZClipAudioProcessorEditor::paint(Graphics& g)
{
    Image bg = ImageCache::getFromFile(File("C:/ZClip/Assets/background.png"));
    if (bg.isValid())
        g.drawImageWithin(bg, 0, 0, getWidth(), getHeight(), RectanglePlacement::stretchToFit);
    else
        g.fillAll(Colour::fromFloatRGBA(0.07f, 0.08f, 0.10f, 1.0f));

    auto underTogglesY = scope.getY() - 10;
    g.setColour(Colours::white.withAlpha(0.06f));
    g.fillRect(0, underTogglesY, getWidth(), 2);
}

static void placeLabelAndCtrl(Label& L, Component& C, Rectangle<int> area)
{
    L.setBounds(area.removeFromTop(18));
    C.setBounds(area.reduced(0, 6));
}

void ZClipAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(24);

    auto top = r.removeFromTop(64);
    const int colW = 240;
    Rectangle<int> osRect(top.getCentreX() - colW/2, top.getY()+10, colW, 48);
    placeLabelAndCtrl(lOS, os, osRect);

    auto toggles = r.removeFromTop(32);
    automakeup.setBounds(toggles.removeFromLeft(170));
    toggles.removeFromLeft(12);
    isp.setBounds(toggles.removeFromLeft(200));
    toggles.removeFromLeft(12);
    upEnable.setBounds(toggles.removeFromLeft(200));
    r.removeFromTop(6);

    const int scopeH = 200;
    const int scopeY = r.getY();
    r.removeFromTop(scopeH);
    scope.setBounds(16, scopeY, getWidth()-32, scopeH);

    r.removeFromTop(16);

    auto knobs = r.removeFromTop(230);
    const int gap = 14;
    const int cols = 8;
    const int colW2 = (knobs.getWidth() - gap * (cols - 1)) / cols;

    auto colX = [&](int i) { return knobs.getX() + i * (colW2 + gap); };
    auto slot = [&](int i) { return Rectangle<int>(colX(i), knobs.getY(), colW2, knobs.getHeight()); };

    placeLabelAndCtrl(lPre,    pregain,  slot(0));
    placeLabelAndCtrl(lCeil,   ceiling,  slot(1));
    placeLabelAndCtrl(lDrive,  drive,    slot(2));
    placeLabelAndCtrl(lSoft,   softness, slot(3));
    placeLabelAndCtrl(lUpAmt,  upAmount, slot(4));
    placeLabelAndCtrl(lUpKnee, upKnee,   slot(5));
    placeLabelAndCtrl(lMix,    mix,      slot(6));
    placeLabelAndCtrl(lOut,    output,   slot(7));
}
