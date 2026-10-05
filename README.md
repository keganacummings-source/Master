# INXOMNIA

JUCE 8 VST3 effect-rack project inspired by the supplied INSOMNIA.html.

## Rack behavior
- 200 unique effect names and IDs sourced from `INSOMNIA.html` (`FX_DEFS`).
- 200 explicit native DSP recipes built from dedicated delay, reverb, modulation, filter, dynamics, saturation, stereo, and rhythmic-gate algorithms.
- Each module has an on/off toggle and an amount slider; custom category chips and search filter the rack without removing or renumbering modules. Filtered cards are created on demand, and stable-ID favorites are saved with the plugin state and sorted first.
- `RANDOM FX` chooses 1–10 distinct effects with stable per-effect levels; `RANDOM VALUES` chooses 1–10 with genuinely random levels; `XTRMRND` chooses 10–20 distinct effects with random levels, inclusive.
- `ALL OFF` disables every module.
- Authenticated DreamShare users can select themes from the same 16-theme catalog. The selected theme's CSS color variables are applied to the native UI; GOONR and Trippah receive native animations, while scene/veil markup is not interpreted by the plugin.
- The DreamShare FX Builder is shown only when the authenticated `plugin_capabilities` response grants `fxBuilder`. Its two-column workspace separates the effect library from the ordered chain; category chips/search filter the library, selecting a module sets the new step's amount, and selecting a chain row edits that step. Reorder/remove and named-chain save/load controls stay beside the chain.
- Current effects each have one functional amount parameter; no unconnected extra sliders are presented.
- Named chains, active processing order, and stable-ID favorites live in the APVTS plugin state, alongside host project state. Login passwords and session tokens are never written to plugin state; Windows Credential Manager securely retains the session token between opens, while other platforms use memory-only sessions.
- DreamShare login, session checks, theme selection, online-count polling, and presence heartbeats use asynchronous HTTPS from a worker thread; no network work runs in the audio callback or blocks the UI.
- On Windows, the `session` check persists any refreshed token in Credential Manager. Unsupported platforms use memory-only sessions.
- Matrix styling, glowing eyes, and a DreamDaw.com link are included.
- The native UI starts with a complete red default palette before login. Favorite modules show a persistent `FAV *` marker and gold card outline independent of selection; all interface labels use plain ASCII for consistent Windows rendering.
- Parameter state is saved with the host session and exposed to automation.
- Effect state/amount controls retain their existing `fxN` and `amtN` host IDs.
- Smoothed enable/amount transitions, bounded feedback, DC blocking, finite-value guards, and a linked sample-peak safety limiter help keep arbitrary and stacked inputs stable. The limiter is not a true-peak or loudness mastering processor.

## Build
Configure with CMake and a C++17 compiler. JUCE 8.0.6 is fetched by CMake. Run the independent DSP checks with `cmake --build build --config Release --target DreamMasterDSPTests` followed by `ctest --test-dir build -C Release --output-on-failure`. CI runs these checks before building the VST3 artifact.

## Audio note
The native rack has a dedicated recipe for every slot; related effects may share stable DSP building blocks but use distinct settings and processing behavior. Automated renders verify ID/name uniqueness, audible and pairwise-distinct outputs, targeted chorus/phaser/tremolo/stutter behavior, mono/stereo operation, finite samples, and peak bounds at multiple sample rates. These checks do not replace critical listening in the target DAW. Start with effects disabled and add modules deliberately, especially on a master bus; the output guard is not a mastering or true-peak certification.

The stable version-1 per-effect amount mapping multiplies the effect index by `2654435761`, adds `0x9e3779b9`, XORs the result with itself shifted right 16 bits, then computes `0.15 + (value % 701) / 1000`. It produces values from 0.150 through 0.850 and is reproducible across sessions.

FX Builder access is controlled by the authenticated Worker `plugin_capabilities` action. If that optional action is unavailable (for example, before the Worker update is deployed), DreamShare login, theme selection, and public online-count display continue to work, while the builder remains locked rather than assuming access.

Website: https://dreamdaw.com
