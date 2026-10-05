#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace dm {

struct PresetStep {
    juce::String effectId;
    float amount = 0.22f;
};

struct CustomEffectPart {
    juce::String effectId;
    std::array<float, 5> controls { 0.22f, 0.50f, 0.50f, 0.65f, 0.50f };
};

struct CustomPreset {
    juce::String id;
    juce::String name;
    std::vector<PresetStep> steps;
    std::vector<CustomEffectPart> parts;
};

inline juce::ValueTree createCustomPresetTree(const CustomPreset& preset) {
    juce::ValueTree tree("PRESET");
    tree.setProperty("id", preset.id, nullptr);
    tree.setProperty("name", preset.name, nullptr);
    for (const auto& step : preset.steps) {
        juce::ValueTree child("STEP");
        child.setProperty("effectId", step.effectId, nullptr);
        child.setProperty("amount", step.amount, nullptr);
        tree.addChild(child, -1, nullptr);
    }
    return tree;
}

inline bool readCustomPresetTree(const juce::ValueTree& tree, CustomPreset& preset) {
    if (!tree.hasType("PRESET"))
        return false;

    CustomPreset parsed;
    parsed.id = tree.getProperty("id").toString();
    parsed.name = tree.getProperty("name").toString();
    if (parsed.id.isEmpty() || parsed.name.trim().isEmpty())
        return false;

    for (int i = 0; i < tree.getNumChildren(); ++i) {
        const auto child = tree.getChild(i);
        if (!child.hasType("STEP"))
            continue;
        const auto effectId = child.getProperty("effectId").toString();
        const auto amountText = child.getProperty("amount").toString();
        if (effectId.isEmpty() || amountText.isEmpty())
            return false;
        const float amount = amountText.getFloatValue();
        if (!std::isfinite(amount) || amount < 0.0f || amount > 1.0f)
            return false;
        parsed.steps.push_back({effectId, amount});
    }
    if (parsed.steps.empty())
        return false;
    preset = std::move(parsed);
    return true;
}

} // namespace dm
