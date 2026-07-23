# Development Guide for Jangolizer Plugin

This document provides guidance for developers working on the Jangolizer plugin.

## Architecture Overview

### Signal Flow

Stages run sequentially (not mutually exclusive modes) — each stage blends
dry/wet via its own mix parameter, so all three can contribute at once.
Order is chosen for an ambient, dark-folk character (smooth swell first,
darkened by the filter, smeared last by the reverse stage):

```
Audio Input
    ↓
[Smoothed Gain Parameter]
    ↓
[Saturation Stage (tanh)]
    ↓
[VCA Stage: Multiply by LFO, blend via VCA_MIX]
    ↓
[VCF Stage: Bandpass Filter with LFO-controlled cutoff, blend via VCF_MIX]
    ↓
[REV Stage: SPEED-synced reverse chunks, blend via REV_MIX]
    ↓
Audio Output

Parallel to audio path:
[LFO Generator (PolyBLEP)]
    ├─→ Apply Speed (frequency)
    ├─→ Apply Depth (amplitude scaling)
    └─→ Apply Bias (DC offset)
```

Swap VCA/VCF ordering for a more industrial feel (filter grit before the
gate) — the LFO and saturation stages stay shared across all three.

## Core Components

### 1. **PolyBLEPOscillator.h**
- Anti-aliased waveform generator using Polynomial Band-Limited Step technique
- Supports 5 waveforms: Square, Triangle, Sawtooth, Inverted Sawtooth, Sine
- **Key Methods:**
  - `setSampleRate(double)` – Configure sample rate for frequency calculations
  - `setFrequency(float)` – Set oscillation frequency (Hz)
  - `setWaveform(Waveform)` – Select output waveform
  - `advance()` – Advance phase by one sample
  - `getSample()` – Return current oscillator value (-1.0 to 1.0)

**Aliasing Control:**
The `polyBlep()` function smooths discontinuities in square/sawtooth waveforms using residual compensation. This prevents high-frequency aliasing artifacts (especially critical at audio-rate LFO frequencies 50+ Hz).

### 2. **PluginProcessor.h / PluginProcessor.cpp**
The heart of the plugin. Inherits from `juce::AudioProcessor`.

**Key Responsibilities:**
- Parameter management via `AudioProcessorValueTreeState (apvts)`
- DSP process loop in `processBlock()`
- State serialization (preset save/load)
- Conditional UI creation based on `ELK_HEADLESS` flag

**Real-Time Safety (Elk OS):**
- No dynamic allocations in `processBlock()`
- `LinearSmoothedValue<float>` prevents parameter clicks
- `ProcessorDuplicator<FilterType, FilterState>` handles stereo filtering efficiently

**Process Loop (`processBlock`):**
1. Read smoothed parameter values
2. For each sample:
   - Advance LFO oscillator
   - Compute modulation signal (LFO × Depth + Bias)
   - Apply input gain + saturation
   - Apply VCA, VCF, REV stages in sequence, each blended by its own mix
     parameter (VCA_MIX / VCF_MIX / REV_MIX)

### 3. **PluginEditor.h / PluginEditor.cpp** (Desktop Only)
Conditional compilation: **Only compiled when `ELK_HEADLESS=0`**

Custom dark/industrial editor (`JangolizerAudioProcessorEditor`), not the generic
JUCE editor:
- `RotarySliderLook` — custom `LookAndFeel_V4` for the rotary knobs
- `setupSlider()` / `setupComboBox()` — shared styling helpers for each control
- Four rotary knobs (SPEED, DEPTH, BIAS, GAIN), WAVEFORM/MODE combo boxes, and a
  BYPASS toggle, all bound to `apvts` via `SliderAttachment` / `ComboBoxAttachment`
  / `ButtonAttachment`
- `backgroundImage` — static owl-eyes artwork loaded from `BinaryData::background_png`
  (built from `Source/Resources/background.png` via `juce_add_binary_data`), drawn full-bleed
  in `paint()`
- `drawIndustrialBackground()` — procedural fallback background, only used if the
  binary image fails to load
- Layout lives in `resized()`; colours/fonts are set per-control in the `setup*()` helpers

## Building & Testing

### Debug Build
```bash
cmake --preset default
cmake --build --preset default --config Debug
```
- Creates VST3, LV2, Standalone
- Includes GUI
- Larger binary, full debug symbols

### Release Build
```bash
cmake --preset release
cmake --build --preset release
```
- Optimized code
- Smaller binary
- Production-ready

### Elk Audio OS Build
```bash
cmake --preset elk-headless
cmake --build --preset elk-headless
```
- No GUI code compiled
- Minimal footprint
- Real-time safe

## Extending the Plugin

### Adding a New Parameter

