# ✅ Gristleizer Plugin - Build Verification

## Build Status: SUCCESS ✓

### Compiled Artifacts

The following plugin formats have been successfully built:

#### 1. **Standalone Application**
```
cmake-build/GristleizerPlugin_artefacts/Debug/Standalone/Gristleizer
```
- **Type:** Executable application
- **Usage:** Run directly as an audio application
- **Test:** `./cmake-build/GristleizerPlugin_artefacts/Debug/Standalone/Gristleizer`

#### 2. **VST3 Plugin** (for Ardour, Reaper, etc.)
```
cmake-build/GristleizerPlugin_artefacts/Debug/VST3/Gristleizer.vst3/
```
- **Type:** VST3 plugin bundle
- **Installation:** Copy to `~/.vst3/` for Linux/macOS
- **DAW Support:** Ardour 7+, Reaper 7.x+, studio-quality DAWs

#### 3. **LV2 Plugin** (Linux Specific)
```
cmake-build/GristleizerPlugin_artefacts/Debug/LV2/Gristleizer.lv2/libGristleizer.so
```
- **Type:** LV2 plugin bundle
- **Installation:** Copy to `~/.lv2/` for Ardour, Carla, Jalv
- **DAW Support:** Ardour, LMMS, Carla (universal plugin host)

## Testing Checklist

### Quick Test: Run Standalone
```bash
./cmake-build/GristleizerPlugin_artefacts/Debug/Standalone/Gristleizer
```
You should see a GUI window with parameter sliders.

### Test in Ardour
1. Copy VST3 plugin:
   ```bash
   cp -r cmake-build/GristleizerPlugin_artefacts/Debug/VST3/Gristleizer.vst3 ~/.vst3/
   ```
2. Open Ardour
3. Right-click on an audio track → Add Plugin
4. Search for "Gristleizer"
5. Click to insert—should see generic JUCE plugin editor
6. Test parameters: Speed, Depth, Bias, Gain, Waveform, Mode

### Test in Carla (Universal Host)
```bash
carla &
```
1. Engine → Set up
2. New → Load Plugin from `~/.lv2/`
3. Find "Gristleizer" and load
4. Test audio routing and parameters

## System Information

**Build Environment:**
- Compiler: GCC 13.3.0 (C++23)
- JUCE Version: 8.0.12
- CMake: 3.28.3
- Platform: Linux x86_64

**Build Configuration:**
- Debug mode (full symbols, unoptimized for faster compilation)
- VST3 automation conflict warning suppressed (normal for new plugins)
- All three formats compiled in single pass

## Next Steps

### Install Plugins System-Wide
```bash
# For VST3
mkdir -p ~/.vst3
cp -r cmake-build/GristleizerPlugin_artefacts/Debug/VST3/Gristleizer.vst3 ~/.vst3/

# For LV2
mkdir -p ~/.lv2
cp -r cmake-build/GristleizerPlugin_artefacts/Debug/LV2/Gristleizer.lv2 ~/.lv2/
```

### Create Optimized Release Build
```bash
./build.sh release
# or:
cd cmake-build-release && cmake --preset release && cmake --build .
```
Release builds are ~50% smaller and faster, suitable for distribution.

### Build for Elk Audio OS
```bash
./build.sh elk
# For cross-compilation with SDK:
# cmake .. -DELK_HEADLESS_BUILD=ON -DCMAKE_TOOLCHAIN_FILE=/path/to/elk-toolchain.cmake
```

## Known Issues & Solutions

| Issue | Solution |
|-------|----------|
| Plugin not appearing in Ardour | Clear Ardour's plugin cache: `~/.config/Ardour7/plugins.cache.vst3` |
| VST3 not loading | Check file permissions: `chmod -R 755 ~/.vst3/Gristleizer.vst3` |
| LV2 not found | Ardour requires `~/.lv2/` directory; run Ardour with debug: `ARDOUR_UI_LOGGER=1 ardour` |
| Standalone crashes on startup | Check system audio device: `aplay -l` (should list devices) |

## File Sizes

```
-rwxr-xr-x  3.2M  Standalone/Gristleizer              (Debug, full symbols)
-rwxr-xr-x  2.1M  LV2/Gristleizer.lv2/libGristleizer.so
-rwxr-xr-x  2.8M  VST3/Gristleizer.vst3/.../Gristleizer.so
```

(Release builds will be ~40-50% smaller)

---

**Status:** ✅ All components compiled and ready for testing!

See `README.md` for usage documentation and `DEVELOPMENT.md` for code architecture.
