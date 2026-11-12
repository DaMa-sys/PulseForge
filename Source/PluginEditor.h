#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "OrageTheme.h"
#include "SequenceStripComponent.h"

//==============================================================================
// Main Editor Window (Orage Evolver-Inspired 3-Track Strips & Global Header)
//==============================================================================
class PulseForgeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::DragAndDropContainer,
                                       private juce::Timer
{
public:
    explicit PulseForgeAudioProcessorEditor (PulseForgeAudioProcessor&);
    ~PulseForgeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderCachedShadows (int width, int height);

    PulseForgeAudioProcessor& audioProcessor;
    OrageLookAndFeel customLookAndFeel;

    juce::Image cachedBackground;

    // 1. Global Header Controls
    juce::ComboBox scaleBox;
    juce::ComboBox rootBox;

    juce::TextButton trioGenBtn    { "TRIO HARMONIZE" };
    juce::TextButton runBtn        { "FREE RUN" };
    juce::TextButton previewBtn    { "PREVIEW SYNTH" };
    juce::TextButton dragMidiBtn   { "DRAG MIDI" };

    // Attachments
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAttachment> scaleAttach, rootAttach;
    std::unique_ptr<ButtonAttachment> runAttach, previewAttach;

    // 2. The 3 Horizontal Sequence Strips (Lanes)
    std::array<std::unique_ptr<SequenceStripComponent>, PulseForge::kNumTracks> strips;
    int deviceCheckCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeAudioProcessorEditor)
};
