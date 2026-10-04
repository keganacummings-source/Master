# DreamMasterLite

JUCE 8 VST3 effect-rack project inspired by the supplied INSOMNIA.html.

## Rack behavior
- 200 unique effect names and IDs sourced from `INSOMNIA.html` (`FX_DEFS`).
- 200 explicit native DSP recipes built from dedicated delay, reverb, modulation, filter, dynamics, saturation, stereo, and rhythmic-gate algorithms.
- Each module has an on/off toggle and an amount slider.
- `RANDOM FX PICK (MAX 10)` clears the rack and enables a random 1–10 distinct effects only.
- `ALL OFF` disables every module.
- Green Matrix styling, glowing eyes, and a DreamDaw.com link are included.
- Parameter state is saved with the host session and exposed to automation.
- Effect state/amount controls retain their existing `fxN` and `amtN` host IDs.
- Smoothed enable/amount transitions, bounded feedback, DC blocking, finite-value guards, and a linked sample-peak safety limiter help keep arbitrary and stacked inputs stable. The limiter is not a true-peak or loudness mastering processor.

## Build
Configure with CMake and a C++17 compiler. JUCE 8.0.6 is fetched by CMake. Run the independent DSP checks with `cmake --build build --config Release --target DreamMasterDSPTests` followed by `ctest --test-dir build -C Release --output-on-failure`. CI runs these checks before building the VST3 artifact.

## Audio note
The native rack has a dedicated recipe for every slot; related effects may share stable DSP building blocks but use distinct settings and processing behavior. Automated renders verify ID/name uniqueness, audible and pairwise-distinct outputs, targeted chorus/phaser/tremolo/stutter behavior, mono/stereo operation, finite samples, and peak bounds at multiple sample rates. These checks do not replace critical listening in the target DAW. Start with effects disabled and add modules deliberately, especially on a master bus; the output guard is not a mastering or true-peak certification.

Website: https://dreamdaw.com
