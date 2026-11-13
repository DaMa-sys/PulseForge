#include "PluginProcessor.h"
#include "PluginEditor.h"

PulseForgeAudioProcessor::PulseForgeAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    activeNotes.reserve (64);
    pendingSubTriggers.reserve (64);
    heldInputNotes.reserve (16);

    // Initialize 3 independent multi-timbral sequences
    tracks[0].midiChannel = 1;
    tracks[0].role = PulseForge::InstrumentRole::MonoBass;
    tracks[0].baseOctave = 1;
    tracks[0].name = "SEQ 1: BASS";
    tracks[0].muted = false;

    tracks[1].midiChannel = 2;
    tracks[1].role = PulseForge::InstrumentRole::ClassicArp;
    tracks[1].baseOctave = 2;
    tracks[1].name = "SEQ 2: ARP";
    tracks[1].muted = false;

    tracks[2].midiChannel = 3;
    tracks[2].role = PulseForge::InstrumentRole::PolyChordStab;
    tracks[2].baseOctave = 2;
    tracks[2].name = "SEQ 3: CHORD";
    tracks[2].muted = false;

    // Track 0: Driving House / Melodic Bass groove
    PulseForge::StyleGenerator::GenerationParams gp0;
    gp0.genre = PulseForge::GenreStyle::ClassicHouse;
    gp0.role  = PulseForge::InstrumentRole::MonoBass;
    gp0.scale = PulseForge::ScaleType::Dorian;
    gp0.density = 0.60f;
    gp0.syncopation = 0.55f;
    gp0.spice = 0.40f;
    gp0.pitchRange = 0.40f;
    gp0.numSteps = 16;
    PulseForge::StyleGenerator::generatePattern (tracks[0].pattern, gp0, 101);

    // Track 1: Melodic 16th Arp / Lead Hook
    PulseForge::StyleGenerator::GenerationParams gp1;
    gp1.genre = PulseForge::GenreStyle::ClassicHouse;
    gp1.role  = PulseForge::InstrumentRole::ClassicArp;
    gp1.scale = PulseForge::ScaleType::Dorian;
    gp1.density = 0.75f;
    gp1.syncopation = 0.40f;
    gp1.spice = 0.50f;
    gp1.pitchRange = 0.65f;
    gp1.numSteps = 16;
    PulseForge::StyleGenerator::generatePattern (tracks[1].pattern, gp1, 202);

    // Track 2: Polyphonic Chord Stabs
    PulseForge::StyleGenerator::GenerationParams gp2;
    gp2.genre = PulseForge::GenreStyle::ClassicHouse;
    gp2.role  = PulseForge::InstrumentRole::PolyChordStab;
    gp2.scale = PulseForge::ScaleType::Dorian;
    gp2.density = 0.45f;
    gp2.syncopation = 0.60f;
    gp2.spice = 0.35f;
    gp2.pitchRange = 0.40f;
    gp2.numSteps = 16;
    PulseForge::StyleGenerator::generatePattern (tracks[2].pattern, gp2, 303);

    // Setup Hardware Serial Controller Callbacks (Arduino Mega Desk Controller)
    hardwareSerial.onGenerateTrigger = [this](int track)
    {
        juce::MessageManager::callAsync ([this, track] { triggerGenerateTrack (track); });
    };
    hardwareSerial.onMutateTrigger   = [this](int track)
    {
        juce::MessageManager::callAsync ([this, track] { triggerMutateTrack (track); });
    };
    hardwareSerial.onParameterChange = [this](int track, const juce::String& param, int val)
    {
        juce::MessageManager::callAsync ([this, track, param, val]
        {
            const juce::String prefix = "t" + juce::String (track) + "_";
            if (param == "DENS")
            {
                if (auto* p = apvts.getParameter (prefix + "density"))
                    p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (val)));
                updateTrackComplexity (track, static_cast<float> (val));
            }
            else if (param == "SWING")
            {
                if (auto* p = apvts.getParameter (prefix + "swing"))
                    p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (val)));
                updatePlaybackSnapshot();
            }
            else if (param == "BOCT")
            {
                if (auto* p = apvts.getParameter (prefix + "baseOctave"))
                    p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (val)));
                updatePlaybackSnapshot();
            }
            else if (param == "AOCT")
            {
                if (auto* p = apvts.getParameter (prefix + "arpOctaves"))
                    p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (val)));
                triggerGenerateTrack (track);
            }
            else if (param == "RATIO")
            {
                if (auto* p = apvts.getParameter (prefix + "tempoRatio"))
                    p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (val)));
                updatePlaybackSnapshot();
            }
        });
    };
    hardwareSerial.onEncoderStep = [this](int track, const juce::String& target, int dir)
    {
        juce::MessageManager::callAsync ([this, track, target, dir]
        {
            const juce::String prefix = "t" + juce::String (track) + "_";
            if (target == "STEPS")
            {
                if (auto* p = apvts.getParameter (prefix + "numSteps"))
                {
                    int cur = static_cast<int> (std::round (p->convertFrom0to1 (p->getValue())));
                    int next = juce::jlimit (0, 3, cur + dir);
                    static constexpr int kStepChoices[4] = { 4, 8, 16, 32 };
                    setTrackNumSteps (track, kStepChoices[next]);
                    triggerGenerateTrack (track);
                }
            }
            else if (target == "GENRE")
            {
                if (auto* p = apvts.getParameter (prefix + "genre"))
                {
                    int cur = static_cast<int> (std::round (p->convertFrom0to1 (p->getValue())));
                    int next = juce::jlimit (0, 7, cur + dir);
                    setTrackGenre (track, static_cast<PulseForge::GenreStyle> (next));
                    triggerGenerateTrack (track);
                }
            }
            else if (target == "ROLE")
            {
                if (auto* p = apvts.getParameter (prefix + "role"))
                {
                    int cur = static_cast<int> (std::round (p->convertFrom0to1 (p->getValue())));
                    int next = juce::jlimit (0, 6, cur + dir);
                    setTrackRole (track, static_cast<PulseForge::InstrumentRole> (next));
                    triggerGenerateTrack (track);
                }
            }
        });
    };
    hardwareSerial.onProcessorConnectionStatusChanged = [this](bool connected, const juce::String&)
    {
        if (connected)
        {
            juce::MessageManager::callAsync ([this]()
            {
                syncAllTracksToHardware();
            });
        }
    };

    // Register APVTS Listeners for true Bi-Directional GUI <-> Hardware Sync
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const juce::String pfx = "t" + juce::String (t) + "_";
        apvts.addParameterListener (pfx + "density", this);
        apvts.addParameterListener (pfx + "swing", this);
        apvts.addParameterListener (pfx + "baseOctave", this);
        apvts.addParameterListener (pfx + "genre", this);
        apvts.addParameterListener (pfx + "role", this);
        apvts.addParameterListener (pfx + "numSteps", this);
    }

    hardwareSerial.startAutoConnect();

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::updatePlaybackSnapshot() noexcept
{
    const int nextIdx = 1 - activeSnapshotIndex.load (std::memory_order_relaxed);
    auto& snap = audioSnapshots[static_cast<size_t>(nextIdx)];
    const float globalGate = apvts.getRawParameterValue ("globalGate")->load() * 0.01f;

    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        auto& dst = snap.tracks[static_cast<size_t>(t)];
        const auto& src = tracks[static_cast<size_t>(t)];

        dst.steps       = src.pattern.steps;
        dst.numSteps    = src.pattern.numSteps;
        dst.mode        = src.mode;
        dst.arpDir      = src.arpDir;
        dst.rateIdx     = src.rateIdx;
        dst.globalGate  = globalGate;

        const juce::String prefix = "t" + juce::String (t) + "_";

        if (auto* p = apvts.getRawParameterValue (prefix + "numSteps"))
        {
            const int sIdx = static_cast<int> (p->load());
            int n = 16;
            if (sIdx == 0) n = 4;
            else if (sIdx == 1) n = 8;
            else if (sIdx == 2) n = 16;
            else if (sIdx == 3) n = 32;
            dst.numSteps = n;
            tracks[static_cast<size_t>(t)].pattern.numSteps = n;
            tracks[static_cast<size_t>(t)].numSteps = n;
        }
        else
        {
            dst.numSteps = src.pattern.numSteps;
        }

        if (auto* p = apvts.getRawParameterValue (prefix + "midiChannel"))
            dst.midiChannel = static_cast<int> (p->load());
        else
            dst.midiChannel = src.midiChannel;

        if (auto* p = apvts.getRawParameterValue (prefix + "muted"))
            dst.muted = (p->load() > 0.5f);
        else
            dst.muted = src.muted;

        if (auto* p = apvts.getRawParameterValue (prefix + "role"))
            dst.role = static_cast<PulseForge::InstrumentRole> (static_cast<int> (p->load()));
        else
            dst.role = src.role;

        if (auto* p = apvts.getRawParameterValue (prefix + "baseOctave"))
            dst.baseOctave = static_cast<int> (p->load());
        else
            dst.baseOctave = src.baseOctave;

        if (auto* p = apvts.getRawParameterValue (prefix + "arpOctaves"))
            dst.arpOctaves = static_cast<int> (p->load());
        else
            dst.arpOctaves = src.arpOctaves;

        if (auto* p = apvts.getRawParameterValue (prefix + "swing"))
            dst.swing = p->load();
        else
            dst.swing = src.swing;

        if (auto* p = apvts.getRawParameterValue (prefix + "tempoRatio"))
            dst.tempoRatioIdx = static_cast<int> (p->load());
        else
            dst.tempoRatioIdx = src.tempoRatioIdx;

        dst.genre = src.genre;
        dst.density = src.density;
    }

    if (auto* p = apvts.getRawParameterValue ("harmonicContext"))
        snap.harmonicContextEnabled = (p->load() > 0.5f);
    else
        snap.harmonicContextEnabled = true;

    snap.progressionIndex = globalProgressionIndex.load();

    activeSnapshotIndex.store (nextIdx, std::memory_order_release);
}

