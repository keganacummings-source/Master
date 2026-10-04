#pragma once

#include "FeatureNames.h"
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>
#include <vector>

namespace dm {

inline bool isKnownEffectId(const juce::String& id) {
    const auto value = id.toStdString();
    return std::find(featureIds.begin(), featureIds.end(), value) != featureIds.end();
}

inline std::vector<juce::String> readFavoriteEffectIds(const juce::ValueTree& root) {
    std::vector<juce::String> favorites;
    const auto stored = root.getChildWithName("FAVORITES");
    for (int i = 0; i < stored.getNumChildren(); ++i) {
        const auto child = stored.getChild(i);
        if (!child.hasType("EFFECT"))
            continue;
        const auto id = child.getProperty("id").toString();
        if (isKnownEffectId(id)
            && std::find(favorites.begin(), favorites.end(), id) == favorites.end())
            favorites.push_back(id);
    }
    return favorites;
}

inline void writeFavoriteEffectIds(juce::ValueTree& root, const std::vector<juce::String>& ids) {
    auto stored = root.getChildWithName("FAVORITES");
    if (!stored.isValid()) {
        stored = juce::ValueTree("FAVORITES");
        root.addChild(stored, -1, nullptr);
    }
    stored.removeAllChildren(nullptr);
    std::vector<juce::String> uniqueIds;
    for (const auto& id : ids) {
        if (!isKnownEffectId(id)
            || std::find(uniqueIds.begin(), uniqueIds.end(), id) != uniqueIds.end())
            continue;
        juce::ValueTree effect("EFFECT");
        effect.setProperty("id", id, nullptr);
        stored.addChild(effect, -1, nullptr);
        uniqueIds.push_back(id);
    }
}

inline std::vector<int> sortEffectsWithFavoritesFirst(
    std::vector<int> effects, const std::vector<juce::String>& favoriteIds) {
    std::stable_partition(effects.begin(), effects.end(), [&favoriteIds](int index) {
        if (index < 0 || index >= static_cast<int>(featureIds.size()))
            return false;
        const auto id = juce::String(featureIds[static_cast<size_t>(index)].data());
        return std::find(favoriteIds.begin(), favoriteIds.end(), id) != favoriteIds.end();
    });
    return effects;
}

} // namespace dm
