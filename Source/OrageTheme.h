#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>
#include "OrageLogoFont.h"

//==============================================================================
// ORAGE AUDIO DESIGN SYSTEM — EVOLVER SERIES AESTHETIC
// Inspired by the Orage Evolver Analog Hardware:
// Anodized brushed aluminum faceplate, high-contrast monochrome silkscreen,
// vintage fluted black pointer knobs, warm tungsten amber jewel LEDs,
// and precision tactile typography.
//==============================================================================
namespace OrageTheme
{
    // 1. Brushed Aluminum & Chassis Tones (Bright Anodized Silver)
    const juce::Colour BG_CHASSIS_DARK   { 0xFF0D0E10 }; // Deep studio surround / rack chassis
    const juce::Colour ALUMINUM_SPECULAR { 0xFFFFFFFF }; // Razor-thin specular top highlight
    const juce::Colour ALUMINUM_LIGHT    { 0xFFE8EBF0 }; // Upper brushed aluminum gradient
    const juce::Colour ALUMINUM_BASE     { 0xFFD8DCE3 }; // Natural anodized silver aluminum faceplate
    const juce::Colour ALUMINUM_DARK     { 0xFFC6CBD4 }; // Lower brushed aluminum shading
    const juce::Colour ALUMINUM_BEVEL    { 0xFFA4AAB5 }; // Machined perimeter bevel line
    const juce::Colour SHADOW_SOFT       { 0x59000000 }; // 35% black ambient drop shadow
    const juce::Colour SHADOW_DEEP       { 0x8C000000 }; // 55% black shadow for inset bays

    // 2. High-Contrast Typography & Silkscreen Markings (Jet-Black on Silver Aluminum)
    const juce::Colour TEXT_DARK         { 0xFF000000 }; // Pure jet black for maximum silkscreen legibility
    const juce::Colour TEXT_LABEL        { 0xFF000000 }; // Pure jet black for panel labels
    const juce::Colour TEXT_MUTED        { 0xFF14161A }; // Deep charcoal for secondary telemetry & markings
    const juce::Colour TEXT_BRIGHT       { 0xFFFFFFFF }; // 100% crisp stark white for text on black buttons/pads
    const juce::Colour PANEL_LINE        { 0xFF101215 }; // Crisp technical silkscreen divider line
    const juce::Colour PANEL_LINE_ENGRAVE{ 0x33FFFFFF }; // 3D engraved white hairline highlight

    // 3. Vintage Warm Amber Jewel LED & Cobalt Blue Jewel LED (Evolver Signature Hardware)
    const juce::Colour AMBER_LED         { 0xFFFFAC20 }; // Authentic warm tungsten amber LED
    const juce::Colour AMBER_LED_BRIGHT  { 0xFFFFD278 }; // Hot center core of the amber lens
    const juce::Colour AMBER_LED_GLOW    { 0x55FFAC20 }; // Warm ambient halo glow

    const juce::Colour BLUE_LED          { 0xFF1E6BFF }; // Radiant Cobalt Blue Jewel Lens (Evolver Overdrive)
    const juce::Colour BLUE_LED_BRIGHT   { 0xFF70B5FF }; // Hot center core of the blue lens
    const juce::Colour BLUE_LED_GLOW     { 0x551E6BFF }; // Royal blue halo glow

    // 4. Recessed Rack Bays (Track Strips & Sequencer Matrix)
    const juce::Colour TRACK_BAY_TOP     { 0xFF1C1E22 }; // Recessed bay dark top shade
    const juce::Colour TRACK_BAY_BOT     { 0xFF141518 }; // Recessed bay deep bottom shade
    const juce::Colour TRACK_BAY_RIM     { 0xFF35383E }; // Inset machined rim bevel
    const juce::Colour PAD_OFF           { 0xFF222428 }; // Inactive step pad (matte charcoal)
    const juce::Colour PAD_OFF_RIM       { 0xFF18191B }; // Inactive step pad border
    const juce::Colour PAD_ACTIVE        { 0xFFFFFFFF }; // Active step pad (crisp stark white, black bold text)
    const juce::Colour PAD_ACTIVE_ACCENT { 0xFFFFF5E0 }; // Warm-white active pad for accented steps