PulseForgeAudioProcessor::~PulseForgeAudioProcessor()
{
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const juce::String pfx = "t" + juce::String (t) + "_";
        apvts.removeParameterListener (pfx + "density", this);
        apvts.removeParameterListener (pfx + "swing", this);
        apvts.removeParameterListener (pfx + "baseOctave", this);
        apvts.removeParameterListener (pfx + "genre", this);
        apvts.removeParameterListener (pfx + "role", this);
        apvts.removeParameterListener (pfx + "numSteps", this);
    }
    hardwareSerial.stopAutoConnect();
    const juce::ScopedLock sl (midiOutLock);
    activeHardwareOuts.clear();
}

juce::AudioProcessorValueTreeState::ParameterLayout PulseForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

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

    const juce::StringArray roles {
        "Mono Bass (SH-101 / Moog)",
        "Acid Line (TB-303 Glide)",
        "Classic Arpeggio (Juno/Jup)",
        "Lead & Hook (Prophet/MS20)",
        "Poly Chord Stabs (House/Dub)",
        "Harmony Chords (Progression)",
        "Modular Ratchet Seq"
    };

    const juce::StringArray tempoRatios {
        "1/4x", "1/3x", "1/2x", "2/3x", "3/4x",
        "1/1x",
        "4/3x", "3/2x", "2/1x", "3/1x", "4/1x"
    };

    // 1. Global Master Parameters
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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rate", "Step Grid",
        juce::StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "globalGate", "Global Gate %", juce::NormalisableRange<float> (20.0f, 150.0f, 1.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("autoPlay", "Free Run (Audition)", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("previewSynth", "Internal Analog Preview", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("harmonicContext", "Harmonic Context Master", true));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("ccNumber", "MIDI CC #", 1, 119, 74));

    // 2. Per-Track Dedicated Parameters (3 Tracks: SEQ 1, SEQ 2, SEQ 3)
    const juce::StringArray stepChoices { "4", "8", "16", "32" };
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const juce::String p = "t" + juce::String (t) + "_";
        const juce::String tName = "SEQ " + juce::String (t + 1) + " ";

        const int defaultRole = (t == 0) ? 0 : ((t == 1) ? 2 : 4);
        const float defaultDensity = (t == 0) ? 65.0f : ((t == 1) ? 75.0f : 45.0f);
        const int defaultBaseOct = (t == 0) ? 1 : 2;
        const int defaultArpOct  = (t == 0) ? 1 : ((t == 1) ? 3 : 2);
        const int defaultMidiCh  = t + 1;

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            p + "genre", tName + "Genre", genres, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            p + "role", tName + "Role", roles, defaultRole));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            p + "numSteps", tName + "Steps", stepChoices, 2)); // default 16 (index 2)

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            p + "density", tName + "Density", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), defaultDensity));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            p + "swing", tName + "Swing", juce::NormalisableRange<float> (0.0f, 75.0f, 1.0f), 20.0f));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            p + "baseOctave", tName + "Base Oct", 0, 5, defaultBaseOct));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            p + "arpOctaves", tName + "Arp Octaves", 1, 4, defaultArpOct));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            p + "midiChannel", tName + "MIDI Channel", 1, 16, defaultMidiCh));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            p + "tempoRatio", tName + "BPM Ratio", tempoRatios, 5));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            p + "muted", tName + "Mute", false));
    }

    // 3. Fallback Legacy Parameters
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Engine Mode", juce::StringArray { "Sequencer", "Arpeggiator" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("genre", "Legacy Genre", genres, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("role", "Legacy Role", roles, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("arpDir", "Arp Direction", juce::StringArray { "Up", "Down", "Up-Down", "Converge", "Pinky Pedal", "Random Walk", "Chord Stabs" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("baseOctave", "Legacy Base Octave", 0, 5, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("arpOctaves", "Legacy Arp Octaves", 1, 4, 2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("density", "Legacy Density", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 65.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("syncopation", "Syncopation", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 55.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("spice", "Spice / Acid", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 45.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("pitchRange", "Pitch Range", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 55.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("swing", "Legacy Swing", juce::NormalisableRange<float> (0.0f, 75.0f, 1.0f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("numSteps", "Pattern Steps", 1, 16, 16));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("midiChannel", "Legacy MIDI Channel", 1, 16, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("tempoRatio", "Legacy BPM Ratio", tempoRatios, 5));

    return { params.begin(), params.end() };
}

const juce::String PulseForgeAudioProcessor::getName() const { return JucePlugin_Name; }
bool PulseForgeAudioProcessor::acceptsMidi() const { return true; }
bool PulseForgeAudioProcessor::producesMidi() const { return true; }
bool PulseForgeAudioProcessor::isMidiEffect() const { return false; }
double PulseForgeAudioProcessor::getTailLengthSeconds() const { return 0.1; }
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
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        lastTriggeredStepIndex[t] = -999999;
        arpWalkIndex[t] = 0;
        arpDirectionSign[t] = 1;
        trackPlaySteps[static_cast<size_t>(t)].store (0);
    }
    activeNotes.clear();
    pendingSubTriggers.clear();
    previewSynth.prepare (currentSampleRate);
    updatePlaybackSnapshot();

    // Standalone .exe: start stopped and silent by default
    if (auto* pRun = apvts.getParameter ("autoPlay"))
        pRun->setValueNotifyingHost (0.0f);
}

