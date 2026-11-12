#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PatternData.h"
#include "../Source/StyleGenerator.h"

int main() {
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::ifstream file("Tests/golden_acid303.txt");
    if (!file.is_open()) {
        std::cerr << "FAIL: Cannot open Tests/golden_acid303.txt" << std::endl;
        return 1;
    }

    std::string line;
    int testsRun = 0;
    int testsPassed = 0;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // SEED:1|STEPS:8|PATTERN:...
        size_t s1 = line.find("SEED:");
        size_t s2 = line.find("|STEPS:");
        size_t s3 = line.find("|PATTERN:");
        if (s1 == std::string::npos || s2 == std::string::npos || s3 == std::string::npos) continue;

        uint32_t seed = std::stoul(line.substr(s1 + 5, s2 - (s1 + 5)));
        int steps = std::stoi(line.substr(s2 + 7, s3 - (s2 + 7)));
        std::string expectedPat = line.substr(s3 + 9);

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

        std::stringstream ss;
        for (int i = 0; i < steps; ++i) {
            const auto& s = bank.steps[i];
            char buf[64];
            snprintf(buf, sizeof(buf), "%s,%d,%d,%s,%s,%d,%.2f;",
                     s.active ? "1" : "0",
                     s.semitoneOffset,
                     s.octaveShift,
                     s.accent ? "A" : "-",
                     s.slide ? "S" : "-",
                     s.velocity,
                     s.gate);
            ss << buf;
        }

        std::string actualPat = ss.str();
        testsRun++;
        if (actualPat == expectedPat) {
            testsPassed++;
        } else {
            std::cerr << "REGRESSION FAILURE on SEED " << seed << " STEPS " << steps << "\n"
                      << "  Expected: " << expectedPat << "\n"
                      << "  Actual:   " << actualPat << std::endl;
            return 2;
        }
    }

    std::cout << "SUCCESS: All " << testsPassed << "/" << testsRun << " Golden Acid 303 regression tests PASSED 100%!" << std::endl;
    return 0;
}
