/*
  ==============================================================================

    PluginEditor.h
    LA9A — Levelling Compressor
    Studio9 Plugins

    Filmstrip build: Compress/Output/Input Gain dials and the Side-Chain/
    Mode/Auto-Level switches all paint from filmstrip images via
    FilmstripLookAndFeel.h, instead of standard JUCE Slider/ToggleButton
    look-and-feel.
  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include "VerticalMeter.h"
#include "FilmstripLookAndFeel.h"
#include "BinaryData.h"

class LA9AAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit LA9AAudioProcessorEditor (LA9AAudioProcessor&);
    ~LA9AAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    LA9AAudioProcessor& processorRef;

    juce::Image backgroundImage;

    // ── Filmstrip images ────────────────────────────────────────────────
    // Compress and Output dials share one 128-frame knob filmstrip; Side-
    // Chain/Auto-Level share one 2-frame On/Off filmstrip; Mode uses its own
    // 2-frame A/B filmstrip.
    //
    // Deliberately loaded here via a default member initializer (not in the
    // .cpp constructor body): these Image members are declared, and so
    // constructed, BEFORE the LookAndFeel members below, which take a COPY
    // of each image in their own constructor. If the images were still
    // loaded in the .cpp ctor body instead, the LookAndFeel objects would
    // end up copying still-empty/invalid Images, since all member
    // initializers run before the ctor body — so the load has to happen
    // here, not there.
    juce::Image knobFilmstripImage   { juce::ImageCache::getFromMemory (BinaryData::Knob_128x128_128f_png,   BinaryData::Knob_128x128_128f_pngSize) };
    juce::Image switchFilmstripImage { juce::ImageCache::getFromMemory (BinaryData::Switch_100x60_2f_png,    BinaryData::Switch_100x60_2f_pngSize) };
    juce::Image modeFilmstripImage   { juce::ImageCache::getFromMemory (BinaryData::Switch_AB_100x60_2f_png, BinaryData::Switch_AB_100x60_2f_pngSize) };

    // ── Filmstrip LookAndFeel instances ────────────────────────────────────
    // Each copies the corresponding image above at construction time (see
    // note above for why the images must already be loaded by this point).
    // Cleared from every control via setLookAndFeel(nullptr) in the
    // destructor before these are torn down.
    FilmstripKnobLookAndFeel   knobLookAndFeel   { knobFilmstripImage, 128 };
    FilmstripSwitchLookAndFeel switchLookAndFeel { switchFilmstripImage, true };  // On=frame0/top, Off=frame1/bottom
    FilmstripSwitchLookAndFeel modeLookAndFeel   { modeFilmstripImage, false };   // A=frame0/top,  B=frame1/bottom

    // ── Controls ────────────────────────────────────────────────────────
    // No captions passed to the ToggleButtons / no separate Labels for the
    // dials — "Compress", "Side-Chain", "Mode", "Output", "Auto Level" are
    // already part of LA9A_Skin.png.
    // Input Gain: -12..+12 dB, true pre-gain (see PluginProcessor::
    // processBlock — applied before metering/detector/compression, not
    // just a display trim). Same look as Output (filmstrip knob, no text
    // box, popup bubble with dB suffix while dragging).
    juce::Slider inputGainSlider;

    juce::Slider compressSlider;

    juce::ToggleButton sideChainButton; // unchecked = Off, checked = On
    juce::ToggleButton modeButton;      // unchecked = A,  checked = B

    juce::Slider outputSlider;

    // Auto Level bypasses (not disables) manual Output when engaged — see
    // LevelingCompressor::process(). outputSlider stays enabled/interactive
    // the whole time; Can still move it while Auto Level is on, the
    // new value just has no audible effect until Auto Level is switched
    // back off.
    juce::ToggleButton autoGainButton;

    // ── Meters ───────────────────────────────────────────────────────────
    // Linear gradient bars per spec: Input/Output = cyan (bottom)
    // to white (top), fills upward from the bottom, TRUE STEREO — one bar
    // per channel with 4px padding between L and R. GR = pale red (top) to
    // red (bottom), fills downward from the top, mono (single bar). Numeric
    // RMS/GR values are shown in the Labels underneath — no side dB scale.
    static constexpr int kStereoMeterGapPx = 4;

    VerticalMeter inputMeterBarL  { juce::Colours::white, juce::Colours::cyan,
                                      VerticalMeter::FillOrigin::Bottom, -60.0f, 0.0f };
    VerticalMeter inputMeterBarR  { juce::Colours::white, juce::Colours::cyan,
                                      VerticalMeter::FillOrigin::Bottom, -60.0f, 0.0f };
    VerticalMeter outputMeterBarL { juce::Colours::white, juce::Colours::cyan,
                                      VerticalMeter::FillOrigin::Bottom, -60.0f, 0.0f };
    VerticalMeter outputMeterBarR { juce::Colours::white, juce::Colours::cyan,
                                      VerticalMeter::FillOrigin::Bottom, -60.0f, 0.0f };
    VerticalMeter grMeterBar      { juce::Colour (0xffff8080), juce::Colours::red,
                                      VerticalMeter::FillOrigin::Top, 0.0f, -18.0f };

    juce::Label inputMeterLabel, outputMeterLabel, grMeterLabel;

    juce::Label versionLabel; 

    // ── APVTS attachments ───────────────────────────────────────────────
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>      inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>      compressAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>      outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>      sideChainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>      autoGainAttachment;

    // MODE is an AudioParameterChoice (A/B), not a bool, so it's driven
    // manually (onClick lambda + polled in timerCallback) rather than via
    // ButtonAttachment, which expects a bool parameter — see .cpp.
    void modeButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA9AAudioProcessorEditor)
};
