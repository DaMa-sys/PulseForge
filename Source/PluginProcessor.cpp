#include "PluginProcessor.h"
#include "PluginEditor.h"

PulseForgeAudioProcessor::PulseForgeAudioProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    activeNotes.reserve (32);
    pendingSubTriggers.reserve (32);
    heldInputNotes.reserve (16);

    // Initialize all 4 pattern slots with musical phrases right out of the box
    const PulseForge::GenreStyle initGenres[4] = {
        PulseForge::GenreStyle::ClassicHouse,
        PulseForge::GenreStyle::MelodicTechno,
        PulseForge::GenreStyle::FunkBoogie,
        PulseForge::GenreStyle::AcidTechno
    };
    const PulseForge::InstrumentRole initRoles[4] = {
        PulseForge::InstrumentRole::MonoBass,
        PulseForge::InstrumentRole::ClassicArp,
        PulseForge::InstrumentRole::MonoBass,
        PulseForge::InstrumentRole::AcidLine
    };

    for (int slot = 0; slot < PulseForge::kNumPatternSlots; ++slot)
    {
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = initGenres[slot];
        gp.role  = initRoles[slot];
        gp.scale = (slot == 3) ? PulseForge::ScaleType::Phrygian : PulseForge::ScaleType::Dorian;
        gp.density = 0.65f;
        gp.syncopation = 0.55f;
        gp.spice = 0.45f;
        gp.pitchRange = 0.55f;
        gp.numSteps = 16;
        PulseForge::StyleGenerator::generatePattern (patternSlots[static_cast<size_t>(slot)], gp, 2026u + static_cast<uint32_t>(slot * 313));
    }

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::updatePlaybackSnapshot() noexcept
{
    const int nextIdx = 1 - activeSnapshotIndex.load (std::memory_order_relaxed);
    const auto& currentPat = getActivePattern();
    audioSnapshots[static_cast<size_t>(nextIdx)].steps = currentPat.steps;
    audioSnapshots[static_cast<size_t>(nextIdx)].numSteps = currentPat.numSteps;
    activeSnapshotIndex.store (nextIdx, std::memory_order_release);
}

PulseForgeAudioProcessor::~PulseForgeAudioProcessor()
{
    const juce::ScopedLock sl (midiOutLock);
    activeHardwareOuts.clear();
}

