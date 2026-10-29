/*
  ==============================================================================

    PluginProcessor.h
    LA9A — Levelling Compressor
    Studio9 Plugins

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "LevelingCompressor.h"

class LA9AAudioProcessor : public juce::AudioProcessor
{
public:
    LA9AAudioProcessor();
    ~LA9AAudioProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    bool supportsDoublePrecisionProcessing() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LA9A"; }

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()                        override { return 1;  }
    int  getCurrentProgram()                     override { return 0;  }
    void setCurrentProgram (int)                 override {}
    const juce::String getProgramName (int)      override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData)        override;
    void setStateInformation (const void* data, int sizeInBytes)  override;

    // ── Parameter IDs ─────────────────────────────────────────────────────
    // Centralised here (rather than string literals scattered through the
    // .cpp/editor) so processor and editor can't drift out of sync.
    static constexpr auto paramInputGain     = "INPUT_GAIN";
    static constexpr auto paramCompressRatio = "COMPRESS_RATIO";
    static constexpr auto paramSideChain     = "SIDE_CHAIN";
    static constexpr auto paramMode          = "MODE";
    static constexpr auto paramOutputGain    = "OUTPUT_GAIN";
    static constexpr auto paramAutoGain      = "AUTO_GAIN";

    // ── Parameter state ───────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // ── Metering — written on audio thread, read on message thread ────────
    // All dB values.
    //   inputMeterL/R      : POST-Input-Gain RMS (see processBlock — Input
    //                        Gain is applied before this is measured),
    //                        range -60..0 dB
    //   outputMeterL/R     : post-output-gain RMS, range -60..0+ dB (>0 = clip)
    //   gainReductionMeter : compressor GR, range -18..0 dB (always <= 0)
    std::atomic<float> inputMeterL         { -60.0f };
    std::atomic<float> inputMeterR         { -60.0f };
    std::atomic<float> outputMeterL        { -60.0f };
    std::atomic<float> outputMeterR        { -60.0f };
    std::atomic<float> gainReductionMeter  {   0.0f };

private:
    // Cached APVTS atomics — avoids string lookup each process block.
    std::atomic<float>* paramInputGainPtr     { nullptr };
    std::atomic<float>* paramCompressRatioPtr { nullptr };
    std::atomic<float>* paramSideChainPtr     { nullptr };
    std::atomic<float>* paramModePtr          { nullptr };
    std::atomic<float>* paramOutputGainPtr    { nullptr };
    std::atomic<float>* paramAutoGainPtr      { nullptr };

    LevelingCompressor compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LA9AAudioProcessor)
};