void PulseForgeAudioProcessor::releaseResources()
{
    previewSynth.reset();
}

bool PulseForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PulseForgeAudioProcessor::setActiveTrackIndex (int idx)
{
    idx = juce::jlimit (0, PulseForge::kNumTracks - 1, idx);
    const int prevIdx = activeTrackIndex.load();
    if (idx == prevIdx)
        return;

    const juce::ScopedLock sl (patternLock);

    // Save previous track state
    auto& prevTrack = tracks[static_cast<size_t>(prevIdx)];
    prevTrack.midiChannel = static_cast<int> (apvts.getRawParameterValue ("midiChannel")->load());
    prevTrack.role        = static_cast<PulseForge::InstrumentRole> (static_cast<int> (apvts.getRawParameterValue ("role")->load()));
    prevTrack.mode        = static_cast<PulseForge::EngineMode> (static_cast<int> (apvts.getRawParameterValue ("mode")->load()));
    prevTrack.arpDir      = static_cast<PulseForge::ArpDirection> (static_cast<int> (apvts.getRawParameterValue ("arpDir")->load()));
    prevTrack.arpOctaves  = static_cast<int> (apvts.getRawParameterValue ("arpOctaves")->load());
    prevTrack.baseOctave  = static_cast<int> (apvts.getRawParameterValue ("baseOctave")->load());
    prevTrack.rateIdx     = static_cast<int> (apvts.getRawParameterValue ("rate")->load());
    prevTrack.numSteps    = static_cast<int> (apvts.getRawParameterValue ("numSteps")->load());
    prevTrack.density     = apvts.getRawParameterValue ("density")->load();
    prevTrack.syncopation = apvts.getRawParameterValue ("syncopation")->load();
    prevTrack.spice       = apvts.getRawParameterValue ("spice")->load();
    prevTrack.pitchRange  = apvts.getRawParameterValue ("pitchRange")->load();

    activeTrackIndex.store (idx);
    auto& newTrack = tracks[static_cast<size_t>(idx)];

    // Sync APVTS to newly selected track
    if (auto* p = apvts.getParameter ("midiChannel"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.midiChannel)));
    if (auto* p = apvts.getParameter ("role"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.role)));
    if (auto* p = apvts.getParameter ("mode"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.mode)));
    if (auto* p = apvts.getParameter ("arpDir"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.arpDir)));
    if (auto* p = apvts.getParameter ("arpOctaves"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.arpOctaves)));
    if (auto* p = apvts.getParameter ("baseOctave"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.baseOctave)));
    if (auto* p = apvts.getParameter ("rate"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.rateIdx)));
    if (auto* p = apvts.getParameter ("numSteps"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (newTrack.numSteps)));
    if (auto* p = apvts.getParameter ("density"))
        p->setValueNotifyingHost (p->convertTo0to1 (newTrack.density));
    if (auto* p = apvts.getParameter ("syncopation"))
        p->setValueNotifyingHost (p->convertTo0to1 (newTrack.syncopation));
    if (auto* p = apvts.getParameter ("spice"))
        p->setValueNotifyingHost (p->convertTo0to1 (newTrack.spice));
    if (auto* p = apvts.getParameter ("pitchRange"))
        p->setValueNotifyingHost (p->convertTo0to1 (newTrack.pitchRange));

    updatePlaybackSnapshot();
}

bool PulseForgeAudioProcessor::isTrackMuted (int idx) const noexcept
{
    idx = juce::jlimit (0, PulseForge::kNumTracks - 1, idx);
    return tracks[static_cast<size_t>(idx)].muted;
}

void PulseForgeAudioProcessor::setTrackMuted (int idx, bool muted)
{
    idx = juce::jlimit (0, PulseForge::kNumTracks - 1, idx);
    const juce::ScopedLock sl (patternLock);
    tracks[static_cast<size_t>(idx)].muted = muted;
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::toggleTrackMuted (int idx)
{
    idx = juce::jlimit (0, PulseForge::kNumTracks - 1, idx);
    setTrackMuted (idx, !isTrackMuted (idx));
}

int PulseForgeAudioProcessor::getTrackNumSteps (int trackIdx) const noexcept
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    return tracks[static_cast<size_t>(trackIdx)].pattern.numSteps;
}

