/*
  ==============================================================================

    LevelingCompressor.cpp
    LA9A — Levelling Compressor
    Studio9 Plugins

  ==============================================================================
*/

#include "LevelingCompressor.h"
#include <cmath>

namespace
{
/** Per-sample coeff for y += c * (x - y) matching time constant tau (seconds).
    Same approach as VLA-One's onePoleCoeffForTau(). */
float onePoleCoeffForTau (double tauSeconds, double sampleRate)
{
    const double denom = tauSeconds * sampleRate;
    if (denom <= 0.0)
        return 1.0f;
    return (float) (1.0 - std::exp (-1.0 / denom));
}

/** Coefficient for a one-pole filter updating once per block (block-rate,
    not per-sample) — used for the slower Auto Gain Match smoothing. */
float onePoleCoeffForTauBlockRate (double tauSeconds, double blockSeconds)
{
    if (tauSeconds <= 0.0)
        return 1.0f;
    return (float) (1.0 - std::exp (-blockSeconds / tauSeconds));
}
} // namespace

// =============================================================================
void LevelingCompressor::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1; // filters run per-channel, mono each

    sideChainFilterL.prepare (spec);
    sideChainFilterR.prepare (spec);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        sampleRate, kSideChainFreqHz, kSideChainQButterworth);
    sideChainFilterL.coefficients = coeffs;
    sideChainFilterR.coefficients = coeffs;

    const double blockSeconds = juce::jmax (samplesPerBlock, 1) / sampleRate;
    autoGainCoeff = onePoleCoeffForTauBlockRate (kAutoGainWindowSeconds, blockSeconds);

    juce::ignoreUnused (numChannels);

    reset();
}

void LevelingCompressor::reset()
{
    sideChainFilterL.reset();
    sideChainFilterR.reset();

    smoothedGainDb         = 0.0f;
    currentGainReductionDb = 0.0f;
    inputRmsSlowDb         = -60.0f;
    outputRmsSlowDb        = -60.0f;
    autoGainCompDb         = 0.0f;
}

// =============================================================================
void LevelingCompressor::setRatio (float newRatio)
{
    ratio = juce::jlimit (kRatioMin, kRatioMax, newRatio);
}

void LevelingCompressor::setMode (int newMode)
{
    mode = newMode == 1 ? 1 : 0;
}

void LevelingCompressor::setSideChainEnabled (bool shouldBeEnabled)
{
    sideChainEnabled = shouldBeEnabled;
}

void LevelingCompressor::setAutoGainMatchEnabled (bool shouldBeEnabled)
{
    autoGainEnabled = shouldBeEnabled;
}

void LevelingCompressor::setOutputGainDb (float newOutputGainDb)
{
    outputGainDb = juce::jlimit (0.0f, 18.0f, newOutputGainDb);
}

// =============================================================================
void LevelingCompressor::attackReleaseMsForRatio (float ratioValue, int modeValue,
                                                    float& attackMsOut, float& releaseMsOut) const
{
    // Normalise ratio position across its range, 0 = softest (1:1.1),
    // 1 = hardest (1:6).
    const float t = juce::jlimit (0.0f, 1.0f,
        (ratioValue - kRatioMin) / (kRatioMax - kRatioMin));

    // Mode A: fast->slow as ratio increases. Mode B: slow->fast (reversed),
    // using Vatche's revised Mode B range (80ms->5ms attack, 200ms->40ms
    // release) rather than a straight mirror of Mode A's numbers.
    if (modeValue == 0)
    {
        attackMsOut  = juce::jmap (t, 0.0f, 1.0f, 10.0f, 40.0f);
        releaseMsOut = juce::jmap (t, 0.0f, 1.0f, 60.0f, 500.0f);
    }
    else
    {
        attackMsOut  = juce::jmap (t, 0.0f, 1.0f, 80.0f, 5.0f);
        releaseMsOut = juce::jmap (t, 0.0f, 1.0f, 200.0f, 40.0f);
    }
}

