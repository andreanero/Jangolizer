<div align="center">

# Jangolizer — Lo‑Fi Modulation Plugin

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
![C++](https://img.shields.io/badge/C++-23-00599C.svg?logo=cplusplus)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-orange.svg)
[![Buy Me a Coffee](https://img.shields.io/badge/PayPal-Buy%20me%20a%20coffee-00457C.svg?logo=paypal)](https://paypal.me/andreaveronese)

Jangolizer is a compact, performance-minded audio effect inspired by circuit‑bent hardware. It provides an anti‑aliased LFO engine, soft saturation, and a sequential VCA → VCF → REV effect chain, each stage blended independently via its own mix knob. The desktop build includes a stylized GUI; the embedded build for Elk Audio OS is headless and parameter-only.

</div>

## UI

Dark, industrial look with orange accents, static owl-eyes background artwork (`Source/Resources/background.png`, compiled in via JUCE BinaryData):
- Title banner ("JANGOLIZER") over the background artwork.
- Four rotary knobs — SPEED, DEPTH, BIAS, GAIN — in a row.
- WAVEFORM selector plus VCA_MIX, VCF_MIX, REV_MIX knobs below the main row.
- BYPASS toggle below the selectors — on by default every time the plugin loads.
- Headless (Elk Audio OS) builds omit the UI entirely; only the parameters remain.

## ✨ Highlights

- PolyBLEP anti‑aliased oscillator (Square, Triangle, Saw, Inverted Saw, Sine)
- Wide LFO range (0.1 Hz – 400 Hz) for modulation and ring modulation
- Sequential VCA → VCF → REV chain (tremolo/ring → LFO‑modulated bandpass → SPEED‑synced reverse chunks), each stage independently blendable
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

### Headless (Elk Audio OS, Raspberry Pi 4 / aarch64)

The headless target (`ELK_HEADLESS_BUILD=ON`, `ELK_HEADLESS=1`) strips the GUI (`PluginEditor.cpp`/`.h` and `JangolizerBinaryData` are excluded from the build, see `CMakeLists.txt`) and builds parameter-only, for Elk Audio OS running on a Pi 4. This is a **cross-compile**, not a native build — you need the Elk Pi4 SDK on your host (Linux or WSL) to get the `aarch64-elk-linux` toolchain.

1. Install the Elk Pi4 cross-compilation SDK (`elkpi-sdk-*.sh`, from the [Elk Audio OS releases](https://github.com/elk-audio/elkpi-sdk)). Default install path is `/opt/elk`.
2. Source the SDK environment in the shell you'll build from (this exports `CC`/`CXX` and, importantly, `OE_CMAKE_TOOLCHAIN_FILE`):
   ```bash
   unset LD_LIBRARY_PATH
   source /opt/elk/<version>/environment-setup-aarch64-elk-linux
   ```
3. Configure and build. The `elk-headless` preset picks up `OE_CMAKE_TOOLCHAIN_FILE` from the environment automatically (see `CMakePresets.json`):
   ```bash
   cmake --preset elk-headless
   cmake --build --preset elk-headless
   ```
4. Copy the resulting VST3/LV2 artefact from `cmake-build-elk/` to the board (e.g. `scp` into `/udata/sushi/plugins/`) and point Sushi's config at it.

If step 3 fails with an empty/invalid toolchain file path, the SDK environment wasn't sourced in that shell — steps 2 and 3 must run in the same shell session.

## Project Layout

- Source/: DSP and UI code (PluginProcessor, PluginEditor, PolyBLEP core)
- Source/Resources/: UI assets (background.png), compiled in via JUCE BinaryData
- cmake/: build helpers and CPM integration
- libs/: external dependencies (gitignored)

## Parameters

- SPEED: 0.1 – 400 Hz (default 5 Hz) — LFO rate for VCA/VCF stages, reverse chunk rate for REV stage (chunk length clamped to 2 s max)
- DEPTH: 0.0 – 1.0 (default 0.7) — modulation depth for VCA/VCF stages, dry/reversed mix inside the REV stage
- BIAS: -1.0 – 1.0 (default 0.0)
- GAIN: 1.0 – 10.0 (default 1.0)
- WAVEFORM: Square / Triangle / Saw / InvSaw / Sine
- VCA_MIX / VCF_MIX / REV_MIX: 0.0 – 1.0 — dry/wet blend for each chain stage, applied in order (VCA → VCF → REV)
- BYPASS: on / off (default on, reset to on every load — not restored from saved state)

## Resources

- [WolfSound](https://thewolfsound.com/) — audio DSP & JUCE tutorials
- [WolfSound courses](https://thewolfsound.com/courses/) — structured JUCE plugin development courses
- [Elk Audio OS forum](https://forum.elk.audio/) — Elk Pi4 SDK/Sushi questions and community support

## License

MIT — see LICENSE.md

## Contributing

Bugs and feature requests via GitHub issues. Pull requests welcome.

---

Built with JUCE 8.0.12 | C++23 | Real‑time safe
