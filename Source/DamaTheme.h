#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>

//==============================================================================
// LANDR FX Monochromatic Soft-UI Aesthetic with Subtle 3D Physical Lighting
// Directional light from above-left (Top highlight, bottom-right shadow)
// Palette: Pure Cyan/Turquoise gradient (#2FC0D0 -> #2587B5) + White / Deep Teal
//==============================================================================
namespace LandrFxTheme
{
    // 1. Primary Canvas Gradient
    const juce::Colour BG_TOP         { 0xFF2FC0D0 }; // Top Cyan/Turquoise
    const juce::Colour BG_BOTTOM      { 0xFF2587B5 }; // Bottom Deep Azure
    const juce::Colour SHADOW_COLOR   { 0x4D0B3D5C }; // Soft ambient shadow (~30% opacity)
    const juce::Colour SHADOW_DEEP    { 0x590B3D5C }; // ~35% shadow for knobs

    // 2. High-Contrast Text Colors
    const juce::Colour TEXT_DARK      { 0xFF1B6F8A }; // Deep teal for primary buttons
    const juce::Colour TEXT_BRIGHT    { 0xFFFFFFFF }; // 100% white for values & indicators
    const juce::Colour TEXT_LABEL     { 0xB3FFFFFF }; // ~70% white for uppercase labels
    const juce::Colour TEXT_MUTED     { 0x80FFFFFF }; // ~50% white for inactive hints/status

    // 3. Active Cells & Highlights
    const juce::Colour CELL_ACTIVE_BG { 0xFFFFFFFF }; // Crisp white fill
    const juce::Colour CELL_ACTIVE_TINT{ 0xFFBFF3F8 }; // Soft glowing ice-cyan
    const juce::Colour ACTIVE_GLOW    { 0x66BFF3F8 }; // Soft halo

    // 4. Inactive Transparencies
    const juce::Colour GLASS_PANEL    { 0x14FFFFFF }; // ~8% White section panel fill
    const juce::Colour CELL_OFF       { 0x0FFFFFFF }; // ~6% White inactive cell fill
    const juce::Colour TRACK_BG       { 0x140B3D5C }; // Recessed well dark shadow (~8%)

    // 5. Knob 3D Tones
    const juce::Colour KNOB_TOP       { 0xFF5FD3E0 }; // Light turquoise top
    const juce::Colour KNOB_BOT       { 0xFF2A9BC0 }; // Deeper azure bottom

    inline juce::Font font (float size, bool bold = false, float kerning = 0.08f)
    {
        juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        return (kerning != 0.0f) ? f.withExtraKerningFactor (kerning) : f;
    }

    // Helper to render recessed well (scavato): sfondo -5%, perimetro morbido continuo, nessun taglio squadrato
    inline void drawRecessedWell (juce::Graphics& g, juce::Rectangle<float> r, float corner = 6.0f,
                                  juce::Colour bg = juce::Colour (0xFF2796BD))
    {
        // 1. Fill: stessa luminosita dello sfondo -5%
        g.setColour (bg.darker (0.06f));
        g.fillRoundedRectangle (r, corner);

        // 2. Ombra interna morbida continua che segue la sagoma arrotondata
        juce::Path perimeter;
        perimeter.addRoundedRectangle (r.getX() + 0.5f, r.getY() + 0.5f, r.getWidth() - 1.0f, r.getHeight() - 1.0f, corner);
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.22f));
        g.strokePath (perimeter, juce::PathStrokeType (1.2f));

        // 3. Bordo inferiore chiaro (bianco 8%)
        juce::Path botRim;
        botRim.addRoundedRectangle (r.getX() + corner * 0.5f, r.getBottom() - 1.0f, r.getWidth() - corner, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillPath (botRim);
    }
}

