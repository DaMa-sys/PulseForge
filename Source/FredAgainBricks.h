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
// Mattoncini Statistici e Astratti: FRED AGAIN.. (Emotional UK House / Garage)
//==============================================================================

static const int kFredAgainProgressions[8][8] = {
    {  0,  0,  8,  8,  3,  3, 10, 10 }, // 0: i -> VI -> III -> VII (Danielle / Bleu anthem)
    {  0,  0,  7,  7,  8,  8,  5,  5 }, // 1: i -> v -> VI -> iv (Kammy deep sadness)
    {  8,  8, 10, 10,  0,  0,  0,  0 }, // 2: VI -> VII -> i -> i (Delilah ascending rise)
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 3: i -> iv -> v -> i (Classic UK garage sorrow)
    {  5,  5,  7,  7,  0,  0,  0,  0 }, // 4: IV -> V -> vi -> vi (Bittersweet piano style)
    {  0,  0,  3,  3,  8,  8,  7,  7 }, // 5: i -> III -> VI -> v (Marea building hope)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 6: i -> b7 -> b6 -> b7 (Jungle / Garage cadence)
    {  0,  0,  0,  0,  8,  8, 10, 10 }  // 7: i pedal -> VI -> VII (Emotional explosion)
};

struct FredMelodyCell
{
    int degrees[4];
};

static const FredMelodyCell kFredVocalMotifs[8] = {
    {  0,  3,  7,  5 }, // 0: Root -> b3 -> 5 -> 4
    {  7, 10, 12,  7 }, // 1: 5 -> b7 -> Octave -> 5
    {  3,  2,  0, -2 }, // 2: b3 -> 2 -> Root -> b7
    {  0,  7,  5,  3 }, // 3: Root -> 5 -> 4 -> b3
    {  5,  7,  8,  7 }, // 4: 4 -> 5 -> b6 -> 5
    {  0,  0,  3,  2 }, // 5: Echo chop
    {  3,  5,  7, 10 }, // 6: b3 -> 4 -> 5 -> b7
    {  0, 12, 10,  7 }  // 7: High falsetto drop
};

static const FredMelodyCell kFredBassMotifs[8] = {
    {  0,  0, -2,  0 }, // 0: Root appoggio b7
    {  0,  3,  0, -2 }, // 1: Root -> b3 -> Root -> b7
    {  0,  0,  5,  0 }, // 2: Salto di quarta/quinta
    {  0, -2, -4, -2 }, // 3: Cadenza discendente
    {  0,  0,  0, 12 }, // 4: Pop d'ottava
    {  0,  3,  5,  7 }, // 5: Walking bassline
    {  0,  7,  5,  3 }, // 6: Discesa melodica
    {  0,  0,  3,  2 }  // 7: Sospiro minore
};

//==============================================================================
// Generatore Layered per Fred again..
//==============================================================================
class LayeredFredAgainGenerator
{
public:
    static std::vector<LayeredCandidateEvent> buildCandidateEvents (uint32_t seed,
                                                                   InstrumentRole role,
                                                                   ScaleType scale)
    {
        std::vector<LayeredCandidateEvent> events;
        events.reserve (128);

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

        const int progIdx = static_cast<int> (hashVal (1) % 8);
        const int leadMotifA = static_cast<int> (hashVal (2) % 8);
        const int leadMotifB = static_cast<int> (hashVal (3) % 8);
        const int leadMotifTension = static_cast<int> (hashVal (4) % 8);

        const int bassMotifA = static_cast<int> (hashVal (5) % 8);
        const int bassMotifB = static_cast<int> (hashVal (6) % 8);

        if (role != InstrumentRole::MonoBass && role != InstrumentRole::LeadHook)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kFredAgainProgressions[progIdx][bar];
                const int barRoot     = quantize (barRootRaw);

                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            return events;
        }

        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension  = (bar == 6);
            const bool isFillBar  = (bar == 7);

            const int barRootRaw = kFredAgainProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barMotIdx = static_cast<int> ((hashVal (bar * 17 + 5) + (isResponse ? leadMotifB : leadMotifA)) % 8);
            const auto& lMot = isTension ? kFredVocalMotifs[leadMotifTension] : kFredVocalMotifs[barMotIdx];

            const int barBassMotIdx = static_cast<int> ((hashVal (bar * 19 + 7) + (isResponse ? bassMotifB : bassMotifA)) % 8);
            const auto& bMot = isTension ? kFredBassMotifs[4] : kFredBassMotifs[barBassMotIdx];

