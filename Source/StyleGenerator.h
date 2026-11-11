#pragma once
#include "PatternData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <random>

namespace PulseForge
{

class StyleGenerator
{
public:
    struct GenerationParams
    {
        GenreStyle genre = GenreStyle::ClassicHouse;
        InstrumentRole role = InstrumentRole::MonoBass;
        EngineMode mode = EngineMode::Sequencer;
        ScaleType scale = ScaleType::Dorian;
        float density = 0.65f;       // 0.0 .. 1.0
        float syncopation = 0.55f;   // 0.0 .. 1.0
        float spice = 0.45f;         // 0.0 .. 1.0 (slides, accents, ratchets, octave jumps)
        float pitchRange = 0.50f;    // 0.0 .. 1.0
        int numSteps = 16;
    };

    struct PhraseTemplate
    {
        const char* rhythmMask;          // 16 chars: 'X' = core hit, 'o' = optional density hit, '.' = rest
        std::array<int, 16> semitones;   // Semitone offsets from Root (quantized to chosen scale)
        std::array<int, 16> octaves;     // Octave shifts (-1, 0, +1)
        float sustainRatio;              // How much of the gap to the next note should be filled (0.35 .. 0.95)
    };

    static void getRecommendedSetup (GenreStyle genre, InstrumentRole role,
                                     ScaleType& outScale, int& outBaseOctave, float& outSwing)
    {
        outSwing = 0.0f;
        outBaseOctave = 2;

        switch (genre)
        {
            case GenreStyle::ClassicHouse:
                outScale = ScaleType::Dorian;
                outSwing = 0.24f;
                break;
            case GenreStyle::AcidTechno:
                outScale = ScaleType::Phrygian;
                outSwing = 0.06f;
                break;
            case GenreStyle::ItaloSynthwave:
                outScale = ScaleType::MinorNatural;
                outSwing = 0.0f;
                break;
            case GenreStyle::BerlinSchool:
                outScale = ScaleType::Dorian;
                outSwing = 0.0f;
                break;
            case GenreStyle::MelodicTechno:
                outScale = ScaleType::MinorNatural;
                outSwing = 0.05f;
                break;
            case GenreStyle::DetroitElectro:
                outScale = ScaleType::MinorNatural;
                outSwing = 0.14f;
                break;
            case GenreStyle::FunkBoogie:
                outScale = ScaleType::MinorPentatonic;
                outSwing = 0.30f;
                break;
            case GenreStyle::EuclideanIDM:
                outScale = ScaleType::MinorNatural;
                outSwing = 0.10f;
                break;
            default:
                outScale = ScaleType::MinorNatural;
                break;
        }

        switch (role)
        {
            case InstrumentRole::MonoBass:
            case InstrumentRole::AcidLine:
                outBaseOctave = 1; // C1 / C2 deep analog bass range
                break;
            default:
                outBaseOctave = 2; // C2 / C3 midrange for Arps, Leads & Chords
                break;
        }
    }

    // Calculates musical gate lengths based on inter-onset distance (gap to next active step)
    static void applyMusicalGates (PatternBank& bank, float sustainRatio, bool isArpOrPluck, float spice)
    {
        const int n = juce::jlimit (1, kMaxSteps, bank.numSteps);

        for (int i = 0; i < n; ++i)
        {
            auto& s = bank.steps[static_cast<size_t>(i)];
            if (!s.active)
            {
                s.gate = 0.55f;
                continue;
            }

            // Find distance in steps to the next active step
            int gap = 1;
            for (int stepAhead = 1; stepAhead < n; ++stepAhead)
            {
                if (bank.steps[static_cast<size_t>((i + stepAhead) % n)].active)
                {
                    gap = stepAhead;
                    break;
                }
            }

            if (s.slide)
            {
                // True hardware legato: slightly exceed the gap to overlap into the next active note!
                s.gate = static_cast<float> (gap) + 0.08f;
            }
            else if (isArpOrPluck)
            {
                // Arps & plucks stay crisp, with slight sustain on wider gaps
                s.gate = (gap == 1) ? 0.68f : juce::jmin (1.80f, static_cast<float> (gap) * 0.65f);
            }
            else
            {
                // Basslines, Leads & Chords fill the musical gap (like SmashUpTheStudio & Nano Funky House MIDIs!)
                float targetGate = static_cast<float> (gap) * sustainRatio;
                if (gap == 1)
                    targetGate = 0.72f + 0.12f * spice;
                s.gate = juce::jlimit (0.35f, 3.80f, targetGate);
            }
        }
    }

