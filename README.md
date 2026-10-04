# DreamMasterLite

JUCE 8 VST3 effect-rack project inspired by the supplied INSOMNIA.html.

## Rack behavior
- 200 unique effect names and IDs sourced from `INSOMNIA.html` (`FX_DEFS`).
- Each module has an on/off toggle and an amount slider.
- `RANDOM FX PICK (MAX 10)` clears the rack and enables a random 1–10 distinct effects only.
- `ALL OFF` disables every module.
- Green Matrix styling, glowing eyes, and a DreamDaw.com link are included.
- Parameter state is saved with the host session and exposed to automation.
- A conservative output peak ceiling prevents digital overs; this is not a loudness mastering or true-peak limiter.

## Build
Open this project in GitHub and let `.github/workflows/main.yml` run, or configure with CMake and a C++17 compiler. JUCE 8.0.6 is fetched by CMake. The workflow publishes platform build artifacts.

## Important audio note
The 200 names and module states are distinct. The native engine implements multiple DSP families with per-effect variations, but it is not a verified one-to-one native port of all 200 browser-side Web Audio algorithms. Start with effects disabled, enable only what you need, and use subtle amounts on a master bus. Build and audition in your DAW before relying on it for final delivery.

Website: https://dreamdaw.com
