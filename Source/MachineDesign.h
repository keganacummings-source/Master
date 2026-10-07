#pragma once
#include <JuceHeader.h>
#include "Themes.h"
#include <algorithm>
#include <cmath>

// Persistent machine specification shared by the builder, saved modules and Plugin View.
// The model deliberately contains presentation + behavior metadata so a shared machine can
// be reconstructed without depending on a particular editor window size.
struct MachineDesign
{
    enum class PlaygroundMode { Portrait45, Square11, Landscape54, Freeform };
    enum class SignalType { Audio, Control, Visual, Modulation, Macro };

    struct Part
    {
        juce::String id, type, themeFamily;
        juce::Rectangle<float> bounds;
        juce::Array<juce::String> allowedPlacements, allowedChildren, forbiddenChildren, snapPoints, mountPoints, acceptedSignals;
        int zLayer = 0;
        bool interactive = false;

        juce::var toVar() const
        {
            auto* o = new juce::DynamicObject();
            o->setProperty("id", id); o->setProperty("type", type); o->setProperty("themeFamily", themeFamily);
            o->setProperty("x", (double) bounds.getX()); o->setProperty("y", (double) bounds.getY());
            o->setProperty("w", (double) bounds.getWidth()); o->setProperty("h", (double) bounds.getHeight());
            o->setProperty("zLayer", zLayer); o->setProperty("interactive", interactive);
            auto arrayVar = [](const juce::Array<juce::String>& a) { juce::Array<juce::var> out; for (const auto& s : a) out.add(s); return juce::var(out); };
            o->setProperty("allowedPlacements", arrayVar(allowedPlacements));
            o->setProperty("allowedChildren", arrayVar(allowedChildren));
            o->setProperty("forbiddenChildren", arrayVar(forbiddenChildren));
            o->setProperty("snapPoints", arrayVar(snapPoints));
            o->setProperty("mountPoints", arrayVar(mountPoints));
            o->setProperty("acceptedSignals", arrayVar(acceptedSignals));
            return juce::var(o);
        }
    };

    struct Connection
    {
        juce::String from, to;
        SignalType signal = SignalType::Audio;
        juce::String parameter;
        juce::var toVar() const
        {
            auto* o = new juce::DynamicObject();
            o->setProperty("from", from); o->setProperty("to", to); o->setProperty("signal", signalName(signal)); o->setProperty("parameter", parameter);
            return juce::var(o);
        }
        static juce::String signalName(SignalType s)
        {
            switch (s) { case SignalType::Control: return "CONTROL"; case SignalType::Visual: return "VISUAL"; case SignalType::Modulation: return "MODULATION"; case SignalType::Macro: return "MACRO"; default: return "AUDIO"; }
        }
    };

    PlaygroundMode playgroundMode = PlaygroundMode::Square11;
    int playgroundWidth = 800, playgroundHeight = 800;
    juce::String aspectRatio = "1:1";
    juce::String theme = "trippah";
    juce::String bodyDesign = "Bare Frame";
    juce::Array<Part> decals, modules;
    juce::Array<Connection> connections;
    juce::Array<juce::String> macros;

    const kt::ThemePalette& palette() const
    {
        return kt::themeById(theme.isEmpty() ? juce::String("trippah") : theme);
    }

    void normalizeThemeIds()
    {
        theme = kt::themeById(theme).id;
        for (auto& p : decals) p.themeFamily = kt::themeById(p.themeFamily.isEmpty() ? theme : p.themeFamily).id;
        for (auto& p : modules) p.themeFamily = kt::themeById(p.themeFamily.isEmpty() ? theme : p.themeFamily).id;
    }

    void choosePlayground(PlaygroundMode mode)
    {
        playgroundMode = mode;
        switch (mode)
        {
            case PlaygroundMode::Portrait45: playgroundWidth = 640; playgroundHeight = 800; aspectRatio = "4:5"; break;
            case PlaygroundMode::Landscape54: playgroundWidth = 800; playgroundHeight = 640; aspectRatio = "5:4"; break;
            case PlaygroundMode::Freeform: playgroundWidth = 900; playgroundHeight = 700; aspectRatio = "FREEFORM"; break;
            default: playgroundWidth = 800; playgroundHeight = 800; aspectRatio = "1:1"; break;
        }
    }

