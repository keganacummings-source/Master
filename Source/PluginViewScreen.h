#pragma once
#include <map>
#include "PluginEditor.h"
#include "FxCatalog.h"
#include "HardwareInternals.h"
#include <cmath>

static_assert(sizeof(hb::kInternals) / sizeof(hb::kInternals[0]) == (size_t) pb::kShellCount, "every template needs hardware internals");

// Pluggin View - the viewer surface. While the editor is in plugin view this overlay covers the
// whole window and shows only the built pluggin running live: its shell, its placed bays with
// real values, its live waveform - plus exactly two controls, BACK and GEEK. The Geek button
// melts moving transparent spots into the top cover so the internal hardware (motherboard,
// battery, caps, fan...) shows through; hovering a part highlights it and reads out its settings.
class PluginViewScreen final : public juce::Component, private juce::Timer
{
public:
    explicit PluginViewScreen(KyotoAudioProcessorEditor& e) : editor(e)
    {
        backBtn.setButtonText("< BACK");
        geekBtn.setButtonText("GEEK");
        geekBtn.setClickingTogglesState(true);
        addAndMakeVisible(backBtn);
        addAndMakeVisible(geekBtn);
        backBtn.onClick = [this] { editor.setPluginView(false); };
        geekBtn.onClick = [this]
        {
            geekMode = geekBtn.getToggleState();
            if (! geekMode) geekHot = -1;
        };
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        const auto& theme = editor.machineDesign.palette();
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& shell = pb::kShells[shellIdx];
        const auto accent = kt::c(theme.accent);
        const auto ink = kt::c(theme.text);
        const auto muted = kt::c(theme.muted);

        g.fillAll(kt::c(theme.bg));

        const auto caseR = pluginCase();
        const float bodyRadius = pb::shellRadius(shell);

        // Case body - this template's own hardware shell: silhouette, ears/handles/feet, trim pattern and tint.
        pb::paintShellBody(g, caseR, shell, theme, true);

        // Corner screws (pulled in for the cut-corner silhouettes).
        const float so = (shell.shape == 1 || shell.shape == 5) ? 26.f : 14.f;
        g.setColour(muted.withAlpha(0.6f));
        for (auto p : { juce::Point<float>(caseR.getX() + so, caseR.getY() + so),
                        juce::Point<float>(caseR.getRight() - so, caseR.getY() + so),
                        juce::Point<float>(caseR.getX() + so, caseR.getBottom() - so),
                        juce::Point<float>(caseR.getRight() - so, caseR.getBottom() - so) })
        {
            g.fillEllipse(p.x - 3.f, p.y - 3.f, 6.f, 6.f);
            g.setColour(kt::c(theme.bg).withAlpha(0.7f));
            g.drawLine(p.x - 2.f, p.y - 2.f, p.x + 2.f, p.y + 2.f, 1.f);
            g.setColour(muted.withAlpha(0.6f));
        }

        // Shell wordmark + live LED on the case.
        g.setColour(accent);
        g.setFont(kt::font(theme, 13.f, true));
        g.drawText(juce::String(shell.name).toUpperCase(), caseR.getX() + 24, (int) caseR.getY() + 10, 240, 18, juce::Justification::centredLeft);
        g.setColour(muted);
        g.setFont(kt::font(theme, 9.f));
        g.drawText("PLUGGIN  -  LIVE", caseR.getX() + 24, (int) caseR.getY() + 27, 240, 13, juce::Justification::centredLeft);
        const float pulse = 0.45f + 0.4f * std::sin(animPhase * 2.f);
        g.setColour(accent.withAlpha(pulse));
        g.fillEllipse(caseR.getRight() - 30.f, caseR.getY() + 12.f, 9.f, 9.f);
        g.setColour(accent.withAlpha(0.35f * pulse));
        g.drawEllipse(caseR.getRight() - 34.f, caseR.getY() + 8.f, 17.f, 17.f, 1.2f);

        const auto face = pb::faceRect(caseR);
        const auto interior = pluginInterior();
        const bool geekOn = geekReveal > 0.01f;

        // 1) Internal hardware underneath the top cover.
        if (geekOn) drawInternals(g, interior);

        // 2) The top cover: faceplate, bay wiring and the placed parts running live.
        //    In Geek mode the cover fades to a faint ghost, so the whole machine can be read at a glance
        //    (no more jumping spot-lights - it is meant to be watched, not hunted).
        const float cover = 1.f - 0.84f * geekReveal;
        g.setColour(kt::c(theme.bg).withAlpha(0.95f * cover));
        g.fillRoundedRectangle(face, bodyRadius);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(face, bodyRadius, 1.3f);
        if (geekOn) g.beginTransparencyLayer(juce::jmax(0.14f, cover));
        for (int i = 0; i < shell.slotCount; ++i)
            if (shell.slots[i].kind == pb::SlotKind::Board)
                pb::paintScreenBezel(g, pb::slotRect(face, shell.slots[i]).reduced(3.f), pb::boardScreenTypeOf(editor.proc.uiState, shell.screenStyle), theme);
        drawBayWiring(g, face, shell);
        drawPlacedParts(g, face, shell);
        if (geekOn) g.endTransparencyLayer();

        // 3) Geek x-ray overlay: slow scan band, steady part labels, hovered part tooltip.
        if (geekOn) drawXrayOverlay(g, interior, accent);
    }

