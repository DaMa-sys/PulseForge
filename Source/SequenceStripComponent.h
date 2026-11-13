#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "OrageTheme.h"

//==============================================================================
// SequenceStripComponent: A modular, high-contrast horizontal sequence lane
// with dedicated Per-Track USB Hardware Output, Generative Controls & 16-Step Pads
// Styled after Orage Evolver Hardware Aesthetics
//==============================================================================
class SequenceStripComponent : public juce::Component,
                               public juce::SettableTooltipClient
{
public:
    SequenceStripComponent (PulseForgeAudioProcessor& proc, int tIndex,
                            juce::Colour trackAccent, const juce::String& defaultName,
                            OrageLookAndFeel& lookAndFeel,
                            juce::Colour cardTopColor = juce::Colour(),
                            juce::Colour cardBotColor = juce::Colour())
        : processor (proc), trackIndex (tIndex), accent (trackAccent), trackName (defaultName),
          customLnF (lookAndFeel), customCardTop (cardTopColor), customCardBot (cardBotColor)
    {
        setTooltip ("Sequence lane: Click step to toggle. Drag/scroll to change pitch. Click EXP for Accent, Glide, Ratchet & Legato.");
        const juce::String prefix = "t" + juce::String (trackIndex) + "_";

        // 1. Mute Button
        muteBtn.setButtonText ("MUTE");
        muteBtn.setClickingTogglesState (true);
        muteBtn.setLookAndFeel (&customLnF);
        muteBtn.onClick = [this]()
        {
            processor.setTrackMuted (trackIndex, muteBtn.getToggleState());
        };
        addAndMakeVisible (muteBtn);

        // 2. Action Buttons (GENERATE, MUTATE, EXP)
        genBtn.setButtonText ("GENERATE");
        genBtn.getProperties().set ("primary", true);
        genBtn.setLookAndFeel (&customLnF);
        genBtn.onClick = [this]()
        {
            processor.triggerGenerateTrack (trackIndex);
            repaint();
        };
        addAndMakeVisible (genBtn);

        mutBtn.setButtonText ("MUTATE");
        mutBtn.setLookAndFeel (&customLnF);
        mutBtn.onClick = [this]()
        {
            processor.triggerMutateTrack (trackIndex);
            repaint();
        };
        addAndMakeVisible (mutBtn);

        expandBtn.setButtonText ("EXP +");
        expandBtn.setTooltip ("Toggle Expanded Controls (Accent, Glide/Slide, Ratchet, Legato, Velocity)");
        expandBtn.setClickingTogglesState (true);
        expandBtn.setLookAndFeel (&customLnF);
        expandBtn.onClick = [this]()
        {
            isExpanded = expandBtn.getToggleState();
            expandBtn.setButtonText (isExpanded ? "EXP -" : "EXP +");
            if (onExpandToggled)
                onExpandToggled();
        };
        addAndMakeVisible (expandBtn);


        // 3. Dropdowns (Genre, Role, Steps Length, and Per-Track USB Output!)
        const juce::StringArray genres {
            "Classic House (Deep/90s)",
            "Acid 303 (TB-303 Reference)",
            "Italo Disco / EDM",
            "Fisher (Tech House)",
            "Paul Kalkbrenner (Melodic Techno)",
            "UK Garage / Detroit (Disclosure)",
            "Fred again.. (Emotional UK)",
            "Bonobo (Organic Downtempo)"
        };
        genreBox.addItemList (genres, 1);
        genreBox.setLookAndFeel (&customLnF);
        genreBox.onChange = [this]()
        {
            const int id = genreBox.getSelectedId() - 1;
            if (id >= 0)
            {
                processor.setTrackGenre (trackIndex, static_cast<PulseForge::GenreStyle> (id));
                processor.triggerGenerateTrack (trackIndex);
                repaint();
            }
        };
        addAndMakeVisible (genreBox);

        const juce::StringArray roles {
            "Mono Bass", "Acid Line", "Classic Arp", "Lead Hook", "Chord Stabs", "Harmony Chords", "Modular Ratchet"
        };
        roleBox.addItemList (roles, 1);
        roleBox.setLookAndFeel (&customLnF);
        roleBox.onChange = [this]()
        {
            const int id = roleBox.getSelectedId() - 1;
            if (id >= 0)
            {
                processor.setTrackRole (trackIndex, static_cast<PulseForge::InstrumentRole> (id));
                processor.triggerGenerateTrack (trackIndex);
                repaint();
            }
        };
        addAndMakeVisible (roleBox);

        // Per-track Steps Selector (4, 8, 16, 32)
        const juce::StringArray stepOptions { "4", "8", "16", "32" };
        stepsBox.addItemList (stepOptions, 1);
        stepsBox.setLookAndFeel (&customLnF);
        stepsBox.onChange = [this]()
        {
            const int id = stepsBox.getSelectedId();
            int n = 16;
            if (id == 1) n = 4;
            else if (id == 2) n = 8;
            else if (id == 3) n = 16;
            else if (id == 4) n = 32;

            processor.setTrackNumSteps (trackIndex, n);
            processor.triggerGenerateTrack (trackIndex);
            repaint();
        };
        addAndMakeVisible (stepsBox);

        outputBox.setLookAndFeel (&customLnF);
        outputBox.onChange = [this]()
        {
            const int id = outputBox.getSelectedId();
            if (id == 1)
                processor.setTrackTargetHardwareMidi (trackIndex, "DAW Only");
            else if (id == 2)
                processor.setTrackTargetHardwareMidi (trackIndex, "All USB Synths");
            else if (id >= 3)
                processor.setTrackTargetHardwareMidi (trackIndex, outputBox.getText());
        };
        addAndMakeVisible (outputBox);
        updateOutputDeviceList();

        // 4. 6 Prominent Tactile Knobs (Increased size & clarity)
        setupKnob (densitySlider,     densityLabel,     "COMPLEXITY");
        densitySlider.onValueChange = [this]()
        {
            processor.updateTrackComplexity (trackIndex, static_cast<float> (densitySlider.getValue()));
            repaint();
        };
        setupKnob (swingSlider,       swingLabel,       "SWING");
        setupKnob (baseOctSlider,     baseOctLabel,     "BASE OCT");
        setupKnob (arpOctSlider,      arpOctLabel,      "ARP OCT");
        setupKnob (chanSlider,        chanLabel,        "MIDI CH");
        setupKnob (tempoRatioSlider,  tempoRatioLabel,  "RATIO");

        // 5. APVTS Attachments
        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

        densityAttach    = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "density",     densitySlider);
        swingAttach      = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "swing",       swingSlider);
        baseOctAttach    = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "baseOctave",  baseOctSlider);
        arpOctAttach     = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "arpOctaves",  arpOctSlider);
        chanAttach       = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "midiChannel", chanSlider);
        tempoRatioAttach = std::make_unique<SliderAttachment>   (processor.apvts, prefix + "tempoRatio",  tempoRatioSlider);
        genreAttach      = std::make_unique<ComboBoxAttachment> (processor.apvts, prefix + "genre",       genreBox);
        roleAttach       = std::make_unique<ComboBoxAttachment> (processor.apvts, prefix + "role",        roleBox);
        stepsAttach      = std::make_unique<ComboBoxAttachment> (processor.apvts, prefix + "numSteps",    stepsBox);
        muteAttach       = std::make_unique<ButtonAttachment>   (processor.apvts, prefix + "muted",       muteBtn);
    }

    ~SequenceStripComponent() override
    {
        muteBtn.setLookAndFeel (nullptr);
        genBtn.setLookAndFeel (nullptr);
        mutBtn.setLookAndFeel (nullptr);
        expandBtn.setLookAndFeel (nullptr);
        genreBox.setLookAndFeel (nullptr);
        roleBox.setLookAndFeel (nullptr);
        stepsBox.setLookAndFeel (nullptr);
        outputBox.setLookAndFeel (nullptr);
        densitySlider.setLookAndFeel (nullptr);
        swingSlider.setLookAndFeel (nullptr);
        baseOctSlider.setLookAndFeel (nullptr);
        arpOctSlider.setLookAndFeel (nullptr);
        chanSlider.setLookAndFeel (nullptr);
        tempoRatioSlider.setLookAndFeel (nullptr);
        densityLabel.setLookAndFeel (nullptr);
        swingLabel.setLookAndFeel (nullptr);
        baseOctLabel.setLookAndFeel (nullptr);
        arpOctLabel.setLookAndFeel (nullptr);
        chanLabel.setLookAndFeel (nullptr);
        tempoRatioLabel.setLookAndFeel (nullptr);
    }

    void updateOutputDeviceList()
    {
        if (outputBox.isPopupActive())
            return;

        const juce::String currentTarget = processor.getTrackTargetHardwareMidi (trackIndex);
        outputBox.clear (juce::dontSendNotification);
        outputBox.addItem ("DAW Only", 1);
        outputBox.addItem ("All USB Synths", 2);

        auto hwDevices = processor.getAvailableHardwareMidiOuts();
        int selectedId = 1;

        if (currentTarget == "All USB Synths")
            selectedId = 2;

        for (int i = 0; i < hwDevices.size(); ++i)
        {
            const int itemId = 3 + i;
            outputBox.addItem (hwDevices[i], itemId);
            if (hwDevices[i] == currentTarget)
                selectedId = itemId;
        }

        outputBox.setSelectedId (selectedId, juce::dontSendNotification);
    }

    bool getIsExpanded() const noexcept { return isExpanded; }
    int getStripHeight() const noexcept { return isExpanded ? 224 : 134; }
    void setIsExpanded (bool exp)
    {
        isExpanded = exp;
        expandBtn.setToggleState (exp, juce::dontSendNotification);
        expandBtn.setButtonText (isExpanded ? "EXP -" : "EXP +");
    }

    std::function<void()> onExpandToggled;

    void updatePlayheadAndState()
    {
        const int curStep = processor.getTrackPlayStep (trackIndex);
        const uint32_t curRev = processor.getPatternRevision();
        if (curStep != lastDrawnStep || processor.isTransportRunning() != wasPlaying || curRev != lastDrawnRevision)
        {
            lastDrawnStep = curStep;
            wasPlaying = processor.isTransportRunning();
            lastDrawnRevision = curRev;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace OrageTheme;
        const auto bounds = getLocalBounds().toFloat();

        // 1. Strip Card Chassis: Brushed Anodized Aluminum with Subtle Track Metallic Tint
        auto cardArea = bounds.reduced (2.0f);

        // Soft ambient drop shadow beneath strip
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (cardArea.translated (0.0f, 2.5f), 8.0f);

        // Brushed aluminum gradient faceplate with subtle sophisticated metallic tint
        juce::Colour topAlum, midAlum, botAlum, bevelCol;

        if (trackIndex == 0) // Track 1: Bass — Warm Satin Orange / Amber Tint
        {
            topAlum  = juce::Colour (0xFFF6ECE0); // Satin warm cream highlight
            midAlum  = juce::Colour (0xFFEBDECE); // Soft brushed amber/orange aluminum
            botAlum  = juce::Colour (0xFFD9C9B8); // Shaded bronze/orange aluminum
            bevelCol = juce::Colour (0xFFBBA894); // Warm machined bevel
        }
        else if (trackIndex == 1) // Track 2: Arp — Cool Satin Cyan / Ice Blue Tint
        {
            topAlum  = juce::Colour (0xFFE5EEF5); // Satin ice silver highlight
            midAlum  = juce::Colour (0xFFD3E2EC); // Soft brushed cyan aluminum
            botAlum  = juce::Colour (0xFFBFD2DE); // Shaded cobalt/cyan aluminum
            bevelCol = juce::Colour (0xFF92A7B5); // Cool cyan machined bevel
        }
        else // Track 3: Chords — Fresh Satin Sage / Emerald Green Tint
        {
            topAlum  = juce::Colour (0xFFE6EFE8); // Satin sage silver highlight
            midAlum  = juce::Colour (0xFFD4E3D8); // Soft brushed jade/green aluminum
            botAlum  = juce::Colour (0xFFC1D4C7); // Shaded emerald aluminum
            bevelCol = juce::Colour (0xFF92A999); // Sage machined bevel
        }

        juce::ColourGradient cardGrad (topAlum, 0.0f, cardArea.getY(),
                                       botAlum, 0.0f, cardArea.getBottom(), false);
        cardGrad.addColour (0.45, midAlum);
        g.setGradientFill (cardGrad);
        g.fillRoundedRectangle (cardArea, 6.0f);

        // Precision machined perimeter bevel matching metallic tint
        g.setColour (bevelCol);
        g.drawRoundedRectangle (cardArea, 6.0f, 1.0f);

        // Specular top hairline (machined chamfer highlight)
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawHorizontalLine (static_cast<int> (cardArea.getY() + 0.5f), cardArea.getX() + 6.0f, cardArea.getRight() - 6.0f);

        // 2. Left Track Badge: Solid black anodized plate with stark white text & Evolver Jewel LED
        auto badgeArea = juce::Rectangle<float> (cardArea.getX() + 12.0f, cardArea.getY() + 19.0f, 150.0f, 22.0f);
        g.setColour (juce::Colour (0xFF101114));
        g.fillRoundedRectangle (badgeArea, 3.0f);
        g.setColour (juce::Colour (0xFF3A3D44));
        g.drawRoundedRectangle (badgeArea, 3.0f, 1.0f);

        // Evolver Jewel Indicator Lamp: Track 0=Amber, Track 1=Cobalt Blue, Track 2=Emerald/Amber
        const float pipX = badgeArea.getX() + 12.0f;
        const float pipY = badgeArea.getCentreY();
        if (trackIndex == 1) // Arp Track: Iconic Evolver Cobalt Blue Jewel
            drawJewelLed (g, pipX, pipY, 3.5f, BLUE_LED, BLUE_LED_BRIGHT, BLUE_LED_GLOW);
        else if (trackIndex == 2) // Chords Track
            drawJewelLed (g, pipX, pipY, 3.5f, juce::Colour (0xFF10B981), juce::Colour (0xFF6EE7B7), juce::Colour (0x5510B981));
        else // Bass Track: Warm Tungsten Amber Jewel
            drawJewelLed (g, pipX, pipY, 3.5f, AMBER_LED, AMBER_LED_BRIGHT, AMBER_LED_GLOW);

        g.setColour (TEXT_BRIGHT);
        g.setFont (font (10.5f, true, 0.10f));
        g.drawText (trackName, static_cast<int> (badgeArea.getX() + 24.0f), static_cast<int> (badgeArea.getY()),
                    static_cast<int> (badgeArea.getWidth() - 28.0f), static_cast<int> (badgeArea.getHeight()),
                    juce::Justification::centredLeft, true);

        // 3. Dropdown Labels in Row 1: High-contrast technical jet-black silkscreen labels
        g.setColour (TEXT_LABEL);
        g.setFont (montserratSemiBold (9.5f, 0.06f));
        g.drawText ("GENRE STYLE", genreBox.getX(),  static_cast<int> (cardArea.getY() + 2.0f), genreBox.getWidth(),  13, juce::Justification::centredLeft);
        g.drawText ("ROLE / TYPE", roleBox.getX(),   static_cast<int> (cardArea.getY() + 2.0f), roleBox.getWidth(),   13, juce::Justification::centredLeft);
        g.drawText ("STEPS",       stepsBox.getX(),  static_cast<int> (cardArea.getY() + 2.0f), stepsBox.getWidth(),  13, juce::Justification::centredLeft);
        g.drawText ("MIDI OUTPUT", outputBox.getX(), static_cast<int> (cardArea.getY() + 2.0f), outputBox.getWidth(), 13, juce::Justification::centredLeft);

        // 4. Evolver Hairline Silkscreen Enclosures
        const float frameY = cardArea.getY() + 45.0f;
        const float frameH = 82.0f;

        // Knobs Macro Bay: clean hairline silkscreen enclosure
        auto knobBay = juce::Rectangle<float> (cardArea.getX() + 10.0f, frameY, 376.0f, frameH);
        g.setColour (juce::Colour (0xFF000000).withAlpha (0.45f));
        g.drawRoundedRectangle (knobBay, 5.0f, 1.0f);

        // High-contrast jet-black knob titles (pure silkscreen on aluminum)
        const char* knobTitles[] = { "COMPLEXITY", "SWING", "BASE OCT", "ARP OCT", "MIDI CH", "RATIO" };
        g.setColour (TEXT_DARK);
        int tkx = 16;
        for (int i = 0; i < 6; ++i)
        {
            const float fontSize = (i == 0) ? 9.0f : 10.0f;
            g.setFont (montserratSemiBold (fontSize, 0.04f));
            g.drawText (knobTitles[i], tkx - 2, static_cast<int> (frameY + 2.0f), 62, 13, juce::Justification::centred);
            tkx += 58 + 5;
        }

        // Step Matrix Bay: Recessed matte graphite socket well for high-contrast tactile chiclet keys
        auto matrixBay = juce::Rectangle<float> (cardArea.getX() + 392.0f, frameY, cardArea.getWidth() - 402.0f, frameH);
        g.setColour (juce::Colour (0xFF14161A));
        g.fillRoundedRectangle (matrixBay, 5.0f);
        g.setColour (juce::Colour (0xFF2E323A));
        g.drawRoundedRectangle (matrixBay, 5.0f, 1.0f);

        // 5. Horizontal Pitch Matrix + EXP Controls
        paintStepMatrix (g, cardArea);
    }

    void resized() override
    {
        const int w = getWidth();

        // Row 1: Header controls (Slender height = 22px, topY = 19)
        const int topY = 19;
        const int btnH = 22;
        int x = 168;

        muteBtn.setBounds (x, topY, 44, btnH);
        x += 49;

        genreBox.setBounds (x, topY, 155, btnH);
        x += 160;

        roleBox.setBounds (x, topY, 120, btnH);
        x += 125;

        stepsBox.setBounds (x, topY, 50, btnH);
        x += 55;

        outputBox.setBounds (x, topY, 118, btnH);
        x += 123;

        genBtn.setBounds (x, topY, 82, btnH);
        x += 87;

        mutBtn.setBounds (x, topY, 66, btnH);
        x += 71;

        expandBtn.setBounds (w - 88, topY, 72, btnH);

        // Row 2: 6 Tactile Knobs (Increased spacing, clear separation from Row 1)
        const int knobY = 60;
        const int knobW = 58;
        const int knobGap = 5;
        int kx = 16;

        juce::Slider* knobs[] = {
            &densitySlider, &swingSlider, &baseOctSlider,
            &arpOctSlider, &chanSlider, &tempoRatioSlider
        };

        for (int i = 0; i < 6; ++i)
        {
            knobs[i]->setBounds (kx, knobY, knobW, 64);
            kx += knobW + knobGap;
        }
    }



    void mouseDown (const juce::MouseEvent& e) override
    {
        if (handleExpModifierClick (e.getPosition()))
            return;

        handleStepClick (e.getPosition(), e.mods.isRightButtonDown());
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggedVelStepIndex >= 0)
        {
            const int dy = dragStartVelY - e.y;
            const juce::ScopedLock sl (processor.patternLock);
            auto& step = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(draggedVelStepIndex)];
            step.velocity = juce::jlimit (1, 127, initialDragVel + dy * 2);
            processor.updatePlaybackSnapshot();
            repaint();
            return;
        }

        if (draggedStepIndex >= 0)
        {
            const int dy = dragStartY - e.y;
            const int semiDelta = dy / 8;
            if (semiDelta != lastDragSemiDelta)
            {
                lastDragSemiDelta = semiDelta;
                const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));
                const juce::ScopedLock sl (processor.patternLock);
                auto& step = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(draggedStepIndex)];
                step.semitoneOffset = juce::jlimit (-24, 24, initialDragOffset + semiDelta);
                step.semitoneOffset = PulseForge::quantizeSemitoneToScale (step.semitoneOffset, scale);
                processor.updatePlaybackSnapshot();
                repaint();
            }
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        draggedStepIndex = -1;
        draggedVelStepIndex = -1;
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        const int s = getStepAtPoint (e.getPosition());
        if (s >= 0)
        {
            const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));
            const int delta = (wheel.deltaY > 0.0f) ? 1 : ((wheel.deltaY < 0.0f) ? -1 : 0);
            if (delta != 0)
            {
                const juce::ScopedLock sl (processor.patternLock);
                auto& step = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(s)];
                step.semitoneOffset = juce::jlimit (-24, 24, step.semitoneOffset + delta);
                step.semitoneOffset = PulseForge::quantizeSemitoneToScale (step.semitoneOffset, scale);
                processor.updatePlaybackSnapshot();
                repaint();
            }
        }
    }

