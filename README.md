# KYOTRIPPAH 0.4.2

Native VST3 instrument (**KYOTO**) and effect (**KYOTRIPPAH FX**).

## This revision

### Login
First open asks for DreamShare username and password. The session token and username are saved in the app-data folder (`KYOTRIPPAH/session.json`). Password is never stored and the login fields stay hidden until Log Out.

### DreamShare
Chat uses the live worker actions `chat_list` and `chat_send` on `https://dreamshare-api.keganacummings.workers.dev`. Catalog is a card browser (name, face, author, load), not a combo box. Upload uses `module_publish` / `module_list` / `module_get`.

The Cloudflare Worker entry point is `worker.js`. It imports the single Kyoto module/catalog implementation from `module-rules.js`. Bind `DREAMSHARE_KV` for durable module/catalog storage. The old `WORKER_DREAMSHARE.js` names are retained only as compatibility copies; deploy `worker.js` so there is one canonical entry point.

### FX Builder
Stacks effects into **one** saved effect (`kyoteppah-effect-1`), not a chain. Saved effects can be dropped into the Chain builder as a single linked block. Upload catalogues them on the API as face `effect`.

### Chain builder
Renamed from the free-peg builder. Each ADD NEXT links to the previous step. There is no drag-and-drop. The grid combo has ten placement styles and reflows the series:

1. Series Row
2. Series Column
3. Performance Deck
4. Keys Wall
5. Diagonal Cascade
6. Twin Columns
7. Console Faders
8. Hero Wave
9. Arc Satellites
10. Split Bay

Widget kinds: Dial, Fader, Key, WAV view, Saved effect. LOAD WAV decodes a real file, draws its peaks, and plays it from a Key. Live WAV views read the output scope, not a fake sine.

### Worker / API routing
The authenticated Kyoto route is centralized in one block and covers `module_list`, `module_get`, `module_publish`, `module_delete`, `catalog`, `catalog_delete`, `community`, `community_get`, and `community_publish`. `list_threads` loads its feed independently so it does not depend on a later feed variable. Kyoto effects accept `kyoteppah-effect-1` and preserve up to 16 FX Builder steps.

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Push this folder to GitHub. Actions workflow `Build KYOTRIPPAH` uploads the VST3 zips.

JUCE is AGPLv3 unless you hold a commercial licence.

## UI refinement 0.4.1

- Rebuilt the native theme registry as 42 stable theme presets. Theme IDs are serialization-safe and shared by the editor, machine builder, wave surfaces, and future modular parts.
- Theme selection now drives fonts as well as colours, popup menus, text editors, labels, sliders, FX catalog surfaces, and plugin-view text.
- Reworked text rendering/layout to reduce overdraw and cramped controls; chat text uses explicit theme fonts and increased line spacing.
- Fixed the PLUGIN VIEW hit area: the account/status controls no longer overlap the button.
- Rebuilt the DreamShare home screen around a dashboard hero, responsive live-room/catalog split, cleaner utility controls, and a responsive catalog card grid.
- Reflowed the Chain builder controls into two compact action rows so the UI remains usable at the minimum editor size.
- Human-readable DreamShare utility labels replace raw API action names in the dropdown while preserving the same API actions internally.

## Minimal launch finish (0.4.2)

Base44's `minimal-launch` pass added catalog tabs (Plugins / Effects / My Plugins / Pending), admin approve-deny-tag, and pending uploads. It did not copy that worker surface into the compatibility files. This package finishes that:

- `WORKER_DREAMSHARE.js`, `worker/WORKER_DREAMSHARE.js`, and `JV_WORKER.js` match `worker.js`.
- `JV_MODULE_RULES.js` matches `module-rules.js`.
- Catalog cards render tag arrays, not only string tags.
- CMake project version is 0.4.2, matching this README.
