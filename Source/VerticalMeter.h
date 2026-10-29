/*
  ==============================================================================

    VerticalMeter.h
    LA9A — Levelling Compressor
    Studio9 Plugins

    Reusable linear vertical meter bar: a fixed top-to-bottom colour gradient
    (defined once, across the component's full height) revealed by a filled
    rectangle whose size tracks the current level. No side scale/tick labels
    — per Vatche's brief, the numeric read-out lives in a separate Label
    underneath (see PluginEditor).

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VerticalMeter : public juce::Component
{
public:
    /** Bottom  = meter fills upward from the bottom as the level rises
                  toward fullDb (Input/Output convention).
        Top     = meter fills downward from the top as the level moves
                  toward fullDb (Gain Reduction convention — "resting" at
                  the top with 0 dB GR, extending down as reduction grows). */
    enum class FillOrigin { Bottom, Top };

    /** topColour/bottomColour: fixed gradient anchors for the bar's full
        height (top of component -> bottom of component), independent of
        how much is currently filled.
        restDb: level at which the meter reads empty (fraction 0).
        fullDb: level at which the meter reads completely filled (fraction 1).
        (For Input/Output, restDb=-60, fullDb=0. For GR, restDb=0, fullDb=-18 —
        see PluginEditor for the actual values used.) */
    VerticalMeter (juce::Colour topColour, juce::Colour bottomColour,
                    FillOrigin fillOrigin, float restDb, float fullDb);

    void setLevelDb (float newLevelDb);

    void paint (juce::Graphics&) override;

private:
    juce::Colour topColour, bottomColour;
    FillOrigin   fillOrigin;
    float        restDb, fullDb;
    float        levelDb { -60.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalMeter)
};
