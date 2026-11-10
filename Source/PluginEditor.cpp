#include "PluginEditor.h"

//==============================================================================
// StepMatrixComponent Implementation (LANDR FX Monochromatic 3D Polish)
//==============================================================================
StepMatrixComponent::StepMatrixComponent (PulseForgeAudioProcessor& proc)
    : processor (proc)
{
    setOpaque (false);
}

void StepMatrixComponent::resized()
{
    // Pre-render soft drop shadow for matrix panel once on resize
    const int w = juce::jmax (1, getWidth());
    const int h = juce::jmax (1, getHeight());
    cachedPanelShadow = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics gShadow (cachedPanelShadow);

    auto r = getLocalBounds().toFloat();
    // Soft ambient drop shadow (blur ~20px, offset y ~6px, #0B3D5C al 25%)
    for (int step = 6; step >= 1; --step)
    {
        const float t = static_cast<float> (step) / 6.0f;
        const float expand = t * 14.0f;
        const float alpha = 0.25f * (1.0f - t * t) / 2.0f;
        gShadow.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (alpha));
        gShadow.fillRoundedRectangle (r.translated (0.0f, 6.0f * t).expanded (expand * 0.5f, expand), 14.0f + expand);
    }
}

void StepMatrixComponent::paint (juce::Graphics& g)
{
    using namespace LandrFxTheme;
    const auto bounds = getLocalBounds().toFloat();

    // 1. Draw pre-rendered shadow
    if (cachedPanelShadow.isValid())
        g.drawImageAt (cachedPanelShadow, 0, 0);

    // 2. Translucent Panel Fill (~7% white, soft-UI surface)
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (bounds, 14.0f);

    // 3. Top Specular Hairline (1px, 8% white)
    juce::Path topHighlight;
    topHighlight.addRoundedRectangle (bounds.getX() + 2.0f, bounds.getY() + 0.5f, bounds.getWidth() - 4.0f, 1.0f, 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillPath (topHighlight);

    const float leftHeaderW = 104.0f;
    const float gridW = bounds.getWidth() - leftHeaderW - 16.0f;
    const float colW  = gridW / static_cast<float> (PulseForge::kMaxSteps);

    const float hStepNum = 24.0f;
    const float hPitch   = 116.0f;
    const float hGate    = 32.0f;
    const float hOctave  = 26.0f;
    const float hAccent  = 26.0f;
    const float hSlide   = 26.0f;
    const float hRatchet = 26.0f;
    const float hBottom  = bounds.getHeight() - (hStepNum + hPitch + hGate + hOctave + hAccent + hSlide + hRatchet) - 16.0f;

    const float yStepNum = 8.0f;
    const float yPitch   = yStepNum + hStepNum;
    const float yGate    = yPitch + hPitch;
    const float yOctave  = yGate + hGate;
    const float yAccent  = yOctave + hOctave;
    const float ySlide   = yAccent + hAccent;
    const float yRatchet = ySlide + hSlide;
    const float yBottom  = yRatchet + hRatchet;

    // Local background color interpolation helper
    auto getLocalBg = [this] (float localY) -> juce::Colour
    {
        const float screenY = static_cast<float> (getY()) + localY;
        const float normY = screenY / static_cast<float> (juce::jmax (1, getParentHeight()));
        return LandrFxTheme::BG_TOP.interpolatedWith (LandrFxTheme::BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));
    };

    // Soft-UI Cella Incassata (Stato spento/vuoto: sfondo -5%, ombra continua sul perimetro, nessun taglio squadrato)
    auto drawRecessedCell = [&] (juce::Rectangle<float> r, float corner, juce::Colour bg)
    {
        const juce::Colour cellBg = bg.darker (0.05f);
        g.setColour (cellBg);
        g.fillRoundedRectangle (r, corner);

        juce::Path perimeter;
        perimeter.addRoundedRectangle (r.getX() + 0.5f, r.getY() + 0.5f, r.getWidth() - 1.0f, r.getHeight() - 1.0f, corner);
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.22f));
        g.strokePath (perimeter, juce::PathStrokeType (1.2f));

        juce::Path botRim;
        botRim.addRoundedRectangle (r.getX() + corner * 0.5f, r.getBottom() - 1.0f, r.getWidth() - corner, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillPath (botRim);
    };

    // Soft-UI Cella in Rilievo (Stato acceso: sfondo +8%, ombra morbida come nel knob, highlight)
    auto drawRaisedCell = [&] (juce::Rectangle<float> r, float corner, juce::Colour bg)
    {
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), corner);

        const juce::Colour raisedBg = bg.brighter (0.08f);
        juce::ColourGradient raisedGrad (raisedBg.brighter (0.04f), r.getX(), r.getY(),
                                         raisedBg.darker (0.04f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (raisedGrad);
        g.fillRoundedRectangle (r, corner);

        auto innerR = r.reduced (0.8f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (innerR, juce::jmax (1.0f, corner - 0.8f), 1.0f);

        juce::Path topHairline;
        topHairline.addRoundedRectangle (r.getX() + corner * 0.4f, r.getY() + 0.5f, r.getWidth() - corner * 0.8f, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.24f));
        g.fillPath (topHairline);
    };

    // Left Row Labels: Solo testo in bianco 55%, senza riquadro in rilievo (Gerarchia pulita)
    auto drawRowLabel = [&] (float y, float h, const juce::String& title)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.setFont (font (9.5f, true, 0.10f));
        g.drawText (title, 14, static_cast<int> (y), static_cast<int> (leftHeaderW - 22.0f), static_cast<int> (h),
                    juce::Justification::centredLeft, false);
    };

    drawRowLabel (yStepNum, hStepNum, "STEP");
    drawRowLabel (yPitch,   hPitch,   "NOTE / PITCH");
    drawRowLabel (yGate,    hGate,    "GATE");
    drawRowLabel (yOctave,  hOctave,  "OCTAVE");
    drawRowLabel (yAccent,  hAccent,  "ACCENT");
    drawRowLabel (ySlide,   hSlide,   "SLIDE");
    drawRowLabel (yRatchet, hRatchet, "RATCHET");

    juce::String botLabel = "VELOCITY";
    if (bottomMode == BottomLaneMode::GateLength)       botLabel = "GATE STEPS";
    else if (bottomMode == BottomLaneMode::FilterCC)    botLabel = "FILTER CC";
    else if (bottomMode == BottomLaneMode::Probability) botLabel = "PROBABILITY";
    drawRowLabel (yBottom, hBottom, botLabel);

    const juce::ScopedLock sl (processor.patternLock);
    const auto& pat = processor.getActivePattern();
    const int numSteps   = juce::jlimit (1, PulseForge::kMaxSteps, static_cast<int> (processor.apvts.getRawParameterValue ("numSteps")->load()));
    const int playStep   = processor.getCurrentPlayStep();
    const bool isRunning = processor.isTransportRunning();
    const int rootKey    = static_cast<int> (processor.apvts.getRawParameterValue ("rootNote")->load());
    const int baseOctave = static_cast<int> (processor.apvts.getRawParameterValue ("baseOctave")->load());
    const auto scale     = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));
    const int rootMidi   = (baseOctave + 1) * 12 + rootKey;

    const juce::Colour bgStep  = getLocalBg (yStepNum + hStepNum * 0.5f);
    const juce::Colour bgPitch = getLocalBg (yPitch + hPitch * 0.5f);
    const juce::Colour bgGate  = getLocalBg (yGate + hGate * 0.5f);
    const juce::Colour bgOct   = getLocalBg (yOctave + hOctave * 0.5f);
    const juce::Colour bgAcc   = getLocalBg (yAccent + hAccent * 0.5f);
    const juce::Colour bgSld   = getLocalBg (ySlide + hSlide * 0.5f);
    const juce::Colour bgRat   = getLocalBg (yRatchet + hRatchet * 0.5f);
    const juce::Colour bgBot   = getLocalBg (yBottom + hBottom * 0.5f);

    for (int i = 0; i < PulseForge::kMaxSteps; ++i)
    {
        const float x = leftHeaderW + i * colW;
        const auto& step = pat.steps[static_cast<size_t>(i)];
        const bool inActiveLoop = (i < numSteps);
        const bool isPlayhead   = isRunning && (i == playStep) && inActiveLoop;
        const bool isBeatStart  = (i % 4 == 0);

        // Step Corrente Playhead: Soft white halo (non un riquadro netto), colonna leggermente in rilievo
        if (isPlayhead)
        {
            for (int halo = 3; halo >= 1; --halo)
            {
                const float expand = static_cast<float> (halo) * 2.5f;
                const float alpha = 0.04f * static_cast<float> (4 - halo);
                g.setColour (juce::Colours::white.withAlpha (alpha));
                g.fillRoundedRectangle (x - expand, yStepNum - expand,
                                        colW + expand * 2.0f, bounds.getHeight() - 16.0f + expand * 2.0f, 8.0f);
            }
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (x + 1.0f, yStepNum, colW - 2.0f, bounds.getHeight() - 16.0f, 6.0f);
        }
        else if (isBeatStart)
        {
            g.setColour (juce::Colours::white.withAlpha (0.02f));
            g.fillRect (x + 1.0f, yStepNum, colW - 2.0f, bounds.getHeight() - 16.0f);
        }

        const float alphaMult = inActiveLoop ? 1.0f : 0.30f;

        // 1. Step Number Cell (Incassata, numero bianco 50%, step corrente bianco pieno + glow)
        juce::Rectangle<float> rStep (x + 2.0f, yStepNum + 1.5f, colW - 4.0f, hStepNum - 3.0f);
        drawRecessedCell (rStep, 4.0f, bgStep);

        if (isPlayhead)
        {
            g.setColour (juce::Colours::white.withAlpha (0.22f));
            g.fillRoundedRectangle (rStep, 4.0f);
            g.setColour (TEXT_BRIGHT);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.50f * alphaMult));
        }
        g.setFont (font (10.0f, true, 0.05f));
        g.drawText (juce::String (i + 1), rStep, juce::Justification::centred, false);

        // 2. Note / Pitch Cell (Recessed well with soft-UI gradient bars)
        juce::Rectangle<float> rPitch (x + 2.0f, yPitch + 2.0f, colW - 4.0f, hPitch - 4.0f);
        drawRecessedCell (rPitch, 6.0f, bgPitch);

        const float zeroY = rPitch.getBottom() - 24.0f;
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (static_cast<int> (zeroY), rPitch.getX() + 2.0f, rPitch.getRight() - 2.0f);

        const int midiNote = processor.computeStepMidiNote (i, step, rootMidi, scale);
        const int quantSemi = PulseForge::quantizeSemitoneToScale (step.semitoneOffset, scale);
        const float normPitch = juce::jlimit (0.0f, 1.0f, static_cast<float> (quantSemi + 5) / 29.0f);
        const float barTopY = rPitch.getBottom() - 18.0f - normPitch * (rPitch.getHeight() - 34.0f);

        juce::Rectangle<float> pitchBar (rPitch.getX() + 2.5f,
                                         juce::jmin (barTopY, zeroY - 4.0f),
                                         rPitch.getWidth() - 5.0f,
                                         juce::jmax (7.0f, std::abs (zeroY - barTopY) + 4.0f));

        const float barOpacity = (step.active ? 0.85f : 0.35f) * alphaMult;

        // Piccola ombra sotto come se uscisse dalla traccia
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (step.active ? 0.28f : 0.10f));
        g.fillRoundedRectangle (pitchBar.translated (0.5f, 1.5f), 3.5f);

        // Barra con gradiente verticale sottile (#E8FBFD alto -> #9FE3EE basso), niente bordi netti
        juce::ColourGradient pitchBarGrad (juce::Colour (0xFFE8FBFD).withAlpha (barOpacity), pitchBar.getX(), pitchBar.getY(),
                                           juce::Colour (0xFF9FE3EE).withAlpha (barOpacity), pitchBar.getX(), pitchBar.getBottom(), false);
        g.setGradientFill (pitchBarGrad);
        g.fillRoundedRectangle (pitchBar, 3.5f);

        // Sustain tail (se attivo e gate lungo)
        if (step.active && step.gate > 1.15f && i < PulseForge::kMaxSteps - 1)
        {
            const float sustainW = juce::jmin (colW * (step.gate - 0.8f), (PulseForge::kMaxSteps - i) * colW - 6.0f);
            g.setColour (juce::Colour (0xFF9FE3EE).withAlpha (0.30f * alphaMult));
            g.fillRoundedRectangle (pitchBar.getX(), pitchBar.getY(), sustainW, pitchBar.getHeight(), 3.0f);
        }

        // Note Name: #E8FBFD luminoso se attivo, bianco 45% se gate off
        g.setColour (step.active ? juce::Colour (0xFFE8FBFD).withAlpha (alphaMult) : juce::Colours::white.withAlpha (0.45f * alphaMult));
        g.setFont (font (10.5f, true, 0.05f));
        g.drawText (PulseForge::getMidiNoteName (midiNote),
                    rPitch.withHeight (18.0f).withY (rPitch.getY() + 3.0f),
                    juce::Justification::centred, false);

        // Numeric Semitone Offset under bar: Bianco 70%, piccolo
        g.setColour (juce::Colours::white.withAlpha (0.70f * alphaMult));
        g.setFont (font (9.0f, false));
        juce::String semiStr = (quantSemi >= 0 ? "+" : "") + juce::String (quantSemi);
        g.drawText (semiStr, rPitch.withHeight (15.0f).withY (rPitch.getBottom() - 16.0f),
                    juce::Justification::centred, false);

        const juce::Colour activeTextCol { 0xFFE8FBFD };

        // 3. Gate ON/OFF Pill
        juce::Rectangle<float> rGate (x + 2.0f, yGate + 2.0f, colW - 4.0f, hGate - 4.0f);
        if (step.active)
        {
            drawRaisedCell (rGate, 6.0f, bgGate);
            g.setColour (activeTextCol);
        }
        else
        {
            drawRecessedCell (rGate, 6.0f, bgGate);
            g.setColour (juce::Colours::white.withAlpha (0.22f * alphaMult));
        }
        g.setFont (font (10.0f, true));
        g.drawText (step.active ? "ON" : "·", rGate, juce::Justification::centred, false);

        // 4. Octave Shift Pill (Active if octaveShift != 0)
        juce::Rectangle<float> rOct (x + 2.0f, yOctave + 2.0f, colW - 4.0f, hOctave - 4.0f);
        const bool hasOct = (step.octaveShift != 0);
        if (hasOct)
        {
            drawRaisedCell (rOct, 6.0f, bgOct);
            g.setColour (activeTextCol);
        }
        else
        {
            drawRecessedCell (rOct, 6.0f, bgOct);
            g.setColour (juce::Colours::white.withAlpha (0.22f * alphaMult));
        }
        g.setFont (font (9.5f, true));
        juce::String octText = (step.octaveShift > 0) ? ("+" + juce::String (step.octaveShift))
                             : (step.octaveShift < 0) ? juce::String (step.octaveShift) : "0";
        g.drawText (octText, rOct, juce::Justification::centred, false);

        // 5. Accent Pill (Active if step.accent)
        juce::Rectangle<float> rAcc (x + 2.0f, yAccent + 2.0f, colW - 4.0f, hAccent - 4.0f);
        if (step.accent)
        {
            drawRaisedCell (rAcc, 6.0f, bgAcc);
            g.setColour (activeTextCol);
        }
        else
        {
            drawRecessedCell (rAcc, 6.0f, bgAcc);
            g.setColour (juce::Colours::white.withAlpha (0.22f * alphaMult));
        }
        g.setFont (font (9.5f, true));
        g.drawText (step.accent ? "ACC" : "·", rAcc, juce::Justification::centred, false);

        // 6. Slide Pill (Active if step.slide)
        juce::Rectangle<float> rSld (x + 2.0f, ySlide + 2.0f, colW - 4.0f, hSlide - 4.0f);
        if (step.slide)
        {
            drawRaisedCell (rSld, 6.0f, bgSld);
            g.setColour (activeTextCol);
        }
        else
        {
            drawRecessedCell (rSld, 6.0f, bgSld);
            g.setColour (juce::Colours::white.withAlpha (0.22f * alphaMult));
        }
        g.setFont (font (9.0f, true));
        g.drawText (step.slide ? "SLIDE" : "·", rSld, juce::Justification::centred, false);

        if (step.slide && i < PulseForge::kMaxSteps - 1)
        {
            g.setColour (activeTextCol.withAlpha (0.45f * alphaMult));
            g.fillRect (rSld.getRight() - 2.0f, rSld.getCentreY() - 1.5f, 8.0f, 3.0f);
        }

        // 7. Ratchet Pill (Active if step.ratchet > 1)
        juce::Rectangle<float> rRat (x + 2.0f, yRatchet + 2.0f, colW - 4.0f, hRatchet - 4.0f);
        const bool hasRatchet = (step.ratchet > 1);
        if (hasRatchet)
        {
            drawRaisedCell (rRat, 6.0f, bgRat);
            g.setColour (activeTextCol);
        }
        else
        {
            drawRecessedCell (rRat, 6.0f, bgRat);
            g.setColour (juce::Colours::white.withAlpha (0.22f * alphaMult));
        }
        g.setFont (font (9.5f, true));
        g.drawText (juce::String (step.ratchet) + "x", rRat, juce::Justification::centred, false);

        // 8. Bottom Multi-Parameter Recessed Bar (Velocity / Gate / CC / Probability)
        juce::Rectangle<float> rBot (x + 2.0f, yBottom + 2.0f, colW - 4.0f, hBottom - 4.0f);
        drawRecessedCell (rBot, 6.0f, bgBot);

        float normVal = 0.75f;
        juce::String valText;
        if (bottomMode == BottomLaneMode::Velocity)
        {
            normVal = static_cast<float> (step.velocity) / 127.0f;
            valText = juce::String (step.velocity);
        }
        else if (bottomMode == BottomLaneMode::GateLength)
        {
            normVal = juce::jlimit (0.05f, 1.0f, step.gate / 3.8f);
            valText = juce::String (step.gate, 1) + "s";
        }
        else if (bottomMode == BottomLaneMode::FilterCC)
        {
            normVal = static_cast<float> (step.ccValue) / 127.0f;
            valText = juce::String (step.ccValue);
        }
        else
        {
            normVal = static_cast<float> (step.probability) / 100.0f;
            valText = juce::String (step.probability) + "%";
        }

        const float fillH = juce::jmax (4.0f, normVal * (rBot.getHeight() - 6.0f));
        juce::Rectangle<float> fillRect (rBot.getX() + 2.0f, rBot.getBottom() - 3.0f - fillH, rBot.getWidth() - 4.0f, fillH);

        const float botOpacity = (step.active ? 0.85f : 0.35f) * alphaMult;

        // Piccola ombra sotto come se uscisse dalla traccia
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (step.active ? 0.25f : 0.08f));
        g.fillRoundedRectangle (fillRect.translated (0.5f, 1.5f), 3.0f);

        // Barra con gradiente verticale sottile (#E8FBFD alto -> #9FE3EE basso), niente bordi netti
        juce::ColourGradient botBarGrad (juce::Colour (0xFFE8FBFD).withAlpha (botOpacity), fillRect.getX(), fillRect.getY(),
                                         juce::Colour (0xFF9FE3EE).withAlpha (botOpacity), fillRect.getX(), fillRect.getBottom(), false);
        g.setGradientFill (botBarGrad);
        g.fillRoundedRectangle (fillRect, 3.0f);

        // Numeric Value under the bar: Bianco al 70%, piccolo
        g.setColour (juce::Colours::white.withAlpha (0.70f * alphaMult));
        g.setFont (font (9.0f, false));
        g.drawText (valText, rBot.withHeight (15.0f).withY (rBot.getBottom() - 16.0f), juce::Justification::centred, false);
    }
}