void PulseForgeAudioProcessor::setTrackNumSteps (int trackIdx, int numSteps)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    tracks[static_cast<size_t>(trackIdx)].pattern.numSteps = juce::jlimit (4, PulseForge::kMaxSteps, numSteps);
    tracks[static_cast<size_t>(trackIdx)].numSteps = tracks[static_cast<size_t>(trackIdx)].pattern.numSteps;

    const juce::String prefix = "t" + juce::String (trackIdx) + "_";
    if (auto* p = apvts.getParameter (prefix + "numSteps"))
    {
        int choiceIdx = 2; // 16
        if (numSteps <= 4) choiceIdx = 0;
        else if (numSteps <= 8) choiceIdx = 1;
        else if (numSteps <= 16) choiceIdx = 2;
        else choiceIdx = 3;
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (choiceIdx)));
    }

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::setTrackGenre (int trackIdx, PulseForge::GenreStyle genre)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    tracks[static_cast<size_t>(trackIdx)].genre = genre;

    const juce::String prefix = "t" + juce::String (trackIdx) + "_";
    if (auto* p = apvts.getParameter (prefix + "genre"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (genre)));

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::setTrackRole (int trackIdx, PulseForge::InstrumentRole role)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    tracks[static_cast<size_t>(trackIdx)].role = role;

    const juce::String prefix = "t" + juce::String (trackIdx) + "_";
    if (auto* p = apvts.getParameter (prefix + "role"))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (role)));

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerGenerateTrio()
{
    const juce::ScopedLock sl (patternLock);
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));

    // Trio Harmonize allinea anche la progressione armonica globale per una perfetta sintonia delle 3 parti
    const int newProg = static_cast<int> (juce::Random::getSystemRandom().nextInt (8));
    globalProgressionIndex.store (newProg);

    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const juce::String prefix = "t" + juce::String (t) + "_";
        PulseForge::GenreStyle genre = tracks[static_cast<size_t>(t)].genre;
        if (auto* p = apvts.getRawParameterValue (prefix + "genre"))
            genre = static_cast<PulseForge::GenreStyle> (static_cast<int> (p->load()));
        else if (auto* pg = apvts.getRawParameterValue ("genre"))
            genre = static_cast<PulseForge::GenreStyle> (static_cast<int> (pg->load()));

        PulseForge::InstrumentRole role = tracks[static_cast<size_t>(t)].role;
        if (auto* p = apvts.getRawParameterValue (prefix + "role"))
            role = static_cast<PulseForge::InstrumentRole> (static_cast<int> (p->load()));

        float density = tracks[static_cast<size_t>(t)].density * 0.01f;
        if (auto* p = apvts.getRawParameterValue (prefix + "density"))
            density = p->load() * 0.01f;

        uint32_t newSeed = 0;
        while (newSeed == 0 || newSeed == tracks[static_cast<size_t>(t)].seed)
        {
            newSeed = static_cast<uint32_t> (juce::Random::getSystemRandom().nextInt()) ^ static_cast<uint32_t> (juce::Time::getHighResolutionTicks() + t * 7919);
        }
        tracks[static_cast<size_t>(t)].seed = newSeed;
        tracks[static_cast<size_t>(t)].genre = genre;
        tracks[static_cast<size_t>(t)].role = role;

        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre       = genre;
        gp.role        = role;
        gp.scale       = scale;
        gp.density     = density;
        gp.syncopation = (t == 0) ? 0.55f : ((t == 1) ? 0.40f : 0.60f);
        gp.spice       = (t == 0) ? 0.40f : ((t == 1) ? 0.50f : 0.35f);
        gp.pitchRange  = (t == 0) ? 0.40f : ((t == 1) ? 0.65f : 0.40f);
        gp.numSteps    = tracks[static_cast<size_t>(t)].numSteps;

        PulseForge::StyleGenerator::generatePattern (tracks[static_cast<size_t>(t)].pattern, gp, newSeed);
    }

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerNewProgression()
{
    const juce::ScopedLock sl (patternLock);
    const int cur = globalProgressionIndex.load();
    const int next = (cur + 1) % 8;
    globalProgressionIndex.store (next);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerGenerateTrack (int trackIdx)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);

    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    const juce::String prefix = "t" + juce::String (trackIdx) + "_";

    PulseForge::GenreStyle genre = tracks[static_cast<size_t>(trackIdx)].genre;
    if (auto* p = apvts.getRawParameterValue (prefix + "genre"))
        genre = static_cast<PulseForge::GenreStyle> (static_cast<int> (p->load()));

    PulseForge::InstrumentRole role = tracks[static_cast<size_t>(trackIdx)].role;
    if (auto* p = apvts.getRawParameterValue (prefix + "role"))
        role = static_cast<PulseForge::InstrumentRole> (static_cast<int> (p->load()));

    float density = tracks[static_cast<size_t>(trackIdx)].density * 0.01f;
    if (auto* p = apvts.getRawParameterValue (prefix + "density"))
        density = p->load() * 0.01f;

    int numSteps = tracks[static_cast<size_t>(trackIdx)].numSteps;
    if (auto* p = apvts.getRawParameterValue (prefix + "numSteps"))
    {
        const int sIdx = static_cast<int> (p->load());
        if (sIdx == 0) numSteps = 4;
        else if (sIdx == 1) numSteps = 8;
        else if (sIdx == 2) numSteps = 16;
        else if (sIdx == 3) numSteps = 32;
    }
    tracks[static_cast<size_t>(trackIdx)].genre = genre;
    tracks[static_cast<size_t>(trackIdx)].role = role;
    tracks[static_cast<size_t>(trackIdx)].numSteps = numSteps;
    tracks[static_cast<size_t>(trackIdx)].pattern.numSteps = numSteps;

    // Genera un nuovo SEED casuale e unico a ogni click di GENERATE
    uint32_t newSeed = 0;
    while (newSeed == 0 || newSeed == tracks[static_cast<size_t>(trackIdx)].seed)
    {
        newSeed = static_cast<uint32_t> (juce::Random::getSystemRandom().nextInt()) ^ static_cast<uint32_t> (juce::Time::getHighResolutionTicks());
    }
    tracks[static_cast<size_t>(trackIdx)].seed = newSeed;

    PulseForge::StyleGenerator::GenerationParams gp;
    gp.genre       = genre;
    gp.role        = role;
    gp.scale       = scale;
    gp.density     = density;
    gp.syncopation = 0.50f;
    gp.spice       = 0.45f;
    gp.pitchRange  = 0.55f;
    gp.numSteps    = numSteps;

    PulseForge::StyleGenerator::generatePattern (tracks[static_cast<size_t>(trackIdx)].pattern, gp, newSeed);
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerMutateTrack (int trackIdx)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    auto& trk = tracks[static_cast<size_t>(trackIdx)];
    const auto scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));

    // Autentica mutazione musicale sul pattern esistente (~10-15% variazione note, step e articolazioni)
    PulseForge::StyleGenerator::mutatePattern (trk.pattern, scale, 0.45f);

    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::updateTrackComplexity (int trackIdx, float newComplexity)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    auto& trk = tracks[static_cast<size_t>(trackIdx)];
    trk.density = newComplexity;

    if (!transportRunning.load())
    {
        const juce::ScopedLock sl (patternLock);
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = trk.genre;
        gp.role = trk.role;
        gp.scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
        gp.density = newComplexity * 0.01f;
        gp.numSteps = trk.numSteps;
        PulseForge::StyleGenerator::generatePattern (trk.pattern, gp, trk.seed);
        updatePlaybackSnapshot();
    }
    else
    {
        pendingComplexityUpdate[static_cast<size_t>(trackIdx)].store (newComplexity);
    }
}

void PulseForgeAudioProcessor::triggerClearTrack (int trackIdx)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    auto& pat = tracks[static_cast<size_t>(trackIdx)].pattern;
    for (auto& s : pat.steps)
    {
        s.active = false;
        s.semitoneOffset = 0;
        s.octaveShift = 0;
        s.accent = false;
        s.ratchet = 1;
        s.gate = 0.75f;
    }
    pat.steps[0].active = true;
    updatePlaybackSnapshot();
}

void PulseForgeAudioProcessor::triggerRotateTrack (int trackIdx, int delta)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    PulseForge::StyleGenerator::rotatePattern (tracks[static_cast<size_t>(trackIdx)].pattern, delta);
    updatePlaybackSnapshot();
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

int PulseForgeAudioProcessor::computeHarmonicStepMidiNote (int trackIdx, int stepIdx, const PulseForge::StepData& step,
                                                          int rootMidi, PulseForge::ScaleType scale,
                                                          int globalBar, int progIdx, PulseForge::GenreStyle genre,
                                                          PulseForge::InstrumentRole role, bool harmonicContextOn) const
{
    // Se HARMONIC CONTEXT è disattivato: comportamento identico originale (golden standard)
    if (!harmonicContextOn)
        return computeStepMidiNote (stepIdx, step, rootMidi, scale);

    // Con HARMONIC CONTEXT ATTIVO:
    // 1. Calcola la radice armonica dell'accordo della battuta corrente dal profilo di genere
    const int chordRootSemiRaw = PulseForge::getGenreProgressionSemitone (genre, progIdx, globalBar);
    const int chordRootSemi    = PulseForge::quantizeSemitoneToScale (chordRootSemiRaw, scale);

    // Intervalli della scala selezionata
    const auto& scaleInts = PulseForge::getScaleIntervals (scale);
    auto quantizeScale = [&scaleInts] (int s) -> int
    {
        if (scaleInts.empty()) return 0;
        int oct = (s >= 0) ? (s / 12) : ((s - 11) / 12);
        int rem = s - (oct * 12);
        int best = scaleInts[0];
        int bestDiff = 999;
        for (int sc : scaleInts)
        {
            int diff = std::abs (sc - rem);
            if (diff < bestDiff) { bestDiff = diff; best = sc; }
        }
        return oct * 12 + best;
    };

    int noteOffsetFromScaleRoot = 0;

    switch (step.harmonicFunc)
    {
        case PulseForge::HarmonicFunction::Root:
            noteOffsetFromScaleRoot = chordRootSemi;
            break;

        case PulseForge::HarmonicFunction::Third:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + 3);
            break;

        case PulseForge::HarmonicFunction::Fifth:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + 7);
            break;

        case PulseForge::HarmonicFunction::Seventh:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + 10);
            break;

        case PulseForge::HarmonicFunction::Ninth:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + 14);
            break;

        case PulseForge::HarmonicFunction::Eleventh:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + 17);
            break;

        case PulseForge::HarmonicFunction::OctavePop:
            noteOffsetFromScaleRoot = chordRootSemi + 12;
            break;

        case PulseForge::HarmonicFunction::Passing:
        case PulseForge::HarmonicFunction::Tension:
        default:
            noteOffsetFromScaleRoot = quantizeScale (chordRootSemi + step.semitoneOffset);
            break;
    }

    // Separazione dei Registri per evitare incroci di voci
    // BASS: C1 - E2 (24..40)
    // CHORDS: G2 - B3 (43..59)
    // LEAD / ARP: C4 - G5 (60..79)
    int registerBaseOct = 0;
    if (role == PulseForge::InstrumentRole::MonoBass || role == PulseForge::InstrumentRole::AcidLine)
    {
        registerBaseOct = 0; // Registro basso
    }
    else if (role == PulseForge::InstrumentRole::PolyChordStab || role == PulseForge::InstrumentRole::HarmonyChords)
    {
        registerBaseOct = 1; // Registro medio
    }
    else // ClassicArp, LeadHook, ModularRatchet
    {
        registerBaseOct = 2; // Registro alto
    }

    const int finalNote = (rootMidi - 12) + (registerBaseOct * 12) + noteOffsetFromScaleRoot + (step.octaveShift * 12) + liveTransposeSemitones;
    return juce::jlimit (12, 115, finalNote);
}

