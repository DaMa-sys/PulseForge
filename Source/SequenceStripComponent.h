#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "DamaTheme.h"

//==============================================================================
// SequenceStripComponent: A modular, high-contrast horizontal sequence lane
// with dedicated Per-Track USB Hardware Output, Generative Controls & 16-Step Pads
//==============================================================================
class SequenceStripComponent : public juce::Component,
                               public juce::SettableTooltipClient
{
public:
    SequenceStripComponent (PulseForgeAudioProcessor& proc, int tIndex,
                            juce::Colour trackAccent, const juce::String& defaultName,
                            LandrLookAndFeel& lookAndFeel)
        : processor (proc), trackIndex (tIndex), accent (trackAccent), trackName (defaultName),
          customLnF (lookAndFeel)
    {
        setTooltip ("16-step sequence: Click step to toggle. Drag/scroll to change pitch. Right-click for accent & ratchet.");
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

        // 2. Action Buttons (GENERATE, MUTATE, MODIFIERS)
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

        expandBtn.setButtonText ("[ + MODS ]");
        expandBtn.setClickingTogglesState (true);
        expandBtn.setLookAndFeel (&customLnF);
        expandBtn.onClick = [this]()
        {
            isExpanded = expandBtn.getToggleState();
            expandBtn.setButtonText (isExpanded ? "[ - HIDE ]" : "[ + MODS ]");
            if (onExpandToggled)
                onExpandToggled();
        };
        addAndMakeVisible (expandBtn);

        // 3. Dropdowns (Genre, Role, Steps Length, and Per-Track USB Output!)
        const juce::StringArray genres {
            "Classic House", "Acid 303", "Italo/Synthwave", "Berlin School",
            "Melodic Techno", "Detroit Electro", "Funk Boogie", "Euclidean IDM"
        };
        genreBox.addItemList (genres, 1);
        genreBox.setLookAndFeel (&customLnF);
        genreBox.onChange = [this]()
        {
            processor.triggerGenerateTrack (trackIndex);
            repaint();
        };
        addAndMakeVisible (genreBox);

        const juce::StringArray roles {
            "Mono Bass", "Acid Line", "Classic Arp", "Lead Hook", "Chord Stabs", "Harmony Chords", "Modular Ratchet"
        };
        roleBox.addItemList (roles, 1);
        roleBox.setLookAndFeel (&customLnF);
        roleBox.onChange = [this]()
        {
            processor.triggerGenerateTrack (trackIndex);
            repaint();
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
                processor.setTrackTargetHardwareMidi (trackIndex, outputBox.getItemText (outputBox.getSelectedItemIndex()));
        };
        addAndMakeVisible (outputBox);
        updateOutputDeviceList();

        // 4. 6 Prominent Tactile Knobs (Increased size & clarity)
        setupKnob (densitySlider,     densityLabel,     "DENSITY");
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
    }

    void updateOutputDeviceList()
    {
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
    int getStripHeight() const noexcept { return isExpanded ? 168 : 126; }
    void setIsExpanded (bool exp)
    {
        isExpanded = exp;
        expandBtn.setToggleState (exp, juce::dontSendNotification);
        expandBtn.setButtonText (isExpanded ? "[ - HIDE ]" : "[ + MODS ]");
    }

    std::function<void()> onExpandToggled;

    void updatePlayheadAndState()
    {
        const int curStep = processor.getTrackPlayStep (trackIndex);
        if (curStep != lastDrawnStep || processor.isTransportRunning() != wasPlaying)
        {
            lastDrawnStep = curStep;
            wasPlaying = processor.isTransportRunning();
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace LandrFxTheme;
        const auto bounds = getLocalBounds().toFloat();

        // 1. Strip Card Chassis (Satin gradient with 14px rounded corners)
        auto cardArea = bounds.reduced (2.0f);

        // Soft ambient drop shadow
        g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.24f));
        g.fillRoundedRectangle (cardArea.translated (0.0f, 3.5f), 14.0f);

        // Card glass background
        const float normY = static_cast<float> (getY()) / static_cast<float> (juce::jmax (1, getParentHeight()));
        const juce::Colour localBg = BG_TOP.interpolatedWith (BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));
        juce::ColourGradient cardGrad (localBg.brighter (0.04f), 0.0f, cardArea.getY(),
                                       localBg.darker (0.05f), 0.0f, cardArea.getBottom(), false);
        g.setGradientFill (cardGrad);
        g.fillRoundedRectangle (cardArea, 14.0f);

        // Specular top hairline
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine (static_cast<int> (cardArea.getY() + 0.5f), cardArea.getX() + 8.0f, cardArea.getRight() - 8.0f);

        // 2. Left Accent Badge (Wider, no truncated text)
        auto badgeArea = juce::Rectangle<float> (cardArea.getX() + 12.0f, cardArea.getY() + 12.0f, 150.0f, 26.0f);
        g.setColour (accent.withAlpha (0.18f));
        g.fillRoundedRectangle (badgeArea, 6.0f);
        g.setColour (accent.withAlpha (0.60f));
        g.drawRoundedRectangle (badgeArea, 6.0f, 1.0f);

        // Luminous Color Indicator Pip
        g.setColour (accent);
        g.fillEllipse (badgeArea.getX() + 8.0f, badgeArea.getCentreY() - 4.5f, 9.0f, 9.0f);

        g.setColour (TEXT_BRIGHT);
        g.setFont (font (11.5f, true, 0.08f));
        g.drawText (trackName, static_cast<int> (badgeArea.getX() + 24.0f), static_cast<int> (badgeArea.getY()),
                    static_cast<int> (badgeArea.getWidth() - 28.0f), static_cast<int> (badgeArea.getHeight()),
                    juce::Justification::centredLeft, true);

        // 3. Dropdown Labels in Row 1
        g.setColour (TEXT_LABEL);
        g.setFont (font (8.5f, true, 0.06f));
        g.drawText ("GENRE STYLE", genreBox.getX(),  static_cast<int> (cardArea.getY() + 1.0f), genreBox.getWidth(),  11, juce::Justification::centredLeft);
        g.drawText ("ROLE / TYPE", roleBox.getX(),   static_cast<int> (cardArea.getY() + 1.0f), roleBox.getWidth(),   11, juce::Justification::centredLeft);
        g.drawText ("STEPS",       stepsBox.getX(),  static_cast<int> (cardArea.getY() + 1.0f), stepsBox.getWidth(),  11, juce::Justification::centredLeft);
        g.drawText ("MIDI OUTPUT", outputBox.getX(), static_cast<int> (cardArea.getY() + 1.0f), outputBox.getWidth(), 11, juce::Justification::centredLeft);

        // 4. Horizontal 16-Step Pitch Matrix
        paintStepMatrix (g, cardArea);
    }

    void resized() override
    {
        const int w = getWidth();

        // Row 1: Header controls
        const int topY = 14;
        const int btnH = 26;
        int x = 168;

        muteBtn.setBounds (x, topY, 44, btnH);
        x += 50;

        genreBox.setBounds (x, topY, 115, btnH);
        x += 121;

        roleBox.setBounds (x, topY, 115, btnH);
        x += 121;

        stepsBox.setBounds (x, topY, 52, btnH);
        x += 58;

        outputBox.setBounds (x, topY, 115, btnH);
        x += 121;

        genBtn.setBounds (x, topY, 80, btnH);
        x += 86;

        mutBtn.setBounds (x, topY, 64, btnH);
        x += 70;

        expandBtn.setBounds (w - 96, topY, 80, btnH);

        // Row 2: 6 Tactile Knobs (Aligned to 8px rhythm, height 68)
        const int knobY = 44;
        const int knobW = 58;
        const int knobGap = 5;
        int kx = 16;

        juce::Slider* knobs[] = {
            &densitySlider, &swingSlider, &baseOctSlider,
            &arpOctSlider, &chanSlider, &tempoRatioSlider
        };
        juce::Label* labels[] = {
            &densityLabel, &swingLabel, &baseOctLabel,
            &arpOctLabel, &chanLabel, &tempoRatioLabel
        };

        for (int i = 0; i < 6; ++i)
        {
            labels[i]->setBounds (kx, knobY, knobW, 12);
            knobs[i]->setBounds  (kx, knobY + 12, knobW, 56);
            kx += knobW + knobGap;
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        handleStepClick (e.getPosition(), e.mods.isRightButtonDown());
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
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

    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
        s.setLookAndFeel (&customLnF);
        s.onDragEnd = [this, &s]()
        {
            if (&s == &densitySlider || &s == &arpOctSlider)
                processor.triggerGenerateTrack (trackIndex);
            else
                processor.updatePlaybackSnapshot();
            repaint();
        };
        addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setFont (LandrFxTheme::font (9.0f, true, 0.07f));
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, LandrFxTheme::TEXT_LABEL);
        addAndMakeVisible (l);
    }

    int getNumSteps() const noexcept
    {
        return processor.getTrackNumSteps (trackIndex);
    }

    juce::Rectangle<float> getStepPadBounds (int stepIdx, juce::Rectangle<float> cardArea) const
    {
        const int nSteps = juce::jlimit (4, 32, getNumSteps());
        const float matrixX = cardArea.getX() + 398.0f;
        const float matrixY = cardArea.getY() + 56.0f;
        const float availW  = cardArea.getRight() - 14.0f - matrixX;
        const float padH    = 58.0f;

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
        using namespace LandrFxTheme;
        const auto& track = processor.getTrack (trackIndex);
        const int curPlayStep = processor.getTrackPlayStep (trackIndex);
        const bool isPlaying = processor.isTransportRunning();

        const float normY = static_cast<float> (getY()) / static_cast<float> (juce::jmax (1, getParentHeight()));
        const juce::Colour localBg = BG_TOP.interpolatedWith (BG_BOTTOM, juce::jlimit (0.0f, 1.0f, normY));

        const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (processor.apvts.getRawParameterValue ("scale")->load()));
        const int rootKey = static_cast<int> (processor.apvts.getRawParameterValue ("rootNote")->load());
        const int rootMidi = (track.baseOctave + 1) * 12 + rootKey;
        const int nSteps = juce::jlimit (4, 32, getNumSteps());

        for (int i = 0; i < nSteps; ++i)
        {
            auto pad = getStepPadBounds (i, cardArea);
            const auto& step = track.pattern.steps[static_cast<size_t>(i)];
            const bool isPlayhead = isPlaying && (curPlayStep == i);

            // 1. Recessed Pad Background
            g.setColour (localBg.darker (0.12f));
            g.fillRoundedRectangle (pad, 6.0f);
            g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.35f));
            g.drawRoundedRectangle (pad.reduced (0.5f), 6.0f, 1.0f);

            // 2. Active Step Pill
            if (step.active)
            {
                auto innerPill = pad.reduced (2.0f);
                juce::Colour topCol = accent.brighter (0.16f);
                juce::Colour botCol = accent.darker (0.12f);

                if (step.accent)
                {
                    topCol = topCol.brighter (0.25f);
                    botCol = botCol.brighter (0.10f);
                }

                juce::ColourGradient pillGrad (topCol, innerPill.getX(), innerPill.getY(),
                                               botCol, innerPill.getRight(), innerPill.getBottom(), false);
                g.setGradientFill (pillGrad);
                g.fillRoundedRectangle (innerPill, 5.0f);

                // Top highlight
                g.setColour (juce::Colours::white.withAlpha (0.32f));
                g.drawHorizontalLine (static_cast<int> (innerPill.getY() + 1.0f), innerPill.getX() + 2.0f, innerPill.getRight() - 2.0f);

                // Note Name Badge (Displays rich chord name like Cm9, F9, Ebmaj9, Gm11 or note pitch)
                juce::String noteName;
                if (track.role == PulseForge::InstrumentRole::HarmonyChords || track.role == PulseForge::InstrumentRole::PolyChordStab || step.chordType > 0)
                {
                    noteName = PulseForge::getChordBadgeName (rootMidi, step, scale, track.role);
                }
                else
                {
                    const int noteNum = processor.computeStepMidiNote (i, step, rootMidi, scale);
                    noteName = PulseForge::getMidiNoteName (noteNum);
                }

                g.setColour (TEXT_DARK);
                const float noteFontSize = (nSteps > 16) ? ((noteName.length() > 4) ? 7.0f : 8.5f) : ((noteName.length() > 4) ? 9.5f : 11.5f);
                g.setFont (font (noteFontSize, true, 0.02f));
                g.drawText (noteName, innerPill, juce::Justification::centred, false);

                // Accent dot
                if (step.accent)
                {
                    g.setColour (juce::Colours::white);
                    g.fillEllipse (innerPill.getRight() - 6.0f, innerPill.getY() + 3.0f, 3.5f, 3.5f);
                }

                // Slide marker '~'
                if (step.slide)
                {
                    g.setColour (TEXT_DARK.withAlpha (0.80f));
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
                // Inactive Dash
                g.setColour (juce::Colours::white.withAlpha (0.16f));
                g.setFont (font (11.0f, false, 0.0f));
                g.drawText ("-", pad, juce::Justification::centred, false);
            }

            // 3. Step Number Label at Top (Min 10px, 60% white, full white when playhead)
            g.setColour (isPlayhead ? juce::Colours::white : juce::Colours::white.withAlpha (0.60f));
            g.setFont (font (10.0f, true, 0.04f));
            g.drawText (juce::String (i + 1), static_cast<int> (pad.getX()), static_cast<int> (pad.getY() - 14.0f), static_cast<int> (pad.getWidth()), 12, juce::Justification::centred);

            // 4. Real-time Playhead LED Beam
            if (isPlayhead)
            {
                g.setColour (juce::Colours::white.withAlpha (0.90f));
                g.drawRoundedRectangle (pad.expanded (1.0f), 7.0f, 2.0f);

                g.setColour (juce::Colours::white);
                g.fillEllipse (pad.getCentreX() - 2.5f, pad.getBottom() + 2.0f, 5.0f, 5.0f);
            }
        }

        // 5. If Expanded: Paint Velocity & Gate modulation mini-lane
        if (isExpanded)
        {
            const float matrixX = cardArea.getX() + 398.0f;
            const float modY = cardArea.getY() + 122.0f;
            g.setColour (TEXT_LABEL);
            g.setFont (font (8.0f, true, 0.05f));
            g.drawText ("STEP VELOCITY", static_cast<int> (matrixX), static_cast<int> (modY - 10.0f), 120, 10, juce::Justification::centredLeft);

            for (int i = 0; i < nSteps; ++i)
            {
                auto pad = getStepPadBounds (i, cardArea);
                auto modPad = juce::Rectangle<float> (pad.getX(), modY, pad.getWidth(), 22.0f);
                const auto& step = track.pattern.steps[static_cast<size_t>(i)];

                g.setColour (localBg.darker (0.10f));
                g.fillRoundedRectangle (modPad, 3.0f);

                if (step.active)
                {
                    const float velNorm = static_cast<float> (step.velocity) / 127.0f;
                    const float barH = juce::jmax (2.0f, modPad.getHeight() * velNorm);
                    auto bar = modPad.removeFromBottom (barH);
                    g.setColour (accent.withAlpha (0.75f));
                    g.fillRoundedRectangle (bar, 2.0f);
                }
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceStripComponent)
};
