# LA9A Levelling Compressor

LA9A is a simple, great-sounding levelling compressor for vocals, instruments, and full mixes. It is designed around a single main `Compress` control, making it easy to dial in a natural, musical amount of gain reduction without a long list of controls.

## Features

- One-dial compression workflow
- Two modes: A and B with different character
- Input gain, output gain, and Auto Level matching
- Optional side-chain low-cut to reduce bass-triggered pumping
- Stereo input/output meters and gain reduction meter
- Clean front-panel layout for quick mixing decisions

## Controls

- Input: Adjusts the signal level before compression (-12 dB to +12 dB)
- Compress: Main compression amount, clockwise = more compression
- Side-Chain: Filters low frequencies from the detector when enabled
- Mode: A or B, offering two different compression responses
- Output: Makeup gain after compression (0 dB to +18 dB)
- Auto Level: Matches output level to input level when enabled
- Meters: Input/output level on the left and right, gain reduction in the center

## Quick Start

1. Open the project in a CMake-enabled environment.
2. Point CMake to a local JUCE checkout:
   ```bash
   cmake -B build -DJUCE_PATH=/path/to/JUCE
   cmake --build build --config Release -j4
   ```
3. The plugin is built as a VST3 plugin, and on macOS also as an AU.

## Project Layout

- `Source/` contains the plugin DSP, editor, and UI code
- `Assets/` contains the GUI skin and control graphics
- `CMakeLists.txt` defines the build for JUCE and the plugin target
- `LA9A User Manual 100.pdf` is the reference manual for the product

## License

This project is distributed under the Studio9 Plugins freeware license. It is free for personal, educational, non-profit, and commercial use, with restrictions on resale, redistribution, modification, and reverse engineering. See the manual for the full license terms and warranty disclaimer.

## Notes

This plugin is intended as a free, lightweight mixing tool. The manual recommends trusting your ears first and using the meters as a guide rather than as the final authority.