            // Variazione deterministica dei punti di syncopazione per battuta
            const int stepPivotA = 2 + static_cast<int> (hashVal (bar * 13 + 1) % 2); // 2 o 3
            const int stepPivotB = 10 + static_cast<int> (hashVal (bar * 13 + 2) % 2); // 10 o 11
            const int stepSkipA  = 6 + static_cast<int> (hashVal (bar * 13 + 3) % 3);  // 6, 7 o 8
            const int stepSkipB  = 13 + static_cast<int> (hashVal (bar * 13 + 4) % 2); // 13 o 14

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int beatIdx = s / 4;
                const int sub4 = s % 4;

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: Delicate emotional plucks a semicrome
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 18.0f;
                        int rawDeg = barRoot + lMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 80;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 104;
                        ev.accentGate = 0.36f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 32.0f;
                        int rawDeg = barRoot + lMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 76;
                        ev.baseGate = 0.25f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 100;
                        ev.accentGate = 0.34f;
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
                        int rawDeg = barRoot + lMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 72;
                        ev.baseGate = 0.22f;
                        ev.accentThreshold = 80.0f;
                        ev.accentVelocity = 96;
                        ev.accentGate = 0.32f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 68.0f;
                        int rawDeg = barRoot + (isFillBar ? 12 : lMot.degrees[3]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = (isFillBar) ? 3 : 2;
                        ev.baseVelocity = 86;
                        ev.baseGate = 0.35f;
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 108;
                        ev.accentGate = 0.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: Vocal Chop / Piano Melody cantabile
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int rawDeg = barRoot + lMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 1;
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.85f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 1.05f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == stepSkipA || s == stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 24.0f;
                        int rawDeg = barRoot + lMot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = (s == stepSkipB && isResponse) ? 2 : 1;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.75f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 115;
                        ev.accentGate = 0.95f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 4 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 46.0f;
                        int rawDeg = barRoot + lMot.degrees[sub4] + ((hashVal (gStep + 5) % 2 == 0) ? 7 : 3);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = isResponse ? 2 : 1;
                        ev.baseVelocity = 84;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.85f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 7 || s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 15) ? 58.0f : 64.0f;
                        int rawDeg = barRoot + (s == 15 && isFillBar ? 12 : lMot.degrees[sub4]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = (isTension || isFillBar) ? 2 : 1;
                        ev.baseVelocity = 86;
                        ev.baseGate = 0.70f;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.90f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 14 && isTension)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 50.0f;
                        int rawDeg = barRoot + 7;
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 95;
                        ev.baseGate = 0.80f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 1.00f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::PolyChordStab)
                {
                    // =========================================================
                    // RUOLO CHORD STABS: Garage M1/Rhodes chord stabs syncopati
                    // =========================================================
                    if (s == stepPivotA || s == stepSkipA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == stepPivotA) ? 0.0f : ((s == stepSkipA) ? 25.0f : 55.0f);
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 4; // 9th Stab
                        ev.baseVelocity = 84;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 114;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::HarmonyChords)
                {
                    // =========================================================
                    // RUOLO HARMONY CHORDS: Gospel / ambient piano sostenuto
                    // =========================================================
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0) ? 0.0f : 26.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 0;
                        ev.chordType = 4; // 9th Voicing
                        ev.baseVelocity = 78;
                        ev.baseGate = 1.25f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 98;
                        ev.accentGate = 1.50f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::AcidLine)
                {
                    // =========================================================
                    // RUOLO ACID LINE: Garage Acid Glides & Squawks
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + (s == stepPivotA ? 0 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == stepPivotB) ? 1 : 0;
                        ev.slide = (s == stepPivotB);
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 125;
                        ev.accentGate = 0.85f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == stepSkipA || s == stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 28.0f;
                        int deg = barRoot + (s == stepSkipA ? 3 : 10);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1;
                        ev.slide = true;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.60f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.78f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: UK Garage Stutter & Beat Repeats
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
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
                    if (s == 7 || s == 11 || s == 14 || s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 15) ? 40.0f : 55.0f;
                        int deg = barRoot + (s == 15 ? 12 : 5);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 15) ? 2 : 1;
                        ev.ratchet = (s == 15) ? (isFillBar ? 4 : 3) : 2;
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.25f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else
                {
                    // =========================================================
                    // RUOLO MONO BASS: 2-STEP GARAGE BASS
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 108;
                        ev.baseGate = 0.42f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.52f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepSkipA)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 22.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 102;
                        ev.baseGate = 0.38f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 116;
                        ev.accentGate = 0.48f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 38.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == 4 || s == 14 || s == 0)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 56.0f + static_cast<float> (hashVal (gStep + 300) % 15);
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 52;
                        ev.baseGate = 0.28f;
                        ev.isGhost = true;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 74.0f;

                        int nextBar = (bar + 1) % 8;
                        int nextRoot = quantize (kFredAgainProgressions[progIdx][nextBar]);
                        int turnChoice = static_cast<int> (hashVal (bar * 29 + 11) % 3);
                        int deg = (isFillBar || isTension) ? (nextRoot - 2)
                                                           : (turnChoice == 0 ? barRoot
                                                                              : (turnChoice == 1 ? barRoot - 2
                                                                                                 : barRoot + 7));

                        ev.semitone = quantize (deg);
                        ev.octaveShift = (isFillBar) ? 1 : 0;
                        ev.baseVelocity = 116;
                        ev.baseGate = 0.46f;
                        ev.isOctavePop = (ev.octaveShift > 0);
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 125;
                        ev.accentGate = 0.55f;
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
