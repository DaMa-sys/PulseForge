#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/PatternData.h"
#include "../Source/GenreProfiles.h"
#include "../Source/LayeredPatternGenerator.h"
#include "../Source/KalkbrennerBricks.h"
#include "../Source/FredAgainBricks.h"
#include "../Source/DisclosureBricks.h"
#include "../Source/BonoboBricks.h"
#include "../Source/HouseEdmBricks.h"
#include "../Source/StyleGenerator.h"

// Calcola somiglianza tra due pattern (0..1)
double calculateSimilarity (const std::vector<PulseForge::StepData>& p1,
                            const std::vector<PulseForge::StepData>& p2)
{
    int total = 0;
    int matches = 0;
    int len = std::min (p1.size(), p2.size());
    for (int i = 0; i < len; ++i)
    {
        total++;
        if (p1[i].active == p2[i].active)
        {
            if (!p1[i].active)
            {
                matches++;
            }
            else
            {
                int pDiff = std::abs (p1[i].semitoneOffset - p2[i].semitoneOffset);
                if (pDiff == 0 && p1[i].octaveShift == p2[i].octaveShift)
                    matches++;
                else if (pDiff <= 2)
                    matches += 0.5;
            }
        }
    }
    return total > 0 ? (static_cast<double>(matches) / total) : 0.0;
}

struct ArtistTestConfig
{
    std::string name;
    PulseForge::GenreStyle genre;
    PulseForge::InstrumentRole role;
    PulseForge::ScaleType scale;
};

