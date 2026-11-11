# DaMA PulseForge — Guida Completa per lo Sviluppatore (Antigravity AI)

Benvenuto nel progetto **PulseForge** di **DaMA Audio Labs**!  
Questo pacchetto contiene tutto il codice sorgente, l'architettura, gli script e il firmware per proseguire lo sviluppo del plugin VST3 / Standalone e del controller hardware su **Antigravity**.

---

## 1. Come Aprire e Usare il Progetto con Antigravity

1. Apri **Antigravity**.
2. Seleziona **Open Workspace** (o apri la cartella principale del progetto `PulseForge`).
3. Antigravity rileverà automaticamente i file sorgente C++, CMake e il firmware Arduino.
4. Puoi interagire direttamente in linguaggio naturale per aggiungere funzionalità, raffinare algoritmi o modificare la grafica.

---

## 2. Struttura del Repository

```
PulseForge/
├── CMakeLists.txt                 # Configurazione CMake per VST3 & Standalone .exe (JUCE 7)
├── install_vst3.bat               # Script a 1-click per installare il VST3 e resettare la cache Ableton
├── LEGGIMI.txt                    # Istruzioni d'uso rapide per l'utente finale
├── README.md                      # Presentazione del progetto e specifiche
├── GUIDA_SVILUPPO_ANTIGRAVITY.md  # Questa guida tecnica per sviluppatori
├── Hardware_Arduino/              # Firmware per la scheda Arduino Mega Desk Controller
│   └── controller_sequencer_PulseForge/
│       └── controller_sequencer_PulseForge.ino
└── Source/                        # Codice sorgente C++ (JUCE Framework)
    ├── DamaTheme.h                # Design System: palette a 3 toni azzurri, font, LookAndFeel soft-UI
    ├── HardwareControllerSerial.h # Driver seriale Win32 asincrono, auto-discovery & coda lock-free
    ├── PatternData.h              # Strutture dati musicali a 32-step, accordi drop-2, scale modali
    ├── PluginProcessor.h/.cpp     # Motore DSP, MIDI multi-timbrale a 3 tracce, APVTS, Thread Safety
    ├── PluginEditor.h/.cpp        # Finestra principale UI, armonizzatore Trio, telemetria a 60 FPS
    ├── PreviewSynth.h             # Sintetizzatore sottrattivo polifonico interno con filtro SVF
    ├── SequenceStripComponent.h   # Striscia sequencer orizzontale, controlli EXP (ACC, GLD, RAT, LEG, VEL)
    └── StyleGenerator.h           # Motore algoritmico generativo su 8 stili musicali
```

---

## 3. Requisiti di Compilazione

- **Sistema Operativo**: Windows 10 / 11 (64-bit)
- **Compilatore C++**: Microsoft Visual Studio 2022 (MSVC v143) o build tools con supporto C++17
- **CMake**: Versione 3.22 o successiva
- **JUCE Framework**: JUCE 7.x
  - Se installato in `C:/JUCE/JUCE`, CMake lo rileva automaticamente.
  - In alternativa, definisci la variabile d'ambiente `JUCE_PATH` impostandola sul percorso della tua cartella JUCE.

### Comandi Rapidi di Compilazione (Terminale PowerShell):

```powershell
# Configurazione CMake (la prima volta)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Compilazione Release di VST3 e Standalone
cmake --build build --config Release --parallel

# I file generati si troveranno in:
# build/PulseForge_artefacts/Release/VST3/PulseForge.vst3
# build/PulseForge_artefacts/Release/Standalone/PulseForge.exe
```

---

## 4. Architettura Tecnica & Vincoli Fondamentali (Standard DaMA)

### A. Audio-Thread Safety (Tempo Reale a 0 Glitch)
- **MAI chiamate bloccanti dentro `processBlock()`**: nessun I/O su file, nessuna chiamata Win32 (`WriteFile`, `ReadFile`, `ClearCommError`), nessuna allocazione dinamica di memoria heap (`new`, `malloc`, `std::vector::push_back` non riservato).
- **Coda Tx Lock-Free (`HardwareControllerSerial::txQueue`)**:  
  Quando il sequencer fa scattare un LED flash ad ogni step (`sendStepFlash`), l'evento viene scritto istantaneamente in microsecondi in una coda circolare atomica lock-free (`OutgoingItem txQueue[128]`). È il thread di background (`flushOutgoingTx`) a trasmettere i byte sulla COM.
