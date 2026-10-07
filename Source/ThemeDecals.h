#pragma once
#include <JuceHeader.h>
#include "Themes.h"

// ============================================================================
//  THEME DECALS
//  Every theme gets its OWN physical stickers/engravings on the plugin case so two
//  plugins with different themes look like different objects, not recolours.
//  Drawn under the bays, clipped to the case silhouette. Pure cosmetics.
// ============================================================================
namespace pb
{
enum DecalFamily { DStars, DFlames, DWaves, DPetals, DCircuit, DHazard, DLeaves, DSnow, DArcade, DBolts, DMoon, DPlates, DSunGrid, kDecalFamilies };

inline int decalFamilyFor(const juce::String& id)
{
    struct M { const char* id; int fam; };
    static const M map[] = {
        { "trippah", DMoon }, { "goonr", DBolts }, { "abyss", DWaves }, { "amber", DHazard }, { "bloodmoon", DFlames },
        { "cobalt", DWaves }, { "ember", DFlames }, { "fog", DSnow }, { "graphite", DPlates }, { "honey", DHazard },
        { "ice", DSnow }, { "ink", DMoon }, { "lagoon", DWaves }, { "lilac", DPetals }, { "mint", DLeaves },
        { "neon", DCircuit }, { "pine", DLeaves }, { "plum", DPetals }, { "rust", DPlates }, { "steel", DPlates },
        { "void", DStars }, { "wine", DBolts }, { "sakura", DPetals }, { "moss", DLeaves }, { "ultraviolet", DStars },
        { "vapor", DSunGrid }, { "arctic", DSnow }, { "sunset", DSunGrid }, { "ocean", DWaves }, { "carbon", DCircuit },
        { "rose", DPetals }, { "mono", DCircuit }, { "terminal", DCircuit }, { "candy", DArcade }, { "solar", DHazard },
        { "forest", DLeaves }, { "midnight", DStars }, { "orchid", DPetals }, { "copper", DPlates }, { "ghost", DSnow },
        { "arcade", DArcade }
    };
    for (const auto& m : map) if (id.equalsIgnoreCase(m.id)) return m.fam;
    return (int) (std::abs(id.hashCode()) % kDecalFamilies);
}

inline const char* decalFamilyName(int f)
{
    static const char* n[] = { "STARFIELD", "FLAME WRAP", "TIDE LINES", "PETAL BLOOM", "CIRCUIT TRACE", "HAZARD STRIPE", "LEAF VINE", "SNOWFALL", "ARCADE PIXEL", "VOLT BOLTS", "MOON PHASE", "RIVET PLATE", "SUN GRID" };
    return n[juce::jlimit(0, (int) kDecalFamilies - 1, f)];
}

inline void paintThemeDecals(juce::Graphics& g, juce::Rectangle<float> r, const kt::ThemePalette& theme, int shellSeed)
{
    const int fam = decalFamilyFor(theme.id);
    const auto acc = kt::c(theme.accent);
    const auto hot = kt::c(theme.pegHot);
    const auto dim = kt::c(theme.muted);
    juce::Random rng(juce::String(theme.id).hashCode() * 31 + shellSeed);
    const float W = r.getWidth(), H = r.getHeight();
    auto rx = [&] { return r.getX() + 12.f + rng.nextFloat() * juce::jmax(1.f, W - 24.f); };
    auto ry = [&] { return r.getY() + 12.f + rng.nextFloat() * juce::jmax(1.f, H - 24.f); };

    auto star = [&](float x, float y, float s, float a) {
        juce::Path p; p.startNewSubPath(x, y - s); p.quadraticTo(x, y, x + s, y); p.quadraticTo(x, y, x, y + s); p.quadraticTo(x, y, x - s, y); p.quadraticTo(x, y, x, y - s);
        g.setColour(hot.withAlpha(a)); g.fillPath(p);
    };
    auto petal = [&](float x, float y, float s, float a) {
        g.setColour(acc.withAlpha(a));
        for (int i = 0; i < 5; ++i) {
            const float ang = juce::MathConstants<float>::twoPi * (float) i / 5.f;
            g.fillEllipse(x + std::cos(ang) * s * 0.55f - s * 0.36f, y + std::sin(ang) * s * 0.55f - s * 0.36f, s * 0.72f, s * 0.72f);
        }
        g.setColour(hot.withAlpha(a + 0.15f)); g.fillEllipse(x - s * 0.18f, y - s * 0.18f, s * 0.36f, s * 0.36f);
    };

    switch (fam)
    {
        case DStars:
            for (int i = 0; i < 26; ++i) star(rx(), ry(), 2.f + rng.nextFloat() * 5.f, 0.12f + rng.nextFloat() * 0.25f);
            g.setColour(acc.withAlpha(0.20f)); g.drawEllipse(r.getRight() - 80.f, r.getY() + 12.f, 64.f, 64.f, 1.2f);
            star(r.getRight() - 48.f, r.getY() + 44.f, 12.f, 0.5f);
            break;
        case DFlames:
        {
            for (int i = 0; i < 9; ++i) {
                const float x0 = r.getX() + W * ((float) i + 0.3f) / 9.f, w = W / 9.f, h = 22.f + rng.nextFloat() * 36.f;
                juce::Path f; f.startNewSubPath(x0, r.getBottom()); f.quadraticTo(x0 + w * 0.1f, r.getBottom() - h * 0.6f, x0 + w * 0.5f, r.getBottom() - h);
                f.quadraticTo(x0 + w * 0.9f, r.getBottom() - h * 0.5f, x0 + w, r.getBottom()); f.closeSubPath();
                g.setColour(acc.withAlpha(0.22f)); g.fillPath(f);
                g.setColour(hot.withAlpha(0.20f)); g.fillPath(f, juce::AffineTransform::scale(0.55f, 0.6f, x0 + w * 0.5f, r.getBottom()));
            }
            break;
        }
        case DWaves:
            for (int k = 0; k < 4; ++k) {
                juce::Path p; const float y0 = r.getBottom() - 14.f - (float) k * 11.f;
                for (float x = r.getX(); x <= r.getRight(); x += 4.f) { const float y = y0 + std::sin((x + (float) k * 23.f) * 0.045f) * 5.f; if (x == r.getX()) p.startNewSubPath(x, y); else p.lineTo(x, y); }
                g.setColour(acc.withAlpha(0.28f - (float) k * 0.05f)); g.strokePath(p, juce::PathStrokeType(2.f));
            }
            g.setColour(hot.withAlpha(0.2f)); for (int i = 0; i < 6; ++i) g.drawEllipse(rx(), ry(), 7.f, 7.f, 1.f);
            break;
        case DPetals:
            for (int i = 0; i < 9; ++i) petal(rx(), ry(), 7.f + rng.nextFloat() * 10.f, 0.12f + rng.nextFloat() * 0.14f);
            petal(r.getRight() - 40.f, r.getY() + 40.f, 26.f, 0.35f);
            break;
        case DCircuit:
        {
            g.setColour(acc.withAlpha(0.22f));
            for (int i = 0; i < 9; ++i) {
                juce::Path t; float x = rx(), y = ry(); t.startNewSubPath(x, y);
                for (int s = 0; s < 3; ++s) { if (s % 2 == 0) x += (rng.nextBool() ? 1.f : -1.f) * (20.f + rng.nextFloat() * 50.f); else y += (rng.nextBool() ? 1.f : -1.f) * (14.f + rng.nextFloat() * 40.f); t.lineTo(x, y); }
                g.strokePath(t, juce::PathStrokeType(1.4f)); g.fillEllipse(x - 3.f, y - 3.f, 6.f, 6.f);
            }
            g.setColour(acc.withAlpha(0.35f)); g.drawRect(r.getRight() - 46.f, r.getBottom() - 46.f, 30.f, 30.f, 1.6f);
            for (int i = 0; i < 4; ++i) { g.drawLine(r.getRight() - 40.f + (float) i * 7.f, r.getBottom() - 52.f, r.getRight() - 40.f + (float) i * 7.f, r.getBottom() - 46.f, 1.4f); }
            break;
        }
        case DHazard:
        {
            const juce::Rectangle<float> strip(r.getX(), r.getBottom() - 16.f, W, 10.f);
            g.saveState(); g.reduceClipRegion(strip.toNearestInt());
            for (float x = strip.getX() - 10.f; x < strip.getRight(); x += 16.f) { juce::Path s; s.addQuadrilateral(x, strip.getBottom(), x + 8.f, strip.getBottom(), x + 18.f, strip.getY(), x + 10.f, strip.getY()); g.setColour(acc.withAlpha(0.4f)); g.fillPath(s); }
            g.restoreState();
            g.setColour(acc.withAlpha(0.3f)); g.drawRoundedRectangle(r.getX() + 12.f, r.getY() + 12.f, 52.f, 20.f, 3.f, 1.6f);
            g.setFont(kt::font(theme, 10.f, true)); g.drawText("CAUTION", juce::Rectangle<float>(r.getX() + 12.f, r.getY() + 12.f, 52.f, 20.f), juce::Justification::centred);
            break;
        }
        case DLeaves:
            for (int i = 0; i < 12; ++i) {
                const float x = rx(), y = ry(), s = 8.f + rng.nextFloat() * 12.f, a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                juce::Path l; l.startNewSubPath(0.f, -s); l.quadraticTo(s * 0.9f, 0.f, 0.f, s); l.quadraticTo(-s * 0.9f, 0.f, 0.f, -s);
                g.setColour(acc.withAlpha(0.15f + rng.nextFloat() * 0.12f)); g.fillPath(l, juce::AffineTransform::rotation(a).translated(x, y));
            }
            { juce::Path v; v.startNewSubPath(r.getX() + 6.f, r.getBottom() - 8.f); v.cubicTo(r.getX() + W * 0.3f, r.getBottom() - 40.f, r.getX() + W * 0.6f, r.getBottom() + 10.f, r.getRight() - 6.f, r.getBottom() - 24.f); g.setColour(acc.withAlpha(0.3f)); g.strokePath(v, juce::PathStrokeType(2.f)); }
            break;
        case DSnow:
            for (int i = 0; i < 14; ++i) {
                const float x = rx(), y = ry(), s = 4.f + rng.nextFloat() * 7.f; g.setColour(hot.withAlpha(0.14f + rng.nextFloat() * 0.18f));
                for (int k = 0; k < 3; ++k) { const float a = (float) k * juce::MathConstants<float>::pi / 3.f; g.drawLine(x - std::cos(a) * s, y - std::sin(a) * s, x + std::cos(a) * s, y + std::sin(a) * s, 1.2f); }
            }
            g.setColour(acc.withAlpha(0.18f)); g.fillRoundedRectangle(r.getX(), r.getBottom() - 10.f, W, 10.f, 4.f);
            break;
        case DArcade:
            for (float x = r.getX(); x < r.getRight(); x += 14.f) for (int k = 0; k < 2; ++k) { if (((int) (x / 14.f) + k) % 2 == 0) { g.setColour(acc.withAlpha(0.28f)); g.fillRect(x, r.getBottom() - 14.f + (float) k * 7.f, 14.f, 7.f); } }
            { static const char* inv[] = { "..#.....#..", "...#...#...", "..#######..", ".##.###.##.", "###########", "#.#######.#", "#.#.....#.#", "...##.##..." };
              const float px = 4.f; const float ox = r.getX() + 16.f, oy = r.getY() + 14.f; g.setColour(hot.withAlpha(0.4f));
              for (int yy = 0; yy < 8; ++yy) for (int xx = 0; xx < 11; ++xx) if (inv[yy][xx] == '#') g.fillRect(ox + (float) xx * px, oy + (float) yy * px, px - 0.5f, px - 0.5f); }
            break;
        case DBolts:
            for (int i = 0; i < 5; ++i) {
                const float x = rx(), y = ry(), s = 10.f + rng.nextFloat() * 14.f; juce::Path b;
                b.startNewSubPath(x, y - s); b.lineTo(x - s * 0.45f, y + s * 0.1f); b.lineTo(x - s * 0.02f, y + s * 0.1f); b.lineTo(x - s * 0.3f, y + s); b.lineTo(x + s * 0.5f, y - s * 0.2f); b.lineTo(x + s * 0.05f, y - s * 0.2f); b.closeSubPath();
                g.setColour(acc.withAlpha(0.2f + rng.nextFloat() * 0.15f)); g.fillPath(b);
            }
            break;
        case DMoon:
        {
            const float cx = r.getRight() - 54.f, cy = r.getY() + 54.f;
            juce::Path moon; moon.addEllipse(cx - 24.f, cy - 24.f, 48.f, 48.f); juce::Path cut; cut.addEllipse(cx - 12.f, cy - 28.f, 44.f, 44.f); moon.setUsingNonZeroWinding(false); moon.addPath(cut);
            g.setColour(hot.withAlpha(0.3f)); g.fillPath(moon);
            g.setColour(acc.withAlpha(0.18f)); g.drawEllipse(cx - 44.f, cy - 44.f, 88.f, 88.f, 1.f);
            for (int i = 0; i < 10; ++i) star(rx(), ry(), 2.f + rng.nextFloat() * 3.f, 0.15f);
            break;
        }
        case DPlates:
            g.setColour(acc.withAlpha(0.16f));
            for (const auto& p : { juce::Point<float>(r.getX() + 10.f, r.getY() + 10.f), juce::Point<float>(r.getRight() - 34.f, r.getY() + 10.f), juce::Point<float>(r.getX() + 10.f, r.getBottom() - 34.f), juce::Point<float>(r.getRight() - 34.f, r.getBottom() - 34.f) }) {
                g.fillRoundedRectangle(p.x, p.y, 24.f, 24.f, 3.f); g.setColour(hot.withAlpha(0.35f)); g.fillEllipse(p.x + 4.f, p.y + 4.f, 4.f, 4.f); g.fillEllipse(p.x + 16.f, p.y + 4.f, 4.f, 4.f); g.fillEllipse(p.x + 4.f, p.y + 16.f, 4.f, 4.f); g.fillEllipse(p.x + 16.f, p.y + 16.f, 4.f, 4.f); g.setColour(acc.withAlpha(0.16f));
            }
            g.setColour(dim.withAlpha(0.22f)); g.drawLine(r.getX() + 40.f, r.getCentreY(), r.getRight() - 40.f, r.getCentreY(), 1.f);
            break;
        case DSunGrid:
        {
            const float cx = r.getRight() - 70.f, cy = r.getBottom() - 36.f, rad = 46.f;
            g.saveState(); juce::Path half; half.addPieSegment(cx - rad, cy - rad, rad * 2.f, rad * 2.f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, 0.f); g.reduceClipRegion(half);
            g.setColour(acc.withAlpha(0.32f)); g.fillRect(cx - rad, cy - rad, rad * 2.f, rad);
            g.setColour(kt::c(theme.panel)); for (int i = 0; i < 4; ++i) g.fillRect(cx - rad, cy - 8.f - (float) i * 9.f, rad * 2.f, 2.f + (float) i);
            g.restoreState();
            g.setColour(acc.withAlpha(0.14f)); for (float x = r.getX(); x < r.getRight(); x += 22.f) g.drawLine(x, r.getBottom() - 30.f, x + (x - r.getCentreX()) * 0.4f, r.getBottom(), 1.f);
            break;
        }
        default: break;
    }
    // Serial sticker: every plugin states which theme law it follows.
    const juce::Rectangle<float> tag(r.getX() + 12.f, r.getBottom() - 38.f, 150.f, 16.f);
    g.setColour(kt::c(theme.bg).withAlpha(0.55f)); g.fillRoundedRectangle(tag, 3.f);
    g.setColour(acc.withAlpha(0.65f)); g.drawRoundedRectangle(tag, 3.f, 1.f);
    g.setFont(kt::font(theme, 8.5f, true));
    g.drawText(juce::String(theme.name).toUpperCase() + "  -  " + decalFamilyName(fam), tag.reduced(5.f, 0.f), juce::Justification::centredLeft, true);
}
} // namespace pb