juce::AudioProcessorValueTreeState::ParameterLayout PulseForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Engine Mode",
        juce::StringArray { "Sequencer", "Arpeggiator" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "genre", "Genre Style",
        juce::StringArray {
            "Classic House (90s/Deep)",
            "Acid House & Techno (303)",
            "Italo Disco & Synthwave",
            "Berlin School (70s Modular)",
            "Melodic Techno & Prog",
            "Detroit Electro & Bass House",
            "Funk & Old-School Boogie",
            "Organic / Euclidean IDM"
        }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "role", "Instrument Type",
        juce::StringArray {
            "Mono Bass (SH-101 / Moog)",
            "Acid Line (TB-303 Glide)",
            "Classic Arpeggio (Juno/Jup)",
            "Lead & Hook (Prophet/MS20)",
            "Poly Chord Stabs (House/Dub)",
            "Modular Ratchet Seq"
        }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "scale", "Musical Scale",
        juce::StringArray {
            "Minor Natural",
            "Dorian (House/Funk)",
            "Phrygian (Acid/Techno)",
            "Phrygian Dominant",
            "Minor Pentatonic",
            "Major Natural",
            "Mixolydian (Boogie)",
            "Harmonic Minor",
            "Lydian (Berlin/Ambient)",
            "Chromatic"
        }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rootNote", "Root Key",
        juce::StringArray { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> ("baseOctave", "Base Octave", 0, 5, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rate", "Step Grid",
        juce::StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "arpDir", "Arp Direction",
        juce::StringArray { "Up", "Down", "Up-Down", "Converge", "Pinky Pedal", "Random Walk", "Chord Stabs" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> ("arpOctaves", "Arp Octaves", 1, 4, 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "density", "Density", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 65.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "syncopation", "Syncopation", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 55.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "spice", "Spice / Acid", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 45.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "pitchRange", "Pitch Range", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 55.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "swing", "Swing", juce::NormalisableRange<float> (0.0f, 75.0f, 1.0f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "globalGate", "Global Gate %", juce::NormalisableRange<float> (20.0f, 150.0f, 1.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterInt> ("numSteps", "Pattern Steps", 1, 16, 16));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("midiChannel", "MIDI Channel", 1, 16, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("ccNumber", "MIDI CC #", 1, 119, 74));

    // Tempo Ratio: Fractions & Multiples of the Ableton Project BPM!
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "tempoRatio", "Tempo BPM Ratio",
        juce::StringArray {
            "1/4x", "1/3x", "1/2x", "2/3x", "3/4x",
            "1/1x",
            "4/3x", "3/2x", "2/1x", "3/1x", "4/1x"
        }, 5)); // Default 5 = "1/1x" (Exact Project BPM)

    // In DAW (VST3), defaults to false so PulseForge starts/stops strictly with Ableton's Play/Stop!
    params.push_back (std::make_unique<juce::AudioParameterBool> ("autoPlay", "Free Run (Audition)", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("previewSynth", "Internal Analog Preview", true));

    return { params.begin(), params.end() };
}

const juce::String PulseForgeAudioProcessor::getName() const { return JucePlugin_Name; }
bool PulseForgeAudioProcessor::acceptsMidi() const { return true; }
bool PulseForgeAudioProcessor::producesMidi() const { return true; }
bool PulseForgeAudioProcessor::isMidiEffect() const { return true; }
double PulseForgeAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PulseForgeAudioProcessor::getNumPrograms() { return 1; }
int PulseForgeAudioProcessor::getCurrentProgram() { return 0; }
void PulseForgeAudioProcessor::setCurrentProgram (int) {}
const juce::String PulseForgeAudioProcessor::getProgramName (int) { return {}; }
void PulseForgeAudioProcessor::changeProgramName (int, const juce::String&) {}

void PulseForgeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    internalPpqPosition = 0.0;
    lastExpectedNextBlockPpq = -1.0;
    wasPlayingLastBlock = false;
    lastTriggeredStepIndex = -999999;
    activeNotes.clear();
    pendingSubTriggers.clear();
    previewSynth.prepare (currentSampleRate);
    updatePlaybackSnapshot();

    // If running as Standalone .exe, enable Free Run automatically so it plays immediately
    if (wrapperType == wrapperType_Standalone)
    {
        if (auto* pRun = apvts.getParameter ("autoPlay"))
            pRun->setValueNotifyingHost (1.0f);
    }
}

void PulseForgeAudioProcessor::releaseResources()
{
    previewSynth.reset();
}

bool PulseForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
}

void PulseForgeAudioProcessor::setActiveSlotIndex (int idx)
{
    const juce::ScopedLock sl (patternLock);
    activeSlotIndex.store (juce::jlimit (0, PulseForge::kNumPatternSlots - 1, idx));
    updatePlaybackSnapshot();
    apvts.getParameter ("numSteps")->setValueNotifyingHost (
        apvts.getParameterRange ("numSteps").convertTo0to1 (static_cast<float> (getActivePattern().numSteps)));
}

void PulseForgeAudioProcessor::copyPatternSlot (int srcIdx, int dstIdx)
{
    const juce::ScopedLock sl (patternLock);
    srcIdx = juce::jlimit (0, PulseForge::kNumPatternSlots - 1, srcIdx);
    dstIdx = juce::jlimit (0, PulseForge::kNumPatternSlots - 1, dstIdx);
    if (srcIdx != dstIdx)
    {
        patternSlots[static_cast<size_t>(dstIdx)] = patternSlots[static_cast<size_t>(srcIdx)];
        updatePlaybackSnapshot();
    }
}

void PulseForgeAudioProcessor::triggerGeneratePattern (bool applyRecommendedScaleAndSwing)
{
    const juce::ScopedLock sl (patternLock);

    PulseForge::StyleGenerator::GenerationParams gp;
    gp.mode        = static_cast<PulseForge::EngineMode> (static_cast<int> (apvts.getRawParameterValue ("mode")->load()));
    gp.genre       = static_cast<PulseForge::GenreStyle> (static_cast<int> (apvts.getRawParameterValue ("genre")->load()));
    gp.role        = static_cast<PulseForge::InstrumentRole> (static_cast<int> (apvts.getRawParameterValue ("role")->load()));
    gp.density     = apvts.getRawParameterValue ("density")->load() * 0.01f;
    gp.syncopation = apvts.getRawParameterValue ("syncopation")->load() * 0.01f;
    gp.spice       = apvts.getRawParameterValue ("spice")->load() * 0.01f;
    gp.pitchRange  = apvts.getRawParameterValue ("pitchRange")->load() * 0.01f;
    gp.numSteps    = static_cast<int> (apvts.getRawParameterValue ("numSteps")->load());

    if (applyRecommendedScaleAndSwing)
    {
        PulseForge::ScaleType recScale = PulseForge::ScaleType::Dorian;
        int recOct = 1;
        float recSwing = 0.20f;
        PulseForge::StyleGenerator::getRecommendedSetup (gp.genre, gp.role, recScale, recOct, recSwing);

        gp.scale = recScale;
        if (auto* pScale = apvts.getParameter ("scale"))
            pScale->setValueNotifyingHost (pScale->convertTo0to1 (static_cast<float> (recScale)));
        if (auto* pOct = apvts.getParameter ("baseOctave"))
            pOct->setValueNotifyingHost (pOct->convertTo0to1 (static_cast<float> (recOct)));
        if (auto* pSwing = apvts.getParameter ("swing"))
            pSwing->setValueNotifyingHost (pSwing->convertTo0to1 (recSwing * 100.0f));
    }
    else
    {
        gp.scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    }

    PulseForge::StyleGenerator::generatePattern (getActivePattern(), gp);
    lastLoadedMidiName.clear();
    updatePlaybackSnapshot();
}

bool PulseForgeAudioProcessor::triggerLoadFromUserSamplesLibrary()
{
    const juce::ScopedLock sl (patternLock);
    const auto genre = static_cast<PulseForge::GenreStyle> (static_cast<int> (apvts.getRawParameterValue ("genre")->load()));
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));

    juce::String loadedName = PulseForge::StyleGenerator::loadRandomFromUserSamplesLibrary (getActivePattern(), genre, scale);
    if (loadedName.isNotEmpty())
    {
        lastLoadedMidiName = loadedName;
        updatePlaybackSnapshot();
        return true;
    }
    return false;
}

bool PulseForgeAudioProcessor::triggerLoadSpecificMidiFile (const juce::File& file)
{
    const juce::ScopedLock sl (patternLock);
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    if (PulseForge::StyleGenerator::loadPatternFromMidiFile (file, getActivePattern(), scale))
    {
        lastLoadedMidiName = file.getFileNameWithoutExtension();
        updatePlaybackSnapshot();
        return true;
    }
    return false;
}