    void resized() override
    {
        backBtn.setBounds(24, 20, 116, 38);
        geekBtn.setBounds(148, 20, 116, 38);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        if (! geekMode || geekReveal < 0.5f) return;
        const int hit = hitHardwarePart(e.position);
        if (hit != geekHot) { geekHot = hit; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (geekHot != -1) { geekHot = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto& shell = pb::kShells[juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex)];
        auto face = pb::faceRect(pluginCase());
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            if (! r.contains(e.position)) continue;
            const auto kind = node.getProperty("kind").toString();
            if (kind == "key")
            {
                editor.proc.noteOn((int) node.getProperty("note", 60), 0.9f);
                editor.proc.triggerSample();
                heldKey = node;
            }
            else if (kind == "dial" || kind == "slider")
            {
                dragNode = node;
                dragStartY = e.position.y;
                dragStartValue = liveParamValue(node);
            }
            return;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragNode.isValid())
        {
            auto* param = liveParam(dragNode);
            if (param != nullptr)
            {
                const float v = juce::jlimit(0.f, 1.f, dragStartValue + (dragStartY - e.position.y) * 0.005f);
                param->setValueNotifyingHost(v);
                repaint();
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (heldKey.isValid()) { editor.proc.noteOff((int) heldKey.getProperty("note", 60)); heldKey = {}; }
        dragNode = {};
    }

private:
    void timerCallback() override
    {
        const bool active = editor.inPluginView();
        if (active != isVisible()) { setVisible(active); if (active) toFront(false); }
        if (active && getBounds() != editor.getLocalBounds()) setBounds(editor.getLocalBounds());
        if (! active) { geekMode = false; geekBtn.setToggleState(false, juce::dontSendNotification); geekHot = -1; }
        animPhase += 0.035f;
        if (animPhase > juce::MathConstants<float>::twoPi) animPhase -= juce::MathConstants<float>::twoPi;
        geekReveal = juce::jlimit(0.f, 1.f, geekReveal + (geekMode ? 0.055f : -0.055f));
        if (isVisible()) repaint();
    }

    juce::Rectangle<float> pluginCase() const
    {
        auto b = getLocalBounds().toFloat();
        return { b.getX() + 22.f, b.getY() + 66.f, b.getWidth() - 44.f, b.getHeight() - 88.f };
    }

    juce::Rectangle<float> pluginInterior() const
    {
        auto face = pb::faceRect(pluginCase());
        return face.reduced(face.getWidth() * 0.02f, face.getHeight() * 0.035f);
    }

    void drawInternals(juce::Graphics& g, juce::Rectangle<float> interior) const
    {
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        for (int i = 0; i < parts.count; ++i)
            hb::drawHardwarePart(g, editor.machineDesign.palette(), parts.parts[i], hb::partRect(interior, parts.parts[i]), animPhase, i == geekHot);
    }

    void drawXrayOverlay(juce::Graphics& g, juce::Rectangle<float> interior, juce::Colour accent) const
    {
        const auto& theme = editor.machineDesign.palette();
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        const float reveal = geekReveal;

        // Slow scan band sweeping across the open machine.
        {
            g.saveState();
            g.reduceClipRegion(interior.toNearestInt());
            const float t = animPhase / juce::MathConstants<float>::twoPi;
            const float bandW = interior.getWidth() * 0.22f;
            const float sx = interior.getX() - bandW + t * (interior.getWidth() + 2.f * bandW);
            juce::ColourGradient grad(accent.withAlpha(0.f), sx - bandW, 0.f, accent.withAlpha(0.f), sx + bandW, 0.f, false);
            grad.addColour(0.5, accent.withAlpha(0.16f * reveal));
            g.setGradientFill(grad);
            g.fillRect(juce::Rectangle<float>(sx - bandW, interior.getY(), bandW * 2.f, interior.getHeight()));
            g.restoreState();
        }

        // Faint frame around the readable area.
        g.setColour(accent.withAlpha(0.22f * reveal));
        g.drawRoundedRectangle(interior.reduced(1.f), 8.f, 1.f);

        // Steady labels, one per internal part, so a passive viewer can read the machine without hovering.
        g.setFont(kt::font(theme, 10.f, true));
        for (int i = 0; i < parts.count; ++i)
        {
            const auto pr = hb::partRect(interior, parts.parts[i]);
            const juce::String name = parts.parts[i].name;
            const float w = juce::jmin(interior.getWidth() - 8.f, (float) name.length() * 7.0f + 16.f);
            const float x = juce::jlimit(interior.getX() + 4.f, juce::jmax(interior.getX() + 4.f, interior.getRight() - w - 4.f), pr.getX() + 3.f);
            const float y = juce::jlimit(interior.getY() + 4.f, juce::jmax(interior.getY() + 4.f, interior.getBottom() - 22.f), pr.getY() + 3.f);
            auto pill = juce::Rectangle<float>(x, y, w, 17.f);
            const bool hot = (i == geekHot);
            g.setColour(kt::c(theme.bg).withAlpha((hot ? 0.92f : 0.78f) * reveal));
            g.fillRoundedRectangle(pill, 8.f);
            g.setColour(accent.withAlpha((hot ? 0.95f : 0.45f) * reveal));
            g.drawRoundedRectangle(pill, 8.f, hot ? 1.6f : 1.f);
            g.setColour(kt::c(theme.text).withAlpha(reveal));
            g.drawText(name, pill, juce::Justification::centred, true);
            if (hot)
            {
                g.setColour(accent.withAlpha(0.9f));
                g.drawRoundedRectangle(pr.expanded(2.f), 6.f, 2.f);
            }
        }

        // Caption.
        g.setColour(kt::c(theme.muted).withAlpha(0.9f * reveal));
        g.setFont(kt::font(theme, 11.f));
        g.drawText("X-RAY  -  hover a part for its live readout", interior.withTrimmedTop(interior.getHeight() - 22.f).toNearestInt(), juce::Justification::centred, true);

        if (geekHot >= 0 && geekHot < parts.count)
            drawTooltip(g, interior, geekHot, hb::partRect(interior, parts.parts[geekHot]));
    }

    void punchHole(juce::Graphics& g, juce::Rectangle<float> interior, float cx, float cy, float r, juce::Colour accent) const
    {
        if (r < 1.f) return;
        g.saveState();
        juce::Path hole;
        hole.addEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
        g.reduceClipRegion(hole);
        drawInternals(g, interior);
        g.restoreState();
        g.setColour(accent.withAlpha(0.35f));
        g.drawEllipse(cx - r, cy - r, r * 2.f, r * 2.f, 1.4f);
        g.setColour(accent.withAlpha(0.12f));
        g.drawEllipse(cx - r - 4.f, cy - r - 4.f, r * 2.f + 8.f, r * 2.f + 8.f, 1.f);
    }

    int hitHardwarePart(juce::Point<float> pos) const
    {
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& parts = hb::kInternals[shellIdx];
        const auto interior = pluginInterior();
        for (int i = 0; i < parts.count; ++i)
            if (hb::partRect(interior, parts.parts[i]).contains(pos)) return i;
        return -1;
    }

    juce::RangedAudioParameter* liveParam(const juce::ValueTree& node) const
    {
        const int slot = (int) node.getProperty("slot", -1);
        if (slot < 0) return nullptr;
        const auto id = "s" + juce::String(slot + 1).paddedLeft('0', 2) + node.getProperty("param").toString();
        return editor.proc.apvts.getParameter(id);
    }

    float liveParamValue(const juce::ValueTree& node) const
    {
        auto* p = liveParam(node);
        return p != nullptr ? p->getValue() : 0.f;
    }

    static juce::String fxNameFor(int type)
    {
        if (type == KyotoAudioProcessor::kMixType) return "MASTER MIX";
        if (type == KyotoAudioProcessor::kBreakType) return "CHAIN BREAK";
        if (type >= 0 && type < kt::kFxCount) return kt::kFx[type].name;
        return "FX";
    }

    int liveTypeValue(const juce::ValueTree& node) const
    {
        const int slot = (int) node.getProperty("slot", -1);
        if (slot < 0) return -1;
        auto* p = editor.proc.apvts.getParameter("s" + juce::String(slot + 1).paddedLeft('0', 2) + "type");
        return p != nullptr ? (int) std::lround(p->convertFrom0to1(p->getValue())) : -1;
    }

    juce::String pct(float v) const { return juce::String(juce::roundToInt(v * 100.f)) + "%"; }

    int activeBayCount() const
    {
        int n = 0;
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (node.hasType("w") && (int) node.getProperty("shellSlot", -1) > 0) ++n;
        }
        return n;
    }

    // Live settings readout for a hovered internal part.
    juce::String liveSettingsFor(const hb::HardwarePart& part) const
    {
        auto& apvts = editor.proc.apvts;
        if (part.kind == hb::PartKind::Board || part.kind == hb::PartKind::Battery || part.kind == hb::PartKind::Psu)
        {
            juce::String line;
            if (auto* type = apvts.getParameter("s01type"))
                line << "FX: " << fxNameFor((int) std::lround(type->convertFrom0to1(type->getValue()))) << "   ";
            if (auto* on = apvts.getParameter("s01on"))
                line << (on->getValue() >= 0.5f ? "ON" : "STANDBY") << "   ";
            if (auto* mix = apvts.getParameter("s01mix"))
                line << "MIX " << pct(mix->getValue());
            line << "   BAYS " << activeBayCount();
            return line;
        }
        return {};
    }

    void drawTooltip(juce::Graphics& g, juce::Rectangle<float> interior, int partIndex, juce::Rectangle<float> partRect) const
    {
        const auto& theme = editor.machineDesign.palette();
        const auto accent = kt::c(theme.accent);
        const int shellIdx = juce::jlimit(0, pb::kShellCount - 1, editor.shellIndex);
        const auto& part = hb::kInternals[shellIdx].parts[partIndex];
        const auto settings = liveSettingsFor(part);
        const float cardW = juce::jmin(300.f, interior.getWidth() * 0.7f);
        const float cardH = settings.isEmpty() ? 62.f : 80.f;
        auto card = juce::Rectangle<float>(partRect.getCentreX() - cardW * 0.5f, partRect.getY() - cardH - 12.f, cardW, cardH);
        if (card.getY() < interior.getY()) card.setPosition(card.getX(), partRect.getBottom() + 12.f);
        card.setX(juce::jlimit(interior.getX(), interior.getRight() - card.getWidth(), card.getX()));
        card.setY(juce::jlimit(interior.getY(), interior.getBottom() - card.getHeight(), card.getY()));

        g.setColour(kt::c(theme.panel).withAlpha(0.97f));
        g.fillRoundedRectangle(card, 10.f);
        g.setColour(accent);
        g.fillRoundedRectangle(card.getX(), card.getY(), 3.5f, card.getHeight(), 2.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(card, 10.f, 1.f);
        g.setColour(accent);
        g.setFont(kt::font(theme, 12.5f, true));
        g.drawText(part.name, (int) card.getX() + 12, (int) card.getY() + 7, (int) card.getWidth() - 24, 16, juce::Justification::centredLeft);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 10.f));
        g.drawFittedText(part.blurb, (int) card.getX() + 12, (int) card.getY() + 23, (int) card.getWidth() - 24, settings.isEmpty() ? 32 : 20, juce::Justification::topLeft, 2);
        if (settings.isNotEmpty())
        {
            g.setColour(kt::c(theme.text));
            g.setFont(kt::font(theme, 10.f, true));
            g.drawText(settings, (int) card.getX() + 12, (int) card.getBottom() - 22, (int) card.getWidth() - 24, 16, juce::Justification::centredLeft);
        }
    }

