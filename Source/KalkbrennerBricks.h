#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "PatternData.h"
#include "GenreProfiles.h"
#include "LayeredPatternGenerator.h"

namespace PulseForge
{

//==============================================================================
// Mattoncini Statistici e Astratti: Paul Kalkbrenner (Melodic Techno)
//
// ASTRAZIONI RICAVATE DALL'ANALISI CORPUS SU 2.598 FILE:
// - Ritmica: Rolling 16ths continuo (63%), Crome pulsanti (20%), Offbeat drive (10%)
// - Melodia: Gradi congiunti (76%), salti > 4a contenuti (11-24%), pedale di tonica
// - Macro-struttura: 8 battute (Statement, Response, Progression, Tension, Turnaround/Fill)
// - Progressione armonica: 8 percorsi tipici della Melodic Techno berlinese
// - Monotonia e Sottoinsieme: Soglia per evento (Complexity >= Threshold)
//==============================================================================

// 8 Progressioni armoniche a 8 battute (gradi/semitoni della fondamentale per battuta)
static const int kKalkbrennerProgressions[8][8] = {
    {  0,  0,  0,  0,  8,  8, 10, 10 }, // 0: i -> VI -> VII (Berlin Anthem)
    {  0,  0,  5,  5,  8,  8,  7,  7 }, // 1: i -> iv -> VI -> v (Emotional Journey)
    {  0,  0,  0,  0,  3,  3,  2,  2 }, // 2: i -> bIII -> ii (Deep Melancholy)
    {  0,  0,  8,  8,  5,  5,  7,  7 }, // 3: i -> VI -> iv -> v (Classic Afterlife)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 4: i -> VII -> VI -> VII (Sky & Sand style)
    {  0,  0,  0,  0,  0,  0, 10, 10 }, // 5: Tonic Drone with Turnaround
    {  0,  0,  7,  7,  8,  8,  7,  7 }, // 6: i -> v -> VI -> v (Hypnotic Loop)
    {  0,  0,  3,  3,  5,  5, 10, 10 }  // 7: i -> bIII -> iv -> VII (Ascending Hope)
};

// 8 Motivi melodici per arpeggio (4 gradi relativi alla radice della battuta)
struct KalkbrennerMelodyCell
{
    int degrees[4];
};

static const KalkbrennerMelodyCell kKalkbrennerArpMotifs[8] = {
    {  0,  3,  7, 12 }, // 0: Salita d'accordo minore (Root -> b3 -> 5 -> Octave)
    { 12,  7,  3,  2 }, // 1: Cascata discendente morbida (Octave -> 5 -> b3 -> 2)
    {  0,  2,  3,  5 }, // 2: Salita per gradi congiunti (Root -> 2 -> b3 -> 4)
    {  7,  5,  3,  2 }, // 3: Sospiro melodico Kalkbrenner (5 -> 4 -> b3 -> 2)
    {  0,  7, 10, 12 }, // 4: Apertura su b7 e ottava (Root -> 5 -> b7 -> Octave)
    {  0,  3,  2,  0 }, // 5: Pendolo ipnotico (Root -> b3 -> 2 -> Root)
    {  3,  5,  7, 10 }, // 6: Struttura superiore (b3 -> 4 -> 5 -> b7)
    {  0, 12,  7,  3 }  // 7: Salto d'ottava e discesa (Root -> Octave -> 5 -> b3)
};

// 6 Variazioni di movimento di basso per battuta
struct KalkbrennerBassMotion
{
    int degrees[4]; // Gradi su step 0, 4, 8, 12
};

static const KalkbrennerBassMotion kKalkbrennerBassMotions[6] = {
    { 0,  0,  0,  0 }, // 0: Pedale puro solido
    { 0,  0,  0, -2 }, // 1: Appoggio su b7 allo step 12
    { 0,  3,  0, -2 }, // 2: Tocco di b3 sul levare e b7 finale
    { 0,  0,  7,  0 }, // 3: Rinforzo di quinta sullo step 8
    { 0,  0,  0, 12 }, // 4: Pop d'ottava sullo step 12
    { 0, -2, -4, -2 }  // 5: Cadenza discendente (0 -> b7 -> b6 -> b7)
};

//==============================================================================
// Generatore Layered Master per Paul Kalkbrenner (Melodic Techno)
//==============================================================================
class LayeredKalkbrennerGenerator
{
public:
    static std::vector<LayeredCandidateEvent> buildCandidateEvents (uint32_t seed,
                                                                   InstrumentRole role,
                                                                   ScaleType scale)
    {
        std::vector<LayeredCandidateEvent> events;
        events.reserve (128);

        // Hashing deterministico dato il seed
        auto hashVal = [seed] (uint32_t salt) -> uint32_t
        {
            uint32_t h = seed ^ (salt * 0x45d9f3b);
            h = ((h >> 16) ^ h) * 0x45d9f3b;
            h = ((h >> 16) ^ h) * 0x45d9f3b;
            return (h >> 16) ^ h;
        };

        const auto& scaleInts = getScaleIntervals (scale);
        auto quantize = [&scaleInts] (int semi) -> int
        {
            if (scaleInts.empty()) return 0;
            int oct = (semi >= 0) ? (semi / 12) : ((semi - 11) / 12);
            int rem = semi - (oct * 12);
            int best = scaleInts[0];
            int bestDiff = 999;
            for (int s : scaleInts)
            {
                int diff = std::abs (s - rem);
                if (diff < bestDiff) { bestDiff = diff; best = s; }
            }
            return oct * 12 + best;
        };

        // 1. Parametri armonici e stilistici guidati dal seed
        const int progIdx = static_cast<int> (hashVal (1) % 8);
        const int bassMotionA = static_cast<int> (hashVal (2) % 6);
        const int bassMotionB = static_cast<int> (hashVal (3) % 6);
        const int rhythmStyle = static_cast<int> (hashVal (4) % 4); // 0=Rolling, 1=Straight 8th, 2=Gallop, 3=Minimal

        const int arpMotifA = static_cast<int> (hashVal (5) % 8);
        const int arpMotifB = static_cast<int> (hashVal (6) % 8);
        const int arpMotifTension = static_cast<int> (hashVal (7) % 8);
        const int arpOctaveReg = static_cast<int> (hashVal (8) % 2); // 0 = standard, 1 = +1 ottava

        const int popStepChoice = 6 + static_cast<int> (hashVal (9) % 3) * 4; // 6, 10, o 14

        if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kKalkbrennerProgressions[progIdx][bar];
                const int barRoot     = quantize (barRootRaw);

                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            return events;
        }