juce::String PulseForgeAudioProcessor::getLastLoadedMidiName() const
{
    const juce::ScopedLock sl (patternLock);
    return lastLoadedMidiName;
}

void PulseForgeAudioProcessor::triggerMutatePattern()
{
    const juce::ScopedLock sl (patternLock);
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    const float spice = apvts.getRawParameterValue ("spice")->load() * 0.01f;
    PulseForge::StyleGenerator::mutatePattern (getActivePattern(), scale, spice);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerRotatePattern (int delta)
{
    const juce::ScopedLock sl (patternLock);
    getActivePattern().numSteps = static_cast<int> (apvts.getRawParameterValue ("numSteps")->load());
    PulseForge::StyleGenerator::rotatePattern (getActivePattern(), delta);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerInvertPattern()
{
    const juce::ScopedLock sl (patternLock);
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    PulseForge::StyleGenerator::invertPattern (getActivePattern(), scale);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerRequantizeToScale()
{
    const juce::ScopedLock sl (patternLock);
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    PulseForge::StyleGenerator::requantizeToScale (getActivePattern(), scale);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerClearPattern()
{
    const juce::ScopedLock sl (patternLock);
    auto& pat = getActivePattern();
    for (auto& s : pat.steps)
    {
        s.active = false;
        s.semitoneOffset = 0;
        s.octaveShift = 0;
        s.slide = false;
        s.accent = false;
        s.ratchet = 1;
        s.gate = 0.75f;
    }
    pat.steps[0].active = true;
    lastLoadedMidiName.clear();
    updatePlaybackSnapshot();
}

int PulseForgeAudioProcessor::computeStepMidiNote (int stepIdx, const PulseForge::StepData& step,
                                                   int rootMidi, PulseForge::ScaleType scale) const
{
    juce::ignoreUnused (stepIdx);
    const int quantSemi = PulseForge::quantizeSemitoneToScale (step.semitoneOffset, scale);
    const int note = rootMidi + quantSemi + (step.octaveShift * 12) + liveTransposeSemitones;
    return juce::jlimit (12, 115, note);
}

void PulseForgeAudioProcessor::sendMidiOut (const juce::MidiMessage& msg, juce::MidiBuffer& midiOut, int sampleOffset)
{
    midiOut.addEvent (msg, juce::jmax (0, sampleOffset));

    const juce::ScopedLock sl (midiOutLock);
    for (auto& port : activeHardwareOuts)
    {
        if (port.device != nullptr)
            port.device->sendMessageNow (msg);
    }
}

void PulseForgeAudioProcessor::stopAllActiveNotes (juce::MidiBuffer& midiOut, int sampleOffset)
{
    for (const auto& an : activeNotes)
    {
        if (an.midiNote >= 0)
        {
            sendMidiOut (juce::MidiMessage::noteOff (an.channel, an.midiNote), midiOut, sampleOffset);
            previewSynth.noteOff (an.midiNote);
        }
    }
    activeNotes.clear();
    pendingSubTriggers.clear();
    previewSynth.allNotesOff();
}

void PulseForgeAudioProcessor::triggerStepEvents (int stepIdx, double stepDurationSamples,
                                                  int sampleOffset, juce::MidiBuffer& midiOut,
                                                  const PlaybackSnapshot& snap)
{
    const auto& step = snap.steps[static_cast<size_t>(stepIdx % PulseForge::kMaxSteps)];

    // IMPORTANT FOR MUSICAL SUSTAIN:
    // If this step is a rest (!step.active), do NOT kill notes that are still sustaining
    // from an earlier step (e.g., a 2-step or 3-step bass note in X..X..X.)!
    if (!step.active)
        return;

    const int midiChan    = static_cast<int> (apvts.getRawParameterValue ("midiChannel")->load());
    const int ccNumber    = static_cast<int> (apvts.getRawParameterValue ("ccNumber")->load());
    const int rootKey     = static_cast<int> (apvts.getRawParameterValue ("rootNote")->load());
    const int baseOctave  = static_cast<int> (apvts.getRawParameterValue ("baseOctave")->load());
    const auto scale      = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    const auto mode       = static_cast<PulseForge::EngineMode> (static_cast<int> (apvts.getRawParameterValue ("mode")->load()));
    const auto role       = static_cast<PulseForge::InstrumentRole> (static_cast<int> (apvts.getRawParameterValue ("role")->load()));
    const auto arpDir     = static_cast<PulseForge::ArpDirection> (static_cast<int> (apvts.getRawParameterValue ("arpDir")->load()));
    const int arpOctaves  = static_cast<int> (apvts.getRawParameterValue ("arpOctaves")->load());
    const float globalGateMult = apvts.getRawParameterValue ("globalGate")->load() * 0.01f;

    // Check probability (100% always fires)
    if (step.probability < 100)
    {
        int roll = std::uniform_int_distribution<int> (1, 100) (probRng);
        if (roll > step.probability)
            return;
    }

    // Send per-step MIDI CC modulation (e.g. Filter Cutoff CC74)
    const int effectiveCC = juce::jlimit (0, 127, step.accent ? juce::jmax (step.ccValue, 112) : step.ccValue);
    sendMidiOut (juce::MidiMessage::controllerEvent (midiChan, ccNumber, effectiveCC), midiOut, sampleOffset);
    previewSynth.setCutModCC (effectiveCC);

    const int rootMidi = (baseOctave + 1) * 12 + rootKey; // baseOctave 1 -> C1 (36)
    std::vector<int> notesToTrigger;
    notesToTrigger.reserve (8);

    if (mode == PulseForge::EngineMode::Arpeggiator)
    {
        std::vector<int> basePool;
        basePool.reserve (8);
        if (!heldInputNotes.empty())
        {
            basePool = heldInputNotes;
            std::sort (basePool.begin(), basePool.end());
        }
        else
        {
            const auto& intervals = PulseForge::getScaleIntervals (scale);
            basePool.push_back (rootMidi + intervals[0]);
            if (intervals.size() > 2) basePool.push_back (rootMidi + intervals[2]);
            if (intervals.size() > 4) basePool.push_back (rootMidi + intervals[4]);
            if (intervals.size() > 6) basePool.push_back (rootMidi + intervals[6]);
        }

        std::vector<int> expandedPool;
        expandedPool.reserve (16);
        for (int oct = 0; oct < arpOctaves; ++oct)
        {
            for (int n : basePool)
                expandedPool.push_back (juce::jlimit (12, 115, n + oct * 12 + step.octaveShift * 12));
        }

        if (expandedPool.empty())
            expandedPool.push_back (rootMidi);

        const int poolSize = static_cast<int> (expandedPool.size());

        if (arpDir == PulseForge::ArpDirection::ChordTrigger)
        {
            for (int n : basePool)
                notesToTrigger.push_back (juce::jlimit (12, 115, n + step.octaveShift * 12 + PulseForge::quantizeSemitoneToScale (step.semitoneOffset, scale)));
        }
        else
        {
            int chosenIdx = 0;
            switch (arpDir)
            {
                case PulseForge::ArpDirection::Up:
                    chosenIdx = arpWalkIndex % poolSize;
                    arpWalkIndex = (arpWalkIndex + 1) % poolSize;
                    break;
                case PulseForge::ArpDirection::Down:
                    chosenIdx = (poolSize - 1) - (arpWalkIndex % poolSize);
                    arpWalkIndex = (arpWalkIndex + 1) % poolSize;
                    break;
                case PulseForge::ArpDirection::UpDown:
                    if (poolSize <= 1) chosenIdx = 0;
                    else
                    {
                        chosenIdx = juce::jlimit (0, poolSize - 1, arpWalkIndex);
                        arpWalkIndex += arpDirectionSign;
                        if (arpWalkIndex >= poolSize - 1) { arpWalkIndex = poolSize - 1; arpDirectionSign = -1; }
                        else if (arpWalkIndex <= 0)       { arpWalkIndex = 0; arpDirectionSign = 1; }
                    }
                    break;
                case PulseForge::ArpDirection::Converge:
                {
                    int half = arpWalkIndex / 2;
                    chosenIdx = (arpWalkIndex % 2 == 0) ? (half % poolSize) : ((poolSize - 1 - (half % poolSize)) % poolSize);
                    arpWalkIndex = (arpWalkIndex + 1) % (poolSize * 2);
                    break;
                }
                case PulseForge::ArpDirection::PinkyPedal:
                    chosenIdx = (stepIdx % 2 == 0) ? ((arpWalkIndex++) % poolSize) : (poolSize - 1);
                    break;
                case PulseForge::ArpDirection::RandomWalk:
                default:
                    chosenIdx = std::uniform_int_distribution<int> (0, poolSize - 1) (probRng);
                    break;
            }
            int arpNote = expandedPool[static_cast<size_t>(chosenIdx)];
            if (step.semitoneOffset != 0)
                arpNote = PulseForge::quantizeSemitoneToScale (arpNote - rootMidi + step.semitoneOffset, scale) + rootMidi;
            notesToTrigger.push_back (juce::jlimit (12, 115, arpNote));
        }
    }
    else
    {
        const int baseNote = computeStepMidiNote (stepIdx, step, rootMidi, scale);
        notesToTrigger.push_back (baseNote);

        if (role == PulseForge::InstrumentRole::PolyChordStab || step.chordType > 0)
        {
            const int cType = (role == PulseForge::InstrumentRole::PolyChordStab && step.chordType == 0) ? 3 : step.chordType;
            if (cType == 1)
            {
                notesToTrigger.push_back (juce::jlimit (12, 115, baseNote + 7));
            }
            else if (cType >= 2)
            {
                const int third = rootMidi + PulseForge::quantizeSemitoneToScale (step.semitoneOffset + 3, scale) + step.octaveShift * 12 + liveTransposeSemitones;
                const int fifth = rootMidi + PulseForge::quantizeSemitoneToScale (step.semitoneOffset + 7, scale) + step.octaveShift * 12 + liveTransposeSemitones;
                notesToTrigger.push_back (juce::jlimit (12, 115, third));
                notesToTrigger.push_back (juce::jlimit (12, 115, fifth));
                if (cType >= 3)
                {
                    const int seventh = rootMidi + PulseForge::quantizeSemitoneToScale (step.semitoneOffset + 10, scale) + step.octaveShift * 12 + liveTransposeSemitones;
                    notesToTrigger.push_back (juce::jlimit (12, 115, seventh));
                }
                if (cType >= 4)
                {
                    const int ninth = rootMidi + PulseForge::quantizeSemitoneToScale (step.semitoneOffset + 14, scale) + step.octaveShift * 12 + liveTransposeSemitones;
                    notesToTrigger.push_back (juce::jlimit (12, 115, ninth));
                }
            }
        }
    }

    const int vel = juce::jlimit (1, 127, step.accent ? juce::jmax (step.velocity, 120) : step.velocity);
    const int ratchetCount = juce::jlimit (1, 4, step.ratchet);

    // Separate previous legato-slide notes from normal notes
    std::vector<ActiveNoteEvent> previousLegatoNotes;
    previousLegatoNotes.reserve (8);
    for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[static_cast<size_t>(i)].waitingForLegatoOverlap)
        {
            previousLegatoNotes.push_back (activeNotes[static_cast<size_t>(i)]);
            activeNotes.erase (activeNotes.begin() + i);
        }
        else
        {
            // Turn off any still-sounding non-slide note right before the new active step triggers
            sendMidiOut (juce::MidiMessage::noteOff (activeNotes[static_cast<size_t>(i)].channel,
                                                     activeNotes[static_cast<size_t>(i)].midiNote),
                         midiOut, sampleOffset);
            previewSynth.noteOff (activeNotes[static_cast<size_t>(i)].midiNote);
            activeNotes.erase (activeNotes.begin() + i);
        }
    }

    const bool isLegatoArrival = !previousLegatoNotes.empty();

    if (ratchetCount > 1 && !step.slide)
    {
        const double subStepLen = stepDurationSamples / static_cast<double> (ratchetCount);
        const double subGateLen = juce::jmax (32.0, subStepLen * 0.58 * globalGateMult);

        for (int r = 0; r < ratchetCount; ++r)
        {
            const int subVel = juce::jlimit (1, 127, vel - (r * 6));
            if (r == 0)
            {
                for (int n : notesToTrigger)
                {
                    sendMidiOut (juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (subVel)), midiOut, sampleOffset);
                    previewSynth.noteOn (n, subVel, isLegatoArrival);
                    activeNotes.push_back ({ n, midiChan, subGateLen, false });
                    lastTriggeredNote.store (n);
                }
            }
            else
            {
                for (int n : notesToTrigger)
                {
                    pendingSubTriggers.push_back ({
                        subStepLen * static_cast<double> (r),
                        subGateLen,
                        n,
                        subVel,
                        midiChan
                    });
                }
            }
        }
    }
    else
    {
        // step.gate can be 0.10 .. 4.00 steps (allows rich multi-step bass/lead sustain across rest steps!)
        const double effectiveGateSteps = step.slide ? 4.0 : juce::jlimit (0.10, 3.95, static_cast<double> (step.gate * globalGateMult));
        const double noteLenSamples = juce::jmax (64.0, stepDurationSamples * effectiveGateSteps);

        for (int n : notesToTrigger)
        {
            sendMidiOut (juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (vel)), midiOut, sampleOffset);
            previewSynth.noteOn (n, vel, isLegatoArrival);
            activeNotes.push_back ({ n, midiChan, noteLenSamples, step.slide });
            lastTriggeredNote.store (n);
        }
    }

    // Release previous legato notes AFTER the new Note-On was sent (true analog mono portamento!)
    for (const auto& prev : previousLegatoNotes)
    {
        bool sameNoteRetriggered = false;
        for (int n : notesToTrigger)
            if (n == prev.midiNote) sameNoteRetriggered = true;

        if (!sameNoteRetriggered)
        {
            sendMidiOut (juce::MidiMessage::noteOff (prev.channel, prev.midiNote), midiOut, sampleOffset + 1);
            previewSynth.noteOff (prev.midiNote);
        }
    }
}

void PulseForgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const double hostSr = getSampleRate();
    if (hostSr > 1000.0 && std::abs (hostSr - currentSampleRate) > 1.0)
    {
        currentSampleRate = hostSr;
        previewSynth.prepare (currentSampleRate);
    }

    previewSynth.setEnabled (apvts.getRawParameterValue ("previewSynth")->load() > 0.5f);

    // 1. Parse incoming MIDI (for live transposition in Sequencer mode, or chord holding in Arpeggiator mode)
    const int rootKey = static_cast<int> (apvts.getRawParameterValue ("rootNote")->load());
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            if (std::find (heldInputNotes.begin(), heldInputNotes.end(), note) == heldInputNotes.end())
                heldInputNotes.push_back (note);

            liveTransposeSemitones = (note % 12) - rootKey;
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            heldInputNotes.erase (std::remove (heldInputNotes.begin(), heldInputNotes.end(), note), heldInputNotes.end());
            if (heldInputNotes.empty())
                liveTransposeSemitones = 0;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            heldInputNotes.clear();
            liveTransposeSemitones = 0;
        }
    }

    midiMessages.clear();

    //==========================================================================
    // 2. ABLETON LIVE SAMPLE-ACCURATE SYNC & FRACTIONAL TEMPO RATIO
    //==========================================================================
    double hostBpm = currentHostBpm.load();
    bool hostIsPlaying = false;
    bool hasHostPpq = false;
    double blockStartPpq = internalPpqPosition;

    if (auto* currentPlayHead = getPlayHead())
    {
        if (auto posInfo = currentPlayHead->getPosition())
        {
            if (auto bpmOpt = posInfo->getBpm())
            {
                if (*bpmOpt > 20.0)
                    hostBpm = *bpmOpt;
            }

            hostIsPlaying = posInfo->getIsPlaying();
            if (hostIsPlaying)
            {
                if (auto ppqOpt = posInfo->getPpqPosition())
                {
                    blockStartPpq = *ppqOpt;
                    hasHostPpq = true;
                }
            }
        }
    }

    currentHostBpm.store (hostBpm);

    // Tempo Ratio Multiplier (1/4x .. 1/1x .. 4/1x of the Ableton Project BPM!)
    const int tempoRatioIdx = static_cast<int> (apvts.getRawParameterValue ("tempoRatio")->load());
    const double tempoMult  = PulseForge::getTempoRatioMultiplier (tempoRatioIdx);
    const double effBpm     = hostBpm * tempoMult;
    effectiveBpm.store (effBpm);

    const bool freeRunAudition = apvts.getRawParameterValue ("autoPlay")->load() > 0.5f;
    const bool shouldPlay = hostIsPlaying || freeRunAudition;

    transportRunning.store (shouldPlay);

    if (!shouldPlay)
    {
        if (wasPlayingLastBlock || !activeNotes.empty() || !pendingSubTriggers.empty())
            stopAllActiveNotes (midiMessages, 0);

        wasPlayingLastBlock = false;
        lastTriggeredStepIndex = -999999;
        lastExpectedNextBlockPpq = -1.0;
        internalPpqPosition = 0.0;
        return;
    }

    // Step Grid in Quarter Notes (PPQ), scaled by the Tempo Ratio Multiplier!
    const int rateIdx = static_cast<int> (apvts.getRawParameterValue ("rate")->load());
    double baseStepPpq = 0.25; // 1/16th note = 0.25 quarter notes
    switch (rateIdx)
    {
        case 0: baseStepPpq = 1.0;        break; // 1/4
        case 1: baseStepPpq = 0.5;        break; // 1/8
        case 2: baseStepPpq = 1.0 / 3.0;  break; // 1/8T
        case 3: baseStepPpq = 0.25;       break; // 1/16
        case 4: baseStepPpq = 1.0 / 6.0;  break; // 1/16T
        case 5: baseStepPpq = 0.125;      break; // 1/32
        default: baseStepPpq = 0.25;      break;
    }

    // Divide baseStepPpq by tempoMult so 2/1x runs twice as fast as project BPM and 1/2x runs half as fast!
    const double stepLengthPpq = baseStepPpq / tempoMult;
    const double ppqPerSample  = (hostBpm / 60.0) / currentSampleRate;
    const double stepDurationSamples = stepLengthPpq / ppqPerSample;

    // Detect Transport Start OR Ableton Loop Jump / Seek!
    const bool transportJustStarted = !wasPlayingLastBlock;
    const bool hostSeekedOrLooped   = hasHostPpq && (lastExpectedNextBlockPpq >= 0.0)
                                   && (std::abs (blockStartPpq - lastExpectedNextBlockPpq) > 0.015);

    if (transportJustStarted || hostSeekedOrLooped)
    {
        stopAllActiveNotes (midiMessages, 0);
        lastTriggeredStepIndex = -999999;
        arpWalkIndex = 0;
        arpDirectionSign = 1;
        if (!hasHostPpq && transportJustStarted)
            blockStartPpq = 0.0;
    }

    wasPlayingLastBlock = true;

    const int numSteps = juce::jlimit (1, PulseForge::kMaxSteps,
                                       static_cast<int> (apvts.getRawParameterValue ("numSteps")->load()));
    const double swingAmount = static_cast<double> (apvts.getRawParameterValue ("swing")->load()) * 0.01 * 0.42;

    const auto& playbackSnap = audioSnapshots[static_cast<size_t>(activeSnapshotIndex.load (std::memory_order_acquire))];

    // 3. Sample-by-sample timing loop locked to Host PPQ
    for (int s = 0; s < numSamples; ++s)
    {
        // A) Process active note-offs
        for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
        {
            auto& an = activeNotes[static_cast<size_t>(i)];
            an.remainingSamples -= 1.0;
            if (an.remainingSamples <= 0.0 && !an.waitingForLegatoOverlap)
            {
                sendMidiOut (juce::MidiMessage::noteOff (an.channel, an.midiNote), midiMessages, s);
                previewSynth.noteOff (an.midiNote);
                activeNotes.erase (activeNotes.begin() + i);
            }
        }

        // B) Process pending ratchet sub-triggers
        for (int i = static_cast<int> (pendingSubTriggers.size()) - 1; i >= 0; --i)
        {
            auto& sub = pendingSubTriggers[static_cast<size_t>(i)];
            sub.delaySamples -= 1.0;
            if (sub.delaySamples <= 0.0)
            {
                sendMidiOut (juce::MidiMessage::noteOn (sub.channel, sub.midiNote, static_cast<juce::uint8> (sub.velocity)),
                             midiMessages, s);
                previewSynth.noteOn (sub.midiNote, sub.velocity, false);
                activeNotes.push_back ({ sub.midiNote, sub.channel, sub.durationSamples, false });
                pendingSubTriggers.erase (pendingSubTriggers.begin() + i);
            }
        }

        // C) Compute exact step index from Ableton PPQ position (with Swing on odd steps)
        const double currentPpq = blockStartPpq + static_cast<double> (s) * ppqPerSample;
        if (currentPpq >= 0.0)
        {
            const double rawStepFloat = currentPpq / stepLengthPpq;
            const int pairIndex = static_cast<int> (std::floor (rawStepFloat / 2.0));
            const double posInPair = rawStepFloat - static_cast<double> (pairIndex * 2);

            const int unmodStepIndex = (posInPair < (1.0 + swingAmount))
                                     ? (pairIndex * 2)
                                     : (pairIndex * 2 + 1);

            if (unmodStepIndex != lastTriggeredStepIndex)
            {
                lastTriggeredStepIndex = unmodStepIndex;
                const int effectiveStepCount = juce::jmax (1, numSteps);
                const int wrappedStep = ((unmodStepIndex % effectiveStepCount) + effectiveStepCount) % effectiveStepCount;
                currentPlayStep.store (wrappedStep);
                triggerStepEvents (wrappedStep, stepDurationSamples, s, midiMessages, playbackSnap);
            }
        }
    }

    const double blockEndPpq = blockStartPpq + static_cast<double> (numSamples) * ppqPerSample;
    lastExpectedNextBlockPpq = blockEndPpq;
    internalPpqPosition      = blockEndPpq;

    // 4. Render internal virtual analog preview synth if enabled
    previewSynth.render (buffer, 0, numSamples);
}

