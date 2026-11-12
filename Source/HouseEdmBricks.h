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
// Mattoncini Statistici e Astratti: CLASSIC CHICAGO HOUSE & ITALO / EDM
//==============================================================================

static const int kClassicHouseProgressions[8][8] = {
    {  0,  0,  3,  3,  5,  5,  0,  0 }, // 0: i -> bIII -> iv -> i
    {  0,  0,  8,  8, 10, 10,  0,  0 }, // 1: i -> VI -> VII -> i
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 2: i -> iv -> v -> i
    {  0,  0,  0,  0,  3,  3,  5,  5 }, // 3: i pedal -> bIII -> iv
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 4: i -> b7 -> b6 -> b7
    {  0,  0,  2,  2,  5,  5,  0,  0 }, // 5: i -> ii -> iv -> i
    {  0,  0,  7,  7,  5,  5,  0,  0 }, // 6: i -> v -> iv -> i
    {  0,  0,  3,  3,  2,  2,  0,  0 }  // 7: i -> bIII -> ii -> i
};

static const int kEdmProgressions[8][8] = {
    {  0,  0,  8,  8,  3,  3, 10, 10 }, // 0: i -> VI -> III -> VII
    {  8,  8, 10, 10,  0,  0,  0,  0 }, // 1: VI -> VII -> i -> i
    {  0,  0,  5,  5,  8,  8, 10, 10 }, // 2: i -> iv -> VI -> VII
    {  0,  0,  0,  0,  8,  8, 10, 10 }, // 3: Moroder driving drone -> VI -> VII
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 4: i -> bIII -> iv -> v
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 5: i -> VII -> VI -> VII
    {  8,  8,  5,  5,  0,  0, 10, 10 }, // 6: VI -> iv -> i -> VII
    {  0,  0,  7,  7,  8,  8, 10, 10 }  // 7: i -> v -> VI -> VII
};

struct HouseBassCell
{
    int degrees[4];
};

static const HouseBassCell kHouseBassMotifs[8] = {
    {  0,  0,  3, -2 }, // 0: Classic Chicago bounce
    {  0,  3,  5,  0 }, // 1: Walking dorian
    {  0,  0, -2,  0 }, // 2: Minimal sub pedal
    {  0,  7,  5,  3 }, // 3: Soulful drop
    {  0, -2, -4, -2 }, // 4: Deep basement
    {  0,  0, 12, -2 }, // 5: Octave jump
    {  0,  5,  0, -2 }, // 6: Quarta and leading
    {  0,  3,  2,  0 }  // 7: Jazzy house sigh
};

