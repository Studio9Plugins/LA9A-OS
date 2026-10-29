/*
  ==============================================================================

    LevelingCompressor.h
    LA9A — Levelling Compressor
    Studio9 Plugins

    Core DSP engine, isolated from PluginProcessor so it can be bench-tested
    and iterated on independently of the plugin wrapper/UI. 

    DRAFT / PLACEHOLDER STATUS:
      - Threshold is fixed at kThresholdDb below. Flagged as likely to need
        retuning after real audio testing — it's a single named constant so
        that's a one-line change.
      - Auto Gain Match uses a simple slow-RMS input/output comparison.

  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>

/**
    Single-threshold, ratio-driven leveling compressor.

    Behavior (per the LA9A spec):
      - Threshold is fixed (not user-exposed). Only signal ABOVE threshold
        is compressed, in relation to Ratio — this is deliberately different
        from compressors that also shape signal below threshold.
      - Ratio (1:1.1 .. 1:6) also drives Attack/Release timing via linear
        interpolation, direction controlled by Mode A/B (see
        attackReleaseMsForRatio()).
      - Gain is derived from a per-sample peak detector, smoothed by an
        asymmetric one-pole attack/release filter — the target gain is never
        applied instantly, which is what actually prevents zipper noise /
        clicking / unnatural pumping.
      - An internally-parameterized side-chain low-cut (160 Hz, 12 dB/oct)
        can be switched into the detector path only — it never touches the
        audio signal itself. Frequency/slope are kept as fields (not literal
        constants inline) so a future "user-adjustable HPF" feature is just
        a matter of exposing them as a parameter, not a redesign.
      - Auto Gain Match, when enabled, compares slow (~300 ms class)
        smoothed input/output RMS and applies the difference as an
        additional, separately-smoothed compensation gain — deliberately a
        much slower process than the main Attack/Release envelope, so it
        doesn't itself introduce pumping. It is held at 0 dB whenever the
        input signal is at/below threshold (see .cpp), and when enabled it
        REPLACES the manual Output control rather than adding to it.
*/
class LevelingCompressor
{
public:
    LevelingCompressor() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    // ── Parameter setters — call once per block from processBlock() ─────────
    void setRatio                (float newRatio);        // 1.1 .. 6.0
    void setMode                 (int newMode);             // 0 = A, 1 = B
    void setSideChainEnabled     (bool shouldBeEnabled);
    void setAutoGainMatchEnabled (bool shouldBeEnabled);
    void setOutputGainDb         (float newOutputGainDb);   // 0 .. +18 dB (ignored while Auto Level is on — see class comment)

    /** Processes numSamples of interleaved-by-channel audio in place. */
    void process (juce::AudioBuffer<float>& buffer);

    // ── Metering read-back (message thread) ────────────────────────────────
    float getGainReductionDb() const noexcept { return currentGainReductionDb; }

    // Internal side-chain filter frequency/slope — not user-exposed yet, but
    // kept here (rather than as inline literals) so a future parameter can
    // drive these directly. See class comment above.
    static constexpr float kSideChainFreqHz       = 160.0f;
    static constexpr float kSideChainQButterworth = 0.70710678f; // 12 dB/oct

    static constexpr double kAutoGainWindowSeconds = 0.3;

private:
    /** Attack/release times (ms) for the current ratio + mode. */
    void attackReleaseMsForRatio (float ratioValue, int modeValue,
                                   float& attackMsOut, float& releaseMsOut) const;

    // Fixed threshold — see class comment: this is expected to need real-world
    // retuning, kept as a single named constant deliberately.
    static constexpr float kThresholdDb = -24.0f;

    // Ratio range — see class comment.
    static constexpr float kRatioMin = 1.1f;
    static constexpr float kRatioMax = 6.0f;

    double sampleRate { 44100.0 };

    // Parameters (block-rate snapshot; see setters above).
    float ratio            { 1.1f };
    int   mode              { 0 };
    bool  sideChainEnabled  { false };
    bool  autoGainEnabled   { false };
    float outputGainDb      { 0.0f };

    // Main compressor envelope state.
    float smoothedGainDb         { 0.0f };
    float currentGainReductionDb { 0.0f };

    // Side-chain detector filters (audio-path untouched; these only see a
    // copy of the signal used for level detection).
    juce::dsp::IIR::Filter<float> sideChainFilterL, sideChainFilterR;

    // Auto Gain Match state — deliberately slower/separate from the main
    // envelope above. ~300 ms class smoothing per Vatche's brief. Held at
    // 0 dB whenever the input is at/below threshold (see process()).
    float inputRmsSlowDb   { -60.0f };
    float outputRmsSlowDb  { -60.0f };
    float autoGainCompDb   { 0.0f };
    float autoGainCoeff    { 0.0f }; // computed in prepare()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelingCompressor)
};
