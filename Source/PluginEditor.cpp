/*
  ==============================================================================

    PluginEditor.cpp
    LA9A — Levelling Compressor
    Studio9 Plugins

  ==============================================================================
*/

#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth  = 900;
constexpr int kEditorHeight = 500; // matches LA9A_Skin.png's native size
} // namespace

// =============================================================================
LA9AAudioProcessorEditor::LA9AAudioProcessorEditor (LA9AAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::LA9A_Skin_png,
                                                          BinaryData::LA9A_Skin_pngSize);

    // ── Input Gain ───────────────────────────────────────────────────────
    // Same treatment as Output — filmstrip-painted, full 64x64 bounds, no
    // built-in text box, value shown via popup bubble while dragging.
    inputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    inputGainSlider.setPopupDisplayEnabled (true, true, this);
    inputGainSlider.setTextValueSuffix (" dB");
    inputGainSlider.setLookAndFeel (&knobLookAndFeel);
    addAndMakeVisible (inputGainSlider);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, LA9AAudioProcessor::paramInputGain, inputGainSlider);

    // ── Compress (Ratio) ────────────────────────────────────────────────
    // Filmstrip-painted (see FilmstripLookAndFeel.h) — no built-in text box,
    // the 180x180 bounds are filled entirely by the knob artwork. A popup
    // bubble (using the same "1:x" formatting) still shows the value while
    // dragging.
    compressSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    compressSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    compressSlider.setPopupDisplayEnabled (true, true, this);
    // Displays the underlying float (1.1 .. 8.0) as a "1:x" ratio string.
    compressSlider.textFromValueFunction = [] (double value)
    {
        return "1:" + juce::String (value, 1);
    };
    compressSlider.setLookAndFeel (&knobLookAndFeel);
    addAndMakeVisible (compressSlider);

    compressAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, LA9AAudioProcessor::paramCompressRatio, compressSlider);

    // ── Side-Chain ───────────────────────────────────────────────────────
    sideChainButton.setLookAndFeel (&switchLookAndFeel);
    addAndMakeVisible (sideChainButton);
    sideChainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, LA9AAudioProcessor::paramSideChain, sideChainButton);

    // ── Mode (A/B) — AudioParameterChoice, driven manually ─────────────────
    modeButton.setLookAndFeel (&modeLookAndFeel);
    addAndMakeVisible (modeButton);

    if (auto* modeParam = processorRef.apvts.getParameter (LA9AAudioProcessor::paramMode))
        modeButton.setToggleState (modeParam->getValue() > 0.5f, juce::dontSendNotification);

    modeButton.onClick = [this] { modeButtonClicked(); };

    // ── Output ───────────────────────────────────────────────────────────
    // Same reasoning as Compress above — full 128x128 bounds reserved for
    // the dial artwork, value shown via popup bubble instead of a text box.
    outputSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    outputSlider.setPopupDisplayEnabled (true, true, this);
    outputSlider.setTextValueSuffix (" dB");
    outputSlider.setLookAndFeel (&knobLookAndFeel);
    addAndMakeVisible (outputSlider);

    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, LA9AAudioProcessor::paramOutputGain, outputSlider);

    // ── Auto Level ───────────────────────────────────────────────────────
    // When engaged, Auto Level REPLACES the manual Output control's EFFECT
    // (see LevelingCompressor::process()), but the Output dial itself stays
    // fully enabled/interactive the whole time — Can still turn it
    // while Auto Level is on, it just has no audible effect until Auto
    // Level is switched back off, at which point whatever value the dial
    // was left at applies immediately.
    autoGainButton.setLookAndFeel (&switchLookAndFeel);
    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, LA9AAudioProcessor::paramAutoGain, autoGainButton);

    // ── Meters ───────────────────────────────────────────────────────────
    for (auto* bar : { &inputMeterBarL, &inputMeterBarR, &outputMeterBarL, &outputMeterBarR, &grMeterBar })
        addAndMakeVisible (bar);

    for (auto* label : { &inputMeterLabel, &outputMeterLabel, &grMeterLabel })
    {
        label->setFont (juce::Font (8.0f, juce::Font::bold));
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::backgroundColourId, juce::Colours::black);
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (label);
    }

    // ── Version (title text removed — now part of the skin) ────────────────
#if defined (LA9A_VERSION_STRING)
    versionLabel.setText ("Version " LA9A_VERSION_STRING, juce::dontSendNotification);
#else
    versionLabel.setText ("Version (dev build)", juce::dontSendNotification);
#endif
    versionLabel.setJustificationType (juce::Justification::centred);
    versionLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (versionLabel);

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30); // meter refresh rate
}

LA9AAudioProcessorEditor::~LA9AAudioProcessorEditor()
{
    stopTimer();

    // Clear every control's LookAndFeel pointer before the LookAndFeel
    // members (and the images they copied) are torn down.
    inputGainSlider.setLookAndFeel (nullptr);
    compressSlider.setLookAndFeel (nullptr);
    outputSlider.setLookAndFeel (nullptr);
    sideChainButton.setLookAndFeel (nullptr);
    autoGainButton.setLookAndFeel (nullptr);
    modeButton.setLookAndFeel (nullptr);
}

// =============================================================================
void LA9AAudioProcessorEditor::modeButtonClicked()
{
    if (auto* modeParam = processorRef.apvts.getParameter (LA9AAudioProcessor::paramMode))
    {
        const bool isB = modeButton.getToggleState();
        modeParam->setValueNotifyingHost (isB ? 1.0f : 0.0f);
    }
}