    juce::var toVar() const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("playgroundMode", (int) playgroundMode); o->setProperty("playgroundWidth", playgroundWidth); o->setProperty("playgroundHeight", playgroundHeight);
        o->setProperty("aspectRatio", aspectRatio); o->setProperty("theme", theme); o->setProperty("bodyDesign", bodyDesign);
        juce::Array<juce::var> ds, ms, cs, mac;
        for (const auto& p : decals) ds.add(p.toVar());
        for (const auto& p : modules) ms.add(p.toVar());
        for (const auto& c : connections) cs.add(c.toVar());
        for (const auto& m : macros) mac.add(m);
        o->setProperty("decals", ds); o->setProperty("modules", ms); o->setProperty("connections", cs); o->setProperty("macros", mac);
        return juce::var(o);
    }

    void randomize(juce::Random& rng)
    {
        static const char* bodies[] = { "Dream Console", "Broken Rack", "Pocket Pedal", "Laboratory", "CRT Dream", "Modular", "Bare Frame", "Chaos" };
        bodyDesign = bodies[rng.nextInt(8)];
        decals.clear(); modules.clear(); connections.clear();
        const int count = 3 + rng.nextInt(5);
        for (int i = 0; i < count; ++i)
        {
            Part p;
            p.id = "part-" + juce::String(i + 1); p.type = (i % 3 == 0 ? "SCREEN" : (i % 3 == 1 ? "KNOB PANEL" : "LABEL PLATE"));
            p.themeFamily = theme; p.zLayer = i % 2; p.interactive = p.type.contains("KNOB") || p.type.contains("SCREEN");
            p.bounds = juce::Rectangle<float>(0, 0, p.type == "SCREEN" ? 220.f : 120.f, p.type == "SCREEN" ? 120.f : 76.f);
            p.snapPoints.add("top"); p.snapPoints.add("centre"); p.mountPoints.add("socket-" + juce::String(i + 1));
            if (p.type == "SCREEN") { p.allowedChildren.add("WAVE"); p.allowedChildren.add("SPECTRUM"); p.allowedChildren.add("VU"); p.acceptedSignals.add("AUDIO"); p.acceptedSignals.add("VISUAL"); }
            if (p.type == "KNOB PANEL") { p.allowedChildren.add("DIAL"); p.allowedChildren.add("KNOB"); p.acceptedSignals.add("CONTROL"); p.acceptedSignals.add("MODULATION"); }
            modules.add(p);
        }
        // Deterministic collision-safe placement: candidates are accepted only when inside the
        // chosen playground and separated from interactive modules.
        for (int i = 0; i < modules.size(); ++i)
        {
            auto& p = modules.getReference(i);
            bool placed = false;
            for (int attempt = 0; attempt < 128 && !placed; ++attempt)
            {
                const float maxX = juce::jmax(0.f, (float) playgroundWidth - p.bounds.getWidth() - 24.f);
                const float maxY = juce::jmax(0.f, (float) playgroundHeight - p.bounds.getHeight() - 24.f);
                auto candidate = p.bounds.withPosition(12.f + rng.nextFloat() * maxX, 12.f + rng.nextFloat() * maxY);
                bool hit = false;
                for (int j = 0; j < i; ++j) if (candidate.expanded(10.f).intersects(modules[j].bounds)) { hit = true; break; }
                if (!hit) { p.bounds = candidate; placed = true; }
            }
            if (!placed) p.bounds = juce::Rectangle<float>(12.f + (i * 137) % juce::jmax(1, playgroundWidth - (int)p.bounds.getWidth() - 24), 12.f + (i * 83) % juce::jmax(1, playgroundHeight - (int)p.bounds.getHeight() - 24), p.bounds.getWidth(), p.bounds.getHeight());
        }
    }

    static const MachineDesign fromVar(const juce::var& v)
    {
        MachineDesign d;
        if (auto* o = v.getDynamicObject())
        {
            d.playgroundMode = (PlaygroundMode) juce::jlimit(0, 3, (int)o->getProperty("playgroundMode"));
            d.playgroundWidth = juce::jmax(1, (int)o->getProperty("playgroundWidth")); d.playgroundHeight = juce::jmax(1, (int)o->getProperty("playgroundHeight"));
            d.aspectRatio = o->getProperty("aspectRatio").toString(); d.theme = o->getProperty("theme").toString(); d.bodyDesign = o->getProperty("bodyDesign").toString();
            auto readParts = [](const juce::var& a, juce::Array<Part>& out) {
                if (auto* arr = a.getArray()) for (auto& e : *arr) if (auto* po = e.getDynamicObject()) {
                    Part p; p.id = po->getProperty("id").toString(); p.type = po->getProperty("type").toString(); p.themeFamily = po->getProperty("themeFamily").toString();
                    p.bounds = juce::Rectangle<float>((float)po->getProperty("x"), (float)po->getProperty("y"), (float)po->getProperty("w"), (float)po->getProperty("h"));
                    p.zLayer = (int)po->getProperty("zLayer"); p.interactive = (bool)po->getProperty("interactive"); out.add(p);
                }
            };
            readParts(o->getProperty("modules"), d.modules); readParts(o->getProperty("decals"), d.decals);
            d.normalizeThemeIds();
            if (auto* arr = o->getProperty("macros").getArray()) for (auto& m : *arr) d.macros.add(m.toString());
        }
        return d;
    }
};
