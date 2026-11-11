# Antigravity Rules for DaMA PulseForge

## Project Context
- **Product**: DaMA PulseForge (Generative Polyrhythmic MIDI Sequencer & Arpeggiator VST3 / Standalone).
- **Framework**: JUCE 7 (C++17).
- **Hardware Controller**: Arduino Mega 2560 Desk Controller connected via USB Serial COM port at 115200 baud.
- **Audio Host Target**: Ableton Live 11 / 12 and standard VST3 DAW environments on Windows 10 / 11.

## Real-Time Audio Constraints (Crucial)
1. **Zero Blocking Calls in Audio Thread**:
   - Never call Win32 system I/O (`WriteFile`, `ReadFile`, `ClearCommError`, `Sleep`) or synchronization primitives (`std::mutex`, `juce::CriticalSection`) inside `processBlock()`, `triggerStepEventsForTrack()`, or any method called on the real-time audio thread.
   - Use the wait-free lock-free circular buffer `HardwareControllerSerial::txQueue` (`enqueueTx`) for any outgoing events.
2. **Zero Dynamic Memory Allocations**:
   - No `new`, `malloc`, `std::vector::push_back` (unless capacity is pre-reserved). Pre-reserve and size buffers in `prepareToPlay()` or the constructor.
3. **Double-Buffering for GUI & DSP Safety**:
   - Keep playback state read-only via `PlaybackSnapshot` double-buffering. Call `updatePlaybackSnapshot()` from GUI or non-realtime threads when modifying patterns.
4. **JUCE MessageManager Dispatch**:
   - Any parameter change or trigger originating from the serial worker thread must be wrapped in `juce::MessageManager::callAsync([this, ...] { ... });` before modifying `apvts` parameters or calling `setValueNotifyingHost`.

## UI / Design Principles (DaMA Style)
- Maintain the soft-UI aesthetic: satin gradients, 14px rounded chassis corners, and subtle perimeter glows.
- Use the 3 distinct azure blue shades for the sequence strips:
  - Strip 1 (Bass): Cobalt Marine Azure (`0xFF14476B` / `0xFF0D324D`, accent `0xFF38BDF8`).
  - Strip 2 (Arp): Electric Cyan Turquoise (`0xFF0F5A6D` / `0xFF0A3B48`, accent `0xFF00F0FF`).
  - Strip 3 (Chords): Royal Indigo Cerulean (`0xFF224E82` / `0xFF153457`, accent `0xFF818CF8`).
- Maintain bold high-contrast typography (`LandrFxTheme::font(..., true)`).
- When modifying controls, test both collapsed (126px) and expanded EXP (208px) layout modes.