void StepMatrixComponent::handleStepInteraction (juce::Point<int> pos, bool isFirstClick, bool isRightClick)
{
    const auto bounds = getLocalBounds().toFloat();
    const float leftHeaderW = 104.0f;
    const float gridW = bounds.getWidth() - leftHeaderW - 16.0f;
    if (pos.x < leftHeaderW || gridW <= 10.0f)
        return;

    const float colW = gridW / static_cast<float> (PulseForge::kMaxSteps);
    const int stepIdx = juce::jlimit (0, PulseForge::kMaxSteps - 1, static_cast<int> ((pos.x - leftHeaderW) / colW));

    const float hStepNum = 24.0f;
    const float hPitch   = 116.0f;
    const float hGate    = 32.0f;
    const float hOctave  = 26.0f;
    const float hAccent  = 26.0f;
    const float hSlide   = 26.0f;
    const float hRatchet = 26.0f;
    const float hBottom  = bounds.getHeight() - (hStepNum + hPitch + hGate + hOctave + hAccent + hSlide + hRatchet) - 16.0f;

    const float yStepNum = 8.0f;
    const float yPitch   = yStepNum + hStepNum;
    const float yGate    = yPitch + hPitch;
    const float yOctave  = yGate + hGate;
    const float yAccent  = yOctave + hOctave;
    const float ySlide   = yAccent + hAccent;
    const float yRatchet = ySlide + hSlide;
    const float yBottom  = yRatchet + hRatchet;

    if (isFirstClick)
    {
        if (pos.y < yPitch)        activeDragRow = 0;
        else if (pos.y < yGate)    activeDragRow = 1;
        else if (pos.y < yOctave)  activeDragRow = 2;
        else if (pos.y < yAccent)  activeDragRow = 3;
        else if (pos.y < ySlide)   activeDragRow = 4;
        else if (pos.y < yRatchet) activeDragRow = 5;
        else if (pos.y < yBottom)  activeDragRow = 6;
        else                       activeDragRow = 7;
        lastToggledStep = -1;
    }

    const juce::ScopedLock sl (processor.patternLock);
    auto& pat = processor.getActivePattern();
    auto& step = pat.steps[static_cast<size_t>(stepIdx)];
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));

    switch (activeDragRow)
    {
        case 0:
            if (auto* pSteps = processor.apvts.getParameter ("numSteps"))
                pSteps->setValueNotifyingHost (pSteps->convertTo0to1 (static_cast<float> (stepIdx + 1)));
            break;

        case 1:
            if (isRightClick)
            {
                step.semitoneOffset = 0;
            }
            else
            {
                float normY = 1.0f - juce::jlimit (0.0f, 1.0f, (static_cast<float> (pos.y) - (yPitch + 12.0f)) / (hPitch - 28.0f));
                int rawSemi = static_cast<int> (std::round (normY * 29.0f)) - 5;
                step.semitoneOffset = PulseForge::quantizeSemitoneToScale (juce::jlimit (-5, 24, rawSemi), scale);
                step.active = true;
            }
            break;

        case 2:
            if (isFirstClick)
                dragGateTargetState = !step.active;
            if (stepIdx != lastToggledStep)
            {
                step.active = dragGateTargetState;
                lastToggledStep = stepIdx;
            }
            break;

        case 3:
            if (isFirstClick || stepIdx != lastToggledStep)
            {
                if (isRightClick)
                    step.octaveShift = 0;
                else
                    step.octaveShift = (step.octaveShift >= 2) ? -2 : (step.octaveShift + 1);
                lastToggledStep = stepIdx;
            }
            break;

        case 4:
            if (isFirstClick)
                dragAccentTargetState = !step.accent;
            if (stepIdx != lastToggledStep)
            {
                step.accent = dragAccentTargetState;
                if (step.accent && step.velocity < 115) step.velocity = 122;
                lastToggledStep = stepIdx;
            }
            break;

        case 5:
            if (isFirstClick)
                dragSlideTargetState = !step.slide;
            if (stepIdx != lastToggledStep)
            {
                step.slide = dragSlideTargetState;
                step.gate  = step.slide ? 1.08f : 0.80f;
                lastToggledStep = stepIdx;
            }
            break;

        case 6:
            if (isFirstClick || stepIdx != lastToggledStep)
            {
                step.ratchet = isRightClick ? 1 : ((step.ratchet % 4) + 1);
                lastToggledStep = stepIdx;
            }
            break;

        case 7:
        {
            float normY = 1.0f - juce::jlimit (0.0f, 1.0f, (static_cast<float> (pos.y) - (yBottom + 4.0f)) / (hBottom - 8.0f));
            if (bottomMode == BottomLaneMode::Velocity)
                step.velocity = juce::jlimit (1, 127, static_cast<int> (std::round (normY * 127.0f)));
            else if (bottomMode == BottomLaneMode::GateLength)
                step.gate = juce::jlimit (0.15f, 3.80f, normY * 3.80f);
            else if (bottomMode == BottomLaneMode::FilterCC)
                step.ccValue = juce::jlimit (0, 127, static_cast<int> (std::round (normY * 127.0f)));
            else
                step.probability = juce::jlimit (0, 100, static_cast<int> (std::round (normY * 100.0f)));
            break;
        }

        default:
            break;
    }

    repaint();
}

