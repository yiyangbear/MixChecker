# Mix Checker Progress

## Current Status

Mix Checker v0.1 is complete. The stereo Audio Unit is validated in Logic Pro, and a signed arm64 VST3 build is now generated and accepted by the Cubase Pro 15 scanner.

Current task:

Post-v0.1 distribution preparation — add VST3 and rewrite the GitHub README (completed)

## Completed Tasks

- Task 1 — Create JUCE AU plugin skeleton
- Task 2 — Implement clean audio pass-through
- Task 3 — Add plugin mode state
- Task 4 — Implement reusable DSP engine
- Task 5 — Implement Bypass and Phone mode
- Task 6 — Build the first custom UI
- Task 7 — Connect UI to mode state and DSP
- Task 8 — Add Laptop, Earbuds, Car, Club and Mono
- Task 9 — Add gain compensation
- Task 10 — Add parameter smoothing
- Task 11 — Save and restore plugin state
- Task 12 — Final Logic Pro testing and v0.1 cleanup

## In Progress

None.

## Remaining Tasks

None for v0.1.

## Current Problems

No functional blocker. Full VST3 insert/editor/audio/state testing remains pending because this pass reached successful Cubase Pro 15 scanning but did not create a safe temporary host project. Public distribution also still needs a chosen repository license plus Developer ID signing/notarization or clear source-build-only positioning.

## Compact UI Layout Reference

All values are logical pixels in the fixed 474 x 208 editor, using top-left `(x, y)` coordinates.

- Editor / embedded faceplate asset: `(0, 0, 474, 208)`; runtime PNG is `1422 x 624` (3x).
- Main buttons: `62 x 60` at y `85`; x positions are `23, 96, 169, 242, 315, 388` for Phone, Laptop, Earbuds, Car, Club, and Mono.
- Main button artwork: `186 x 180` (3x) for both up and down states. Text is drawn inside each complete `62 x 60` button rectangle, centred, 10 px Medium; selected/pressed text moves down 2 px.
- Mode lamps: 11 px diameter, centred at `(54, 72)`, `(127, 72)`, `(200, 72)`, `(273, 72)`, `(346, 72)`, and `(419, 72)`.
- Lamp artwork: `33 x 33` (3x) for both off and on states; ACTIVE reuses the same pair.
- Bypass button: `(30, 160, 76, 24)` with `228 x 78` (3x) up/down artwork; text is centred at 10 px Medium. Its visible frame now clears both the divider and the silver panel's lower bevel.
- Active lamp: 9 px diameter centred at `(420, 35)`; ACTIVE label is `(401, 40, 38, 10)`, directly below the lamp as one compact group.
- Main title: `(34, 29, 300, 20)`, 17 px Regular with 0.10 extra kerning; its restrained highlight is offset to y `30`.
- Subtitle: `(34, 50, 210, 10)`, 7.5 px Regular.
- Stereo Monitoring label: `(304, 174, 134, 10)`, 7.5 px Regular, right-aligned with its right edge at x `438`.
- Four screws: 7 px diameter centred at `(21, 29)`, `(453, 29)`, `(21, 181)`, and `(453, 181)`. All four reuse the same embedded `21 x 21` (3x) screw image and sit inside the silver inner panel rather than on the editor's outer boundary.
- Horizontal divider is embedded once in the faceplate at logical y `157`; JUCE does not draw a duplicate line.
- Outer enclosure, silver panel, bevels, and divider remain in the faceplate asset. The original baked screws were removed and the brushed metal was locally repaired before the shared screw asset was overlaid by JUCE.

## Notes

