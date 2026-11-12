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
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/PatternData.h"
#include "../Source/GenreProfiles.h"
#include "../Source/FisherBassBricks.h"
#include "../Source/StyleGenerator.h"

void exportMidiWithDrums(const std::string& filename, const PulseForge::PatternBank& bank, int rootKey, int baseOctave, double bpm) {
    juce::MidiFile midiFile;
    short ticksPerQuarter = 960;
    midiFile.setTicksPerQuarterNote(ticksPerQuarter);
    double ticksPer16th = ticksPerQuarter / 4.0;
    double microsecondsPerQuarter = 60000000.0 / bpm;

    juce::MidiMessageSequence conductorTrack;
    conductorTrack.addEvent(juce::MidiMessage::tempoMetaEvent(static_cast<int>(microsecondsPerQuarter)), 0.0);
    midiFile.addTrack(conductorTrack);

    juce::MidiMessageSequence bassTrack;
    int n = bank.numSteps;
    int total16ths = 128; // 8 battute
    int rootMidi = (baseOctave + 1) * 12 + rootKey;

    std::mt19937 humanRng(42);
    auto humanJitter = [&]() { return std::uniform_real_distribution<double>(-8.0, 8.0)(humanRng); };

    for (int step = 0; step < total16ths; ++step) {
        int patIdx = step % n;
        const auto& s = bank.steps[patIdx];
        if (s.active) {
            int midiNote = rootMidi + s.semitoneOffset + (s.octaveShift * 12);
            midiNote = juce::jlimit(12, 115, midiNote);
            int vel = juce::jlimit(1, 127, s.velocity);

            double swingOffset = (step % 2 == 1) ? 15.0 : 0.0;
            double startTick = std::max(0.0, step * ticksPer16th + swingOffset + humanJitter());
            double gateLenTicks = std::max(20.0, ticksPer16th * s.gate);
            double endTick = startTick + gateLenTicks;

            bassTrack.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(vel)), startTick);
            bassTrack.addEvent(juce::MidiMessage::noteOff(1, midiNote), endTick);
        }
    }
    bassTrack.updateMatchedPairs();
    midiFile.addTrack(bassTrack);

    juce::MidiMessageSequence drumTrack;
    for (int step = 0; step < total16ths; ++step) {
        double stepTick = step * ticksPer16th;
        if (step % 4 == 0) {
            drumTrack.addEvent(juce::MidiMessage::noteOn(10, 36, (juce::uint8)124), stepTick);
            drumTrack.addEvent(juce::MidiMessage::noteOff(10, 36), stepTick + 120.0);
        }
        if (step % 8 == 4) {
            drumTrack.addEvent(juce::MidiMessage::noteOn(10, 39, (juce::uint8)115), stepTick);
            drumTrack.addEvent(juce::MidiMessage::noteOff(10, 39), stepTick + 100.0);
        }
        if (step % 4 == 2) {
            double hhSwing = stepTick + 15.0;
            drumTrack.addEvent(juce::MidiMessage::noteOn(10, 42, (juce::uint8)108), hhSwing);
            drumTrack.addEvent(juce::MidiMessage::noteOff(10, 42), hhSwing + 60.0);
        }
    }
    drumTrack.updateMatchedPairs();
    midiFile.addTrack(drumTrack);

    juce::File outFile(filename);
    outFile.deleteFile();
    juce::FileOutputStream outStream(outFile);
    if (outStream.openedOk()) {
        midiFile.writeTo(outStream);
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI guiInit;

    std::cout << "================================================================================" << std::endl;
    std::cout << "VERIFICA 2A: DUMP PRIMI 16 STEP DI 4 SEED DIVERSI (FISHER TECH HOUSE BASS)" << std::endl;
    std::cout << "================================================================================" << std::endl;

    uint32_t dumpSeeds[4] = { 101, 202, 303, 404 };

    for (int s = 0; s < 4; ++s) {
        uint32_t seed = dumpSeeds[s];
        PulseForge::PatternBank bank;
        bank.numSteps = 16;
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = PulseForge::GenreStyle::BerlinSchool;
        gp.role = PulseForge::InstrumentRole::MonoBass;
        gp.scale = PulseForge::ScaleType::MinorPentatonic;
        gp.density = 0.55f;
        gp.freedom = 0.35f;

        PulseForge::StyleGenerator::generatePattern(bank, gp, seed);

        std::cout << "\n--- SEED " << seed << " ---" << std::endl;
        std::cout << std::left << std::setw(6) << "Step"
                  << std::setw(12) << "Tick(approx)"
                  << std::setw(12) << "Durata(gate)"
                  << std::setw(12) << "Pitch(semi)"
                  << std::setw(8) << "8va"
                  << std::setw(10) << "Velocity"
                  << "Tipo" << std::endl;
        std::cout << "----------------------------------------------------------------" << std::endl;

        for (int i = 0; i < 16; ++i) {
            const auto& st = bank.steps[i];
            if (!st.active) {
                std::cout << std::left << std::setw(6) << i
                          << std::setw(12) << (i * 240)
                          << std::setw(12) << "-"
                          << std::setw(12) << "-"
                          << std::setw(8) << "-"
                          << std::setw(10) << "-"
                          << "[ PAUSA / KICK ]" << std::endl;
            } else {
                std::string type = "Normal";
                if (st.velocity <= 52) type = "** GHOST **";
                else if (st.octaveShift == 1) type = "## POP 8va ##";
                else if (st.accent) type = "Accent";

                std::cout << std::left << std::setw(6) << i
                          << std::setw(12) << (i * 240)
                          << std::setw(12) << std::fixed << std::setprecision(2) << st.gate
                          << std::setw(12) << st.semitoneOffset
                          << std::setw(8) << st.octaveShift
                          << std::setw(10) << st.velocity
                          << type << std::endl;
            }
        }
    }

    // Test di unicità su 200 seed
    std::cout << "\n================================================================================" << std::endl;
    std::cout << "VERIFICA 2C: TEST UNICITA' SU 200 SEED (FISHER BASS)" << std::endl;
    std::cout << "================================================================================" << std::endl;

    std::set<std::string> uniquePatterns;
    std::vector<std::vector<int>> allPatternsPitches;
    std::vector<std::vector<bool>> allPatternsActive;
    const int kNumTestSeeds = 200;

    for (uint32_t seed = 1; seed <= kNumTestSeeds; ++seed) {
        PulseForge::PatternBank bank;
        bank.numSteps = 16;
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = PulseForge::GenreStyle::BerlinSchool;
        gp.role = PulseForge::InstrumentRole::MonoBass;
        gp.scale = PulseForge::ScaleType::MinorPentatonic;
        gp.density = 0.55f;
        gp.freedom = 0.40f;
        PulseForge::StyleGenerator::generatePattern(bank, gp, seed);

        std::string patSig = "";
        std::vector<int> pitches(16, -99);
        std::vector<bool> activeMask(16, false);

        for (int i = 0; i < 16; ++i) {
            activeMask[i] = bank.steps[i].active;
            if (bank.steps[i].active) {
                int p = bank.steps[i].semitoneOffset + bank.steps[i].octaveShift * 12;
                pitches[i] = p;
                patSig += std::to_string(p) + "@" + std::to_string(bank.steps[i].velocity) + ";";
            } else {
                patSig += ".;";
            }
        }
        uniquePatterns.insert(patSig);
        allPatternsPitches.push_back(pitches);
        allPatternsActive.push_back(activeMask);
    }

    double totalSimilarity = 0.0;
    int comparisons = 0;
    for (int i = 0; i < kNumTestSeeds; ++i) {
        for (int j = i + 1; j < kNumTestSeeds; ++j) {
            int matchCount = 0;
            for (int k = 0; k < 16; ++k) {
                if (allPatternsActive[i][k] == allPatternsActive[j][k]) {
                    if (!allPatternsActive[i][k]) matchCount++;
                    else if (allPatternsPitches[i][k] == allPatternsPitches[j][k]) matchCount++;
                }
            }
            totalSimilarity += (static_cast<double>(matchCount) / 16.0);
            comparisons++;
        }
    }
    double avgSim = (comparisons > 0) ? ((totalSimilarity / comparisons) * 100.0) : 0.0;

    std::cout << "Pattern generati: " << kNumTestSeeds << std::endl;
    std::cout << "Pattern unici:    " << uniquePatterns.size() << " / " << kNumTestSeeds << std::endl;
    std::cout << "Somiglianza media tra seed diversi: " << std::fixed << std::setprecision(1) << avgSim << "%" << std::endl;

    std::cout << "\nEsito Unicita' (100%): " << (uniquePatterns.size() == kNumTestSeeds ? "PASS" : "FAIL") << std::endl;
    std::cout << "Esito Somiglianza (<60%): " << (avgSim < 60.0 ? "PASS" : "FAIL") << std::endl;

    // Rigenera i 4 file MIDI
    for (int i = 0; i < 4; ++i) {
        PulseForge::PatternBank bank;
        bank.numSteps = 16;
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = PulseForge::GenreStyle::BerlinSchool;
        gp.role = PulseForge::InstrumentRole::MonoBass;
        gp.scale = PulseForge::ScaleType::MinorPentatonic;
        gp.density = 0.55f;
        gp.freedom = 0.35f;
        PulseForge::StyleGenerator::generatePattern(bank, gp, dumpSeeds[i]);
        std::string filename = "Tests/Fisher_Bass_Seed" + std::to_string(dumpSeeds[i]) + "_WithDrums.mid";
        exportMidiWithDrums(filename, bank, 0, 1, 126.0);
    }

    return 0;
}
