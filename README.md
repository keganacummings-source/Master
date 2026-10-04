# DreamMasterLite VST3

A JUCE 8 VST3 effect rack inspired by the supplied INSOMNIA HTML reference, with 200 uniquely named modules, a Matrix-green interface, eye graphics, and a DreamDaw.com link.

## Interface updates

- Each effect is displayed in its own bordered card.
- Each card has an enable switch, an ASCII `FAV` toggle, and three parameter controls with family-appropriate labels. Favorited effects move to the top and sort alphabetically by name; other effects keep their original order.
- Mouse-wheel scrolling over sliders is disabled so scrolling the long rack does not change parameter values.
- Favorite stars are stored in the user's local DreamMasterLite settings file and persist across separate plugin instances and FL Studio restarts on that computer. They are not intended to sync between computers.
- Random FX Pick enables a randomly chosen 1–10 distinct effects and clears the other enable switches. All Off disables all effects.
- Parameter values and enabled states are stored in the DAW's plugin state.

## Build with GitHub Actions

1. Upload the contents of this project into your repository, preserving `.github/workflows/build.yml` and `Source/`.
2. Commit and push to `main` or `master`.
3. Open the repository's **Actions** tab and select **DreamMasterLite VST3 Build**.
4. After a successful run, download the platform-specific VST3 artifact from the run's **Artifacts** section.

The workflow builds Windows, macOS, and Linux artifacts. The build must succeed before the plugin can be considered build-verified.

## Audio note

The effect rack currently routes the named modules through shared DSP families (filtering, gain, saturation, limiting, delay, stereo width, modulation, lo-fi, gating, and color shaping). The three controls are mapped to parameters for those families; this is not a one-to-one recreation of all 200 original browser-based effects. Always audition changes at conservative levels, especially on a master bus. The final peak guard is a safety clamp, not a true-peak limiter or loudness mastering processor.
