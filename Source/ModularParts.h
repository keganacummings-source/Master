#pragma once
#include "Themes.h"

// ============================================================================
//  MODULAR PARTS - COSMETIC ONLY
//  Every placed part can carry a SKIN (visual family) and a LOOK note. Modules have NO effect on sound or
//  control behaviour: only effects the user places affect audio. Data lives here; the editor stores "skin" + "quirk"
//  on each placed widget ValueTree node, so skins/quirks serialize with builds
//  for free. Future batches just add rows to the tables below.
// ============================================================================

namespace kt
{

// ---- Skins: "stock" follows the active theme; the rest recolour the part so
// machines can mix families. accentHex/panelHex null = inherit the theme.
struct PartSkin { const char* id; const char* name; const char* accentHex; const char* panelHex; };

inline constexpr PartSkin kPartSkins[] = {
    { "stock",  "Stock",  nullptr,   nullptr   },
    { "copper", "Copper", "#b87333", "#241811" },
    { "mint",   "Mint",   "#69d1a0", "#10241c" },
    { "violet", "Violet", "#9a6cf0", "#191330" },
    { "amber",  "Amber",  "#e0a020", "#291f0a" },
    { "ice",    "Ice",    "#7ec8f0", "#0e2130" }
};
inline constexpr int kPartSkinCount = 6;

inline const PartSkin* partSkinById(const juce::String& id)
{
    for (int i = 0; i < kPartSkinCount; ++i)
        if (id == kPartSkins[i].id) return &kPartSkins[i];
    return nullptr;
}

// ---- First batch of modular pieces, each with a special quirk.
// Implemented quirks (batch 1): snap = quantize to 8 steps, lens = 4x finer
// drag. flip/warp/pulse/ghost ship as data + skin now; behaviour lands next batch.
struct ModPiece { const char* id; const char* name; const char* quirk; const char* skin; };

inline constexpr ModPiece kModPieces[] = {
    { "snap",  "Snap Dial",    "Cosmetic: mint stepped-dial look",      "mint"   },
    { "lens",  "Lens Knob",    "Cosmetic: copper precision-knob look", "copper" },
    { "flip",  "Invert Fader", "Cosmetic: violet fader look",    "violet" },
    { "warp",  "Warp Dial",    "Cosmetic: amber dial look",            "amber"  },
    { "pulse", "Pulse Key",    "Cosmetic: ice key look",                 "ice"    },
    { "ghost", "Ghost Screen", "Cosmetic: stock screen look",    "stock"  }
};
inline constexpr int kModPieceCount = 6;

// ---- Placeable control params: the FX options a dial/fader can be bound to
// when it is placed. Token matches the APVTS suffixes (s01amt, s01mot ...).
struct ControlParam { const char* token; const char* name; };

inline constexpr ControlParam kControlParams[] = {
    { "amt",  "Amount" },
    { "tone", "Tone"   },
    { "mot",  "Motion" },
    { "mix",  "Mix %"  },
    { "shp",  "Shape"  }
};
inline constexpr int kControlParamCount = 5;

} // namespace kt
