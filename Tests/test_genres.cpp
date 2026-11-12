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
#include "../Source/StyleGenerator.h"

struct DetailedMetrics {
    float activePercent = 0.0f;
    float distinctPitchClasses = 0.0f;  // Classi di nota della scala (0..11)
    float distinctAbsoluteNotes = 0.0f; // Note MIDI assolute (Pitch + Octave)
    float accentPercent = 0.0f;
    float slidePercent = 0.0f;
    float repetitionIndex = 0.0f;
    float syncopationIndex = 0.0f;
    float foldedLeapsOverFourthPercent = 0.0f; // Salti melodici ripiegati modulo 12 (senza contare salti d'ottava)
    float octaveLeapsPercent = 0.0f;           // Salti d'ottava calcolati a parte
};

// Calcolo Repetition Index tra sezioni a 4 step
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

// Modello di sincopazione Longuet-Higgins & Lee:
// Una nota è sincopata quando cade su uno step metricamente più debole del successivo che è muto/legato
float calculateSyncopationIndex(const PulseForge::PatternBank& bank) {
    int n = bank.numSteps;
    int metricWeight[16] = {
        4, 0, 2, 0,  // Beat 1 (Downbeat=4, 16th=0, 8th-offbeat=2, 16th=0)
        3, 0, 2, 0,  // Beat 2
        4, 0, 2, 0,  // Beat 3
        3, 0, 2, 0   // Beat 4
    };

    int syncopations = 0;
    int activeNotes = 0;

    for (int i = 0; i < n; ++i) {
        if (bank.steps[i].active) {
            activeNotes++;
            int next = (i + 1) % n;
            int wCur  = metricWeight[i % 16];
            int wNext = metricWeight[next % 16];

            // Se la nota cade su un tempo debole (es. off-beat 2 o sedicesimo 0)
            // e il tempo forte successivo è una pausa, si crea sincopazione proporzionale
            if (!bank.steps[next].active && wCur < wNext) {
                syncopations += (wNext - wCur);
            }
            // Sincopi accentuate sul levare
            else if (wCur == 2 && bank.steps[i].accent) {
                syncopations += 1;
            }
        }
    }
    return (activeNotes > 0) ? (static_cast<float>(syncopations) / activeNotes) : 0.0f;
}

DetailedMetrics calculateDetailedMetrics(const PulseForge::PatternBank& bank) {
    DetailedMetrics m;
    int n = bank.numSteps;
    int activeCount = 0;
    int accentCount = 0;
    int slideCount = 0;
    std::set<int> pitchClasses;
    std::set<int> absoluteNotes;
    std::vector<int> pitches;
    std::vector<int> foldedPitches;

    for (int i = 0; i < n; ++i) {
        const auto& s = bank.steps[i];
        if (s.active) {
            activeCount++;
            if (s.accent) accentCount++;
            if (s.slide)  slideCount++;

            int pc = ((s.semitoneOffset % 12) + 12) % 12;
            int absPitch = 48 + s.semitoneOffset + (s.octaveShift * 12);
            pitchClasses.insert(pc);
            absoluteNotes.insert(absPitch);
            pitches.push_back(absPitch);
            foldedPitches.push_back(pc);
        }
    }

    m.activePercent = (n > 0) ? (static_cast<float>(activeCount) * 100.0f / n) : 0.0f;
    m.distinctPitchClasses = static_cast<float>(pitchClasses.size());
    m.distinctAbsoluteNotes = static_cast<float>(absoluteNotes.size());
    m.accentPercent = (activeCount > 0) ? (static_cast<float>(accentCount) * 100.0f / activeCount) : 0.0f;
    m.slidePercent  = (activeCount > 0) ? (static_cast<float>(slideCount) * 100.0f / activeCount) : 0.0f;

    // Salti melodici ripiegati (modulo 12: distanza minima sulla ruota delle 12 note)
    int totalIntervals = 0;
    int foldedLeapsOver4th = 0;
    int octaveJumps = 0;

    for (size_t i = 1; i < foldedPitches.size(); ++i) {
        int diff = std::abs(foldedPitches[i] - foldedPitches[i - 1]);
        if (diff > 6) diff = 12 - diff; // Distanza circolare minima
        if (diff > 5) foldedLeapsOver4th++; // Salto > 4a (quarta = 5 semitoni)

        int absDiff = std::abs(pitches[i] - pitches[i - 1]);
        if (absDiff >= 12 && absDiff % 12 == 0) octaveJumps++;

        totalIntervals++;
    }

    m.foldedLeapsOverFourthPercent = (totalIntervals > 0) ? (static_cast<float>(foldedLeapsOver4th) * 100.0f / totalIntervals) : 0.0f;
    m.octaveLeapsPercent = (totalIntervals > 0) ? (static_cast<float>(octaveJumps) * 100.0f / totalIntervals) : 0.0f;
    m.repetitionIndex = calculateRepetitionIndex(bank);
    m.syncopationIndex = calculateSyncopationIndex(bank);

    return m;
}

