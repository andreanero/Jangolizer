<div align="center">

# Jangolizer — Lo‑Fi Modulation Plugin

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
![C++](https://img.shields.io/badge/C++-23-00599C.svg?logo=cplusplus)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-orange.svg)

<!-- UI mockup: copy the attached image to assets/ui-mockup.png to see it in the README -->

![UI Mockup](assets/ui-mockup.png)

Jangolizer is a compact, performance-minded audio effect inspired by circuit‑bent hardware. It provides an anti‑aliased LFO engine, soft saturation, and two processing modes (VCA/VCF). The desktop build includes a stylized GUI; the embedded build for Elk Audio OS is headless and parameter-only.

</div>

## UI (Design notes — based on the provided photo)

The project's visual identity uses the attached image as a base: a high‑contrast, desaturated background with vivid green accents (extracted from the subject's eyes). The UI proposal:

- Full‑bleed background image (dark monochrome) at the plugin window's back.
- Large, centered title overlay with bold, textured typography.
- Controls arranged in three vertical zones:
  - Left: LFO section (SPEED, WAVEFORM, DEPTH, BIAS)
  - Center: Visualizer / waveform display and big MODE switch
  - Right: Output & filter (GAIN, FILTER Q, PRESET)
- Knobs and sliders: black/white style with neon green highlights for active values.
- Compact responsive layout for small plugin windows; headless builds omit the UI entirely.

To use the provided photo as the mockup image, copy the attached image into the repository:

cp /path/to/attached/image.png assets/ui-mockup.png

(Replace the path above with the attachment path you used.)


## ✨ Highlights

- PolyBLEP anti‑aliased oscillator (Square, Triangle, Saw, Inverted Saw)
- Wide LFO range (0.1 Hz – 400 Hz) for modulation and ring modulation
- Dual modes: VCA (tremolo/ring) and VCF (LFO‑modulated bandpass)
- RT‑safe DSP: no allocations in audio thread, parameter smoothing, stereo processing
- Desktop GUI + headless Elk Audio OS target

## Quick Build

Clone and build (desktop GUI):

```bash
git clone <repository-url>
cd jangolizer
mkdir cmake-build && cd cmake-build
cmake --preset default
cmake --build --preset default
```

Headless (Elk Audio OS):

```bash
mkdir cmake-build-elk && cd cmake-build-elk
cmake --preset elk-headless
cmake --build --preset elk-headless
```

## Project Layout

- Source/: DSP and UI code (PluginProcessor, PluginEditor, PolyBLEP core)
- cmake/: build helpers and CPM integration
- assets/: UI images and resources (place ui-mockup.png here)
- libs/: external dependencies (gitignored)

## Parameters

- SPEED: 0.1 – 400 Hz (default 5 Hz)
- DEPTH: 0.0 – 1.0 (default 0.7)
- BIAS: -1.0 – 1.0 (default 0.0)
- GAIN: 1.0 – 10.0 (default 1.0)
- WAVEFORM: Square / Triangle / Saw / InvSaw
- MODE: VCA / VCF

## License

MIT — see LICENSE.md

## Contributing

Bugs and feature requests via GitHub issues. Pull requests welcome.

---

Built with JUCE 8.0.12 | C++23 | Real‑time safe