juce::File PulseForgeAudioProcessor::exportActivePatternToTempMidiFile()
{
    const juce::ScopedLock sl (patternLock);
    const auto& pat = getActivePattern();
    const int numSteps   = juce::jlimit (1, PulseForge::kMaxSteps, static_cast<int> (apvts.getRawParameterValue ("numSteps")->load()));
    const int midiChan   = static_cast<int> (apvts.getRawParameterValue ("midiChannel")->load());
    const int rootKey    = static_cast<int> (apvts.getRawParameterValue ("rootNote")->load());
    const int baseOctave = static_cast<int> (apvts.getRawParameterValue ("baseOctave")->load());
    const auto scale     = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    const auto role      = static_cast<PulseForge::InstrumentRole> (static_cast<int> (apvts.getRawParameterValue ("role")->load()));
    const float swingPct = apvts.getRawParameterValue ("swing")->load() * 0.01f * 0.42f;
    const float globalGateMult = apvts.getRawParameterValue ("globalGate")->load() * 0.01f;
    const int tempoRatioIdx = static_cast<int> (apvts.getRawParameterValue ("tempoRatio")->load());
    const double tempoMult  = PulseForge::getTempoRatioMultiplier (tempoRatioIdx);

    const int rootMidi = (baseOctave + 1) * 12 + rootKey;
    const int ticksPerQuarter = 960;
    const double stepTicks = (ticksPerQuarter * 0.25) / tempoMult;

    juce::MidiMessageSequence seq;

    for (int i = 0; i < numSteps; ++i)
    {
        const auto& step = pat.steps[static_cast<size_t>(i)];
        if (!step.active)
            continue;

        const double swingOffsetTicks = (i % 2 == 1) ? (stepTicks * swingPct) : 0.0;
        const double startTick = i * stepTicks + swingOffsetTicks;
        const int vel = juce::jlimit (1, 127, step.accent ? juce::jmax (step.velocity, 120) : step.velocity);
        const int baseNote = computeStepMidiNote (i, step, rootMidi, scale);

        std::vector<int> stepNotes { baseNote };
        if (role == PulseForge::InstrumentRole::PolyChordStab || step.chordType > 0)
        {
            stepNotes.push_back (juce::jlimit (12, 115, baseNote + 3));
            stepNotes.push_back (juce::jlimit (12, 115, baseNote + 7));
            stepNotes.push_back (juce::jlimit (12, 115, baseNote + 10));
        }

        const int ratchets = juce::jlimit (1, 4, step.ratchet);
        if (ratchets > 1 && !step.slide)
        {
            const double subLen = stepTicks / ratchets;
            for (int r = 0; r < ratchets; ++r)
            {
                const double tOn = startTick + r * subLen;
                const double tOff = tOn + subLen * 0.65;
                for (int n : stepNotes)
                {
                    seq.addEvent (juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (vel)), tOn);
                    seq.addEvent (juce::MidiMessage::noteOff (midiChan, n), tOff);
                }
            }
        }
        else
        {
            const double gateSteps = step.slide ? 1.08 : juce::jlimit (0.12, 3.90, static_cast<double> (step.gate * globalGateMult));
            const double tOff = startTick + stepTicks * gateSteps;
            for (int n : stepNotes)
            {
                seq.addEvent (juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (vel)), startTick);
                seq.addEvent (juce::MidiMessage::noteOff (midiChan, n), tOff);
            }
        }
    }

    seq.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (ticksPerQuarter);
    midiFile.addTrack (seq);

    juce::File tempFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("PulseForge_Pattern_" + PulseForge::getMidiNoteName (rootMidi) + ".mid");
    tempFile.deleteFile();
    if (auto stream = std::unique_ptr<juce::FileOutputStream> (tempFile.createOutputStream()))
        midiFile.writeTo (*stream);

    return tempFile;
}