    static void generateHarmonyChords (PatternBank& bank, const GenerationParams& p, std::mt19937& rng)
    {
        auto randInt   = [&](int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };
        auto randFloat = [&]() { return std::uniform_real_distribution<float> (0.0f, 1.0f) (rng); };

        const int n = juce::jlimit (4, kMaxSteps, p.numSteps);
        bank.numSteps = n;

        for (int i = 0; i < kMaxSteps; ++i)
        {
            auto& s = bank.steps[static_cast<size_t>(i)];
            s.active = false;
            s.semitoneOffset = 0;
            s.octaveShift = 0;
            s.accent = false;
            s.slide = false;
            s.ratchet = 1;
            s.probability = 100;
            s.chordType = 3;
        }

        struct ChordChange
        {
            int step;
            int degree;
            int octave;
            float gateSteps;
            int voicing;
        };

        auto pickRichVoicing = [&](int degree) -> int
        {
            if (degree == 0)
            {
                // Tonic: Open 9th (4), Modal 11th (5), or Soulful 6/9 (6)
                const int tonicVoicings[] = { 4, 4, 5, 6 };
                return tonicVoicings[randInt (0, 3)];
            }
            if (degree == 3 || degree == 5)
            {
                // Subdominant / iv / VI: Modal 11th (5), Open 9th (4), or Soulful 6/9 (6)
                const int subVoicings[] = { 5, 4, 6, 3 };
                return subVoicings[randInt (0, 3)];
            }
            if (degree == 4 || degree == 6)
            {
                // Dominant / Cadence (v, VII): 9sus4 tension (8), Open 9th (4), or Drop-2 7th (3)
                const int domVoicings[] = { 8, 4, 8, 3 };
                return domVoicings[randInt (0, 3)];
            }
            // Passing / ii / III: Open 9th (4), Add9 (2), or Quartal (7)
            const int passVoicings[] = { 4, 2, 5, 7 };
            return passVoicings[randInt (0, 3)];
        };

        std::vector<ChordChange> changes;

        if (n <= 4)
        {
            if (randFloat() < 0.65f)
            {
                changes.push_back ({ 0, 0, 0, 3.8f, pickRichVoicing (0) });
            }
            else
            {
                changes.push_back ({ 0, 0, 0, 1.9f, pickRichVoicing (0) });
                const int deg2 = (p.scale == ScaleType::MajorNatural) ? 3 : 3;
                changes.push_back ({ 2, deg2, 0, 1.9f, pickRichVoicing (deg2) });
            }
        }
        else if (n <= 8)
        {
            const int changeStep = (randFloat() < 0.7f) ? 4 : 6;
            const int secondDegreeChoices[] = { 3, 5, 4, 6 };
            const int secondDegree = secondDegreeChoices[randInt (0, 3)];

            changes.push_back ({ 0, 0, 0, static_cast<float> (changeStep) * 0.95f, pickRichVoicing (0) });
            changes.push_back ({ changeStep, secondDegree, (secondDegree >= 4 ? -1 : 0), static_cast<float> (n - changeStep) * 0.95f, pickRichVoicing (secondDegree) });
        }
        else if (n <= 16)
        {
            const int styleType = randInt (0, 3);
            if (styleType == 0)
            {
                const int deg2 = (p.scale == ScaleType::MajorNatural) ? 3 : (randFloat() < 0.5f ? 3 : 5);
                changes.push_back ({ 0, 0, 0, 7.8f, pickRichVoicing (0) });
                changes.push_back ({ 8, deg2, (deg2 >= 4 ? -1 : 0), 7.8f, pickRichVoicing (deg2) });
            }
            else if (styleType == 1)
            {
                const int deg2 = (p.scale == ScaleType::Phrygian) ? 1 : 3;
                const int deg3 = 4;
                changes.push_back ({ 0,  0,    0, 5.8f, pickRichVoicing (0) });
                changes.push_back ({ 6,  deg2, (deg2 >= 4 ? -1 : 0), 3.8f, pickRichVoicing (deg2) });
                changes.push_back ({ 10, deg3, -1, 5.8f, pickRichVoicing (deg3) });
            }
            else
            {
                int d1 = 0, d2 = 5, d3 = 2, d4 = 6;
                if (p.scale == ScaleType::Phrygian || p.scale == ScaleType::PhrygianDominant)
                {
                    d1 = 0; d2 = 1; d3 = 0; d4 = 6;
                }
                else if (p.scale == ScaleType::MajorNatural || p.scale == ScaleType::Mixolydian)
                {
                    d1 = 0; d2 = 4; d3 = 5; d4 = 3;
                }

                changes.push_back ({ 0,  d1, 0, 3.8f, pickRichVoicing (d1) });
                changes.push_back ({ 4,  d2, (d2 >= 4 ? -1 : 0), 3.8f, pickRichVoicing (d2) });
                changes.push_back ({ 8,  d3, 0, 3.8f, pickRichVoicing (d3) });
                changes.push_back ({ 12, d4, (d4 >= 4 ? -1 : 0), 3.8f, pickRichVoicing (d4) });
            }
        }
        else // 32 steps: True 2-bar evolving progression with variations and cadences
        {
            const int styleType = randInt (0, 3);

            if (styleType == 0)
            {
                // Style 0: 4 Broad Sustained Chords (8 steps each = 2 bars)
                std::vector<std::vector<int>> progs;
                if (p.scale == ScaleType::Dorian || p.scale == ScaleType::MinorNatural || p.scale == ScaleType::HarmonicMinor)
                {
                    progs = {
                        { 0, 5, 2, 6 },  // i - VI - III - VII (Epic / Cinematic)
                        { 0, 3, 4, 6 },  // i - iv - v - VII (Classic Deep House)
                        { 0, 1, 0, 6 },  // i - ii - i - VII (Dorian modal drive)
                        { 0, 5, 3, 4 },  // i - VI - iv - v (Soulful House)
                        { 0, 4, 5, 6 },  // i - v - VI - VII (Melodic Progressive)
                        { 0, 2, 3, 4 }   // i - III - iv - v (Ascending minor)
                    };
                }
                else if (p.scale == ScaleType::Phrygian || p.scale == ScaleType::PhrygianDominant)
                {
                    progs = {
                        { 0, 1, 0, 6 },  // i - bII - i - bVII (Dark Tension)
                        { 0, 1, 2, 1 },  // i - bII - bIII - bII (Hypnotic Acid)
                        { 0, 6, 1, 0 },  // i - bVII - bII - i (Turnaround)
                        { 0, 1, 6, 4 }   // i - bII - bVII - v (Deep Phrygian)
                    };
                }
                else // Major / Mixolydian / Lydian
                {
                    progs = {
                        { 0, 4, 5, 3 },  // I - V - vi - IV (Anthem)
                        { 0, 3, 4, 0 },  // I - IV - V - I (Classic Boogie)
                        { 0, 5, 3, 4 },  // I - vi - IV - V (50s / Neo-Soul)
                        { 0, 1, 4, 3 }   // I - ii - V - IV (Jazz Pop)
                    };
                }

                const auto& pr = progs[static_cast<size_t>(randInt (0, static_cast<int>(progs.size()) - 1))];
                changes.push_back ({ 0,  pr[0], 0, 7.8f, pickRichVoicing (pr[0]) });
                changes.push_back ({ 8,  pr[1], (pr[1] >= 4 ? -1 : 0), 7.8f, pickRichVoicing (pr[1]) });
                changes.push_back ({ 16, pr[2], (pr[2] >= 4 ? -1 : 0), 7.8f, pickRichVoicing (pr[2]) });
                changes.push_back ({ 24, pr[3], (pr[3] >= 4 ? -1 : 0), 7.8f, pickRichVoicing (pr[3]) });
            }
            else if (styleType == 1)
            {
                // Style 1: Syncopated House Rhythm across 32 steps (Bar 1 Groove -> Bar 2 Variation & Passing Chords)
                int d1 = 0, d2 = 3, d3 = 4, d4 = 6;
                if (p.scale == ScaleType::Phrygian) { d1 = 0; d2 = 1; d3 = 6; d4 = 1; }
                else if (p.scale == ScaleType::MajorNatural) { d1 = 0; d2 = 3; d3 = 4; d4 = 0; }

                // Bar 1 (0..15): Off-beat rhythmic push
                changes.push_back ({ 0,  d1, 0, 3.2f, pickRichVoicing (d1) });
                changes.push_back ({ 4,  d1, 0, 2.0f, 6 }); // 6/9 groove stab
                changes.push_back ({ 8,  d2, (d2 >= 4 ? -1 : 0), 4.2f, pickRichVoicing (d2) });
                changes.push_back ({ 14, d3, (d3 >= 4 ? -1 : 0), 1.8f, 8 }); // 9sus4 push

                // Bar 2 (16..31): Dynamic cadence with turnaround passing chords
                changes.push_back ({ 16, d1, 0, 3.2f, pickRichVoicing (d1) });
                changes.push_back ({ 20, d2, (d2 >= 4 ? -1 : 0), 3.2f, 5 }); // 11th shimmer
                changes.push_back ({ 24, d4, (d4 >= 4 ? -1 : 0), 3.0f, pickRichVoicing (d4) });
                changes.push_back ({ 28, d3, (d3 >= 4 ? -1 : 0), 1.8f, 8 }); // 9sus4 tension
                changes.push_back ({ 30, (d1 == 0 ? 6 : 0), -1, 1.8f, 2 });  // Passing add9 resolution
            }
            else if (styleType == 2)
            {
                // Style 2: Bar 1 Broad Foundation (steps 0, 8) + Bar 2 Fast Evolution (steps 16, 20, 24, 28)
                int d1 = 0, d2 = (p.scale == ScaleType::Phrygian ? 1 : 5);
                int d3 = 3, d4 = 4, d5 = 5, d6 = 6;
                if (p.scale == ScaleType::MajorNatural) { d1 = 0; d2 = 4; d3 = 5; d4 = 3; d5 = 1; d6 = 4; }

                changes.push_back ({ 0,  d1, 0, 7.5f, pickRichVoicing (d1) });
                changes.push_back ({ 8,  d2, (d2 >= 4 ? -1 : 0), 7.5f, pickRichVoicing (d2) });
                changes.push_back ({ 16, d3, (d3 >= 4 ? -1 : 0), 3.8f, 5 }); // 11th
                changes.push_back ({ 20, d4, (d4 >= 4 ? -1 : 0), 3.8f, 4 }); // 9th
                changes.push_back ({ 24, d5, (d5 >= 4 ? -1 : 0), 3.8f, 6 }); // 6/9
                changes.push_back ({ 28, d6, (d6 >= 4 ? -1 : 0), 3.8f, 8 }); // 9sus4
            }
            else
            {
                // Style 3: 8 Moving Diatonic Chords (every 4 steps across 2 bars)
                const int degs[] = { 0, 2, 3, 5, 4, 3, 1, 6 };
                for (int c = 0; c < 8; ++c)
                {
                    int deg = degs[c];
                    if (p.scale == ScaleType::Phrygian && deg == 2) deg = 1;
                    changes.push_back ({ c * 4, deg, (deg >= 4 ? -1 : 0), 3.7f, pickRichVoicing (deg) });
                }
            }
        }

        for (const auto& ch : changes)
        {
            if (ch.step >= 0 && ch.step < n)
            {
                auto& s = bank.steps[static_cast<size_t>(ch.step)];
                s.active = true;
                s.semitoneOffset = scaleDegreeToSemitone (ch.degree, p.scale);
                s.octaveShift = ch.octave;
                s.velocity = randInt (96, 108);
                s.gate = ch.gateSteps;
                s.accent = (ch.step == 0);
                s.slide = false;
                s.ratchet = 1;
                s.probability = 100;
                s.ccValue = s.accent ? 95 : 75;
                s.chordType = ch.voicing;
            }
        }
    }

