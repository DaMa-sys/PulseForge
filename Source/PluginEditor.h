#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "DamaTheme.h"

//==============================================================================
// LANDR-Styled 16-Step Matrix Component with 3D Depth & Cached Shadow Render
//==============================================================================
class StepMatrixComponent : public juce::Component
{
public:
    enum class BottomLaneMode : int
    {
        Velocity = 0,
        GateLength,
        FilterCC,
        Probability
    };

    explicit StepMatrixComponent (PulseForgeAudioProcessor& proc);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    void setBottomLaneMode (BottomLaneMode m) { bottomMode = m; repaint(); }
    BottomLaneMode getBottomLaneMode() const noexcept { return bottomMode; }

    void setShowModifiers (bool show) { showModifiers = show; resized(); repaint(); }
    bool isShowingModifiers() const noexcept { return showModifiers; }
    void toggleShowModifiers() { setShowModifiers (!showModifiers); }

private:
    void handleStepInteraction (juce::Point<int> pos, bool isFirstClick, bool isRightClick);

    PulseForgeAudioProcessor& processor;
    BottomLaneMode bottomMode = BottomLaneMode::Velocity;
    bool showModifiers = false;
    int activeDragRow = -1;
    int lastToggledStep = -1;
    bool dragGateTargetState = true;
    bool dragAccentTargetState = true;
    bool dragSlideTargetState = true;

    // Pre-rendered shadow cache for fast paint without per-frame blur
    juce::Image cachedPanelShadow;
};

//==============================================================================
// Main Editor Window (LANDR FX Monochromatic Aesthetic & 3D Tactile Polish)
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
    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text, juce::Colour col);
    void updateHwMidiButtonText();
    void showHardwareMidiMenu();
    void renderCachedShadows (int width, int height);

    PulseForgeAudioProcessor& audioProcessor;
    LandrLookAndFeel customLookAndFeel;

    // Pre-rendered shadow & card cache to avoid expensive real-time blur on repaints
    juce::Image cachedBackground;

    // Top Header Controls
    juce::TextButton seqModeBtn { "SEQUENCER" };
    juce::TextButton arpModeBtn { "ARPEGGIATOR" };
    juce::TextButton runBtn { "FREE RUN" };
    juce::TextButton previewBtn { "PREVIEW SYNTH" };
    juce::TextButton dragMidiBtn { "DRAG MIDI" };

    // 3 Multi-Timbral Sequence Selectors & Mutes
    std::array<juce::TextButton, 3> trackBtns {
        juce::TextButton { "1: BASS (CH 1)" },
        juce::TextButton { "2: ARP (CH 2)" },
        juce::TextButton { "3: CHORD (CH 3)" }
    };
    std::array<juce::TextButton, 3> trackMuteBtns {
        juce::TextButton { "M" },
        juce::TextButton { "M" },
        juce::TextButton { "M" }
    };

    // Dropdown Selectors
    juce::ComboBox genreBox, roleBox, scaleBox, rootBox, rateBox, arpDirBox;
    juce::TextButton hwMidiMenuBtn { "DAW ONLY" };

    // Action Buttons
    juce::TextButton generateBtn   { "GENERATE" };
    juce::TextButton trioGenBtn    { "TRIO HARMONIZE" };
    juce::TextButton sampleMidiBtn { "FROM SAMPLES" };
    juce::TextButton mutateBtn     { "MUTATE" };
    juce::TextButton shiftLeftBtn  { "< SHIFT" };
    juce::TextButton shiftRightBtn { "SHIFT >" };
    juce::TextButton invertBtn     { "INVERT" };
    juce::TextButton clearBtn      { "CLEAR" };

    // Collapsible Modifier Section Toggle
    juce::TextButton modifiersToggleBtn { juce::String::fromUTF8 ("\xe2\x96\xbe SHOW MODIFIERS") };

    // Bottom Lane Mode Tabs
    juce::TextButton tabVelBtn  { "VELOCITY" };
    juce::TextButton tabGateBtn { "GATE STEPS" };
    juce::TextButton tabCcBtn   { "FILTER CC" };
    juce::TextButton tabProbBtn { "PROBABILITY" };

    // Macro & Performance Knobs
    juce::Slider densitySlider, syncopationSlider, spiceSlider, rangeSlider;
    juce::Slider swingSlider, gateSlider, stepsSlider, octaveSlider, arpOctSlider, chanSlider, ccNumSlider, tempoRatioSlider;

    juce::Label densityLabel, syncopationLabel, spiceLabel, rangeLabel;
    juce::Label swingLabel, gateLabel, stepsLabel, octaveLabel, arpOctLabel, chanLabel, ccNumLabel, tempoRatioLabel;

    // Attachments
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAttachment> genreAttach, roleAttach, scaleAttach, rootAttach, rateAttach, arpDirAttach;
    std::unique_ptr<SliderAttachment> densityAttach, syncopationAttach, spiceAttach, rangeAttach;
    std::unique_ptr<SliderAttachment> swingAttach, gateAttach, stepsAttach, octaveAttach, arpOctAttach, chanAttach, ccNumAttach, tempoRatioAttach;
    std::unique_ptr<ButtonAttachment> runAttach, previewAttach;

    std::unique_ptr<juce::FileChooser> midiFileChooser;

    StepMatrixComponent stepMatrix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeAudioProcessorEditor)
};
