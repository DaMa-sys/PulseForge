#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PulseForgeAudioProcessorEditor::PulseForgeAudioProcessorEditor (PulseForgeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    // 1. Global Header Dropdowns & Labels
    const juce::StringArray scales {
        "Minor Natural", "Dorian (House/Funk)", "Phrygian (Acid/Techno)",
        "Phrygian Dominant", "Minor Pentatonic", "Major Natural",
        "Mixolydian (Boogie)", "Harmonic Minor", "Lydian (Berlin)", "Chromatic"
    };
    scaleBox.addItemList (scales, 1);
    scaleBox.setLookAndFeel (&customLookAndFeel);
    addAndMakeVisible (scaleBox);

    const juce::StringArray roots { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    rootBox.addItemList (roots, 1);
    rootBox.setLookAndFeel (&customLookAndFeel);
    addAndMakeVisible (rootBox);

    // 2. Global Header Action Buttons
    trioGenBtn.getProperties().set ("primary", true);
    trioGenBtn.setLookAndFeel (&customLookAndFeel);
    trioGenBtn.onClick = [this]()
    {
        audioProcessor.triggerGenerateTrio();
        for (auto& s : strips)
            if (s != nullptr) s->repaint();
    };
    addAndMakeVisible (trioGenBtn);

    runBtn.setClickingTogglesState (true);
    runBtn.setLookAndFeel (&customLookAndFeel);
    addAndMakeVisible (runBtn);

    previewBtn.setClickingTogglesState (true);
    previewBtn.setLookAndFeel (&customLookAndFeel);
    addAndMakeVisible (previewBtn);

    dragMidiBtn.setLookAndFeel (&customLookAndFeel);
    dragMidiBtn.onClick = [this]()
    {
        auto tempFile = audioProcessor.exportActivePatternToTempMidiFile();
        if (tempFile.existsAsFile())
        {
            juce::StringArray files;
            files.add (tempFile.getFullPathName());
            performExternalDragDropOfFiles (files, true);
        }
    };
    addAndMakeVisible (dragMidiBtn);

    // 3. Attachments for Global Parameters
    scaleAttach   = std::make_unique<ComboAttachment>  (audioProcessor.apvts, "scale",        scaleBox);
    rootAttach    = std::make_unique<ComboAttachment>  (audioProcessor.apvts, "rootNote",     rootBox);
    runAttach     = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "autoPlay",     runBtn);
    previewAttach = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "previewSynth", previewBtn);

    // 4. The 3 Dedicated Horizontal Sequence Strips (Custom 3-Tone Azure Blue Palette)
    // Strip 0: Deep Marine Cobalt Azure for Grounded Bass Foundation
    strips[0] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 0, juce::Colour (0xFF38BDF8), "SEQ 1 : BASS", customLookAndFeel,
        juce::Colour (0xFF14476B), juce::Colour (0xFF0D324D));

    // Strip 1: Vibrant Electric Cyan Turquoise for Melodic Arpeggio & Lead Hook
    strips[1] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 1, juce::Colour (0xFF00F0FF), "SEQ 2 : ARP", customLookAndFeel,
        juce::Colour (0xFF0F5A6D), juce::Colour (0xFF0A3B48));

    // Strip 2: Royal Indigo / Cerulean Azure for Polyphonic Chord Progressions
    strips[2] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 2, juce::Colour (0xFF818CF8), "SEQ 3 : CHORDS", customLookAndFeel,
        juce::Colour (0xFF224E82), juce::Colour (0xFF153457));

    for (int i = 0; i < PulseForge::kNumTracks; ++i)
    {
        strips[static_cast<size_t>(i)]->onExpandToggled = [this]()
        {
            int h = 76;
            for (auto& s : strips)
                h += (s != nullptr ? s->getStripHeight() : 126) + 8;
            h += 24; // footer
            setSize (getWidth(), h);
        };
        addAndMakeVisible (*strips[static_cast<size_t>(i)]);
    }

    audioProcessor.hardwareSerial.onConnectionStatusChanged = [this](bool, const juce::String&)
    {
        juce::MessageManager::callAsync ([this]() { repaint(); });
    };

    setSize (1180, 502);
    startTimerHz (60);
}

PulseForgeAudioProcessorEditor::~PulseForgeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PulseForgeAudioProcessorEditor::renderCachedShadows (int width, int height)
{
    using namespace LandrFxTheme;
    cachedBackground = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (cachedBackground);

    auto cardArea = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);

    // Soft drop shadow around window chassis
    g.setColour (SHADOW_COLOR);
    g.fillRoundedRectangle (cardArea.translated (0.0f, 4.0f), 16.0f);

    // Chassis Gradient (#2FC0D0 -> #2587B5)
    juce::ColourGradient bgGrad (BG_TOP, 0.0f, cardArea.getY(),
                                 BG_BOTTOM, 0.0f, cardArea.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillRoundedRectangle (cardArea, 14.0f);

    // Top Master Header Bar
    auto headR = juce::Rectangle<float> (16.0f, 12.0f, static_cast<float> (width - 32), 62.0f);
    g.setColour (juce::Colour (0x0B, 0x3D, 0x5C).withAlpha (0.22f));
    g.fillRoundedRectangle (headR.translated (0.0f, 3.0f), 12.0f);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (headR, 12.0f);

    // Top Specular Hairline
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawHorizontalLine (static_cast<int> (headR.getY() + 0.5f), headR.getX() + 6.0f, headR.getRight() - 6.0f);
}

void PulseForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace LandrFxTheme;

    if (cachedBackground.isValid())
        g.drawImageAt (cachedBackground, 0, 0);

    // 1. Logo & Typography (Upper Left)
    g.setColour (TEXT_BRIGHT);
    g.setFont (font (15.0f, true, 0.14f));
    g.drawText ("PULSEFORGE", 30, 20, 150, 20, juce::Justification::centredLeft, false);

    g.setColour (TEXT_LABEL);
    g.setFont (font (8.5f, true, 0.08f));
    g.drawText ("3-TRACK MIDI ENGINE | DaMA", 30, 38, 170, 14, juce::Justification::centredLeft, false);

    // 2. Global Dropdown Labels
    g.setColour (TEXT_LABEL);
    g.setFont (font (9.0f, true, 0.08f));
    g.drawText ("MUSICAL SCALE", scaleBox.getX(), 15, scaleBox.getWidth(), 12, juce::Justification::centredLeft);
    g.drawText ("ROOT",          rootBox.getX(),  15, rootBox.getWidth(),  12, juce::Justification::centredLeft);

    // 3. Footer Telemetry & Status (Crisp Bold Typography)
    const int footerY = getHeight() - 24;
    const double bpm = audioProcessor.getCurrentHostBpm();
    const bool isPlaying = audioProcessor.isTransportRunning();

    juce::String status = "DaMA PulseForge v1.0 | 3 Multi-Timbral Tracks (Ch 1: Bass | Ch 2: Arp | Ch 3: Chords) | Ableton Sync: "
                        + juce::String (bpm, 1) + " BPM (" + (isPlaying ? "RUNNING" : "STOPPED") + ")";

    g.setColour (TEXT_BRIGHT.withAlpha (0.90f));
    g.setFont (font (10.0f, true, 0.04f));
    g.drawText (status, 24, footerY, getWidth() - 380, 18, juce::Justification::centredLeft, true);

    // Hardware Controller Telemetry Badge
    const bool hwConnected = audioProcessor.hardwareSerial.isConnected();
    const juce::String hwStatus = hwConnected ? ("HW: " + audioProcessor.hardwareSerial.getConnectedPortName()) : "HW: SCANNING";
    g.setColour (hwConnected ? juce::Colour (0xFF10B981) : juce::Colours::white.withAlpha (0.45f));
    g.fillEllipse (static_cast<float> (getWidth() - 355), static_cast<float> (footerY + 5), 7.0f, 7.0f);
    g.setColour (hwConnected ? juce::Colour (0xFF10B981) : TEXT_BRIGHT.withAlpha (0.80f));
    g.setFont (font (9.5f, true, 0.05f));
    g.drawText (hwStatus, getWidth() - 342, footerY, 115, 18, juce::Justification::centredLeft);

    const int lastNote = audioProcessor.getLastTriggeredNote();
    g.setColour (TEXT_BRIGHT.withAlpha (0.90f));
    g.setFont (font (10.0f, true, 0.04f));
    g.drawText ("LIVE MIDI: " + PulseForge::getMidiNoteName (lastNote) + " (#" + juce::String (lastNote) + ")",
                getWidth() - 220, footerY, 195, 18, juce::Justification::centredRight, false);
}

void PulseForgeAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    renderCachedShadows (w, h);

    // 1. Global Header Layout
    const int headY = 26;
    const int headH = 28;

    int x = 205;
    scaleBox.setBounds (x, headY, 160, headH);
    x += 168;

    rootBox.setBounds (x, headY, 72, headH);
    x += 80;

    trioGenBtn.setBounds (x, headY, 140, headH);
    x += 148;

    runBtn.setBounds (x, headY, 84, headH);
    x += 92;

    previewBtn.setBounds (x, headY, 120, headH);
    x += 128;

    dragMidiBtn.setBounds (x, headY, 100, headH);

    // 2. 3 Sequence Strips (Lanes)
    int curY = 76;
    for (int i = 0; i < PulseForge::kNumTracks; ++i)
    {
        if (strips[static_cast<size_t>(i)] != nullptr)
        {
            const int stripH = strips[static_cast<size_t>(i)]->getStripHeight();
            strips[static_cast<size_t>(i)]->setBounds (16, curY, w - 32, stripH);
            curY += stripH + 8;
        }
    }
}

void PulseForgeAudioProcessorEditor::timerCallback()
{
    for (auto& s : strips)
    {
        if (s != nullptr)
            s->updatePlayheadAndState();
    }

    repaint (getWidth() - 380, getHeight() - 26, 360, 24);
}
