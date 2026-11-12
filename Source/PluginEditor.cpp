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

    // 4. The 3 Dedicated Horizontal Sequence Strips (Orage Evolver Titanium / Dark Graphite bays)
    strips[0] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 0, OrageTheme::TEXT_BRIGHT, "SEQ 1 : BASS", customLookAndFeel,
        juce::Colour (0xFF1A1C1F), juce::Colour (0xFF121315));

    strips[1] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 1, OrageTheme::TEXT_BRIGHT, "SEQ 2 : ARP", customLookAndFeel,
        juce::Colour (0xFF1C1E22), juce::Colour (0xFF141517));

    strips[2] = std::make_unique<SequenceStripComponent> (
        audioProcessor, 2, OrageTheme::TEXT_BRIGHT, "SEQ 3 : CHORDS", customLookAndFeel,
        juce::Colour (0xFF1E2024), juce::Colour (0xFF151719));

    for (int i = 0; i < PulseForge::kNumTracks; ++i)
    {
        strips[static_cast<size_t>(i)]->onExpandToggled = [this]()
        {
            int h = 78;
            for (auto& s : strips)
                h += (s != nullptr ? s->getStripHeight() : 134) + 8;
            h += 24; // footer
            setSize (getWidth(), h);
        };
        addAndMakeVisible (*strips[static_cast<size_t>(i)]);
    }

    audioProcessor.hardwareSerial.onConnectionStatusChanged = [this](bool, const juce::String&)
    {
        juce::MessageManager::callAsync ([this]() { repaint(); });
    };

    setSize (1180, 528);
    startTimerHz (60);
}

PulseForgeAudioProcessorEditor::~PulseForgeAudioProcessorEditor()
{
    audioProcessor.hardwareSerial.onConnectionStatusChanged = nullptr;
    stopTimer();
    setLookAndFeel (nullptr);
}

void PulseForgeAudioProcessorEditor::renderCachedShadows (int width, int height)
{
    using namespace OrageTheme;
    cachedBackground = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (cachedBackground);

    // 1. Dark Studio Surround / Outer Chassis
    g.fillAll (BG_CHASSIS_DARK);

    auto cardArea = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);

    // Soft ambient drop shadow around aluminum faceplate
    g.setColour (SHADOW_SOFT);
    g.fillRoundedRectangle (cardArea.translated (0.0f, 4.0f), 10.0f);

    // 2. Anodized Natural Brushed Aluminum Faceplate
    juce::ColourGradient alGrad (ALUMINUM_LIGHT, 0.0f, cardArea.getY(),
                                 ALUMINUM_DARK,  0.0f, cardArea.getBottom(), false);
    alGrad.addColour (0.45, ALUMINUM_BASE);
    g.setGradientFill (alGrad);
    g.fillRoundedRectangle (cardArea, 8.0f);

    // Precision machined perimeter bevel
    g.setColour (ALUMINUM_BEVEL);
    g.drawRoundedRectangle (cardArea, 8.0f, 1.2f);

    // Top specular hairline
    g.setColour (ALUMINUM_SPECULAR.withAlpha (0.75f));
    g.drawHorizontalLine (static_cast<int> (cardArea.getY() + 0.5f), cardArea.getX() + 8.0f, cardArea.getRight() - 8.0f);

    // 3. Top Master Header Silkscreen Frame (Seamless desktop chassis)
    auto headR = juce::Rectangle<float> (16.0f, 10.0f, static_cast<float> (width - 32), 62.0f);
    g.setColour (PANEL_LINE.withAlpha (0.40f));
    g.drawRoundedRectangle (headR, 6.0f, 1.0f);

    auto headWell = headR.reduced (1.0f);
    g.setColour (ALUMINUM_BASE.darker (0.02f));
    g.fillRoundedRectangle (headWell, 5.0f);
}


void PulseForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace OrageTheme;

    if (cachedBackground.isValid())
        g.drawImageAt (cachedBackground, 0, 0);

    // 1. Logo & Brand Typography (Upper Left, aligned with header)
    g.setColour (TEXT_DARK);
    g.setFont (getOrageLogoFont (27.0f));
    g.drawText ("Orage", 28, 14, 120, 27, juce::Justification::centredLeft, false);

    // Subtitle in Montserrat Italic (400) - Jet black, crisp definition
    g.setColour (TEXT_LABEL);
    g.setFont (montserratItalic (10.5f));
    g.drawText ("3-Track Generative Synthesizer", 30, 43, 240, 16, juce::Justification::centredLeft, false);

    // 2. Global Dropdown Labels in Montserrat Semi-Bold (600) - Jet black, high-contrast
    g.setColour (TEXT_LABEL);
    g.setFont (montserratSemiBold (10.0f, 0.08f));
    g.drawText ("MUSICAL SCALE", scaleBox.getX(), 11, scaleBox.getWidth(), 16, juce::Justification::centredLeft);
    g.drawText ("ROOT",          rootBox.getX(),  11, rootBox.getWidth(),  16, juce::Justification::centredLeft);

    // 3. Top Right: Warm Amber Power Jewel Lamp
    const float ledX = static_cast<float> (getWidth() - 36);
    const float ledY = 32.0f;
    const float ledR = 5.0f;
    drawJewelLed (g, ledX, ledY, ledR, AMBER_LED, AMBER_LED_BRIGHT, AMBER_LED_GLOW);

    // Power text label below LED in Montserrat Italic
    g.setColour (TEXT_LABEL);
    g.setFont (montserratItalic (9.0f));
    g.drawText ("power", static_cast<int> (ledX - 25.0f), static_cast<int> (ledY + 8.0f), 50, 14, juce::Justification::centred);

    // 4. Footer Telemetry & Status
    const int footerY = getHeight() - 22;
    const double bpm = audioProcessor.getCurrentHostBpm();
    const bool isPlaying = audioProcessor.isTransportRunning();

    juce::String status = "PulseForge v1.0 | 3-Track Generative Engine (Bass / Arp / Chords) | Ableton Live Sync: "
                        + juce::String (bpm, 1) + " BPM (" + (isPlaying ? "RUNNING" : "STOPPED") + ")";

    g.setColour (TEXT_LABEL);
    g.setFont (montserratSemiBold (10.0f, 0.04f));
    g.drawText (status, 18, footerY, getWidth() - 360, 18, juce::Justification::centredLeft, true);

    // Hardware Controller Telemetry Badge
    const bool hwConnected = audioProcessor.hardwareSerial.isConnected();
    const juce::String hwStatus = hwConnected ? ("HW: " + audioProcessor.hardwareSerial.getConnectedPortName()) : "HW: SCANNING";
    g.setColour (hwConnected ? AMBER_LED : juce::Colour (0xFF4A5060));
    g.fillEllipse (static_cast<float> (getWidth() - 335), static_cast<float> (footerY + 5), 7.0f, 7.0f);
    g.setColour (hwConnected ? TEXT_DARK : TEXT_MUTED);
    g.setFont (montserratSemiBold (9.5f, 0.05f));
    g.drawText (hwStatus, getWidth() - 322, footerY, 115, 18, juce::Justification::centredLeft);

    const int lastNote = audioProcessor.getLastTriggeredNote();
    g.setColour (TEXT_LABEL);
    g.setFont (montserratSemiBold (10.0f, 0.04f));
    g.drawText ("LIVE MIDI: " + PulseForge::getMidiNoteName (lastNote) + " (#" + juce::String (lastNote) + ")",
                getWidth() - 200, footerY, 180, 18, juce::Justification::centredRight, false);

}

void PulseForgeAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    renderCachedShadows (w, h);

    // 1. Global Header Layout (Slender height = 23px)
    const int headY = 29;
    const int headH = 23;

    int x = 206;
    scaleBox.setBounds (x, headY, 150, headH);
    x += 158;

    rootBox.setBounds (x, headY, 68, headH);
    x += 76;

    trioGenBtn.setBounds (x, headY, 140, headH);
    x += 148;

    runBtn.setBounds (x, headY, 82, headH);
    x += 90;

    previewBtn.setBounds (x, headY, 118, headH);
    x += 126;

    dragMidiBtn.setBounds (x, headY, 96, headH);


    // 2. 3 Sequence Strips (Lanes)
    int curY = 78;
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

    if (++deviceCheckCounter >= 60)
    {
        deviceCheckCounter = 0;
        for (auto& s : strips)
        {
            if (s != nullptr)
                s->updateOutputDeviceList();
        }
    }

    repaint (getWidth() - 380, getHeight() - 26, 360, 24);
}
