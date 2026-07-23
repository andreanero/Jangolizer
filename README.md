<div align="center">

# Jangolizer — Lo‑Fi Modulation Plugin

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
![C++](https://img.shields.io/badge/C++-23-00599C.svg?logo=cplusplus)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-orange.svg)
[![Buy Me a Coffee](https://img.shields.io/badge/PayPal-Buy%20me%20a%20coffee-00457C.svg?logo=paypal)](https://paypal.me/andreaveronese)

Jangolizer is a compact, performance-minded audio effect inspired by circuit‑bent hardware. It provides an anti‑aliased LFO engine, soft saturation, and three processing modes (VCA/VCF/REV). The desktop build includes a stylized GUI; the embedded build for Elk Audio OS is headless and parameter-only.

</div>

## UI

Dark, industrial look with orange accents, static owl-eyes background artwork (`Source/Resources/background.png`, compiled in via JUCE BinaryData):
- Title banner ("JANGOLIZER") over the background artwork.
- Four rotary knobs — SPEED, DEPTH, BIAS, GAIN — in a row.
- WAVEFORM and MODE selectors below the knobs.
- BYPASS toggle below the selectors — on by default every time the plugin loads.
- Headless (Elk Audio OS) builds omit the UI entirely; only the parameters remain.

## ✨ Highlights

- PolyBLEP anti‑aliased oscillator (Square, Triangle, Saw, Inverted Saw, Sine)
- Wide LFO range (0.1 Hz – 400 Hz) for modulation and ring modulation
- Three modes: VCA (tremolo/ring), VCF (LFO‑modulated bandpass), REV (SPEED‑synced reverse chunks)
- RT‑safe DSP: no allocations in audio thread, parameter smoothing, stereo processing
- Desktop GUI + headless Elk Audio OS target

## Quick Build

Clone the repo, then configure and build with CMake presets — no OS-specific script needed.

### Linux / macOS

```bash
git clone <repository-url>
cd jangolizer
cmake --preset default
cmake --build --preset default
```

Release build: `cmake --preset release && cmake --build --preset release`

### Windows

```bat
git clone <repository-url>
cd jangolizer
cmake --preset default
cmake --build --preset default --config Debug
```

Uses whatever Visual Studio version CMake detects on your machine. To pin a generator explicitly, use the `vs` preset (Visual Studio 2026) or `ninja-debug`/`ninja-release` (needs Ninja + MSVC on `PATH`, e.g. from a "Developer PowerShell for VS").

### Headless (Elk Audio OS)

```bash
cmake --preset elk-headless
cmake --build --preset elk-headless
```

## Project Layout

- Source/: DSP and UI code (PluginProcessor, PluginEditor, PolyBLEP core)
- Source/Resources/: UI assets (background.png), compiled in via JUCE BinaryData
- cmake/: build helpers and CPM integration
- libs/: external dependencies (gitignored)

## Parameters

- SPEED: 0.1 – 400 Hz (default 5 Hz) — LFO rate in VCA/VCF, reverse chunk rate in REV (chunk length clamped to 2 s max)
- DEPTH: 0.0 – 1.0 (default 0.7) — modulation depth in VCA/VCF, dry/reversed mix in REV
- BIAS: -1.0 – 1.0 (default 0.0)
- GAIN: 1.0 – 10.0 (default 1.0)
- WAVEFORM: Square / Triangle / Saw / InvSaw / Sine
- MODE: VCA / VCF / REV
- BYPASS: on / off (default on, reset to on every load — not restored from saved state)

## License

MIT — see LICENSE.md

## Contributing

Bugs and feature requests via GitHub issues. Pull requests welcome.

---

Built with JUCE 8.0.12 | C++23 | Real‑time safe
