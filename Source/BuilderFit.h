#pragma once
#include <JuceHeader.h>
#include "PluginShells.h"

// Module fit + template roll helpers for the Plugin Builder.
namespace bf
{
inline int partCapacity(juce::Rectangle<float> module)
{
    const float area = juce::jmax(1.f, module.getWidth() * module.getHeight());
    const int byArea = (int) std::floor(area / (52.f * 48.f));
    return juce::jlimit(1, 6, byArea);
}

inline juce::String sliderStyleFor(juce::Rectangle<int> r)
{
    if (r.getWidth() < 8 || r.getHeight() < 8) return "slider";
    return r.getWidth() >= r.getHeight() ? "hfader" : "slider";
}

inline bool essential(const juce::String& kind)
{
    return kind == "wave" || kind == "key" || kind == "dial" || kind == "sound";
}

inline juce::Rectangle<float> fittedSlot(juce::Rectangle<float> face, const pb::Slot& slot, int index, int count)
{
    auto r = pb::slotRect(face, slot);
    // Keep a gap so neighbouring modules never share an edge after a resize.
    const float gap = juce::jlimit(2.f, 8.f, face.getWidth() * 0.008f);
    r = r.reduced(gap);
    if (r.getWidth() < 18.f || r.getHeight() < 18.f) return {};
    juce::ignoreUnused(index, count);
    return r;
}
}