void exportMidi8Bars(const std::string& filename, const PulseForge::PatternBank& bank, int rootKey, int baseOctave, double bpm) {
    juce::MidiMessageSequence seq;
    double ticksPerQuarter = 960.0;
    double microsecondsPerQuarter = 60000000.0 / bpm;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent(static_cast<int>(microsecondsPerQuarter)), 0.0);

    int n = bank.numSteps;
    int total16ths = 128;
    double ticksPer16th = ticksPerQuarter / 4.0;
    int rootMidi = (baseOctave + 1) * 12 + rootKey;

    for (int step = 0; step < total16ths; ++step) {
        int patIdx = step % n;
        const auto& s = bank.steps[patIdx];
        if (s.active) {
            int midiNote = rootMidi + s.semitoneOffset + (s.octaveShift * 12);
            midiNote = juce::jlimit(12, 115, midiNote);
            int vel = s.accent ? 122 : 95;

            double startTick = step * ticksPer16th;
            double gateLenTicks = ticksPer16th * s.gate;
            double endTick = startTick + gateLenTicks;

            seq.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(vel)), startTick);
            seq.addEvent(juce::MidiMessage::noteOff(1, midiNote), endTick);
        }
    }

    seq.updateMatchedPairs();
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(static_cast<short>(ticksPerQuarter));
    midiFile.addTrack(seq);

    juce::File outFile(filename);
    outFile.deleteFile();
    juce::FileOutputStream outStream(outFile);
    if (outStream.openedOk()) {
        midiFile.writeTo(outStream);
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI guiInit;

    struct GenreTest {
        PulseForge::GenreStyle genre;
        PulseForge::InstrumentRole role;
        PulseForge::ScaleType scale;
        const char* name;
        const char* midiName;
    };

    std::vector<GenreTest> testGenres = {
        { PulseForge::GenreStyle::AcidTechno,     PulseForge::InstrumentRole::AcidLine,   PulseForge::ScaleType::Phrygian,     "Acid 303 (Ref)",  "Acid303_8Bars.mid" },
        { PulseForge::GenreStyle::ClassicHouse,    PulseForge::InstrumentRole::MonoBass,   PulseForge::ScaleType::Dorian,       "Classic House",   "ClassicHouse_8Bars.mid" },
        { PulseForge::GenreStyle::BerlinSchool,    PulseForge::InstrumentRole::MonoBass,   PulseForge::ScaleType::MinorNatural, "Tech House",      "TechHouse_8Bars.mid" },
        { PulseForge::GenreStyle::MelodicTechno,   PulseForge::InstrumentRole::ClassicArp, PulseForge::ScaleType::MinorNatural, "Melodic Techno",  "MelodicTechno_8Bars.mid" },
        { PulseForge::GenreStyle::DetroitElectro,  PulseForge::InstrumentRole::MonoBass,   PulseForge::ScaleType::MinorNatural, "UK Garage",       "UKGarage_8Bars.mid" },
        { PulseForge::GenreStyle::ItaloSynthwave,  PulseForge::InstrumentRole::ClassicArp, PulseForge::ScaleType::MinorNatural, "Italo / EDM",    "ItaloEDM_8Bars.mid" }
    };

    // 1. Stampa tabelle a COMPLEXITY = 0, 50, 100
    float compValues[] = { 0.0f, 0.50f, 1.0f };

    for (float comp : compValues) {
        std::cout << "\n================================================================================================================" << std::endl;
        std::cout << "TABELLA METRICHE A COMPLEXITY = " << static_cast<int>(comp * 100) << "% (50 Seeds per Genere, 16 Steps)" << std::endl;
        std::cout << "================================================================================================================" << std::endl;
        std::cout << std::left << std::setw(18) << "Genere"
                  << std::setw(9) << "Active%"
                  << std::setw(9) << "ScalePC"
                  << std::setw(9) << "AbsNote"
                  << std::setw(9) << "Accent%"
                  << std::setw(9) << "Slide%"
                  << std::setw(9) << "Repetit"
                  << std::setw(9) << "Syncop"
                  << std::setw(10) << "FoldLeap>4"
                  << std::setw(9) << "OctLeap%" << std::endl;
        std::cout << "----------------------------------------------------------------------------------------------------------------" << std::endl;

        for (const auto& gt : testGenres) {
            float avgAct = 0, avgPC = 0, avgAbs = 0, avgAcc = 0, avgSli = 0, avgRep = 0, avgSync = 0, avgFoldLeap = 0, avgOctLeap = 0;

            for (uint32_t seed = 1; seed <= 50; ++seed) {
                PulseForge::PatternBank bank;
                bank.numSteps = 16;

                PulseForge::StyleGenerator::GenerationParams gp;
                gp.genre = gt.genre;
                gp.role  = gt.role;
                gp.scale = gt.scale;
                gp.density = comp;
                gp.spice = 0.40f;
                gp.pitchRange = 0.50f;
                gp.syncopation = 0.35f;

                PulseForge::StyleGenerator::generatePattern(bank, gp, seed);
                DetailedMetrics m = calculateDetailedMetrics(bank);

                avgAct += m.activePercent;
                avgPC  += m.distinctPitchClasses;
                avgAbs += m.distinctAbsoluteNotes;
                avgAcc += m.accentPercent;
                avgSli += m.slidePercent;
                avgRep += m.repetitionIndex;
                avgSync += m.syncopationIndex;
                avgFoldLeap += m.foldedLeapsOverFourthPercent;
                avgOctLeap  += m.octaveLeapsPercent;
            }

            std::cout << std::left << std::setw(18) << gt.name
                      << std::fixed << std::setprecision(1)
                      << std::setw(9) << (avgAct / 50.0f)
                      << std::setw(9) << (avgPC / 50.0f)
                      << std::setw(9) << (avgAbs / 50.0f)
                      << std::setw(9) << (avgAcc / 50.0f)
                      << std::setw(9) << (avgSli / 50.0f)
                      << std::setprecision(2)
                      << std::setw(9) << (avgRep / 50.0f)
                      << std::setw(9) << (avgSync / 50.0f)
                      << std::setprecision(1)
                      << std::setw(10) << (avgFoldLeap / 50.0f)
                      << std::setw(9) << (avgOctLeap / 50.0f) << std::endl;
        }
    }

    // 2. Mostra 5 seed diversi per ogni genere (Varietà tra seed)
    std::cout << "\n================================================================================================================" << std::endl;
    std::cout << "VARIETA' RITMICA TRA SEED (Maschere 16 step per Seed 1..5 a Complexity 50%)" << std::endl;
    std::cout << "================================================================================================================" << std::endl;

    for (const auto& gt : testGenres) {
        std::cout << "\n[" << gt.name << "]:" << std::endl;
        for (uint32_t seed = 1; seed <= 5; ++seed) {
            PulseForge::PatternBank bank;
            bank.numSteps = 16;
            PulseForge::StyleGenerator::GenerationParams gp;
            gp.genre = gt.genre;
            gp.role = gt.role;
            gp.scale = gt.scale;
            gp.density = 0.50f;
            PulseForge::StyleGenerator::generatePattern(bank, gp, seed);

            std::string mask = "";
            for (int k = 0; k < 16; ++k) mask += (bank.steps[k].active ? "X " : ". ");
            std::cout << "  Seed " << seed << ": " << mask << std::endl;
        }
    }

    // 3. Matrice Completa di Distanza di Hamming Mediata su 50 Seed
    std::cout << "\n================================================================================================================" << std::endl;
    std::cout << "MATRICE COMPLETA DISTANZA DI HAMMING (MEDIA SU 50 SEED, MAX 16 STEP)" << std::endl;
    std::cout << "================================================================================================================" << std::endl;

    const size_t numG = testGenres.size();
    std::vector<std::vector<float>> hammingMatrix(numG, std::vector<float>(numG, 0.0f));

    for (uint32_t seed = 1; seed <= 50; ++seed) {
        std::vector<std::vector<bool>> masks(numG, std::vector<bool>(16, false));
        for (size_t g = 0; g < numG; ++g) {
            PulseForge::PatternBank bank;
            bank.numSteps = 16;
            PulseForge::StyleGenerator::GenerationParams gp;
            gp.genre = testGenres[g].genre;
            gp.role = testGenres[g].role;
            gp.scale = testGenres[g].scale;
            gp.density = 0.50f;
            PulseForge::StyleGenerator::generatePattern(bank, gp, seed);
            for (int k = 0; k < 16; ++k) masks[g][k] = bank.steps[k].active;
        }

        for (size_t i = 0; i < numG; ++i) {
            for (size_t j = 0; j < numG; ++j) {
                int diff = 0;
                for (int k = 0; k < 16; ++k) {
                    if (masks[i][k] != masks[j][k]) diff++;
                }
                hammingMatrix[i][j] += static_cast<float>(diff);
            }
        }
    }

    std::cout << std::setw(18) << " ";
    for (size_t j = 0; j < numG; ++j) std::cout << std::setw(15) << testGenres[j].name;
    std::cout << "\n----------------------------------------------------------------------------------------------------------------" << std::endl;

    for (size_t i = 0; i < numG; ++i) {
        std::cout << std::left << std::setw(18) << testGenres[i].name;
        for (size_t j = 0; j < numG; ++j) {
            float avgDiff = hammingMatrix[i][j] / 50.0f;
            std::cout << std::fixed << std::setprecision(1) << std::setw(15) << avgDiff;
        }
        std::cout << std::endl;
    }

    // 4. Esporta MIDI a 8 battute con seed fisso 10
    for (const auto& gt : testGenres) {
        PulseForge::PatternBank bank;
        bank.numSteps = 16;
        PulseForge::StyleGenerator::GenerationParams gp;
        gp.genre = gt.genre;
        gp.role = gt.role;
        gp.scale = gt.scale;
        gp.density = 0.55f;
        PulseForge::StyleGenerator::generatePattern(bank, gp, 10);
        std::string path = "Tests/" + std::string(gt.midiName);
        exportMidi8Bars(path, bank, 0, (gt.role == PulseForge::InstrumentRole::AcidLine || gt.role == PulseForge::InstrumentRole::MonoBass) ? 1 : 2, 126.0);
    }

    std::cout << "\nFile MIDI esportati in Tests/" << std::endl;
    return 0;
}