    static void generatePattern (PatternBank& bank, const GenerationParams& p, uint32_t seed = 0)
    {
        std::mt19937 rng (seed != 0 ? seed : std::random_device{}());
        auto randFloat = [&]() { return std::uniform_real_distribution<float> (0.0f, 1.0f) (rng); };
        auto randInt   = [&](int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };

        bank.numSteps = juce::jlimit (1, kMaxSteps, p.numSteps);

        if (p.role == InstrumentRole::HarmonyChords)
        {
            generateHarmonyChords (bank, p, rng);
            return;
        }

        //======================================================================
        // Curated Library of Real Musical Templates Distilled from G:\samples\MIDI
        //======================================================================
        static const PhraseTemplate houseTemplates[] = {
            // 1. SmashUpTheStudio bassloop16: Classic Deep House syncopated groove
            { "X.oX.XX.X.o.X..X", { 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 7, 0, 0, -2 }, { 0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0 }, 0.82f },
            // 2. PML Tribe Double Tresillo (3+3+2 | 3+3+2) Call & Response
            { "X..X..X.X..X..Xo", { 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0 }, 0.85f },
            // 3. SmashUpTheStudio bassloop10: Deep Root & Sub-5th (-5st) groove
            { "X..X..X.X..X..o.", { 0, 0, 0, 0, 0, 0, -5, 0, 0, 0, 0, 0, -5, 0, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.80f },
            // 4. SmashUpTheStudio bassloop13: Rising Root -> 5th -> Octave hook
            { "X..X..o.X..X..X.", { 0, 0, 0, 0, 0, 0, 3, 0, 7, 0, 0, 0, 0, 0, 7, 0 },  { 0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0 }, 0.84f },
            // 5. Classic Chicago Off-Beat + Syncopated Pickup
            { "X.X.o.X.X.X.o.XX", { 0, 0, 0, 0, 3, 0, 5, 0, 0, 0, 0, 0, 7, 0, 3, 0 },  { 0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0 }, 0.78f }
        };

        static const PhraseTemplate acidTemplates[] = {
            // 1. Classic TB-303 Phrygian Pedal + Octave Jumps & Slides
            { "XXXXoXXXXXXoXXXX", { 0, 0, 1, 0, 0, 3, 0, 7, 0, 0, 1, 0, 7, 3, 1, 0 }, { 0,1,0,-1, 0,0,1,0, 0,1,0,-1, 1,0,0,0 }, 0.65f },
            // 2. ElectroniSounds Acid Trance Hypnotic 303
            { "X.XXoXXX.XXoXXXX", { 0, 0, 0, 1, 0, 0, 7, 0, 0, 0, 3, 0, 10, 7, 3, 0 }, { 0,0,1,0, -1,0,1,0, 0,0,1,0, 1,0,0,-1 }, 0.68f },
            // 3. Hard Warehouse 303 Call & Response
            { "XXXXXX.XXXXXXX.X", { 0, 0, 0, 1, 3, 0, 0, 0, 0, 0, 7, 8, 7, 3, 0, 1 }, { 0,1,0,0, 1,-1,0,0, 0,1,0,0, 1,0,-1,0 }, 0.62f },
            // 4. Rolling 303 Sub-Octave Counterpoint
            { "XXXXXXXXXXXXoXXX", { 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 3, 0, 7, 5, 3, 1 }, { 0,-1,0,1, -1,0,-1,1, 0,-1,0,1, 0,1,0,-1 }, 0.65f }
        };

        static const PhraseTemplate italoSynthwaveTemplates[] = {
            // 1. Classic Moroder / Juno Octave-Pumping Bass (I - VI - IV - V feel)
            { "XXXXXXXXXXXXXXXX", { 0, 0, 0, 0, -4, -4, -4, -4, -7, -7, -7, -7, -5, -5, -2, -2 }, { 0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1 }, 0.62f },
            // 2. DMS Classic Arp (Root - Octave - 5th Power Hook)
            { "XXXXXXXXXXXXXXXX", { 0, 0, 7, 0, 7, 7, 0, 0, 7, 0, 0, 7, 0, 0, 7, 3 }, { 0,1,1,1, 1,1,0,1, 1,0,1,1, 0,1,1,0 }, 0.68f },
            // 3. 80s Synthwave Melodic Hook (A - A - A - B)
            { "X.XXX.XXX.XXX.XX", { 0, 0, 3, 7, 0, 0, 3, 7, 0, 0, 3, 7, 10, 7, 3, 2 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.75f },
            // 4. Italo Gallop with Octave Jumps
            { "X.XXo.XXX.XXo.XX", { 0, 0, 0, 0, 3, 0, 7, 5, 0, 0, 0, 0, 7, 5, 3, 2 }, { 0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0 }, 0.72f }
        };

        static const PhraseTemplate berlinTemplates[] = {
            // 1. Tangerine Dream Counterpoint Ostinato (Pedal 0 alternating with modal arch)
            { "XXXXXXXXXXXXXXXX", { 0, 7, 0, 5, 0, 3, 0, 5, 0, 7, 0, 10, 0, 7, 5, 3 }, { 0,0,0,0, 0,0,0,0, 0,1,0,0, 0,0,0,0 }, 0.70f },
            // 2. Moog 6-Note Polymetric Loop over 16 steps (6 + 6 + 4)
            { "XXXXXXXXXXXXXXXX", { 0, 0, 5, 7, 8, 0, 0, 0, 5, 7, 8, 0, 2, 0, 5, 7 }, { 0,1,1,1, 1,2,0,1, 1,1,1,2, 0,1,1,1 }, 0.72f },
            // 3. Krautrock Hypnotic 8th/16th Pendulum
            { "X.XXX.XXX.XXX.XX", { 0, 0, 7, 3, 0, 0, 8, 7, 0, 0, 7, 3, 0, 5, 3, 2 }, { 0,0,0,0, -1,0,0,0, 0,0,0,0, -1,0,0,0 }, 0.74f }
        };

        static const PhraseTemplate melodicTechnoTemplates[] = {
            // 1. PML-MTM-Arp_012 / 019 Signature Groove: XX.XXX.XXX.XXX.X with Counterpoint Pedal
            { "XX.XXX.XXX.XXX.X", { 0, -1, 0, -5, 3, 0, 0, 0, -1, 0, 7, 3, 0, -5, -1, 0 }, { 0,0,0,0, 0,-1,0,0, 0,0,0,0, -1,0,0,0 }, 0.82f },
            // 2. PML-MTM-Arp_017 Beach: Off-Beat Sidechain Groove (.XXX.XXX.XXX.XXX)
            { ".XXX.XXX.XXX.XXX", { 0, 0, -1, -3, 0, 0, -1, -3, 0, 0, -1, -3, 0, 3, 2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.78f },
            // 3. PML-MTM-Arp_016 Euphoria: 6-Note Motif Repeated Twice + Turnaround
            { "XXXXXXXXXXXXXXXX", { 0, 3, 2, -4, 5, -4, 0, 3, 2, -4, 5, -4, 0, 3, 7, 5 }, { 0,0,0,-1, 0,-1,0,0, 0,-1,0,-1, 0,0,0,0 }, 0.75f },
            // 4. Afterlife Bodzin Lead Hook (Tresillo + Long Legato Ties)
            { "X..X..X.X..X..XX", { 0, 0, 0, 1, 0, 0, 5, 0, 3, 0, 0, 1, 0, 0, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.90f }
        };

        static const PhraseTemplate electroBassHouseTemplates[] = {
            // 1. DABRO Ras Bass: XXX.....XXX..... (Punchy 3-note walkdown + long sustain)
            { "XXX...o.XXX...oo", { 0, -2, -3, 0, 0, 0, 0, 0, 0, -2, -3, 0, 0, 0, 3, 2 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.92f },
            // 2. DABRO NewWave Bass: X.X.X.X.X...X... (Chromatic/Minor descent)
            { "X.X.X.X.X.o.X.o.", { 0, 0, 0, 0, -1, 0, -1, 0, -3, 0, 0, 0, -5, 0, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.88f },
            // 3. Detroit Drexciya Syncopated Electro
            { "X..X.XX...XX.XX.", { 0, 0, 0, 7, 0, 3, 0, 0, 0, 0, 7, 5, 0, 3, 0, -2 }, { 0,0,0,0, 0,0,1,0, 0,0,0,0, 0,1,0,0 }, 0.78f },
            // 4. DABRO Camel Bass: Syncopated Octave/5th Bounce
            { "X...X.X.X.X.X.X.", { 0, 0, 0, 0, 5, 0, 0, 0, 2, 0, 3, 0, 0, 0, 3, 0 }, { 0,0,0,0, 1,0,1,0, 1,0,1,0, 1,0,1,0 }, 0.85f }
        };

        static const PhraseTemplate funkBoogieTemplates[] = {
            // 1. Nano Funky House "Dipper": Long Beat-1 Root + Pentatonic Walkdown (12->10->7->3->0->-2)
            { "X..oXXXXX....o.X", { 0, 0, 0, 0, 0, 10, 7, 3, 0, 0, 0, 0, 0, 3, 0, -2 }, { 0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.88f },
            // 2. Nano Funky House "Spurs": Call & Response Pentatonic Groove
            { "X...XX..o...XXX.", { 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 5, 7, 10, 0 }, { 0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.86f },
            // 3. Nano Funky House "Stealthy": Octave-Drop Slap Funk
            { "X.X.X.X.X.X.X.XX", { 0, 0, 0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 3, 0, 3, 5 }, { 0,0,-1,0, 0,0,-1,0, 0,0,-1,0, 0,0,-1,0 }, 0.80f },
            // 4. Nano Funky House "Zinc": Offbeat Octave Pop & 5th/6th Turnaround
            { "X...o.XXX....oXX", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 9 }, { 0,0,0,0, 0,0,1,1, 1,0,0,0, 0,0,0,0 }, 0.85f }
        };

        static const PhraseTemplate euclideanTemplates[] = {
            // 1. PML Come Arp: ..X.XXXX..X.XXXX Symmetric 8-Step Motif
            { "o.X.XXXXo.X.XXXX", { 0, 0, 0, 0, 5, 7, -2, 0, 0, 0, 0, 0, 5, 7, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.82f },
            // 2. Organic House Subber: ..X...X...o.X.X.
            { "o.X...X.o.X.X.X.", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 7, 0, 5, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.85f },
            // 3. PML Close Arp: X.XXX.XXX.XXX.X. (2-Note Hypnotic Pendulum)
            { "X.XXX.XXX.XXX.X.", { 3, 0, 3, 3, 2, 0, 3, 3, 2, 0, 3, 3, 2, 0, 0, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.76f },
            // 4. PML_120_Amin_Arp_008: Continuous Stepwise Modal Weave
            { "XXXXXXXXXXXXXXXX", { 0, 3, 2, 0, -2, 2, 0, -2, -4, -2, 0, 2, -5, -4, -2, 0 }, { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }, 0.78f }
        };

        // Select template pool for Genre
        const PhraseTemplate* pool = houseTemplates;
        int poolCount = 5;
        switch (p.genre)
        {
            case GenreStyle::ClassicHouse:   pool = houseTemplates;          poolCount = 5; break;
            case GenreStyle::AcidTechno:     pool = acidTemplates;           poolCount = 4; break;
            case GenreStyle::ItaloSynthwave: pool = italoSynthwaveTemplates; poolCount = 4; break;
            case GenreStyle::BerlinSchool:   pool = berlinTemplates;         poolCount = 3; break;
            case GenreStyle::MelodicTechno:  pool = melodicTechnoTemplates;  poolCount = 4; break;
            case GenreStyle::DetroitElectro: pool = electroBassHouseTemplates; poolCount = 4; break;
            case GenreStyle::FunkBoogie:     pool = funkBoogieTemplates;     poolCount = 4; break;
            case GenreStyle::EuclideanIDM:   pool = euclideanTemplates;      poolCount = 4; break;
            default: break;
        }

        const PhraseTemplate& tmpl = pool[randInt (0, poolCount - 1)];

        // Optional musical transposition of the non-root motif tones (keeps Root=0 anchored!)
        const int motifShiftChoices[] = { 0, 0, 2, -2, 5 };
        const int motifShift = motifShiftChoices[randInt (0, 4)];
        const float rangeScale = juce::jlimit (0.25f, 1.50f, p.pitchRange * 1.45f);

        for (int i = 0; i < kMaxSteps; ++i)
        {
            auto& s = bank.steps[static_cast<size_t>(i)];
            if (i >= bank.numSteps)
            {
                s.active = false;
                continue;
            }

            const int mod16 = i % 16;
            const char maskChar = tmpl.rhythmMask[mod16];

            // 1. Rhythmic Activation (preserves the core groove, scales density smoothly!)
            if (maskChar == 'X')
            {
                // Core groove step: always active unless density is very low
                s.active = (i == 0) || (p.density > 0.22f) || (randFloat() < (0.4f + p.density));
            }
            else if (maskChar == 'o')
            {
                // Secondary groove step: activates as Density / Syncopation increases
                s.active = (randFloat() < (p.density * 0.75f + p.syncopation * 0.35f));
            }
            else
            {
                // Rest step: only activates at very high density (>78%) and mirrors step (i % 8) for symmetry!
                s.active = (p.density > 0.78f) && (randFloat() < (p.density - 0.72f) * 1.8f);
            }

            if (p.role == InstrumentRole::PolyChordStab && (i % 2 == 1) && maskChar != 'X')
                s.active = (randFloat() < 0.25f * p.syncopation);

            // 2. Musical Pitch (Quantized to Scale, preserving Call-and-Response symmetry!)
            int rawSemi = tmpl.semitones[static_cast<size_t>(mod16)];
            if (rawSemi != 0)
            {
                rawSemi = static_cast<int> (std::round ((rawSemi + motifShift) * rangeScale));
            }
            s.semitoneOffset = juce::jlimit (-12, 24, quantizeSemitoneToScale (rawSemi, p.scale));

            // 3. Octave Shift (from template + controlled Spice variation)
            s.octaveShift = tmpl.octaves[static_cast<size_t>(mod16)];
            if (p.pitchRange < 0.25f)
                s.octaveShift = 0;
            else if (p.spice > 0.65f && (i == 7 || i == 15) && randFloat() < 0.35f)
                s.octaveShift = 1;

            // 4. Velocity & Accent (Musical phrasing: strong on beat 1, tresillo accents & turnarounds)
            const bool isTresilloHit = (i == 0 || i == 3 || i == 6 || i == 8 || i == 11 || i == 14);
            const bool isDownbeat = (i % 4 == 0);
            s.accent = s.active && ((i == 0) || (isTresilloHit && randFloat() < (0.35f + 0.45f * p.spice)));

            if (s.accent)
            {
                s.velocity = randInt (115, 126);
                s.ccValue  = randInt (102, 125);
            }
            else if (isDownbeat)
            {
                s.velocity = randInt (96, 108);
                s.ccValue  = randInt (72, 92);
            }
            else
            {
                s.velocity = randInt (76, 94);
                s.ccValue  = randInt (48, 76);
            }

            // 5. Slide / Legato Glide (Placed musically on transitions into or out of non-root notes!)
            s.slide = false;
            if (s.active)
            {
                float slideChance = 0.0f;
                if (p.genre == GenreStyle::AcidTechno || p.role == InstrumentRole::AcidLine)
                    slideChance = (s.semitoneOffset != 0 || s.octaveShift != 0) ? (0.35f + 0.40f * p.spice) : (0.15f * p.spice);
                else if (p.genre == GenreStyle::MelodicTechno || p.role == InstrumentRole::LeadHook)
                    slideChance = (i == 3 || i == 6 || i == 11 || i == 14) ? (0.28f + 0.35f * p.spice) : 0.0f;
                else if (p.genre == GenreStyle::FunkBoogie || p.genre == GenreStyle::DetroitElectro)
                    slideChance = (i >= 12) ? (0.30f * p.spice) : (0.08f * p.spice);

                s.slide = (randFloat() < slideChance);
            }

            // 6. Ratchets (Only on turnaround steps 7, 14, 15 or in Berlin/Modular mode so they never clutter the groove!)
            s.ratchet = 1;
            if (s.active && !s.slide)
            {
                if ((p.genre == GenreStyle::BerlinSchool || p.role == InstrumentRole::ModularRatchet)
                    && (i == 6 || i == 7 || i == 14 || i == 15) && randFloat() < (0.25f + 0.45f * p.spice))
                {
                    s.ratchet = (randFloat() < 0.7f) ? 2 : 3;
                }
                else if (p.spice > 0.72f && (i == 15) && randFloat() < 0.30f)
                {
                    s.ratchet = 2;
                }
            }

            s.probability = 100;
            s.chordType = (p.role == InstrumentRole::PolyChordStab) ? ((p.genre == GenreStyle::ClassicHouse || p.genre == GenreStyle::FunkBoogie) ? 4 : 3) : 0;
        }

        // Enforce Phrase Symmetry: copy rhythmic/pitch tweaks from Bar-Half 1 (0..5) to Bar-Half 2 (8..13) when appropriate
        if (p.genre != GenreStyle::FunkBoogie && randFloat() < 0.65f)
        {
            for (int i = 0; i < 6; ++i)
            {
                if (i + 8 < bank.numSteps)
                    bank.steps[static_cast<size_t>(i + 8)].active = bank.steps[static_cast<size_t>(i)].active;
            }
        }

        // 32-Step Phrase Evolution (A - B form): Bar 2 (16..31) variations and cadence turnaround
        if (bank.numSteps > 16)
        {
            // 1. Subtle rhythmic or pitch evolution in Bar 2 body (steps 18..25)
            if (randFloat() < 0.65f)
            {
                for (int sIdx : { 19, 21, 23, 25 })
                {
                    if (sIdx < bank.numSteps && bank.steps[static_cast<size_t>(sIdx)].active)
                    {
                        if (randFloat() < 0.50f)
                            bank.steps[static_cast<size_t>(sIdx)].octaveShift = 1;
                    }
                }
            }

            // 2. Bar 2 Turnaround & Cadence (steps 26..31): Fills, octave hooks, and pickup notes
            const int turnaroundChoices[] = { 5, 7, -2, -5, 10, 3 };
            const int tPick = turnaroundChoices[randInt (0, 5)];
            const int tSemi = quantizeSemitoneToScale (tPick, p.scale);

            // Pickup on step 26 or 28
            const int pickupStep = (randFloat() < 0.5f) ? 26 : 28;
            if (pickupStep < bank.numSteps)
            {
                auto& st = bank.steps[static_cast<size_t>(pickupStep)];
                st.active = true;
                st.semitoneOffset = tSemi;
                st.accent = true;
                st.velocity = randInt (118, 127);
                st.ccValue = 120;
            }

            // Final drum/synth roll on step 30 / 31
            if (30 < bank.numSteps && randFloat() < (0.35f + 0.40f * p.spice))
            {
                auto& st30 = bank.steps[30];
                st30.active = true;
                st30.ratchet = 2;
                st30.velocity = 110;
            }
            if (31 < bank.numSteps && randFloat() < (0.25f + 0.50f * p.spice))
            {
                auto& st31 = bank.steps[31];
                st31.active = true;
                st31.ratchet = (p.spice > 0.6f) ? 2 : 1;
                st31.slide = true;
                st31.velocity = 122;
                st31.semitoneOffset = quantizeSemitoneToScale (-2, p.scale); // leading tone back to Root 0
            }
        }

        // Calculate intelligent musical gate lengths based on inter-onset gaps!
        const bool isArpOrPluck = (p.role == InstrumentRole::ClassicArp || p.mode == EngineMode::Arpeggiator);
        applyMusicalGates (bank, tmpl.sustainRatio, isArpOrPluck, p.spice);
    }

    //==========================================================================
    // Direct MIDI File Loader & Analyzer (from G:\samples\MIDI or any .mid file)
    //==========================================================================
    static bool loadPatternFromMidiFile (const juce::File& midiFile, PatternBank& bank, ScaleType targetScale)
    {
        juce::FileInputStream stream (midiFile);
        if (!stream.openedOk())
            return false;

        juce::MidiFile mf;
        if (!mf.readFrom (stream))
            return false;

        const short tpqn = mf.getTimeFormat();
        const double ticksPerQuarter = (tpqn > 0) ? static_cast<double> (tpqn) : 480.0;
        const double stepTicks = ticksPerQuarter * 0.25; // 1/16th note

        struct ParsedNote
        {
            int step = 0;
            int pitch = 60;
            int velocity = 100;
            float durationSteps = 1.0f;
        };

        std::vector<ParsedNote> notes;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            const auto* track = mf.getTrack (t);
            if (track == nullptr) continue;

            juce::MidiMessageSequence seq (*track);
            seq.updateMatchedPairs();

            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                auto* ev = seq.getEventPointer (i);
                if (ev->message.isNoteOn() && ev->message.getVelocity() > 0)
                {
                    const double startTick = ev->message.getTimeStamp();
                    double endTick = startTick + stepTicks;
                    if (ev->noteOffObject != nullptr)
                        endTick = ev->noteOffObject->message.getTimeStamp();

                    const int stepIdx = static_cast<int> (std::round (startTick / stepTicks));
                    if (stepIdx >= 0 && stepIdx < kMaxSteps)
                    {
                        float durSteps = static_cast<float> (juce::jmax (0.2, (endTick - startTick) / stepTicks));
                        notes.push_back ({ stepIdx, ev->message.getNoteNumber(), static_cast<int> (ev->message.getVelocity()), durSteps });
                    }
                }
            }
        }

        if (notes.empty())
            return false;

        // Sort by step, then lowest pitch first (so bass/root notes take priority)
        std::sort (notes.begin(), notes.end(), [] (const ParsedNote& a, const ParsedNote& b)
        {
            if (a.step != b.step) return a.step < b.step;
            return a.pitch < b.pitch;
        });

        // Determine anchor root pitch from the first note (or most frequent bass pitch)
        const int baseNote = notes.front().pitch;

        for (auto& s : bank.steps)
        {
            s.active = false;
            s.semitoneOffset = 0;
            s.octaveShift = 0;
            s.gate = 0.75f;
            s.accent = false;
            s.slide = false;
            s.ratchet = 1;
            s.probability = 100;
        }

        int notesOnStep[kMaxSteps] = { 0 };
        for (const auto& n : notes)
        {
            auto& s = bank.steps[static_cast<size_t>(n.step)];
            notesOnStep[n.step]++;

            if (!s.active) // Take lowest/first note on this step as base pitch
            {
                s.active = true;
                int diff = n.pitch - baseNote;
                int oct = 0;
                while (diff >= 12)  { diff -= 12; ++oct; }
                while (diff <= -12) { diff += 12; --oct; }

                s.semitoneOffset = quantizeSemitoneToScale (diff, targetScale);
                s.octaveShift = juce::jlimit (-2, 2, oct);
                s.velocity = juce::jlimit (40, 127, n.velocity);
                s.gate = juce::jlimit (0.25f, 3.80f, n.durationSteps);
                s.accent = (n.velocity >= 110);
                s.ccValue = s.accent ? 110 : 72;
            }
            else if (notesOnStep[n.step] >= 3)
            {
                s.chordType = 3; // Multi-note chord detected in MIDI file!
            }
        }

        // Detect legato slides (when a step's gate reaches or exceeds the distance to the next active step)
        for (int i = 0; i < kMaxSteps; ++i)
        {
            auto& s = bank.steps[static_cast<size_t>(i)];
            if (!s.active) continue;

            int gap = 1;
            for (int d = 1; d < kMaxSteps; ++d)
            {
                if (bank.steps[static_cast<size_t>((i + d) % kMaxSteps)].active)
                {
                    gap = d;
                    break;
                }
            }
            if (s.gate >= static_cast<float> (gap) + 0.02f && gap <= 2)
                s.slide = true;
        }

        bank.name = midiFile.getFileNameWithoutExtension();
        return true;
    }

    // Picks a random authentic .mid file from Daniele's G:\samples\MIDI matching the current Genre!
    static juce::String loadRandomFromUserSamplesLibrary (PatternBank& bank, GenreStyle genre, ScaleType scale)
    {
        juce::StringArray candidateFolders;

        switch (genre)
        {
            case GenreStyle::ClassicHouse:
                candidateFolders.add ("G:/samples/MIDI/SmashUpTheStudio Midikeys Deep & Jazzy House Grooves MIDI");
                candidateFolders.add ("G:/samples/CONNECTD Audio - Classic Garage And U S House");
                candidateFolders.add ("G:/samples/MIDI/PML - Midi Complete Pack/PML MIDI Pack V2");
                break;
            case GenreStyle::FunkBoogie:
                candidateFolders.add ("G:/samples/MIDI/Nano Musik Loops - Funky House Bassline");
                candidateFolders.add ("G:/samples/Sample Tools By Cr2 Soul Tech");
                break;
            case GenreStyle::MelodicTechno:
                candidateFolders.add ("G:/samples/MIDI/PML - Midi Complete Pack/PML - Melodic Techno Melody Arps - MIDI Arps (Version 1.0)");
                candidateFolders.add ("G:/samples/MIDI/PML - Midi Complete Pack/PML MIDI Pack V2");
                break;
            case GenreStyle::DetroitElectro:
                candidateFolders.add ("G:/samples/DABRO Music - Bass House WAV MiDi");
                candidateFolders.add ("G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Basslines");
                break;
            case GenreStyle::AcidTechno:
                candidateFolders.add ("G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Synth Leads & Riffs");
                candidateFolders.add ("G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Basslines");
                break;
            case GenreStyle::ItaloSynthwave:
            case GenreStyle::BerlinSchool:
                candidateFolders.add ("G:/samples/MIDI/DMS - Trance Bundle MIDI/DMS Classic Trance MIDI Vol 1");
                candidateFolders.add ("G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Arps");
                break;
            case GenreStyle::EuclideanIDM:
            default:
                candidateFolders.add ("G:/samples/Production Music Live - Organic House Essentials");
                candidateFolders.add ("G:/samples/LotusTunes - Organic House");
                candidateFolders.add ("G:/samples/MIDI/PML - Midi Complete Pack/PML - Future Bass MIDIs - Vol 1");
                break;
        }

        candidateFolders.add ("G:/samples/MIDI");

        juce::Array<juce::File> midiFiles;
        for (const auto& folderPath : candidateFolders)
        {
            juce::File dir (folderPath);
            if (dir.isDirectory())
            {
                dir.findChildFiles (midiFiles, juce::File::findFiles, true, "*.mid");
                if (midiFiles.size() >= 10)
                    break;
            }
        }

        if (midiFiles.isEmpty())
            return {};

        std::mt19937 rng (std::random_device{}());
        for (int attempt = 0; attempt < 12; ++attempt)
        {
            int idx = std::uniform_int_distribution<int> (0, midiFiles.size() - 1) (rng);
            const auto& f = midiFiles.getReference (idx);
            if (loadPatternFromMidiFile (f, bank, scale))
                return f.getFileNameWithoutExtension();
        }
        return {};
    }

    static void mutatePattern (PatternBank& bank, ScaleType scale, float spice)
    {
        std::mt19937 rng (std::random_device{}());
        auto randFloat = [&]() { return std::uniform_real_distribution<float> (0.0f, 1.0f) (rng); };
        auto randInt   = [&](int lo, int hi) { return std::uniform_int_distribution<int> (lo, hi) (rng); };

        const int n = juce::jlimit (4, kMaxSteps, bank.numSteps);

        // Turnaround targets in bar 1 and bar 2
        static const int mutableSteps16[] = { 3, 6, 7, 11, 13, 14, 15 };
        static const int mutableSteps32[] = { 3, 6, 7, 11, 13, 14, 15, 19, 22, 23, 27, 28, 29, 30, 31 };

        const int* targets = (n > 16) ? mutableSteps32 : mutableSteps16;
        const int numTargets = (n > 16) ? 15 : 7;

        for (int i = 0; i < numTargets; ++i)
        {
            const int stepIdx = targets[i];
            if (stepIdx >= n) continue;

            auto& s = bank.steps[static_cast<size_t>(stepIdx)];
            if (randFloat() < 0.38f)
            {
                const int musicalIntervals[] = { 0, 0, 3, 5, 7, -2, -5, 10 };
                int pick = musicalIntervals[randInt (0, 7)];
                s.semitoneOffset = quantizeSemitoneToScale (pick, scale);
            }
            if (randFloat() < 0.25f * (0.4f + spice))
                s.octaveShift = (randFloat() < 0.65f) ? 1 : 0;
            if (randFloat() < 0.20f)
                s.slide = !s.slide && s.active;
        }
        applyMusicalGates (bank, 0.80f, false, spice);
    }

    static void rotatePattern (PatternBank& bank, int delta)
    {
        const int n = juce::jlimit (1, kMaxSteps, bank.numSteps);
        std::array<StepData, kMaxSteps> copy = bank.steps;
        for (int i = 0; i < n; ++i)
        {
            int src = ((i - delta) % n + n) % n;
            bank.steps[static_cast<size_t>(i)] = copy[static_cast<size_t>(src)];
        }
    }

    static void requantizeToScale (PatternBank& bank, ScaleType scale)
    {
        for (auto& s : bank.steps)
            s.semitoneOffset = quantizeSemitoneToScale (s.semitoneOffset, scale);
    }

    static void invertPattern (PatternBank& bank, ScaleType scale)
    {
        for (auto& s : bank.steps)
        {
            if (s.semitoneOffset != 0)
                s.semitoneOffset = quantizeSemitoneToScale (-s.semitoneOffset, scale);
            s.octaveShift = -s.octaveShift;
        }
    }
};

} // namespace PulseForge
