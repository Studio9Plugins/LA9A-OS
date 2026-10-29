/*
  ==============================================================================

    PluginProcessor.cpp
    LA9A — Levelling Compressor
    Studio9 Plugins

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
LA9AAudioProcessor::LA9AAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameters())
{
    paramInputGainPtr     = apvts.getRawParameterValue (paramInputGain);
    paramCompressRatioPtr = apvts.getRawParameterValue (paramCompressRatio);
    paramSideChainPtr     = apvts.getRawParameterValue (paramSideChain);
    paramModePtr          = apvts.getRawParameterValue (paramMode);
    paramOutputGainPtr    = apvts.getRawParameterValue (paramOutputGain);
    paramAutoGainPtr      = apvts.getRawParameterValue (paramAutoGain);
}

LA9AAudioProcessor::~LA9AAudioProcessor() {}

// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    LA9AAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input Gain — true pre-gain applied at the very start of processBlock(),
    // before metering, before the side-chain detector, and before
    // compression itself. So it also changes how hard the compressor is
    // driven, not just the displayed levels.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        paramInputGain, "Input Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    // Compress (Ratio). Displayed to the user as "1:x" — see editor for the
    // value-to-text mapping. Threshold is NOT a parameter (fixed internally,
    // see LevelingCompressor::kThresholdDb).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        paramCompressRatio, "Compress",
        juce::NormalisableRange<float> (1.1f, 8.0f, 0.01f), 1.1f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        paramSideChain, "Side-Chain", false)); // default Off

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        paramMode, "Mode",
        juce::StringArray { "A", "B" },
        0)); // default 0 = A 

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        paramOutputGain, "Output",
        juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        paramAutoGain, "Auto Level", false)); // default Off

    return { params.begin(), params.end() };
}

// =============================================================================
void LA9AAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Zero-latency processor — all stages are sample-by-sample with no
    // look-ahead buffers. Explicit call so hosts doing PDC (Plugin Delay
    // Compensation) have an accurate value to work with.
    setLatencySamples (0);

    compressor.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

// =============================================================================
void LA9AAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // ── Input Gain (-12..+12 dB, true pre-gain) ─────────────────────────────
    // Applied FIRST, directly to the buffer, before anything else reads or
    // measures it — so the input meters, the side-chain detector (and
    // therefore how hard the compressor is driven), and Auto Level's input
    // reference all see the signal AFTER Input Gain, not before. This is a 
    // genuine pre-gain stage, not just a metering  trim.
    const float inputGainLinear = juce::Decibels::decibelsToGain (paramInputGainPtr->load());
    buffer.applyGain (0, numSamples, inputGainLinear);

    // ── Input meter (post-Input-Gain, stereo) ───────────────────────────────
    if (numChannels > 0)
        inputMeterL.store (juce::Decibels::gainToDecibels (
            buffer.getRMSLevel (0, 0, numSamples), -60.0f));
    if (numChannels > 1)
        inputMeterR.store (juce::Decibels::gainToDecibels (
            buffer.getRMSLevel (1, 0, numSamples), -60.0f));
    else
        inputMeterR.store (inputMeterL.load());

    // ── Parameter snapshot (one block, cached atomic pointers) ────────────
    compressor.setRatio                (*paramCompressRatioPtr);
    compressor.setSideChainEnabled     (*paramSideChainPtr > 0.5f);
    compressor.setMode                 ((int) *paramModePtr);
    compressor.setOutputGainDb         (*paramOutputGainPtr);
    compressor.setAutoGainMatchEnabled (*paramAutoGainPtr > 0.5f);

    compressor.process (buffer);

    gainReductionMeter.store (compressor.getGainReductionDb());

    // ── Output meter (post-processing, stereo) ─────────────────────────────
    // Values > 0 dB indicate clipping — the editor's meter component should
    // latch a clip indicator on those internally, same convention as
    // VLA-One's FlatLevelMeter.
    if (numChannels > 0)
        outputMeterL.store (juce::Decibels::gainToDecibels (
            buffer.getRMSLevel (0, 0, numSamples), -60.0f));
    if (numChannels > 1)
        outputMeterR.store (juce::Decibels::gainToDecibels (
            buffer.getRMSLevel (1, 0, numSamples), -60.0f));
    else
        outputMeterR.store (outputMeterL.load());
}

// =============================================================================
void LA9AAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LA9AAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml.get() != nullptr)
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// =============================================================================
juce::AudioProcessorEditor* LA9AAudioProcessor::createEditor()
{
    return new LA9AAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LA9AAudioProcessor();
}