//==============================================================================
// Generatore Layered per Classic House
//==============================================================================
class LayeredClassicHouseGenerator
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
        const int motA = static_cast<int> (hashVal (2) % 8);
        const int motB = static_cast<int> (hashVal (3) % 8);

        if (role != InstrumentRole::MonoBass)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kClassicHouseProgressions[progIdx][bar];
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

            const int barRootRaw = kClassicHouseProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barMotIdx = static_cast<int> ((hashVal (bar * 19 + 7) + (isResponse ? motB : motA)) % 8);
            const auto& bMot = isTension ? kHouseBassMotifs[5] : kHouseBassMotifs[barMotIdx];

            const int stepPivotA = 2 + static_cast<int> (hashVal (bar * 11 + 1) % 2); // 2 o 3
            const int stepPivotB = 10 + static_cast<int> (hashVal (bar * 11 + 2) % 2); // 10 o 11
            const int stepSkipA  = 6 + static_cast<int> (hashVal (bar * 11 + 3) % 2); // 6 o 7
            const int stepSkipB  = 13 + static_cast<int> (hashVal (bar * 11 + 4) % 2); // 13 o 14

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int beatIdx = s / 4;

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: 90s Piano / Pluck Arpeggio
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 18.0f;
                        int deg = barRoot + (s == 0 ? 0 : 5);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 84;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 106;
                        ev.accentGate = 0.38f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 30.0f;
                        int deg = barRoot + (s == 2 ? 3 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 80;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 102;
                        ev.accentGate = 0.36f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s % 2 == 1 && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 52.0f + static_cast<float> (hashVal (gStep) % 15);
                        ev.semitone = quantize (barRoot + 10);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 74;
                        ev.baseGate = 0.25f;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 98;
                        ev.accentGate = 0.34f;
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
                        ev.semitone = quantize (barRoot + (isFillBar ? 12 : 7));
                        ev.octaveShift = isFillBar ? 3 : 2;
                        ev.baseVelocity = 90;
                        ev.baseGate = 0.35f;
                        ev.ratchet = isFillBar ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: 90s Catchy Organ / Synth Hook
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + (s == stepPivotA ? 0 : 5);
                        ev.semitone = quantize (deg);
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
                        int deg = barRoot + (s == stepSkipA ? 3 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == stepSkipB) ? 2 : 1;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.78f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 114;
                        ev.accentGate = 0.95f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 10 || (s == 14 && isTension))
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 48.0f;
                        int deg = barRoot + (isTension ? 7 : 4);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 90;
                        ev.baseGate = 0.75f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.95f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::PolyChordStab)
                {
                    // =========================================================
                    // RUOLO CHORD STABS: Chicago House M1 Piano Stabs in levare
                    // =========================================================
                    if (s == stepPivotA || s == stepSkipA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == stepPivotA) ? 0.0f : 30.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 4; // 9th Stab
                        ev.baseVelocity = 86;
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
                    // RUOLO HARMONY CHORDS: Lush String / Pad Progression
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
                        ev.chordType = 4; // 9th Voicing
                        ev.baseVelocity = 76;
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
                    // RUOLO ACID LINE: Chicago 303 Glides & Squelch
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
                        ev.accentVelocity = 126;
                        ev.accentGate = 0.85f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == stepSkipA || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 26.0f;
                        int deg = barRoot + (s == 14 ? 10 : 3);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1;
                        ev.slide = true;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.62f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.80f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: 90s Drum Machine / Modular Ratchets
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
                        ev.baseVelocity = 96;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 116;
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
                        ev.activationThreshold = (s == 15) ? 42.0f : 55.0f;
                        int deg = barRoot + (s == 15 ? 12 : 7);
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
                    // RUOLO MONO BASS: Chicago Offbeat Bassline
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

                    if (s == stepSkipA || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 18.0f;
                        int deg = barRoot + (s == 14 ? -2 : bMot.degrees[beatIdx]);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.38f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 116;
                        ev.accentGate = 0.46f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 36.0f;
                        int deg = barRoot - 2;
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.40f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s % 2 == 1 && s != 15 && s != stepPivotA && s != stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 56.0f + static_cast<float> (hashVal (gStep + 100) % 15);
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
                        ev.activationThreshold = 72.0f;

                        int nextBar = (bar + 1) % 8;
                        int nextRoot = quantize (kClassicHouseProgressions[progIdx][nextBar]);
                        int turnChoice = static_cast<int> (hashVal (bar * 29 + 7) % 3);
                        int deg = (isFillBar || isTension) ? (nextRoot - 2)
                                                           : (turnChoice == 0 ? barRoot
                                                                              : (turnChoice == 1 ? barRoot - 2
                                                                                                 : barRoot + 7));

                        ev.semitone = quantize (deg);
                        ev.octaveShift = (isFillBar) ? 1 : 0;
                        ev.baseVelocity = 116;
                        ev.baseGate = 0.48f;
                        ev.isOctavePop = (ev.octaveShift > 0);
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 125;
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

//==============================================================================
// Generatore Layered per Italo Disco / Festival EDM
//==============================================================================
class LayeredItaloEdmGenerator
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
        const int edmMotA = static_cast<int> (hashVal (2) % 8);
        const int edmMotB = static_cast<int> (hashVal (3) % 8);

        if (role != InstrumentRole::MonoBass)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kEdmProgressions[progIdx][bar];
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

            const int barRootRaw = kEdmProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barMotIdx = static_cast<int> ((hashVal (bar * 17 + 5) + (isResponse ? edmMotB : edmMotA)) % 8);

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int sub4 = s % 4;

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: Moroder Rolling 16th Octave Arpeggio
                    // =========================================================
                    if (s % 2 == 0)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 18.0f;
                        int deg = barRoot + (sub4 == 0 ? 0 : (sub4 == 2 ? 7 : (barMotIdx % 3 == 0 ? 3 : 12)));
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 112;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s % 2 == 1 && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 48.0f + static_cast<float> (hashVal (gStep) % 15);
                        int deg = barRoot + (sub4 == 1 ? 3 : 10);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 82;
                        ev.baseGate = 0.30f;
                        ev.accentThreshold = 70.0f;
                        ev.accentVelocity = 104;
                        ev.accentGate = 0.38f;
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
                        ev.semitone = quantize (barRoot + (isFillBar ? 12 : 7));
                        ev.octaveShift = isFillBar ? 3 : 2;
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.40f;
                        ev.ratchet = isFillBar ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 124;
                        ev.accentGate = 0.50f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: Euphoric Festival Anthem Hook
                    // =========================================================
                    if (s == 0 || s == 6)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + (s == 0 ? 0 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 96;
                        ev.baseGate = 0.90f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 124;
                        ev.accentGate = 1.15f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 3 || s == 10)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 22.0f;
                        int deg = barRoot + (s == 3 ? 3 : 10);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 90;
                        ev.baseGate = 0.80f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 1.00f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 12 || (s == 14 && isTension))
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 45.0f;
                        int deg = barRoot + (isTension ? 12 : 5);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 3;
                        ev.baseVelocity = 94;
                        ev.baseGate = 0.75f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.95f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::PolyChordStab)
                {
                    // =========================================================
                    // RUOLO CHORD STABS: High-Energy Supersaw Stabs
                    // =========================================================
                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 2 || s == 10) ? 0.0f : 28.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 3; // Drop-2 7th
                        ev.baseVelocity = 90;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 50.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.44f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::HarmonyChords)
                {
                    // =========================================================
                    // RUOLO HARMONY CHORDS: Epic Anthem Pads & Chords
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
                        ev.chordType = 3; // Drop-2 7th Voicing
                        ev.baseVelocity = 84;
                        ev.baseGate = 1.25f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 105;
                        ev.accentGate = 1.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::AcidLine)
                {
                    // =========================================================
                    // RUOLO ACID LINE: High-Energy Acid Trance Glides
                    // =========================================================
                    if (s == 0 || s == 6 || s == 10 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int deg = barRoot + (s == 0 ? 0 : (s == 6 ? 7 : (s == 10 ? 3 : 12)));
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 6 || s == 12) ? 1 : 0;
                        ev.slide = (s == 6 || s == 12);
                        ev.baseVelocity = 106;
                        ev.baseGate = 0.68f;
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
                        ev.activationThreshold = 28.0f;
                        int deg = barRoot + (s == 14 ? 10 : 5);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1;
                        ev.slide = true;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.65f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.82f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: Festival Snare/Synth Buildup Rolls
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.baseVelocity = 98;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 40.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                    if (s == 6 || s == 10 || s == 14 || s == 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 15) ? 40.0f : 55.0f;
                        int deg = barRoot + (s == 15 ? 12 : 7);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = (s == 15) ? 2 : 1;
                        ev.ratchet = (s == 15) ? (isFillBar ? 4 : 3) : 2;
                        ev.baseVelocity = 94;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 65.0f;
                        ev.accentVelocity = 125;
                        ev.accentGate = 0.44f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else
                {
                    // =========================================================
                    // RUOLO MONO BASS: Moroder Rolling 16th Octave Bassline
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 15.0f;
                        int deg = barRoot + ((s == 12 && isResponse) ? 3 : 0);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 106;
                        ev.baseGate = 0.35f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 120;
                        ev.accentGate = 0.44f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == 2 || s == 6 || s == 10 || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 28.0f;
                        int deg = barRoot + ((s == 14 && isTension) ? -2 : 0);
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1;
                        ev.isOctavePop = true;
                        ev.baseVelocity = 102;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 50.0f;
                        ev.accentVelocity = 114;
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
                        ev.activationThreshold = 52.0f + static_cast<float> (hashVal (gStep) % 15);
                        ev.semitone = barRoot;
                        ev.octaveShift = (sub4 == 1) ? 0 : 1;
                        ev.baseVelocity = 84;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 70.0f;
                        ev.accentVelocity = 106;
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
                        ev.activationThreshold = 68.0f;

                        int nextBar = (bar + 1) % 8;
                        int nextRoot = quantize (kEdmProgressions[progIdx][nextBar]);
                        int turnChoice = static_cast<int> (hashVal (bar * 23 + 11) % 3);
                        int deg = (isFillBar || isTension) ? (nextRoot - 2)
                                                           : (turnChoice == 0 ? barRoot
                                                                              : (turnChoice == 1 ? barRoot - 2
                                                                                                 : barRoot + 7));

                        ev.semitone = quantize (deg);
                        ev.octaveShift = (isFillBar) ? 1 : 0;
                        ev.baseVelocity = 116;
                        ev.baseGate = 0.44f;
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
