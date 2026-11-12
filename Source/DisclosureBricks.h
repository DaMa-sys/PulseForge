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
// Mattoncini Statistici e Astratti: DISCLOSURE / UK GARAGE
//==============================================================================

static const int kDisclosureProgressions[8][8] = {
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 0: i -> iv -> v -> i (Latch classic vibe)
    {  0,  0,  8,  8, 10, 10,  0,  0 }, // 1: i -> VI -> VII -> i (White Noise drive)
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 2: i -> bIII -> iv -> v (Soulful bounce)
    {  0,  0,  2,  2,  5,  5,  0,  0 }, // 3: i -> ii -> iv -> i (Dorian jazz groove)
    {  0,  0,  0,  0,  5,  5,  7,  7 }, // 4: i pedal -> iv -> v (Tension build)
    {  0,  0, 10, 10,  5,  5,  7,  7 }, // 5: i -> b7 -> iv -> v (Omen dark groove)
    {  0,  0,  8,  8,  7,  7,  5,  5 }, // 6: i -> VI -> v -> iv (Deep shuffle)
    {  0,  0,  3,  3,  2,  2, 10, 10 }  // 7: i -> bIII -> ii -> b7 (London underground)
};

struct DisclosureRiffCell
{
    int degrees[4];
};

static const DisclosureRiffCell kDisclosureLeadRiffs[8] = {
    {  0,  3,  5,  7 }, // 0: Pentatonica ascendente (Root -> b3 -> 4 -> 5)
    {  7,  5,  3,  0 }, // 1: Pentatonica discendente (5 -> 4 -> b3 -> Root)
    {  0, 10,  7,  5 }, // 2: Appoggio su b7 e 5a
    {  3,  5,  7, 10 }, // 3: Apertura superiore (b3 -> 4 -> 5 -> b7)
    {  0,  0, 12,  0 }, // 4: Rimbalzo d'ottava secco
    {  5,  7, 10, 12 }, // 5: Rincorsa verso l'ottava
    {  0,  3,  2,  0 }, // 6: Sospiro dorico (Root -> b3 -> 2 -> Root)
    {  7, 10,  7,  3 }  // 7: Hook sincopato a rimbalzo
};

static const DisclosureRiffCell kDisclosureBassMotifs[8] = {
    {  0,  0, 12,  0 }, // 0: Rimbalzo d'ottava centrale
    {  0,  5,  0, -2 }, // 1: Spinta di quarta e appoggio b7
    {  0,  3,  5,  7 }, // 2: Walking doriano
    {  0,  0,  0, 12 }, // 3: Salto d'ottava finale
    {  0, -2, -4, -2 }, // 4: Cadenza UKG profonda
    {  0,  7,  5,  3 }, // 5: Discesa pentatonica
    {  0,  0,  3,  0 }, // 6: Sospiro di b3
    {  0, 12,  7, -2 }  // 7: London skip acrobatica
};

