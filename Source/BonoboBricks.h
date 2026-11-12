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
// Mattoncini Statistici e Astratti: BONOBO (Organic Downtempo)
//==============================================================================

static const int kBonoboProgressions[8][8] = {
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 0: i -> bIII -> iv -> v (Cirrus meditative flow)
    {  0,  0,  8,  8, 10, 10,  7,  7 }, // 1: i -> VI -> VII -> v (Black Sands melancholy)
    {  0,  0,  2,  2,  5,  5,  0,  0 }, // 2: i -> ii -> iv -> i (Kerala dorian breeze)
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 3: i -> iv -> v -> i (Warm acoustic loop)
    {  0,  0,  0,  0,  3,  3,  2,  2 }, // 4: i pedal -> bIII -> ii (Pensive morning)
    {  0,  0,  8,  8,  5,  5,  7,  7 }, // 5: i -> VI -> iv -> v (Expansive horizon)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 6: i -> b7 -> b6 -> b7 (Slow hypnotic pulse)
    {  0,  0,  0,  0,  5,  5,  0,  0 }  // 7: i pedal -> iv -> i (Pure zen stillness)
};

struct BonoboMelodyCell
{
    int degrees[4];
};

static const BonoboMelodyCell kBonoboKalimbaMotifs[8] = {
    {  0,  7,  3,  5 }, // 0: Kalimba climb (Root -> 5 -> b3 -> 4)
    {  7,  5,  3,  0 }, // 1: Gentle cascade (5 -> 4 -> b3 -> Root)
    {  0,  3,  7, 10 }, // 2: Minor 7th spread
    {  2,  3,  5,  7 }, // 3: Stepwise ascending dorian prayer
    {  0,  0,  5,  3 }, // 4: Soft acoustic echo
    {  3,  7, 10, 12 }, // 5: Upper register chime
    {  0, 10,  7,  3 }, // 6: Open drop
    {  0,  5,  7,  0 }  // 7: Pendulum rest
};

static const BonoboMelodyCell kBonoboBassMotifs[8] = {
    {  0,  7,  5, -2 }, // 0: Root -> 5th -> 4th -> b7
    {  0,  3,  5,  0 }, // 1: Root -> b3 -> 4th -> Root
    {  0, -2, -4, -2 }, // 2: Root -> b7 -> b6 -> b7 (Cadenza profonda)
    {  0,  7,  3,  5 }, // 3: Kalimba bass arch
    {  0,  5,  7, 10 }, // 4: Ascending pentatonic
    {  0,  3,  2,  0 }, // 5: Dorian sigh
    {  0, 12,  7, -2 }, // 6: Wide acoustic leap & drop
    {  0, -2,  0,  5 }  // 7: Deep drone walking
};

