#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <functional>
#include "PatternData.h"
#include "GenreProfiles.h"
#include "LayeredPatternGenerator.h"

namespace PulseForge
{

//==============================================================================
// MOTIF DNA (Architettura Archetipi Curati Sequentia PLF)
//==============================================================================
struct PLFArtistMotifDNA
{
    const char* name;
    int numSteps;
    std::vector<int> activeSteps;
    std::vector<int> scaleDegrees;
    std::vector<int> octaves;
    std::vector<int> velocities;
    std::vector<bool> accents;
    std::vector<bool> slides;
    std::vector<float> nudges;
};

//==============================================================================
// 1. KIASMOS (Hypnotic Pulse / Minimalist Neo-Classical)
//==============================================================================
static const int kKiasmosProgressions[8][8] = {
    {  0,  0,  8,  8,  3,  3, 10, 10 }, // 0: i -> VI -> III -> VII (Looped Minimalist)
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 1: i -> iv -> v -> i (Blurred Undulation)
    {  0,  0,  3,  3,  8,  8, 10, 10 }, // 2: i -> bIII -> VI -> VII (Held Staccato)
    {  0,  0,  8,  8,  5,  5,  7,  7 }, // 3: i -> VI -> iv -> v (Swayed Prime)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 4: i -> b7 -> b6 -> b7 (Thrown Cascade)
    {  0,  0,  2,  2,  5,  5,  0,  0 }, // 5: i -> ii -> iv -> i (Ornt Deep Lead)
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 6: i -> bIII -> iv -> v (Burst Neo-Classical)
    {  0,  0,  0,  0,  8,  8, 10, 10 }  // 7: i pedal -> VI -> VII (Flite Micro-Cascade)
};

static const PLFArtistMotifDNA kKiasmosArchetypes[10] = {
    // 1: Looped Minimalist (14 steps)
    { "Looped Minimalist", 14,
      { 0, 1, 3, 4, 6, 7, 9, 10, 12, 13 },
      { 0, 2, 0, 4, 2, 0, 6, 4, 2, 0 },
      { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
      { 78, 64, 82, 90, 96, 76, 114, 102, 85, 68 },
      { false, false, false, false, false, false, true, false, false, false },
      { false, true, false, false, true, false, false, false, false, false },
      { 0.0f, 1.8f, -0.5f, 2.2f, 0.2f, 1.5f, -0.8f, 2.0f, 0.4f, 1.9f } },

    // 2: Blurred Undulation (12 steps)
    { "Blurred Undulation", 12,
      { 0, 1, 2, 4, 5, 6, 8, 9, 10 },
      { 0, 2, 4, 0, 2, 4, 2, 4, 6 },
      { 0, 0, 0, 0, 0, 0, 0, 1, 1 },
      { 82, 70, 88, 76, 92, 106, 88, 118, 95 },
      { false, false, false, false, false, true, false, true, false },
      { false, false, true, false, false, true, false, false, false },
      { -0.5f, 2.0f, 0.0f, 1.8f, -0.2f, 0.5f, 2.4f, -0.6f, 1.5f } },

    // 3: Held Staccato (10 steps)
    { "Held Staccato", 10,
      { 0, 1, 3, 4, 6, 7, 9 },
      { 0, 1, 2, 4, 2, 1, 0 },
      { 0, 0, 0, 0, 0, 0, 0 },
      { 92, 65, 84, 112, 88, 60, 96 },
      { false, false, false, true, false, false, false },
      { false, false, false, false, false, false, false },
      { 0.0f, 1.5f, 0.5f, -0.5f, 1.8f, 0.2f, 2.0f } },

    // 4: Swayed Prime (13 steps)
    { "Swayed Prime", 13,
      { 0, 2, 3, 5, 7, 8, 10, 12 },
      { 0, 2, 4, 6, 4, 2, 1, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 85, 72, 94, 116, 98, 80, 68, 88 },
      { false, false, false, true, false, false, false, false },
      { false, true, false, false, true, false, false, false },
      { 0.2f, 2.1f, -0.4f, 0.8f, 2.5f, -0.2f, 1.9f, 0.0f } },

    // 5: Thrown Cascade (11 steps)
    { "Thrown Cascade", 11,
      { 0, 2, 4, 6, 7, 9, 10 },
      { 0, 4, 2, 6, 7, 4, 2 },
      { 0, 0, 0, 1, 1, 0, 0 },
      { 95, 80, 88, 118, 104, 76, 82 },
      { true, false, false, true, false, false, false },
      { false, false, true, false, true, false, false },
      { 0.0f, 1.8f, 0.4f, -0.6f, 2.2f, 0.2f, 1.7f } },

    // 6: Ornt Deep Lead (9 steps)
    { "Ornt Deep Lead", 9,
      { 0, 2, 3, 5, 7, 8 },
      { 0, 2, 4, 2, 1, 0 },
      { 0, 0, 0, 0, 0, 0 },
      { 110, 78, 92, 122, 86, 72 },
      { true, false, false, true, false, false },
      { false, true, false, false, true, false },
      { 0.0f, 1.4f, -0.8f, 0.2f, 1.6f, 0.5f } },

    // 7: Burst Neo-Classical (15 steps)
    { "Burst Neo-Classical", 15,
      { 0, 1, 3, 5, 6, 8, 10, 12, 13 },
      { 0, 2, 4, 2, 6, 4, 7, 6, 4 },
      { 0, 0, 0, 0, 1, 0, 1, 1, 0 },
      { 88, 74, 96, 82, 115, 92, 124, 108, 84 },
      { false, false, false, false, true, false, true, false, false },
      { false, false, true, false, true, false, false, true, false },
      { -0.2f, 1.6f, 0.4f, 1.8f, -0.5f, 2.0f, -0.8f, 1.5f, 0.8f } },

    // 8: Shed Syncopated (7 steps)
    { "Shed Syncopated", 7,
      { 0, 2, 3, 5 },
      { 0, 4, 2, 0 },
      { 0, 0, 0, 0 },
      { 114, 70, 108, 64 },
      { true, false, true, false },
      { false, true, false, false },
      { 0.0f, 2.2f, -0.5f, 1.6f } },

    // 9: Grown Analog Pulse (13 steps)
    { "Grown Analog Pulse", 13,
      { 0, 1, 3, 4, 6, 8, 9, 11, 12 },
      { 0, 0, 2, 0, 4, 2, 0, 4, 2 },
      { 0, 0, 0, 0, 0, 0, 0, 1, 0 },
      { 82, 68, 86, 74, 112, 94, 88, 120, 90 },
      { false, false, false, false, true, false, false, true, false },
      { false, false, true, false, false, true, false, false, false },
      { 0.2f, 1.8f, -0.2f, 1.5f, -0.4f, 2.0f, 0.5f, -0.8f, 1.4f } },

    // 10: Flite Micro-Cascade (11 steps)
    { "Flite Micro-Cascade", 11,
      { 0, 1, 3, 5, 6, 8, 10 },
      { 0, 2, 4, 6, 4, 2, 0 },
      { 0, 0, 0, 1, 0, 0, 0 },
      { 90, 76, 98, 124, 102, 84, 66 },
      { false, false, false, true, false, false, false },
      { false, true, false, true, false, true, false },
      { 0.0f, 1.5f, 0.4f, -0.6f, 2.0f, 0.2f, 1.8f } }
};

//==============================================================================
// 2. STEPHAN BODZIN (Melodic Drive / Moog Techno)
//==============================================================================
static const int kBodzinProgressions[8][8] = {
    {  0,  0,  8,  8, 10, 10,  0,  0 }, // 0: i -> VI -> VII -> i (Boavista Anthem)
    {  0,  0,  5,  5,  8,  8,  7,  7 }, // 1: i -> iv -> VI -> v (Singularity Drive)
    {  0,  0,  0,  0,  8,  8, 10, 10 }, // 2: i -> VI -> VII (Strand Journey)
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 3: i -> bIII -> iv -> v (Earth Sub)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 4: i -> VII -> VI -> VII (Zodiac Peak)
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 5: i -> iv -> v -> i (Wir Soaring)
    {  0,  0,  3,  3,  5,  5, 10, 10 }, // 6: i -> bIII -> iv -> VII (Liebe Ist Vintage)
    {  0,  0,  0,  0,  0,  0, 10, 10 }  // 7: i drone -> VII Turnaround (Diamant)
};

static const PLFArtistMotifDNA kBodzinArchetypes[10] = {
    // 1: Singularity 14-Step (14 steps)
    { "Singularity 14-Step", 14,
      { 0, 1, 2, 3, 5, 6, 7, 9, 10, 12, 13 },
      { 0, 0, 0, 2, 0, 4, 0, 6, 0, 4, 2 },
      { 0, 0, 0, 1, 0, 1, 0, 2, 0, 1, 0 },
      { 98, 88, 86, 125, 92, 122, 90, 127, 92, 118, 94 },
      { false, false, false, true, false, true, false, true, false, true, false },
      { false, false, false, true, false, true, false, true, false, false, false },
      { -1.0f, 0.5f, 0.2f, -1.5f, 0.8f, -1.2f, 0.4f, -1.8f, 0.6f, -1.0f, 0.2f } },

    // 2: Strand Engine (12 steps)
    { "Strand Engine", 12,
      { 0, 1, 2, 4, 5, 7, 8, 10, 11 },
      { 0, 0, 2, 0, 4, 0, 6, 4, 2 },
      { 0, 0, 0, 0, 1, 0, 1, 1, 0 },
      { 110, 88, 96, 92, 124, 90, 126, 112, 95 },
      { true, false, false, false, true, false, true, false, false },
      { false, false, true, false, true, false, false, true, false },
      { -0.8f, 0.4f, 0.0f, 0.6f, -1.4f, 0.5f, -1.6f, 0.2f, 0.4f } },

    // 3: Boavista Lead (15 steps)
    { "Boavista Lead", 15,
      { 0, 2, 3, 5, 6, 8, 9, 11, 12, 14 },
      { 0, 2, 0, 4, 2, 6, 4, 7, 6, 4 },
      { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
      { 104, 86, 92, 112, 96, 122, 108, 127, 115, 90 },
      { false, false, false, true, false, true, false, true, false, false },
      { false, true, false, false, true, false, true, false, true, false },
      { 0.0f, 1.2f, -0.4f, 0.8f, 1.5f, -0.8f, 1.0f, -1.2f, 0.6f, 1.8f } },

    // 4: Zodiac Bass (13 steps)
    { "Zodiac Bass", 13,
      { 0, 1, 3, 4, 6, 7, 9, 10, 12 },
      { 0, 0, 2, 0, 4, 0, 2, 4, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 1, 0 },
      { 112, 86, 98, 90, 124, 92, 96, 120, 88 },
      { true, false, false, false, true, false, false, true, false },
      { false, false, true, false, true, false, false, true, false },
      { -0.5f, 0.4f, 0.0f, 0.8f, -1.2f, 0.6f, 0.2f, -1.0f, 0.4f } },

    // 5: Earth Sub (10 steps)
    { "Earth Sub", 10,
      { 0, 1, 2, 4, 5, 7, 8 },
      { 0, 0, 0, 4, 0, 6, 2 },
      { 0, 0, 0, 1, 0, 1, 0 },
      { 115, 88, 90, 126, 92, 124, 94 },
      { true, false, false, true, false, true, false },
      { false, false, false, true, false, true, false },
      { -0.8f, 0.3f, 0.5f, -1.5f, 0.6f, -1.4f, 0.2f } },

    // 6: Wir Soaring Octaves (11 steps)
    { "Wir Soaring Octaves", 11,
      { 0, 2, 4, 5, 7, 9, 10 },
      { 0, 2, 4, 6, 4, 2, 0 },
      { 0, 0, 1, 1, 1, 0, 0 },
      { 106, 88, 120, 126, 114, 92, 78 },
      { false, false, true, true, false, false, false },
      { false, true, false, true, false, true, false },
      { 0.0f, 1.2f, -0.6f, -1.0f, 0.8f, 1.6f, 0.4f } },

    // 7: Liebe Ist Vintage (9 steps)
    { "Liebe Ist Vintage", 9,
      { 0, 2, 3, 5, 7, 8 },
      { 0, 2, 0, 4, 2, 0 },
      { 0, 0, 0, 1, 0, 0 },
      { 118, 82, 94, 124, 96, 76 },
      { true, false, false, true, false, false },
      { false, true, false, true, false, false },
      { -0.4f, 1.6f, -0.2f, -1.0f, 1.2f, 0.6f } },

    // 8: Phobos Pulse (7 steps)
    { "Phobos Pulse", 7,
      { 0, 1, 3, 4, 5 },
      { 0, 0, 2, 0, 4 },
      { 0, 0, 0, 0, 1 },
      { 122, 86, 124, 90, 126 },
      { true, false, true, false, true },
      { false, false, true, false, true },
      { -0.6f, 0.2f, -1.2f, 0.4f, -1.4f } },

    // 9: River Squelch (13 steps)
    { "River Squelch", 13,
      { 0, 1, 2, 4, 6, 7, 9, 11, 12 },
      { 0, 0, 2, 4, 2, 6, 4, 7, 4 },
      { 0, 0, 0, 0, 0, 1, 1, 1, 0 },
      { 108, 92, 96, 120, 88, 126, 102, 124, 90 },
      { false, false, false, true, false, true, false, true, false },
      { false, false, true, true, false, true, false, true, false },
      { -0.5f, 0.4f, 0.0f, -1.2f, 0.8f, -1.5f, 0.6f, -1.2f, 0.2f } },

    // 10: Diamant Peak Swell (14 steps)
    { "Diamant Peak Swell", 14,
      { 0, 1, 3, 5, 6, 8, 9, 11, 12, 13 },
      { 0, 0, 4, 0, 6, 4, 7, 6, 4, 2 },
      { 0, 0, 1, 0, 1, 1, 2, 1, 1, 0 },
      { 114, 88, 124, 92, 127, 106, 127, 118, 96, 84 },
      { true, false, true, false, true, false, true, false, false, false },
      { false, false, true, false, true, false, true, false, true, false },
      { -0.6f, 0.3f, -1.4f, 0.5f, -1.6f, 0.2f, -1.8f, 0.4f, 0.8f, 0.2f } }
};

//==============================================================================
// 3. BURIAL (Burial Garage / Untrue)
//==============================================================================
static const int kBurialProgressions[8][8] = {
    {  0,  0,  3,  3,  5,  5,  0,  0 }, // 0: i -> bIII -> iv -> i (Archangel Skank)
    {  0,  0, 10, 10,  8,  8, 10, 10 }, // 1: i -> b7 -> b6 -> b7 (Untrue Shadow)
    {  0,  0,  8,  8, 10, 10,  0,  0 }, // 2: i -> VI -> VII -> i (Near Dark Drift)
    {  0,  0,  5,  5,  7,  7,  0,  0 }, // 3: i -> iv -> v -> i (Rival Dealer)
    {  0,  0,  3,  3,  8,  8,  7,  7 }, // 4: i -> bIII -> VI -> v (Kindred)
    {  0,  0, 10, 10,  0,  0, 10, 10 }, // 5: i -> b7 -> i -> b7 (Loner)
    {  0,  0,  0,  0,  5,  5, 10, 10 }, // 6: i pedal -> iv -> b7 (Etched Headplate)
    {  0,  0,  7,  7,  5,  5,  0,  0 }  // 7: i -> v -> iv -> i (Stolen Dog)
};

static const PLFArtistMotifDNA kBurialArchetypes[10] = {
    // 1: Archangel Skank (11 steps)
    { "Archangel Skank", 11,
      { 0, 2, 4, 6, 8, 10 },
      { 0, 2, 4, 6, 4, 0 },
      { 0, 0, 0, 1, 0, 0 },
      { 116, 46, 88, 122, 92, 42 },
      { true, false, false, true, false, false },
      { false, false, true, false, false, false },
      { -1.0f, -3.2f, 4.5f, -0.5f, 3.8f, -3.8f } },

    // 2: Untrue Shadow (13 steps)
    { "Untrue Shadow", 13,
      { 0, 2, 4, 6, 8, 10, 12 },
      { 0, 4, 6, 7, 4, 2, 0 },
      { 0, 0, 1, 1, 0, 0, 0 },
      { 118, 48, 96, 124, 90, 50, 84 },
      { true, false, false, true, false, false, false },
      { false, false, true, false, false, false, false },
      { 0.0f, -4.0f, 5.0f, -0.8f, 4.2f, -3.5f, 1.8f } },

    // 3: Ghost Hardware (7 steps)
    { "Ghost Hardware", 7,
      { 0, 2, 4, 6 },
      { 0, 2, 4, 2 },
      { 0, 0, 1, 0 },
      { 112, 42, 118, 52 },
      { true, false, true, false },
      { false, false, true, false },
      { 0.0f, -3.6f, 4.8f, -2.8f } },

    // 4: Near Dark Drift (15 steps)
    { "Near Dark Drift", 15,
      { 0, 3, 5, 8, 10, 12, 14 },
      { 0, 2, 4, 6, 7, 4, 0 },
      { 0, 0, 0, 1, 1, 0, 0 },
      { 115, 86, 50, 122, 108, 48, 90 },
      { true, false, false, true, false, false, false },
      { false, true, false, false, true, false, false },
      { -0.6f, 3.8f, -3.2f, -0.4f, 4.2f, -3.0f, 1.5f } },

    // 5: Rival Dealer Sub (9 steps)
    { "Rival Dealer Sub", 9,
      { 0, 2, 4, 6, 8 },
      { 0, 1, 0, 4, 0 },
      { 0, 0, 0, 0, 0 },
      { 120, 84, 52, 116, 44 },
      { true, false, false, true, false },
      { false, true, false, false, false },
      { 0.0f, 3.5f, -3.0f, 0.4f, -4.2f } },

    // 6: Kindred Industrial (15 steps)
    { "Kindred Industrial", 15,
      { 0, 2, 4, 6, 9, 11, 13, 14 },
      { 0, 1, 0, 2, 0, 4, 2, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 124, 50, 92, 118, 48, 126, 88, 42 },
      { true, false, false, true, false, true, false, false },
      { false, false, true, false, false, true, false, false },
      { -0.8f, -3.5f, 3.2f, 0.4f, -4.0f, -0.8f, 3.5f, -2.6f } },

    // 7: Loner Rave-Garage (12 steps)
    { "Loner Rave-Garage", 12,
      { 0, 2, 3, 5, 6, 8, 9, 11 },
      { 0, 0, 2, 4, 2, 6, 4, 2 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 122, 88, 56, 124, 90, 125, 78, 52 },
      { true, false, false, true, false, true, false, false },
      { false, false, true, false, true, false, false, false },
      { 0.0f, 3.2f, -2.5f, 0.2f, 3.5f, -0.5f, 2.8f, -3.2f } },

    // 8: In McDonalds Suspended (7 steps)
    { "In McDonalds Suspended", 7,
      { 0, 2, 4, 5 },
      { 0, 4, 7, 4 },
      { 0, 0, 1, 0 },
      { 110, 44, 116, 48 },
      { true, false, true, false },
      { false, true, false, false },
      { 0.0f, -4.5f, 3.8f, -2.0f } },

    // 9: Etched Headplate (13 steps)
    { "Etched Headplate", 13,
      { 0, 3, 4, 6, 8, 10, 11 },
      { 0, 2, 0, 4, 6, 4, 2 },
      { 0, 0, 0, 0, 1, 0, 0 },
      { 118, 52, 86, 122, 104, 46, 82 },
      { true, false, false, true, false, false, false },
      { false, false, true, false, false, true, false },
      { -0.5f, 4.0f, -3.5f, -0.6f, 4.5f, -3.2f, 1.8f } },

    // 10: Stolen Dog Music Box (11 steps)
    { "Stolen Dog Music Box", 11,
      { 0, 2, 4, 6, 7, 9 },
      { 0, 4, 2, 7, 6, 4 },
      { 0, 0, 0, 1, 1, 0 },
      { 114, 42, 90, 120, 88, 40 },
      { true, false, false, true, false, false },
      { false, true, false, false, true, false },
      { 0.0f, -3.8f, 4.2f, -0.4f, 3.6f, -2.5f } }
};

//==============================================================================
// 4. LAOUM (Emotive Modular / Luca Fiorillo)
//==============================================================================
static const int kLaoumProgressions[8][8] = {
    {  0,  0,  8,  8,  5,  5,  7,  7 }, // 0: i -> VI -> iv -> v (those aren't mountains)
    {  0,  0,  5,  5,  3,  3,  0,  0 }, // 1: i -> iv -> bIII -> i (felt my love)
    {  0,  0, 10, 10,  8,  8,  7,  7 }, // 2: i -> b7 -> VI -> v (elements)
    {  0,  0,  3,  3,  5,  5,  7,  7 }, // 3: i -> bIII -> iv -> v (birds of passage)
    {  0,  0,  2,  2,  5,  5,  0,  0 }, // 4: i -> ii -> iv -> i (again)
    {  0,  0,  8,  8, 10, 10,  0,  0 }, // 5: i -> VI -> VII -> i (acousmatic stories)
    {  0,  0,  3,  3,  2,  2,  0,  0 }, // 6: i -> bIII -> ii -> i (horizon bloom)
    {  0,  0,  0,  0,  5,  5,  7,  7 }  // 7: i pedal -> iv -> v (Sector 01)
};

static const PLFArtistMotifDNA kLaoumArchetypes[16] = {
    // 1: those aren't mountains (14 steps)
    { "those aren't mountains", 14,
      { 0, 1, 3, 5, 6, 8, 10, 11, 13 },
      { 0, 2, 4, 6, 4, 2, 7, 6, 4 },
      { 0, 0, 0, 0, 0, 0, 1, 1, 0 },
      { 108, 74, 88, 116, 90, 78, 124, 110, 82 },
      { true, false, false, true, false, false, true, false, false },
      { false, false, true, false, false, true, false, true, false },
      { 0.0f, 1.4f, 2.2f, -0.6f, 1.8f, 2.5f, -0.8f, 1.2f, 2.0f } },

    // 2: chance (11 steps)
    { "chance", 11,
      { 0, 2, 4, 5, 7, 9, 10 },
      { 0, 2, 0, 4, 2, 6, 4 },
      { 0, 0, 0, 0, 0, 1, 0 },
      { 114, 46, 86, 110, 50, 122, 76 },
      { true, false, false, true, false, true, false },
      { false, false, false, true, false, false, false },
      { 0.0f, -3.4f, 3.8f, 0.5f, -2.8f, -0.6f, 3.5f } },

    // 3: felt my love (13 steps)
    { "felt my love", 13,
      { 0, 2, 3, 5, 7, 8, 10, 12 },
      { 0, 2, 4, 6, 7, 6, 4, 2 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 112, 76, 92, 116, 126, 96, 82, 70 },
      { true, false, false, true, true, false, false, false },
      { false, true, false, true, false, true, false, false },
      { 0.2f, 2.0f, 1.2f, -0.4f, -0.8f, 1.8f, 2.4f, 1.0f } },

    // 4: elements (15 steps)
    { "elements", 15,
      { 0, 1, 3, 4, 6, 8, 9, 11, 13, 14 },
      { 0, 2, 4, 2, 6, 4, 7, 6, 4, 0 },
      { 0, 0, 0, 0, 1, 0, 1, 1, 0, 0 },
      { 110, 80, 88, 84, 120, 94, 125, 110, 88, 74 },
      { true, false, false, false, true, false, true, false, false, false },
      { false, false, true, false, true, false, true, false, true, false },
      { -0.4f, 1.5f, 0.6f, 1.0f, -0.8f, 0.4f, -1.0f, 0.8f, 1.6f, 2.0f } },

    // 5: birds of passage (10 steps)
    { "birds of passage", 10,
      { 0, 2, 3, 5, 6, 8, 9 },
      { 0, 4, 2, 7, 6, 4, 2 },
      { 0, 0, 0, 1, 1, 0, 0 },
      { 116, 84, 48, 122, 104, 78, 46 },
      { true, false, false, true, false, false, false },
      { false, true, false, false, true, false, false },
      { 0.0f, 2.2f, -3.0f, -0.5f, 1.8f, 2.5f, -2.6f } },

    // 6: over (9 steps)
    { "over", 9,
      { 0, 2, 3, 5, 6, 8 },
      { 0, 2, 0, 4, 2, 0 },
      { 0, 0, 0, 0, 0, 0 },
      { 120, 82, 54, 124, 96, 68 },
      { true, false, false, true, false, false },
      { false, true, false, true, false, false },
      { 0.0f, 1.8f, -1.5f, 0.2f, 2.4f, -1.0f } },

    // 7: again (14 steps)
    { "again", 14,
      { 0, 1, 3, 4, 6, 8, 10, 11, 12 },
      { 0, 2, 4, 2, 6, 4, 7, 6, 4 },
      { 0, 0, 0, 0, 0, 0, 1, 1, 0 },
      { 112, 78, 90, 82, 118, 96, 126, 108, 86 },
      { true, false, false, false, true, false, true, false, false },
      { false, false, true, false, false, true, false, true, false },
      { -0.3f, 1.6f, 0.8f, 1.2f, -0.6f, 2.0f, -0.8f, 1.4f, 2.2f } },

    // 8: acousmatic stories (12 steps)
    { "acousmatic stories", 12,
      { 0, 2, 4, 5, 7, 9, 10 },
      { 0, 4, 2, 6, 4, 2, 0 },
      { 0, 0, 0, 1, 0, 0, 0 },
      { 114, 72, 88, 120, 92, 76, 58 },
      { true, false, false, true, false, false, false },
      { false, true, false, false, true, false, false },
      { 0.0f, 2.5f, 1.0f, -0.4f, 2.8f, 1.6f, -1.8f } },

    // 9: scenario (11 steps)
    { "scenario", 11,
      { 0, 1, 3, 5, 7, 8, 10 },
      { 0, 1, 4, 2, 5, 4, 1 },
      { 0, 0, 0, 0, 1, 0, 0 },
      { 118, 86, 94, 122, 108, 88, 70 },
      { true, false, false, true, false, false, false },
      { false, true, false, true, false, true, false },
      { -0.4f, 1.2f, -0.6f, 0.4f, 2.2f, 0.8f, 1.5f } },

    // 10: horizon bloom (13 steps)
    { "horizon bloom", 13,
      { 0, 2, 3, 5, 7, 8, 10, 12 },
      { 0, 2, 4, 6, 7, 6, 4, 2 },
      { 0, 0, 0, 1, 1, 1, 0, 0 },
      { 110, 80, 92, 124, 127, 112, 94, 78 },
      { true, false, false, true, true, false, false, false },
      { false, false, true, false, true, false, true, false },
      { 0.0f, 1.8f, 0.8f, -0.5f, -0.8f, 1.4f, 2.2f, 1.0f } },

    // 11: Sector 01 (13 steps)
    { "Sector 01", 13,
      { 0, 1, 3, 5, 6, 8, 10, 11 },
      { 0, 2, 0, 4, 2, 6, 4, 1 },
      { 0, 0, 0, 1, 0, 1, 0, 0 },
      { 122, 70, 88, 126, 76, 124, 94, 60 },
      { true, false, false, true, false, true, false, false },
      { false, false, true, false, false, true, false, false },
      { 0.0f, -2.8f, 2.2f, -0.6f, -1.8f, 0.4f, 2.5f, -3.2f } },

    // 12: quiet and loud (11 steps)
    { "quiet and loud", 11,
      { 0, 2, 4, 6, 7, 9 },
      { 0, 2, 4, 7, 4, 2 },
      { 0, 0, 0, 1, 0, 0 },
      { 127, 42, 65, 126, 55, 48 },
      { true, false, false, true, false, false },
      { false, true, false, false, true, false },
      { 0.0f, 3.6f, -2.5f, -0.8f, 2.8f, -3.0f } },

    // 13: Charles Babbage (7 steps)
    { "Charles Babbage", 7,
      { 0, 1, 3, 4, 6 },
      { 0, 4, 2, 6, 4 },
      { 0, 0, 0, 1, 0 },
      { 124, 88, 105, 126, 78 },
      { true, false, false, true, false },
      { false, false, true, false, false },
      { -0.6f, 1.8f, -1.2f, 0.4f, 2.2f } },

    // 14: eros mos (14 steps)
    { "eros mos", 14,
      { 0, 2, 3, 5, 7, 8, 10, 12 },
      { 0, 2, 4, 6, 7, 6, 4, 2 },
      { 0, 0, 0, 0, 1, 1, 0, 0 },
      { 114, 84, 92, 118, 125, 108, 92, 80 },
      { true, false, false, true, true, false, false, false },
      { false, true, false, true, true, true, false, false },
      { 0.0f, 2.4f, 1.2f, -0.5f, 0.2f, 2.0f, 1.6f, -1.2f } },

    // 15: Settembre (12 steps)
    { "Settembre", 12,
      { 0, 2, 4, 5, 7, 9, 11 },
      { 0, 2, 3, 5, 4, 2, 0 },
      { 0, 0, 0, 0, 1, 0, 0 },
      { 116, 78, 86, 122, 110, 84, 62 },
      { true, false, false, true, false, false, false },
      { false, true, false, false, true, false, false },
      { 0.2f, 3.8f, 1.5f, -0.6f, 2.5f, 0.4f, -2.8f } },

    // 16: from the jar (10 steps)
    { "from the jar", 10,
      { 0, 2, 3, 5, 7, 8 },
      { 0, 4, 2, 0, 6, 4 },
      { 0, 0, 0, 0, 1, 0 },
      { 120, 52, 94, 76, 125, 68 },
      { true, false, false, false, true, false },
      { false, false, true, false, false, true },
      { -0.4f, -3.5f, 2.8f, 1.0f, -0.8f, 3.2f } }
};

//==============================================================================
// HELPER PER LA CONVERSIONE ARCHEOTIPO -> CANDIDATE EVENTS (8 BATTUTE)
//==============================================================================
inline void buildPLFArchetypeCandidates (std::vector<LayeredCandidateEvent>& events,
                                        const PLFArtistMotifDNA& dna,
                                        int bar,
                                        int barRoot,
                                        InstrumentRole role,
                                        ScaleType scale,
                                        std::function<int(int)> quantize,
                                        uint32_t barSeed)
{
    const int n = juce::jlimit (1, 16, dna.numSteps);
    const int halfN = n / 2;
    const size_t count = dna.activeSteps.size();

    auto hashVal = [barSeed] (uint32_t salt) -> uint32_t
    {
        uint32_t h = barSeed ^ (salt * 0x45d9f3b);
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        return (h >> 16) ^ h;
    };

    for (size_t k = 0; k < count; ++k)
    {
        const int s = dna.activeSteps[k] % 16;
        const int gStep = bar * 16 + s;

        const int velRaw = (k < dna.velocities.size()) ? dna.velocities[k] : 90;
        const int velJitter = static_cast<int> (hashVal (static_cast<uint32_t> (gStep * 17 + 5)) % 7) - 3;
        const int vel = juce::jlimit (1, 127, velRaw + velJitter);

        const bool acc  = (k < dna.accents.size()) ? dna.accents[k] : false;
        const bool sld  = (k < dna.slides.size()) ? dna.slides[k] : false;
        const int oct   = (k < dna.octaves.size()) ? dna.octaves[k] : 0;
        const int degRaw = (k < dna.scaleDegrees.size()) ? dna.scaleDegrees[k] : 0;

        // Metric Priority del Dev Kit originale
        int priority = 0;
        if (s == 0)                          priority = 0; // Primary downbeat
        else if (s == halfN)                 priority = 1; // Midpoint anchor
        else if (acc)                        priority = 2; // Accented peaks
        else if (s % 4 == 0)                 priority = 3; // Quarter note pulses
        else if (s % 2 == 0)                 priority = 4; // Eighth note pulses
        else if (vel < 60)                   priority = 12;// Ghost brushes
        else                                 priority = 5 + (s % 5);

        // Mappatura monotona su COMPLEXITY (0..100)
        float activation = 0.0f;
        if (priority <= 1)
            activation = 0.0f;
        else if (priority == 2)
            activation = 20.0f;
        else if (priority == 3)
            activation = 28.0f;
        else if (priority == 4)
            activation = 38.0f;
        else if (priority >= 5 && priority <= 9)
            activation = 46.0f + static_cast<float> (priority - 5) * 5.5f;
        else
            activation = 76.0f;

        LayeredCandidateEvent ev;
        ev.bar = bar;
        ev.stepInBar = s;
        ev.globalStep = gStep;
        ev.activationThreshold = activation;

        // Pitch & Diatonic Mapping
        const int pitchDeg = barRoot + scaleDegreeToSemitone (degRaw, scale);
        ev.semitone = quantize (pitchDeg);

        // Octave range in base al ruolo
        if (role == InstrumentRole::MonoBass)
            ev.octaveShift = oct - 1;
        else
            ev.octaveShift = oct + (role == InstrumentRole::ClassicArp ? 1 : 0);

        // Harmonic Function per Shared Harmonic Context
        const int dMod = ((degRaw % 7) + 7) % 7;
        if (dMod == 0)      ev.harmonicFunc = (degRaw >= 7) ? HarmonicFunction::OctavePop : HarmonicFunction::Root;
        else if (dMod == 1) ev.harmonicFunc = HarmonicFunction::Ninth;
        else if (dMod == 2) ev.harmonicFunc = HarmonicFunction::Third;
        else if (dMod == 3) ev.harmonicFunc = HarmonicFunction::Eleventh;
        else if (dMod == 4) ev.harmonicFunc = HarmonicFunction::Fifth;
        else if (dMod == 5) ev.harmonicFunc = HarmonicFunction::Passing;
        else if (dMod == 6) ev.harmonicFunc = HarmonicFunction::Seventh;

        // Dynamics & Articulation
        ev.baseVelocity = vel;
        ev.accentVelocity = juce::jmax (116, vel + 12);
        ev.accentThreshold = acc ? 22.0f : 80.0f;

        ev.baseGate = 0.72f;
        ev.slide = sld;

        events.push_back (ev);
    }

    // Aggiunta ghost notes organiche per arricchire il groove a complessità medio-alta
    const int ghostStepA = static_cast<int> ((hashVal (static_cast<uint32_t> (bar * 17 + 3)) % 15) + 1);
    const int ghostStepB = static_cast<int> ((hashVal (static_cast<uint32_t> (bar * 23 + 11)) % 15) + 1);

    auto tryAddGhost = [&] (int gStepInBar, float thresh)
    {
        for (const auto& existing : events)
        {
            if (existing.bar == bar && existing.stepInBar == gStepInBar)
                return;
        }
        LayeredCandidateEvent gEv;
        gEv.bar = bar;
        gEv.stepInBar = gStepInBar;
        gEv.globalStep = bar * 16 + gStepInBar;
        gEv.activationThreshold = thresh;
        gEv.semitone = quantize (barRoot + ((hashVal (static_cast<uint32_t> (gStepInBar)) % 2 == 0) ? 0 : 7));
        gEv.octaveShift = (role == InstrumentRole::MonoBass) ? -1 : 0;
        gEv.baseVelocity = static_cast<int> (42 + (hashVal (static_cast<uint32_t> (gStepInBar * 7)) % 14));
        gEv.baseGate = 0.28f;
        gEv.isGhost = true;
        gEv.harmonicFunc = HarmonicFunction::Passing;
        events.push_back (gEv);
    };

    tryAddGhost (ghostStepA, 54.0f + static_cast<float> (hashVal (static_cast<uint32_t> (bar * 13)) % 8));
    tryAddGhost (ghostStepB, 64.0f + static_cast<float> (hashVal (static_cast<uint32_t> (bar * 29)) % 8));
}

//==============================================================================
// GENERATORI CANDIDATI PER CIASCUN ARTISTA
//==============================================================================

// 1. KIASMOS
class LayeredKiasmosGenerator
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
        const int archA = static_cast<int> (hashVal (2) % 10);
        const int archB = static_cast<int> (hashVal (3) % 10);
        const int archTension = static_cast<int> (hashVal (4) % 10);
        const int archFill = static_cast<int> (hashVal (5) % 10);

        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension  = (bar == 6);
            const bool isFillBar  = (bar == 7);
            const int barRootRaw  = kKiasmosProgressions[progIdx][bar];
            const int barRoot     = quantize (barRootRaw);

            if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp && role != InstrumentRole::AcidLine)
            {
                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            else
            {
                int archIdx = archA;
                if (isFillBar)           archIdx = archFill;
                else if (isTension)      archIdx = archTension;
                else if (isResponse)     archIdx = archB;
                else if (bar == 4)       archIdx = static_cast<int> ((archA + (hashVal (static_cast<uint32_t> (bar * 13 + 7)) % 3)) % 10);

                buildPLFArchetypeCandidates (events, kKiasmosArchetypes[archIdx], bar, barRoot, role, scale, quantize, hashVal (static_cast<uint32_t> (bar * 31 + seed)));
            }
        }
        return events;
    }
};