// =============================================================================
void LevelingCompressor::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || numChannels <= 0)
        return;

    // ── Pre-compression RMS (for Auto Gain Match) ──────────────────────────
    float preRms = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        preRms += buffer.getRMSLevel (ch, 0, numSamples);
    preRms /= (float) numChannels;
    const float preRmsDb = juce::Decibels::gainToDecibels (preRms, -60.0f);

    // ── Attack/Release timing for this block (ratio + mode driven) ────────
    float attackMs = 10.0f, releaseMs = 60.0f;
    attackReleaseMsForRatio (ratio, mode, attackMs, releaseMs);
    const float attackCoeff  = onePoleCoeffForTau (attackMs  / 1000.0, sampleRate);
    const float releaseCoeff = onePoleCoeffForTau (releaseMs / 1000.0, sampleRate);

    // ── Main compressor loop ───────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        const float L = buffer.getSample (0, i);
        const float R = (numChannels > 1) ? buffer.getSample (1, i) : L;

        // Detector signal: either the raw L/R or, if Side-Chain is on, the
        // same signal passed through the 160 Hz / 12 dB-oct low-cut. Either
        // way this ONLY feeds the detector — the audio path itself is
        // untouched by the filter.
        float detL = L, detR = R;
        if (sideChainEnabled)
        {
            detL = sideChainFilterL.processSample (L);
            detR = sideChainFilterR.processSample (R);
        }

        const float detector   = juce::jmax (std::abs (detL), std::abs (detR));
        const float detectorDb = juce::Decibels::gainToDecibels (detector, -60.0f);

        // Gain computer: only signal ABOVE threshold is compressed, per Ratio.
        float targetGrDb = 0.0f;
        if (detectorDb > kThresholdDb)
        {
            const float over           = detectorDb - kThresholdDb;
            const float compressedOver = over / ratio;
            targetGrDb = compressedOver - over; // <= 0
        }

        // Asymmetric attack/release smoothing — the gain is NEVER applied
        // instantly. Moving toward more reduction uses Attack; recovering
        // toward less reduction uses Release.
        const float coeff = (targetGrDb < smoothedGainDb) ? attackCoeff : releaseCoeff;
        smoothedGainDb += coeff * (targetGrDb - smoothedGainDb);

        const float linGain = juce::Decibels::decibelsToGain (smoothedGainDb);
        buffer.setSample (0, i, L * linGain);
        if (numChannels > 1)
            buffer.setSample (1, i, R * linGain);
    }

    currentGainReductionDb = juce::jlimit (-18.0f, 0.0f, smoothedGainDb);

    // ── Post-compression RMS (for Auto Gain Match) ─────────────────────────
    float postRms = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        postRms += buffer.getRMSLevel (ch, 0, numSamples);
    postRms /= (float) numChannels;
    const float postRmsDb = juce::Decibels::gainToDecibels (postRms, -60.0f);

    // ── Auto Gain Match ─────────────────────────────────────────────────────
    // Deliberately slow (~300 ms class), separate from the main envelope
    // above, so the compensation itself doesn't pump. Held at 0 dB while the
    // input is at/below threshold — with nothing being compressed, RMS
    // comparisons there are just noise, and per design plan, no auto-gain
    // should be applied in that case. First-pass implementation — TODO:
    // validate by ear / retune once audible.
    const bool inputAboveThreshold = preRmsDb > kThresholdDb;
    if (inputAboveThreshold)
    {
        inputRmsSlowDb  += autoGainCoeff * (preRmsDb  - inputRmsSlowDb);
        outputRmsSlowDb += autoGainCoeff * (postRmsDb - outputRmsSlowDb);
    }
    const float targetCompDb = inputAboveThreshold
        ? juce::jlimit (-18.0f, 18.0f, inputRmsSlowDb - outputRmsSlowDb)
        : 0.0f;
    autoGainCompDb += autoGainCoeff * (targetCompDb - autoGainCompDb);

    // ── Output stage ─────────────────────────────────────────────────────
    // When Auto Level is ON it REPLACES the manual Output control rather
    // than adding to it (otherwise the two fight each other / double up) —
    // Output is effectively bypassed while Auto Level
    // is engaged.
    const float totalOutputDb = autoGainEnabled ? autoGainCompDb : outputGainDb;
    const float outputLinear  = juce::Decibels::decibelsToGain (totalOutputDb);

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.applyGain (ch, 0, numSamples, outputLinear);
}
