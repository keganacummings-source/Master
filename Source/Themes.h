#pragma once

#include <JuceHeader.h>

// Shared theme registry for the native editor and every modular surface.
// Theme ids are stable serialization keys: modules should store themeFamily/theme, not raw colours.
namespace kt {

struct ThemePalette
{
    const char* id;
    const char* name;
    juce::uint32 bg;
    juce::uint32 panel;
    juce::uint32 accent;
    juce::uint32 text;
    juce::uint32 muted;
    juce::uint32 peg;
    juce::uint32 pegHot;
    juce::uint32 knob;
    juce::uint32 border;
    const char* fontFamily = "Segoe UI";
    float textScale = 1.0f;
    float cornerRadius = 8.0f;
    const char* backgroundStyle = "flat";
    const char* bodyMaterial = "panel";
    const char* borderStyle = "rounded";
    const char* screenStyle = "crt";
    const char* knobStyle = "soft";
    const char* ledStyle = "dot";
    const char* textStyle = "clean";
    const char* panelTexture = "none";
    const char* waveformStyle = "line";
    const char* meterStyle = "bar";
    const char* highlightStyle = "glow";
};

inline constexpr ThemePalette kThemes[] = {
    { "trippah", "Trippah", 0xff0e0c14, 0xff1a1624, 0xffc77dff, 0xfff0e6ff, 0xff8a7aa8, 0xff3a2e4a, 0xffe0a0ff, 0xff2a2238, 0xff4a3a5e, "Segoe UI", 1.0f, 8.0f },
    { "goonr", "Goonr", 0xff0a1210, 0xff121c18, 0xff3dffb0, 0xffe0fff0, 0xff6a9a80, 0xff1e3a30, 0xff80ffc0, 0xff1a2a22, 0xff2a4a3a, "Trebuchet MS", 1.0f, 8.0f },
    { "abyss", "Abyss", 0xff05080f, 0xff0c121c, 0xff3a8cff, 0xffd0e4ff, 0xff5a7aaa, 0xff1a2838, 0xff80b0ff, 0xff121a28, 0xff2a3a50, "Arial", 1.0f, 8.0f },
    { "amber", "Amber", 0xff140e08, 0xff221810, 0xffff9a3c, 0xfffff0e0, 0xffa08060, 0xff3a2a18, 0xffffc080, 0xff2a1e12, 0xff4a3a20, "Segoe UI", 1.0f, 8.0f },
    { "bloodmoon", "Bloodmoon", 0xff120808, 0xff1e1010, 0xffff4060, 0xffffe0e4, 0xffa06070, 0xff3a1a20, 0xffff80a0, 0xff2a1418, 0xff4a2028, "Arial", 1.0f, 8.0f },
    { "cobalt", "Cobalt", 0xff080c14, 0xff10182a, 0xff4080ff, 0xffe0ecff, 0xff6080b0, 0xff1a2840, 0xff80b0ff, 0xff121c30, 0xff2a3a58, "Tahoma", 1.0f, 8.0f },
    { "ember", "Ember", 0xff120a06, 0xff1e140c, 0xffff6030, 0xffffece0, 0xffa07050, 0xff3a2418, 0xffffa080, 0xff2a1a10, 0xff4a3020, "Trebuchet MS", 1.0f, 8.0f },
    { "fog", "Fog", 0xff101418, 0xff1a2028, 0xffa0c0d0, 0xffe8f0f4, 0xff708090, 0xff2a3038, 0xffc0d8e0, 0xff1e242c, 0xff384048, "Verdana", 1.0f, 8.0f },
    { "graphite", "Graphite", 0xff101010, 0xff1a1a1a, 0xffb0b0b0, 0xfff0f0f0, 0xff707070, 0xff2a2a2a, 0xffd0d0d0, 0xff1e1e1e, 0xff3a3a3a, "Segoe UI", 1.0f, 8.0f },
    { "honey", "Honey", 0xff14100a, 0xff221c12, 0xffffc040, 0xfffff8e0, 0xffa09050, 0xff3a3018, 0xffffe080, 0xff2a2210, 0xff4a3a20, "Segoe UI", 1.0f, 8.0f },
    { "ice", "Ice", 0xff0a1014, 0xff121c24, 0xff80d0ff, 0xffe8f8ff, 0xff60a0c0, 0xff1a3040, 0xffb0e8ff, 0xff142028, 0xff2a4050, "Arial", 1.0f, 8.0f },
    { "ink", "Ink", 0xff08080c, 0xff101018, 0xff6080ff, 0xffe0e4ff, 0xff5060a0, 0xff1a1a30, 0xffa0b0ff, 0xff121220, 0xff2a2a48, "Segoe UI", 1.0f, 8.0f },
    { "lagoon", "Lagoon", 0xff061210, 0xff0c1e1a, 0xff30d0a0, 0xffe0fff4, 0xff50a080, 0xff1a3a30, 0xff80ffc0, 0xff102820, 0xff2a4a3a, "Trebuchet MS", 1.0f, 8.0f },
    { "lilac", "Lilac", 0xff100e14, 0xff1a1622, 0xffc080ff, 0xfff4e8ff, 0xff8070a0, 0xff2a2438, 0xffe0c0ff, 0xff1e1a2a, 0xff3a3048, "Segoe UI", 1.0f, 8.0f },
    { "mint", "Mint", 0xff0a1210, 0xff121c18, 0xff40e0a0, 0xffe0fff0, 0xff60a080, 0xff1a3a2a, 0xff80ffc0, 0xff12281e, 0xff2a4a38, "Segoe UI", 1.0f, 8.0f },
    { "neon", "Neon", 0xff08080c, 0xff101018, 0xff00ffc0, 0xffe0fff8, 0xff40a080, 0xff1a2a28, 0xff80ffe0, 0xff121a1a, 0xff2a3a38, "Segoe UI", 1.0f, 8.0f },
    { "pine", "Pine", 0xff0a100c, 0xff121a14, 0xff40c060, 0xffe0ffe8, 0xff509060, 0xff1a3020, 0xff80e0a0, 0xff122018, 0xff2a4030, "Verdana", 1.0f, 8.0f },
    { "plum", "Plum", 0xff10080e, 0xff1a1018, 0xffc040a0, 0xffffe0f0, 0xffa06080, 0xff3a1a30, 0xffff80d0, 0xff24121e, 0xff4a2840, "Trebuchet MS", 1.0f, 8.0f },
    { "rust", "Rust", 0xff120c08, 0xff1e1610, 0xffe07030, 0xfffff0e0, 0xffa07050, 0xff3a2818, 0xffffa060, 0xff2a1c12, 0xff4a3020, "Segoe UI", 1.0f, 8.0f },
    { "steel", "Steel", 0xff0c1014, 0xff141c24, 0xff80a0c0, 0xffe8f0f8, 0xff6080a0, 0xff1a2838, 0xffb0c8e0, 0xff121a24, 0xff2a3a4a, "Arial", 1.0f, 8.0f },
    { "void", "Void", 0xff060608, 0xff0c0c10, 0xffa080ff, 0xfff0e8ff, 0xff7060a0, 0xff1a1830, 0xffc0a0ff, 0xff10101a, 0xff2a2848, "Segoe UI", 1.0f, 8.0f },
    { "wine", "Wine", 0xff10080a, 0xff1a1014, 0xffc04060, 0xffffe0e8, 0xffa06070, 0xff3a1a24, 0xffff80a0, 0xff241218, 0xff4a2030, "Segoe UI", 1.0f, 8.0f },
    { "default", "Default", 0xff12100e, 0xff1c1814, 0xffefe6d6, 0xffefe6d6, 0xff8a8070, 0xff3a3128, 0xffffe0a0, 0xff2a241e, 0xff4a4038, "Segoe UI", 1.0f, 8.0f },
    { "sakura", "Sakura", 0xff140a10, 0xff21111b, 0xffff79b0, 0xffffeaf3, 0xffa66f86, 0xff3a1d2b, 0xffffa8c9, 0xff2c1824, 0xff523044, "Segoe UI", 1.0f, 8.0f },
    { "moss", "Moss", 0xff0b1109, 0xff151d11, 0xff9acb52, 0xffefffe1, 0xff718d55, 0xff27361d, 0xffc0e982, 0xff1d2917, 0xff3d512e, "Verdana", 1.0f, 8.0f },
    { "ultraviolet", "Ultraviolet", 0xff090615, 0xff161026, 0xff9b6cff, 0xfff0eaff, 0xff7763a8, 0xff2b1d4a, 0xffc5aaff, 0xff21163a, 0xff45336a, "Tahoma", 1.0f, 8.0f },
    { "vapor", "Vapor", 0xff080d16, 0xff111d2c, 0xff59d9ff, 0xffe8fbff, 0xff6397aa, 0xff1d3541, 0xff9ceaff, 0xff172936, 0xff315363, "Trebuchet MS", 1.0f, 8.0f },
    { "arctic", "Arctic", 0xff071116, 0xff10202a, 0xff9be7ff, 0xffe9fbff, 0xff6d9da9, 0xff1b3942, 0xffc5f2ff, 0xff142b35, 0xff35525e, "Arial", 1.0f, 8.0f },
    { "sunset", "Sunset", 0xff160b0a, 0xff261512, 0xffff7657, 0xffffeee7, 0xffa67868, 0xff40251e, 0xffffaa90, 0xff302019, 0xff5a382f, "Segoe UI", 1.0f, 8.0f },
    { "ocean", "Ocean", 0xff061018, 0xff0c1b29, 0xff2ec4ff, 0xffe2f6ff, 0xff5f8ea8, 0xff183646, 0xff7ee0ff, 0xff102938, 0xff2b4b5f, "Segoe UI", 1.0f, 8.0f },
    { "carbon", "Carbon", 0xff090a0b, 0xff141618, 0xffd2d6da, 0xfff3f5f6, 0xff7c8389, 0xff25292d, 0xffedf0f2, 0xff1b1f22, 0xff3a4045, "Arial", 1.0f, 8.0f },
    { "rose", "Rose", 0xff130a0f, 0xff21121a, 0xfff18ab0, 0xffffedf4, 0xffa7798b, 0xff3a2230, 0xffffb0c9, 0xff2c1822, 0xff513140, "Trebuchet MS", 1.0f, 8.0f },
    { "mono", "Mono", 0xff090b0d, 0xff12161a, 0xffb8ff72, 0xffeaffdc, 0xff77965f, 0xff24351c, 0xffd0ff9b, 0xff1b2817, 0xff3b5130, "Segoe UI", 1.0f, 8.0f },
    { "terminal", "Terminal", 0xff020705, 0xff07110c, 0xff38ff88, 0xffd6ffe6, 0xff4c9f6a, 0xff12301d, 0xff8dffb5, 0xff0d2416, 0xff255536, "Segoe UI", 1.0f, 8.0f },
    { "candy", "Candy", 0xff120b14, 0xff211425, 0xffff72d0, 0xffffeffa, 0xffa9789e, 0xff3b2340, 0xffffa6df, 0xff2c1a31, 0xff52364e, "Trebuchet MS", 1.0f, 8.0f },
    { "solar", "Solar", 0xff130f06, 0xff231c0c, 0xffffd34d, 0xfffff7d0, 0xffa99a6a, 0xff3d3314, 0xffffe28a, 0xff2d2510, 0xff56481e, "Verdana", 1.0f, 8.0f },
    { "forest", "Forest", 0xff07100a, 0xff0e1a11, 0xff4fe07c, 0xffe3ffe9, 0xff5d936d, 0xff173323, 0xff8bf0a6, 0xff11251a, 0xff2b4a35, "Segoe UI", 1.0f, 8.0f },
    { "midnight", "Midnight", 0xff05070d, 0xff0c1020, 0xff7d8dff, 0xffe8ebff, 0xff626d9d, 0xff1b2340, 0xffaab4ff, 0xff121a31, 0xff303c62, "Segoe UI", 1.0f, 8.0f },
    { "orchid", "Orchid", 0xff100812, 0xff1c1020, 0xffe08cff, 0xffffebff, 0xff966fa4, 0xff35203f, 0xfff0b0ff, 0xff28172f, 0xff503657, "Segoe UI", 1.0f, 8.0f },
    { "copper", "Copper", 0xff120c08, 0xff21150f, 0xffe19a63, 0xfffff0df, 0xff9b7659, 0xff3b281b, 0xffffbd8a, 0xff2d1d14, 0xff533d2d, "Tahoma", 1.0f, 8.0f },
    { "ghost", "Ghost", 0xff0b0d0f, 0xff171a1d, 0xffdbe4ea, 0xfff7fbff, 0xff85919a, 0xff2b3339, 0xffffffff, 0xff20262b, 0xff414a51, "Arial", 1.0f, 8.0f },
    { "arcade", "Arcade", 0xff0a0712, 0xff171024, 0xffff4dd8, 0xfff5e9ff, 0xffa45f98, 0xff321d42, 0xffff8ee6, 0xff21152d, 0xff4a2f58, "Trebuchet MS", 1.0f, 8.0f },
};

inline constexpr int kThemeCount = sizeof(kThemes) / sizeof(kThemes[0]);
static_assert(kThemeCount == 42, "KYOTRIPPAH theme registry must retain the 42 stable presets");

inline const ThemePalette& themeById(const juce::String& id)
{
    for (int i = 0; i < kThemeCount; ++i)
        if (id.equalsIgnoreCase(kThemes[i].id))
            return kThemes[i];
    return kThemes[kThemeCount - 1];
}

inline juce::Colour c(juce::uint32 argb) { return juce::Colour(argb); }

inline juce::Font font(const ThemePalette& t, float size, bool bold = false)
{
    return juce::Font(juce::FontOptions(t.fontFamily, size * t.textScale, bold ? juce::Font::bold : juce::Font::plain));
}

// ---- DreamShare readability scale -------------------------------------------------------
// DreamShare (chat, threads, catalog cards) uses dsFont() so text can be made larger or smaller
// without touching the builders. The value is remembered between sessions.
inline float& dsScale()
{
    static float s = 1.12f;
    return s;
}

inline juce::File dsScaleFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("KyotoSpxrit").getChildFile("dreamshare_ui_scale.txt");
}

inline void loadDsScale()
{
    auto f = dsScaleFile();
    if (f.existsAsFile())
    {
        const float v = f.loadFileAsString().trim().getFloatValue();
        if (v >= 0.8f && v <= 1.6f) dsScale() = v;
    }
}

inline void saveDsScale()
{
    auto f = dsScaleFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText(juce::String(dsScale(), 2));
}

inline juce::Font dsFont(const ThemePalette& t, float size, bool bold = false)
{
    return font(t, size * dsScale(), bold);
}

} // namespace kt