- **Snapshot Double-Buffering**:  
  La GUI modifica `tracks[]` sotto `patternLock`, poi chiama `updatePlaybackSnapshot()`. L'audio thread in `processBlock()` legge atomicamente solo lo snapshot di sola lettura attivo (`audioSnapshots[activeSnapshotIndex]`), garantendo zero contese con la GUI.
- **Dispath Asincrono dei Parametri**:  
  Tutti i comandi ricevuti dal controller fisico (potenziometri, pulsanti ed encoder) vengono recapitati ai parametri host tramite `juce::MessageManager::callAsync([this, ...] { p->setValueNotifyingHost(...); });` per evitare deadlock tra DAW e thread di sistema.

### B. Sistema Multi-Timbrico & Routing MIDI
- **3 Tracce Indipendenti**:
  - `SEQ 1`: Basso Monofonico (Canale MIDI 1 - Tonalità Cobalt Azure)
  - `SEQ 2`: Arpeggio / Lead Hook (Canale MIDI 2 - Tonalità Cyan Electric)
  - `SEQ 3`: Armonia & Accordi Polifonici (Canale MIDI 3 - Tonalità Royal Cerulean)
- **Uscita MIDI USB Diretta & Mirroring Canale 1**:  
  Nel menu `MIDI OUTPUT` di ciascuna striscia è possibile selezionare direttamente un synth USB hardware. Per massima compatibilità con i sintetizzatori fisici (che di fabbrica ascoltano sul Canale 1), il motore invia le note sia sul canale impostato per la traccia, sia sul Canale 1 quando indirizzato a una porta hardware specifica.

### C. Sezione EXP (Expanded Modifier Matrix)
- Cliccando su `[ + EXP ]` la striscia del sequencer si espande a 208px rivelando la matrice orizzontale interattiva:
  - **ACC**: Accento dinamico per-step (+velocity & cutoff boost).
  - **GLD**: Slide analogico TB-303 (legato portamento con sovrapposizione Note-On/Note-Off).
  - **RAT**: Ratchet roll (ripetizioni ritmiche 1x, 2x, 3x, 4x nello stesso step).
  - **LEG**: Gate sustain esteso (legato a 1.50x).
  - **VEL**: Barra di velocity interattiva con drag verticale e valore numerico.

---

## 5. Firmware Controller Hardware (Arduino Mega 2560)

Il firmware risiede in `Hardware_Arduino/controller_sequencer_PulseForge/`:
- **Baud Rate**: 115200 baud
- **Handshake Automatico**:  
  All'avvio, il plugin esegue la scansione delle porte COM Windows e invia `PF_PING?\n`.  
  Arduino risponde con `PF_PONG:PulseForge_Controller\n` abilitando la connessione bidirezionale.
- **Mappatura Pin**:
  - **Pulsanti**: Pin 2..7 con resistenze di pull-up interne e debounce software a 40ms.
  - **Potenziometri**: Ingressi analogici A0..A14 con deadband filter (soglia ±6) per azzerare il rumore a 50Hz dei pin non collegati.
  - **Encoder Rotativi**: Pin 22..48 per gestione Steps, Genre e Role.
  - **Display OLED SSD1306**: Predisposto via protocollo `OLED:T0:L1:testo` (modulo opzionale `#ifdef ENABLE_OLED`).

---

## 6. Installazione Rapida con Reset Cache Ableton

Lo script [`install_vst3.bat`](install_vst3.bat) automatizza l'intero processo di testing:
1. Copia `PulseForge.vst3` direttamente in `C:\Program Files\Common Files\VST3\DaMA\`.
2. Resetta la voce di scansione in `PluginScanDb.txt` in tutte le versioni di Ableton Live installate in `%APPDATA%\Ableton`.
3. Al successivo avvio di Ableton Live, il plugin viene ricaricato pulito e aggiornato all'ultimo build.
