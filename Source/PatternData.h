#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <cmath>

namespace PulseForge
{

static constexpr int kMaxSteps = 32;
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
    HarmonyChords,        // Musical chord progressions (held diatonic chords, beautiful voice leading)
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

//==============================================================================
// Ruolo Armonico Astratto della Nota (Indipendente dall'Accordo Assoluto)
//==============================================================================
enum class HarmonicFunction : int
{
    Root = 0,       // Fondamentale dell'accordo corrente
    Third = 1,      // 3a dell'accordo (minore o maggiore)
    Fifth = 2,      // 5a giusta
    Seventh = 3,    // 7a dell'accordo
    Ninth = 4,      // 9a estesa / colore
    Eleventh = 5,   // 11a / 4a
    OctavePop = 6,  // Ottava della fondamentale (+12)
    Passing = 7,    // Nota di passaggio diatonica
    Tension = 8     // Nota di tensione o sospensione (es. b6, sus)
};

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
    HarmonicFunction harmonicFunc = HarmonicFunction::Root; // Ruolo armonico contrappuntistico
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
            const int mod16 = i % 16;
            steps[static_cast<size_t>(i)].active = (mod16 == 0 || mod16 == 3 || mod16 == 6 || mod16 == 8 || mod16 == 11 || mod16 == 14);
            steps[static_cast<size_t>(i)].semitoneOffset = (mod16 == 6) ? 3 : ((mod16 == 14) ? -2 : 0);
            steps[static_cast<size_t>(i)].octaveShift = (mod16 == 11) ? 1 : 0;
            steps[static_cast<size_t>(i)].velocity = (i % 4 == 0) ? 115 : 92;
            steps[static_cast<size_t>(i)].gate = (mod16 == 0 || mod16 == 3 || mod16 == 8 || mod16 == 11) ? 1.8f : 0.85f;
            steps[static_cast<size_t>(i)].accent = (mod16 == 0 || mod16 == 6 || mod16 == 14);
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
    GenreStyle genre = GenreStyle::ClassicHouse;
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
    float swing = 20.0f;
    int tempoRatioIdx = 5; // 1/1x (Sync)
    int numSteps = 16;
    uint32_t seed = 101;
    bool isExpanded = false;
    juce::String targetHwDevice = "DAW Only";
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

inline int semitoneToScaleDegree (int semitone, ScaleType scale)
{
    const auto& intervals = getScaleIntervals (scale);
    const int n = static_cast<int> (intervals.size());
    if (n == 0) return 0;

    int oct = (semitone >= 0) ? (semitone / 12) : ((semitone - 11) / 12);
    int rem = ((semitone % 12) + 12) % 12;

    int bestDeg = 0;
    int minDiff = 99;
    for (int i = 0; i < n; ++i)
    {
        int diff = std::abs (rem - intervals[static_cast<size_t>(i)]);
        if (diff < minDiff)
        {
            minDiff = diff;
            bestDeg = i;
        }
    }
    return oct * n + bestDeg;
}

inline std::vector<int> buildChordVoicingNotes (int baseNote, const StepData& step,
                                               int rootMidi, ScaleType scale,
                                               InstrumentRole role, int liveTranspose = 0)
{
    std::vector<int> notes;
    notes.push_back (baseNote);

    if (role != InstrumentRole::PolyChordStab && role != InstrumentRole::HarmonyChords && step.chordType == 0)
        return notes;

    const int deg = semitoneToScaleDegree (step.semitoneOffset, scale);
    const int octBase = rootMidi + (step.octaveShift * 12) + liveTranspose;

    const int r = scaleDegreeToSemitone (deg, scale);
    const int rootPitch    = octBase + r;
    const int fourthPitch  = octBase + scaleDegreeToSemitone (deg + 3, scale);
    const int fifthPitch   = octBase + scaleDegreeToSemitone (deg + 4, scale);
    const int seventhPitch = octBase + scaleDegreeToSemitone (deg + 6, scale);
    const int thirdPitch   = octBase + scaleDegreeToSemitone (deg + 2, scale);
    const int ninthPitch   = octBase + scaleDegreeToSemitone (deg + 8, scale);
    const int eleventhPitch= octBase + scaleDegreeToSemitone (deg + 10, scale);
    const int thirteenthPitch = octBase + scaleDegreeToSemitone (deg + 12, scale);

    const int cType = step.chordType;

    if (cType == 1)
    {
        // Power 5th Open: Root + Fifth + Octave
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, rootPitch + 12));
    }
    else if (cType == 2)
    {
        // Sus2 / Add9 Modern: Root + 5th + 9th + 3rd (octave up)
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, ninthPitch));
        notes.push_back (juce::jlimit (12, 115, thirdPitch + 12));
    }
    else if (cType == 3)
    {
        // Drop-2 7th: Root in bass + 5th + 7th (mid) + 3rd (octave up)
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, seventhPitch));
        notes.push_back (juce::jlimit (12, 115, thirdPitch + 12));
    }
    else if (cType == 5)
    {
        // Ambient 11th Modal Voicing (PML / Melodic Techno / Atmospheric):
        // Root + 5th + 7th + 9th + 11th shimmer
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, seventhPitch));
        notes.push_back (juce::jlimit (12, 115, ninthPitch));
        notes.push_back (juce::jlimit (12, 115, eleventhPitch));
    }
    else if (cType == 6)
    {
        // Soulful 6/9 / 13th Voicing (Steely Dan / Neo-Soul / French House):
        // Root + 5th + 9th + 3rd (octave up) + 13th (octave up)
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, ninthPitch));
        notes.push_back (juce::jlimit (12, 115, thirdPitch + 12));
        notes.push_back (juce::jlimit (12, 115, thirteenthPitch));
    }
    else if (cType == 7)
    {
        // Quartal Modern Minimal (Techno / Berlin / IDM):
        // Stack of 4ths: Root + 4th + 7th + 10th(3rd+oct)
        notes.push_back (juce::jlimit (12, 115, fourthPitch));
        notes.push_back (juce::jlimit (12, 115, seventhPitch));
        notes.push_back (juce::jlimit (12, 115, thirdPitch + 12));
    }
    else if (cType == 8)
    {
        // 9sus4 Gospel Tension: Root + 4th + 5th + 7th + 9th
        notes.push_back (juce::jlimit (12, 115, fourthPitch));
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, seventhPitch));
        notes.push_back (juce::jlimit (12, 115, ninthPitch));
    }
    else
    {
        // Default (cType == 4 or 0 on HarmonyChords / PolyChordStab):
        // Lush Neo-Soul / Deep House Open 9th Voicing:
        // Root + 5th + 9th + 3rd (octave up) + 7th (octave up)
        notes.push_back (juce::jlimit (12, 115, fifthPitch));
        notes.push_back (juce::jlimit (12, 115, ninthPitch));
        notes.push_back (juce::jlimit (12, 115, thirdPitch + 12));
        notes.push_back (juce::jlimit (12, 115, seventhPitch + 12));
    }

    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    return notes;
}