1. **Add to `createParameterLayout()`** in `PluginProcessor.cpp`:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    "PARAM_ID",      // Unique identifier
    "Display Name",  // Human-readable label
    0.0f, 10.0f,     // Min, Max
    5.0f             // Default
));
```

2. **Create a smoothed value** in `PluginProcessor.h`:
```cpp
juce::LinearSmoothedValue<float> smoothedNewParam;
```

3. **Initialize in `prepareToPlay()`**:
```cpp
smoothedNewParam.reset(sampleRate, 0.02); // 20ms smoothing
```

4. **Use in `processBlock()`**:
```cpp
smoothedNewParam.setTargetValue(*apvts.getRawParameterValue("PARAM_ID"));
float value = smoothedNewParam.getNextSample();
```

### Adding a New Waveform

1. **Extend `PolyBLEPOscillator::Waveform` enum**:
```cpp
enum Waveform { Square, Triangle, Sawtooth, InvSawtooth, MyNewWave };
```

2. **Implement in `getSample()`**:
```cpp
case MyNewWave:
    output = /* your computation */;
    break;
```

3. **Update CMake parameter** in `PluginProcessor.cpp`:
```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    "WAVE", "LFO Waveform", 
    juce::StringArray { "Square", "Triangle", "Sawtooth", "InvSawtooth", "MyNewWave" }, 
    1
));
```

### Running Unit Tests

Tests live in `test/` (GoogleTest, fetched via CPM) and cover the DSP core
(`PolyBLEPOscillator`) and the processor (`PluginProcessor`: parameter defaults,
`processBlock()` output sanity in both VCA/VCF modes, state save/load round-trip).

```bash
cmake --preset default-with-tests
cmake --build --preset default-with-tests
ctest --preset default-with-tests
```

The test target compiles both `PluginProcessor.cpp` and `PluginEditor.cpp` with
`ELK_HEADLESS=0`, since `createEditor()` constructs the real
`JangolizerAudioProcessorEditor` — even though no test exercises the editor directly,
it has to be compiled and linked for the test binary to build.

### Adding a New Test

1. Add a `.cpp` file under `test/` with `TEST(...)` / `TEST_P(...)` cases
2. List it in `test/CMakeLists.txt`'s `add_executable(JangolizerTests ...)` sources

### Adding a New UI Control

To add a new control to the existing custom editor (`PluginEditor.h/.cpp`):

1. Declare the component (and label, if any) in `PluginEditor.h`
2. Style it via `setupSlider()` / `setupComboBox()`, or add a new helper for other
   component types (see `bypassButton` setup for a `ToggleButton` example)
3. Bind it to `apvts` with the matching attachment type
   (`SliderAttachment` / `ComboBoxAttachment` / `ButtonAttachment`)
4. Position it in `resized()`

## Performance Considerations

### Real-Time Safety Checklist
- [ ] No `new` / `delete` in `processBlock()`
- [ ] No `std::vector::resize()` in audio loop
- [ ] No mutex locks in audio thread
- [ ] No file I/O in audio thread
- [ ] Use `juce::LinearSmoothedValue` for smooth parameter changes
- [ ] Use `juce::ScopedNoDenormals` to prevent CPU overhead from denormalized floats

### Optimization Tips
1. **LFO Frequency**: The PolyBLEP computation is O(1). For high-freq LFOs (>100 Hz), consider band-limiting in a separate thread.
2. **Filter Updates**: IIR coefficients are computed once per block (from the block's final modulation value), not per sample — `bandPassFilter.process()` only ever sees the last-set coefficients anyway, so per-sample recomputation was pure waste.
3. **Saturation**: `std::tanh()` is relatively expensive. Consider lookup table for ultra-low-latency Elk deployments.

## Elk Audio OS Integration

### Mapping Parameters to Hardware
In Sushi's config.json:
```json
{
  "control_interface": "osc",
  "osc_server_port": 7890,
  "plugins": [
    {
      "uid": "jangolizer",
      "path": "path/to/plugin.so",
      "parameters": [
        { "id": "SPEED", "gpio": 3, "min": 0.1, "max": 400 },
        { "id": "DEPTH", "gpio": 4 }
      ]
    }
  ]
}
```

### OSC Control Example
```bash
oscsend localhost 7890 /parameter/jangolizer/SPEED f 50.0
```

## Common Issues & Solutions

### Plugin not loading in Ardour
- Check that VST3 plugin is in `~/.vst3/`
- Verify it was built for your platform (64-bit)
- Check Ardour's plugin browser for errors

### Elk Audio OS crashes
- Enable real-time kernel monitoring
- Check for allocations in `processBlock()`
- Use `juce::Logger` to debug (output goes to Elk logs)

### Parameter automation not working
- Verify parameter IDs match in `createParameterLayout()` and `processBlock()`
- Check that `apvts` is properly initialized in constructor
- Ensure `getStateInformation()` and `setStateInformation()` are implemented

## Code Style

This project follows:
- **C++23** modern conventions
- **JUCE style guide** (snake_case for variables, camelCase for methods)
- **clang-format** configuration in `.clang-format`

Run formatter:
```bash
clang-format -i Source/*.cpp Source/*.h
```

---

**For questions or contributions, refer to the main README.md**
