#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>

namespace PulseForge
{

static constexpr int kMaxSteps = 16;
static constexpr int kNumPatternSlots = 4;
static constexpr int kNumTracks = 3;

enum class EngineMode : int
{
    Sequencer = 0,
    Arpeggiator = 1
};

enum class GenreStyle : int
{
    ClassicHouse = 0,     // 90s Chicago / Deep & Jazzy House syncopated grooves
    AcidTechno,           // TB-303 style hypnotic 16ths with slides & accents
    ItaloSynthwave,       // 80s Moroder / Juno rolling octaves & melodic hooks
    BerlinSchool,         // 70s Tangerine Dream / Krautrock ostinatos & ratchets
    MelodicTechno,        // Modern Afterlife / PML Melodic Techno arps & basslines
    DetroitElectro,       // Kraftwerk / Drexciya / Bass House syncopation
    FunkBoogie,           // Funky House / Minimoog slap-funk & pentatonic walkdowns
    EuclideanIDM,         // Aphex / Buchla algorithmic polyrhythms & modal leaps
    Count
};

enum class InstrumentRole : int
{
    MonoBass = 0,         // SH-101 / Moog solid low-end groove
    AcidLine,             // TB-303 squelchy glide & accent pattern
    ClassicArp,           // Juno-60 / Jupiter multi-octave arpeggio
    LeadHook,             // Prophet / Odyssey expressive synth lead
    PolyChordStab,        // House / Dub Techno multi-note chord stabs
    ModularRatchet,       // Berlin / Buchla percussive ratchet sequence
    Count
};

enum class ArpDirection : int
{
    Up = 0,
    Down,
    UpDown,
    Converge,
    PinkyPedal,           // Alternates ascending notes with the top/root pedal note
    RandomWalk,
    ChordTrigger,         // Triggers full chord voicing on active steps
    Count
};

enum class ScaleType : int
{
    MinorNatural = 0,
    Dorian,               // Classic House / Funk minor with raised 6th
    Phrygian,             // Dark Techno / Acid with flat 2nd
    PhrygianDominant,     // Exotic / Melodic Techno (flat 2nd + major 3rd)
    MinorPentatonic,      // Classic Bass / Deep House
    MajorNatural,         // Italo / Pop / Synthwave
    Mixolydian,           // Electro / Funk / Boogie
    HarmonicMinor,        // Dramatic / Melodic Techno
    Lydian,               // Dreamy Berlin School / Ambient
    Chromatic,
    Count
};

// Fractions and multiples of the Ableton Live Project BPM
inline double getTempoRatioMultiplier (int ratioIndex)
{
    static const double multipliers[] = {
        0.25,           // 0: 1/4x (Quarter Speed)
        1.0 / 3.0,      // 1: 1/3x (Triplet Slow)
        0.50,           // 2: 1/2x (Half Time)
        2.0 / 3.0,      // 3: 2/3x (Dotted Slow)
        0.75,           // 4: 3/4x (Polymetric 3:4)
        1.00,           // 5: 1/1x (Exact Project BPM - Default)
        4.0 / 3.0,      // 6: 4/3x (Polymetric 4:3)
        1.50,           // 7: 3/2x (Dotted Fast)
        2.00,           // 8: 2/1x (Double Time)
        3.00,           // 9: 3/1x (Triple Speed)
        4.00            // 10: 4/1x (Quadruple Speed)
    };
    ratioIndex = juce::jlimit (0, 10, ratioIndex);
    return multipliers[ratioIndex];
}

inline juce::String getTempoRatioName (int ratioIndex)
{
    static const char* names[] = {
        "1/4x", "1/3x", "1/2x", "2/3x", "3/4x",
        "1/1x (Sync)",
        "4/3x", "3/2x", "2/1x", "3/1x", "4/1x"
    };
    ratioIndex = juce::jlimit (0, 10, ratioIndex);
    return names[ratioIndex];
}

struct StepData
{
    bool active = true;
    int degreeOffset = 0;     // Scale degree offset
    int semitoneOffset = 0;   // Semitone offset from root (-12..+24)
    int octaveShift = 0;      // -2, -1, 0, +1, +2
    int velocity = 100;       // 1..127
    float gate = 0.75f;       // 0.10 .. 4.00 (in step units! >1.0 sustains across rest steps)
    bool accent = false;      // Boosts velocity & filter CC
    bool slide = false;       // Overlaps MIDI note with next active step for analog portamento
    int ratchet = 1;          // 1, 2, 3, or 4 sub-step repeats
    int probability = 100;    // 0..100%
    int ccValue = 64;         // 0..127 step modulation lane (e.g. Filter Cutoff CC74)
    int chordType = 0;        // 0 = Single Note, 1 = Power 5th, 2 = Triad, 3 = 7th, 4 = 9th Stab
};

struct PatternBank
{
    std::array<StepData, kMaxSteps> steps;
    int numSteps = 16;
    juce::String name = "Init Pattern";