private:
    PulseForgeAudioProcessor& processor;
    int trackIndex = 0;
    juce::Colour accent;
    juce::String trackName;
    LandrLookAndFeel& customLnF;

    bool isExpanded = false;
    int lastDrawnStep = -1;
    bool wasPlaying = false;

    // Mouse drag state
    int draggedStepIndex = -1;
    int dragStartY = 0;
    int initialDragOffset = 0;
    int lastDragSemiDelta = 0;

    int draggedVelStepIndex = -1;
    int dragStartVelY = 0;
    int initialDragVel = 100;
    uint32_t lastDrawnRevision = 0;

    // Custom Module Blue Hue
    juce::Colour customCardTop;
    juce::Colour customCardBot;

    // UI Controls
    juce::TextButton muteBtn;
    juce::TextButton genBtn;
    juce::TextButton mutBtn;
    juce::TextButton expandBtn;
    juce::ComboBox   genreBox;
    juce::ComboBox   roleBox;
    juce::ComboBox   outputBox;
    juce::ComboBox   stepsBox;

    juce::Slider densitySlider, swingSlider, baseOctSlider, arpOctSlider, chanSlider, tempoRatioSlider;
    juce::Label  densityLabel,  swingLabel,  baseOctLabel,  arpOctLabel,  chanLabel,  tempoRatioLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   densityAttach, swingAttach, baseOctAttach, arpOctAttach, chanAttach, tempoRatioAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> genreAttach, roleAttach, stepsAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   muteAttach;

    void setupKnob (juce::Slider& s, juce::Label&, const juce::String&)
    {
        s.setLookAndFeel (&customLnF);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

        s.onDragEnd = [this, &s]()
        {
            if (&s == &densitySlider)
                processor.triggerGenerateTrack (trackIndex);
            else
                processor.updatePlaybackSnapshot();
            repaint();
        };
        addAndMakeVisible (s);
    }



    int getNumSteps() const noexcept
    {
        return processor.getTrackNumSteps (trackIndex);
    }

    juce::Rectangle<float> getStepPadBounds (int stepIdx, juce::Rectangle<float> cardArea) const
    {
        const int nSteps = juce::jlimit (4, 32, getNumSteps());
        const float matrixX = cardArea.getX() + 398.0f;
        const float matrixY = cardArea.getY() + 58.0f;
        const float availW  = cardArea.getRight() - 14.0f - matrixX;
        const float padH    = 60.0f;


        const float padGap = (nSteps > 16) ? 2.0f : 3.5f;
        const float beatExtraGap = (nSteps > 16) ? 3.0f : 5.0f;
        const int numBeatGroups = juce::jmax (0, (nSteps / 4) - 1);
        const float totalGaps = (static_cast<float> (nSteps - 1) * padGap) + (static_cast<float> (numBeatGroups) * beatExtraGap);
        const float padW = juce::jmax (16.0f, (availW - totalGaps) / static_cast<float> (nSteps));

        const int beatGroups = stepIdx / 4;
        const float x = matrixX + static_cast<float> (stepIdx) * (padW + padGap) + static_cast<float> (beatGroups) * beatExtraGap;
        return { x, matrixY, padW, padH };
    }

    int getStepAtPoint (juce::Point<int> pt) const
    {
        const auto cardArea = getLocalBounds().toFloat().reduced (2.0f);
        const int nSteps = juce::jlimit (4, 32, getNumSteps());
        for (int i = 0; i < nSteps; ++i)
        {
            if (getStepPadBounds (i, cardArea).contains (pt.toFloat()))
                return i;
        }
        return -1;
    }

    bool handleExpModifierClick (juce::Point<int> pt)
    {
        if (!isExpanded)
            return false;

        const auto cardArea = getLocalBounds().toFloat().reduced (2.0f);
        const float expY = cardArea.getY() + 116.0f;
        if (pt.y < expY || pt.y > expY + 86.0f)
            return false;

        const int nSteps = juce::jlimit (4, 32, getNumSteps());
        for (int i = 0; i < nSteps; ++i)
        {
            auto pad = getStepPadBounds (i, cardArea);
            if (pt.x >= pad.getX() && pt.x <= pad.getRight())
            {
                const juce::ScopedLock sl (processor.patternLock);
                auto& step = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(i)];

                if (pt.y >= expY && pt.y < expY + 16.0f)
                {
                    step.accent = !step.accent;
                }
                else if (pt.y >= expY + 17.0f && pt.y < expY + 33.0f)
                {
                    step.slide = !step.slide;
                }
                else if (pt.y >= expY + 34.0f && pt.y < expY + 50.0f)
                {
                    step.ratchet = (step.ratchet % 4) + 1;
                }
                else if (pt.y >= expY + 51.0f && pt.y < expY + 67.0f)
                {
                    step.gate = (step.gate >= 1.2f) ? 0.75f : 1.50f;
                }
                else if (pt.y >= expY + 68.0f && pt.y < expY + 86.0f)
                {
                    draggedVelStepIndex = i;
                    dragStartVelY = pt.y;
                    initialDragVel = step.velocity;
                }

                processor.updatePlaybackSnapshot();
                repaint();
                return true;
            }
        }
        return false;
    }

    void handleStepClick (juce::Point<int> pt, bool isRightClick)
    {
        const int s = getStepAtPoint (pt);
        if (s < 0 || s >= PulseForge::kMaxSteps)
            return;

        const juce::ScopedLock sl (processor.patternLock);
        auto& step = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(s)];

        if (isRightClick)
        {
            juce::PopupMenu menu;
            menu.addItem (1, "Toggle Accent", true, step.accent);
            menu.addItem (2, "Toggle Slide (Portamento)", true, step.slide);
            menu.addSeparator();
            menu.addItem (3, "Ratchet: 1x (Standard)", true, step.ratchet == 1);
            menu.addItem (4, "Ratchet: 2x (Double Repeat)", true, step.ratchet == 2);
            menu.addItem (5, "Ratchet: 3x (Triplet Burst)", true, step.ratchet == 3);
            menu.addItem (6, "Ratchet: 4x (Roll)", true, step.ratchet == 4);
            menu.addSeparator();
            menu.addItem (7, "Octave: -1", true, step.octaveShift == -1);
            menu.addItem (8, "Octave: 0",  true, step.octaveShift == 0);
            menu.addItem (9, "Octave: +1", true, step.octaveShift == 1);

            menu.showMenuAsync (juce::PopupMenu::Options(), [this, s] (int result)
            {
                if (result == 0) return;
                const juce::ScopedLock sl (processor.patternLock);
                auto& st = processor.getTrack (trackIndex).pattern.steps[static_cast<size_t>(s)];
                if (result == 1) st.accent = !st.accent;
                else if (result == 2) st.slide = !st.slide;
                else if (result == 3) st.ratchet = 1;
                else if (result == 4) st.ratchet = 2;
                else if (result == 5) st.ratchet = 3;
                else if (result == 6) st.ratchet = 4;
                else if (result == 7) st.octaveShift = -1;
                else if (result == 8) st.octaveShift = 0;
                else if (result == 9) st.octaveShift = 1;
                processor.updatePlaybackSnapshot();
                repaint();
            });
            return;
        }

        // Left click: toggle active
        step.active = !step.active;
        processor.updatePlaybackSnapshot();
        repaint();

        // Start drag tracking for pitch change
        draggedStepIndex = s;
        dragStartY = pt.y;
        initialDragOffset = step.semitoneOffset;
        lastDragSemiDelta = 0;
    }

    void paintStepMatrix (juce::Graphics& g, juce::Rectangle<float> cardArea)
    {
        using namespace OrageTheme;
        const auto& track = processor.getTrack (trackIndex);
        const int curPlayStep = processor.getTrackPlayStep (trackIndex);
        const bool isPlaying = processor.isTransportRunning();

        const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));
        const int rootKey = static_cast<int> (processor.apvts.getRawParameterValue ("rootNote")->load());
        const juce::String pfx = "t" + juce::String (trackIndex) + "_";
        const int curBaseOct = (processor.apvts.getRawParameterValue (pfx + "baseOctave") != nullptr)
                                 ? static_cast<int> (std::round (processor.apvts.getRawParameterValue (pfx + "baseOctave")->load()))
                                 : track.baseOctave;
        const int curArpOct  = (processor.apvts.getRawParameterValue (pfx + "arpOctaves") != nullptr)
                                 ? static_cast<int> (std::round (processor.apvts.getRawParameterValue (pfx + "arpOctaves")->load()))
                                 : track.arpOctaves;
        const int rootMidi = (curBaseOct + 1) * 12 + rootKey;
        const int nSteps = juce::jlimit (4, 32, getNumSteps());

        for (int i = 0; i < nSteps; ++i)
        {
            auto pad = getStepPadBounds (i, cardArea);
            const auto& step = track.pattern.steps[static_cast<size_t>(i)];
            const bool isPlayhead = isPlaying && (curPlayStep == i);

            // 1. Recessed Pad Socket (Machined black pocket)
            g.setColour (juce::Colour (0xFF0E0F11));
            g.fillRoundedRectangle (pad, 4.0f);
            g.setColour (juce::Colour (0xFF26282D));
            g.drawRoundedRectangle (pad.reduced (0.5f), 4.0f, 1.0f);

            // 2. Step Pad Body
            if (step.active)
            {
                auto innerPill = pad.reduced (1.5f);
                juce::Colour topCol = step.accent ? PAD_ACTIVE_ACCENT : PAD_ACTIVE;
                juce::Colour botCol = step.accent ? juce::Colour (0xFFEDE3D0) : juce::Colour (0xFFDFE2E6);

                juce::ColourGradient pillGrad (topCol, innerPill.getX(), innerPill.getY(),
                                               botCol, innerPill.getRight(), innerPill.getBottom(), false);
                g.setGradientFill (pillGrad);
                g.fillRoundedRectangle (innerPill, 3.5f);

                // Top highlight
                g.setColour (juce::Colours::white.withAlpha (0.60f));
                g.drawHorizontalLine (static_cast<int> (innerPill.getY() + 0.5f), innerPill.getX() + 2.0f, innerPill.getRight() - 2.0f);

                // Note Name Badge
                juce::String noteName;
                if (track.role == PulseForge::InstrumentRole::HarmonyChords || track.role == PulseForge::InstrumentRole::PolyChordStab || step.chordType > 0)
                {
                    noteName = PulseForge::getChordBadgeName (rootMidi, step, scale, track.role);
                }
                else
                {
                    const int noteNum = processor.computeStepMidiNote (i, step, rootMidi, scale, curArpOct);
                    noteName = PulseForge::getMidiNoteName (noteNum);
                }

                // Pure deep black text on stark white pad for maximum legibility!
                g.setColour (TEXT_DARK);
                const float noteFontSize = (nSteps > 16) ? ((noteName.length() > 4) ? 8.5f : 10.0f) : ((noteName.length() > 4) ? 10.5f : 12.5f);
                g.setFont (montserratSemiBold (noteFontSize, 0.02f));
                g.drawText (noteName, innerPill, juce::Justification::centred, false);


                // Accent: Warm amber vintage jewel pip
                if (step.accent)
                {
                    g.setColour (AMBER_LED_GLOW);
                    g.fillEllipse (innerPill.getRight() - 7.5f, innerPill.getY() + 1.5f, 6.0f, 6.0f);
                    g.setColour (AMBER_LED);
                    g.fillEllipse (innerPill.getRight() - 6.0f, innerPill.getY() + 3.0f, 3.5f, 3.5f);
                }

                // Slide marker '~'
                if (step.slide)
                {
                    g.setColour (TEXT_DARK.withAlpha (0.85f));
                    g.setFont (font (8.0f, true, 0.0f));
                    g.drawText ("~", static_cast<int> (innerPill.getX() + 2.0f), static_cast<int> (innerPill.getY() + 1.0f), 8, 8, juce::Justification::centred);
                }

                // Ratchet marker 'x2', 'x3'
                if (step.ratchet > 1)
                {
                    g.setColour (TEXT_DARK.withAlpha (0.85f));
                    g.setFont (font (7.5f, true, 0.0f));
                    g.drawText ("x" + juce::String (step.ratchet), static_cast<int> (innerPill.getRight() - 14.0f), static_cast<int> (innerPill.getBottom() - 11.0f), 12, 10, juce::Justification::centredRight);
                }
            }
            else
            {
                // Inactive Pad: Matte charcoal with soft subtle dash
                auto innerPill = pad.reduced (1.5f);
                g.setColour (PAD_OFF);
                g.fillRoundedRectangle (innerPill, 3.5f);

                g.setColour (juce::Colours::white.withAlpha (0.16f));
                g.setFont (font (11.0f, false, 0.0f));
                g.drawText ("-", pad, juce::Justification::centred, false);
            }

            // 3. Step Number Label at Top: High-contrast white/silver on dark graphite pocket
            g.setColour (isPlayhead ? AMBER_LED : juce::Colour (0xFFE2E6EC));
            g.setFont (montserratSemiBold (11.0f, 0.04f));
            g.drawText (juce::String (i + 1), static_cast<int> (pad.getX()), static_cast<int> (pad.getY() - 15.0f), static_cast<int> (pad.getWidth()), 14, juce::Justification::centred);

            // 4. Real-time Playhead: Warm Amber glowing border & moving LED
            if (isPlayhead)
            {
                g.setColour (AMBER_LED);
                g.drawRoundedRectangle (pad.expanded (1.0f), 5.0f, 1.8f);

                g.setColour (AMBER_LED_GLOW);
                g.fillEllipse (pad.getCentreX() - 4.0f, pad.getBottom() + 1.0f, 8.0f, 8.0f);
                g.setColour (AMBER_LED);
                g.fillEllipse (pad.getCentreX() - 2.5f, pad.getBottom() + 2.5f, 5.0f, 5.0f);
            }
        }

        // 5. If Expanded: Paint Full Interactive Modifiers Matrix (ACC, GLD, RAT, LEG, VEL)
        if (isExpanded)
        {
            const float expY = cardArea.getY() + 116.0f;
            const float labelX = cardArea.getX() + 328.0f;
            const float labelW = 62.0f;

            // Row Title Tags on the Left
            g.setFont (font (8.5f, true, 0.06f));

            g.setColour (AMBER_LED);
            g.drawText ("ACCENT", static_cast<int> (labelX), static_cast<int> (expY + 1.0f), static_cast<int> (labelW), 14, juce::Justification::centredRight);

            g.setColour (juce::Colour (0xFFE2E5EA));
            g.drawText ("GLIDE", static_cast<int> (labelX), static_cast<int> (expY + 18.0f), static_cast<int> (labelW), 14, juce::Justification::centredRight);

            g.setColour (juce::Colour (0xFFD0D4DA));
            g.drawText ("RATCHET", static_cast<int> (labelX), static_cast<int> (expY + 35.0f), static_cast<int> (labelW), 14, juce::Justification::centredRight);

            g.setColour (juce::Colour (0xFFB0B5BD));
            g.drawText ("LEGATO", static_cast<int> (labelX), static_cast<int> (expY + 52.0f), static_cast<int> (labelW), 14, juce::Justification::centredRight);

            g.setColour (juce::Colour (0xFF8A909A));
            g.drawText ("VELOCITY", static_cast<int> (labelX), static_cast<int> (expY + 69.0f), static_cast<int> (labelW), 14, juce::Justification::centredRight);

            for (int i = 0; i < nSteps; ++i)
            {
                auto pad = getStepPadBounds (i, cardArea);
                const auto& step = track.pattern.steps[static_cast<size_t>(i)];

                auto rAcc = juce::Rectangle<float> (pad.getX(), expY, pad.getWidth(), 14.0f);
                auto rGld = juce::Rectangle<float> (pad.getX(), expY + 17.0f, pad.getWidth(), 14.0f);
                auto rRat = juce::Rectangle<float> (pad.getX(), expY + 34.0f, pad.getWidth(), 14.0f);
                auto rLeg = juce::Rectangle<float> (pad.getX(), expY + 51.0f, pad.getWidth(), 14.0f);
                auto rVel = juce::Rectangle<float> (pad.getX(), expY + 68.0f, pad.getWidth(), 14.0f);

                // Row 1: ACCENT
                if (step.accent)
                {
                    g.setColour (AMBER_LED);
                    g.fillRoundedRectangle (rAcc, 3.0f);
                    g.setColour (TEXT_DARK);
                    g.setFont (font (8.5f, true, 0.02f));
                    g.drawText ("ACC", rAcc, juce::Justification::centred, false);
                }
                else
                {
                    g.setColour (juce::Colours::black.withAlpha (0.40f));
                    g.fillRoundedRectangle (rAcc, 3.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.25f));
                    g.setFont (font (8.0f, false, 0.0f));
                    g.drawText ("-", rAcc, juce::Justification::centred, false);
                }

                // Row 2: GLIDE (TB-303 Slide)
                if (step.slide)
                {
                    g.setColour (juce::Colour (0xFFE2E5EA));
                    g.fillRoundedRectangle (rGld, 3.0f);
                    g.setColour (TEXT_DARK);
                    g.setFont (font (8.5f, true, 0.02f));
                    g.drawText ("GLD", rGld, juce::Justification::centred, false);
                }
                else
                {
                    g.setColour (juce::Colours::black.withAlpha (0.40f));
                    g.fillRoundedRectangle (rGld, 3.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.25f));
                    g.setFont (font (8.0f, false, 0.0f));
                    g.drawText ("-", rGld, juce::Justification::centred, false);
                }

                // Row 3: RATCHET
                if (step.ratchet > 1)
                {
                    g.setColour (juce::Colour (0xFFFFF0D0));
                    g.fillRoundedRectangle (rRat, 3.0f);
                    g.setColour (TEXT_DARK);
                    g.setFont (font (8.5f, true, 0.02f));
                    g.drawText ("x" + juce::String (step.ratchet), rRat, juce::Justification::centred, false);
                }
                else
                {
                    g.setColour (juce::Colours::black.withAlpha (0.40f));
                    g.fillRoundedRectangle (rRat, 3.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.25f));
                    g.setFont (font (8.0f, false, 0.0f));
                    g.drawText ("1x", rRat, juce::Justification::centred, false);
                }

                // Row 4: LEGATO
                if (step.gate >= 1.2f)
                {
                    g.setColour (juce::Colour (0xFFC8CCD4));
                    g.fillRoundedRectangle (rLeg, 3.0f);
                    g.setColour (TEXT_DARK);
                    g.setFont (font (8.5f, true, 0.02f));
                    g.drawText ("LEG", rLeg, juce::Justification::centred, false);
                }
                else
                {
                    g.setColour (juce::Colours::black.withAlpha (0.40f));
                    g.fillRoundedRectangle (rLeg, 3.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.25f));
                    g.setFont (font (8.0f, false, 0.0f));
                    g.drawText ("-", rLeg, juce::Justification::centred, false);
                }

                // Row 5: VELOCITY
                g.setColour (juce::Colours::black.withAlpha (0.45f));
                g.fillRoundedRectangle (rVel, 3.0f);

                if (step.active)
                {
                    const float velRatio = static_cast<float> (juce::jlimit (1, 127, step.velocity)) / 127.0f;
                    auto bar = rVel.withWidth (rVel.getWidth() * velRatio);
                    g.setColour (AMBER_LED.withAlpha (0.85f));
                    g.fillRoundedRectangle (bar, 3.0f);

                    g.setColour (juce::Colours::white);
                    g.setFont (font (7.5f, true, 0.0f));
                    g.drawText (juce::String (step.velocity), rVel, juce::Justification::centred, false);
                }
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceStripComponent)
};