int main()
{
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "SUITE DI VERIFICA COMPLETA: TUTTI GLI ARTISTI E GENERI MODULARI" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    std::vector<ArtistTestConfig> artists = {
        { "Fisher (Tech House Bass)",          PulseForge::GenreStyle::BerlinSchool,   PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::Dorian },
        { "Kalkbrenner (Melodic Techno Arp)", PulseForge::GenreStyle::MelodicTechno,  PulseForge::InstrumentRole::ClassicArp,  PulseForge::ScaleType::MinorNatural },
        { "Kalkbrenner (Melodic Techno Bass)",PulseForge::GenreStyle::MelodicTechno,  PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::MinorNatural },
        { "Fred again.. (Emotional UK Lead)", PulseForge::GenreStyle::FunkBoogie,     PulseForge::InstrumentRole::LeadHook,    PulseForge::ScaleType::MinorNatural },
        { "Fred again.. (Emotional UK Bass)", PulseForge::GenreStyle::FunkBoogie,     PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::MinorNatural },
        { "Disclosure (UK Garage Bass)",      PulseForge::GenreStyle::DetroitElectro, PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::Dorian },
        { "Disclosure (UK Garage Lead/Stab)", PulseForge::GenreStyle::DetroitElectro, PulseForge::InstrumentRole::LeadHook,    PulseForge::ScaleType::Dorian },
        { "Bonobo (Organic Downtempo Arp)",   PulseForge::GenreStyle::EuclideanIDM,   PulseForge::InstrumentRole::ClassicArp,  PulseForge::ScaleType::Dorian },
        { "Bonobo (Organic Downtempo Bass)",  PulseForge::GenreStyle::EuclideanIDM,   PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::Dorian },
        { "Classic House (Chicago Bass)",     PulseForge::GenreStyle::ClassicHouse,   PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::Dorian },
        { "Italo / EDM (Moroder Bass)",       PulseForge::GenreStyle::ItaloSynthwave, PulseForge::InstrumentRole::MonoBass,    PulseForge::ScaleType::MinorNatural }
    };

    bool allPassed = true;

    for (const auto& art : artists)
    {
        std::cout << "\n>>> TEST: " << art.name << std::endl;

        // 1. Unicita' su 100 seed a coppie (4950 coppie) su 128 step (8 battute complete)
        const int numSeeds = 100;
        std::vector<std::vector<PulseForge::StepData>> patterns (numSeeds);

        for (int s = 0; s < numSeeds; ++s)
        {
            uint32_t seed = static_cast<uint32_t> (1000 + s * 17);
            std::vector<PulseForge::LayeredCandidateEvent> cand;

            switch (art.genre)
            {
                case PulseForge::GenreStyle::BerlinSchool:
                    cand = PulseForge::LayeredFisherBassGenerator::buildCandidateEvents (seed, art.scale);
                    break;
                case PulseForge::GenreStyle::MelodicTechno:
                    cand = PulseForge::LayeredKalkbrennerGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                case PulseForge::GenreStyle::FunkBoogie:
                    cand = PulseForge::LayeredFredAgainGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                case PulseForge::GenreStyle::DetroitElectro:
                    cand = PulseForge::LayeredDisclosureGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                case PulseForge::GenreStyle::EuclideanIDM:
                    cand = PulseForge::LayeredBonoboGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                case PulseForge::GenreStyle::ClassicHouse:
                    cand = PulseForge::LayeredClassicHouseGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                case PulseForge::GenreStyle::ItaloSynthwave:
                    cand = PulseForge::LayeredItaloEdmGenerator::buildCandidateEvents (seed, art.role, art.scale);
                    break;
                default:
                    break;
            }

            PulseForge::LayeredFisherBassGenerator::evaluate128Steps (cand, 65.0f, patterns[s]);
        }

        double simSum = 0.0;
        int pairCount = 0;
        int identical = 0;

        for (int i = 0; i < numSeeds; ++i)
        {
            for (int j = i + 1; j < numSeeds; ++j)
            {
                double sim = calculateSimilarity (patterns[i], patterns[j]);
                simSum += sim;
                if (sim >= 0.999) identical++;
                pairCount++;
            }
        }

        double avgSim = (simSum / pairCount) * 100.0;
        std::cout << "  - Unicita': Identici = " << identical << " / " << pairCount
                  << ", Somiglianza media = " << std::fixed << std::setprecision(1) << avgSim << "%";

        if (identical == 0 && avgSim < 60.0)
        {
            std::cout << " [PASS]" << std::endl;
        }
        else
        {
            std::cout << " [FAIL]" << std::endl;
            allPassed = false;
        }

        // 2. Monotonia e proprieta' di sottoinsieme su 11 livelli di Complexity (0..100)
        int subsetViolations = 0;
        for (int s = 0; s < 10; ++s)
        {
            uint32_t seed = static_cast<uint32_t> (2000 + s * 31);
            std::vector<PulseForge::StepData> prevActive (32);
            bool isFirst = true;

            for (int c = 0; c <= 100; c += 10)
            {
                PulseForge::PatternBank bank;
                bank.numSteps = 32;

                PulseForge::StyleGenerator::GenerationParams p;
                p.genre = art.genre;
                p.role = art.role;
                p.scale = art.scale;
                p.density = static_cast<float>(c) / 100.0f;

                PulseForge::StyleGenerator::generatePattern (bank, p, seed);

                if (!isFirst)
                {
                    for (int i = 0; i < 32; ++i)
                    {
                        if (prevActive[i].active && !bank.steps[static_cast<size_t>(i)].active)
                            subsetViolations++;
                    }
                }

                for (int i = 0; i < 32; ++i)
                    prevActive[i] = bank.steps[static_cast<size_t>(i)];

                isFirst = false;
            }
        }

        std::cout << "  - Sottoinsieme e Monotonia: Violazioni = " << subsetViolations;
        if (subsetViolations == 0)
        {
            std::cout << " [PASS]" << std::endl;
        }
        else
        {
            std::cout << " [FAIL]" << std::endl;
            allPassed = false;
        }
    }

    std::cout << "\n==========================================================================================" << std::endl;
    if (allPassed)
    {
        std::cout << "ESITO FINALE: TUTTI I PROFILI ARTISTA HANNO SUPERATO I TEST AL 100%!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "ESITO FINALE: ALCUNI TEST SONO FALLITI." << std::endl;
        return 1;
    }
}