//==============================================================================
// Generatore Layered per Bonobo
//==============================================================================
class LayeredBonoboGenerator
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
        const int motifA = static_cast<int> (hashVal (2) % 8);
        const int motifB = static_cast<int> (hashVal (3) % 8);
        const int bassMotA = static_cast<int> (hashVal (4) % 8);
        const int bassMotB = static_cast<int> (hashVal (5) % 8);

        if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kBonoboProgressions[progIdx][bar];
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

            const int barRootRaw = kBonoboProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barMotIdx = static_cast<int> ((hashVal (bar * 17 + 5) + (isResponse ? motifB : motifA)) % 8);
            const auto& mot = isTension ? kBonoboKalimbaMotifs[5] : kBonoboKalimbaMotifs[barMotIdx];

            const int barBassIdx = static_cast<int> ((hashVal (bar * 19 + 7) + (isResponse ? bassMotB : bassMotA)) % 8);
            const auto& bMot = isTension ? kBonoboBassMotifs[7] : kBonoboBassMotifs[barBassIdx];

            const int stepPivotA = (hashVal (bar * 7 + 5) % 3 == 0) ? 2 : 0;
            const int stepPivotB = 4 + static_cast<int> (hashVal (bar * 11 + 1) % 5); // 4, 5, 6, 7 o 8
            const int stepSub    = 9 + static_cast<int> (hashVal (bar * 11 + 2) % 4); // 9, 10, 11 o 12
            const int stepPick   = 13 + static_cast<int> (hashVal (bar * 11 + 3) % 3); // 13, 14 o 15

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int beatIdx = s / 4;
                const int sub4 = s % 4;

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: Organic Kalimba / Harp Arpeggio
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 18.0f;
                        int rawDeg = barRoot + mot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 76;
                        ev.baseGate = 0.35f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 100;
                        ev.accentGate = 0.45f;
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
                        int rawDeg = barRoot + mot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 72;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 96;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s % 2 == 1 && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 54.0f + static_cast<float> (hashVal (gStep + 100) % 15);
                        int rawDeg = barRoot + mot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 66;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 80.0f;
                        ev.accentVelocity = 92;
                        ev.accentGate = 0.36f;
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
                        int rawDeg = barRoot + (isFillBar ? 12 : mot.degrees[3]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = (isFillBar) ? 3 : 2;
                        ev.baseVelocity = 82;
                        ev.baseGate = 0.38f;
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 104;
                        ev.accentGate = 0.48f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: Flauto / Chitarra cantabile con respiro
                    // =========================================================
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int rawDeg = barRoot + mot.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 1;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.90f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 115;
                        ev.accentGate = 1.15f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 3 || s == 11)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 24.0f;
                        int rawDeg = barRoot + mot.degrees[2];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 1;
                        ev.baseVelocity = 84;
                        ev.baseGate = 0.82f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 110;
                        ev.accentGate = 1.05f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 6 || (s == 14 && isTension))
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 48.0f;
                        int rawDeg = barRoot + (isTension ? 7 : 4);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 86;
                        ev.baseGate = 0.80f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 114;
                        ev.accentGate = 1.00f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::PolyChordStab)
                {
                    // =========================================================
                    // RUOLO CHORD STABS: Mellow Rhodes / acoustic chord stabs
                    // =========================================================
                    if (s == 3 || s == 7 || s == 11 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 3 || s == 11) ? 0.0f : 35.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 6; // Soulful 13th
                        ev.baseVelocity = 78;
                        ev.baseGate = 0.38f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 104;
                        ev.accentGate = 0.48f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::HarmonyChords)
                {
                    // =========================================================
                    // RUOLO HARMONY CHORDS: Lush organic orchestral pads
                    // =========================================================
                    if (s == 0 || s == 8)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0) ? 0.0f : 25.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 0;
                        ev.chordType = 6; // Soulful 13th Voicing
                        ev.baseVelocity = 72;
                        ev.baseGate = 1.30f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 94;
                        ev.accentGate = 1.55f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::AcidLine)
                {
                    // =========================================================
                    // RUOLO ACID LINE: Organic Hybrid Acid Glides
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
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.68f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.88f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == stepSub || s == stepPick)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 28.0f;
                        int deg = barRoot + (s == stepPick ? 10 : 3);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1;
                        ev.slide = true;
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 116;
                        ev.accentGate = 0.82f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: Organic Granular / Percussion Rolls
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
                        ev.baseVelocity = 92;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 6 || s == 11 || s == 14 || s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 15) ? 42.0f : 55.0f;
                        int deg = barRoot + (s == 15 ? 12 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 15) ? 2 : 1;
                        ev.ratchet = (s == 15) ? (isFillBar ? 3 : 2) : 2;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 116;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else
                {
                    // =========================================================
                    // RUOLO MONO BASS: WARM ORGANIC ACOUSTIC / SUB BASS
                    // =========================================================
                    if (s == stepPivotA)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + bMot.degrees[0];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.75f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 20.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.55f;
                        ev.accentThreshold = 50.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.65f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepSub)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 38.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 94;
                        ev.baseGate = 0.45f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 108;
                        ev.accentGate = 0.55f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepPick)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 55.0f;
                        int deg = barRoot + (s == 15 && isFillBar ? 12 : bMot.degrees[3]);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 15 && isFillBar) ? 1 : 0;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.38f;
                        ev.accentThreshold = 70.0f;
                        ev.accentVelocity = 104;
                        ev.accentGate = 0.48f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s % 2 == 1 && s != stepPivotB && s != stepSub && s != stepPick)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 52.0f + static_cast<float> (hashVal (gStep + 200) % 15);
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 48; // Delicato ghost tap acustico
                        ev.baseGate = 0.25f;
                        ev.isGhost = true;
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