- Read `SPEC.md` before making changes.
- Work on only one task at a time.
- Do not start the next task unless explicitly requested.
- After completing a task, update this file.
- Keep the project buildable after every task.
- Task 1 uses JUCE 9.0.3 through CMake FetchContent when JUCE is not already installed.
- The Release AU build passed `auval` strict validation, including stereo 2-in/2-out, rendering, Cocoa editor view, and repeated load/unload checks.
- Logic Pro 11.2.2 scanned and loaded the stereo plugin, and its editor window opened successfully.
- Task 2 keeps `processBlock()` intentionally empty because JUCE supplies effect input and output in the same buffer; leaving the samples untouched is transparent pass-through.
- The automated pass-through test confirmed distinct left and right buffers remain bit-for-bit identical and the processor reports zero latency.
- The Task 2 Release AU build passed strict `auval` validation and loaded, bypassed, reactivated, and opened its editor in Logic Pro 11.2.2 without crashing.
- Task 3 defines one strongly typed mode enum with Bypass, Phone, Laptop, Earbuds, Car, Club, and Mono values.
- The current mode defaults to Bypass and is exposed through thread-safe `getMode()` and `setMode()` methods; no parameter or persistence layer has been added.
- Automated tests confirmed all seven modes can be selected one at a time and that stereo audio remains bit-for-bit unchanged in every mode.
- The Task 3 Release AU build passed strict `auval` validation and remained loadable in Logic Pro 11.2.2 with its editor opening normally.
- Task 4 adds one reusable stereo DSP engine with high-pass, one parametric EQ band, low-pass, stereo width, and output gain stages.
- The DSP engine is prepared by `prepareToPlay()`, processed by `processBlock()`, and reset by `releaseResources()`; filter state allocation occurs during preparation rather than in the audio callback.
- Neutral defaults bypass the filters and flat EQ and skip unity width and gain arithmetic, preserving bit-exact stereo pass-through in every mode.
- Automated tests confirmed neutral bit-exact output, all existing mode transitions, code-based DSP configuration, and valid processing through an enabled generic chain.
- The Task 4 Release AU passed strict `auval` validation and remained loadable in Logic Pro 11.2.2 with its editor opening normally.
- Task 5 maps Bypass to a direct transparent path and Phone to a reusable DSP preset; all other modes remain neutral and unimplemented.
- Phone settings: 180 Hz HPF, 7.5 kHz LPF, +3 dB peak EQ at 1.6 kHz with Q 0.8, stereo width 0.35, and -1.5 dB output gain.
- Automated tests confirmed bit-exact Bypass, reduced 80 Hz and 12 kHz response relative to 1.6 kHz, narrower stereo width, stable finite output, and a clear signal difference from Bypass.
- The Task 5 Release AU passed strict `auval` validation and remained loadable in Logic Pro 11.2.2 with its editor opening normally.
- Mode changes are intentionally unsmoothed in Task 5 and may click; smoothing remains scheduled for Task 10.
- Task 6 replaces the generic JUCE editor with a fixed 560 x 260 custom editor using a warm dark-gray panel, off-white text, subtle borders, rounded buttons, and a muted amber active state.
- The editor exposes Phone, Laptop, Earbuds, Car, Club, Mono, and Bypass in a balanced two-row layout with a centered Bypass control; radio-group behavior and processor mode callbacks keep exactly one button selected.
- Phone and Bypass retain their Task 5 processing. Laptop, Earbuds, Car, Club, and Mono update the internal mode only and remain transparent; no new device DSP was added.
- Automated tests verify the fixed editor size, all seven visible in-bounds controls, default Bypass selection, one selected mode after every button click, existing bit-exact neutral paths, and Phone processing.
- The Task 6 Release AU passed strict `auval` validation, including its custom Cocoa view and stereo render tests. Logic Pro 11.2.2 displayed the custom editor, switched all seven modes correctly, played audio through Bypass and Phone, and repeatedly opened and closed the editor without crashing.
- The Task 6 UI polish pass reduces the title weight, tightens all mode controls, gives Bypass the same compact treatment, and adds a recessed playback-environment panel with restrained ivory and gray-brown typography.
- Each button now has a low-contrast inactive status lamp and a warm amber active lamp, plus subtle border, top highlight, bottom inset shadow, and 7 px corners. No gradients, images, textures, animation, DSP changes, or new features were added.
- The polished Release AU passed the automated test suite and strict `auval` validation. Logic Pro 11.2.2 displayed the refined editor correctly and verified the amber indicator moving between Phone and Bypass while maintaining exactly one selected mode.
- Task 7 keeps the processor as the single source of truth: button callbacks request a mode through `setMode()`, then redraw from `getMode()` rather than storing an independent active mode in the editor.
- A lightweight 30 Hz editor timer follows processor-side mode changes and updates the selected indicators only when the reported mode differs; button clicks still update immediately.
- Phone continues to select the existing Task 5 DSP preset, Bypass remains bit-exact, and Laptop, Earbuds, Car, Club, and Mono remain bit-exact neutral paths. No Task 8 DSP was added.
- Automated tests verify every UI button against processor state, exactly-one selection, Phone processing, neutral output for the six non-Phone modes, and processor-to-editor resynchronization.
- The Task 7 Release AU passed strict `auval` validation. During active playback in Logic Pro 11.2.2, the Phone → Bypass → Phone → Laptop → Car → Bypass stress sequence completed with synchronized indicators, continued playback, a responsive UI, and no crash.
- Task 8 adds reusable presets for Laptop, Earbuds, Car, and Club through the existing HPF → parametric EQ → LPF → stereo-width → output-gain engine. Phone and Bypass are unchanged.
- Laptop settings: 120 Hz HPF, 14 kHz LPF, +2 dB peak EQ at 1.8 kHz with Q 0.8, stereo width 0.75, and -1 dB output gain.
- Earbuds settings: 30 Hz HPF, 18 kHz LPF, +2 dB peak EQ at 110 Hz with Q 0.8, stereo width 1.05, and -1 dB output gain.
- Car settings: 30 Hz HPF, 12 kHz LPF, +3 dB peak EQ at 180 Hz with Q 0.7, stereo width 1.15, and -2 dB output gain.
- Club settings: 25 Hz HPF, 18 kHz LPF, +3 dB peak EQ at 75 Hz with Q 0.8, stereo width 1.10, and -2 dB output gain.
- Mono disables HPF, LPF, and EQ, keeps 0 dB output gain, and sets stereo width to 0.0; the existing mid/side stage therefore writes `(L + R) * 0.5` to both channels, including standard anti-phase cancellation.
- Automated tests confirm every preset's exact values, distinct non-Bypass processing, finite stereo output, the intended relative frequency-response differences, byte-identical Mono channels, anti-phase Mono cancellation, and bit-exact Bypass.
- The Task 8 Release AU built successfully, retained a valid ad-hoc signature, and passed strict `auval` validation, including stereo format, Cocoa editor, and multi-rate render tests.
- Logic Pro 11.2.2 loaded the freshly installed AU after restart. During active stereo playback, all seven modes selected exactly one indicator, three rapid complete mode cycles retained active output and a responsive UI, and the test ended safely in Bypass without a crash.
- Task 8 intentionally adds no smoothing, gain compensation, state persistence, or UI changes; those remain later tasks.
- Task 9 keeps gain compensation inside each existing DSP preset and makes every mode's output gain explicit: Bypass 0 dB, Phone +1 dB, Laptop 0 dB, Earbuds -1 dB, Car -1.5 dB, Club -1.5 dB, and Mono 0 dB.
- Phone receives a modest lift to offset its strong band-limiting; Laptop remains at unity because its gentler filtering and mid boost already balance; Earbuds, Car, and Club retain conservative attenuation for their boosted or widened responses; Mono remains at unity because standard averaging was already closest on correlated mix material.
- No filter, EQ, stereo-width, mode-state, or UI behavior changed. No automatic analysis, LUFS/RMS matching, adaptive gain, metering, or smoothing was added.
- Automated tests compare every preset against a deterministic stereo multi-band signal, require the fixed compensation to move level no farther from Bypass than unity output gain, keep every mode within 3 dB of Bypass, and reject clipping. Existing Bypass, frequency-response, stereo-width, and Mono tests continue to pass.
- The Task 9 Release AU built successfully, retained a valid ad-hoc signature, and passed strict `auval` validation, including stereo format, Cocoa editor, and multi-rate render tests.
- Logic Pro 11.2.2 loaded the freshly installed AU after a clean restart. The same stereo mix looped through all seven modes with responsive one-at-a-time selection; output peaked at -14.1 dB with clipping off, and the comparison ended safely in Bypass.
- Task 10 adds a 30 ms linear transition between complete DSP chains. This smooths output gain, stereo width/Mono, HPF and LPF cutoff changes, parametric EQ changes, and transitions to or from Bypass together without changing any settled preset value.
- Three preallocated processing chains allow rapid changes to queue the latest requested configuration while the current transition completes. Filter coefficients are prepared on an inactive chain outside `processBlock()`; the audio callback performs no allocation, locking, object recreation, file I/O, or logging.
- The transition target starts with reset filter state at zero contribution and fades in sample by sample. Mono therefore blends smoothly through stereo width 0.0, while Bypass finishes on the neutral chain and returns to bit-exact transparent audio after 30 ms.
- Automated tests verify bounded sample-to-sample changes for output gain and anti-phase Mono transitions, exact neutral output after returning to Bypass, finite output through 300 rapid mode changes, and all earlier DSP, UI, compensation, and Mono behavior.
- The Task 10 Release AU built successfully and passed strict `auval` validation, including stereo format, Cocoa editor, sliced rendering, and multi-rate render tests.
- After a clean restart, Logic Pro 11.2.2 loaded the freshly installed AU and completed the required Bypass → Phone → Laptop → Car → Club → Mono → Bypass sequence, followed by a 31.4-second rapid stress test with 39 mode changes during continuous playback. The UI stayed responsive, playback and output metering continued with clipping off, and the test returned safely to stopped Bypass without a crash or observed dropout.
- Task 10 adds no UI controls, new modes, persistence, presets, meters, LUFS matching, or Task 11 functionality.
- Task 11 serializes only the active mode in a small JUCE `ValueTree` named `MixCheckerState`, using stable lowercase mode names rather than enum ordinals.
- State restoration validates the data, tree type, property presence, and mode name; missing, malformed, wrong-type, or unknown state falls back safely to transparent Bypass through the existing thread-safe `setMode()` path.
- Automated tests cover round-trip persistence for all seven modes, restored DSP and UI synchronization, and invalid-state fallback. Logic Pro 11.2.2 save/close/reopen tests restored Bypass, Phone, Laptop, Earbuds, Car, Club, and Mono with exactly one matching active button.
- Task 12 completed a clean Release AU build on macOS 15.7.7 arm64. The generated arm64 component has a valid ad-hoc signature, the full automated regression test passed, and strict `auval` validation succeeded for the stereo 2-in/2-out AUv2, Cocoa editor, sliced rendering, and multiple sample rates.
- Logic Pro 11.2.2 loaded the Release AU as a stereo effect. Required mode sequences retained one synchronized active control, both rapid switching passes completed 180 total changes over more than 85 seconds of continuous playback, transport stop/restart succeeded, output meters remained active with clipping off, and the final state returned to Bypass.
- Four human-paced editor close/reopen cycles during playback completed successfully after the state-restore reopen tests, with no plugin crash, audio dropout, or state loss. One synthetic no-delay accessibility automation loop made Logic's UI unresponsive; a host restart recovered the saved project, and the issue did not reproduce at normal interaction speed.
- Final cleanup found no temporary source logging, debug switches, unused development helpers, or dead feature scaffolding to remove. No DSP, UI, or other product feature was added during Task 12.
- Known v0.1 limitations: the device modes are intentionally broad checking approximations; mode gains are fixed rather than loudness-analyzed; the UI and smoothing time are fixed; and distribution packaging, installer, notarization, licensing, and v0.2 features remain out of scope.
- The post-v0.1 silver-hardware UI polish changes only the editor and test expectations; AU-only output, all seven modes, DSP presets, fixed gains, 30 ms smoothing, and the `MixCheckerState` serialization format remain unchanged.
- The fixed editor is now 820 x 360 logical pixels. Its six environment buttons are arranged in one row with separate lamp assemblies, Bypass is isolated at bottom left, ACTIVE is at top right, and STEREO MONITORING is at bottom right to match the approved reference composition.
- The hardware surface is drawn with a graphite outer enclosure, layered metal bevels, corner screws, cached deterministic 2x brushed-aluminium texture, directional highlights, recessed button wells, textured button faces, contact shadows, metal lamp bezels, dark lenses, and amber glow. No reference image or machine-local asset path is used at runtime.
- The cached texture is generated once when the editor is constructed and is never created in `paint()` or the audio callback. Labels remain JUCE-rendered text for Retina clarity.
- The automated regression suite passes with the new 820 x 360 layout. A 1640 x 720 UI snapshot confirmed no visible clipping, stretching, seams, or out-of-bounds controls.
- The Release AU built and passed strict `auval`, including the Cocoa editor and stereo multi-rate render tests. Logic Pro 11.2.2 loaded the installed AU, all seven controls remained mutually exclusive, mode/ACTIVE lamps followed Phone and Bypass correctly, editor close/reopen preserved synchronization, and a CAR save/quit/reopen test restored CAR before the validation project was saved back to Bypass.
- Visual review still requested: the first version uses procedural aluminium and button texture rather than photographed/PBR texture maps, and the system font cannot exactly reproduce the reference lettering. Hover and mouse-down rendering paths are implemented, but an isolated held-mouse screenshot was not captured by the available UI automation.
- No VST3 target, DSP change, state-format change, new control, meter, menu, or product feature was added.
- The compact asset-preparation pass reduces the fixed editor from 820 x 360 to 474 x 208 without scaling the old component wholesale. Main controls use newly chosen 62 x 60 hit areas, 11 px gaps, 10 px mode labels, and a separate 76 x 26 Bypass hit area.
- Both a native 474 x 208 snapshot and a 948 x 416 Retina snapshot confirm that all six mode names are complete and readable, button hit areas do not overlap, Bypass remains clear of the panel edge, and no text or hardware edge is clipped.
- The new Release arm64 AU built successfully, the automated regression suite passed, and strict `auval` validation succeeded. Logic Pro 11.2.2 displayed the compact editor, all seven buttons remained mutually exclusive, and closing/reopening the editor restored synchronized Bypass state.
- The validation project remains saved in Bypass. No DSP, gain, smoothing, state serialization, plugin-format, or VST3 change was made.
- The supplied local design handoff masters (kept outside the Git repository) are now normalized into seven embedded 3x runtime PNGs: a 1422 x 624 faceplate, 186 x 180 mode up/down states, 228 x 78 Bypass up/down states, and 33 x 33 lamp off/on states. Raw masters remain unchanged locally.
- The faceplate uses a two-segment vertical mapping so its supplied divider lands at logical y 159 without stretching the divider thickness across the whole panel. Each control pair uses one shared crop and canvas, keeping the outer frame, lamp ring, and label baseline fixed while state images change.
- JUCE now draws the supplied faceplate, button, and lamp artwork from `juce_add_binary_data`; images decode once when the editor is created and are reused in paint. Only labels and the small hover overlay remain program-drawn.
- Native Bypass and Car screenshots at 474 x 208 and Retina screenshots at 948 x 416 confirm readable labels, correct divider placement, fixed state geometry, no duplicated hardware borders, and the intended mode/ACTIVE lamp behavior.
- On macOS 15.8.1 arm64, the Release AU built successfully, its automated regression suite passed, and strict `auval` succeeded including the Cocoa editor and stereo multi-rate render tests. Logic Pro 11.2.2 loaded the installed AU, all seven buttons remained mutually exclusive, and closing/reopening the editor restored synchronized Bypass state.
- Embedded artwork increased the compressed AU executable from 10,905,728 to 12,194,704 bytes (+1,288,976 bytes, about 1.23 MiB); the component disk usage increased from 10,664 to 11,924 KiB. Final runtime PNGs total about 1.2 MiB on disk.
- Hover and mouse-down code paths are present, but a held-mouse screenshot was not captured by the available UI automation. Final subjective review of the supplied generated texture and small-size typography remains a manual visual approval item.
- This asset pass did not change DSP, gain compensation, smoothing, mode behavior, state serialization, or plugin formats; CMake still builds AU only and no VST3 target was added.
- The follow-up layout polish moves the six `62 x 60` mode controls to y `85`, their lamps to y `72`, Bypass to `(30, 168, 76, 24)`, ACTIVE to `(427, 30)`, and all text to the compact coordinate reference above. Native and 2x snapshots show complete labels, aligned baselines, a fully visible Bypass control, and clear right-side screw spacing.
- The four inconsistent faceplate screws were removed with deterministic masked same-surface repairs. A single extracted 3x screw asset is decoded with the other UI images and drawn unchanged at all four corners, so size, slot direction, and lighting are identical without double images or visible rectangular repair blocks.
- Mode buttons no longer use JUCE's automatic toggle/radio-group behavior. Each click reads the processor's current mode: clicking the active Phone/Laptop/Earbuds/Car/Club/Mono mode requests Bypass, clicking another mode requests that mode, and Bypass always requests Bypass. `updateButtonStates()` still derives every visual state from the processor, preserving one source of truth and the existing serialized mode format.
- Automated behavior coverage now verifies enter-and-repeat-to-Bypass for all six processed modes, direct Car-to-Phone switching, repeated Bypass clicks, exactly-one selection, external processor-to-editor synchronization, settled audio behavior, state restore, and the existing DSP regression suite.
- The Release arm64 AU rebuilt successfully, the full automated test passed, and strict `auval` succeeded for the installed AUv2, including stereo 2-in/2-out, Cocoa editor, sliced rendering, and multi-rate rendering.
- Logic Pro 11.2.2 loaded the newly installed AU and displayed the revised 474 x 208 editor without a crash. The available UI automation lost the plug-in floating-window focus after the first CAR interaction, so the complete six-mode repeat-click matrix was not independently re-verified inside Logic; it remains a manual host interaction check despite passing the processor/editor regression test.
- No DSP preset, gain compensation, 30 ms smoothing, state serialization field, plugin format, or VST3 target changed in this polish pass.
- The safe-area correction repositions the shared corner screws from the editor boundary to the silver inner panel, moves the title/subtitle below the upper bevel, groups the ACTIVE lamp and label more tightly while lowering them from the top edge, and raises Bypass above the lower bevel. Native and Retina Bypass/Car snapshots were regenerated and visually checked for clipping, overlap, and edge clearance.
- This spacing correction changes only `PluginEditor.cpp` layout constants/draw coordinates and generated validation screenshots. DSP presets, gain compensation, smoothing, state serialization, mode behavior, assets, and the AU-only format remain unchanged.
- The corrected Release arm64 AU rebuilt successfully, the full automated regression test passed, and the freshly installed component passed strict `auval`, including its Cocoa editor plus stereo sliced and multi-rate rendering. Logic Pro was not reopened for this coordinate-only correction.
- GitHub preparation adds JUCE's VST3 target alongside the existing AU target. `JUCE_VST3_CAN_REPLACE_VST2=0` explicitly declares that Mix Checker has no legacy VST2 release to replace, avoiding JUCE's automation-ID compatibility guard without changing any current parameter or state identity.
- Release builds now produce both `Mix Checker.component` and `Mix Checker.vst3`; both bundles receive a final ad-hoc macOS signature. The VST3 binary is arm64, its `Info.plist` and generated `moduleinfo.json` are present, and `codesign --verify --deep --strict` passes.
- The freshly installed VST3 was scanned by Cubase Pro 15. Its scanner log reports `Scanning: Mix Checker OK (381 ms)`, and the Cubase cache records the expected v0.1.0 `Fx` audio-module and controller classes. A full VST3 insert/editor/audio/state test was not completed and remains explicitly documented as pending.
- The complete automated regression test still passes after enabling VST3. The freshly rebuilt and installed AU also passes strict `auval`, including its Cocoa editor and stereo sliced/multi-rate rendering; the existing Logic Pro 11.2.2 result remains the host reference validation.
- `README.md` is rewritten for GitHub with the current silver-hardware screenshot, AU/VST3 build and installation instructions, truthful validation boundaries, mode descriptions, architecture notes, and known limitations. The old UI screenshot was replaced by an optimized 948 x 416 capture of the current Car-active interface.