void PulseForgeAudioProcessor::sendMidiOut (int trackIdx, const juce::MidiMessage& msg, juce::MidiBuffer& midiOut, int sampleOffset)
{
    // 1. Always deliver to DAW host output buffer with zero locks or blocking
    midiOut.addEvent (msg, juce::jmax (0, sampleOffset));

    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const auto& trk = tracks[static_cast<size_t>(trackIdx)];
    const juce::String& target = trk.targetHwDevice;

    if (target.isEmpty() || target == "DAW Only" || target == "DAW (VST3)")
        return;

    // 2. Hardware USB Out: Use tryEnter() (non-blocking try-lock)
    // If the GUI or host state serialization currently holds midiOutLock, NEVER block processBlock!
    // Skip this specific hardware USB mirror rather than causing a buffer dropout in the DAW.
    if (midiOutLock.tryEnter())
    {
        for (auto& port : activeHardwareOuts)
        {
            if (port.device != nullptr)
            {
                if (target == "All USB Synths" || port.name == target || port.name.containsIgnoreCase (target))
                {
                    port.device->sendMessageNow (msg);
                    // Mirror on Channel 1 for dedicated synths set to factory Ch1
                    if (target != "All USB Synths" && msg.getChannel() != 1)
                    {
                        auto ch1Msg = msg;
                        ch1Msg.setChannel (1);
                        port.device->sendMessageNow (ch1Msg);
                    }
                }
            }
        }
        midiOutLock.exit();
    }
}

void PulseForgeAudioProcessor::stopAllActiveNotes (juce::MidiBuffer& midiOut, int sampleOffset)
{
    for (const auto& an : activeNotes)
    {
        if (an.midiNote >= 0)
        {
            sendMidiOut (an.trackIndex, juce::MidiMessage::noteOff (an.channel, an.midiNote), midiOut, sampleOffset);
            previewSynth.noteOff (an.midiNote);
        }
    }
    activeNotes.clear();
    pendingSubTriggers.clear();
    previewSynth.allNotesOff();
}