// =============================================================================
void LA9AAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat());
    else
        g.fillAll (juce::Colour (0xff0a1a1f)); // fallback if the skin fails to load
}

// =============================================================================
void LA9AAudioProcessorEditor::resized()
{
    // EXACT pixel placement from plan's Component_Locations.txt (900x502
    // canvas, matches LA9A_Skin.png). Kept as literal X/Y/W/H per component
    // — deliberately NOT computed/proportional — so each rectangle can be
    // adjusted independently by editing a single line here.

    // Input Signal Level Meter: X=27, Y=32, W=46, H=380 (stereo: split into
    // L/R bars with kStereoMeterGapPx between them). Numeric read-out kept,
    // placed directly under the bar — Vatche will adjust its position.
    {
        const juce::Rectangle<int> inputMeterRect (27, 32, 46, 380);
        const int barWidth = (inputMeterRect.getWidth() - kStereoMeterGapPx) / 2;
        inputMeterBarL.setBounds (inputMeterRect.getX(), inputMeterRect.getY(),
                                    barWidth, inputMeterRect.getHeight());
        inputMeterBarR.setBounds (inputMeterRect.getX() + barWidth + kStereoMeterGapPx,
                                    inputMeterRect.getY(), barWidth, inputMeterRect.getHeight());
        inputMeterLabel.setBounds (inputMeterRect.getX(), inputMeterRect.getBottom() - 8,
                                     inputMeterRect.getWidth(), 18);
    }

    // Output Signal Level Meter: X=827, Y=32, W=46, H=380 (stereo).
    {
        const juce::Rectangle<int> outputMeterRect (827, 32, 46, 380);
        const int barWidth = (outputMeterRect.getWidth() - kStereoMeterGapPx) / 2;
        outputMeterBarL.setBounds (outputMeterRect.getX(), outputMeterRect.getY(),
                                     barWidth, outputMeterRect.getHeight());
        outputMeterBarR.setBounds (outputMeterRect.getX() + barWidth + kStereoMeterGapPx,
                                     outputMeterRect.getY(), barWidth, outputMeterRect.getHeight());
        outputMeterLabel.setBounds (outputMeterRect.getX(), outputMeterRect.getBottom() - 8,
                                      outputMeterRect.getWidth(), 18);
    }

    // GR Meter: X=482, Y=32, W=36, H=380 (mono, single bar).
    {
        const juce::Rectangle<int> grMeterRect (482, 32, 36, 380);
        grMeterBar.setBounds (grMeterRect);
        grMeterLabel.setBounds (grMeterRect.getX(), grMeterRect.getBottom() - 8, //
                                  grMeterRect.getWidth(), 18);
    }

    // Input Gain Dial: X=642, Y=90, W=64, H=64 — half the size of the
    // Output dial, same filmstrip (frame scaled down to fit, no manual
    // resize needed).
    inputGainSlider.setBounds (642, 90, 64, 64);

    // Compressor Dial: X=200, Y=145, W=180, H=180 — filmstrip drawn 1:1 into
    // this box by FilmstripKnobLookAndFeel (source frame scaled to fit).
    compressSlider.setBounds (200, 147, 180, 180);

    // Output Dial: X=606, Y=197, W=128, H=128.
    outputSlider.setBounds (610, 203, 128, 128);

    // Side-Chain Button: X=144, Y=400, W=50, H=30.
    sideChainButton.setBounds (144, 400, 50, 30);

    // Mode A/B Button: X=368, Y=400, W=50, H=30.
    modeButton.setBounds (368, 400, 50, 30);

    // Auto-Level Button: X=646, Y=400, W=50, H=30.
    autoGainButton.setBounds (646, 400, 50, 30);

    // Version Number Text: X=330, Y=37, W=105, H=20.
    versionLabel.setBounds (330, 37, 105, 20);
}

// =============================================================================
void LA9AAudioProcessorEditor::timerCallback()
{
    const float inputL  = processorRef.inputMeterL.load();
    const float inputR  = processorRef.inputMeterR.load();
    const float outputL = processorRef.outputMeterL.load();
    const float outputR = processorRef.outputMeterR.load();
    const float grDb    = processorRef.gainReductionMeter.load();

    // True stereo: one bar per channel. GR stays mono (single bar).
    inputMeterBarL.setLevelDb  (inputL);
    inputMeterBarR.setLevelDb  (inputR);
    outputMeterBarL.setLevelDb (outputL);
    outputMeterBarR.setLevelDb (outputR);
    grMeterBar.setLevelDb      (grDb);

    inputMeterLabel.setText (
        juce::String (inputL, 1) + " | " + juce::String (inputR, 1), // + " dB",
        juce::dontSendNotification);

    outputMeterLabel.setText (
        juce::String (outputL, 1) + " | " + juce::String (outputR, 1), // + " dB",
        juce::dontSendNotification);

    grMeterLabel.setText (
        juce::String (grDb, 1), // + " dB",
        juce::dontSendNotification);

    if (auto* modeParam = processorRef.apvts.getParameter (LA9AAudioProcessor::paramMode))
    {
        const bool isB = modeParam->getValue() > 0.5f;
        if (modeButton.getToggleState() != isB)
            modeButton.setToggleState (isB, juce::dontSendNotification);
    }
}