        // 2. Costruzione delle 8 battute (128 step)
        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isStatement = (bar == 0 || bar == 1 || bar == 4);
            const bool isResponse  = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension   = (bar == 6);
            const bool isFillBar   = (bar == 7);

            // Radice armonica della battuta (quantizzata sulla scala)
            const int barRootRaw = kKalkbrennerProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barBassMotIdx = static_cast<int> ((hashVal (bar * 13 + 3) + (isResponse ? bassMotionB : bassMotionA)) % 6);
            const auto& bMot = isTension ? kKalkbrennerBassMotions[4]
                                         : kKalkbrennerBassMotions[barBassMotIdx];

            const int barArpMotIdx = static_cast<int> ((hashVal (bar * 17 + 5) + (isResponse ? arpMotifB : arpMotifA)) % 8);
            const auto& aMot = isTension ? kKalkbrennerArpMotifs[arpMotifTension]
                                         : kKalkbrennerArpMotifs[barArpMotIdx];

            const int barPopStep = 6 + static_cast<int> ((hashVal (bar * 7 + 9) % 3) * 4); // 6, 10, o 14

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int beatIdx = s / 4;  // 0, 1, 2, 3
                const int sub4 = s % 4;     // 0, 1, 2, 3

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: Tapestry ipnotico a semicrome
                    // =========================================================
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int rawDeg = barRoot + aMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = arpOctaveReg;
                        ev.baseVelocity = 82;
                        ev.baseGate = 0.35f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 106;
                        ev.accentGate = 0.45f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 4 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 16.0f;
                        int rawDeg = barRoot + aMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = arpOctaveReg;
                        ev.baseVelocity = 80;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 102;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 34.0f;
                        int rawDeg = barRoot + aMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = arpOctaveReg;
                        ev.baseVelocity = 78;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 98;
                        ev.accentGate = 0.38f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s % 2 == 1 && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 52.0f + static_cast<float> (hashVal (gStep + 100) % 15);
                        int rawDeg = barRoot + aMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = arpOctaveReg + ((sub4 == 3 && (hashVal (gStep) % 3 == 0)) ? 1 : 0);
                        ev.baseVelocity = 74;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 85.0f;
                        ev.accentVelocity = 94;
                        ev.accentGate = 0.35f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 70.0f;
                        int rawDeg = barRoot + (isFillBar ? 12 : aMot.degrees[3]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = arpOctaveReg + (isFillBar ? 1 : 0);
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.40f;
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 80.0f;
                        ev.accentVelocity = 108;
                        ev.accentGate = 0.50f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: Tema cantabile con respiro (Sky & Sand)
                    // =========================================================
                    if (s == 0 || s == 6)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int rawDeg = barRoot + (s == 0 ? 0 : aMot.degrees[2]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 1;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.85f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 1.05f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 3 || s == 10)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 25.0f;
                        int rawDeg = barRoot + (s == 3 ? aMot.degrees[1] : aMot.degrees[3]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 1;
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.75f;
                        ev.accentThreshold = 50.0f;
                        ev.accentVelocity = 114;
                        ev.accentGate = 0.95f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 12 || (s == 14 && isTension))
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 55.0f;
                        int rawDeg = barRoot + (isTension ? 7 : 4);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 94;
                        ev.baseGate = 0.70f;
                        ev.accentThreshold = 70.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.90f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::PolyChordStab)
                {
                    // =========================================================
                    // RUOLO CHORD STABS: Staccato ritmico in levare
                    // =========================================================
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 2 || s == 10) ? 0.0f : 35.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 3; // Drop-2 7th
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 50.0f;
                        ev.accentVelocity = 115;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::HarmonyChords)
                {
                    // =========================================================
                    // RUOLO HARMONY CHORDS: Pad atmosferico disteso
                    // =========================================================
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0) ? 0.0f : 28.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 0;
                        ev.chordType = 5; // Ambient 11th
                        ev.baseVelocity = 76;
                        ev.baseGate = 1.20f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 96;
                        ev.accentGate = 1.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::AcidLine)
                {
                    // =========================================================
                    // RUOLO ACID LINE: Squelch 303 & Portamento Glides
                    // =========================================================
                    if (s == 0 || s == 6 || s == 10 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 10) ? 0.0f : 20.0f;
                        int deg = barRoot + (s == 0 ? 0 : (s == 6 ? 3 : (s == 10 ? 7 : 10)));
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 6 || s == 12) ? 1 : 0;
                        ev.slide = (s == 6 || s == 12);
                        ev.baseVelocity = 100;
                        ev.baseGate = 0.72f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 126;
                        ev.accentGate = 0.88f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 3 || s == 7 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 45.0f;
                        int deg = barRoot + (s == 14 ? 12 : 5);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 14) ? 1 : 0;
                        ev.slide = true;
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.80f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: Burst ritmici e stutter
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 20.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.baseVelocity = 95;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 115;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 6 || s == 10 || s == 14 || s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 14) ? 38.0f : ((s == 15) ? 55.0f : 42.0f);
                        int deg = barRoot + (s == 15 ? 12 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 15) ? 2 : 1;
                        ev.ratchet = (s == 14 || s == 15) ? (isFillBar ? 4 : (isTension ? 3 : 2)) : 2;
                        ev.baseVelocity = 90;
                        ev.baseGate = 0.25f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else
                {
                    // =========================================================
                    // RUOLO MONO BASS: Pulsazione Melodic Techno solida
                    // =========================================================

                    // Strato 0: Scheletro (Crome 0 e 8) - Fondamentale solida sulla radice armonica
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 106;
                        ev.baseGate = 0.36f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.46f;
                        events.push_back (ev);
                        continue;
                    }

                    // Strato 1: Crome 4 e 12 (completamento battiti 2 e 4)
                    if (s == 4 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 18.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 12 && isTension) ? 1 : 0;
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.35f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 116;
                        ev.accentGate = 0.44f;
                        events.push_back (ev);
                        continue;
                    }

                    // Strato 2: Semicrome sui levare (2, 6, 10, 14)
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;

                        ev.activationThreshold = (rhythmStyle == 3) ? 45.0f : 32.0f;

                        bool isPopStep = (s == barPopStep);
                        int deg = barRoot + (isPopStep ? 12 : bMot.degrees[beatIdx]);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = isPopStep ? 1 : 0;
                        ev.isOctavePop = isPopStep;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }

                    // Strato 3: Rolling 16ths e ghost notes (1, 3, 5, 7, 9, 11, 13)
                    if (s % 2 == 1 && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;

                        float baseThresh = (rhythmStyle == 1) ? 68.0f : 54.0f;
                        ev.activationThreshold = baseThresh + static_cast<float> (hashVal (gStep + 200) % 11);

                        int deg = barRoot;
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 50; // Ghost note delicata
                        ev.baseGate = 0.28f;
                        ev.isGhost = true;
                        events.push_back (ev);
                        continue;
                    }

                    // Strato 4: Salto d'ottava e turnaround battuta (step 15)
                    if (s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 72.0f;

                        int nextBar = (bar + 1) % 8;
                        int nextRoot = quantize (kKalkbrennerProgressions[progIdx][nextBar]);
                        int turnChoice = static_cast<int> (hashVal (bar * 23 + 11) % 3);
                        int deg = (isFillBar || isTension) ? (nextRoot - 2)
                                                           : (turnChoice == 0 ? barRoot
                                                                              : (turnChoice == 1 ? barRoot - 2
                                                                                                 : barRoot + 7));

                        ev.semitone = quantize (deg);
                        ev.octaveShift = (isFillBar) ? 1 : 0;
                        ev.baseVelocity = 114;
                        ev.baseGate = 0.48f;
                        ev.isOctavePop = (ev.octaveShift > 0);
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 124;
                        ev.accentGate = 0.58f;
                        events.push_back (ev);
                        continue;
                    }
                }
            }
        }

        return events;
    }
};

} // namespace PulseForge
