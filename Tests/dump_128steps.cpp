#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <juce_core/juce_core.h>
#include "../Source/PatternData.h"
#include "../Source/GenreProfiles.h"
#include "../Source/FisherBassBricks.h"
#include "../Source/StyleGenerator.h"

int main() {
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "DUMP COMPLETO 8 BATTUTE (128 STEP) - FISHER TECH HOUSE BASS (SEED 101)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    const uint32_t seed = 101;
    // Generiamo su 32 step per mostrare l'evoluzione di frase
    PulseForge::PatternBank bank;
    bank.numSteps = 32;

    PulseForge::StyleGenerator::GenerationParams gp;
    gp.genre = PulseForge::GenreStyle::BerlinSchool;
    gp.role = PulseForge::InstrumentRole::MonoBass;
    gp.scale = PulseForge::ScaleType::MinorPentatonic;
    gp.density = 0.55f;
    gp.freedom = 0.35f;

    PulseForge::StyleGenerator::generatePattern(bank, gp, seed);

    const char* barRoles[8] = {
        "Battuta 1: STATEMENT A (Tonica ipnotica, spazio cassa)",
        "Battuta 2: RESPONSE A' (Anticipo sincopato / b7)",
        "Battuta 3: STATEMENT A (Ripresa perno solido)",
        "Battuta 4: RESPONSE A' (Variazione levare)",
        "Battuta 5: RIPRESA STATEMENT (Ghost rimbalzo)",
        "Battuta 6: RESPONSE & TENSIONE (Pop d'ottava controllato)",
        "Battuta 7: TENSIONE CRESCO (Drop cassa / stacco)",
        "Battuta 8: FILL CADENZA / TURNAROUND (Semicrome piene & Leading-Tone)"
    };

    for (int bar = 0; bar < 8; ++bar) {
        std::cout << "\n>>> " << barRoles[bar] << std::endl;
        std::cout << std::left << std::setw(6) << "Step"
                  << std::setw(8) << "Tick"
                  << std::setw(12) << "Gate(dur)"
                  << std::setw(10) << "Pitch"
                  << std::setw(8) << "8va"
                  << std::setw(10) << "Velocity"
                  << "Articolazione / Ruolo" << std::endl;
        std::cout << "------------------------------------------------------------------------------------------" << std::endl;

        for (int s = 0; s < 16; ++s) {
            int globalStep = bar * 16 + s;
            int patIdx = globalStep % bank.numSteps;
            const auto& st = bank.steps[patIdx];

            int tick = globalStep * 240;

            if (!st.active) {
                std::cout << std::left << std::setw(6) << globalStep
                          << std::setw(8) << tick
                          << std::setw(12) << "-"
                          << std::setw(10) << "-"
                          << std::setw(8) << "-"
                          << std::setw(10) << "-"
                          << "[ PAUSA / KICK ]" << std::endl;
            } else {
                std::string role = "Core Downbeat";
                if (st.velocity <= 52) role = "** GHOST NOTE (Percezione ritmo) **";
                else if (st.octaveShift == 1) role = "## POP OTTAVA ACCENTATO ##";
                else if (st.accent) role = "Accent Rimbalzo";
                else role = "Drive Groove";

                std::cout << std::left << std::setw(6) << globalStep
                          << std::setw(8) << tick
                          << std::setw(12) << std::fixed << std::setprecision(2) << st.gate
                          << std::setw(10) << st.semitoneOffset
                          << std::setw(8) << st.octaveShift
                          << std::setw(10) << st.velocity
                          << role << std::endl;
            }
        }
    }

    return 0;
}
