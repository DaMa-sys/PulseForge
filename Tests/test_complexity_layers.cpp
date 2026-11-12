#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <set>
#include <cmath>
#include <numeric>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/PatternData.h"
#include "../Source/GenreProfiles.h"
#include "../Source/LayeredPatternGenerator.h"

//==============================================================================
// Harness di Test per COMPLEXITY A STRATI
//==============================================================================
struct MetricsResult
{
    double notesPerBar = 0.0;
    double noteClasses = 0.0;
    double ghostPerBar = 0.0;
    double accentPerBar = 0.0;
    double popPerBar = 0.0;
    double ratchetPerBar = 0.0;
    double velocityVariance = 0.0;
    double avgNoteLength = 0.0;
};

MetricsResult computeMetrics (const std::vector<PulseForge::StepData>& steps128)
{
    MetricsResult res;
    int totalActive = 0;
    int totalGhosts = 0;
    int totalAccents = 0;
    int totalPops = 0;
    int totalRatchets = 0;

    std::set<int> uniquePitchClasses;
    std::vector<double> vels;
    std::vector<double> gates;

    for (int i = 0; i < 128; ++i)
    {
        const auto& s = steps128[static_cast<size_t>(i)];
        if (!s.active)
            continue;

        totalActive++;
        int pitchClass = ((s.semitoneOffset % 12) + 12) % 12;
        uniquePitchClasses.insert (pitchClass);

        vels.push_back (static_cast<double> (s.velocity));
        gates.push_back (static_cast<double> (s.gate));

        if (s.velocity < 60)
            totalGhosts++;
        if (s.accent)
            totalAccents++;
        if (s.octaveShift > 0)
            totalPops++;
        if (s.ratchet > 1)
            totalRatchets++;
    }

    res.notesPerBar = totalActive / 8.0;
    res.noteClasses = static_cast<double> (uniquePitchClasses.size());
    res.ghostPerBar = totalGhosts / 8.0;
    res.accentPerBar = totalAccents / 8.0;
    res.popPerBar = totalPops / 8.0;
    res.ratchetPerBar = totalRatchets / 8.0;

    // Varianza della velocity
    if (vels.size() > 1)
    {
        double sum = std::accumulate (vels.begin(), vels.end(), 0.0);
        double mean = sum / vels.size();
        double sqSum = 0.0;
        for (double v : vels)
            sqSum += (v - mean) * (v - mean);
        res.velocityVariance = sqSum / (vels.size() - 1);
    }
    else
    {
        res.velocityVariance = 0.0;
    }

    // Lunghezza media delle note
    if (!gates.empty())
    {
        double sum = std::accumulate (gates.begin(), gates.end(), 0.0);
        res.avgNoteLength = sum / gates.size();
    }
    else
    {
        res.avgNoteLength = 0.0;
    }

    return res;
}

void exportMidiWithDrums (const std::string& filename,
                          const std::vector<PulseForge::StepData>& steps128,
                          int rootMidiNote = 36)
{
    const int ticksPerQuarter = 960;
    const double stepTicks = ticksPerQuarter * 0.25; // 16th note = 240 ticks

    juce::MidiMessageSequence bassSeq;
    juce::MidiMessageSequence drumSeq;

    // Traccia Basso (Channel 1)
    for (int i = 0; i < 128; ++i)
    {
        const auto& s = steps128[static_cast<size_t>(i)];
        if (!s.active)
            continue;

        double startTick = i * stepTicks;
        int noteNum = rootMidiNote + s.semitoneOffset + (s.octaveShift * 12);
        int vel = juce::jlimit (1, 127, s.velocity);

        if (s.ratchet > 1)
        {
            double subLen = stepTicks / s.ratchet;
            for (int r = 0; r < s.ratchet; ++r)
            {
                double tOn = startTick + r * subLen;
                double tOff = tOn + subLen * 0.65;
                bassSeq.addEvent (juce::MidiMessage::noteOn (1, noteNum, static_cast<juce::uint8> (vel)), tOn);
                bassSeq.addEvent (juce::MidiMessage::noteOff (1, noteNum), tOff);
            }
        }
        else
        {
            double durTicks = stepTicks * s.gate;
            double endTick = startTick + durTicks;
            bassSeq.addEvent (juce::MidiMessage::noteOn (1, noteNum, static_cast<juce::uint8> (vel)), startTick);
            bassSeq.addEvent (juce::MidiMessage::noteOff (1, noteNum), endTick);
        }
    }

    // Traccia Batteria Tech House (Channel 10): 8 battute = 32 beat da 4 quarti
    // Kick = 36 (sui quarti 0, 4, 8, 12)
    // Closed Hi-Hat = 42 (sui levare 2, 6, 10, 14)
    // Clap/Snare = 38 (sul 2 e 4: step 4, 12)
    for (int i = 0; i < 128; ++i)
    {
        double tOn = i * stepTicks;
        int sub4 = i % 4;
        int stepInBar = i % 16;

        // Kick sui 4 quarti
        if (sub4 == 0)
        {
            drumSeq.addEvent (juce::MidiMessage::noteOn (10, 36, static_cast<juce::uint8> (120)), tOn);
            drumSeq.addEvent (juce::MidiMessage::noteOff (10, 36), tOn + 180.0);
        }

        // Clap sul secondo e quarto battito (step 4 e 12)
        if (stepInBar == 4 || stepInBar == 12)
        {
            drumSeq.addEvent (juce::MidiMessage::noteOn (10, 38, static_cast<juce::uint8> (110)), tOn);
            drumSeq.addEvent (juce::MidiMessage::noteOff (10, 38), tOn + 200.0);
        }

        // Open/Closed Hi-Hat sui levare (step 2 di ogni beat)
        if (sub4 == 2)
        {
            drumSeq.addEvent (juce::MidiMessage::noteOn (10, 42, static_cast<juce::uint8> (95)), tOn);
            drumSeq.addEvent (juce::MidiMessage::noteOff (10, 42), tOn + 140.0);
        }
    }

    bassSeq.updateMatchedPairs();
    drumSeq.updateMatchedPairs();

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (ticksPerQuarter);
    midiFile.addTrack (bassSeq);
    midiFile.addTrack (drumSeq);

    juce::File outFile (filename);
    outFile.deleteFile();
    if (auto outStream = std::unique_ptr<juce::FileOutputStream> (outFile.createOutputStream()))
    {
        midiFile.writeTo (*outStream);
    }
}

