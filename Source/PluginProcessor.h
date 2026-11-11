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
    //==========================================================================
    // Track & Multi-Sequence Management (Up to 3 Sequences)
    //==========================================================================
    int getActiveTrackIndex() const noexcept { return activeTrackIndex.load(); }
    void setActiveTrackIndex (int idx);

    PulseForge::TrackData& getTrack (int idx)
    {
        return tracks[static_cast<size_t>(juce::jlimit (0, PulseForge::kNumTracks - 1, idx))];
    }
    const PulseForge::TrackData& getTrack (int idx) const
    {
        return tracks[static_cast<size_t>(juce::jlimit (0, PulseForge::kNumTracks - 1, idx))];
    }

    PulseForge::TrackData& getActiveTrack()
    {
        return getTrack (activeTrackIndex.load());
    }
    const PulseForge::TrackData& getActiveTrack() const
    {
        return getTrack (activeTrackIndex.load());
    }

    PulseForge::PatternBank& getActivePattern()
    {
        return getActiveTrack().pattern;
    }
    const PulseForge::PatternBank& getActivePattern() const
    {
        return getActiveTrack().pattern;
    }

    bool isTrackMuted (int idx) const noexcept;
    void setTrackMuted (int idx, bool muted);
    void toggleTrackMuted (int idx);

    void triggerGeneratePattern (bool applyRecommendedScaleAndSwing = true);
    void triggerGenerateTrack (int trackIdx);
    void triggerMutateTrack (int trackIdx);
    void triggerClearTrack (int trackIdx);
    void triggerRotateTrack (int trackIdx, int delta);
    void triggerGenerateTrio();
    bool triggerLoadFromUserSamplesLibrary();
    bool triggerLoadSpecificMidiFile (const juce::File& file);
    void triggerMutatePattern();
    void triggerRotatePattern (int delta);
    void triggerInvertPattern();
    void triggerRequantizeToScale();
    void triggerClearPattern();

    juce::File exportActivePatternToTempMidiFile();

    //==========================================================================
    // Multi-Device Direct Hardware MIDI Output Management
    //==========================================================================
    juce::StringArray getAvailableHardwareMidiOuts() const;
    bool isHardwareMidiOutEnabled (const juce::String& deviceName) const;
    void setHardwareMidiOutEnabled (const juce::String& deviceName, bool shouldEnable);
    void toggleHardwareMidiOut (const juce::String& deviceName);
    void setAllHardwareMidiOuts (bool enableAll);
    juce::StringArray getEnabledHardwareMidiOutNames() const;
    int getNumEnabledHardwareMidiOuts() const;
    juce::String getHardwareMidiSummaryText() const;

    //==========================================================================
    // UI Telemetry
    //==========================================================================
    int getCurrentPlayStep() const noexcept { return currentPlayStep.load(); }
    int getTrackPlayStep (int trackIdx) const noexcept
    {
        trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
        return trackPlaySteps[static_cast<size_t>(trackIdx)].load();
    }
    int getLastTriggeredNote() const noexcept { return lastTriggeredNote.load(); }
    bool isTransportRunning() const noexcept { return transportRunning.load(); }
    double getCurrentHostBpm() const noexcept { return currentHostBpm.load(); }
    double getEffectiveBpm() const noexcept { return effectiveBpm.load(); }
    juce::String getLastLoadedMidiName() const;

    int computeStepMidiNote (int stepIdx, const PulseForge::StepData& step,
                             int rootMidi, PulseForge::ScaleType scale) const;

    struct TrackPlaybackSnapshot
    {
        std::array<PulseForge::StepData, PulseForge::kMaxSteps> steps {};
        int numSteps = 16;
        int midiChannel = 1;
        bool muted = false;
        PulseForge::EngineMode mode = PulseForge::EngineMode::Sequencer;
        PulseForge::InstrumentRole role = PulseForge::InstrumentRole::MonoBass;
        PulseForge::ArpDirection arpDir = PulseForge::ArpDirection::Up;
        int arpOctaves = 2;
        int baseOctave = 1;
        int rateIdx = 3;
        float swing = 20.0f;
        int tempoRatioIdx = 5;
        float globalGate = 1.0f;
    };

    struct PlaybackSnapshot
    {
        std::array<TrackPlaybackSnapshot, PulseForge::kNumTracks> tracks {};
    };

    void updatePlaybackSnapshot() noexcept;
    const PlaybackSnapshot& getPlaybackSnapshot() const noexcept
    {
        return audioSnapshots[static_cast<size_t>(activeSnapshotIndex.load (std::memory_order_acquire))];
    }

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
        int trackIndex = 0;
    };

    struct PendingSubTrigger
    {
        double delaySamples = 0.0;
        double durationSamples = 0.0;
        int midiNote = 60;
        int velocity = 100;
        int channel = 1;
        int trackIndex = 0;
    };

    void sendMidiOut (const juce::MidiMessage& msg, juce::MidiBuffer& midiOut, int sampleOffset);
    void stopAllActiveNotes (juce::MidiBuffer& midiOut, int sampleOffset);
    void triggerStepEventsForTrack (int trackIdx, int stepIdx, double stepDurationSamples,
                                    int sampleOffset, juce::MidiBuffer& midiOut,
                                    const PlaybackSnapshot& snap);

    std::array<PulseForge::TrackData, PulseForge::kNumTracks> tracks;
    std::atomic<int> activeTrackIndex { 0 };

    PlaybackSnapshot audioSnapshots[2];
    std::atomic<int> activeSnapshotIndex { 0 };

    // Playback & Host Sync timing state
    double currentSampleRate = 44100.0;
    double internalPpqPosition = 0.0;
    double lastExpectedNextBlockPpq = -1.0;
    bool wasPlayingLastBlock = false;
    int lastTriggeredStepIndex[PulseForge::kNumTracks] = { -999999, -999999, -999999 };
    int arpWalkIndex[PulseForge::kNumTracks] = { 0, 0, 0 };
    int arpDirectionSign[PulseForge::kNumTracks] = { 1, 1, 1 };

    std::vector<ActiveNoteEvent> activeNotes;
    std::vector<PendingSubTrigger> pendingSubTriggers;
    std::vector<int> heldInputNotes;
    int liveTransposeSemitones = 0;

    std::atomic<int> currentPlayStep { 0 };
    std::array<std::atomic<int>, PulseForge::kNumTracks> trackPlaySteps { 0, 0, 0 };
    std::atomic<int> lastTriggeredNote { 36 };
    std::atomic<bool> transportRunning { false };
    std::atomic<double> currentHostBpm { 124.0 };
    std::atomic<double> effectiveBpm { 124.0 };
    juce::String lastLoadedMidiName;

    PulseForge::PreviewSynth previewSynth;

    // Multi-Device Direct Hardware MIDI Output
    mutable juce::CriticalSection midiOutLock;
    struct HardwareOutputPort
    {
        juce::String name;
        std::unique_ptr<juce::MidiOutput> device;
    };
    std::vector<HardwareOutputPort> activeHardwareOuts;
    juce::StringArray enabledHardwareMidiNames;

    std::mt19937 probRng { std::random_device{}() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PulseForgeAudioProcessor)
};
