/*
  ==============================================================================

    FilmstripLookAndFeel.h
    LA9A — Levelling Compressor
    Studio9 Plugins

    Two small custom LookAndFeel classes that draw a control from a vertical
    filmstrip image: N frames stacked top-to-bottom, each frame the full
    image width and (imageHeight / N) tall.

    Frame width/height are computed from the ACTUAL loaded image's
    dimensions at runtime, never hard-coded — so this works regardless of
    the filmstrip's real resolution (e.g. Local high-res originals
    vs. any preview/placeholder copy), as long as it's a single vertical
    column of equal-height frames.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// =============================================================================
/** Rotary knob drawn from a vertical filmstrip. Frame 0 (top of the strip)
    is the knob's minimum value/position; the last frame (bottom of the
    strip) is its maximum. Used for the Compress
    and Output dials (both share one instance of this class, same
    Knob_128x128_128f filmstrip).

    Disabled-state dimming is hand-coded here — JUCE's Component::setEnabled
    only auto-dims the DEFAULT LookAndFeel painting, not a custom-painted
    filmstrip frame (see the Output dial, disabled while Auto Level is on). */
class FilmstripKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripKnobLookAndFeel (const juce::Image& filmstrip, int numFrames)
        : image (filmstrip), frameCount (juce::jmax (1, numFrames)) {}

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                             float sliderPosProportional, float /*rotaryStartAngle*/,
                             float /*rotaryEndAngle*/, juce::Slider& slider) override
    {
        if (! image.isValid())
            return;

        const int frameWidth  = image.getWidth();
        const int frameHeight = image.getHeight() / frameCount;

        const int frameIndex = juce::jlimit (0, frameCount - 1,
            (int) std::lround (sliderPosProportional * (float) (frameCount - 1)));

        g.drawImage (image, x, y, width, height,
                      0, frameIndex * frameHeight, frameWidth, frameHeight);

        if (! slider.isEnabled())
        {
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (x, y, width, height);
        }
    }

private:
    juce::Image image;
    int frameCount;
};

// =============================================================================
/** Two-frame toggle switch drawn from a vertical filmstrip (frame 0 = top
    of the strip, frame 1 = bottom).

    trueMeansFrameZero controls which way the boolean toggle state maps to
    a frame:
      - Side-Chain / Auto-Level: checked ("On") = frame 0/top, unchecked
        ("Off") = frame 1/bottom  -> trueMeansFrameZero = true
      - Mode: checked ("B") = frame 1/bottom, unchecked ("A") = frame
        0/top -> trueMeansFrameZero = false (the mapping is the reverse of
        the other two switches) */
class FilmstripSwitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripSwitchLookAndFeel (const juce::Image& filmstrip, bool trueMeansFrameZeroIn)
        : image (filmstrip), trueMeansFrameZero (trueMeansFrameZeroIn) {}

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                             bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        if (! image.isValid())
            return;

        const int frameWidth  = image.getWidth();
        const int frameHeight = image.getHeight() / 2; // always a 2-frame strip

        const bool state      = button.getToggleState();
        const int  frameIndex = (state == trueMeansFrameZero) ? 0 : 1;

        const auto bounds = button.getLocalBounds();
        g.drawImage (image, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                      0, frameIndex * frameHeight, frameWidth, frameHeight);

        if (! button.isEnabled())
        {
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (bounds);
        }
    }

private:
    juce::Image image;
    bool trueMeansFrameZero;
};
