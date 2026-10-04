#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace dm {

inline std::vector<int> randomEffectSelection(uint32_t seed, int minimum, int maximum) {
    constexpr int effectCount = 200;
    minimum = std::clamp(minimum, 1, effectCount);
    maximum = std::clamp(maximum, minimum, effectCount);
    std::array<int, effectCount> indices{};
    for (int i = 0; i < effectCount; ++i)
        indices[static_cast<size_t>(i)] = i;

    std::mt19937 generator(seed);
    std::shuffle(indices.begin(), indices.end(), generator);
    std::uniform_int_distribution<int> countDistribution(minimum, maximum);
    const int count = countDistribution(generator);
    return {indices.begin(), indices.begin() + count};
}

inline float definedRandomAmount(int effectIndex) {
    const auto index = static_cast<uint32_t>(effectIndex);
    uint32_t value = index * 2654435761u + 0x9e3779b9u;
    value ^= value >> 16;
    return 0.15f + static_cast<float>(value % 701u) / 1000.0f;
}

} // namespace dm
