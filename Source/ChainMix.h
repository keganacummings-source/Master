#pragma once
#include <cmath>

namespace kt
{
// Shared by the audio path and its standalone checks. Gains belong to chain
// ordinals (the segments separated by BREAK), not to individual FX wet/dry mixes.
struct ChainMix
{
    float left = 0.f, right = 0.f;
    int count = 0;

    void add(float l, float r, float gain) noexcept
    {
        left += l * gain;
        right += r * gain;
        ++count;
    }

    void output(float& l, float& r, bool perChainLevels) const noexcept
    {
        const float scale = perChainLevels || count <= 1 ? 1.f : 1.f / std::sqrt((float) count);
        l = left * scale;
        r = right * scale;
    }
};
}
