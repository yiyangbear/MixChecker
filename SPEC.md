# Mix Checker v0.1

## Goal

Build a simple macOS audio plugin for quickly checking how a mix translates to common playback environments.

The plugin is a checking tool, not an accurate physical simulation of specific devices.

## Platform

- macOS
- JUCE
- AU plugin
- Target DAW: Logic Pro

## Core Interaction

The user inserts Mix Checker on the master bus.

They can switch between:

- Bypass
- Phone
- Laptop
- Earbuds
- Car
- Club
- Mono

Only one mode can be active at a time.

## MVP DSP

Use simple DSP only:

- High-pass filter
- Low-pass filter
- Parametric EQ
- Stereo width
- Mono processing
- Output gain

Do not use convolution or complex acoustic simulation.

## UI

Fixed-size simple plugin window.

Buttons:

- Phone
- Laptop
- Earbuds
- Car
- Club
- Mono
- Bypass

The active mode must be visually obvious.

## Important Requirements

- Plugin must load correctly in Logic Pro
- Audio must pass through correctly
- Mode switching must not cause clicks or pops
- Bypass must restore the original signal
- Plugin state must be restored when reopening a Logic project

## Do Not Build Yet

Do not add:

- AI analysis
- Mix scoring
- Spectrum analyzer
- LUFS meter
- Reference tracks
- User presets
- Custom EQ
- IR convolution
- Account system
- Cloud features
- Licensing system
- Animations
- Resizable UI

## Development Order

1. Create JUCE AU plugin
2. Implement clean audio pass-through
3. Add plugin mode state
4. Implement reusable DSP engine
5. Implement Bypass and Phone mode
6. Add minimal UI for Bypass and Phone
7. Connect UI to DSP
8. Add Laptop, Earbuds, Car, Club and Mono
9. Add gain compensation
10. Add parameter smoothing
11. Save and restore plugin state
12. Test in Logic Pro

## Rule

Do not implement the whole project at once.

Complete one task at a time.

After each task:

- Make sure the project builds
- Make sure the plugin still loads
- Explain what changed
- Do not start the next task until requested