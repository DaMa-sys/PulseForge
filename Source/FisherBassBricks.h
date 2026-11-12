#pragma once
#include "PatternData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace PulseForge
{

// Mattoncini modulari ritmici per Fisher Bass (4 step ciascuno)
// 'X' = core hit, 'g' = ghost percussiva, '.' = pausa cassa, 'p' = pop ottava, 'o' = hit secondario
struct FisherRhythmBrick {
    const char* pattern;
};

static const FisherRhythmBrick kFisherBricksA[8] = {
    { "X.Xg" }, // 1. Rolling classico: cassa-pausa-nota-ghost
    { "X..X" }, // 2. Offbeat puro con respiro cassa
    { "XX.g" }, // 3. Coppia di semicrome pulsante
    { "X.gX" }, // 4. Sincope anticipata prima del levare
    { "X.X." }, // 5. Staccato essenziale
    { "X..g" }, // 6. Minimale con rimbalzo ghost
    { ".XXg" }, // 7. Downbeat vuoto (pickup groove)
    { "XggX" }  // 8. Doppio rimbalzo percussivo
};

// Mattoncini per Frase di Risposta (Battute 3-4 e 7-8)
static const FisherRhythmBrick kFisherBricksB[8] = {
    { "XX.X" }, // 1. Spinta incalzante
    { "X.XX" }, // 2. Rullo verso il levare
    { ".X.X" }, // 3. Offbeat sincopato puro
    { "X..p" }, // 4. Pop d'ottava a fine battuta
    { "X.Xp" }, // 5. Rolling con pop d'ottava
    { "XXgX" }, // 6. Fitta e articolata
    { "Xg.p" }, // 7. Ghost + pop turnaround
    { "XXXX" }  // 8. Rullo di 16esimi pieno per drop/fill
};

struct FisherMelodyBrick {
    int degrees[4]; // Semitoni relativi (0=Root, 12=8va, 10=b7, 7=5th, 3=b3)
};

static const FisherMelodyBrick kFisherMelodiesA[8] = {
    { 0,  0,  0,  0 }, // Pedale tonica fisso
    { 0,  0, 10,  0 }, // Rimbalzo sottotonica b7
    { 0,  0,  0, 12 }, // Pop d'ottava finale
    { 0, 10,  0,  0 }, // Anticipo sulla settima
    { 0,  0,  7,  0 }, // Tocco di quinta
    { 0,  3,  0,  0 }, // Terza minore blues/house
    { 0,  0,  0, 10 }, // Uscita sulla settima
    { 0, 12,  0,  0 }  // Pop d'ottava anticipato
};

static const FisherMelodyBrick kFisherMelodiesB[8] = {
    { 0,  0, 10, 12 }, // Turnaround classico b7 -> 8va
    { 0, 10,  7, 10 }, // Movimento pentatonico discendente
    { 0, 12, 10,  0 }, // Salita all'ottava e discesa
    { 0,  0,  0, 12 }, // Pop d'ottava deciso
    { 0,  7, 10, 12 }, // Arpeggio ascendente
    { 0,  0,  3,  0 }, // Risoluzione b3 -> Root
    { 0, 10,  0, 12 }, // Alternanza aperta
    { 0,  0,  0,  0 }  // Pedale solido di riposo
};

} // namespace PulseForge