    void drawBayWiring(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell) const
    {
        // Every part is wired into the part it was connected to (the chain), not just to the motherboard.
        std::map<int, juce::Point<float>> centre;
        std::map<int, int> parentOf;
        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            centre[bay] = pb::slotRect(face, shell.slots[bay]).getCentre();
            parentOf[bay] = (int) node.getProperty("parent", 0);
        }
        if (centre.find(0) == centre.end()) return;
        const auto accent = kt::c(editor.machineDesign.palette().accent);
        for (auto& entry : centre)
        {
            const int bay = entry.first;
            if (bay == 0) continue;
            int par = parentOf[bay];
            if (par == bay || centre.find(par) == centre.end()) par = 0;
            const auto from = centre[par];
            const auto to = entry.second;
            juce::Path wire;
            wire.startNewSubPath(from);
            wire.cubicTo(from.x, (from.y + to.y) * 0.5f, to.x, (from.y + to.y) * 0.5f, to.x, to.y);
            g.setColour(accent.withAlpha(0.30f));
            g.strokePath(wire, juce::PathStrokeType(1.6f));
            g.setColour(accent.withAlpha(0.8f));
            g.fillEllipse(to.x - 2.5f, to.y - 2.5f, 5.f, 5.f);
        }
    }

    void drawPlacedParts(juce::Graphics& g, juce::Rectangle<float> face, const pb::Shell& shell) const
    {
        const auto& theme = editor.machineDesign.palette();
        const auto accent = kt::c(theme.accent);
        const auto ink = kt::c(theme.text);
        const auto muted = kt::c(theme.muted);
        bool placedAny = false;

        for (int i = 0; i < editor.proc.uiState.getNumChildren(); ++i)
        {
            auto node = editor.proc.uiState.getChild(i);
            if (! node.hasType("w")) continue;
            const int bay = (int) node.getProperty("shellSlot", -1);
            if (bay < 0 || bay >= shell.slotCount) continue;
            auto r = pb::slotRect(face, shell.slots[bay]);
            if (r.getWidth() < 8.f || r.getHeight() < 8.f) continue;
            placedAny = true;
            const auto kind = node.getProperty("kind").toString();
            const auto label = node.getProperty("label").toString();

            g.setColour(kt::c(theme.panel).withAlpha(0.92f));
            g.fillRoundedRectangle(r.reduced(3.f), 8.f);
            g.setColour(accent.withAlpha(0.55f));
            g.drawRoundedRectangle(r.reduced(3.f), 8.f, 1.2f);

            if (kind == "board")
            {
                float live[128] {};
                editor.proc.copyScope(live, 128);
                const int st = pb::boardScreenTypeOf(editor.proc.uiState, shell.screenStyle);
                pb::paintScreenFace(g, r.reduced(5.f), st, theme, live, 128, juce::String("SCREEN  -  ") + pb::kScreenTypes[st]);
            }
            else if (kind == "dial")
            {
                const float v = liveParamValue(node);
                auto knobArea = r.reduced(6.f);
                const float rad = juce::jmin(knobArea.getWidth(), knobArea.getHeight()) * 0.5f - 4.f;
                const auto centre = knobArea.getCentre();
                g.setColour(accent.withAlpha(0.16f));
                g.fillEllipse(centre.x - rad, centre.y - rad, rad * 2.f, rad * 2.f);
                g.setColour(kt::c(theme.knob));
                g.fillEllipse(centre.x - rad * 0.72f, centre.y - rad * 0.72f, rad * 1.44f, rad * 1.44f);
                const float a0 = juce::MathConstants<float>::pi * 1.25f;
                const float sweep = a0 + (juce::MathConstants<float>::twoPi * 0.75f) * v;
                juce::Path arc;
                arc.addCentredArc(centre.x, centre.y, rad, rad, 0.f, a0, a0 + (juce::MathConstants<float>::twoPi * 0.75f) * v, true);
                g.setColour(accent);
                g.strokePath(arc, juce::PathStrokeType(2.2f));
                g.drawLine(centre.x, centre.y, centre.x + std::cos(sweep) * rad * 0.62f, centre.y + std::sin(sweep) * rad * 0.62f, 2.f);
                g.setColour(ink);
                g.setFont(kt::font(theme, 9.f, true));
                g.drawText(pct(v), r.reduced(3.f).removeFromBottom(13.f), juce::Justification::centred);
                g.setColour(muted);
                g.setFont(kt::font(theme, 8.f));
                g.drawText(label, r.reduced(3.f).removeFromTop(12.f), juce::Justification::centred);
            }
            else if (kind == "slider")
            {
                const float v = liveParamValue(node);
                auto track = r.reduced(10.f, 8.f).withSizeKeepingCentre(juce::jmax(6.f, r.getWidth() * 0.16f), r.getHeight() - 30.f);
                g.setColour(kt::c(theme.bg));
                g.fillRoundedRectangle(track, 3.f);
                const float thumbY = track.getBottom() - v * track.getHeight();
                g.setColour(accent);
                g.fillRoundedRectangle(track.getX(), thumbY - 5.f, track.getWidth(), 10.f, 3.f);
                g.setColour(ink);
                g.setFont(kt::font(theme, 9.f, true));
                g.drawText(pct(v), r.reduced(3.f).removeFromBottom(13.f), juce::Justification::centred);
                g.setColour(muted);
                g.setFont(kt::font(theme, 8.f));
                g.drawText(label, r.reduced(3.f).removeFromTop(12.f), juce::Justification::centred);
            }
            else if (kind == "key")
            {
                g.setColour(accent.withAlpha(0.25f));
                g.fillRoundedRectangle(r.reduced(6.f), 6.f);
                g.setColour(accent);
                g.drawRoundedRectangle(r.reduced(6.f), 6.f, 1.4f);
                g.setColour(ink);
                g.setFont(kt::font(theme, 9.f, true));
                g.drawText(label, r, juce::Justification::centred);
            }
            else if (kind == "wave")
            {
                float samples[256] {};
                editor.proc.copyScope(samples, 256);
                auto waveR = r.reduced(8.f, 6.f);
                waveR.removeFromBottom(14.f);
                g.setColour(kt::c(theme.bg).brighter(0.03f));
                g.fillRoundedRectangle(waveR, 4.f);
                juce::Path wave;
                const float mid = waveR.getCentreY();
                wave.startNewSubPath(waveR.getX() + 4.f, mid);
                for (int s = 0; s < 128; ++s)
                    wave.lineTo(waveR.getX() + 4.f + (waveR.getWidth() - 8.f) * (float) s / 127.f,
                                mid - samples[s * 2] * waveR.getHeight() * 0.46f);
                g.setColour(accent);
                g.strokePath(wave, juce::PathStrokeType(1.5f));
                g.setColour(muted);
                g.setFont(kt::font(theme, 8.f, true));
                g.drawText("LIVE", r.reduced(6.f).removeFromBottom(12.f), juce::Justification::centred);
            }
            else
            {
                const auto style = node.getProperty("style").toString();
                g.setColour(accent.withAlpha(0.35f));
                if (style == "vent")
                    for (int l = 0; l < 4; ++l) g.drawLine(r.getX() + 8.f, r.getY() + 10.f + (float) l * 7.f, r.getRight() - 8.f, r.getY() + 10.f + (float) l * 7.f, 1.3f);
                else if (style == "rail")
                    g.fillRoundedRectangle(r.reduced(r.getWidth() * 0.35f, 6.f), 3.f);
                else
                    g.fillEllipse(r.reduced(8.f));
            }
        }

        // Nothing built on the shell yet: still show the playground machine so the view is never empty.
        if (! placedAny)
        {
            const auto& md = editor.machineDesign;
            g.setColour(muted);
            g.setFont(kt::font(theme, 12.f));
            g.drawText("No bays filled yet - parts placed in the Plugin Builder show up here, live.",
                       rtoInt(face).removeFromTop(juce::jmax(20, (int) (face.getHeight() * 0.08f))),
                       juce::Justification::centred);
            const float sx = face.getWidth() / (float) juce::jmax(1, md.playgroundWidth);
            const float sy = face.getHeight() / (float) juce::jmax(1, md.playgroundHeight);
            for (const auto& part : md.modules)
            {
                auto q = juce::Rectangle<float>(face.getX() + part.bounds.getX() * sx, face.getY() + part.bounds.getY() * sy,
                                               part.bounds.getWidth() * sx, part.bounds.getHeight() * sy);
                const float loX = face.getX() + 4.f, hiX = juce::jmax(loX, face.getRight() - q.getWidth() - 4.f);
                const float loY = face.getY() + 4.f, hiY = juce::jmax(loY, face.getBottom() - q.getHeight() - 4.f);
                q.setX(juce::jlimit(loX, hiX, q.getX()));
                q.setY(juce::jlimit(loY, hiY, q.getY()));
                g.setColour(kt::c(theme.panel).brighter(0.08f));
                g.fillRoundedRectangle(q, 10.f);
                g.setColour(accent.withAlpha(0.72f));
                g.drawRoundedRectangle(q, 10.f, 1.5f);
                g.setColour(ink);
                g.setFont(kt::font(theme, 9.f, true));
                g.drawFittedText(part.type, q.reduced(7.f).toNearestInt(), juce::Justification::centred, 1);
            }
        }
    }

    static juce::Rectangle<int> rtoInt(juce::Rectangle<float> r) { return r.toNearestInt(); }

    KyotoAudioProcessorEditor& editor;
    juce::TextButton backBtn, geekBtn;
    bool geekMode = false;
    float geekReveal = 0.f;
    float animPhase = 0.f;
    int geekHot = -1;
    juce::ValueTree dragNode, heldKey;
    float dragStartValue = 0.f, dragStartY = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginViewScreen)
};

// Installs the viewer overlay once the editor is parented into a host window.
// The overlay is added as a hidden child; it only becomes visible inside Plugin View.
inline void KyotoAudioProcessorEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();
    if (viewScreen == nullptr)
    {
        viewScreen = new PluginViewScreen(*this);
        addChildComponent(viewScreen);
        viewScreen->setBounds(getLocalBounds());
        viewScreen->resized();
        viewScreen->toFront(false);
    }
}