void StepMatrixComponent::mouseDown (const juce::MouseEvent& e)
{
    handleStepInteraction (e.getPosition(), true, e.mods.isRightButtonDown());
}

void StepMatrixComponent::mouseDrag (const juce::MouseEvent& e)
{
    handleStepInteraction (e.getPosition(), false, e.mods.isRightButtonDown());
}

void StepMatrixComponent::mouseUp (const juce::MouseEvent&)
{
    activeDragRow = -1;
    lastToggledStep = -1;
}

//==============================================================================
// PulseForgeAudioProcessorEditor Implementation
//==============================================================================
PulseForgeAudioProcessorEditor::PulseForgeAudioProcessorEditor (PulseForgeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      stepMatrix (p)
{
    setLookAndFeel (&customLookAndFeel);
    setResizable (true, true);
    setResizeLimits (1000, 680, 1600, 1100);

    // 1. Mode Switch Buttons
    auto updateModeButtons = [this]()
    {
        const int mode = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("mode")->load());
        seqModeBtn.setToggleState (mode == 0, juce::dontSendNotification);
        arpModeBtn.setToggleState (mode == 1, juce::dontSendNotification);
    };

    seqModeBtn.onClick = [this, updateModeButtons]()
    {
        if (auto* param = audioProcessor.apvts.getParameter ("mode"))
            param->setValueNotifyingHost (0.0f);
        updateModeButtons();
    };
    arpModeBtn.onClick = [this, updateModeButtons]()
    {
        if (auto* param = audioProcessor.apvts.getParameter ("mode"))
            param->setValueNotifyingHost (1.0f);
        updateModeButtons();
    };
    addAndMakeVisible (seqModeBtn);
    addAndMakeVisible (arpModeBtn);
    updateModeButtons();

    // 2. Free Run & Synth Preview
    runBtn.setClickingTogglesState (true);
    addAndMakeVisible (runBtn);
    runAttach = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "autoPlay", runBtn);

    previewBtn.setClickingTogglesState (true);
    addAndMakeVisible (previewBtn);
    previewAttach = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "previewSynth", previewBtn);

    // 3. Drag MIDI to DAW Button
    dragMidiBtn.onClick = [this]()
    {
        juce::File midiFile = audioProcessor.exportActivePatternToTempMidiFile();
        if (midiFile.existsAsFile())
        {
            juce::StringArray files { midiFile.getFullPathName() };
            performExternalDragDropOfFiles (files, false, &dragMidiBtn);
        }
    };
    addAndMakeVisible (dragMidiBtn);

    // 4. Pattern Slot Buttons A / B / C / D
    for (int i = 0; i < 4; ++i)
    {
        auto& b = slotBtns[static_cast<size_t>(i)];
        b.onClick = [this, i]()
        {
            if (juce::ModifierKeys::currentModifiers.isShiftDown())
                audioProcessor.copyPatternSlot (audioProcessor.getActiveSlotIndex(), i);
            audioProcessor.setActiveSlotIndex (i);
            stepMatrix.repaint();
        };
        addAndMakeVisible (b);
    }

    // 5. Dropdown ComboBoxes
    auto populateCombo = [this] (juce::ComboBox& box, const juce::String& paramID)
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (paramID)))
        {
            box.clear (juce::dontSendNotification);
            for (int i = 0; i < choiceParam->choices.size(); ++i)
                box.addItem (choiceParam->choices[i], i + 1);
        }
        addAndMakeVisible (box);
    };

    populateCombo (genreBox,  "genre");
    populateCombo (roleBox,   "role");
    populateCombo (scaleBox,  "scale");
    populateCombo (rootBox,   "rootNote");
    populateCombo (rateBox,   "rate");
    populateCombo (arpDirBox, "arpDir");

    genreAttach  = std::make_unique<ComboAttachment> (audioProcessor.apvts, "genre",    genreBox);
    roleAttach   = std::make_unique<ComboAttachment> (audioProcessor.apvts, "role",     roleBox);
    scaleAttach  = std::make_unique<ComboAttachment> (audioProcessor.apvts, "scale",    scaleBox);
    rootAttach   = std::make_unique<ComboAttachment> (audioProcessor.apvts, "rootNote", rootBox);
    rateAttach   = std::make_unique<ComboAttachment> (audioProcessor.apvts, "rate",     rateBox);
    arpDirAttach = std::make_unique<ComboAttachment> (audioProcessor.apvts, "arpDir",   arpDirBox);

    genreBox.onChange = [this]() { stepMatrix.repaint(); };
    roleBox.onChange  = [this]() { stepMatrix.repaint(); };
    scaleBox.onChange = [this]()
    {
        audioProcessor.triggerRequantizeToScale();
        stepMatrix.repaint();
    };

    addAndMakeVisible (hwMidiBox);
    refreshHardwareMidiList();
    hwMidiBox.onChange = [this]()
    {
        audioProcessor.setHardwareMidiOutDevice (hwMidiBox.getText());
    };

    // 6. Action Buttons: GENERATE with crisp readable text (#1B6F8A on white/cyan)!
    generateBtn.getProperties().set ("primary", true);
    generateBtn.setColour (juce::TextButton::textColourOnId,  LandrFxTheme::TEXT_DARK);
    generateBtn.setColour (juce::TextButton::textColourOffId, LandrFxTheme::TEXT_DARK);
    generateBtn.onClick = [this]()
    {
        audioProcessor.triggerGeneratePattern (true);
        stepMatrix.repaint();
    };
    addAndMakeVisible (generateBtn);

    sampleMidiBtn.onClick = [this]()
    {
        if (juce::ModifierKeys::currentModifiers.isShiftDown())
        {
            juce::File startDir ("G:/samples/MIDI");
            if (!startDir.isDirectory())
                startDir = juce::File ("G:/samples");

            midiFileChooser = std::make_unique<juce::FileChooser> ("Scegli un file MIDI dalla tua libreria Samples", startDir, "*.mid");
            midiFileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this] (const juce::FileChooser& fc)
                {
                    auto f = fc.getResult();
                    if (f.existsAsFile())
                    {
                        audioProcessor.triggerLoadSpecificMidiFile (f);
                        stepMatrix.repaint();
                    }
                });
        }
        else
        {
            audioProcessor.triggerLoadFromUserSamplesLibrary();
            stepMatrix.repaint();
        }
    };
    addAndMakeVisible (sampleMidiBtn);

    mutateBtn.onClick = [this]()
    {
        audioProcessor.triggerMutatePattern();
        stepMatrix.repaint();
    };
    addAndMakeVisible (mutateBtn);

    shiftLeftBtn.onClick  = [this]() { audioProcessor.triggerRotatePattern (-1); stepMatrix.repaint(); };
    shiftRightBtn.onClick = [this]() { audioProcessor.triggerRotatePattern (+1); stepMatrix.repaint(); };
    invertBtn.onClick     = [this]() { audioProcessor.triggerInvertPattern();    stepMatrix.repaint(); };
    clearBtn.onClick      = [this]() { audioProcessor.triggerClearPattern();     stepMatrix.repaint(); };

    addAndMakeVisible (shiftLeftBtn);
    addAndMakeVisible (shiftRightBtn);
    addAndMakeVisible (invertBtn);
    addAndMakeVisible (clearBtn);

    // 7. Bottom Lane Mode Tabs
    auto updateLaneTabs = [this]()
    {
        const auto m = stepMatrix.getBottomLaneMode();
        tabVelBtn.setToggleState  (m == StepMatrixComponent::BottomLaneMode::Velocity,    juce::dontSendNotification);
        tabGateBtn.setToggleState (m == StepMatrixComponent::BottomLaneMode::GateLength,  juce::dontSendNotification);
        tabCcBtn.setToggleState   (m == StepMatrixComponent::BottomLaneMode::FilterCC,    juce::dontSendNotification);
        tabProbBtn.setToggleState (m == StepMatrixComponent::BottomLaneMode::Probability, juce::dontSendNotification);
    };

    tabVelBtn.onClick  = [this, updateLaneTabs]() { stepMatrix.setBottomLaneMode (StepMatrixComponent::BottomLaneMode::Velocity);    updateLaneTabs(); };
    tabGateBtn.onClick = [this, updateLaneTabs]() { stepMatrix.setBottomLaneMode (StepMatrixComponent::BottomLaneMode::GateLength);  updateLaneTabs(); };
    tabCcBtn.onClick   = [this, updateLaneTabs]() { stepMatrix.setBottomLaneMode (StepMatrixComponent::BottomLaneMode::FilterCC);    updateLaneTabs(); };
    tabProbBtn.onClick = [this, updateLaneTabs]() { stepMatrix.setBottomLaneMode (StepMatrixComponent::BottomLaneMode::Probability); updateLaneTabs(); };

    addAndMakeVisible (tabVelBtn);
    addAndMakeVisible (tabGateBtn);
    addAndMakeVisible (tabCcBtn);
    addAndMakeVisible (tabProbBtn);
    updateLaneTabs();

    // 8. LANDR FX Style Knobs
    setupKnob (densitySlider,     densityLabel,     "DENSITY",     LandrFxTheme::TEXT_BRIGHT);
    setupKnob (syncopationSlider, syncopationLabel, "SYNCOPATION", LandrFxTheme::TEXT_BRIGHT);
    setupKnob (spiceSlider,       spiceLabel,       "SPICE / ACID",LandrFxTheme::TEXT_BRIGHT);
    setupKnob (rangeSlider,       rangeLabel,       "PITCH RANGE", LandrFxTheme::TEXT_BRIGHT);

    setupKnob (swingSlider,       swingLabel,       "SWING %",     LandrFxTheme::TEXT_BRIGHT);
    setupKnob (gateSlider,        gateLabel,        "GATE SCALE",  LandrFxTheme::TEXT_BRIGHT);
    setupKnob (stepsSlider,       stepsLabel,       "STEPS",       LandrFxTheme::TEXT_BRIGHT);
    setupKnob (octaveSlider,      octaveLabel,      "BASE OCT",    LandrFxTheme::TEXT_BRIGHT);
    setupKnob (arpOctSlider,      arpOctLabel,      "ARP OCTS",    LandrFxTheme::TEXT_BRIGHT);
    setupKnob (chanSlider,        chanLabel,        "MIDI CH",     LandrFxTheme::TEXT_BRIGHT);
    setupKnob (ccNumSlider,       ccNumLabel,       "FILTER CC#",  LandrFxTheme::TEXT_BRIGHT);
    setupKnob (tempoRatioSlider,  tempoRatioLabel,  "BPM RATIO",   LandrFxTheme::TEXT_BRIGHT);

    densityAttach     = std::make_unique<SliderAttachment> (audioProcessor.apvts, "density",     densitySlider);
    syncopationAttach = std::make_unique<SliderAttachment> (audioProcessor.apvts, "syncopation", syncopationSlider);
    spiceAttach       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "spice",       spiceSlider);
    rangeAttach       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "pitchRange",  rangeSlider);
    swingAttach       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "swing",       swingSlider);
    gateAttach        = std::make_unique<SliderAttachment> (audioProcessor.apvts, "globalGate",  gateSlider);
    stepsAttach       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "numSteps",    stepsSlider);
    octaveAttach      = std::make_unique<SliderAttachment> (audioProcessor.apvts, "baseOctave",  octaveSlider);
    arpOctAttach      = std::make_unique<SliderAttachment> (audioProcessor.apvts, "arpOctaves",  arpOctSlider);
    chanAttach        = std::make_unique<SliderAttachment> (audioProcessor.apvts, "midiChannel", chanSlider);
    ccNumAttach       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "ccNumber",    ccNumSlider);
    tempoRatioAttach  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "tempoRatio",  tempoRatioSlider);

    tempoRatioSlider.textFromValueFunction = [] (double val)
    {
        return PulseForge::getTempoRatioName (static_cast<int> (std::round (val)));
    };
    tempoRatioSlider.updateText();

    addAndMakeVisible (stepMatrix);

    setSize (1140, 740);
    startTimerHz (30);
}

