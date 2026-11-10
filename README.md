# PulseForge (VST3 & Standalone MIDI Plugin)

Generative Algorithmic MIDI Sequencer, Pattern Morphing Engine & Arpeggiator with high-performance Soft-UI tactile design.

[![Download Release](https://img.shields.io/badge/Download-Windows%20v1.0.0%20(Pre--Compiled)-00B4D8?style=for-the-badge&logo=windows)](https://github.com/DaMa-sys/PulseForge/releases/latest)
[![Website](https://img.shields.io/badge/Official-Website%20Showcase-2FC0D0?style=for-the-badge)](https://dama-sys.github.io/PulseForge/)
[![Category](https://img.shields.io/badge/Category-Generative%20MIDI%20Sequencer-1B6F8A?style=for-the-badge)]()

---

## ⚡ Download Ready-To-Use Plugin (No Compilation Required)

If you don't want to build from source, download the pre-compiled binary package:
👉 **[Download PulseForge-Windows-v1.0.0.zip](https://dama-sys.github.io/PulseForge/download/PulseForge-Windows-v1.0.0.zip)**  
*(Also available on the [GitHub Releases](https://github.com/DaMa-sys/PulseForge/releases/latest) page)*

### Inside the ZIP:
1. `PulseForge.vst3` — Copy to `C:\Program Files\Common Files\VST3\DaMA\` to use in Ableton Live, FL Studio, Reaper, Cubase, Studio One, Bitwig.
2. `PulseForge.exe` — Standalone app (run directly to play and test patterns with the internal synth preview without opening a DAW).
3. `LEGGIMI.txt` — Quick start guide in Italian and English.

---

## 🎨 Visual Interface & Tactile Soft-UI Design

PulseForge features a custom **Soft-UI claymorphism / tactile membrane** interface inspired by minimal, modern hardware aesthetics:
- **Zero Visual Noise**: Seamless monochromatic cyan/azure gradient palette (`#2FC0D0` to `#2587B5`) with consistent directional lighting.
- **Physical Depth**: Rotary knobs and buttons extruded from the background membrane with soft radial wells, progressive multi-pass drop shadows, and delicate 1px upper specular rims.
- **High-Contrast Readability**: Active steps and generative triggers clearly separated with glowing ice-cyan (`#E8FBFD`) against muted unselected wells.
- **Hardware Acceleration & Zero CPU Overhead**: Complex shadows and gradient fields are cached into pre-rendered bitmap frames during resize, leaving 100% of CPU cycles dedicated to precision MIDI timing.

---

## 🚀 Key Features

### 1. 16-Step Polymetric Sequencer Matrix
- **Note / Pitch**: Micro-step note quantization to musical scales (Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Harmonic Minor, Melodic Minor, Pentatonic, Blues, Diminished, Whole Tone, Arabic).
- **Gate ON/OFF**: Individual step triggers with variable duration.
- **Octave Shift**: Step-by-step octave offsets ($-2$ to $+2$ octaves).
- **Accent**: Dynamic velocity punch with subtle swing accentuation.
- **Slide / Portamento**: Authentic TB-303 style legato pitch glide with automatic gate extension.
- **Ratchet**: High-speed rhythmic note bursts ($1\text{x}, 2\text{x}, 3\text{x}, 4\text{x}$ sub-divisions per step).
- **Modulation Lane**: Multi-mode bottom automation lane switchable between **Velocity**, **Gate Length**, **Filter CC**, and **Probability**.

### 2. Algorithmic Composition Generator
- **Multi-Genre Styles**: Techno (Driving / Hypnotic), Melodic House, Acid 303, Cyberpunk, Neo-Soul, Lo-Fi Chill, and Ambient Glitch.
- **Instrument Roles**: Basslines, Leads, Arpeggiated Plucks, Chords, and Texture Pads.
- **Algorithmic Density & Syncopation**: Control note density, rhythmic syncopation, octave jump probability, and acid spice.
- **Scale Re-Quantization**: Change key or scale in real-time while preserving harmonic coherence.

### 3. DAW Synchronization & Hardware Out
- **DAW Host Tempo Sync**: Locks perfectly to project BPM with fractional tempo multipliers/dividers ($1/4$, $1/8$, $1/16$, Triplets, Dotted).
- **Drag MIDI to DAW**: One-click button to drag the generated sequence directly into any MIDI track in your DAW.
- **4 Live Pattern Slots (A / B / C / D)**: Instant pattern switching with seamless transition for live performance.
- **Direct Hardware MIDI Out**: Route output directly to external hardware synthesizers (DIN MIDI / USB MIDI) without DAW latency.
- **Internal Synth Preview**: Audition sequences immediately with the built-in polyphonic subtractive synthesis engine.

---

## 🛠️ Building from Source

### Prerequisites
- **CMake** 3.22 or newer
- **Visual Studio 2022** (Windows) or **Xcode / Clang** (macOS)
- **JUCE Framework**

### Build Instructions
```bash
git clone https://github.com/DaMa-sys/PulseForge.git
cd PulseForge
cmake -B build -S .
cmake --build build --config Release
```

The compiled artefacts will be located in:
- `build/PulseForge_artefacts/Release/VST3/PulseForge.vst3`
- `build/PulseForge_artefacts/Release/Standalone/PulseForge.exe`

---

## 📄 License

Developed by **DaMA Audio**.  
Licensed under the **MIT License**.
