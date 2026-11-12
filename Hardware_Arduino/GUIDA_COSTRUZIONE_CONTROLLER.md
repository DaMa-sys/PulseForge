# Oragen PulseForge — Guida Completa alla Costruzione del Controller Hardware

Guida passo-passo per assemblare, cablare e collaudare la **Desk Console Hardware dedicata a 3 Tracce** per **PulseForge** (VST3 / Standalone).

---

## 1. Panoramica del Progetto

Il controller hardware di PulseForge è progettato con l'ergonomia di una **console hardware da studio d'élite** (ispirata all'OXI One, Elektron e ai channel strip analogici).  
Offre il controllo tattile istantaneo a zero latenza sulle 3 tracce del sequencer:
- **Canale 1**: Basso Monofonico (Traccia 1)
- **Canale 2**: Arpeggiatore / Lead Melodico (Traccia 2)
- **Canale 3**: Armonia & Accordi Polifonici (Traccia 3)

### Protocollo di Comunicazione
- **Microcontrollore**: Arduino Mega 2560 (scelto per l'elevatissimo numero di pin I/O nativi, 16 ADC analogici e 54 I/O digitali senza bisogno di multiplexer esterni).
- **Interfaccia**: USB Seriale ad alta velocità (**115200 Baud**).
- **Auto-Discovery Win32**: All'avvio del plugin o della Standalone, PulseForge scansiona automaticamente le porte COM di Windows inviando `PF_PING?`. Arduino risponde con `PF_PONG:PulseForge_Controller` stabilendo istantaneamente il link bidirezionale.
- **Zero Glitch / Thread-Safe**: Lo stream seriale corre su un thread Win32 asincrono dedicato a bassa priorità nel plugin, con una coda circolare lock-free per i feedback LED (`STEP:Tx:ACC`).

---

## 2. Lista Componenti (BOM - Bill of Materials)

| Componente | Quantità | Note Tecniche |
| :--- | :---: | :--- |
| **Arduino Mega 2560 R3** | 1 | Originale o clone compatibile con chip USB-Seriale (ATmega16U2 o CH340). |
| **Cavo USB Type-B** | 1 | Per alimentazione e trasferimento dati seriali dal PC. |
| **Potenziometri Lineari 10kΩ (B10K)** | 15 | 5 per canale (Density, Swing, Base Oct, Arp Oct, Ratio). Con alberino zigrinato o a D. |
| **Manopole per Potenziometri (Knobs)** | 15 | Diametro 15–20mm, con indicatore di posizione bianco o luminescente. |
| **Pulsanti Tattili Momentanei** | 6 | 2 per canale: `GENERATE` e `MUTATE` (tipo SPST normal-open, arcade o soft-touch). |
| **Rotary Encoders con Push-Button (EC11)** | 9 | 3 per canale: `GENRE`, `ROLE`, `STEPS`. Con clic rotativo e switch integrato. |
| **LED 5mm (o 3mm)** | 3 | Indicatori Step/Beat/Accent per ciascuna traccia (es. Ciano, Ambra, Blu/Verde). |
| **Resistenze 220Ω - 330Ω** | 3 | Resistenze di limitazione corrente per i 3 LED. |
| **Display OLED 0.96" I2C SSD1306** *(Opzionale)* | 3 | Risoluzione 128x64 px. Uno per ogni traccia (mostra ruolo, note e step correnti). |
| **Basetta Millefori o Stripboard** | 1-2 | Per distribuire comodamente linee VCC (5V) e Ground (GND). |
| **Cavi Dupont e Cavi Elettrici Flessibili** | ~60 | Maschio-Femmina e Femmina-Femmina per collegamenti rapidi. |
| **Box / Case Console Desktop** | 1 | In legno, alluminio o plastica stampata in 3D con pannello frontale forato. |

---

## 3. Mappatura Completa dei Pin (Arduino Mega 2560)

Tutti i pin sono già configurati nel firmware ufficiale in:  
`Hardware_Arduino/controller_sequencer_PulseForge/controller_sequencer_PulseForge.ino`.

### Canale 1: Basso (Track 0)
| Funzione | Tipo Hardware | Pin Arduino Mega | Protocollo Seriale Inviato |
| :--- | :--- | :--- | :--- |
| **Density** | Potenziometro B10K | **A0** | `T0:DENS:0..100` |
| **Swing** | Potenziometro B10K | **A1** | `T0:SWING:0..75` |
| **Base Octave** | Potenziometro B10K | **A2** | `T0:BOCT:0..5` |
| **Arp Octave** | Potenziometro B10K | **A3** | `T0:AOCT:1..4` |
| **Tempo Ratio** | Potenziometro B10K | **A4** | `T0:RATIO:0..10` |
| **Generate** | Pulsante (GND) | **Pin 2** (Pullup) | `T0:GEN` |
| **Mutate** | Pulsante (GND) | **Pin 3** (Pullup) | `T0:MUT` |
| **Step Beat LED** | LED (+ Res. 220Ω) | **Pin 8** | Feedback da `STEP:T0:x` |
| **Genre Encoder** | CLK / DT / SW | **22 / 23 / 24** | `T0:ENC:GENRE:+1/-1`, `T0:CLICK:GENRE` |
| **Role Encoder** | CLK / DT / SW | **25 / 26 / 27** | `T0:ENC:ROLE:+1/-1`, `T0:CLICK:ROLE` |
| **Steps Encoder** | CLK / DT / SW | **28 / 29 / 30** | `T0:ENC:STEPS:+1/-1`, `T0:CLICK:STEPS` |
| *OLED 1 (Opz.)* | SCL / SDA (Software I2C)| **12 / 11** | Riceve `OLED:T0:L1..3:testo` |

---

### Canale 2: Arpeggio & Lead (Track 1)
| Funzione | Tipo Hardware | Pin Arduino Mega | Protocollo Seriale Inviato |
| :--- | :--- | :--- | :--- |
| **Density** | Potenziometro B10K | **A5** | `T1:DENS:0..100` |
| **Swing** | Potenziometro B10K | **A6** | `T1:SWING:0..75` |
| **Base Octave** | Potenziometro B10K | **A7** | `T1:BOCT:0..5` |
| **Arp Octave** | Potenziometro B10K | **A8** | `T1:AOCT:1..4` |
| **Tempo Ratio** | Potenziometro B10K | **A9** | `T1:RATIO:0..10` |
| **Generate** | Pulsante (GND) | **Pin 4** (Pullup) | `T1:GEN` |
| **Mutate** | Pulsante (GND) | **Pin 5** (Pullup) | `T1:MUT` |
| **Step Beat LED** | LED (+ Res. 220Ω) | **Pin 9** | Feedback da `STEP:T1:x` |
| **Genre Encoder** | CLK / DT / SW | **31 / 32 / 33** | `T1:ENC:GENRE:+1/-1`, `T1:CLICK:GENRE` |
| **Role Encoder** | CLK / DT / SW | **34 / 35 / 36** | `T1:ENC:ROLE:+1/-1`, `T1:CLICK:ROLE` |
| **Steps Encoder** | CLK / DT / SW | **37 / 38 / 39** | `T1:ENC:STEPS:+1/-1`, `T1:CLICK:STEPS` |
| *OLED 2 (Opz.)* | SCL / SDA (Software I2C)| **15 / 14** | Riceve `OLED:T1:L1..3:testo` |

---

### Canale 3: Armonia & Accordi (Track 2)
| Funzione | Tipo Hardware | Pin Arduino Mega | Protocollo Seriale Inviato |
| :--- | :--- | :--- | :--- |
| **Density** | Potenziometro B10K | **A10** | `T2:DENS:0..100` |
| **Swing** | Potenziometro B10K | **A11** | `T2:SWING:0..75` |
| **Base Octave** | Potenziometro B10K | **A12** | `T2:BOCT:0..5` |
| **Arp Octave** | Potenziometro B10K | **A13** | `T2:AOCT:1..4` |
| **Tempo Ratio** | Potenziometro B10K | **A14** | `T2:RATIO:0..10` |
| **Generate** | Pulsante (GND) | **Pin 6** (Pullup) | `T2:GEN` |
| **Mutate** | Pulsante (GND) | **Pin 7** (Pullup) | `T2:MUT` |
| **Step Beat LED** | LED (+ Res. 220Ω) | **Pin 10** | Feedback da `STEP:T2:x` |
| **Genre Encoder** | CLK / DT / SW | **40 / 41 / 42** | `T2:ENC:GENRE:+1/-1`, `T2:CLICK:GENRE` |
| **Role Encoder** | CLK / DT / SW | **43 / 44 / 45** | `T2:ENC:ROLE:+1/-1`, `T2:CLICK:ROLE` |
| **Steps Encoder** | CLK / DT / SW | **46 / 47 / 48** | `T2:ENC:STEPS:+1/-1`, `T2:CLICK:STEPS` |
| *OLED 3 (Opz.)* | SCL / SDA (Software I2C)| **17 / 16** | Riceve `OLED:T2:L1..3:testo` |

---

## 4. Guida al Cablaggio Elettrico

```
       +5V (Arduino) ----------------+-----------------+
                                     |                 |
                                   [Pot 1]           [Pot 2] ... (Piedino 1)
                                     |                 |
     Pin Analogico (es. A0) <--- (Cursore 2)     (Cursore 2)
                                     |                 |
                                   [Pot 1]           [Pot 2] ... (Piedino 3)
                                     |                 |
       GND (Arduino) ----------------+-----------------+
```

1. **Alimentazione Comune (Bus +5V e GND)**:
   - Salda o distribuisci su una basetta un binario continuo per i **5V** e uno per la **Massa (GND)**.
2. **Potenziometri**:
   - Piedino 1: collegato al binario **+5V**.
   - Piedino 3: collegato al binario **GND**.
   - Piedino centrale (Cursore): collegato al relativo pin **A0..A14** dell'Arduino.
3. **Pulsanti**:
   - Usano i resistori di pull-up interni del microcontrollore.
   - Un terminale del pulsante va al pin digitale (es. Pin 2).
   - L'altro terminale va direttamente a **GND**. Quando premi il tasto, il segnale va a 0V (LOW).
4. **Rotary Encoders**:
   - Pin `+` / VCC: non serve se usi i pullup interni dell'Arduino Mega (oppure collegalo a +5V se il modulo breakout ha resistori a bordo).
   - Pin `GND`: collegato a **GND**.
   - Pin `CLK`, `DT`, `SW`: collegati ai pin digitali assegnati (i pullup interni sono attivati dal firmware).
5. **LED di Stato**:
   - Anodo (gambo lungo): collegato al Pin (8, 9 o 10) tramite una resistenza da 220Ω in serie.
   - Catodo (gambo corto): collegato a **GND**.

---

## 5. Montaggio Incrementale (Costruzione Anti-Bug Senza Stress)

Il firmware di PulseForge dispone di **Hardware Connection Masks** all'inizio del codice:

```cpp
const bool CONNECTED_POTS[3][5] = {
    { false, false, false, false, false }, // Traccia 1
    { false, false, false, false, false }, // Traccia 2
    { false, false, false, false, false }  // Traccia 3
};
const bool CONNECTED_BUTTONS_GEN[3] = { false, false, false };
const bool CONNECTED_BUTTONS_MUT[3] = { false, false, false };
const bool CONNECTED_ENCODERS[3][3] = { ... };
const bool CONNECTED_LEDS[3] = { false, false, false };
```

> [!TIP]
> **Come procedere a step modulari:**
> 1. Inizia collegando **solo i 3 LED** e i **2 pulsanti della Traccia 1** (Pin 2 e Pin 3).
> 2. Nel file `.ino`, imposta `CONNECTED_LEDS[0] = true;` e `CONNECTED_BUTTONS_GEN[0] = true;`. Lascia tutti gli altri a `false`.
> 3. I pin impostati a `false` vengono bloccati in `INPUT_PULLUP` via software, quindi **non fluttuano**, **non captano ronzio a 50Hz** e **non inviano comandi casuali**.
> 4. Man mano che saldi nuovi potenziometri o encoder, cambia il rispettivo valore da `false` a `true` e ricarica lo sketch. In questo modo individui subito qualsiasi eventuale errore di saldatura!

---

## 6. Caricamento del Firmware su Arduino

1. Installa l'**Arduino IDE** (versione 2.x o 1.8.x).
2. Se intendi utilizzare gli schermi OLED:
   - Vai su *Strumenti -> Gestione Librerie...*
   - Cerca ed installa **U8g2** di olikraus.
   - All'inizio del file `.ino` de-commenta `#define ENABLE_OLED`.
   *(Se non monti gli schermi OLED, lascia `#define ENABLE_OLED` commentato)*.
3. Collega l'Arduino Mega 2560 al PC tramite cavo USB.
4. Seleziona in Arduino IDE:
   - Scheda: **Arduino Mega or Mega 2560**
   - Processore: **ATmega2560 (Mega 2560)**
   - Porta: Seleziona la tua porta COM (es. `COM3` o `COM5`).
5. Clicca su **Carica (Upload)**.
6. All'avvio, i LED montati faranno un doppio lampeggio rapido di benvenuto.

---

## 7. Collaudo e Utilizzo con PulseForge

1. Lascia l'Arduino collegato alla porta USB del computer.
2. Apri **PulseForge Standalone** (`C:\vst plugin\Oragen\PulseForge.exe`) oppure carica **PulseForge.vst3** su una traccia MIDI in **Ableton Live**.
3. PulseForge cercherà e aggancerà la porta COM automaticamente in meno di 1 secondo:
   - I LED dell'Arduino lampeggeranno una volta per confermare il collegamento avvenuto.
   - Nella barra di stato o nella GUI di PulseForge comparirà l'indicazione del controller connesso.
4. Quando avvii il sequencer (Play su Ableton o barra spazio nella Standalone), i LED dell'Arduino lampeggeranno a tempo con gli step e con gli accenti!
5. Girando i potenziometri o premendo i tasti `GENERATE` / `MUTATE`, i pattern e i valori si aggiorneranno in tempo reale a schermo con zero latenza percettibile.