void PulseForgeAudioProcessor::triggerStepEventsForTrack (int trackIdx, int stepIdx, double stepDurationSamples,
                                                          int sampleOffset, juce::MidiBuffer& midiOut,
                                                          const PlaybackSnapshot& snap)
{
    const auto& trackSnap = snap.tracks[static_cast<size_t>(trackIdx)];
    if (trackSnap.muted)
        return;

    const auto& step = trackSnap.steps[static_cast<size_t>(stepIdx % PulseForge::kMaxSteps)];
    if (!step.active)
        return;

    hardwareSerial.sendStepFlash (trackIdx, stepIdx, step.accent);

    const int midiChan         = trackSnap.midiChannel;
    const int ccNumber         = static_cast<int> (apvts.getRawParameterValue ("ccNumber")->load());
    const int rootKey          = static_cast<int> (apvts.getRawParameterValue ("rootNote")->load());
    const int baseOctave       = trackSnap.baseOctave;
    const auto scale           = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
    const auto mode            = trackSnap.mode;
    const auto role            = trackSnap.role;
    const auto arpDir          = trackSnap.arpDir;
    const int arpOctaves       = trackSnap.arpOctaves;
    const float globalGateMult = trackSnap.globalGate;
    const bool isPreviewTrack  = (trackIdx == activeTrackIndex.load());

    // Check probability (100% always fires)
    if (step.probability < 100)
    {
        int roll = std::uniform_int_distribution<int> (1, 100) (probRng);
        if (roll > step.probability)
            return;
    }

    // Send per-step MIDI CC modulation (e.g. Filter Cutoff CC74)
    const int effectiveCC = juce::jlimit (0, 127, step.accent ? juce::jmax (step.ccValue, 112) : step.ccValue);
    sendMidiOut (trackIdx, juce::MidiMessage::controllerEvent (midiChan, ccNumber, effectiveCC), midiOut, sampleOffset);
    if (isPreviewTrack)
        previewSynth.setCutModCC (effectiveCC);

    const int rootMidi = (baseOctave + 1) * 12 + rootKey;
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
                    chosenIdx = arpWalkIndex[trackIdx] % poolSize;
                    arpWalkIndex[trackIdx] = (arpWalkIndex[trackIdx] + 1) % poolSize;
                    break;
                case PulseForge::ArpDirection::Down:
                    chosenIdx = (poolSize - 1) - (arpWalkIndex[trackIdx] % poolSize);
                    arpWalkIndex[trackIdx] = (arpWalkIndex[trackIdx] + 1) % poolSize;
                    break;
                case PulseForge::ArpDirection::UpDown:
                    if (poolSize <= 1) chosenIdx = 0;
                    else
                    {
                        chosenIdx = juce::jlimit (0, poolSize - 1, arpWalkIndex[trackIdx]);
                        arpWalkIndex[trackIdx] += arpDirectionSign[trackIdx];
                        if (arpWalkIndex[trackIdx] >= poolSize - 1) { arpWalkIndex[trackIdx] = poolSize - 1; arpDirectionSign[trackIdx] = -1; }
                        else if (arpWalkIndex[trackIdx] <= 0)       { arpWalkIndex[trackIdx] = 0; arpDirectionSign[trackIdx] = 1; }
                    }
                    break;
                case PulseForge::ArpDirection::Converge:
                {
                    int half = arpWalkIndex[trackIdx] / 2;
                    chosenIdx = (arpWalkIndex[trackIdx] % 2 == 0) ? (half % poolSize) : ((poolSize - 1 - (half % poolSize)) % poolSize);
                    arpWalkIndex[trackIdx] = (arpWalkIndex[trackIdx] + 1) % (poolSize * 2);
                    break;
                }
                case PulseForge::ArpDirection::PinkyPedal:
                    chosenIdx = (stepIdx % 2 == 0) ? ((arpWalkIndex[trackIdx]++) % poolSize) : (poolSize - 1);
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
        const int curBar = currentTransportBar.load();
        const int baseNote = computeHarmonicStepMidiNote (trackIdx, stepIdx, step, rootMidi, scale,
                                                          curBar, snap.progressionIndex, trackSnap.genre,
                                                          role, snap.harmonicContextEnabled);
        notesToTrigger = PulseForge::buildChordVoicingNotes (baseNote, step, rootMidi, scale, role, liveTransposeSemitones);
    }

    const int vel = juce::jlimit (1, 127, step.accent ? juce::jmax (step.velocity, 120) : step.velocity);
    const int ratchetCount = juce::jlimit (1, 4, step.ratchet);

    // Separate previous legato-slide notes from normal notes FOR THIS TRACK
    std::vector<ActiveNoteEvent> previousLegatoNotes;
    previousLegatoNotes.reserve (8);
    for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[static_cast<size_t>(i)].trackIndex == trackIdx)
        {
            if (activeNotes[static_cast<size_t>(i)].waitingForLegatoOverlap)
            {
                previousLegatoNotes.push_back (activeNotes[static_cast<size_t>(i)]);
                activeNotes.erase (activeNotes.begin() + i);
            }
            else
            {
                sendMidiOut (trackIdx,
                             juce::MidiMessage::noteOff (activeNotes[static_cast<size_t>(i)].channel,
                                                         activeNotes[static_cast<size_t>(i)].midiNote),
                             midiOut, sampleOffset);
                if (isPreviewTrack)
                    previewSynth.noteOff (activeNotes[static_cast<size_t>(i)].midiNote);
                activeNotes.erase (activeNotes.begin() + i);
            }
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
                    sendMidiOut (trackIdx, juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (subVel)), midiOut, sampleOffset);
                    if (isPreviewTrack)
                        previewSynth.noteOn (n, subVel, isLegatoArrival);
                    activeNotes.push_back ({ n, midiChan, subGateLen, false, trackIdx });
                    if (isPreviewTrack)
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
                        midiChan,
                        trackIdx
                    });
                }
            }
        }
    }
    else
    {
        const double effectiveGateSteps = step.slide ? 4.0 : juce::jlimit (0.10, 3.95, static_cast<double> (step.gate * globalGateMult));
        const double noteLenSamples = juce::jmax (64.0, stepDurationSamples * effectiveGateSteps);

        for (int n : notesToTrigger)
        {
            sendMidiOut (trackIdx, juce::MidiMessage::noteOn (midiChan, n, static_cast<juce::uint8> (vel)), midiOut, sampleOffset);
            if (isPreviewTrack)
                previewSynth.noteOn (n, vel, isLegatoArrival);
            activeNotes.push_back ({ n, midiChan, noteLenSamples, step.slide, trackIdx });
            if (isPreviewTrack)
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
            sendMidiOut (trackIdx, juce::MidiMessage::noteOff (prev.channel, prev.midiNote), midiOut, sampleOffset + 1);
            if (isPreviewTrack)
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
        for (int t = 0; t < PulseForge::kNumTracks; ++t)
            lastTriggeredStepIndex[t] = -999999;
        lastExpectedNextBlockPpq = -1.0;
        internalPpqPosition = 0.0;
        return;
    }

    const double ppqPerSample = (hostBpm / 60.0) / currentSampleRate;

    // Detect Transport Start OR Ableton Loop Jump / Seek!
    const bool transportJustStarted = !wasPlayingLastBlock;
    const bool hostSeekedOrLooped   = hasHostPpq && (lastExpectedNextBlockPpq >= 0.0)
                                   && (std::abs (blockStartPpq - lastExpectedNextBlockPpq) > 0.015);

    if (transportJustStarted || hostSeekedOrLooped)
    {
        stopAllActiveNotes (midiMessages, 0);
        for (int t = 0; t < PulseForge::kNumTracks; ++t)
        {
            lastTriggeredStepIndex[t] = -999999;
            arpWalkIndex[t] = 0;
            arpDirectionSign[t] = 1;
        }
        if (!hasHostPpq && transportJustStarted)
            blockStartPpq = 0.0;
    }

    wasPlayingLastBlock = true;

    const auto& playbackSnap = audioSnapshots[static_cast<size_t>(activeSnapshotIndex.load (std::memory_order_acquire))];
    const int activeTrack = activeTrackIndex.load();

    // 3. Process active note-offs sample-by-sample
    for (int s = 0; s < numSamples; ++s)
    {
        for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
        {
            auto& an = activeNotes[static_cast<size_t>(i)];
            an.remainingSamples -= 1.0;
            if (an.remainingSamples <= 0.0 && !an.waitingForLegatoOverlap)
            {
                sendMidiOut (an.trackIndex, juce::MidiMessage::noteOff (an.channel, an.midiNote), midiMessages, s);
                if (an.trackIndex == activeTrack)
                    previewSynth.noteOff (an.midiNote);
                activeNotes.erase (activeNotes.begin() + i);
            }
        }

        for (int i = static_cast<int> (pendingSubTriggers.size()) - 1; i >= 0; --i)
        {
            auto& sub = pendingSubTriggers[static_cast<size_t>(i)];
            sub.delaySamples -= 1.0;
            if (sub.delaySamples <= 0.0)
            {
                sendMidiOut (sub.trackIndex, juce::MidiMessage::noteOn (sub.channel, sub.midiNote, static_cast<juce::uint8> (sub.velocity)),
                             midiMessages, s);
                if (sub.trackIndex == activeTrack)
                    previewSynth.noteOn (sub.midiNote, sub.velocity, false);
                activeNotes.push_back ({ sub.midiNote, sub.channel, sub.durationSamples, false, sub.trackIndex });
                pendingSubTriggers.erase (pendingSubTriggers.begin() + i);
            }
        }
    }

    // Calcolo della battuta musicale globale (8 battute totali, da 0 a 7) dal transport della DAW
    // In tempo 4/4 standard, ogni battuta equivale esattamente a 4.0 PPQ (quarti)
    const int calculatedBar = (blockStartPpq >= 0.0)
                            ? static_cast<int> (std::floor (blockStartPpq / 4.0)) % 8
                            : 0;
    currentTransportBar.store ((calculatedBar + 8) % 8);

    // 4. Advance all 3 tracks concurrently locked to PPQ
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const auto& trackSnap = playbackSnap.tracks[static_cast<size_t>(t)];
        if (trackSnap.muted)
            continue;

        double baseStepPpq = 0.25;
        switch (trackSnap.rateIdx)
        {
            case 0: baseStepPpq = 1.0;        break; // 1/4
            case 1: baseStepPpq = 0.5;        break; // 1/8
            case 2: baseStepPpq = 1.0 / 3.0;  break; // 1/8T
            case 3: baseStepPpq = 0.25;       break; // 1/16
            case 4: baseStepPpq = 1.0 / 6.0;  break; // 1/16T
            case 5: baseStepPpq = 0.125;      break; // 1/32
            default: baseStepPpq = 0.25;      break;
        }

        const double trackTempoMult = PulseForge::getTempoRatioMultiplier (trackSnap.tempoRatioIdx);
        const double stepLengthPpq = baseStepPpq / trackTempoMult;
        const double stepDurationSamples = stepLengthPpq / ppqPerSample;
        const double trackSwing = static_cast<double> (trackSnap.swing) * 0.01 * 0.42;

        for (int s = 0; s < numSamples; ++s)
        {
            const double currentPpq = blockStartPpq + static_cast<double> (s) * ppqPerSample;
            if (currentPpq >= 0.0)
            {
                const double rawStepFloat = currentPpq / stepLengthPpq;
                const int pairIndex = static_cast<int> (std::floor (rawStepFloat / 2.0));
                const double posInPair = rawStepFloat - static_cast<double> (pairIndex * 2);

                const int unmodStepIndex = (posInPair < (1.0 + trackSwing))
                                         ? (pairIndex * 2)
                                         : (pairIndex * 2 + 1);

                if (unmodStepIndex != lastTriggeredStepIndex[t])
                {
                    lastTriggeredStepIndex[t] = unmodStepIndex;
                    const int effectiveStepCount = juce::jmax (1, trackSnap.numSteps);
                    const int wrappedStep = ((unmodStepIndex % effectiveStepCount) + effectiveStepCount) % effectiveStepCount;
                    trackPlaySteps[static_cast<size_t>(t)].store (wrappedStep);
                    if (t == activeTrack)
                        currentPlayStep.store (wrappedStep);

                    // Aggiornamento dinamico Complexity alla battuta successiva senza click né note appese
                    if (wrappedStep == 0)
                    {
                        float pendingC = pendingComplexityUpdate[static_cast<size_t>(t)].exchange (-1.0f);
                        if (pendingC >= 0.0f)
                        {
                            auto& trk = tracks[static_cast<size_t>(t)];
                            PulseForge::StyleGenerator::GenerationParams gp;
                            gp.genre = trk.genre;
                            gp.role = trk.role;
                            gp.scale = static_cast<PulseForge::ScaleType> (static_cast<int> (apvts.getRawParameterValue ("scale")->load()));
                            gp.density = pendingC * 0.01f;
                            gp.numSteps = trk.numSteps;
                            PulseForge::StyleGenerator::generatePattern (trk.pattern, gp, trk.seed);
                            updatePlaybackSnapshot();
                        }
                    }

                    triggerStepEventsForTrack (t, wrappedStep, stepDurationSamples, s, midiMessages, playbackSnap);
                }
            }
        }
    }

    const double blockEndPpq = blockStartPpq + static_cast<double> (numSamples) * ppqPerSample;
    lastExpectedNextBlockPpq = blockEndPpq;
    internalPpqPosition      = blockEndPpq;

    // 5. Render internal virtual analog preview synth if enabled
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
        const bool harmOn = (apvts.getRawParameterValue ("harmonicContext") != nullptr)
                          ? (apvts.getRawParameterValue ("harmonicContext")->load() > 0.5f) : true;
        const int bar = (i / 16) % 8;
        const int baseNote = computeHarmonicStepMidiNote (activeTrackIndex.load(), i, step, rootMidi, scale,
                                                          bar, globalProgressionIndex.load(),
                                                          tracks[static_cast<size_t>(activeTrackIndex.load())].genre,
                                                          role, harmOn);
        const std::vector<int> stepNotes = PulseForge::buildChordVoicingNotes (baseNote, step, rootMidi, scale, role, liveTransposeSemitones);

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