    PatternBank()
    {
        for (int i = 0; i < kMaxSteps; ++i)
        {
            steps[static_cast<size_t>(i)].active = (i == 0 || i == 3 || i == 6 || i == 8 || i == 11 || i == 14);
            steps[static_cast<size_t>(i)].semitoneOffset = (i == 6) ? 3 : ((i == 14) ? -2 : 0);
            steps[static_cast<size_t>(i)].octaveShift = (i == 11) ? 1 : 0;
            steps[static_cast<size_t>(i)].velocity = (i % 4 == 0) ? 115 : 92;
            steps[static_cast<size_t>(i)].gate = (i == 0 || i == 3 || i == 8 || i == 11) ? 1.8f : 0.85f;
            steps[static_cast<size_t>(i)].accent = (i == 0 || i == 6 || i == 14);
            steps[static_cast<size_t>(i)].slide = false;
            steps[static_cast<size_t>(i)].ratchet = 1;
            steps[static_cast<size_t>(i)].probability = 100;
            steps[static_cast<size_t>(i)].ccValue = (i % 4 == 0) ? 105 : 68;
            steps[static_cast<size_t>(i)].chordType = 0;
        }
    }
};

struct TrackData
{
    PatternBank pattern;
    int midiChannel = 1;
    bool muted = false;
    InstrumentRole role = InstrumentRole::MonoBass;
    EngineMode mode = EngineMode::Sequencer;
    ArpDirection arpDir = ArpDirection::Up;
    int arpOctaves = 2;
    int baseOctave = 1;
    int rateIdx = 3; // 1/16
    float density = 65.0f;
    float syncopation = 55.0f;
    float spice = 45.0f;
    float pitchRange = 55.0f;
    int numSteps = 16;
    juce::String name = "SEQ 1";
};

inline const std::vector<int>& getScaleIntervals (ScaleType scale)
{
    static const std::vector<int> minorNat     = { 0, 2, 3, 5, 7, 8, 10 };
    static const std::vector<int> dorian       = { 0, 2, 3, 5, 7, 9, 10 };
    static const std::vector<int> phrygian     = { 0, 1, 3, 5, 7, 8, 10 };
    static const std::vector<int> phrygDom     = { 0, 1, 4, 5, 7, 8, 10 };
    static const std::vector<int> minPent      = { 0, 3, 5, 7, 10 };
    static const std::vector<int> majorNat     = { 0, 2, 4, 5, 7, 9, 11 };
    static const std::vector<int> mixolydian   = { 0, 2, 4, 5, 7, 9, 10 };
    static const std::vector<int> harmMinor    = { 0, 2, 3, 5, 7, 8, 11 };
    static const std::vector<int> lydian       = { 0, 2, 4, 6, 7, 9, 11 };
    static const std::vector<int> chromatic    = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    switch (scale)
    {
        case ScaleType::MinorNatural:     return minorNat;
        case ScaleType::Dorian:           return dorian;
        case ScaleType::Phrygian:         return phrygian;
        case ScaleType::PhrygianDominant: return phrygDom;
        case ScaleType::MinorPentatonic:  return minPent;
        case ScaleType::MajorNatural:     return majorNat;
        case ScaleType::Mixolydian:       return mixolydian;
        case ScaleType::HarmonicMinor:    return harmMinor;
        case ScaleType::Lydian:           return lydian;
        case ScaleType::Chromatic:        return chromatic;
        default:                          return minorNat;
    }
}

inline int quantizeSemitoneToScale (int semitone, ScaleType scale)
{
    const auto& intervals = getScaleIntervals (scale);
    if (intervals.empty())
        return semitone;

    int oct = (semitone >= 0) ? (semitone / 12) : ((semitone - 11) / 12);
    int rem = ((semitone % 12) + 12) % 12;

    int best = intervals[0];
    int minDiff = 99;
    for (int iv : intervals)
    {
        int diff = std::abs (rem - iv);
        if (diff < minDiff)
        {
            minDiff = diff;
            best = iv;
        }
    }
    return oct * 12 + best;
}

inline int scaleDegreeToSemitone (int degree, ScaleType scale)
{
    const auto& intervals = getScaleIntervals (scale);
    const int n = static_cast<int> (intervals.size());
    if (n == 0) return 0;

    int oct = 0;
    while (degree < 0) { degree += n; --oct; }
    oct += degree / n;
    int idx = degree % n;
    return oct * 12 + intervals[static_cast<size_t>(idx)];
}

inline juce::String getMidiNoteName (int midiNote)
{
    static const char* names[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    midiNote = juce::jlimit (0, 127, midiNote);
    int n = midiNote % 12;
    int oct = (midiNote / 12) - 2; // C3 = 60 in Ableton convention
    return juce::String (names[n]) + juce::String (oct);
}

inline juce::String getGenreDescription (GenreStyle genre)
{
    switch (genre)
    {
        case GenreStyle::ClassicHouse:
            return "Deep & Jazzy House (SmashUp/Chicago): Groove Tresillo & Off-beat, fondamentale solida con risposte di 3a/5a/7a e note basse tenute.";
        case GenreStyle::AcidTechno:
            return "TB-303 Acid & Techno: Pedale di 16esimi sulla tonica alternato a b2/b3/5a/Ottava con Slide (legato analogico) e Accenti filtro.";
        case GenreStyle::ItaloSynthwave:
            return "Italo Disco & Synthwave (Moroder/DMS): Ottave pulsanti (Root-Oct-5th), arpeggi in 16esimi simmetrici A-A-A-B e tiro dritto.";
        case GenreStyle::BerlinSchool:
            return "Berlin School (Tangerine Dream/Moog): Ostinati ipnotici a contrappunto (pedale basso + melodia modale) con Ratchets 2x-4x.";
        case GenreStyle::MelodicTechno:
            return "Melodic Techno (PML Afterlife/Bodzin): Groove XX.XXX.XXX.XXX.X con arpeggi contrappuntistici tra registro medio e pedale a -12st.";
        case GenreStyle::DetroitElectro:
            return "Electro & Bass House (DABRO/Detroit): Riff sincopati Call & Response, walkdown cromatici/frigi e salti d'ottava percussivi.";
        case GenreStyle::FunkBoogie:
            return "Funky House & Boogie (Nano Loops): 'The One' lungo sul primo quarto seguito da walkdown pentatonici (12->10->7->3->0) con swing.";
        case GenreStyle::EuclideanIDM:
            return "Organic / Euclidean Electronica: Poliritmie di Bjorklund con motivi ripetuti di 3 o 6 note su griglie di 16 step.";
        default:
            return "";
    }
}

} // namespace PulseForge
