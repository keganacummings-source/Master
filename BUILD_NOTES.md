# INXOMNIA VST3 — Builder overhaul

This repository is the updated INXOMNIA source based on the supplied Master UI-polish version.

## Implemented

- Product/plugin display name changed to **INXOMNIA** while retaining the existing C++ class names for compatibility.
- Every one of the 200 FX modules now exposes five host-automatable controls:
  - Intensity
  - Tone
  - Motion
  - Mix
  - Shape
- The FX rack cards display all five controls.
- `RANDOM FX` still chooses new effects.
- `RANDOM PICKED VALUES` only changes values on effects that are already ON; it does not pick or unpick effects.
- Favorites remain stored in plugin state and continue to sort/filter correctly.
- FX Builder is now a **Composite Effect Builder**, not a user-facing chain builder.
- A composite can contain multiple different FX modules, each with all five controls.
- Composite effects are saved as lightweight local `.dmefx` text/String code files.
- The first save asks for a folder. The selected folder is remembered and subsequent saves/loads use it automatically.
- Loading reads `.dmefx` files from that folder and activates the component FX with their stored controls.
- File parsing/writing is guarded so malformed custom files do not crash the editor.
- Added the Worker `plugin_capabilities` response required by the native builder (`fxBuilder`, `compositeFx`, `localFxCode`, per-effect control names, themes).
- Added a theme-independent visual glitch pulse at a deliberately lower cadence than the GOONR animation reference. Reduced Motion disables it.
- Credential/settings application name moved to INXOMNIA.
- GitHub Actions target/artifact naming updated to INXOMNIA VST3.

## Custom FX file format

Example:

```text
DMEFX1
name=Dream Rot
kind=composite
parts=2
part=drive|0.72|0.45|0.35|0.78|0.62
part=chorus|0.40|0.68|0.82|0.52|0.44
end
```

The files contain only the recipe; no audio, password, or DreamShare token is written.

## Build verification

The source was structurally checked after the overhaul and the Worker passes `node --check`.
A local CMake configure could not fetch JUCE in this execution environment because outbound GitHub DNS/network access is unavailable. The repository therefore must be confirmed by the included GitHub Actions Windows/macOS runners before claiming a completed binary build.