PulseForgeAudioProcessorEditor::~PulseForgeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PulseForgeAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& l,
                                                const juce::String& text, juce::Colour col)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
    s.setColour (juce::Slider::rotarySliderFillColourId, col);
    s.setColour (juce::Slider::textBoxTextColourId, LandrFxTheme::TEXT_BRIGHT);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (LandrFxTheme::font (9.5f, true, 0.08f));
    l.setColour (juce::Label::textColourId, LandrFxTheme::TEXT_LABEL);
    addAndMakeVisible (l);
}

void PulseForgeAudioProcessorEditor::refreshHardwareMidiList()
{
    const auto list = audioProcessor.getAvailableHardwareMidiOuts();
    hwMidiBox.clear (juce::dontSendNotification);
    for (int i = 0; i < list.size(); ++i)
        hwMidiBox.addItem (list[i], i + 1);

    const auto current = audioProcessor.getCurrentHardwareMidiOutName();
    int idx = list.indexOf (current);
    hwMidiBox.setSelectedId (idx >= 0 ? (idx + 1) : 1, juce::dontSendNotification);
}

void PulseForgeAudioProcessorEditor::renderCachedShadows (int width, int height)
{
    using namespace LandrFxTheme;
    cachedBackground = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (cachedBackground);

    // 1. Primary Card: Rounded corners (16px) with vertical gradient (#2FC0D0 -> #2587B5)
    auto cardArea = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);

    // Outer ambient drop shadows (~30% #0B3D5C)
    g.setColour (SHADOW_COLOR);
    g.fillRoundedRectangle (cardArea.translated (0.0f, 6.0f), 18.0f);
    g.setColour (SHADOW_COLOR.withAlpha (0.15f));
    g.fillRoundedRectangle (cardArea.translated (0.0f, 12.0f), 22.0f);

    // Vertical Gradient Canvas
    juce::ColourGradient bgGrad (BG_TOP, 0.0f, cardArea.getY(),
                                 BG_BOTTOM, 0.0f, cardArea.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (cardArea, 16.0f);

    // Helper for rendering section panels with soft-UI drop shadow and top specular hairline
    auto drawSection = [&] (juce::Rectangle<float> r, float corner = 14.0f)
    {
        // Panel soft drop shadow (blur ~20px, offset y ~6px, #0B3D5C al 25%)
        for (int step = 6; step >= 1; --step)
        {
            const float t = static_cast<float> (step) / 6.0f;
            const float expand = t * 14.0f;
            const float alpha = 0.25f * (1.0f - t * t) / 2.0f;
            g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (alpha));
            g.fillRoundedRectangle (r.translated (0.0f, 6.0f * t).expanded (expand * 0.5f, expand), corner + expand);
        }

        // Panel glass fill (~7% white, soft-UI membrane)
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (r, corner);

        // Highlight di 1px bianco al 8% solo sul bordo superiore
        juce::Path topHairline;
        topHairline.addRoundedRectangle (r.getX() + 2.0f, r.getY() + 0.5f, r.getWidth() - 4.0f, 1.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillPath (topHairline);
    };

    // Header Panel
    drawSection (juce::Rectangle<float> (16.0f, 16.0f, static_cast<float> (width - 32), 48.0f), 14.0f);

    // Section 1: Dropdowns Panel
    drawSection (juce::Rectangle<float> (16.0f, 72.0f, static_cast<float> (width - 32), 66.0f), 14.0f);

    // Section 2: Macro Knobs Panel
    drawSection (juce::Rectangle<float> (16.0f, 146.0f, static_cast<float> (width - 32), 94.0f), 14.0f);

    // Sub-header bar
    drawSection (juce::Rectangle<float> (16.0f, 248.0f, static_cast<float> (width - 32), 32.0f), 10.0f);
}

