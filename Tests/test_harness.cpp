#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <set>
#include <map>
#include <algorithm>

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PatternData.h"
#include "../Source/StyleGenerator.h"

struct Metrics {
    float activePercent = 0.0f;
    float distinctNotes = 0.0f;
    float accentPercent = 0.0f;
    float slidePercent = 0.0f;
    float repetitionIndex = 0.0f;
    float syncopationIndex = 0.0f;
    float leapsOverFourthPercent = 0.0f;
};

// Calculate repetition index: compares 4-step slices or autocorrelation
float calculateRepetitionIndex(const PulseForge::PatternBank& bank) {
    int n = bank.numSteps;
    if (n < 8) return 1.0f;
    int sliceLen = (n >= 16) ? 4 : 2;
    int matches = 0;
    int totalComparisons = 0;

    for (int i = 0; i < n - sliceLen; ++i) {
        for (int j = i + sliceLen; j <= n - sliceLen; j += sliceLen) {
            bool sameRhythm = true;
            bool samePitch = true;
            for (int k = 0; k < sliceLen; ++k) {
                const auto& s1 = bank.steps[i + k];
                const auto& s2 = bank.steps[j + k];
                if (s1.active != s2.active) sameRhythm = false;
                if (s1.active && s2.active) {
                    if (s1.semitoneOffset != s2.semitoneOffset || s1.octaveShift != s2.octaveShift)
                        samePitch = false;
                }
            }
            if (sameRhythm) matches++;
            if (sameRhythm && samePitch) matches++;
            totalComparisons += 2;
        }
    }
    return (totalComparisons > 0) ? (static_cast<float>(matches) / totalComparisons) : 0.0f;
}

// Calculate syncopation index (Longuet-Higgins & Lee metric model adapted for 16th steps)
float calculateSyncopationIndex(const PulseForge::PatternBank& bank) {
    int n = bank.numSteps;
    int metricWeight[32];
    for (int i = 0; i < 32; ++i) {
        if (i % 16 == 0)      metricWeight[i] = 4; // Downbeat
        else if (i % 8 == 0) metricWeight[i] = 3; // Half bar
        else if (i % 4 == 0) metricWeight[i] = 2; // Quarter note
        else if (i % 2 == 0) metricWeight[i] = 1; // 8th note
        else                 metricWeight[i] = 0; // 16th off-beat
    }

    int syncopations = 0;
    int activeNotes = 0;

    for (int i = 0; i < n; ++i) {
        if (bank.steps[i].active) {
            activeNotes++;
            int next = (i + 1) % n;
            // Note on weak beat followed by rest or weaker tie on strong beat
            if (!bank.steps[next].active && metricWeight[i] < metricWeight[next]) {
                syncopations += (metricWeight[next] - metricWeight[i]);
            }
        }
    }
    return (activeNotes > 0) ? (static_cast<float>(syncopations) / activeNotes) : 0.0f;
}

Metrics calculateMetricsForPattern(const PulseForge::PatternBank& bank) {
    Metrics m;
    int n = bank.numSteps;
    int activeCount = 0;
    int accentCount = 0;
    int slideCount = 0;
    std::set<int> distinctPitches;
    std::vector<int> pitches;

    for (int i = 0; i < n; ++i) {
        const auto& s = bank.steps[i];
        if (s.active) {
            activeCount++;
            if (s.accent) accentCount++;
            if (s.slide)  slideCount++;
            int pitch = s.semitoneOffset + s.octaveShift * 12;
            distinctPitches.insert(pitch);
            pitches.push_back(pitch);
        }
    }

    m.activePercent = (n > 0) ? (static_cast<float>(activeCount) * 100.0f / n) : 0.0f;
    m.distinctNotes = static_cast<float>(distinctPitches.size());
    m.accentPercent = (activeCount > 0) ? (static_cast<float>(accentCount) * 100.0f / activeCount) : 0.0f;
    m.slidePercent  = (activeCount > 0) ? (static_cast<float>(slideCount) * 100.0f / activeCount) : 0.0f;

    // Leaps > Fourth (interval > 5 semitones)
    int totalIntervals = 0;
    int leapsOver4th = 0;
    for (size_t i = 1; i < pitches.size(); ++i) {
        int diff = std::abs(pitches[i] - pitches[i - 1]);
        if (diff > 5) leapsOver4th++;
        totalIntervals++;
    }
    m.leapsOverFourthPercent = (totalIntervals > 0) ? (static_cast<float>(leapsOver4th) * 100.0f / totalIntervals) : 0.0f;

    m.repetitionIndex = calculateRepetitionIndex(bank);
    m.syncopationIndex = calculateSyncopationIndex(bank);

    return m;
}

