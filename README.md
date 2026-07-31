<div align="center">

# Jangolizer — Lo‑Fi Modulation Plugin

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
![C++](https://img.shields.io/badge/C++-23-00599C.svg?logo=cplusplus)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-orange.svg)
[![Buy Me a Coffee](https://img.shields.io/badge/PayPal-Buy%20me%20a%20coffee-00457C.svg?logo=paypal)](https://paypal.me/andreaveronese)

Jangolizer is a compact, performance-minded audio effect inspired by circuit‑bent hardware. Mono in, mono out (single-channel guitar/instrument source, matching the Elk Audio OS hardware target). It provides an anti‑aliased LFO engine, soft saturation/VCA stage, an LFO‑swept, resonant bandpass filter (VCF) applied to the entry signal itself (auto‑wah style), and an internal noise generator — darkened toward brown/pink, driven into fuzz saturation, and shaped by its own low, resonant low‑pass filter for a sunn O)))‑style wall of low drone noise — whose output swells on top of the entry signal. A fixed feedback delay and a final soft-clip add a dark, cavernous sustain suited to industrial/post‑punk/drone playing. The desktop build includes a stylized GUI; the embedded build for Elk Audio OS is headless and parameter-only.

</div>

## UI

Dark, industrial look with orange accents, static owl-eyes background artwork (`Source/Resources/background.png`, compiled in via JUCE BinaryData):
- Title banner ("JANGOLIZER") over the background artwork.
- Four rotary knobs — SPEED, DEPTH, BIAS, GAIN — in a row.
- WAVEFORM selector plus VCA_MIX, VCF_MIX, NOISE_LEVEL knobs below the main row.
- BYPASS toggle below the selectors — on by default every time the plugin loads.
- Headless (Elk Audio OS) builds omit the UI entirely; only the parameters remain.

## ✨ Highlights

- PolyBLEP anti‑aliased oscillator (Square, Triangle, Saw, Inverted Saw, Sine)
- Wide LFO range (0.1 Hz – 400 Hz) for modulation and ring modulation
- VCA stage (tremolo/ring) followed by an LFO‑swept, near‑self‑oscillating bandpass filter (VCF) applied to the entry signal (auto‑wah sweep)
- Independent noise drone path: brown/pink-darkened, fuzz-saturated noise through its own low, resonant low-pass filter (wall of low drone), layered additively on top of the entry signal with a slow swell/fade envelope (VCF_MIX blends both dry/filtered signal and raw/filtered noise, NOISE_LEVEL sets drone volume)
- Fixed feedback delay and final soft-clip for a dark, cavernous sustain tail — always on, no dedicated knob
- RT‑safe DSP: no allocations in audio thread, parameter smoothing, mono processing
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

- SPEED: 0.1 – 400 Hz (default 5 Hz) — LFO rate driving the VCA tremolo and the VCF cutoff sweep
- DEPTH: 0.0 – 1.0 (default 0.7) — modulation depth for the VCA/VCF LFO
- BIAS: -1.0 – 1.0 (default 0.0)
- GAIN: 1.0 – 10.0 (default 1.0)
- WAVEFORM: Square / Triangle / Saw / InvSaw / Sine
- VCA_MIX: 0.0 – 1.0 — dry/wet blend of the tremolo stage against the saturated entry signal
- VCF_MIX: 0.0 – 1.0 — dual-purpose: dry/wet blend of the resonant LFO‑swept filter against the entry signal itself (auto‑wah sweep), AND raw fuzzed noise (0) vs. low-pass filtered noise (1) for the drone
- NOISE_LEVEL: 0.0 – 1.0 — volume of the noise drone, added on top of the entry signal (additive, not a crossfade)
- BYPASS: on / off (default on, reset to on every load — not restored from saved state)

Not exposed as parameters (fixed character of the effect): filter resonance (Q ≈ 9, near self‑oscillating), feedback delay (~0.35s, 40% feedback, 30% wet, always on), and a final soft-clip safety stage.

## License

MIT — see LICENSE.md

## Contributing

Bugs and feature requests via GitHub issues. Pull requests welcome.

---

Built with JUCE 8.0.12 | C++23 | Real‑time safe
