#pragma once
#include "PatternData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace PulseForge
{

//==============================================================================
// GENRE CONFIGURATION PROFILE (Dizionario unico di costanti e idiomi)
//==============================================================================
struct GenreProfile
{
    const char* name;
    float defaultSwing;

    // Pesi metrici base sui 16 step (per generare cellule ritmiche autentiche)
    // 0..15 corrispondono alle semicrome della battuta
    std::array<float, 16> rhythmWeights;

    // Articolazioni (moltiplicatori scalati da COMPLEXITY)
    float accentWeight;          // Target accenti
    float slideWeight;           // Target slide (es. 0 fuori da Garage/Acid)
    float octaveJumpWeight;      // Moltiplicatore salti d'ottava
    float ghostNoteWeight;       // Moltiplicatore ghost notes

    // Palette & Melodia
    int minPaletteNotes;         // Note minime a comp=0
    int maxPaletteNotes;         // Note massime a comp=100
    float rootAnchorWeight;      // Tendenza alla tonica su tempi forti
    float stepwiseTendency;      // Tendenza a gradi congiunti vs salti (0..1)
    
    // Struttura
    float variationRate;         // Tasso di mutazione tra A, A', B (controlla repetition)
    float sustainRatio;
};

inline const GenreProfile& getGenreProfile (GenreStyle genre)
{
    static const GenreProfile profiles[] = {
        // 0. Classic House: Off-beat puro sui levare (step 2, 6, 10, 14) + pickup su 7/15, tempo forte 0 scarico/sincopato
        {
            "Classic House",
            0.24f,
            // Semicrome:  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
            // Pesi:     [Down, 16th, Off8, Pick, Down, 16th, Off8, Pick, Down, 16th, Off8, Pick, Down, 16th, Off8, Pick]
            {             0.35f, 0.10f, 0.95f, 0.40f, 0.20f, 0.10f, 0.90f, 0.65f, 0.35f, 0.10f, 0.95f, 0.40f, 0.20f, 0.15f, 0.90f, 0.70f },
            0.60f, // Accent moderato (target 15-25%)
            0.00f, // Slide zero (target 0-5%)
            0.50f, // Salti ottava moderati
            0.45f,
            3, 4,  // Palette 3-4 note
            0.70f,
            0.75f, // Gradi congiunti / arpeggi
            0.38f, // Variazione A-A'-A-B per repetition 0.45-0.60
            0.78f
        },
        // 1. Acid 303 (Invariato - Riferimento)
        {
            "Acid 303",
            0.06f,
            { 0.95f, 0.85f, 0.80f, 0.85f, 0.50f, 0.85f, 0.85f, 0.85f, 0.90f, 0.85f, 0.85f, 0.85f, 0.50f, 0.85f, 0.85f, 0.85f },
            1.00f, 1.00f, 1.00f, 0.50f,
            3, 7,
            0.85f, 0.40f, 0.65f, 0.65f
        },
        // 2. Italo / EDM: Dritto sui quarti (0, 4, 8, 12) + ottavi dritti, build progressivo, melodia congiunta
        {
            "Italo / EDM",
            0.00f,
            {             0.95f, 0.15f, 0.85f, 0.20f, 0.95f, 0.15f, 0.85f, 0.35f, 0.95f, 0.15f, 0.85f, 0.20f, 0.95f, 0.25f, 0.85f, 0.65f },
            0.65f, // Target accent 15-25%
            0.00f, // Slide 0-5%
            0.75f,
            0.20f,
            3, 5,  // Palette 3-5 note
            0.85f,
            0.88f, // Melodia strettamente per gradi congiunti!
            0.30f, // Repetition 0.55-0.70
            0.70f
        },
        // 3. Tech House (Fisher): Rolling groove 16esimi compatto con accenti marcati (step 0, 6, 12) e pause nette a fine gruppo
        {
            "Tech House",
            0.08f,
            // Rolling con stacchi ritmici netti
            {             0.95f, 0.70f, 0.90f, 0.10f, 0.40f, 0.85f, 0.95f, 0.10f, 0.95f, 0.70f, 0.90f, 0.10f, 0.40f, 0.85f, 0.95f, 0.20f },
            0.85f, // Target accent 20-30%
            0.00f, // Slide 0%
            1.10f, // Ottave perno frequenti
            0.35f,
            2, 4,  // Palette 2-4 note (stretta e ipnotica)
            0.88f,
            0.65f, // Più salti triadici / d'ottava
            0.26f, // Ripetizione ipnotica 0.60-0.75
            0.72f
        },
        // 4. Melodic Techno (Paul Kalkbrenner): Arp continuo a semicrome fluide (.XXX.XXX.XXX.XXX)
        {
            "Melodic Techno",
            0.02f,
            // Arp ad onda fluida
            {             0.30f, 0.95f, 0.95f, 0.90f, 0.30f, 0.95f, 0.95f, 0.90f, 0.30f, 0.95f, 0.95f, 0.90f, 0.30f, 0.95f, 0.95f, 0.95f },
            0.65f, // Target accent 15-25%
            0.08f, // Slide raro 0-8%
            0.40f,
            0.25f,
            3, 5,  // Palette 3-5 note
            0.70f,
            0.92f, // Flusso melodico molto fluido per gradi congiunti
            0.34f, // Repetition 0.50-0.65
            0.80f
        },
        // 5. UK Garage (Disclosure): 2-step shuffle, sincopi marcate (colpi su 0, 3, 6, 10, 13), tempi forti vuoti
        {
            "UK Garage",
            0.28f,
            {             0.85f, 0.15f, 0.40f, 0.90f, 0.20f, 0.25f, 0.88f, 0.45f, 0.30f, 0.15f, 0.92f, 0.35f, 0.25f, 0.88f, 0.20f, 0.75f },
            0.75f, // Target accent 20-30%
            0.60f, // Target slide 10-20%
            0.60f,
            0.50f,
            3, 5,  // Palette 3-5 note
            0.65f,
            0.75f,
            0.45f, // Repetition 0.40-0.55
            0.82f
        },
        // 6. Emotional / Funk (Fred again..): Motivo intimo di 3-4 note ripetuto con respiro
        {
            "Emotional / Funk",
            0.20f,
            {             0.85f, 0.20f, 0.75f, 0.30f, 0.50f, 0.15f, 0.80f, 0.40f, 0.85f, 0.20f, 0.75f, 0.30f, 0.60f, 0.25f, 0.70f, 0.55f },
            0.70f, 0.10f, 0.50f, 0.35f,
            3, 4,
            0.75f, 0.85f, 0.35f, 0.84f
        },
        // 7. Downtempo / Organic (Bonobo): Pause ampie, respiro, accordi estesi
        {
            "Downtempo / Organic",
            0.14f,
            {             0.75f, 0.10f, 0.60f, 0.20f, 0.30f, 0.15f, 0.65f, 0.35f, 0.70f, 0.10f, 0.60f, 0.20f, 0.40f, 0.20f, 0.60f, 0.45f },
            0.60f, 0.15f, 0.40f, 0.30f,
            3, 5,
            0.65f, 0.80f, 0.45f, 0.88f
        }
    };

    int idx = static_cast<int> (genre);
    if (idx < 0 || idx >= static_cast<int> (GenreStyle::Count))
        idx = 0;
    return profiles[idx];
}

} // namespace PulseForge
