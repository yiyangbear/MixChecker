# Mix Checker

A lightweight macOS audio plugin for quickly checking how a mix translates across different playback environments.

![Mix Checker](docs/mix-checker.png)

Mix Checker lets producers switch between common listening scenarios such as phones, laptops, earbuds, cars, clubs, and mono playback directly inside their DAW.

The goal is simple:

> Does my mix still work outside the studio?

## Features

Mix Checker currently includes:

- Phone
- Laptop
- Earbuds
- Car
- Club
- Mono
- Bypass

Each mode applies a lightweight DSP profile designed to highlight common mix translation problems.

Mix Checker is intended as a monitoring and checking tool rather than an exact physical simulation of specific devices.

## Current Version

v0.1

The current version focuses on the core workflow:

1. Insert Mix Checker on the master bus.
2. Play your mix.
3. Switch between playback environments.
4. Compare how the mix translates.

## Platform

Currently tested on:

- macOS
- Apple Silicon
- Logic Pro
- Audio Unit (AU)

## Tech Stack

- C++
- JUCE 9
- CMake
- Audio Unit

## Build

Clone the repository:

```bash
git clone YOUR_REPOSITORY_URL
cd MixChecker
```

Configure the project:

```bash
cmake -S . -B build \
  -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
```

Build the Audio Unit:

```bash
cmake --build build --target MixChecker_AU --parallel 4

The generated plugin can be found at:
build/MixChecker_artefacts/Release/AU/Mix Checker.component
```

Install it to:

```bash
~/Library/Audio/Plug-Ins/Components/
```

Then restart the Audio Component Registrar if necessary:

```bash
killall AudioComponentRegistrar 2>/dev/null || true
```

Validate the plugin:

```bash
auval -v aufx MxCh MxCk -strict
```

## Design Philosophy
Mix Checker intentionally keeps the workflow simple.
It does not attempt to perfectly reproduce a specific phone, car, speaker, or venue.
Instead, each mode represents a useful playback condition that helps expose potential translation problems in a mix.

## Roadmap
Possible future improvements include:

- Improved playback profiles
- Additional listening environments
- UI refinements
- VST3 support
- Better calibration and loudness matching

The current focus is keeping the plugin fast, simple, and useful.

## Status
Mix Checker is currently an early experimental project.
v0.1 is functional and has been tested in Logic Pro on macOS.