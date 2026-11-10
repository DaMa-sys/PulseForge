#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "PatternData.h"
#include "StyleGenerator.h"
#include "PreviewSynth.h"
#include <atomic>

class PulseForgeAudioProcessor : public juce::AudioProcessor
{
public:
    PulseForgeAudioProcessor();
    ~PulseForgeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Pattern Management & Generator Actions
    //==========================================================================
    PulseForge::PatternBank& getActivePattern()
    {
        return patternSlots[static_cast<size_t>(activeSlotIndex.load())];
    }

    const PulseForge::PatternBank& getActivePattern() const
    {
        return patternSlots[static_cast<size_t>(activeSlotIndex.load())];
    }

    int getActiveSlotIndex() const noexcept { return activeSlotIndex.load(); }
    void setActiveSlotIndex (int idx);
    void copyPatternSlot (int srcIdx, int dstIdx);

    void triggerGeneratePattern (bool applyRecommendedScaleAndSwing = true);
    bool triggerLoadFromUserSamplesLibrary();
    bool triggerLoadSpecificMidiFile (const juce::File& file);
    void triggerMutatePattern();
    void triggerRotatePattern (int delta);
    void triggerInvertPattern();
    void triggerRequantizeToScale();
    void triggerClearPattern();

    juce::File exportActivePatternToTempMidiFile();

    //==========================================================================
    // Direct Hardware MIDI Out Port Management
    //==========================================================================
    juce::StringArray getAvailableHardwareMidiOuts() const;
    void setHardwareMidiOutDevice (const juce::String& deviceName);
    juce::String getCurrentHardwareMidiOutName() const;

    //==========================================================================
    // UI Telemetry
    //==========================================================================
    int getCurrentPlayStep() const noexcept { return currentPlayStep.load(); }
    int getLastTriggeredNote() const noexcept { return lastTriggeredNote.load(); }
    bool isTransportRunning() const noexcept { return transportRunning.load(); }
    double getCurrentHostBpm() const noexcept { return currentHostBpm.load(); }
    double getEffectiveBpm() const noexcept { return effectiveBpm.load(); }
    juce::String getLastLoadedMidiName() const;

    int computeStepMidiNote (int stepIdx, const PulseForge::StepData& step,
                             int rootMidi, PulseForge::ScaleType scale) const;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    mutable juce::CriticalSection patternLock;

private:
    struct ActiveNoteEvent
    {
        int midiNote = -1;
        int channel = 1;
        double remainingSamples = 0.0;
        bool waitingForLegatoOverlap = false;
    };

    struct PendingSubTrigger
    {
        double delaySamples = 0.0;
        double durationSamples = 0.0;
        int midiNote = 60;
        int velocity = 100;
        int channel = 1;
    };

    void sendMidiOut (const juce::MidiMessage& msg, juce::MidiBuffer& midiOut, int sampleOffset);
    void stopAllActiveNotes (juce::MidiBuffer& midiOut, int sampleOffset);
    void triggerStepEvents (int stepIdx, double stepDurationSamples, int sampleOffset, juce::MidiBuffer& midiOut);

    std::array<PulseForge::PatternBank, PulseForge::kNumPatternSlots> patternSlots;
    std::atomic<int> activeSlotIndex { 0 };

    // Playback & Host Sync timing state
    double currentSampleRate = 44100.0;
    double internalPpqPosition = 0.0;
    double lastExpectedNextBlockPpq = -1.0;
    bool wasPlayingLastBlock = false;
    int lastTriggeredStepIndex = -999999;
    int arpWalkIndex = 0;
    int arpDirectionSign = 1;

    std::vector<ActiveNoteEvent> activeNotes;
    std::vector<PendingSubTrigger> pendingSubTriggers;
    std::vector<int> heldInputNotes;
    int liveTransposeSemitones = 0;

    std::atomic<int> currentPlayStep { 0 };
    std::atomic<int> lastTriggeredNote { 36 };
    std::atomic<bool> transportRunning { false };
    std::atomic<double> currentHostBpm { 124.0 };
    std::atomic<double> effectiveBpm { 124.0 };
    juce::String lastLoadedMidiName;

    PulseForge::PreviewSynth previewSynth;

    // Optional Direct Hardware MIDI Output
    mutable juce::CriticalSection midiOutLock;
    std::unique_ptr<juce::MidiOutput> hardwareMidiOut;
    juce::String currentHardwareMidiOutName { "None (DAW MIDI Out)" };

    std::mt19937 probRng { std::random_device{}() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeAudioProcessor)
};
