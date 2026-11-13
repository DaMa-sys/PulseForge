#pragma once
#include "PatternData.h"
#include "GenreProfiles.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace PulseForge
{

//==============================================================================
// Configurazione Range di Complessità per Profilo
//==============================================================================
struct ProfileComplexityRange
{
    float floor;    // Minimo (verso 0: scheletrico)
    float ceiling;  // Massimo (verso 100: ricco ed evoluto)
};

inline ProfileComplexityRange getProfileComplexityRange (GenreStyle genre)
{
    switch (genre)
    {
        case GenreStyle::BerlinSchool:   // Fisher Tech House
            return { 0.0f, 100.0f };
        case GenreStyle::MelodicTechno:  // Paul Kalkbrenner
            return { 0.0f, 100.0f };
        case GenreStyle::DetroitElectro: // UK Garage / Disclosure
            return { 0.0f, 100.0f };
        case GenreStyle::FunkBoogie:     // Fred again..
            return { 0.0f, 100.0f };
        case GenreStyle::EuclideanIDM:   // Bonobo
            return { 0.0f, 100.0f };
        case GenreStyle::ClassicHouse:
            return { 0.0f, 100.0f };
        case GenreStyle::ItaloSynthwave:
            return { 0.0f, 100.0f };
        case GenreStyle::AcidTechno:     // 303: Mantiene semantica originale
        default:
            return { 0.0f, 100.0f };
    }
}

//==============================================================================
// Definizione Evento Candidato a Strati (8 Battute = 128 Step)
//==============================================================================
struct LayeredCandidateEvent
{
    int bar = 0;              // 0..7
    int stepInBar = 0;        // 0..15
    int globalStep = 0;       // 0..127
    float activationThreshold = 0.0f; // 0..100: attivo se complexity >= activationThreshold

    int semitone = 0;         // Offset semitonale
    int octaveShift = 0;      // 0 o +1
    int baseVelocity = 90;    // Velocity base
    float baseGate = 0.50f;   // Durata base

    bool isGhost = false;     // Se è ghost percussiva
    bool isOctavePop = false; // Se è pop d'ottava
    int ratchet = 1;          // 1, 2, 3 o 4
    bool slide = false;       // Portamento slide per AcidLine
    int chordType = 0;        // 0 = single note, 1 = minor7/stab

    float accentThreshold = 101.0f; // Se complexity >= accentThreshold -> accent = true
    int accentVelocity = 118;
    float accentGate = 0.70f;
    HarmonicFunction harmonicFunc = HarmonicFunction::Root;
};

//==============================================================================
// Palette di Pattern Ritmici Syncopati & Tematici per i Ruoli Non-Bass (8 Maschere ciascuno)
//==============================================================================
static const char* const kRoleRhythmsLeadHook[8] = {
    "X..X..X...X.X...", // 0: Syncopated vocal hook (0, 3, 6, 10, 12)
    ".X..X..X..X...X.", // 1: Delayed push (1, 4, 7, 10, 14)
    "X.X...X..X..X.X.", // 2: Call and quick response (0, 2, 6, 9, 12, 14)
    "..X..X...X..X..X", // 3: Offbeat vocal chop (2, 5, 9, 12, 15)
    "X...X...X.X...X.", // 4: Driving 4-bar lead theme (0, 4, 8, 10, 14)
    ".X.X...X...X.X..", // 5: Skitter chop (1, 3, 7, 11, 13)
    "X..X.X...X..X...", // 6: Funky hook (0, 3, 5, 9, 12)
    "..X...X.X...X.X."  // 7: Anticipated drop (2, 6, 8, 12, 14)
};

static const char* const kRoleRhythmsChordStab[8] = {
    "..X...X...X...X.", // 0: Classic Offbeat stabs (2, 6, 10, 14)
    "..X..X..X..X..X.", // 1: Double-syncopated stabs (2, 5, 8, 11, 14)
    "X..X..X.X..X..X.", // 2: Tresillo 3+3+2 chord groove (0, 3, 6, 8, 11, 14)
    "..X.X...X.X...X.", // 3: Dub house double-stab (2, 4, 8, 10, 14)
    "....X.......X...", // 4: Sparse deep stabs on beat 2 & 4 (4, 12)
    "..X...X...X..X.X", // 5: Offbeat with turnaround push (2, 6, 10, 13, 15)
    "X...X...X...X...", // 6: Straight on-beat 4-on-the-floor stabs (0, 4, 8, 12)
    ".X..X...X..X..X."  // 7: Syncopated Garage stabs (1, 4, 8, 11, 14)
};

static const char* const kRoleRhythmsModularRatchet[8] = {
    "..X...X...X...rr", // 0: Offbeats with double ratchet turnaround (2, 6, 10, 14-r, 15-r)
    "X...X...X.r...rr", // 1: On-beat pulse with mid-bar & end ratchets (0, 4, 8, 10-r, 14-r, 15-r)
    "..r...X...r...r.", // 2: Kinetic stutter bursts (2-r, 6, 10-r, 14-r)
    "X..r..X.X..r..X.", // 3: Polyrhythmic ratchet accents (0, 3-r, 6, 8, 11-r, 14)
    "....X...r...X.rr", // 4: Sparse glitch with rolling tail (4, 8-r, 12, 14-r, 15-r)
    "..X.r...X.r...X.", // 5: Double-hit ratchet flutter (2, 4-r, 8, 10-r, 14)
    "r...X...r...X.r.", // 6: Downbeat ratchet into solid stab (0-r, 4, 8-r, 12, 14-r)
    "X.r.X.r.X.r.X.rr"  // 7: High-energy full ratchet groove (0, 2-r, 4, 6-r, 8, 10-r, 12, 14-r, 15-r)
};

static const char* const kRoleRhythmsAcidLine[8] = {
    "X.XX.XX.X.XX.XX.", // 0: Classic 303 rolling gallop
    "XX.X.XX.XX.X.XX.", // 1: Hard driving Acid shuffle
    "X..X.X.XX..X.X.X", // 2: Tresillo syncopated acid hook
    "XXXX.XXXXXX.XXXX", // 3: Near-continuous 16th acid wall
    "X.X.X.X.XX.X.X.X", // 4: Pumping acid offbeat line
    ".XX.XX.XX.XX.XX.", // 5: Inverted syncopated acid groove
    "X..XX..XX..XX..X", // 6: 3-step polymetric acid motif
    "XX..XX..XXXX..XX"  // 7: Heavy acid punch with double-stabs
};

static const char* const kRoleRhythmsClassicArp[8] = {
    "XXXXXXXXXXXXXXXX", // 0: Relentless straight 16ths
    "X.X.X.X.X.X.X.X.", // 1: Hypnotic 8th-note arp
    "X..X..X.X..X..X.", // 2: 3+3+2 Melodic arpeggio
    ".X.X.X.X.X.X.X.X", // 3: Pure offbeat arpeggiated sparkle
    "XX.XX.XX.XX.XX.X", // 4: Galloping 16th arpeggio
    "X...X.X.X...X.X.", // 5: Breathing dynamic arp
    "X.XXX.XXX.XXX.XX", // 6: Driving Tangerine / Moroder cascade
    "..XX..XX..XX..XX"  // 7: Syncopated double-pluck arp
};

static const char* const kRoleRhythmsHarmonyChords[4] = {
    "X...............", // 0: Whole note pad chord (step 0)
    "X.......X.......", // 1: Half note chord progression (step 0, 8)
    "X.....X.........", // 2: Pushed chord syncopation (step 0, 6)
    "..X.......X....."  // 3: Offbeat swell chord (step 2, 10)
};

//==============================================================================
// Helper Universale per Iniettare Eventi Candidati Ritmici Dinamici nei Ruoli Non-Bass
//==============================================================================
template <typename QuantizeFn>
inline void appendRoleCandidates (std::vector<LayeredCandidateEvent>& events,
                                  InstrumentRole role,
                                  uint32_t seed,
                                  int bar,
                                  int barRoot,
                                  bool isResponse,
                                  bool isTension,
                                  bool isFillBar,
                                  QuantizeFn&& quantize)
{
    // Funzione hash deterministica per battuta e pattern
    auto hashVal = [seed] (uint32_t salt) -> uint32_t
    {
        uint32_t h = seed ^ (salt * 0x45d9f3b);
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        return (h >> 16) ^ h;
    };

    const uint32_t barSeed = hashVal (bar * 37 + 101);

    if (role == InstrumentRole::LeadHook)
    {
        const int patIdx = static_cast<int> ((barSeed + (isResponse ? 2 : 0) + (isTension ? 4 : 0)) % 8);
        const char* mask = kRoleRhythmsLeadHook[patIdx];
        const int leadPitches[8] = { 0, 3, 5, 7, 10, 12, 14, 15 };
        const HarmonicFunction leadFuncs[8] = {
            HarmonicFunction::Root, HarmonicFunction::Third, HarmonicFunction::Fifth, HarmonicFunction::Passing,
            HarmonicFunction::Seventh, HarmonicFunction::OctavePop, HarmonicFunction::Ninth, HarmonicFunction::Third
        };

        for (int s = 0; s < 16; ++s)
        {
            if (mask[s] == 'X')
            {
                const int gStep = bar * 16 + s;
                const int pIdx = static_cast<int> ((hashVal (gStep * 11 + 5) + s) % 8);
                const int deg = barRoot + leadPitches[pIdx];

                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : ((s % 4 == 0) ? 22.0f : 42.0f);
                ev.semitone = quantize (deg);
                ev.octaveShift = 2;
                ev.baseVelocity = 92;
                ev.baseGate = 0.65f;
                ev.accentThreshold = (s % 4 == 2 || s == 6 || s == 14) ? 45.0f : 78.0f;
                ev.accentVelocity = 122;
                ev.accentGate = 0.85f;
                ev.harmonicFunc = leadFuncs[pIdx];
                events.push_back (ev);
            }
        }
    }
    else if (role == InstrumentRole::PolyChordStab)
    {
        const int patIdx = static_cast<int> ((barSeed + (isResponse ? 1 : 0) + (isFillBar ? 3 : 0)) % 8);
        const char* mask = kRoleRhythmsChordStab[patIdx];
        const int chordVoicings[4] = { 0, 3, 7, 10 };
        const HarmonicFunction chordFuncs[4] = {
            HarmonicFunction::Root, HarmonicFunction::Third, HarmonicFunction::Fifth, HarmonicFunction::Seventh
        };

        for (int s = 0; s < 16; ++s)
        {
            if (mask[s] == 'X')
            {
                const int gStep = bar * 16 + s;
                const int vIdx = static_cast<int> ((hashVal (gStep * 13 + 7) + bar) % 4);
                const int deg = barRoot + chordVoicings[vIdx];

                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (s == 2 || s == 10 || s == 4) ? 0.0f : 36.0f;
                ev.semitone = quantize (deg);
                ev.octaveShift = 1;
                ev.baseVelocity = 88;
                ev.baseGate = 0.32f;
                ev.chordType = 1; // Rich minor 7th stab
                ev.accentThreshold = (s == 6 || s == 14) ? 40.0f : 75.0f;
                ev.accentVelocity = 118;
                ev.accentGate = 0.40f;
                ev.harmonicFunc = chordFuncs[vIdx];
                events.push_back (ev);
            }
        }
    }
    else if (role == InstrumentRole::ModularRatchet)
    {
        const int patIdx = static_cast<int> ((barSeed + (isTension ? 2 : 0) + (isFillBar ? 5 : 0)) % 8);
        const char* mask = kRoleRhythmsModularRatchet[patIdx];

        for (int s = 0; s < 16; ++s)
        {
            const char ch = mask[s];
            if (ch == 'X' || ch == 'r')
            {
                const int gStep = bar * 16 + s;
                const bool isRatchet = (ch == 'r');
                const int deg = barRoot + (isRatchet ? 12 : ((s % 4 == 2) ? 7 : 0));

                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (!isRatchet && (s == 0 || s == 8)) ? 0.0f : (isRatchet ? 38.0f : 20.0f);
                ev.semitone = quantize (deg);
                ev.octaveShift = isRatchet ? 2 : 1;
                ev.baseVelocity = 90;
                ev.baseGate = isRatchet ? 0.22f : 0.32f;
                ev.ratchet = isRatchet ? (isFillBar ? 4 : (isTension ? 3 : 2)) : 1;
                ev.accentThreshold = isRatchet ? 45.0f : 75.0f;
                ev.accentVelocity = 120;
                ev.accentGate = 0.40f;
                ev.harmonicFunc = isRatchet ? HarmonicFunction::OctavePop : ((s % 4 == 2) ? HarmonicFunction::Fifth : HarmonicFunction::Root);
                events.push_back (ev);
            }
        }
    }
    else if (role == InstrumentRole::AcidLine)
    {
        const int patIdx = static_cast<int> ((barSeed + (isResponse ? 3 : 0)) % 8);
        const char* mask = kRoleRhythmsAcidLine[patIdx];
        const int acidIntervals[8] = { 0, 10, 12, 3, 7, 0, 12, 10 };
        const HarmonicFunction acidFuncs[8] = {
            HarmonicFunction::Root, HarmonicFunction::Seventh, HarmonicFunction::OctavePop, HarmonicFunction::Third,
            HarmonicFunction::Fifth, HarmonicFunction::Root, HarmonicFunction::OctavePop, HarmonicFunction::Seventh
        };

        for (int s = 0; s < 16; ++s)
        {
            if (mask[s] == 'X')
            {
                const int gStep = bar * 16 + s;
                const int aIdx = static_cast<int> ((hashVal (gStep * 7 + 3) + s) % 8);
                const int deg = barRoot + acidIntervals[aIdx];
                const bool isOctaveJump = (s % 4 == 2 || s == 7 || s == 14);
                const bool isSlideHit = (s % 4 == 3 || s == 6 || s == 13);

                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (s == 0 || s == 6 || s == 10) ? 0.0f : ((s % 2 == 0) ? 20.0f : 40.0f);
                ev.semitone = quantize (deg);
                ev.octaveShift = isOctaveJump ? 1 : 0;
                ev.slide = isSlideHit;
                ev.baseVelocity = 90;
                ev.baseGate = 0.60f;
                ev.accentThreshold = (isOctaveJump || isSlideHit) ? 35.0f : 75.0f;
                ev.accentVelocity = 126;
                ev.accentGate = 0.85f;
                ev.harmonicFunc = acidFuncs[aIdx];
                events.push_back (ev);
            }
        }
    }
    else if (role == InstrumentRole::ClassicArp)
    {
        const int patIdx = static_cast<int> ((barSeed + (isResponse ? 1 : 0)) % 8);
        const char* mask = kRoleRhythmsClassicArp[patIdx];
        const int arpIntervals[4] = { 0, 3, 7, 10 };
        const HarmonicFunction arpFuncs[4] = {
            HarmonicFunction::Root, HarmonicFunction::Third, HarmonicFunction::Fifth, HarmonicFunction::Seventh
        };

        for (int s = 0; s < 16; ++s)
        {
            if (mask[s] == 'X')
            {
                const int gStep = bar * 16 + s;
                const int deg = barRoot + arpIntervals[(s + hashVal (bar + 3)) % 4];

                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : ((s % 4 == 0) ? 18.0f : 38.0f);
                ev.semitone = quantize (deg);
                ev.octaveShift = 1 + ((s == 6 || s == 14) ? 1 : 0);
                ev.baseVelocity = 82;
                ev.baseGate = 0.35f;
                ev.accentThreshold = (s == 6 || s == 14) ? 38.0f : 80.0f;
                ev.accentVelocity = 108;
                ev.accentGate = 0.45f;
                ev.harmonicFunc = arpFuncs[(s + hashVal (bar + 3)) % 4];
                events.push_back (ev);
            }
        }
    }
    else if (role == InstrumentRole::HarmonyChords)
    {
        const int patIdx = static_cast<int> (barSeed % 4);
        const char* mask = kRoleRhythmsHarmonyChords[patIdx];

        for (int s = 0; s < 16; ++s)
        {
            if (mask[s] == 'X')
            {
                const int gStep = bar * 16 + s;
                LayeredCandidateEvent ev;
                ev.bar = bar;
                ev.stepInBar = s;
                ev.globalStep = gStep;
                ev.activationThreshold = (s == 0) ? 0.0f : 30.0f;
                ev.semitone = quantize (barRoot);
                ev.octaveShift = 1;
                ev.baseVelocity = 78;
                ev.baseGate = 0.95f;
                ev.chordType = 1;
                ev.accentThreshold = 60.0f;
                ev.accentVelocity = 98;
                ev.accentGate = 1.10f;
                ev.harmonicFunc = HarmonicFunction::Root;
                events.push_back (ev);
            }
        }
    }
}

//==============================================================================
// Generatore Master a Strati per Fisher Tech House Bass
//==============================================================================
class LayeredFisherBassGenerator
{
public:
    static std::vector<LayeredCandidateEvent> buildCandidateEvents (uint32_t seed, ScaleType scale)
    {
        return buildCandidateEvents (seed, InstrumentRole::MonoBass, scale);
    }

    static std::vector<LayeredCandidateEvent> buildCandidateEvents (uint32_t seed, InstrumentRole role, ScaleType scale)
    {
        std::vector<LayeredCandidateEvent> events;
        events.reserve (128);

        // Funzione hash deterministica derivata da seed
        auto hashVal = [seed] (uint32_t salt) -> uint32_t
        {
            uint32_t h = seed ^ (salt * 0x45d9f3b);
            h = ((h >> 16) ^ h) * 0x45d9f3b;
            h = ((h >> 16) ^ h) * 0x45d9f3b;
            return (h >> 16) ^ h;
        };

        const auto& scaleInts = getScaleIntervals (scale);
        auto pickScaleNote = [&scaleInts] (int targetSemi) -> int
        {
            if (scaleInts.empty()) return 0;
            int best = scaleInts[0];
            int bestDiff = 999;
            for (int s : scaleInts)
            {
                int diff = std::abs (s - targetSemi);
                if (diff < bestDiff) { bestDiff = diff; best = s; }
            }
            return best;
        };

        static const int kFisherProgressions[8][8] = {
            {  0,  0, 10, 10,  0,  0, 10, 10 }, // 0: i -> b7 -> i -> b7
            {  0,  0,  3,  3,  5,  5,  0,  0 }, // 1: i -> bIII -> iv -> i
            {  0,  0,  8,  8, 10, 10,  0,  0 }, // 2: i -> VI -> VII -> i
            {  0,  0,  5,  5,  7,  7,  0,  0 }, // 3: i -> iv -> v -> i
            {  0,  0,  3,  3,  0,  0, -2, -2 }, // 4: i -> bIII -> i -> b7
            {  0,  0, 10, 10,  8,  8, 10, 10 }, // 5: i -> b7 -> b6 -> b7
            {  0,  0,  7,  7,  5,  5,  0,  0 }, // 6: i -> v -> iv -> i
            {  0,  0,  0,  0,  5,  5, 10, 10 }  // 7: i drone -> iv -> b7
        };
        const int progIdx = static_cast<int> (hashVal (7) % 8);

        const int degreeB3 = pickScaleNote (3);
        const int degree5th = pickScaleNote (7);
        const int degreeB7 = pickScaleNote (10);
        const int passingChoices[5] = { pickScaleNote (2), pickScaleNote (3), pickScaleNote (5), pickScaleNote (7), pickScaleNote (10) };
        const int altPassingDegree = pickScaleNote (5);
        const int tensionDegree = pickScaleNote (11);

        if (role != InstrumentRole::MonoBass)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 1 || bar == 3 || bar == 5 || bar == 7);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRoot     = pickScaleNote (kFisherProgressions[progIdx][bar]);

                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, pickScaleNote);
            }
            return events;
        }

        // =====================================================================
        // DEFAULT: InstrumentRole::MonoBass (Fisher Rolling Tech House Bass)
        // =====================================================================

        for (int bar = 0; bar < 8; ++bar)
        {
            bool isStatement = (bar == 0 || bar == 2 || bar == 4);
            bool isResponse  = (bar == 1 || bar == 3 || bar == 5);
            bool isTension   = (bar == 6);
            bool isFillBar   = (bar == 7);

            const int barRootRaw = kFisherProgressions[progIdx][bar];
            const int barRoot = pickScaleNote (barRootRaw);

            // Variazione deterministica di pop e ghost per battuta
            int popStepA = 6 + static_cast<int> ((hashVal (bar * 11 + 3) % 3) * 4); // 6, 10 o 14
            int popStepB = 14 + static_cast<int> (hashVal (bar * 11 + 4) % 2);     // 14 o 15

            int ghostStep1 = 1 + static_cast<int> ((hashVal (bar * 13 + 5) % 4) * 2); // 1, 3, 5, 7
            int ghostStep2 = 9 + static_cast<int> ((hashVal (bar * 13 + 6) % 4) * 2); // 9, 11, 13, 15

            int barPassingNote = passingChoices[(hashVal (bar * 17 + 7) + (isResponse ? 1 : 0)) % 5];
            int statementNote = (hashVal (bar * 19 + 5) % 2 == 0) ? 0 : degreeB3;
            int step6Note = (hashVal (bar * 23 + 9) % 3 == 0) ? degreeB7 : 0;

            // Per ogni step della battuta (0..15)
            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;

                // -------------------------------------------------------------
                // 1. STRATO 0: SCHELETRO (Soglia 0.0)
                // Punti cardine irrinunciabili del groove Tech House
                // Verso 0: velocity uniforme 90, note lunghe gate 0.50, tono fondamentale
                // -------------------------------------------------------------
                if (s == 2 || s == 10)
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = 0.0f; // SEMPRE attivo
                    ev.semitone = (s == 10) ? (isResponse ? (barRoot + barPassingNote) : (barRoot + statementNote)) : barRoot;
                    ev.octaveShift = (s == 10 && isTension) ? 1 : 0;
                    ev.baseVelocity = 90;
                    ev.baseGate = 0.50f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = (s == 2) ? 35.0f : 45.0f;
                    ev.accentVelocity = (s == 2) ? 118 : 115;
                    ev.accentGate = 0.65f;
                    ev.harmonicFunc = HarmonicFunction::Root;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 2. STRATO 1: PAUSE & PICKUP (Soglia 12.0 .. 24.0)
                // Completamento della griglia sui levare e pickup
                // -------------------------------------------------------------
                if (s == 6 || s == 14)
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = (s == 6) ? 14.0f : 22.0f;
                    ev.semitone = barRoot + ((s == 14) ? (isResponse ? degreeB7 : barPassingNote) : step6Note);
                    ev.octaveShift = 0;
                    ev.baseVelocity = 90;
                    ev.baseGate = 0.50f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = (s == 6) ? 55.0f : 65.0f;
                    ev.accentVelocity = 115;
                    ev.accentGate = 0.65f;
                    ev.harmonicFunc = (s == 14) ? HarmonicFunction::Seventh : HarmonicFunction::Passing;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 3. STRATO 2: GHOST NOTES (Soglia 26.0 .. 42.0 e 76.0)
                // Percussività staccato a bassa velocity
                // -------------------------------------------------------------
                if (s == ghostStep1 || s == ghostStep2)
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = (s == ghostStep1) ? 28.0f : 38.0f;
                    ev.semitone = barRoot + (hashVal (gStep + 13) % 2 == 0 ? 0 : degreeB3);
                    ev.octaveShift = 0;
                    ev.baseVelocity = (s == ghostStep1) ? 42 : 38;
                    ev.baseGate = 0.50f; // Staccato percussivo
                    ev.isGhost = true;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 101.0f; // Mai accento su ghost
                    ev.harmonicFunc = HarmonicFunction::Passing;
                    events.push_back (ev);
                    continue;
                }

                // Terza ghost note rolling ad alta complessità (C >= 76)
                if (s == 5 && (isStatement || isResponse))
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = 76.0f;
                    ev.semitone = barRoot;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 36;
                    ev.baseGate = 0.50f;
                    ev.isGhost = true;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 101.0f;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 4. STRATO 3: NOTE DI PASSAGGIO & b7 (Soglia 46.0 .. 58.0)
                // Frasi di risposta melodica che mantengono lo scheletro intatto
                // -------------------------------------------------------------
                if (s == 8 && (isResponse || isTension || isFillBar))
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = 48.0f;
                    ev.semitone = barRoot + altPassingDegree;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 92;
                    ev.baseGate = 0.52f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 75.0f;
                    ev.accentVelocity = 116;
                    ev.accentGate = 0.66f;
                    events.push_back (ev);
                    continue;
                }

                if (s == 12 && (isResponse || isFillBar))
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = 56.0f;
                    ev.semitone = barRoot + tensionDegree;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 93;
                    ev.baseGate = 0.52f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 78.0f;
                    ev.accentVelocity = 117;
                    ev.accentGate = 0.68f;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 5. STRATO 4: POP D'OTTAVA (Soglia 62.0 .. 74.0)
                // Salti energetici all'ottava superiore (max 2 per battuta)
                // -------------------------------------------------------------
                if ((s == popStepA && (isResponse || isTension)) || (s == popStepB && isResponse))
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = (s == popStepA) ? 63.0f : 72.0f;
                    ev.semitone = barRoot; // Ottava della fondamentale di battuta
                    ev.octaveShift = 1;
                    ev.baseVelocity = 122; // Forte
                    ev.baseGate = 0.80f;   // Sustain pieno d'ottava
                    ev.isGhost = false;
                    ev.isOctavePop = true;
                    ev.ratchet = 1;
                    ev.accentThreshold = 0.0f; // Sempre accentata
                    ev.accentVelocity = 125;
                    ev.accentGate = 0.80f;
                    ev.harmonicFunc = HarmonicFunction::OctavePop;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 6. STRATO 5: RITMO ROLLING E RATCHET (Soglia 76.0 .. 84.0)
                // Semicrome continuous drive
                // -------------------------------------------------------------
                if (s == 4 || s == 0)
                {
                    // Nota sul tempo forte nei momenti ad alta spinta
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = (s == 0) ? 76.0f : 80.0f;
                    ev.semitone = 0;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 90;
                    ev.baseGate = 0.52f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 85.0f;
                    ev.accentVelocity = 118;
                    ev.accentGate = 0.66f;
                    events.push_back (ev);
                    continue;
                }

                // Ratchet finale a battuta 8 (fill cadenza)
                if (isFillBar && s == 15)
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = 82.0f;
                    ev.semitone = degreeB7;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 115;
                    ev.baseGate = 0.65f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 2; // RATCHET ATTIVO
                    ev.accentThreshold = 82.0f;
                    ev.accentVelocity = 120;
                    ev.accentGate = 0.70f;
                    events.push_back (ev);
                    continue;
                }

                // -------------------------------------------------------------
                // 7. STRATO 6: FILL & TENSIONE CRESCO (Soglia 86.0 .. 98.0)
                // Battute 7 e 8: risoluzione e leading tone
                // -------------------------------------------------------------
                if ((isTension || isFillBar) && (s == 5 || s == 13))
                {
                    LayeredCandidateEvent ev;
                    ev.bar = bar;
                    ev.stepInBar = s;
                    ev.globalStep = gStep;
                    ev.activationThreshold = (s == 5) ? 88.0f : 94.0f;
                    ev.semitone = (s == 13) ? degreeB7 : degree5th;
                    ev.octaveShift = 0;
                    ev.baseVelocity = 95;
                    ev.baseGate = 0.54f;
                    ev.isGhost = false;
                    ev.isOctavePop = false;
                    ev.ratchet = 1;
                    ev.accentThreshold = 92.0f;
                    ev.accentVelocity = 120;
                    ev.accentGate = 0.66f;
                    events.push_back (ev);
                    continue;
                }
            }
        }

        return events;
    }

    // Valuta la sequenza di 128 step per un dato valore di Complexity (0..100)
    static void evaluate128Steps (const std::vector<LayeredCandidateEvent>& candidates,
                                  float complexity,
                                  std::vector<StepData>& out128Steps)
    {
        out128Steps.clear();
        out128Steps.resize (128);

        // Inizializza tutti gli step come inattivi
        for (int i = 0; i < 128; ++i)
        {
            out128Steps[static_cast<size_t>(i)].active = false;
            out128Steps[static_cast<size_t>(i)].semitoneOffset = 0;
            out128Steps[static_cast<size_t>(i)].octaveShift = 0;
            out128Steps[static_cast<size_t>(i)].velocity = 0;
            out128Steps[static_cast<size_t>(i)].gate = 0.50f;
            out128Steps[static_cast<size_t>(i)].accent = false;
            out128Steps[static_cast<size_t>(i)].slide = false;
            out128Steps[static_cast<size_t>(i)].ratchet = 1;
        }

        // Un evento suona SOLO SE complexity >= activationThreshold
        for (const auto& ev : candidates)
        {
            if (ev.globalStep >= 0 && ev.globalStep < 128)
            {
                if (complexity >= ev.activationThreshold)
                {
                    auto& s = out128Steps[static_cast<size_t>(ev.globalStep)];
                    s.active = true;
                    s.semitoneOffset = ev.semitone;
                    s.octaveShift = ev.octaveShift;
                    s.ratchet = ev.ratchet;

                    bool hasAccent = (complexity >= ev.accentThreshold);
                    s.accent = hasAccent;

                    // Espansione dinamica rigorosamente non decrescente con Complexity
                    const float cNorm = complexity * 0.01f;

                    if (ev.isGhost)
                    {
                        // Ghost: velocity 42 -> 26 con l'aumentare di complexity (più contrasto)
                        s.velocity = std::max (26, static_cast<int> (ev.baseVelocity - (cNorm * 16.0f)));
                        s.gate = 0.50f + (cNorm * 0.02f);
                    }
                    else if (ev.isOctavePop)
                    {
                        // Pop d'ottava: velocity 124 -> 127
                        s.velocity = std::min (127, static_cast<int> (124 + (cNorm * 3.0f)));
                        s.gate = 0.80f + (cNorm * 0.02f);
                    }
                    else if (hasAccent)
                    {
                        // Accent: velocity 118 -> 126
                        s.velocity = std::min (126, static_cast<int> (ev.accentVelocity + (cNorm * 10.0f)));
                        s.gate = ev.accentGate + (cNorm * 0.03f);
                    }
                    else
                    {
                        // Note normali
                        s.velocity = ev.baseVelocity;
                        s.gate = ev.baseGate + (cNorm * 0.04f);
                    }
                    s.slide = ev.slide;
                    s.chordType = ev.chordType;
                    s.harmonicFunc = ev.harmonicFunc;
                }
            }
        }
    }
};

} // namespace PulseForge