// 2. STEPHAN BODZIN
class LayeredBodzinGenerator
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
        const int archA = static_cast<int> (hashVal (2) % 10);
        const int archB = static_cast<int> (hashVal (3) % 10);
        const int archTension = static_cast<int> (hashVal (4) % 10);
        const int archFill = static_cast<int> (hashVal (5) % 10);

        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension  = (bar == 6);
            const bool isFillBar  = (bar == 7);
            const int barRootRaw  = kBodzinProgressions[progIdx][bar];
            const int barRoot     = quantize (barRootRaw);

            if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp && role != InstrumentRole::AcidLine)
            {
                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            else
            {
                int archIdx = archA;
                if (isFillBar)           archIdx = archFill;
                else if (isTension)      archIdx = archTension;
                else if (isResponse)     archIdx = archB;
                else if (bar == 4)       archIdx = static_cast<int> ((archA + (hashVal (static_cast<uint32_t> (bar * 13 + 7)) % 3)) % 10);

                buildPLFArchetypeCandidates (events, kBodzinArchetypes[archIdx], bar, barRoot, role, scale, quantize, hashVal (static_cast<uint32_t> (bar * 31 + seed)));
            }
        }
        return events;
    }
};

// 3. BURIAL
class LayeredBurialGenerator
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
        const int archA = static_cast<int> (hashVal (2) % 10);
        const int archB = static_cast<int> (hashVal (3) % 10);
        const int archTension = static_cast<int> (hashVal (4) % 10);
        const int archFill = static_cast<int> (hashVal (5) % 10);

        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension  = (bar == 6);
            const bool isFillBar  = (bar == 7);
            const int barRootRaw  = kBurialProgressions[progIdx][bar];
            const int barRoot     = quantize (barRootRaw);

            if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp && role != InstrumentRole::AcidLine)
            {
                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            else
            {
                int archIdx = archA;
                if (isFillBar)           archIdx = archFill;
                else if (isTension)      archIdx = archTension;
                else if (isResponse)     archIdx = static_cast<int> ((archB + (bar == 3 ? (hashVal (static_cast<uint32_t> (seed + 19)) % 2) : 0)) % 10);
                else if (bar == 4)       archIdx = static_cast<int> ((archA + (hashVal (static_cast<uint32_t> (bar * 13 + 7)) % 3)) % 10);
                else if (bar == 1)       archIdx = static_cast<int> ((archA + (hashVal (static_cast<uint32_t> (seed + 29)) % 2)) % 10);

                buildPLFArchetypeCandidates (events, kBurialArchetypes[archIdx], bar, barRoot, role, scale, quantize, hashVal (static_cast<uint32_t> (bar * 31 + seed)));
            }
        }
        return events;
    }
};

