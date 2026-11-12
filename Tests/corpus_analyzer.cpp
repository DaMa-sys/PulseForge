#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

struct RoleAnalysis
{
    std::string roleName;
    int fileCount = 0;
    std::vector<double> velocities;
    std::vector<double> gates;
    std::vector<double> noteCounts;
    std::vector<double> pitchClasses;
    std::vector<double> syncopations;
    std::vector<double> repetitions;
    std::vector<double> foldedLeaps;
    std::vector<double> restRatios;

    std::map<std::string, int> rhythmCells;
    std::map<std::string, int> melodyProfiles;
};

// Sincopazione Longuet-Higgins & Lee normalizzata [0, 1]
double computeSyncopationLHL (const std::vector<bool>& active16)
{
    static const int metricWeight[16] = {
        4, 1, 2, 1,
        3, 1, 2, 1,
        3, 1, 2, 1,
        3, 1, 2, 1
    };

    double totalSync = 0.0;
    int count = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (active16[static_cast<size_t>(i)])
        {
            int next = (i + 1) % 16;
            if (!active16[static_cast<size_t>(next)])
            {
                int diff = metricWeight[next] - metricWeight[i];
                if (diff > 0)
                    totalSync += diff;
            }
            count++;
        }
    }
    return juce::jlimit (0.0, 1.0, totalSync / 8.0);
}

// Percentili (10th, 50th, 90th)
void getPercentiles (std::vector<double>& v, double& p10, double& p50, double& p90, double& mean)
{
    if (v.empty())
    {
        p10 = p50 = p90 = mean = 0.0;
        return;
    }
    std::sort (v.begin(), v.end());
    double sum = 0.0;
    for (double x : v) sum += x;
    mean = sum / v.size();

    auto idx = [&v](double pct) -> double {
        size_t i = static_cast<size_t>(pct * (v.size() - 1));
        return v[i];
    };
    p10 = idx (0.10);
    p50 = idx (0.50);
    p90 = idx (0.90);
}

void analyzeRoleFiles (const juce::var& jsonList, const std::string& roleName, RoleAnalysis& out)
{
    out.roleName = roleName;
    auto* arr = jsonList.getArray();
    if (arr == nullptr) return;

    for (int i = 0; i < arr->size(); ++i)
    {
        juce::String filePath = (*arr)[i]["path"].toString();
        juce::File f (filePath);
        if (!f.existsAsFile()) continue;

        juce::FileInputStream is (f);
        if (!is.openedOk()) continue;

        juce::MidiFile mf;
        if (!mf.readFrom (is)) continue;

        mf.convertTimestampTicksToSeconds();
        juce::MidiMessageSequence seq;
        for (int t = 0; t < mf.getNumTracks(); ++t)
            seq.addSequence (*mf.getTrack (t), 0.0);
        seq.updateMatchedPairs();

        std::vector<double> fileVels;
        std::vector<double> fileGates;
        std::vector<int> fileNotes;
        std::set<int> pitchClasses;

        double totalDur = seq.getEndTime();
        if (totalDur < 0.2) continue;

        // Discretizza in griglia a 16esimi su una battuta tipica
        std::vector<bool> active16 (16, false);
        std::string rhythm4 = "....";

        for (int e = 0; e < seq.getNumEvents(); ++e)
        {
            auto msg = seq.getEventPointer (e)->message;
            if (msg.isNoteOn())
            {
                fileVels.push_back (msg.getVelocity());
                fileNotes.push_back (msg.getNoteNumber());
                pitchClasses.insert (((msg.getNoteNumber() % 12) + 12) % 12);

                int step16 = static_cast<int>(std::floor ((msg.getTimeStamp() / totalDur) * 16.0)) % 16;
                active16[static_cast<size_t>(step16)] = true;

                // Trova NoteOff corrispondente
                auto* offEvent = seq.getEventPointer (e)->noteOffObject;
                if (offEvent != nullptr)
                {
                    double dur = offEvent->message.getTimeStamp() - msg.getTimeStamp();
                    double gateRel = dur / (totalDur / 16.0);
                    fileGates.push_back (juce::jlimit (0.1, 4.0, gateRel));
                }
                else
                {
                    fileGates.push_back (0.65);
                }
            }
        }

        if (fileNotes.empty()) continue;

        out.fileCount++;
        out.noteCounts.push_back (static_cast<double>(fileNotes.size()));
        out.pitchClasses.push_back (static_cast<double>(pitchClasses.size()));

        for (double v : fileVels) out.velocities.push_back (v);
        for (double g : fileGates) out.gates.push_back (g);

        // Calcola pause (% step vuoti)
        int activeCount = 0;
        for (bool a : active16) if (a) activeCount++;
        out.restRatios.push_back (1.0 - (static_cast<double>(activeCount) / 16.0));

        // Sincopazione
        out.syncopations.push_back (computeSyncopationLHL (active16));

        // Cellula ritmica (pattern dei primi 4 step)
        std::string rCell = "";
        for (int s = 0; s < 4; ++s)
            rCell += active16[static_cast<size_t>(s)] ? "X" : ".";
        out.rhythmCells[rCell]++;

        // Ripetizione e salti ripiegati
        if (fileNotes.size() >= 2)
        {
            int repCount = 0;
            int leapCount = 0;
            for (size_t n = 1; n < fileNotes.size(); ++n)
            {
                int diff = std::abs (fileNotes[n] - fileNotes[n - 1]);
                if (diff == 0) repCount++;
                int folded = diff % 12;
                if (folded > 5) folded = 12 - folded;
                if (folded > 4) leapCount++; // Salto > quarta (5 semitoni)
            }
            out.repetitions.push_back (static_cast<double>(repCount) / (fileNotes.size() - 1));
            out.foldedLeaps.push_back (static_cast<double>(leapCount) / (fileNotes.size() - 1));

            // Profilo melodico dei primi 3 intervalli
            if (fileNotes.size() >= 4)
            {
                std::string melProfile = "";
                for (size_t n = 1; n < 4; ++n)
                {
                    int d = fileNotes[n] - fileNotes[n - 1];
                    melProfile += (d >= 0 ? "+" : "") + std::to_string (d) + " ";
                }
                out.melodyProfiles[melProfile]++;
            }
        }
    }
}