void PulseForgeAudioProcessorEditor::timerCallback()
{
    const int mode = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("mode")->load());
    seqModeBtn.setToggleState (mode == 0, juce::dontSendNotification);
    arpModeBtn.setToggleState (mode == 1, juce::dontSendNotification);

    const int activeSlot = audioProcessor.getActiveSlotIndex();
    for (int i = 0; i < 4; ++i)
        slotBtns[static_cast<size_t>(i)].setToggleState (i == activeSlot, juce::dontSendNotification);

    stepMatrix.repaint();
    repaint (0, 244, getWidth(), 38);
    repaint (0, getHeight() - 32, getWidth(), 32);
}

void PulseForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace LandrFxTheme;

    // 1. Draw cached background & panel shadows
    if (cachedBackground.isValid())
        g.drawImageAt (cachedBackground, 0, 0);

    // 2. Title Typography (Uppercase, wide spacing, white ~70%)
    g.setColour (TEXT_BRIGHT.withAlpha (0.90f));
    g.setFont (font (15.0f, true, 0.16f));
    g.drawText ("PULSEFORGE", 32, 22, 140, 20, juce::Justification::centredLeft, false);

    g.setColour (TEXT_LABEL);
    g.setFont (font (9.0f, true, 0.10f));
    g.drawText ("MIDI GENERATOR", 32, 40, 150, 14, juce::Justification::centredLeft, false);

    // 3. Dropdown Labels
    g.setColour (TEXT_LABEL);
    g.setFont (font (9.0f, true, 0.09f));
    g.drawText ("GENRE STYLE",         genreBox.getX(),  76, genreBox.getWidth(),  15, juce::Justification::centredLeft);
    g.drawText ("INSTRUMENT ROLE",     roleBox.getX(),   76, roleBox.getWidth(),   15, juce::Justification::centredLeft);
    g.drawText ("MUSICAL SCALE",       scaleBox.getX(),  76, scaleBox.getWidth(),  15, juce::Justification::centredLeft);
    g.drawText ("ROOT",                rootBox.getX(),   76, rootBox.getWidth(),   15, juce::Justification::centredLeft);
    g.drawText ("STEP GRID",           rateBox.getX(),   76, rateBox.getWidth(),   15, juce::Justification::centredLeft);
    g.drawText ("ARP MODE",            arpDirBox.getX(), 76, arpDirBox.getWidth(), 15, juce::Justification::centredLeft);
    g.drawText ("DIRECT HW MIDI OUT",  hwMidiBox.getX(), 76, hwMidiBox.getWidth(), 15, juce::Justification::centredLeft);

    // 4. Telemetry in Sub-Header
    const double hostBpm = audioProcessor.getCurrentHostBpm();
    const int ratioIdx = static_cast<int> (audioProcessor.apvts.getRawParameterValue ("tempoRatio")->load());
    const double effBpm = audioProcessor.getEffectiveBpm();

    g.setColour (TEXT_BRIGHT);
    g.fillEllipse (30.0f, 260.0f, 6.0f, 6.0f);

    const juce::String syncText = "SYNC: " + juce::String (hostBpm, 1) + " BPM   |   RATIO: "
                                + PulseForge::getTempoRatioName (ratioIdx) + " (" + juce::String (effBpm, 1) + " BPM)";
    g.setFont (font (10.5f, true, 0.06f));
    g.drawText (syncText, 44, 248, 520, 32, juce::Justification::centredLeft, false);

    g.setColour (TEXT_LABEL);
    g.setFont (font (9.5f, true, 0.08f));
    g.drawText ("MODULATION LANE", getWidth() - 480, 248, 115, 32, juce::Justification::centredRight, false);

    // 5. Footer Status Row: Pure text, 50% white, no background
    const int footerY = getHeight() - 28;
    const auto loadedMidi = audioProcessor.getLastLoadedMidiName();
    const auto genre = static_cast<PulseForge::GenreStyle> (static_cast<int> (audioProcessor.apvts.getRawParameterValue ("genre")->load()));

    juce::String guideStr = loadedMidi.isNotEmpty()
                          ? ("LOADED: \"" + loadedMidi + "\"")
                          : ("STYLE: " + PulseForge::getGenreDescription (genre));

    g.setColour (TEXT_MUTED);
    g.setFont (font (10.5f, false, 0.04f));
    g.drawText (guideStr, 24, footerY, getWidth() - 250, 20, juce::Justification::centredLeft, true);

    const int lastNote = audioProcessor.getLastTriggeredNote();
    g.drawText ("LIVE MIDI: " + PulseForge::getMidiNoteName (lastNote) + " (#" + juce::String (lastNote) + ")",
                getWidth() - 220, footerY, 195, 20, juce::Justification::centredRight, false);
}

void PulseForgeAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Re-render cached backgrounds & soft drop shadows
    renderCachedShadows (w, h);

    // 1. Top Header Bar (8px grid alignment)
    seqModeBtn.setBounds (208, 22, 104, 32);
    arpModeBtn.setBounds (320, 22, 104, 32);

    for (int i = 0; i < 4; ++i)
        slotBtns[static_cast<size_t>(i)].setBounds (440 + i * 36, 22, 30, 32);

    runBtn.setBounds      (592, 22, 88, 32);
    previewBtn.setBounds  (688, 22, 136, 32);
    dragMidiBtn.setBounds (w - 156, 22, 132, 32);

    // 2. Dropdown Row
    int x = 24;
    const int gap = 8;
    genreBox.setBounds  (x, 94, 195, 32); x += 195 + gap;
    roleBox.setBounds   (x, 94, 185, 32); x += 185 + gap;
    scaleBox.setBounds  (x, 94, 165, 32); x += 165 + gap;
    rootBox.setBounds   (x, 94, 68,  32); x += 68  + gap;
    rateBox.setBounds   (x, 94, 74,  32); x += 74  + gap;
    arpDirBox.setBounds (x, 94, 128, 32); x += 128 + gap;
    hwMidiBox.setBounds (x, 94, juce::jmax (120, w - x - 24), 32);

    // 3. Action Buttons & Knobs
    generateBtn.setBounds   (24,  154, 102, 36);
    sampleMidiBtn.setBounds (132, 154, 114, 36);
    mutateBtn.setBounds     (252, 154, 76,  36);

    shiftLeftBtn.setBounds  (24,  196, 70, 32);
    shiftRightBtn.setBounds (100, 196, 70, 32);
    invertBtn.setBounds     (176, 196, 72, 32);
    clearBtn.setBounds      (254, 196, 74, 32);

    // 12 Knobs
    juce::Slider* knobs[] = {
        &densitySlider, &syncopationSlider, &spiceSlider, &rangeSlider,
        &swingSlider, &gateSlider, &stepsSlider, &octaveSlider,
        &arpOctSlider, &chanSlider, &ccNumSlider, &tempoRatioSlider
    };
    juce::Label* labels[] = {
        &densityLabel, &syncopationLabel, &spiceLabel, &rangeLabel,
        &swingLabel, &gateLabel, &stepsLabel, &octaveLabel,
        &arpOctLabel, &chanLabel, &ccNumLabel, &tempoRatioLabel
    };

    const int knobsStartX = 344;
    const int availKnobW = w - knobsStartX - 24;
    const int knobW = availKnobW / 12;

    for (int i = 0; i < 12; ++i)
    {
        const int kx = knobsStartX + i * knobW;
        labels[i]->setBounds (kx, 148, knobW, 14);
        knobs[i]->setBounds  (kx, 162, knobW, 72);
    }

    // 4. Modulation Lane Selector Tabs
    const int tabW = 84;
    const int tabStartX = w - (tabW * 4) - 24;
    tabVelBtn.setBounds  (tabStartX + 0 * tabW, 251, tabW - 4, 26);
    tabGateBtn.setBounds (tabStartX + 1 * tabW, 251, tabW - 4, 26);
    tabCcBtn.setBounds   (tabStartX + 2 * tabW, 251, tabW - 4, 26);
    tabProbBtn.setBounds (tabStartX + 3 * tabW, 251, tabW - 4, 26);

    // 5. Matrix Card
    const int matrixY = 286;
    const int matrixH = h - matrixY - 36;
    stepMatrix.setBounds (16, matrixY, w - 32, matrixH);
}