inline juce::String getChordBadgeName (int rootMidi, const StepData& step, ScaleType scale, InstrumentRole role)
{
    static const char* noteNames[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const int baseNote = rootMidi + quantizeSemitoneToScale (step.semitoneOffset, scale) + (step.octaveShift * 12);
    const int rootIdx = ((baseNote % 12) + 12) % 12;
    const juce::String rName = noteNames[rootIdx];

    const int deg = semitoneToScaleDegree (step.semitoneOffset, scale);
    const int r = scaleDegreeToSemitone (deg, scale);
    const int thirdInterval   = scaleDegreeToSemitone (deg + 2, scale) - r;
    const int fifthInterval   = scaleDegreeToSemitone (deg + 4, scale) - r;
    const int seventhInterval = scaleDegreeToSemitone (deg + 6, scale) - r;

    const bool isMinor = (thirdInterval == 3);
    const bool isDim = (isMinor && fifthInterval == 6);

    juce::String suffix = "m9";
    const int cType = step.chordType;

    if (cType == 1)
    {
        suffix = "5";
    }
    else if (cType == 2)
    {
        suffix = isMinor ? "m(add9)" : "add9";
    }
    else if (cType == 3)
    {
        if (isDim) suffix = "m7b5";
        else suffix = isMinor ? "m7" : (seventhInterval == 11 ? "maj7" : "7");
    }
    else if (cType == 5)
    {
        suffix = isMinor ? "m11" : (seventhInterval == 11 ? "maj11" : "11");
    }
    else if (cType == 6)
    {
        suffix = isMinor ? "m6/9" : "6/9";
    }
    else if (cType == 7)
    {
        suffix = "sus4";
    }
    else if (cType == 8)
    {
        suffix = "9sus4";
    }
    else
    {
        // 4 or default
        if (isDim) suffix = "m7b5";
        else if (isMinor) suffix = "m9";
        else suffix = (seventhInterval == 11) ? "maj9" : "9";
    }

    return rName + suffix;
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
            return "Classic House: Groove sincopato sui levare (offbeat), fondamentale solida con risposte di 3a/5a e respiro per la cassa.";
        case GenreStyle::AcidTechno:
            return "Acid 303 (Reference): Pedale di 16esimi sulla tonica alternato a b2/b3/5a/Ottava con Slide legato analogico e Accenti filtro.";
        case GenreStyle::ItaloSynthwave:
            return "Italo / EDM: Ottave pulsanti Moroder (Root-Oct-5th), arpeggi simmetrici A-A-A-B e tiro dritto sui quarti.";
        case GenreStyle::BerlinSchool:
            return "Fisher (Tech House): Rolling groove a coppie di semicrome, pause strategiche per la cassa, ghost percussive e pop d'ottava a fine battuta.";
        case GenreStyle::MelodicTechno:
            return "Paul Kalkbrenner (Melodic Techno): Arpeggi ipnotici in semicrome, minore, evoluzione lenta e contorno melodico morbido.";
        case GenreStyle::DetroitElectro:
            return "UK Garage / Detroit (Disclosure): 2-step sincopato, swing 56-58%, ghost notes e slide controllati.";
        case GenreStyle::FunkBoogie:
            return "Fred again.. (Emotional UK): Motivo intimo di 3-4 note ripetuto, micro-variazioni di dinamica e sospensioni.";
        case GenreStyle::EuclideanIDM:
            return "Bonobo (Organic Downtempo): Accordi estesi, timing umano, dinamica ampia e pause ampie.";
        default:
            return "";
    }
}

} // namespace PulseForge