int main()
{
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "ANALISI STATISTICA CORPUS MELODIC TECHNO (PAUL KALKBRENNER)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    juce::File cacheFile ("Tests/corpus_melodic_techno_cache.json");
    if (!cacheFile.existsAsFile())
    {
        std::cerr << "ERRORE: File di cache non trovato: " << cacheFile.getFullPathName() << std::endl;
        return 1;
    }

    auto json = juce::JSON::parse (cacheFile);
    if (!json.isObject())
    {
        std::cerr << "ERRORE: JSON non valido" << std::endl;
        return 1;
    }

    RoleAnalysis arpAnalysis, bassAnalysis, chordAnalysis;
    analyzeRoleFiles (json["arp"], "ARP", arpAnalysis);
    analyzeRoleFiles (json["bass"], "BASS", bassAnalysis);
    analyzeRoleFiles (json["chords"], "CHORDS", chordAnalysis);

    auto printStats = [](const RoleAnalysis& ra)
    {
        std::cout << "\n------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "RUOLO: " << ra.roleName << " (File analizzati: " << ra.fileCount << ")" << std::endl;
        std::cout << "------------------------------------------------------------------------------------------" << std::endl;

        auto printMetric = [](const char* name, std::vector<double> v)
        {
            double p10, p50, p90, mean;
            getPercentiles (v, p10, p50, p90, mean);
            std::cout << "  " << std::left << std::setw (24) << name
                      << " Media: " << std::fixed << std::setprecision (2) << std::setw (7) << mean
                      << " [10°-90° perc: " << std::setw (5) << p10 << " .. " << std::setw (5) << p90 << "]" << std::endl;
        };

        printMetric ("Velocity", ra.velocities);
        printMetric ("Gate (durata rel.)", ra.gates);
        printMetric ("Classi di Nota", ra.pitchClasses);
        printMetric ("Sincopazione [0-1]", ra.syncopations);
        printMetric ("Ripetizione", ra.repetitions);
        printMetric ("Salti > 4a (%)", ra.foldedLeaps);
        printMetric ("Pause (% vuoti)", ra.restRatios);

        // Top 5 Cellule Ritmiche
        std::cout << "\n  Top 5 Cellule Ritmiche (4 step):" << std::endl;
        std::vector<std::pair<std::string, int>> sortedRhythm (ra.rhythmCells.begin(), ra.rhythmCells.end());
        std::sort (sortedRhythm.begin(), sortedRhythm.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
        for (size_t i = 0; i < std::min (size_t (5), sortedRhythm.size()); ++i)
        {
            double pct = (100.0 * sortedRhythm[i].second) / ra.fileCount;
            std::cout << "    " << (i + 1) << ". [" << sortedRhythm[i].first << "]  ("
                      << std::fixed << std::setprecision (1) << pct << "%)" << std::endl;
        }

        // Top 5 Profili Melodici
        std::cout << "\n  Top 5 Profili Melodici (semitoni consecutivi):" << std::endl;
        std::vector<std::pair<std::string, int>> sortedMel (ra.melodyProfiles.begin(), ra.melodyProfiles.end());
        std::sort (sortedMel.begin(), sortedMel.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
        for (size_t i = 0; i < std::min (size_t (5), sortedMel.size()); ++i)
        {
            double pct = (100.0 * sortedMel[i].second) / ra.fileCount;
            std::cout << "    " << (i + 1) << ". " << std::setw(15) << sortedMel[i].first << " ("
                      << std::fixed << std::setprecision (1) << pct << "%)" << std::endl;
        }
    };

    printStats (arpAnalysis);
    printStats (bassAnalysis);
    printStats (chordAnalysis);

    return 0;
}