// 4. LAOUM
class LayeredLaoumGenerator
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
        const int archA = static_cast<int> (hashVal (2) % 16);
        const int archB = static_cast<int> (hashVal (3) % 16);
        const int archTension = static_cast<int> (hashVal (4) % 16);
        const int archFill = static_cast<int> (hashVal (5) % 16);

        for (int bar = 0; bar < 8; ++bar)
        {
            const bool isResponse = (bar == 2 || bar == 3 || bar == 5);
            const bool isTension  = (bar == 6);
            const bool isFillBar  = (bar == 7);
            const int barRootRaw  = kLaoumProgressions[progIdx][bar];
            const int barRoot     = quantize (barRootRaw);

            if (role != InstrumentRole::MonoBass && role != InstrumentRole::ClassicArp && role != InstrumentRole::AcidLine)
            {
                appendRoleCandidates (events, role, seed, bar, barRoot, isResponse, isTension, isFillBar, quantize);
            }
            else
            {
                int archIdx = archA;
                if (isFillBar)           archIdx = archFill;
                else if (isTension)      archIdx = archTension;
                else if (isResponse)     archIdx = archB;
                else if (bar == 4)       archIdx = static_cast<int> ((archA + (hashVal (static_cast<uint32_t> (bar * 13 + 7)) % 3)) % 16);

                buildPLFArchetypeCandidates (events, kLaoumArchetypes[archIdx], bar, barRoot, role, scale, quantize, hashVal (static_cast<uint32_t> (bar * 31 + seed)));
            }
        }
        return events;
    }
};

} // namespace PulseForge