//==============================================================================
// Generatore Layered per Disclosure / UK Garage
//==============================================================================
class LayeredDisclosureGenerator
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
        const int riffIdxA = static_cast<int> (hashVal (2) % 8);
        const int riffIdxB = static_cast<int> (hashVal (3) % 8);
        const int bassMotA = static_cast<int> (hashVal (4) % 8);
        const int bassMotB = static_cast<int> (hashVal (5) % 8);

        if (role != InstrumentRole::MonoBass && role != InstrumentRole::PolyChordStab && role != InstrumentRole::LeadHook)
        {
            for (int bar = 0; bar < 8; ++bar)
            {
                const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
                const bool isTension  = (bar == 6);
                const bool isFillBar  = (bar == 7);
                const int barRootRaw  = kDisclosureProgressions[progIdx][bar];
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

            const int barRootRaw = kDisclosureProgressions[progIdx][bar];
            const int barRoot = quantize (barRootRaw);

            const int barRiffIdx = static_cast<int> ((hashVal (bar * 17 + 5) + (isResponse ? riffIdxB : riffIdxA)) % 8);
            const auto& riff = isTension ? kDisclosureLeadRiffs[5] : kDisclosureLeadRiffs[barRiffIdx];

            const int barBassIdx = static_cast<int> ((hashVal (bar * 19 + 7) + (isResponse ? bassMotB : bassMotA)) % 8);
            const auto& bMot = isTension ? kDisclosureBassMotifs[7] : kDisclosureBassMotifs[barBassIdx];

            const int stepPivotA = 2 + static_cast<int> (hashVal (bar * 11 + 1) % 2); // 2 o 3
            const int stepPivotB = 8 + static_cast<int> (hashVal (bar * 11 + 2) % 2); // 8 o 9
            const int stepSkipA  = 5 + static_cast<int> (hashVal (bar * 11 + 3) % 2); // 5 o 6
            const int stepSkipB  = 11 + static_cast<int> (hashVal (bar * 11 + 4) % 3); // 11, 12, 13

            for (int s = 0; s < 16; ++s)
            {
                const int gStep = bar * 16 + s;
                const int beatIdx = s / 4;
                const int sub4 = s % 4;

                if (role == InstrumentRole::ClassicArp)
                {
                    // =========================================================
                    // RUOLO CLASSIC ARP: Bouncy UK Garage Marimba/Pluck Arpeggio
                    // =========================================================
                    if (s == 0 || s == 4 || s == 8 || s == 12)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == 0 || s == 8) ? 0.0f : 18.0f;
                        int rawDeg = barRoot + riff.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 82;
                        ev.baseGate = 0.28f;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 106;
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
                        ev.activationThreshold = 30.0f;
                        int rawDeg = barRoot + riff.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = 2;
                        ev.baseVelocity = 78;
                        ev.baseGate = 0.25f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 102;
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
                        int rawDeg = barRoot + riff.degrees[sub4];
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
                        int rawDeg = barRoot + (isFillBar ? 12 : riff.degrees[3]);
                        ev.semitone = quantize (rawDeg);
                        ev.octaveShift = (isFillBar) ? 3 : 2;
                        ev.baseVelocity = 88;
                        ev.baseGate = 0.35f;
                        ev.ratchet = (isFillBar) ? 2 : 1;
                        ev.accentThreshold = 75.0f;
                        ev.accentVelocity = 110;
                        ev.accentGate = 0.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::LeadHook)
                {
                    // =========================================================
                    // RUOLO LEAD HOOK: Catchy Disclosure vocal/synth lead theme
                    // =========================================================
                    if (s == stepPivotA || s == stepPivotB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 0.0f;
                        int rawDeg = barRoot + riff.degrees[sub4];
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
                        ev.activationThreshold = 22.0f;
                        int rawDeg = barRoot + riff.degrees[sub4];
                        ev.semitone = quantize (rawDeg);
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
                        int rawDeg = barRoot + (isTension ? 7 : 4);
                        ev.semitone = quantize (rawDeg);
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
                    // RUOLO CHORD STABS: Disclosure minor 9th offbeat stabs
                    // =========================================================
                    if (s == stepPivotA || s == stepSkipA || s == stepSkipB)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = (s == stepPivotA) ? 0.0f : ((s == stepSkipA) ? 26.0f : 52.0f);
                        ev.semitone = barRoot;
                        ev.octaveShift = 1;
                        ev.chordType = 4; // 9th Stab
                        ev.baseVelocity = 86;
                        ev.baseGate = 0.32f;
                        ev.accentThreshold = 55.0f;
                        ev.accentVelocity = 115;
                        ev.accentGate = 0.42f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::HarmonyChords)
                {
                    // =========================================================
                    // RUOLO HARMONY CHORDS: Lush warm Rhodes/organ pads
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
                        ev.baseGate = 1.20f;
                        ev.accentThreshold = 60.0f;
                        ev.accentVelocity = 98;
                        ev.accentGate = 1.45f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::AcidLine)
                {
                    // =========================================================
                    // RUOLO ACID LINE: Bouncy Garage 303 Glides & Squelch
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
                        ev.baseVelocity = 105;
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
                        ev.accentVelocity = 122;
                        ev.accentGate = 0.80f;
                        events.push_back (ev);
                        continue;
                    }
                }
                else if (role == InstrumentRole::ModularRatchet)
                {
                    // =========================================================
                    // RUOLO MODULAR RATCHET: 2-step Garage Stutter Ratchets
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
                    // RUOLO MONO BASS: BOUNCY UK GARAGE ORGAN / SUB BASS
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
                        ev.baseVelocity = 110;
                        ev.baseGate = 0.42f;
                        ev.accentThreshold = 35.0f;
                        ev.accentVelocity = 124;
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
                        ev.activationThreshold = 20.0f;
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 1; // Salto d'ottava
                        ev.baseVelocity = 104;
                        ev.baseGate = 0.36f;
                        ev.isOctavePop = true;
                        ev.accentThreshold = 45.0f;
                        ev.accentVelocity = 118;
                        ev.accentGate = 0.45f;
                        events.push_back (ev);
                        continue;
                    }

                    if (s == stepSkipB || s == 14)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 38.0f;
                        int deg = barRoot + (s == 14 ? -2 : bMot.degrees[beatIdx]);
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

                    if (s % 2 == 1 && s != stepPivotA && s != stepSkipA && s != stepSkipB && s != 15)
                    {
                        LayeredCandidateEvent ev;
                        ev.bar = bar;
                        ev.stepInBar = s;
                        ev.globalStep = gStep;
                        ev.activationThreshold = 58.0f + static_cast<float> (hashVal (gStep + 200) % 15);
                        int deg = barRoot + bMot.degrees[beatIdx];
                        ev.semitone = quantize (deg);
                        ev.octaveShift = 0;
                        ev.baseVelocity = 52;
                        ev.baseGate = 0.25f;
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
                        int nextRoot = quantize (kDisclosureProgressions[progIdx][nextBar]);
                        int turnChoice = static_cast<int> (hashVal (bar * 29 + 13) % 3);
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