juce::String PulseForgeAudioProcessor::getTrackTargetHardwareMidi (int trackIdx) const
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    const juce::ScopedLock sl (patternLock);
    return tracks[static_cast<size_t>(trackIdx)].targetHwDevice;
}

void PulseForgeAudioProcessor::setTrackTargetHardwareMidi (int trackIdx, const juce::String& devName)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    {
        const juce::ScopedLock sl (patternLock);
        tracks[static_cast<size_t>(trackIdx)].targetHwDevice = devName;
    }
    if (devName.isNotEmpty() && devName != "DAW Only" && devName != "DAW (VST3)")
    {
        ensureHardwareMidiOutOpen (devName);
    }
    syncTrackHardwareToOled (trackIdx);
}

void PulseForgeAudioProcessor::syncTrackHardwareToOled (int trackIdx)
{
    trackIdx = juce::jlimit (0, PulseForge::kNumTracks - 1, trackIdx);
    juce::String dev;
    int numSteps = 16;
    {
        const juce::ScopedLock sl (patternLock);
        dev = tracks[static_cast<size_t>(trackIdx)].targetHwDevice;
        numSteps = tracks[static_cast<size_t>(trackIdx)].numSteps;
    }

    if (dev.isEmpty() || dev == "DAW Only" || dev == "DAW (VST3)")
        dev = "DAW";
    else if (dev == "All USB Synths")
        dev = "ALL SYNTHS";

    // Clean up unnecessary noisy device substrings
    juce::String clean = dev;
    if (clean.endsWithIgnoreCase (" MIDI")) clean = clean.substring (0, clean.length() - 5).trim();
    if (clean.endsWithIgnoreCase (" USB"))  clean = clean.substring (0, clean.length() - 4).trim();

    juce::String header = clean;
    if (header.length() > 16)
        header = header.substring (0, 16);

    hardwareSerial.sendTrackName (trackIdx, header);
    hardwareSerial.sendTrackSteps (trackIdx, numSteps);

    const juce::String pfx = "t" + juce::String (trackIdx) + "_";
    if (auto* pG = apvts.getRawParameterValue (pfx + "genre"))
    {
        int gIdx = juce::jlimit (0, 7, static_cast<int> (std::round (pG->load())));
        const char* gNames[8] = {
            "CLASSIC HOUSE", "ACID 303", "ITALO DISCO", "FISHER",
            "P. KALKBRENNER", "UK GARAGE", "FRED AGAIN..", "BONOBO"
        };
        hardwareSerial.sendTrackGenre (trackIdx, gNames[gIdx]);
    }

    if (auto* pR = apvts.getRawParameterValue (pfx + "role"))
    {
        int rIdx = juce::jlimit (0, 6, static_cast<int> (std::round (pR->load())));
        const char* rNames[7] = {
            "Mono Bassline", "Acid TB-303", "Classic Arp", "Lead Hook",
            "Poly 7th Stab", "Harmony Chords", "Modular Ratchet"
        };
        hardwareSerial.sendTrackRole (trackIdx, rNames[rIdx]);
    }

    if (auto* pOct = apvts.getRawParameterValue (pfx + "baseOctave"))
    {
        hardwareSerial.sendParameter (trackIdx, "OCT", static_cast<int> (std::round (pOct->load())));
    }

    if (auto* pRat = apvts.getRawParameterValue (pfx + "tempoRatio"))
    {
        int ratIdx = juce::jlimit (0, 10, static_cast<int> (std::round (pRat->load())));
        hardwareSerial.sendParameter (trackIdx, "RATIDX", ratIdx);
    }
}

void PulseForgeAudioProcessor::syncAllTracksToHardware()
{
    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        syncTrackHardwareToOled (t);
    }
}

void PulseForgeAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (!hardwareSerial.isConnected())
        return;

    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const juce::String pfx = "t" + juce::String (t) + "_";
        if (parameterID.startsWith (pfx))
        {
            auto sub = parameterID.substring (pfx.length());
            if (sub == "density")
            {
                hardwareSerial.sendParameter (t, "DENS", static_cast<int> (std::round (newValue)));
            }
            else if (sub == "swing")
            {
                hardwareSerial.sendParameter (t, "SWING", static_cast<int> (std::round (newValue)));
            }
            else if (sub == "numSteps")
            {
                int sIdx = static_cast<int> (std::round (newValue));
                int st = (sIdx == 0) ? 4 : ((sIdx == 1) ? 8 : ((sIdx == 2) ? 16 : 32));
                hardwareSerial.sendTrackSteps (t, st);
            }
            else if (sub == "baseOctave")
            {
                hardwareSerial.sendParameter (t, "BOCT", static_cast<int> (std::round (newValue)));
            }
            else if (sub == "arpOctaves")
            {
                hardwareSerial.sendParameter (t, "AOCT", static_cast<int> (std::round (newValue)));
            }
            else if (sub == "tempoRatio")
            {
                hardwareSerial.sendParameter (t, "RATIDX", static_cast<int> (std::round (newValue)));
            }
            else if (sub == "genre")
            {
                int gIdx = juce::jlimit (0, 7, static_cast<int> (std::round (newValue)));
                const char* gNames[8] = {
                    "CLASSIC HOUSE", "ACID 303", "ITALO DISCO", "FISHER",
                    "P. KALKBRENNER", "UK GARAGE", "FRED AGAIN..", "BONOBO"
                };
                hardwareSerial.sendTrackGenre (t, gNames[gIdx]);
            }
            else if (sub == "role")
            {
                int rIdx = juce::jlimit (0, 6, static_cast<int> (std::round (newValue)));
                const char* rNames[7] = {
                    "Mono Bassline", "Acid TB-303", "Classic Arp", "Lead Hook",
                    "Poly 7th Stab", "Harmony Chords", "Modular Ratchet"
                };
                hardwareSerial.sendTrackRole (t, rNames[rIdx]);
            }
            break;
        }
    }
}

void PulseForgeAudioProcessor::ensureHardwareMidiOutOpen (const juce::String& deviceName)
{
    if (deviceName == "All USB Synths")
    {
        setAllHardwareMidiOuts (true);
        return;
    }
    setHardwareMidiOutEnabled (deviceName, true);
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

    xml->setAttribute ("activeTrack", activeTrackIndex.load());
    {
        const juce::ScopedLock msl (midiOutLock);
        xml->setAttribute ("hwMidiOuts", enabledHardwareMidiNames.joinIntoString (";;"));
    }

    for (int t = 0; t < PulseForge::kNumTracks; ++t)
    {
        const auto& trk = tracks[static_cast<size_t>(t)];
        auto* trackXml = xml->createNewChildElement ("Track");
        trackXml->setAttribute ("index", t);
        trackXml->setAttribute ("name", trk.name);
        trackXml->setAttribute ("midiChannel", trk.midiChannel);
        trackXml->setAttribute ("muted", trk.muted);
        trackXml->setAttribute ("genre", static_cast<int> (trk.genre));
        trackXml->setAttribute ("role", static_cast<int> (trk.role));
        trackXml->setAttribute ("mode", static_cast<int> (trk.mode));
        trackXml->setAttribute ("arpDir", static_cast<int> (trk.arpDir));
        trackXml->setAttribute ("arpOctaves", trk.arpOctaves);
        trackXml->setAttribute ("baseOctave", trk.baseOctave);
        trackXml->setAttribute ("rate", trk.rateIdx);
        trackXml->setAttribute ("numSteps", trk.numSteps);
        trackXml->setAttribute ("seed", static_cast<int> (trk.seed));
        trackXml->setAttribute ("density", static_cast<double> (trk.density));
        trackXml->setAttribute ("sync", static_cast<double> (trk.syncopation));
        trackXml->setAttribute ("spice", static_cast<double> (trk.spice));
        trackXml->setAttribute ("range", static_cast<double> (trk.pitchRange));
        trackXml->setAttribute ("swing", static_cast<double> (trk.swing));
        trackXml->setAttribute ("ratio", trk.tempoRatioIdx);
        trackXml->setAttribute ("expanded", trk.isExpanded);
        trackXml->setAttribute ("hwOut", trk.targetHwDevice);

        for (int i = 0; i < PulseForge::kMaxSteps; ++i)
        {
            const auto& st = trk.pattern.steps[static_cast<size_t>(i)];
            auto* stepXml = trackXml->createNewChildElement ("Step");
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
        activeTrackIndex.store (juce::jlimit (0, PulseForge::kNumTracks - 1,
                                              xmlState->getIntAttribute ("activeTrack", xmlState->getIntAttribute ("activeSlot", 0))));

        const juce::String hwMidiStr = xmlState->getStringAttribute ("hwMidiOuts", xmlState->getStringAttribute ("hwMidiOut", ""));
        if (hwMidiStr.isNotEmpty() && hwMidiStr != "None (DAW MIDI Out)")
        {
            auto tokens = juce::StringArray::fromTokens (hwMidiStr, ";;", "");
            for (const auto& dev : tokens)
                if (dev.isNotEmpty()) setHardwareMidiOutEnabled (dev, true);
        }

        for (auto* trackXml : xmlState->getChildWithTagNameIterator ("Track"))
        {
            const int t = juce::jlimit (0, PulseForge::kNumTracks - 1, trackXml->getIntAttribute ("index", 0));
            auto& trk = tracks[static_cast<size_t>(t)];
            trk.name        = trackXml->getStringAttribute ("name", trk.name);
            trk.midiChannel = trackXml->getIntAttribute ("midiChannel", t + 1);
            trk.muted       = trackXml->getBoolAttribute ("muted", false);
            trk.genre       = static_cast<PulseForge::GenreStyle> (trackXml->getIntAttribute ("genre", static_cast<int> (trk.genre)));
            trk.role        = static_cast<PulseForge::InstrumentRole> (trackXml->getIntAttribute ("role", static_cast<int> (trk.role)));
            trk.mode        = static_cast<PulseForge::EngineMode> (trackXml->getIntAttribute ("mode", static_cast<int> (trk.mode)));
            trk.arpDir      = static_cast<PulseForge::ArpDirection> (trackXml->getIntAttribute ("arpDir", static_cast<int> (trk.arpDir)));
            trk.arpOctaves  = trackXml->getIntAttribute ("arpOctaves", trk.arpOctaves);
            trk.baseOctave  = trackXml->getIntAttribute ("baseOctave", trk.baseOctave);
            trk.rateIdx     = trackXml->getIntAttribute ("rate", trk.rateIdx);
            trk.numSteps    = trackXml->getIntAttribute ("numSteps", 16);
            trk.pattern.numSteps = trk.numSteps;
            trk.seed        = static_cast<uint32_t> (trackXml->getIntAttribute ("seed", static_cast<int> (101 + t * 100)));
            trk.density     = static_cast<float> (trackXml->getDoubleAttribute ("density", trk.density));
            trk.syncopation = static_cast<float> (trackXml->getDoubleAttribute ("sync", trk.syncopation));
            trk.spice       = static_cast<float> (trackXml->getDoubleAttribute ("spice", trk.spice));
            trk.pitchRange  = static_cast<float> (trackXml->getDoubleAttribute ("range", trk.pitchRange));
            trk.swing       = static_cast<float> (trackXml->getDoubleAttribute ("swing", trk.swing));
            trk.tempoRatioIdx = trackXml->getIntAttribute ("ratio", trk.tempoRatioIdx);
            trk.isExpanded  = trackXml->getBoolAttribute ("expanded", false);
            trk.targetHwDevice = trackXml->getStringAttribute ("hwOut", "DAW Only");
            if (trk.targetHwDevice != "DAW Only" && trk.targetHwDevice != "DAW (VST3)")
                ensureHardwareMidiOutOpen (trk.targetHwDevice);

            for (auto* stepXml : trackXml->getChildWithTagNameIterator ("Step"))
            {
                const int idx = stepXml->getIntAttribute ("idx", -1);
                if (idx >= 0 && idx < PulseForge::kMaxSteps)
                {
                    auto& st = trk.pattern.steps[static_cast<size_t>(idx)];
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
        syncAllTracksToHardware();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PulseForgeAudioProcessor();
}
