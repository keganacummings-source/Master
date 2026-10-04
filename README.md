# DreamMasterLite VST3

A JUCE 8 VST3 effect rack inspired by the supplied INSOMNIA HTML reference, with 200 uniquely named modules, a Matrix-green interface, eye graphics, and a DreamDaw.com link.

## Interface updates

- Each effect is displayed in its own bordered card.
- Each card has an enable switch, a favorite star, and three parameter controls with family-appropriate labels.
- Mouse-wheel scrolling over sliders is disabled so scrolling the long rack does not change parameter values.
- Favorite stars are stored in the user's local DreamMasterLite settings file and persist across separate plugin instances and FL Studio restarts on that computer. They are not intended to sync between computers.
- Random FX Pick enables a randomly chosen 1–10 distinct effects and clears the other enable switches. All Off disables all effects.
- Parameter values and enabled states are stored in the DAW's plugin state.

## Build

Open this project in GitHub and let `.github/workflows/main.yml` run, or configure with CMake and a C++17 compiler. JUCE 8.0.6 is fetched by CMake. The workflow publishes platform build artifacts.

## Audio note

The effect rack currently routes the named modules through shared DSP families (filtering, gain, saturation, limiting, delay, stereo width, modulation, lo-fi, gating, and color shaping). The three controls are mapped to parameters for those families; this is not a one-to-one recreation of all 200 original browser-based effects. Always audition changes at conservative levels, especially on a master bus. The final peak guard is a safety clamp, not a true-peak limiter or loudness mastering processor.
