# DreamMasterLite

JUCE 8 VST3 effect-rack project inspired by the supplied INSOMNIA.html.

## Rack behavior
- 200 unique effect names and IDs sourced from `INSOMNIA.html` (`FX_DEFS`).
- 200 explicit native DSP recipes built from dedicated delay, reverb, modulation, filter, dynamics, saturation, stereo, and rhythmic-gate algorithms.
- Each module has an on/off toggle and an amount slider; the category menu filters the full rack without removing or renumbering modules.
- `RANDOM FX` chooses 1–10 distinct effects and randomizes their amounts. `XTRMRND` chooses 10–20 distinct effects, inclusive. `DEFINED VALUES` chooses 1–10 effects and assigns repeatable per-effect amounts.
- `ALL OFF` disables every module.
- Authenticated DreamShare users can select themes from the same 16-theme catalog. The selected theme's CSS color variables are applied to the native UI; scene/veil markup is not interpreted by the plugin.
- The DreamShare FX Builder is shown only when the authenticated `plugin_capabilities` response grants `fxBuilder`. Users can add, parameterize, reorder, and save named module chains. DSP processes the active chain in its saved order.
- Named chains and the active processing order live in the APVTS plugin state, alongside host project state. Login passwords and session tokens are never written to plugin state; the session token remains in memory only.
- DreamShare login, session checks, theme selection, online-count polling, and presence heartbeats use HTTPS from the UI thread; no network work runs in the audio callback. The online count is polled from the public Worker feed, and authenticated sessions send presence heartbeats.
- Matrix styling, glowing eyes, and a DreamDaw.com link are included.
- Parameter state is saved with the host session and exposed to automation.
- Effect state/amount controls retain their existing `fxN` and `amtN` host IDs.
- Smoothed enable/amount transitions, bounded feedback, DC blocking, finite-value guards, and a linked sample-peak safety limiter help keep arbitrary and stacked inputs stable. The limiter is not a true-peak or loudness mastering processor.

## Build
Configure with CMake and a C++17 compiler. JUCE 8.0.6 is fetched by CMake. Run the independent DSP checks with `cmake --build build --config Release --target DreamMasterDSPTests` followed by `ctest --test-dir build -C Release --output-on-failure`. CI runs these checks before building the VST3 artifact.

## Audio note
The native rack has a dedicated recipe for every slot; related effects may share stable DSP building blocks but use distinct settings and processing behavior. Automated renders verify ID/name uniqueness, audible and pairwise-distinct outputs, targeted chorus/phaser/tremolo/stutter behavior, mono/stereo operation, finite samples, and peak bounds at multiple sample rates. These checks do not replace critical listening in the target DAW. Start with effects disabled and add modules deliberately, especially on a master bus; the output guard is not a mastering or true-peak certification.

`DEFINED VALUES` uses a stable version-1 mapping from effect index to amount: multiply the index by `2654435761`, add `0x9e3779b9`, xor the result with itself shifted right 16 bits, then compute `0.15 + (value % 701) / 1000`. It therefore produces values from 0.150 through 0.850 and is reproducible across sessions.

FX Builder access is controlled by the authenticated Worker `plugin_capabilities` action. If that optional action is unavailable (for example, before the Worker update is deployed), DreamShare login, theme selection, and public online-count display continue to work, while the builder remains locked rather than assuming access.

Website: https://dreamdaw.com