int main() {
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::cout << "================================================================" << std::endl;
    std::cout << "PulseForge Generator Regression Harness & Baseline Calculator" << std::endl;
    std::cout << "================================================================" << std::endl;

    // 1. Generate & Save Golden Reference for Acid 303 across 20 fixed seeds
    std::ofstream goldenFile("Tests/golden_acid303.txt");
    std::vector<int> stepLengths = { 8, 16, 32 };
    
    goldenFile << "# PULSEFORGE GOLDEN REFERENCE - ACID 303\n";
    goldenFile << "# 20 Fixed Seeds (1..20), steps = 8, 16, 32\n";

    for (int steps : stepLengths) {
        float avgAct = 0, avgDist = 0, avgAcc = 0, avgSli = 0, avgRep = 0, avgSync = 0, avgLeap = 0;

        for (uint32_t seed = 1; seed <= 20; ++seed) {
            PulseForge::PatternBank bank;
            bank.numSteps = steps;

            PulseForge::StyleGenerator::GenerationParams gp;
            gp.genre = PulseForge::GenreStyle::AcidTechno;
            gp.role  = PulseForge::InstrumentRole::AcidLine;
            gp.scale = PulseForge::ScaleType::Phrygian;
            gp.density = 0.55f;
            gp.spice = 0.40f;
            gp.pitchRange = 0.50f;
            gp.syncopation = 0.35f;

            PulseForge::StyleGenerator::generatePattern(bank, gp, seed);

            goldenFile << "SEED:" << seed << "|STEPS:" << steps << "|PATTERN:";
            for (int i = 0; i < steps; ++i) {
                const auto& s = bank.steps[i];
                goldenFile << (s.active ? "1" : "0") << ","
                           << s.semitoneOffset << ","
                           << s.octaveShift << ","
                           << (s.accent ? "A" : "-") << ","
                           << (s.slide ? "S" : "-") << ","
                           << s.velocity << ","
                           << std::fixed << std::setprecision(2) << s.gate << ";";
            }
            goldenFile << "\n";

            Metrics m = calculateMetricsForPattern(bank);
            avgAct += m.activePercent;
            avgDist += m.distinctNotes;
            avgAcc += m.accentPercent;
            avgSli += m.slidePercent;
            avgRep += m.repetitionIndex;
            avgSync += m.syncopationIndex;
            avgLeap += m.leapsOverFourthPercent;
        }

        std::cout << "\n--- GOLDEN METRICS ACID 303 (N=" << steps << ", 20 seeds) ---" << std::endl;
        std::cout << "Active:       " << (avgAct / 20.0f) << "%" << std::endl;
        std::cout << "Distinct Notes: " << (avgDist / 20.0f) << std::endl;
        std::cout << "Accent:       " << (avgAcc / 20.0f) << "%" << std::endl;
        std::cout << "Slide:        " << (avgSli / 20.0f) << "%" << std::endl;
        std::cout << "Repetition:   " << (avgRep / 20.0f) << std::endl;
        std::cout << "Syncopation:  " << (avgSync / 20.0f) << std::endl;
        std::cout << "Leaps > 4th:  " << (avgLeap / 20.0f) << "%" << std::endl;
    }
    goldenFile.close();
    std::cout << "\nGolden Reference saved to Tests/golden_acid303.txt" << std::endl;

    // 2. Calculate baseline metrics for other genres (16 steps, 50 seeds each)
    std::cout << "\n================================================================" << std::endl;
    std::cout << "BASELINE METRICS (BEFORE REFACTOR) - 50 Seeds, 16 Steps" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << std::left << std::setw(18) << "Genre"
              << std::setw(10) << "Active%"
              << std::setw(10) << "DistNotes"
              << std::setw(10) << "Accent%"
              << std::setw(10) << "Slide%"
              << std::setw(10) << "Repetition"
              << std::setw(10) << "Syncop"
              << std::setw(10) << "Leaps>4th%" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;

    struct GenreTest {
        PulseForge::GenreStyle genre;
        PulseForge::InstrumentRole role;
        PulseForge::ScaleType scale;
        const char* name;
    };

    std::vector<GenreTest> testGenres = {
        { PulseForge::GenreStyle::AcidTechno,     PulseForge::InstrumentRole::AcidLine,       PulseForge::ScaleType::Phrygian,     "Acid 303 (Ref)" },
        { PulseForge::GenreStyle::ClassicHouse,    PulseForge::InstrumentRole::MonoBass,       PulseForge::ScaleType::MinorNatural, "Classic House" },
        { PulseForge::GenreStyle::MelodicTechno,   PulseForge::InstrumentRole::ClassicArp,     PulseForge::ScaleType::MinorNatural, "Melodic Techno" },
        { PulseForge::GenreStyle::BerlinSchool,    PulseForge::InstrumentRole::ClassicArp,     PulseForge::ScaleType::Dorian,       "Tech House/Berlin" },
        { PulseForge::GenreStyle::DetroitElectro,  PulseForge::InstrumentRole::MonoBass,       PulseForge::ScaleType::MinorNatural, "Detroit/Garage" },
        { PulseForge::GenreStyle::ItaloSynthwave,  PulseForge::InstrumentRole::ClassicArp,     PulseForge::ScaleType::MinorNatural, "Italo/EDM" }
    };

    for (const auto& gt : testGenres) {
        float avgAct = 0, avgDist = 0, avgAcc = 0, avgSli = 0, avgRep = 0, avgSync = 0, avgLeap = 0;
        for (uint32_t seed = 1; seed <= 50; ++seed) {
            PulseForge::PatternBank bank;
            bank.numSteps = 16;

            PulseForge::StyleGenerator::GenerationParams gp;
            gp.genre = gt.genre;
            gp.role  = gt.role;
            gp.scale = gt.scale;
            gp.density = 0.60f;
            gp.spice = 0.40f;
            gp.pitchRange = 0.50f;
            gp.syncopation = 0.35f;

            PulseForge::StyleGenerator::generatePattern(bank, gp, seed);
            Metrics m = calculateMetricsForPattern(bank);

            avgAct += m.activePercent;
            avgDist += m.distinctNotes;
            avgAcc += m.accentPercent;
            avgSli += m.slidePercent;
            avgRep += m.repetitionIndex;
            avgSync += m.syncopationIndex;
            avgLeap += m.leapsOverFourthPercent;
        }

        std::cout << std::left << std::setw(18) << gt.name
                  << std::fixed << std::setprecision(1)
                  << std::setw(10) << (avgAct / 50.0f)
                  << std::setw(10) << (avgDist / 50.0f)
                  << std::setw(10) << (avgAcc / 50.0f)
                  << std::setw(10) << (avgSli / 50.0f)
                  << std::setprecision(2)
                  << std::setw(10) << (avgRep / 50.0f)
                  << std::setw(10) << (avgSync / 50.0f)
                  << std::setprecision(1)
                  << std::setw(10) << (avgLeap / 50.0f) << std::endl;
    }

    std::cout << "Done." << std::endl;
    return 0;
}
