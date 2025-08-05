# Spreadra Audio Plugin

Spreadra is a stereo width enhancement plugin that takes your audio signal, converts it to mid/side format, enhances the stereo base, and mixes it back with the original parallel signal.

## Features

- **Stereo Width Control**: Adjust the stereo field from 0% to 200%
- **Dry/Wet Mix**: Blend between the original and processed signal
- **Real-time Processing**: Low-latency audio processing
- **Cross-platform**: Available as AU and Standalone formats

## Parameters

### Stereo Width (0% - 200%)
Controls the width of the stereo field:
- 0%: Mono (no stereo separation)
- 100%: Original stereo width
- 200%: Maximum stereo enhancement

### Dry/Wet Mix (0% - 100%)
Blends between the original dry signal and the processed wet signal:
- 0%: Only dry (original) signal
- 50%: Equal mix of dry and wet
- 100%: Only wet (processed) signal

## Algorithm

Spreadra uses a mid/side processing approach:

1. **Input**: Stereo audio signal (L/R)
2. **Mid/Side Conversion**: Convert L/R to Mid/Side format
3. **Side Enhancement**: Apply stereo width processing to the side channel
4. **Reconversion**: Convert back to L/R format
5. **Mixing**: Blend with the original parallel signal using Dry/Wet control

This approach preserves the center image while allowing precise control over the stereo width.

## Installation

### macOS
- AU Component: `~/Library/Audio/Plug-Ins/Components/Spreadra.component`
- Standalone App: Applications folder

### Building from Source

#### Prerequisites
- CMake 3.15+
- JUCE framework
- C++17 compatible compiler
- FFTW3 library

#### Build Steps
```bash
# Clone the repository
git clone https://github.com/petersburgoffice/spreadra.git
cd spreadra

# Build with automatic versioning
./build_with_version.sh

# Or simple build
mkdir build && cd build
cmake ..
make
```

## Version

Current version: **0.9.1**

The project uses automatic versioning - each build increments the build number automatically.

## License

Copyright (c) 2024 SonicMakers. All rights reserved.

## Support

For issues and feature requests, please visit the [GitHub repository](https://github.com/petersburgoffice/spreadra).