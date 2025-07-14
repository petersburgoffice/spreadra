# Reverbix Audio Plugin

A professional reverb audio plugin built with JUCE framework by SonicMakers.

## Features

- **High-Quality Reverb**: Professional Schroeder/FDN algorithm reverb
- **Room Size Control**: Adjustable room size (10-10000 m²)
- **Decay Time**: Full control over reverb decay (0.1-20s)
- **Stereo Width**: Adjustable stereo width (0-200%)
- **Dry/Wet Mix**: Blend between original and reverbed signal (0-100%)
- **Modern UI**: Clean, professional interface with rotary controls
- **High Performance**: Optimized DSP algorithms for low latency

## Parameters

- **Room Size**: 10-10000 m² (default: 1000 m²)
- **Decay Time**: 0.1-20s (default: 3.0s)
- **Stereo Width**: 0-200% (default: 100%)
- **Dry/Wet**: 0-100% (default: 50%)

## Technical Specifications

- **Sample Rate**: 44.1kHz - 96kHz+
- **Bit Depth**: 32-bit float internal processing
- **Algorithm**: Schroeder/FDN with comb and all-pass filters
- **Latency**: Low latency design
- **Formats**: AU, Standalone
- **Platform**: macOS (Intel/Apple Silicon)

## Building

Requirements:
- CMake 3.15+
- JUCE 7.0+
- macOS 10.15+
- Xcode 12+

```bash
mkdir build
cd build
cmake ..
make
```

## Installation

The plugin automatically installs to `~/Library/Audio/Plug-Ins/Components/` after building.

## Company

**SonicMakers** - Professional Audio Plugin Development
- Manufacturer Code: Snmk
- Plugin Code: Rvbx

## Version

Version 0.1.0 