juce::StringArray PulseForgeAudioProcessor::getAvailableHardwareMidiOuts() const
{
    juce::StringArray list;
    for (const auto& dev : juce::MidiOutput::getAvailableDevices())
        list.add (dev.name);
    return list;
}

bool PulseForgeAudioProcessor::isHardwareMidiOutEnabled (const juce::String& deviceName) const
{
    const juce::ScopedLock sl (midiOutLock);
    return enabledHardwareMidiNames.contains (deviceName);
}

void PulseForgeAudioProcessor::setHardwareMidiOutEnabled (const juce::String& deviceName, bool shouldEnable)
{
    const juce::ScopedLock sl (midiOutLock);
    if (shouldEnable)
    {
        if (!enabledHardwareMidiNames.contains (deviceName))
        {
            for (const auto& dev : juce::MidiOutput::getAvailableDevices())
            {
                if (dev.name == deviceName)
                {
                    auto out = juce::MidiOutput::openDevice (dev.identifier);
                    if (out != nullptr)
                    {
                        enabledHardwareMidiNames.add (deviceName);
                        activeHardwareOuts.push_back ({ deviceName, std::move (out) });
                    }
                    break;
                }
            }
        }
    }
    else
    {
        enabledHardwareMidiNames.removeString (deviceName);
        activeHardwareOuts.erase (
            std::remove_if (activeHardwareOuts.begin(), activeHardwareOuts.end(),
                            [&] (const HardwareOutputPort& p) { return p.name == deviceName; }),
            activeHardwareOuts.end());
    }
}