    // 5. Tactile Black Knobs (Evolver Hardware Fluted Pointer Knobs)
    const juce::Colour KNOB_BODY_TOP     { 0xFF2A2C30 }; // Fluted grip top facet
    const juce::Colour KNOB_BODY_BOT     { 0xFF161719 }; // Fluted grip bottom shadow
    const juce::Colour KNOB_NEEDLE       { 0xFFFFFFFF }; // Crisp white pointer needle line

    // Authentic Fugaz One Logo Font for "Orage"
    inline juce::Font getOrageLogoFont (float size)
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (
            OrageAssets::fugazOneData, static_cast<size_t> (OrageAssets::fugazOneSize));
        if (typeface != nullptr)
            return juce::Font (juce::FontOptions (typeface).withHeight (size));
        return juce::Font (juce::FontOptions (size, juce::Font::bold));
    }

    // Authentic Montserrat Semi-Bold (600) for all primary controls, values, notes, and buttons
    inline juce::Font montserratSemiBold (float size, float kerning = 0.03f)
    {
        static auto tf = []() -> juce::Typeface::Ptr {
            juce::File f ("C:/Users/daniele/AppData/Local/Microsoft/Windows/Fonts/Montserrat-VariableFont_wght.ttf");
            if (f.existsAsFile())
            {
                juce::MemoryBlock mb;
                f.loadFileAsData (mb);
                if (mb.getSize() > 0)
                    return juce::Typeface::createSystemTypefaceFor (mb.getData(), mb.getSize());
            }
            return nullptr;
        }();

        juce::Font font;
        if (tf != nullptr)
            font = juce::Font (juce::FontOptions (tf).withHeight (size)).boldened();
        else
            font = juce::Font (juce::FontOptions ("Montserrat", size, juce::Font::bold));

        return (kerning != 0.0f) ? font.withExtraKerningFactor (kerning) : font;
    }

    // Authentic Montserrat Italic (400) for subtitles, secondary indicators, and telemetry
    inline juce::Font montserratItalic (float size, float kerning = 0.0f)
    {
        static auto tf = []() -> juce::Typeface::Ptr {
            juce::File f ("C:/Users/daniele/AppData/Local/Microsoft/Windows/Fonts/Montserrat-Italic-VariableFont_wght.ttf");
            if (f.existsAsFile())
            {
                juce::MemoryBlock mb;
                f.loadFileAsData (mb);
                if (mb.getSize() > 0)
                    return juce::Typeface::createSystemTypefaceFor (mb.getData(), mb.getSize());
            }
            return nullptr;
        }();

        juce::Font font;
        if (tf != nullptr)
            font = juce::Font (juce::FontOptions (tf).withHeight (size)).italicised();
        else
            font = juce::Font (juce::FontOptions ("Montserrat", size, juce::Font::italic));

        return (kerning != 0.0f) ? font.withExtraKerningFactor (kerning) : font;
    }

    // Default font redirects to Montserrat Semi-Bold (600)
    inline juce::Font font (float size, bool bold = true, float kerning = 0.03f)
    {
        auto f = montserratSemiBold (size, kerning);
        return bold ? f.boldened() : f;
    }


    // Helper to draw an authentic Evolver Jewel Lamp (Amber or Cobalt Blue)
    inline void drawJewelLed (juce::Graphics& g, float cx, float cy, float radius,
                              juce::Colour ledCol, juce::Colour brightCol, juce::Colour glowCol)
    {
        // 1. Halo ambient glow
        g.setColour (glowCol);
        g.fillEllipse (cx - radius - 4.5f, cy - radius - 4.5f, (radius + 4.5f) * 2.0f, (radius + 4.5f) * 2.0f);

        // 2. Machined dark bezel rim
        g.setColour (juce::Colour (0xFF1E2024));
        g.fillEllipse (cx - radius - 1.5f, cy - radius - 1.5f, (radius + 1.5f) * 2.0f, (radius + 1.5f) * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.40f));
        g.drawEllipse (cx - radius - 1.5f, cy - radius - 1.5f, (radius + 1.5f) * 2.0f, (radius + 1.5f) * 2.0f, 0.9f);

        // 3. Convex jewel lens gradient
        juce::ColourGradient lensGrad (brightCol, cx - radius * 0.35f, cy - radius * 0.45f,
                                       ledCol.darker (0.25f), cx + radius * 0.35f, cy + radius * 0.45f, false);
        g.setGradientFill (lensGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // 4. Razor specular core highlight
        g.setColour (juce::Colours::white.withAlpha (0.80f));
        g.fillEllipse (cx - radius * 0.40f, cy - radius * 0.50f, radius * 0.65f, radius * 0.50f);
    }

    // Helper to draw realistic 19" rack slotted mounting holes with dark oxide washer
    inline void drawRackSlot (juce::Graphics& g, float cx, float cy, float w = 7.0f, float h = 13.0f)
    {
        auto slotRect = juce::Rectangle<float> (cx - w * 0.5f, cy - h * 0.5f, w, h);

        // Outer washer shadow
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (slotRect.expanded (1.8f), 3.5f);

        // Stainless / nickel washer rim
        juce::ColourGradient washerGrad (juce::Colour (0xFFDCE0E6), cx, cy - h * 0.5f,
                                         juce::Colour (0xFFA0A6B2), cx, cy + h * 0.5f, false);
        g.setGradientFill (washerGrad);
        g.drawRoundedRectangle (slotRect.expanded (1.0f), 3.5f, 1.0f);

        // Inner black cut slot
        g.setColour (juce::Colour (0xFF0A0C0E));
        g.fillRoundedRectangle (slotRect, 2.5f);

        // Screw inside slot
        g.setColour (juce::Colour (0xFF2C2F36));
        g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        g.setColour (juce::Colour (0xFF08090A));
        g.fillEllipse (cx - 1.2f, cy - 1.2f, 2.4f, 2.4f);
    }

    // Helper to draw authentic Evolver hairline framed sections with embedded script title
    inline void drawFramedSection (juce::Graphics& g, juce::Rectangle<float> bounds,
                                   const juce::String& title, juce::Colour frameCol, juce::Colour textCol)
    {
        const float corner = 4.0f;
        const float textW = static_cast<float> (title.length()) * 7.2f + 16.0f;
        const float midX = bounds.getCentreX();



        // Hairline outline with gap at top center for title
        juce::Path p;
        p.startNewSubPath (midX + textW * 0.5f, bounds.getY());
        p.lineTo (bounds.getRight() - corner, bounds.getY());
        p.addArc (bounds.getRight() - corner * 2.0f, bounds.getY(), corner * 2.0f, corner * 2.0f, 0.0f, juce::MathConstants<float>::halfPi);
        p.lineTo (bounds.getRight(), bounds.getBottom() - corner);
        p.addArc (bounds.getRight() - corner * 2.0f, bounds.getBottom() - corner * 2.0f, corner * 2.0f, corner * 2.0f, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);
        p.lineTo (bounds.getX() + corner, bounds.getBottom());
        p.addArc (bounds.getX(), bounds.getBottom() - corner * 2.0f, corner * 2.0f, corner * 2.0f, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
        p.lineTo (bounds.getX(), bounds.getY() + corner);
        p.addArc (bounds.getX(), bounds.getY(), corner * 2.0f, corner * 2.0f, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
        p.lineTo (midX - textW * 0.5f, bounds.getY());

        g.setColour (frameCol);
        g.strokePath (p, juce::PathStrokeType (0.9f));

        // Title centered in the gap
        g.setColour (textCol);
        g.setFont (montserratItalic (11.0f));
        g.drawText (title, bounds.getX(), bounds.getY() - 7.0f, bounds.getWidth(), 16.0f, juce::Justification::centred);
    }


    // Helper to render recessed well in the aluminum plate
    inline void drawRecessedWell (juce::Graphics& g, juce::Rectangle<float> r, float corner = 5.0f,
                                  juce::Colour bg = juce::Colour (0xFFD0D4D9))
    {
        // 1. Recessed dark fill
        g.setColour (bg.darker (0.10f));
        g.fillRoundedRectangle (r, corner);

        // 2. Inner shadow on top & left
        juce::Path perimeter;
        perimeter.addRoundedRectangle (r.getX() + 0.5f, r.getY() + 0.5f, r.getWidth() - 1.0f, r.getHeight() - 1.0f, corner);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (perimeter, juce::PathStrokeType (1.2f));

        // 3. Bottom specular bevel
        juce::Path botRim;
        botRim.addRoundedRectangle (r.getX() + corner * 0.5f, r.getBottom() - 1.0f, r.getWidth() - corner, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.40f));
        g.fillPath (botRim);
    }

    // Helper to draw realistic black hex cap screws at rack corners
    inline void drawRackScrew (juce::Graphics& g, float cx, float cy, float radius = 5.5f)
    {
        // Outer washer/hole shadow
        g.setColour (juce::Colours::black.withAlpha (0.60f));
        g.fillEllipse (cx - radius - 0.5f, cy - radius, (radius + 0.5f) * 2.0f, (radius + 0.5f) * 2.0f);

        // Screw head: Dark oxide steel
        juce::ColourGradient screwGrad (juce::Colour (0xFF3A3D42), cx - radius * 0.4f, cy - radius * 0.5f,
                                        juce::Colour (0xFF141517), cx + radius * 0.4f, cy + radius * 0.5f, false);
        g.setGradientFill (screwGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Hex socket indentation
        const float hexR = radius * 0.52f;
        juce::Path hex;
        for (int i = 0; i < 6; ++i)
        {
            const float a = static_cast<float> (i) * juce::MathConstants<float>::pi / 3.0f;
            const float hx = cx + hexR * std::cos (a);
            const float hy = cy + hexR * std::sin (a);
            if (i == 0) hex.startNewSubPath (hx, hy);
            else        hex.lineTo (hx, hy);
        }
        hex.closeSubPath();
        g.setColour (juce::Colour (0xFF08090A));
        g.fillPath (hex);

        // Subtle specular highlight on top rim of screw
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawEllipse (cx - radius + 0.5f, cy - radius + 0.5f, (radius - 0.5f) * 2.0f, (radius - 0.5f) * 2.0f, 0.7f);
    }
}

//==============================================================================
// Custom LookAndFeel matching Orage Evolver Hardware Aesthetics
//==============================================================================
class OrageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrageLookAndFeel()
    {
        using namespace OrageTheme;
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId,       TEXT_DARK);
        setColour (juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId,      TEXT_DARK);

        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xFF1A1C1F));
        setColour (juce::PopupMenu::textColourId,                  TEXT_BRIGHT);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF303338));
        setColour (juce::PopupMenu::highlightedTextColourId,       AMBER_LED);

        setColour (juce::TextButton::buttonColourId,   juce::Colour (0xFF1E2023));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF111214));
        setColour (juce::TextButton::textColourOffId,  TEXT_BRIGHT);
        setColour (juce::TextButton::textColourOnId,   AMBER_LED);

        setColour (juce::Label::textColourId,          TEXT_LABEL);
        setColour (juce::Slider::textBoxTextColourId,  juce::Colours::white); // 100% stark pure white for values
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xFF4A5162)); // Precision machined bezel border
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xFF0D0F13)); // High-contrast dark inset bezel
    }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (OrageTheme::font (11.5f, true, 0.02f));
        l->setColour (juce::Label::textColourId, juce::Colours::white);
        l->setColour (juce::Label::backgroundColourId, juce::Colour (0xFF0D0F13));
        l->setColour (juce::Label::outlineColourId, juce::Colour (0xFF4A5162));
        l->setJustificationType (juce::Justification::centred);
        return l;
    }



    struct KnobCache
    {
        int width = 0;
        int height = 0;
        juce::Image image;
        float cx = 0.0f;
        float cy = 0.0f;
        float radius = 0.0f;
        float arcRadius = 0.0f;
    };

    std::unordered_map<const juce::Slider*, KnobCache> knobCaches;

    //--------------------------------------------------------------------------
    // EVOLVER FLUTED BLACK POINTER KNOB
    // - Recessed circular well in the aluminum plate
    // - Outer skirt with circular tick ring
    // - 6 Fluted scallops around knob body (tactile knurled grip)
    // - Brushed black top cap
    // - Stark white needle pointer line with rounded tip
    // - Precision warm amber / white value arc
    //--------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, const float startAngle, const float endAngle,
                           juce::Slider& slider) override
    {
        using namespace OrageTheme;
        auto& cache = knobCaches[&slider];

        if (cache.image.isNull() || cache.width != width || cache.height != height)
        {
            cache.width = width;
            cache.height = height;
            cache.cx = static_cast<float> (width) * 0.5f;
            cache.cy = static_cast<float> (height) * 0.38f;
            cache.radius = static_cast<float> (width) * 0.30f;
            cache.arcRadius = cache.radius * 1.18f;


            cache.image = juce::Image (juce::Image::ARGB, width, height, true);
            juce::Graphics imgG (cache.image);

            const float cx = cache.cx;
            const float cy = cache.cy;
            const float radius = cache.radius;
            const float diameter = radius * 2.0f;

            // 1. Recessed aluminum mounting well beneath the knob
            const float wellRadius = radius * 1.25f;
            juce::ColourGradient wellGrad (juce::Colours::black.withAlpha (0.28f), cx, cy - wellRadius,
                                           juce::Colours::transparentBlack, cx, cy + wellRadius, false);
            imgG.setGradientFill (wellGrad);
            imgG.fillEllipse (cx - wellRadius, cy - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f);

            // Subtle well bottom specular rim
            juce::Path botWellRim;
            botWellRim.addCentredArc (cx, cy, wellRadius - 0.5f, wellRadius - 0.5f, 0.0f,
                                      juce::MathConstants<float>::pi * 0.25f,
                                      juce::MathConstants<float>::pi * 0.75f, true);
            imgG.setColour (juce::Colours::white.withAlpha (0.45f));
            imgG.strokePath (botWellRim, juce::PathStrokeType (1.0f));

            // 2. Knob Drop Shadow (directional from top-left)
            const float shadowOffX = diameter * 0.06f;
            const float shadowOffY = diameter * 0.10f;
            for (int s = 6; s >= 1; --s)
            {
                const float t = static_cast<float> (s) / 6.0f;
                const float exp = t * (diameter * 0.25f);
                const float alpha = 0.45f * (1.0f - t * t) / 1.8f;
                imgG.setColour (juce::Colours::black.withAlpha (alpha));
                imgG.fillEllipse (cx + shadowOffX * t - radius - exp,
                                  cy + shadowOffY * t - radius - exp,
                                  (radius + exp) * 2.0f, (radius + exp) * 2.0f);
            }

            // 3. Outer Knob Skirt (Deep matte black with fine chamfer)
            const float skirtRadius = radius * 1.04f;
            juce::ColourGradient skirtGrad (juce::Colour (0xFF222427), cx - skirtRadius * 0.5f, cy - skirtRadius * 0.6f,
                                            juce::Colour (0xFF111214), cx + skirtRadius * 0.5f, cy + skirtRadius * 0.6f, false);
            imgG.setGradientFill (skirtGrad);
            imgG.fillEllipse (cx - skirtRadius, cy - skirtRadius, skirtRadius * 2.0f, skirtRadius * 2.0f);

            // 4. Fluted Body (6 Scallop grips matching the Evolver hardware knobs)
            juce::Path flutedBody;
            const int numFlutes = 6;
            const float innerFluteR = radius * 0.88f;
            const float outerFluteR = radius * 0.98f;

            for (int i = 0; i < numFlutes * 2; ++i)
            {
                const float a = static_cast<float> (i) * juce::MathConstants<float>::pi / static_cast<float> (numFlutes);
                const float r = (i % 2 == 0) ? outerFluteR : innerFluteR;
                const float px = cx + r * std::cos (a);
                const float py = cy + r * std::sin (a);
                if (i == 0) flutedBody.startNewSubPath (px, py);
                else        flutedBody.lineTo (px, py);
            }
            flutedBody.closeSubPath();

            juce::ColourGradient fluteGrad (juce::Colour (0xFF32353A), cx - radius * 0.5f, cy - radius * 0.5f,
                                            juce::Colour (0xFF151618), cx + radius * 0.5f, cy + radius * 0.5f, false);
            imgG.setGradientFill (fluteGrad);
            imgG.fillPath (flutedBody);

            // 5. Center Cap: Brushed black anodized disk with precision machined perimeter rim
            const float capRadius = radius * 0.74f;
            juce::ColourGradient capGrad (juce::Colour (0xFF242629), cx - capRadius * 0.4f, cy - capRadius * 0.4f,
                                          juce::Colour (0xFF121315), cx + capRadius * 0.4f, cy + capRadius * 0.4f, false);
            imgG.setGradientFill (capGrad);
            imgG.fillEllipse (cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f);

            // Cap bevel rim
            imgG.setColour (juce::Colours::white.withAlpha (0.18f));
            imgG.drawEllipse (cx - capRadius, cy - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);

            // Top specular highlight arc on cap rim
            juce::Path capHighlight;
            capHighlight.addCentredArc (cx, cy, capRadius - 0.5f, capRadius - 0.5f, 0.0f,
                                        juce::MathConstants<float>::pi * 1.25f,
                                        juce::MathConstants<float>::pi * 1.75f, true);
            imgG.setColour (juce::Colours::white.withAlpha (0.35f));
            imgG.strokePath (capHighlight, juce::PathStrokeType (1.0f));

            // Arc guide track (subtle dark line)
            juce::Path track;
            track.addCentredArc (cx, cy, cache.arcRadius, cache.arcRadius, 0.0f, startAngle, endAngle, true);
            imgG.setColour (juce::Colours::black.withAlpha (0.18f));
            imgG.strokePath (track, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Graduation ticks around the knob (0 to 10 scale like Evolver hardware)
            const float tickRadius = cache.radius * 1.22f;
            const int numTicks = 9;
            for (int t = 0; t < numTicks; ++t)
            {
                const float frac = static_cast<float> (t) / static_cast<float> (numTicks - 1);
                const float a = startAngle + frac * (endAngle - startAngle);
                const float tx1 = cx + (tickRadius - 1.0f) * std::cos (a - juce::MathConstants<float>::halfPi);
                const float ty1 = cy + (tickRadius - 1.0f) * std::sin (a - juce::MathConstants<float>::halfPi);
                const float tx2 = cx + (tickRadius + 2.0f) * std::cos (a - juce::MathConstants<float>::halfPi);
                const float ty2 = cy + (tickRadius + 2.0f) * std::sin (a - juce::MathConstants<float>::halfPi);

                imgG.setColour (juce::Colour (0xFF000000));
                imgG.drawLine (tx1, ty1, tx2, ty2, (t == 0 || t == 4 || t == 8) ? 1.8f : 1.1f);
            }
        }

        // Draw pre-rendered knob base
        g.drawImageAt (cache.image, x, y);

        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float knobCenterX = static_cast<float> (x) + cache.cx;
        const float knobCenterY = static_cast<float> (y) + cache.cy;

        // 6. Active Value Arc: Warm tungsten amber or stark white
        if (sliderPos > 0.002f)
        {
            juce::Path valArc;
            valArc.addCentredArc (knobCenterX, knobCenterY, cache.arcRadius, cache.arcRadius, 0.0f, startAngle, angle, true);
            g.setColour (AMBER_LED);
            g.strokePath (valArc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 7. Stark White Pointer Line (Evolver Style)
        juce::Path needle;
        const float needleLen = cache.radius * 0.76f;
        needle.addRoundedRectangle (-1.1f, -cache.radius * 0.90f, 2.2f, needleLen, 1.0f);
        needle.applyTransform (juce::AffineTransform::rotation (angle).translated (knobCenterX, knobCenterY));
        g.setColour (TEXT_BRIGHT);
        g.fillPath (needle);

        // 8. Custom High-Visibility Hardware Digital Display Badge
        const float badgeW = 48.0f;
        const float badgeH = 16.0f;
        const float badgeX = static_cast<float> (x) + (static_cast<float> (width) - badgeW) * 0.5f;
        const float badgeY = static_cast<float> (y + height) - badgeH - 1.0f;
        auto badgeRect = juce::Rectangle<float> (badgeX, badgeY, badgeW, badgeH);

        // Deep obsidian recessed socket
        g.setColour (juce::Colour (0xFF08090C));
        g.fillRoundedRectangle (badgeRect, 3.5f);

        // Precision anodized rim
        g.setColour (juce::Colour (0xFF3E4554));
        g.drawRoundedRectangle (badgeRect, 3.5f, 1.0f);

        // Subtle specular highlight on bottom rim
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawHorizontalLine (static_cast<int> (badgeRect.getBottom() - 0.5f), badgeRect.getX() + 2.0f, badgeRect.getRight() - 2.0f);

        // Large, bold, ultra-high contrast value text (Warm white/amber tint)
        g.setColour (juce::Colour (0xFFFFF6E5));
        g.setFont (font (11.5f, true, 0.05f));
        g.drawText (slider.getTextFromValue (slider.getValue()), badgeRect, juce::Justification::centred, false);
    }


    //--------------------------------------------------------------------------
    // REFINED SLENDER TACTILE PUSHBUTTON
    // Sleek brushed dark titanium body with precision micro-bevel,
    // crisp tracked typography, and subtle LED indicator when active.
    //--------------------------------------------------------------------------
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        using namespace OrageTheme;
        const auto totalBounds = button.getLocalBounds().toFloat();
        auto btnArea = totalBounds.reduced (1.0f, 1.0f);
        if (isDown)
            btnArea = btnArea.translated (0.0f, 0.7f);

        const float corner = 2.5f; // Sleek modern chamfered corner
        const bool isPrimary = static_cast<bool> (button.getProperties().getWithDefault ("primary", false));
        const bool isToggled = button.getToggleState();

        // 1. Subtle, lightweight drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        g.fillRoundedRectangle (btnArea.translated (0.0f, 1.2f), corner);

        // 2. Button Body Fill
        if (isPrimary)
        {
            // Primary action (TRIO HARMONIZE / GENERATE): Refined dark titanium with precision amber accent rim
            const juce::Colour topCol = isHighlighted ? juce::Colour (0xFF2F333B) : juce::Colour (0xFF22252C);
            const juce::Colour botCol = isHighlighted ? juce::Colour (0xFF1A1C22) : juce::Colour (0xFF131519);
            juce::ColourGradient grad (topCol, btnArea.getX(), btnArea.getY(),
                                       botCol, btnArea.getX(), btnArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (btnArea, corner);

            // Razor-thin amber satin accent border
            g.setColour (juce::Colour (0xFFD99B26).withAlpha (isHighlighted ? 0.90f : 0.65f));
            g.drawRoundedRectangle (btnArea, corner, 0.9f);

            // Top specular hairline
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawHorizontalLine (static_cast<int> (btnArea.getY() + 0.5f), btnArea.getX() + 2.0f, btnArea.getRight() - 2.0f);
        }
        else if (isToggled)
        {
            // Active / Toggled: Sleek dark face with glowing amber indicator border
            const juce::Colour topCol = isHighlighted ? juce::Colour (0xFF2C2820) : juce::Colour (0xFF201D16);
            const juce::Colour botCol = isHighlighted ? juce::Colour (0xFF1C1912) : juce::Colour (0xFF14120D);
            juce::ColourGradient grad (topCol, btnArea.getX(), btnArea.getY(),
                                       botCol, btnArea.getX(), btnArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (btnArea, corner);

            // Glowing amber line
            g.setColour (AMBER_LED.withAlpha (0.85f));
            g.drawRoundedRectangle (btnArea, corner, 1.0f);
        }
        else
        {
            // Standard Pushbutton: Slender dark slate/titanium button, very light and elegant
            const juce::Colour topCol = isHighlighted ? juce::Colour (0xFF333742) : juce::Colour (0xFF262932);
            const juce::Colour botCol = isHighlighted ? juce::Colour (0xFF20232B) : juce::Colour (0xFF17191F);
            juce::ColourGradient grad (topCol, btnArea.getX(), btnArea.getY(),
                                       botCol, btnArea.getX(), btnArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (btnArea, corner);

            // Thin satin metallic border
            g.setColour (juce::Colour (0xFF4A5162).withAlpha (isHighlighted ? 0.65f : 0.40f));
            g.drawRoundedRectangle (btnArea, corner, 0.85f);

            // Top specular hairline
            g.setColour (juce::Colours::white.withAlpha (0.22f));
            g.drawHorizontalLine (static_cast<int> (btnArea.getY() + 0.5f), btnArea.getX() + 2.0f, btnArea.getRight() - 2.0f);
        }

        if (isDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.20f));
            g.fillRoundedRectangle (btnArea, corner);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return OrageTheme::font (juce::jmin (9.5f, static_cast<float> (buttonHeight) * 0.44f), true, 0.12f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool isMouseOverButton, bool isButtonDown) override
    {
        using namespace OrageTheme;
        auto f = getTextButtonFont (button, button.getHeight());
        g.setFont (f);

        const bool isPrimary = static_cast<bool> (button.getProperties().getWithDefault ("primary", false));
        const bool isToggled = button.getToggleState();

        juce::Colour textCol = juce::Colour (0xFFE8EDF4);
        if (isToggled)
            textCol = AMBER_LED;
        else if (isPrimary)
            textCol = juce::Colour (0xFFFFF8EC);
        else if (isMouseOverButton)
            textCol = juce::Colours::white;

        g.setColour (textCol);

        auto bounds = button.getLocalBounds().reduced (3, 1);
        if (isToggled)
        {
            // Tiny sleek micro-LED pip on left
            const float pipSize = 4.0f;
            const float px = static_cast<float> (bounds.getX() + 5);
            const float py = static_cast<float> (bounds.getCentreY()) - pipSize * 0.5f;

            g.setColour (AMBER_LED_GLOW);
            g.fillEllipse (px - 1.5f, py - 1.5f, pipSize + 3.0f, pipSize + 3.0f);
            g.setColour (AMBER_LED);
            g.fillEllipse (px, py, pipSize, pipSize);
            g.setColour (AMBER_LED_BRIGHT);
            g.fillEllipse (px + 1.0f, py + 1.0f, pipSize * 0.4f, pipSize * 0.4f);

            bounds = bounds.withTrimmedLeft (12);
        }

        g.setColour (textCol);
        g.drawFittedText (button.getButtonText(), bounds, juce::Justification::centred, 1);
    }


    //--------------------------------------------------------------------------
    // EVOLVER RECESSED COMBOBOX
    // Precision milled recess in the aluminum panel with crisp typography
    //--------------------------------------------------------------------------
    void drawComboBox (juce::Graphics& g, int width, int height, bool isDown,
                        int, int, int, int, juce::ComboBox& box) override
    {
        using namespace OrageTheme;
        auto r = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
        const float corner = 4.0f;

        // High-contrast clean enamel display well for maximum legibility
        g.setColour (juce::Colour (0xFFF3F5F8));
        g.fillRoundedRectangle (r, corner);

        // Machined dark bezel rim
        g.setColour (juce::Colour (0xFF7A8291));
        g.drawRoundedRectangle (r, corner, 1.0f);

        // Bottom specular highlight
        g.setColour (juce::Colours::white.withAlpha (0.90f));
        g.drawHorizontalLine (static_cast<int> (r.getBottom()), r.getX() + 2.0f, r.getRight() - 2.0f);

        if (box.hasKeyboardFocus (true) || isDown)
        {
            g.setColour (AMBER_LED);
            g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.2f);
        }

        // Crisp pure black chevron arrow
        const float ax = r.getRight() - 14.0f;
        const float ay = r.getCentreY();
        juce::Path chev;
        chev.startNewSubPath (ax - 3.5f, ay - 2.0f);
        chev.lineTo (ax, ay + 2.0f);
        chev.lineTo (ax + 3.5f, ay - 2.0f);
        g.setColour (juce::Colour (0xFF000000));
        g.strokePath (chev, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return OrageTheme::montserratSemiBold (11.5f, 0.04f);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, juce::Colour (0xFF000000));
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        using namespace OrageTheme;
        g.fillAll (juce::Colour (0xFF16181B));
        g.setColour (juce::Colour (0xFF3A3D44));
        g.drawRect (0, 0, width, height, 1);
    }

    juce::Font getPopupMenuFont() override
    {
        return OrageTheme::font (12.0f, false, 0.03f);
    }
};

// Aliases for backwards compatibility
namespace LandrFxTheme = OrageTheme;
using LandrLookAndFeel = OrageLookAndFeel;
