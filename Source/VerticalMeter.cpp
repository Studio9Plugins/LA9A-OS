/*
  ==============================================================================

    VerticalMeter.cpp
    LA9A — Levelling Compressor
    Studio9 Plugins

  ==============================================================================
*/

#include "VerticalMeter.h"

// =============================================================================
VerticalMeter::VerticalMeter (juce::Colour top, juce::Colour bottom,
                                FillOrigin origin, float restDbIn, float fullDbIn)
    : topColour (top), bottomColour (bottom), fillOrigin (origin),
      restDb (restDbIn), fullDb (fullDbIn)
{
    // Meters are read-only display — don't intercept clicks/drags.
    setInterceptsMouseClicks (false, false);
}

void VerticalMeter::setLevelDb (float newLevelDb)
{
    if (! juce::approximatelyEqual (levelDb, newLevelDb))
    {
        levelDb = newLevelDb;
        repaint();
    }
}

void VerticalMeter::paint (juce::Graphics& g)
{
    const auto fullBounds = getLocalBounds().toFloat();

    // Unlit background track.
    g.setColour (juce::Colours::black);
    g.fillRect (fullBounds);

    const float fraction = juce::jlimit (0.0f, 1.0f,
        (levelDb - restDb) / (fullDb - restDb));
    const float filledHeight = fraction * fullBounds.getHeight();

    if (filledHeight <= 0.0f)
        return;

    const juce::Rectangle<float> filledRect (
        fullBounds.getX(),
        fillOrigin == FillOrigin::Bottom ? fullBounds.getBottom() - filledHeight
                                          : fullBounds.getY(),
        fullBounds.getWidth(),
        filledHeight);

    // Gradient is anchored to the FULL bar geometry (component top to
    // bottom), not the filled rect — so the colour at a given screen
    // position stays fixed as the fill grows or shrinks; only more or less
    // of the gradient becomes visible.
    juce::ColourGradient gradient (topColour, fullBounds.getX(), fullBounds.getY(),
                                     bottomColour, fullBounds.getX(), fullBounds.getBottom(),
                                     false);
    g.setGradientFill (gradient);
    g.fillRect (filledRect);
}