void PulseForgeAudioProcessor::toggleHardwareMidiOut (const juce::String& deviceName)
{
    setHardwareMidiOutEnabled (deviceName, !isHardwareMidiOutEnabled (deviceName));
}

void PulseForgeAudioProcessor::setAllHardwareMidiOuts (bool enableAll)
{
    const auto available = getAvailableHardwareMidiOuts();
    for (const auto& dev : available)
        setHardwareMidiOutEnabled (dev, enableAll);
}

juce::StringArray PulseForgeAudioProcessor::getEnabledHardwareMidiOutNames() const
{
    const juce::ScopedLock sl (midiOutLock);
    return enabledHardwareMidiNames;
}

int PulseForgeAudioProcessor::getNumEnabledHardwareMidiOuts() const
{
    const juce::ScopedLock sl (midiOutLock);
    return enabledHardwareMidiNames.size();
}

juce::String PulseForgeAudioProcessor::getHardwareMidiSummaryText() const
{
    const juce::ScopedLock sl (midiOutLock);
    const int count = enabledHardwareMidiNames.size();
    if (count == 0)
        return "DAW ONLY";
    if (count == 1)
        return "1 USB: " + enabledHardwareMidiNames[0];
    return juce::String (count) + " USB SYNTHS";
}

bool PulseForgeAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PulseForgeAudioProcessor::createEditor()
{
    return new PulseForgeAudioProcessorEditor (*this);
}

void PulseForgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::ScopedLock sl (patternLock);
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    xml->setAttribute ("activeSlot", activeSlotIndex.load());
    xml->setAttribute ("hwMidiOuts", enabledHardwareMidiNames.joinIntoString (";;"));

    for (int slot = 0; slot < PulseForge::kNumPatternSlots; ++slot)
    {
        auto* slotXml = xml->createNewChildElement ("PatternSlot");
        slotXml->setAttribute ("index", slot);
        slotXml->setAttribute ("numSteps", patternSlots[static_cast<size_t>(slot)].numSteps);

        for (int i = 0; i < PulseForge::kMaxSteps; ++i)
        {
            const auto& st = patternSlots[static_cast<size_t>(slot)].steps[static_cast<size_t>(i)];
            auto* stepXml = slotXml->createNewChildElement ("Step");
            stepXml->setAttribute ("idx", i);
            stepXml->setAttribute ("active", st.active);
            stepXml->setAttribute ("semi", st.semitoneOffset);
            stepXml->setAttribute ("oct", st.octaveShift);
            stepXml->setAttribute ("vel", st.velocity);
            stepXml->setAttribute ("gate", static_cast<double> (st.gate));
            stepXml->setAttribute ("accent", st.accent);
            stepXml->setAttribute ("slide", st.slide);
            stepXml->setAttribute ("ratchet", st.ratchet);
            stepXml->setAttribute ("prob", st.probability);
            stepXml->setAttribute ("cc", st.ccValue);
            stepXml->setAttribute ("chord", st.chordType);
        }
    }

    copyXmlToBinary (*xml, destData);
}

void PulseForgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

        const juce::ScopedLock sl (patternLock);
        activeSlotIndex.store (juce::jlimit (0, PulseForge::kNumPatternSlots - 1, xmlState->getIntAttribute ("activeSlot", 0)));

        const juce::String hwMidiStr = xmlState->getStringAttribute ("hwMidiOuts", xmlState->getStringAttribute ("hwMidiOut", ""));
        if (hwMidiStr.isNotEmpty() && hwMidiStr != "None (DAW MIDI Out)")
        {
            auto tokens = juce::StringArray::fromTokens (hwMidiStr, ";;", "");
            for (const auto& dev : tokens)
                if (dev.isNotEmpty()) setHardwareMidiOutEnabled (dev, true);
        }

        for (auto* slotXml : xmlState->getChildWithTagNameIterator ("PatternSlot"))
        {
            const int slot = juce::jlimit (0, PulseForge::kNumPatternSlots - 1, slotXml->getIntAttribute ("index", 0));
            patternSlots[static_cast<size_t>(slot)].numSteps = slotXml->getIntAttribute ("numSteps", 16);

            for (auto* stepXml : slotXml->getChildWithTagNameIterator ("Step"))
            {
                const int idx = stepXml->getIntAttribute ("idx", -1);
                if (idx >= 0 && idx < PulseForge::kMaxSteps)
                {
                    auto& st = patternSlots[static_cast<size_t>(slot)].steps[static_cast<size_t>(idx)];
                    st.active         = stepXml->getBoolAttribute ("active", true);
                    st.semitoneOffset = stepXml->getIntAttribute ("semi", 0);
                    st.octaveShift    = stepXml->getIntAttribute ("oct", 0);
                    st.velocity       = stepXml->getIntAttribute ("vel", 100);
                    st.gate           = static_cast<float> (stepXml->getDoubleAttribute ("gate", 0.75));
                    st.accent         = stepXml->getBoolAttribute ("accent", false);
                    st.slide          = stepXml->getBoolAttribute ("slide", false);
                    st.ratchet        = stepXml->getIntAttribute ("ratchet", 1);
                    st.probability    = stepXml->getIntAttribute ("prob", 100);
                    st.ccValue        = stepXml->getIntAttribute ("cc", 64);
                    st.chordType      = stepXml->getIntAttribute ("chord", 0);
                }
            }
        }

        updatePlaybackSnapshot();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PulseForgeAudioProcessor();
}