//==============================================================================
// Custom LookAndFeel matching LANDR FX Soft Satin 3D Style
//==============================================================================
class LandrLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LandrLookAndFeel()
    {
        using namespace LandrFxTheme;
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId,       TEXT_BRIGHT);
        setColour (juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId,      TEXT_BRIGHT);

        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xFF1E729B));
        setColour (juce::PopupMenu::textColourId,                  TEXT_BRIGHT);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::white.withAlpha (0.28f));
        setColour (juce::PopupMenu::highlightedTextColourId,       TEXT_BRIGHT);

        setColour (juce::TextButton::buttonColourId,   GLASS_PANEL);
        setColour (juce::TextButton::buttonOnColourId, CELL_ACTIVE_BG);
        setColour (juce::TextButton::textColourOffId,  TEXT_LABEL);
        setColour (juce::TextButton::textColourOnId,   TEXT_DARK);

        setColour (juce::Label::textColourId, TEXT_LABEL);
        setColour (juce::Slider::textBoxTextColourId,    TEXT_BRIGHT);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }

    // Cache for pre-rendered soft-UI membrane knob base
    struct KnobBaseCache
    {
        int width = 0;
        int height = 0;
        juce::Image image;
        float cx = 0.0f;
        float cy = 0.0f;
        float radius = 0.0f;
        float arcRadius = 0.0f;
    };

    std::unordered_map<const juce::Slider*, KnobBaseCache> knobCaches;

    //--------------------------------------------------------------------------
    // LANDR FX Soft-UI Membrane Tactile Knob:
    // 1. Alone incassato: anello alla base, leggermente piu scuro dello sfondo,
    //    con gradiente radiale morbido, raggio ~ +25% del knob.
    // 2. Ombra esterna ampia e molto sfocata (blur ~ 40% del diametro, offset in basso a destra, #0B3D5C al 35%).
    //    Doppia ombra: ombra chiara in alto a sinistra (bianco 12%) + scura in basso a destra.
    // 3. Corpo: gradiente lineare molto leggero dall'alto (+6% luminosita) al basso (-6%),
    //    stesso azzurro dello sfondo locale.
    // 4. Cerchio interno sottilissimo (1px, bianco 12%) leggermente piu piccolo del corpo.
    // 5. Indicatore: una sola lineetta bianca con estremita arrotondate.
    // 6. Arco del valore: linea sottile (1.5px) bianca al 70%, attorno al knob.
    //
    // Performance: corpo, alone e ombre pre-renderizzati in cache (al resize).
    // Ad ogni repaint si disegnano solo l'arco e l'indicatore ruotato!
    //--------------------------------------------------------------------------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, const float startAngle, const float endAngle,
                           juce::Slider& slider) override
    {
        using namespace LandrFxTheme;
        auto& cache = knobCaches[&slider];

        if (cache.image.isNull() || cache.width != width || cache.height != height)
        {
            cache.width = width;
            cache.height = height;
            cache.cx = static_cast<float> (width) * 0.5f;
            cache.cy = static_cast<float> (height) * 0.46f;
            cache.radius = static_cast<float> (juce::jmin (width, height)) * 0.32f;
            cache.arcRadius = cache.radius * 1.22f;

            // Determina il colore di sfondo locale interpolato in base alla posizione Y dello slider
            float normY = 0.28f;
            if (auto* parent = slider.getParentComponent())
                normY = static_cast<float> (slider.getY()) / static_cast<float> (juce::jmax (1, parent->getHeight()));
            const juce::Colour localBg = BG_TOP.interpolatedWith (BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));

            cache.image = juce::Image (juce::Image::ARGB, width, height, true);
            juce::Graphics imgG (cache.image);

            const float cx = cache.cx;
            const float cy = cache.cy;
            const float radius = cache.radius;
            const float diameter = radius * 2.0f;

            // 1. ALONE INCASSATO: anello alla base, leggermente piu scuro dello sfondo, raggio ~ +25%
            const float wellRadius = radius * 1.26f;

            // Gradiente radiale morbido per la depressione della membrana
            juce::ColourGradient wellGrad (localBg.darker (0.10f), cx, cy,
                                           juce::Colours::transparentBlack, cx, cy - wellRadius, true);
            imgG.setGradientFill (wellGrad);
            imgG.fillEllipse (cx - wellRadius, cy - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f);

            // Ombra interna scura invertita in alto a sinistra dell'incavo (scavato)
            juce::ColourGradient wellInnerShadow (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.24f),
                                                  cx - wellRadius * 0.45f, cy - wellRadius * 0.45f,
                                                  juce::Colours::transparentBlack,
                                                  cx, cy, false);
            imgG.setGradientFill (wellInnerShadow);
            imgG.fillEllipse (cx - wellRadius, cy - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f);

            // Micro-bordo inferiore chiaro dell'incavo (la luce zenitale colpisce il bordo basso)
            juce::Path botWellRim;
            botWellRim.addCentredArc (cx, cy, wellRadius - 0.5f, wellRadius - 0.5f, 0.0f,
                                      juce::MathConstants<float>::pi * 0.25f,
                                      juce::MathConstants<float>::pi * 0.85f, true);
            imgG.setColour (juce::Colours::white.withAlpha (0.09f));
            imgG.strokePath (botWellRim, juce::PathStrokeType (1.0f));

            // 2. OMBRA ESTERNA AMPIA E MOLTO SFOCATA (in rilievo: doppia ombra)
            //    Ombra scura in basso a destra (#0B3D5C al 35%, blur ~ 40% del diametro)
            const float offDarkX = diameter * 0.08f;
            const float offDarkY = diameter * 0.11f;
            const float maxDarkBlur = diameter * 0.40f;
            const int numDarkSteps = 10;

            for (int step = numDarkSteps; step >= 1; --step)
            {
                const float t = static_cast<float> (step) / static_cast<float> (numDarkSteps);
                const float expand = t * maxDarkBlur;
                const float alpha = 0.35f * (1.0f - t * t) / (numDarkSteps * 0.28f);
                imgG.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (alpha));
                imgG.fillEllipse (cx + offDarkX * t - radius - expand,
                                  cy + offDarkY * t - radius - expand,
                                  (radius + expand) * 2.0f,
                                  (radius + expand) * 2.0f);
            }

            // Ombra chiara in alto a sinistra (bianco 12%, blur diffuso)
            const float offLightX = -diameter * 0.06f;
            const float offLightY = -diameter * 0.06f;
            const float maxLightBlur = diameter * 0.22f;
            const int numLightSteps = 6;

            for (int step = numLightSteps; step >= 1; --step)
            {
                const float t = static_cast<float> (step) / static_cast<float> (numLightSteps);
                const float expand = t * maxLightBlur;
                const float alpha = 0.12f * (1.0f - t) / static_cast<float> (numLightSteps * 0.35f);
                imgG.setColour (juce::Colours::white.withAlpha (alpha));
                imgG.fillEllipse (cx + offLightX * t - radius - expand,
                                  cy + offLightY * t - radius - expand,
                                  (radius + expand) * 2.0f,
                                  (radius + expand) * 2.0f);
            }

            // 3. CORPO: gradiente lineare molto leggero dall'alto (+6%) al basso (-6%),
            //    stesso azzurro dello sfondo (senza bordi netti!)
            const juce::Colour topBodyCol = localBg.brighter (0.06f);
            const juce::Colour botBodyCol = localBg.darker (0.06f);

            juce::ColourGradient bodyGrad (topBodyCol, cx - radius * 0.5f, cy - radius * 0.6f,
                                           botBodyCol, cx + radius * 0.5f, cy + radius * 0.6f, false);
            imgG.setGradientFill (bodyGrad);
            imgG.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

            // 4. CERCHIO INTERNO SOTTILISSIMO (1px, bianco 12%) leggermente piu piccolo del corpo
            const float innerRadius = radius - 1.2f;
            imgG.setColour (juce::Colours::white.withAlpha (0.12f));
            imgG.drawEllipse (cx - innerRadius, cy - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

            // Sottile highlight zenitale sul quarto superiore-sinistro della faccia superiore
            juce::Path topHighlight;
            topHighlight.addCentredArc (cx, cy, innerRadius - 0.5f, innerRadius - 0.5f, 0.0f,
                                        juce::MathConstants<float>::pi * 1.25f,
                                        juce::MathConstants<float>::pi * 1.85f, true);
            imgG.setColour (juce::Colours::white.withAlpha (0.16f));
            imgG.strokePath (topHighlight, juce::PathStrokeType (1.0f));

            // Traccia guida dell'arco spenta (sottilissima, 1.0px, bianco ~6%)
            juce::Path track;
            track.addCentredArc (cx, cy, cache.arcRadius, cache.arcRadius, 0.0f, startAngle, endAngle, true);
            imgG.setColour (juce::Colours::white.withAlpha (0.06f));
            imgG.strokePath (track, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // --- RENDER A RUNTIME: ISTANTANEO (1 solo draw call + arco + indicatore) ---
        g.drawImageAt (cache.image, x, y);

        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float knobCenterX = static_cast<float> (x) + cache.cx;
        const float knobCenterY = static_cast<float> (y) + cache.cy;

        // 6. ARCO DEL VALORE: linea sottile (1.5px) bianca al 70%, attorno al knob. Niente anelli spessi.
        if (sliderPos > 0.002f)
        {
            juce::Path valArc;
            valArc.addCentredArc (knobCenterX, knobCenterY, cache.arcRadius, cache.arcRadius, 0.0f, startAngle, angle, true);
            g.setColour (juce::Colours::white.withAlpha (0.70f));
            g.strokePath (valArc, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 5. INDICATORE: una sola lineetta bianca con estremita arrotondate
        juce::Path needle;
        const float needleLen = cache.radius * 0.42f;
        needle.addRoundedRectangle (-1.0f, -cache.radius * 0.85f, 2.0f, needleLen, 1.0f);
        needle.applyTransform (juce::AffineTransform::rotation (angle).translated (knobCenterX, knobCenterY));
        g.setColour (TEXT_BRIGHT);
        g.fillPath (needle);
    }

    //--------------------------------------------------------------------------
    // LANDR FX Soft-UI Pill Button (Stesso design organico del pomello DENSITY)
    // - Alone incassato alla base a forma di pillola (nessun riquadro o bordo squadrato)
    // - Ombra esterna morbida multi-step sfumata all'interno dei limiti
    // - Corpo pillola con gradiente zenitale coerente
    // - Bordo interno sottilissimo 1px per definire la faccia tattile superiore
    // - Highlight zenitale sul bordo superiore
    //--------------------------------------------------------------------------
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        using namespace LandrFxTheme;
        const auto totalBounds = button.getLocalBounds().toFloat();
        // Margine per contenere ombra e alone senza tagli da rettangolo
        auto pill = totalBounds.reduced (3.0f, 2.5f);
        const float rad = pill.getHeight() * 0.5f;
        const bool isPrimary = static_cast<bool> (button.getProperties().getWithDefault ("primary", false));

        float normY = 0.22f;
        if (auto* parent = button.getParentComponent())
            normY = static_cast<float> (button.getY()) / static_cast<float> (juce::jmax (1, parent->getHeight()));
        const juce::Colour localBg = BG_TOP.interpolatedWith (BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));

        // 1. ALONE INCASSATO ALLA BASE (come l'anello incassato alla base del knob)
        g.setColour (localBg.darker (0.07f));
        g.fillRoundedRectangle (totalBounds.reduced (1.0f, 1.0f), totalBounds.getHeight() * 0.5f);

        // Bordo inferiore chiaro dell'incavo (la luce zenitale colpisce il fondo)
        juce::Path botWellRim;
        botWellRim.addRoundedRectangle (totalBounds.getX() + rad * 0.5f, totalBounds.getBottom() - 1.2f,
                                        totalBounds.getWidth() - rad, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillPath (botWellRim);

        // 2. STATO ATTIVO o PRIMARY (GENERATE): In rilievo 3D come il pomello
        if (button.getToggleState() || isPrimary)
        {
            // Ombra esterna morbida sfocata progressiva (non tocca mai i bordi del rettangolo!)
            for (int step = 4; step >= 1; --step)
            {
                const float t = static_cast<float> (step) / 4.0f;
                const float alpha = 0.28f * (1.0f - t * t) / 1.8f;
                g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (alpha));
                g.fillRoundedRectangle (pill.translated (0.0f, 1.8f * t).expanded (t * 0.8f, t * 0.6f), rad + t * 0.6f);
            }

            // Corpo della pillola:
            if (isPrimary)
            {
                // Pulsante GENERATE: sfumatura ice speculare pulita
                juce::ColourGradient primGrad (juce::Colour (0xFFFFFFFF), pill.getX(), pill.getY(),
                                               juce::Colour (0xFFCEF4F9), pill.getRight(), pill.getBottom(), false);
                g.setGradientFill (primGrad);
                g.fillRoundedRectangle (pill, rad);
            }
            else
            {
                // Pulsante attivo normale: sfondo +8% luminosita
                const juce::Colour topCol = localBg.brighter (0.08f);
                const juce::Colour botCol = localBg.darker (0.05f);
                juce::ColourGradient raisedGrad (topCol, pill.getX(), pill.getY(),
                                                 botCol, pill.getRight(), pill.getBottom(), false);
                g.setGradientFill (raisedGrad);
                g.fillRoundedRectangle (pill, rad);
            }

            if (isDown)
            {
                g.setColour (juce::Colours::black.withAlpha (0.08f));
                g.fillRoundedRectangle (pill, rad);
            }

            // Cerchio/bordo interno sottilissimo (1px, come il cerchio interno del pomello)
            auto innerPill = pill.reduced (1.0f);
            g.setColour (juce::Colours::white.withAlpha (isPrimary ? 0.35f : 0.16f));
            g.drawRoundedRectangle (innerPill, innerPill.getHeight() * 0.5f, 1.0f);

            // Highlight zenitale morbido sul bordo superiore
            juce::Path topHighlight;
            topHighlight.addRoundedRectangle (pill.getX() + rad * 0.4f, pill.getY() + 0.6f, pill.getWidth() - rad * 0.8f, 1.0f, 0.5f);
            g.setColour (juce::Colours::white.withAlpha (isPrimary ? 0.60f : 0.22f));
            g.fillPath (topHighlight);
            return;
        }

        // 3. STATO INATTIVO (FROM SAMPLES, MUTATE, SHIFT, INVERT, CLEAR):
        //    Pillola morbida adagiata nell'incavo (stesso materiale dello sfondo)
        const juce::Colour topCol = isHighlighted ? localBg.brighter (0.05f) : localBg.brighter (0.02f);
        const juce::Colour botCol = isHighlighted ? localBg.darker (0.02f)   : localBg.darker (0.04f);
        juce::ColourGradient inactGrad (topCol, pill.getX(), pill.getY(),
                                        botCol, pill.getRight(), pill.getBottom(), false);
        g.setGradientFill (inactGrad);
        g.fillRoundedRectangle (pill, rad);

        if (isDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.06f));
            g.fillRoundedRectangle (pill, rad);
        }

        // Bordo interno sottilissimo 1px (come nel pomello)
        auto innerPill = pill.reduced (1.0f);
        g.setColour (juce::Colours::white.withAlpha (isHighlighted ? 0.16f : 0.09f));
        g.drawRoundedRectangle (innerPill, innerPill.getHeight() * 0.5f, 1.0f);

        // Sottile luce sul bordo superiore
        juce::Path topHighlight;
        topHighlight.addRoundedRectangle (pill.getX() + rad * 0.4f, pill.getY() + 0.6f, pill.getWidth() - rad * 0.8f, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (isHighlighted ? 0.20f : 0.11f));
        g.fillPath (topHighlight);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return LandrFxTheme::font (juce::jmin (11.5f, static_cast<float> (buttonHeight) * 0.38f), true, 0.09f);
    }

    //--------------------------------------------------------------------------
    // LANDR FX Soft-UI Dropdown ComboBox (Incassato nella superficie)
    //--------------------------------------------------------------------------
    void drawComboBox (juce::Graphics& g, int width, int height, bool isDown,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace LandrFxTheme;
        auto r = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
        const float rad = r.getHeight() * 0.5f;

        float normY = 0.15f;
        if (auto* parent = box.getParentComponent())
            normY = static_cast<float> (box.getY()) / static_cast<float> (juce::jmax (1, parent->getHeight()));
        const juce::Colour localBg = BG_TOP.interpolatedWith (BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));

        drawRecessedWell (g, r, rad, localBg);

        if (box.hasKeyboardFocus (true) || isDown)
        {
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
        }

        // Chevron
        const float ax = r.getRight() - 16.0f;
        const float ay = r.getCentreY();
        juce::Path chev;
        chev.startNewSubPath (ax - 3.5f, ay - 2.0f);
        chev.lineTo (ax, ay + 2.0f);
        chev.lineTo (ax + 3.5f, ay - 2.0f);
        g.setColour (TEXT_BRIGHT);
        g.strokePath (chev, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return LandrFxTheme::font (11.5f, false, 0.04f);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (12, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, LandrFxTheme::TEXT_BRIGHT);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        using namespace LandrFxTheme;
        g.fillAll (juce::Colour (0xFF1E729B));
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRect (0, 0, width, height, 1);
    }

    juce::Font getPopupMenuFont() override
    {
        return LandrFxTheme::font (12.5f, false, 0.03f);
    }
};