int main()
{
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "TEST KNOB COMPLEXITY A STRATI SU 50 SEED (FISHER TECH HOUSE BASS)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    const int numSeeds = 50;
    const std::vector<float> complexities = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f };

    std::vector<MetricsResult> avgMetrics (complexities.size());
    int totalSubsetViolations = 0;

    for (size_t cIdx = 0; cIdx < complexities.size(); ++cIdx)
    {
        float cVal = complexities[cIdx];

        MetricsResult acc;
        for (int s = 1; s <= numSeeds; ++s)
        {
            uint32_t seed = static_cast<uint32_t> (100 + s);
            auto candidates = PulseForge::LayeredFisherBassGenerator::buildCandidateEvents (seed, PulseForge::ScaleType::Dorian);

            std::vector<PulseForge::StepData> steps128;
            PulseForge::LayeredFisherBassGenerator::evaluate128Steps (candidates, cVal, steps128);

            MetricsResult m = computeMetrics (steps128);
            acc.notesPerBar += m.notesPerBar;
            acc.noteClasses += m.noteClasses;
            acc.ghostPerBar += m.ghostPerBar;
            acc.accentPerBar += m.accentPerBar;
            acc.popPerBar += m.popPerBar;
            acc.ratchetPerBar += m.ratchetPerBar;
            acc.velocityVariance += m.velocityVariance;
            acc.avgNoteLength += m.avgNoteLength;
        }

        avgMetrics[cIdx].notesPerBar = acc.notesPerBar / numSeeds;
        avgMetrics[cIdx].noteClasses = acc.noteClasses / numSeeds;
        avgMetrics[cIdx].ghostPerBar = acc.ghostPerBar / numSeeds;
        avgMetrics[cIdx].accentPerBar = acc.accentPerBar / numSeeds;
        avgMetrics[cIdx].popPerBar = acc.popPerBar / numSeeds;
        avgMetrics[cIdx].ratchetPerBar = acc.ratchetPerBar / numSeeds;
        avgMetrics[cIdx].velocityVariance = acc.velocityVariance / numSeeds;
        avgMetrics[cIdx].avgNoteLength = acc.avgNoteLength / numSeeds;
    }

    // -------------------------------------------------------------------------
    // Stampa Tabella Metriche
    // -------------------------------------------------------------------------
    std::cout << std::left
              << std::setw (8)  << "COMPL"
              << std::setw (12) << "Note/Batt"
              << std::setw (14) << "Classi Nota"
              << std::setw (10) << "Ghost"
              << std::setw (10) << "Accent"
              << std::setw (8)  << "Pop"
              << std::setw (10) << "Ratchet"
              << std::setw (14) << "Var(Velocity)"
              << std::setw (14) << "Durata Media"
              << std::endl;
    std::cout << std::string (90, '-') << std::endl;

    for (size_t cIdx = 0; cIdx < complexities.size(); ++cIdx)
    {
        const auto& m = avgMetrics[cIdx];
        std::cout << std::left
                  << std::setw (8)  << static_cast<int> (complexities[cIdx])
                  << std::setw (12) << std::fixed << std::setprecision (2) << m.notesPerBar
                  << std::setw (14) << std::fixed << std::setprecision (2) << m.noteClasses
                  << std::setw (10) << std::fixed << std::setprecision (2) << m.ghostPerBar
                  << std::setw (10) << std::fixed << std::setprecision (2) << m.accentPerBar
                  << std::setw (8)  << std::fixed << std::setprecision (2) << m.popPerBar
                  << std::setw (10) << std::fixed << std::setprecision (2) << m.ratchetPerBar
                  << std::setw (14) << std::fixed << std::setprecision (1) << m.velocityVariance
                  << std::setw (14) << std::fixed << std::setprecision (3) << m.avgNoteLength
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    // Verifica Monotonia (Tutte Non Decrescenti)
    // -------------------------------------------------------------------------
    std::cout << "\n==========================================================================================" << std::endl;
    std::cout << "VERIFICA MONOTONIA METRICHE (NON DECRESCENTI)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    bool allNonDecreasing = true;
    for (size_t i = 1; i < complexities.size(); ++i)
    {
        if (avgMetrics[i].notesPerBar < avgMetrics[i-1].notesPerBar - 1e-5 ||
            avgMetrics[i].noteClasses < avgMetrics[i-1].noteClasses - 1e-5 ||
            avgMetrics[i].ghostPerBar < avgMetrics[i-1].ghostPerBar - 1e-5 ||
            avgMetrics[i].accentPerBar < avgMetrics[i-1].accentPerBar - 1e-5 ||
            avgMetrics[i].popPerBar < avgMetrics[i-1].popPerBar - 1e-5 ||
            avgMetrics[i].ratchetPerBar < avgMetrics[i-1].ratchetPerBar - 1e-5 ||
            avgMetrics[i].velocityVariance < avgMetrics[i-1].velocityVariance - 1e-5 ||
            avgMetrics[i].avgNoteLength < avgMetrics[i-1].avgNoteLength - 1e-5)
        {
            allNonDecreasing = false;
        }
    }

    std::cout << "Esito Monotonia (Tutte e 8 le metriche non decrescenti): "
              << (allNonDecreasing ? "PASS [100%]" : "FAIL") << std::endl;

    // -------------------------------------------------------------------------
    // Verifica Proprietà di Sottoinsieme
    // -------------------------------------------------------------------------
    std::cout << "\n==========================================================================================" << std::endl;
    std::cout << "VERIFICA PROPRIETA' DI SOTTOINSIEME SU 50 SEED" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    for (int s = 1; s <= numSeeds; ++s)
    {
        uint32_t seed = static_cast<uint32_t> (100 + s);
        auto candidates = PulseForge::LayeredFisherBassGenerator::buildCandidateEvents (seed, PulseForge::ScaleType::Dorian);

        for (size_t c1 = 0; c1 < complexities.size(); ++c1)
        {
            for (size_t c2 = c1 + 1; c2 < complexities.size(); ++c2)
            {
                std::vector<PulseForge::StepData> stepsC1, stepsC2;
                PulseForge::LayeredFisherBassGenerator::evaluate128Steps (candidates, complexities[c1], stepsC1);
                PulseForge::LayeredFisherBassGenerator::evaluate128Steps (candidates, complexities[c2], stepsC2);

                for (int st = 0; st < 128; ++st)
                {
                    if (stepsC1[st].active && !stepsC2[st].active)
                    {
                        totalSubsetViolations++;
                    }
                }
            }
        }
    }

    std::cout << "Coppie di complessita' testate per seed: 55" << std::endl;
    std::cout << "Totale verifiche di inclusione su 50 seed: " << numSeeds * 55 * 128 << " step" << std::endl;
    std::cout << "Violazioni di sottoinsieme riscontrate: " << totalSubsetViolations << std::endl;
    std::cout << "Esito Proprieta' Sottoinsieme: " << (totalSubsetViolations == 0 ? "PASS (Zero violazioni)" : "FAIL") << std::endl;

    // -------------------------------------------------------------------------
    // Esportazione MIDI Seed 101 a COMPLEXITY 0, 25, 50, 75, 100 con Batteria
    // -------------------------------------------------------------------------
    std::cout << "\n==========================================================================================" << std::endl;
    std::cout << "ESPORTAZIONE MIDI SEED 101 CON BATTERIA TECH HOUSE (0, 25, 50, 75, 100)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    const std::vector<float> exportComplexities = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
    auto cand101 = PulseForge::LayeredFisherBassGenerator::buildCandidateEvents (101, PulseForge::ScaleType::Dorian);

    for (float cVal : exportComplexities)
    {
        std::vector<PulseForge::StepData> steps128;
        PulseForge::LayeredFisherBassGenerator::evaluate128Steps (cand101, cVal, steps128);

        char nameBuf[64];
        std::snprintf (nameBuf, sizeof (nameBuf), "Tests/Fisher_Complexity_%03d_WithDrums.mid", static_cast<int> (cVal));
        exportMidiWithDrums (nameBuf, steps128, 36);
        std::cout << "  Esportato con successo: " << nameBuf << std::endl;
    }

    std::cout << "\nTest completato con successo." << std::endl;
    return (allNonDecreasing && totalSubsetViolations == 0) ? 0 : 1;
}
