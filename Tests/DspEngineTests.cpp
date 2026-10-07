#include "DspEngine.h"
#include "FeatureNames.h"
#include "Randomizer.h"
#include "CustomPresetState.h"
#include "EffectFavorites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
template <typename Strings>
constexpr bool uniqueStrings(const Strings& strings) {
    for (size_t i = 0; i < strings.size(); ++i)
        for (size_t j = i + 1; j < strings.size(); ++j)
            if (strings[i] == strings[j])
                return false;
    return true;
}

static_assert(dm::featureIds.size() == dm::DspEngine::effectCount);
static_assert(dm::featureNames.size() == dm::DspEngine::effectCount);
static_assert(uniqueStrings(dm::featureIds), "Effect IDs must be unique");
static_assert(uniqueStrings(dm::featureNames), "Effect names must be unique");

[[noreturn]] void fail(const std::string& message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void require(bool condition, const std::string& message) {
    if (!condition)
        fail(message);
}

std::vector<float> makeInput(size_t samples, int channel, double sampleRate) {
    std::vector<float> result(samples);
    uint32_t noise = 0x1234567u + static_cast<uint32_t>(channel * 0x10203);
    for (size_t i = 0; i < samples; ++i) {
        noise = noise * 1664525u + 1013904223u;
        const float texture = (static_cast<float>((noise >> 8) & 0xffffu) / 32767.5f) - 1.0f;
        const float t = static_cast<float>(i / sampleRate);
        result[i] = 0.55f * std::sin(2.0f * 3.14159265f * (173.0f + channel * 61.0f) * t)
                  + 0.34f * std::sin(2.0f * 3.14159265f * 917.0f * t)
                  + 0.08f * texture;
    }
    return result;
}

void testEveryEffect() {
    constexpr size_t samples = 8192;
    const auto sourceL = makeInput(samples, 0, 48000.0);
    const auto sourceR = makeInput(samples, 1, 48000.0);
    std::vector<std::vector<float>> renders;
    renders.reserve(dm::DspEngine::effectCount);

    for (int effect = 0; effect < dm::DspEngine::effectCount; ++effect) {
        dm::DspEngine engine;
        engine.prepare(48000.0);
        auto left = sourceL;
        auto right = sourceR;
        std::array<bool, dm::DspEngine::effectCount> enabled{};
        std::array<float, dm::DspEngine::effectCount> amount{};
        enabled[static_cast<size_t>(effect)] = true;
        amount[static_cast<size_t>(effect)] = 1.0f;
        engine.process(left.data(), right.data(), 2, static_cast<int>(samples), enabled, amount);

        double energy = 0.0;
        double delta = 0.0;
        for (size_t i = 0; i < samples; ++i) {
            require(std::isfinite(left[i]) && std::isfinite(right[i]),
                    std::string(dm::featureNames[static_cast<size_t>(effect)]) + " produced NaN/Inf");
            require(std::abs(left[i]) <= 0.961f && std::abs(right[i]) <= 0.961f,
                    std::string(dm::featureNames[static_cast<size_t>(effect)]) + " exceeded the safety ceiling");
            energy += static_cast<double>(left[i]) * left[i] + static_cast<double>(right[i]) * right[i];
            delta += std::abs(left[i] - sourceL[i]) + std::abs(right[i] - sourceR[i]);
        }
        require(energy / (samples * 2.0) > 1.0e-5,
                std::string(dm::featureNames[static_cast<size_t>(effect)]) + " produced silence");
        require(delta > 0.01,
                std::string(dm::featureNames[static_cast<size_t>(effect)]) + " did not affect the signal");
        renders.push_back(std::move(left));
    }

    for (size_t i = 0; i < renders.size(); ++i) {
        for (size_t j = i + 1; j < renders.size(); ++j) {
            bool differs = false;
            float maxDifference = 0.0f;
            for (size_t n = 0; n < samples && !differs; ++n) {
                maxDifference = std::max(maxDifference, std::abs(renders[i][n] - renders[j][n]));
                differs = maxDifference > 1.0e-6f;
            }
            require(differs, std::string(dm::featureNames[i]) + " and " + std::string(dm::featureNames[j]) + " rendered identically (max delta " + std::to_string(maxDifference) + ")");
        }
    }
}

void testTargetedEffects() {
    constexpr size_t samples = 24000;
    const auto input = makeInput(samples, 0, 48000.0);
    for (const auto id : {0, 1, 7, 10, 16}) {
        dm::DspEngine engine;
        engine.prepare(48000.0);
        auto left = input;
        auto right = input;
        std::array<bool, dm::DspEngine::effectCount> enabled{};
        std::array<float, dm::DspEngine::effectCount> amount{};
        enabled[static_cast<size_t>(id)] = true;
        amount[static_cast<size_t>(id)] = 1.0f;
        engine.process(left.data(), right.data(), 2, static_cast<int>(samples), enabled, amount);

        double difference = 0.0;
        double modulationEnergy = 0.0;
        for (size_t i = 0; i < samples; ++i) {
            difference += std::abs(left[i] - input[i]);
            modulationEnergy += std::abs(left[i] - right[i]);
        }
        require(difference > 10.0, std::string(dm::featureNames[static_cast<size_t>(id)]) + " did not produce a working effect");
        if (id == 1 || id == 10)
            require(modulationEnergy > 0.01, std::string(dm::featureNames[static_cast<size_t>(id)]) + " did not create stereo modulation");
    }
}

void testZeroAmountBypass() {
    constexpr size_t samples = 1024;
    const auto sourceL = makeInput(samples, 0, 48000.0);
    const auto sourceR = makeInput(samples, 1, 48000.0);
    auto processedL = sourceL;
    auto processedR = sourceR;
    auto bypassL = sourceL;
    auto bypassR = sourceR;
    dm::DspEngine processed;
    dm::DspEngine bypass;
    processed.prepare(48000.0);
    bypass.prepare(48000.0);
    std::array<bool, dm::DspEngine::effectCount> enabled{};
    std::array<bool, dm::DspEngine::effectCount> disabled{};
    std::array<float, dm::DspEngine::effectCount> amounts{};
    enabled.fill(true);
    processed.process(processedL.data(), processedR.data(), 2, static_cast<int>(samples), enabled, amounts);
    bypass.process(bypassL.data(), bypassR.data(), 2, static_cast<int>(samples), disabled, amounts);
    for (size_t i = 0; i < samples; ++i)
        require(std::abs(processedL[i] - bypassL[i]) < 1.0e-7f
                && std::abs(processedR[i] - bypassR[i]) < 1.0e-7f,
                "zero amount was not a transparent bypass");
}

void testStutterLoop() {
    constexpr size_t samples = 24000;
    constexpr size_t period = 4660;
    const auto source = makeInput(samples, 0, 48000.0);
    auto left = source;
    auto right = source;
    dm::DspEngine engine;
    engine.prepare(48000.0);
    std::array<bool, dm::DspEngine::effectCount> enabled{};
    std::array<float, dm::DspEngine::effectCount> amount{};
    enabled[16] = true;
    amount[16] = 1.0f;
    engine.process(left.data(), right.data(), 2, static_cast<int>(samples), enabled, amount);

    double repeatedError = 0.0;
    double sourceDifference = 0.0;
    constexpr size_t offset = 250;
    constexpr size_t comparisonSamples = 1000;
    for (size_t i = 0; i < comparisonSamples; ++i) {
        const size_t first = period + offset + i;
        const size_t second = first + period;
        repeatedError += std::abs(left[first] - left[second]);
        sourceDifference += std::abs(source[first] - source[second]);
    }
    require(repeatedError < sourceDifference * 0.75, "Stutter did not repeat a captured loop");
}

void testTremoloDepth() {
    constexpr size_t samples = 48000;
    std::vector<float> left(samples), right(samples);
    for (size_t i = 0; i < samples; ++i)
        left[i] = right[i] = 0.5f * std::sin(2.0f * 3.14159265f * 211.0f * static_cast<float>(i) / 48000.0f);
    dm::DspEngine engine;
    engine.prepare(48000.0);
    std::array<bool, dm::DspEngine::effectCount> enabled{};
    std::array<float, dm::DspEngine::effectCount> amount{};
    enabled[7] = true;
    amount[7] = 1.0f;
    engine.process(left.data(), right.data(), 2, static_cast<int>(samples), enabled, amount);

    const auto rms = [&](size_t start) {
        double energy = 0.0;
        for (size_t i = start; i < start + 1200; ++i)
            energy += static_cast<double>(left[i]) * left[i];
        return std::sqrt(energy / 1200.0);
    };
    require(rms(10500) > rms(33000) * 1.4, "Tremolo did not produce a measurable amplitude cycle");
}

void testSafetyAndMono() {
    constexpr size_t samples = 8192;
    for (const double rate : {44100.0, 96000.0}) {
        dm::DspEngine engine;
        engine.prepare(rate);
        std::vector<float> mono(samples, 1.0f);
        mono[10] = std::numeric_limits<float>::infinity();
        mono[11] = std::numeric_limits<float>::quiet_NaN();
        std::array<bool, dm::DspEngine::effectCount> enabled{};
        std::array<float, dm::DspEngine::effectCount> amount{};
        enabled.fill(true);
        amount.fill(1.0f);
        engine.process(mono.data(), nullptr, 1, static_cast<int>(samples), enabled, amount);
        for (const float sample : mono)
            require(std::isfinite(sample) && std::abs(sample) <= 0.961f, "full-stack mono safety failure");

        engine.prepare(rate);
        std::vector<float> impulseL(samples, 0.0f), impulseR(samples, 0.0f);
        impulseL[4] = 1.0f;
        impulseR[4] = -1.0f;
        engine.process(impulseL.data(), impulseR.data(), 2, static_cast<int>(samples), enabled, amount);
        for (size_t i = 0; i < samples; ++i)
            require(std::isfinite(impulseL[i]) && std::isfinite(impulseR[i])
                    && std::abs(impulseL[i]) <= 0.961f && std::abs(impulseR[i]) <= 0.961f,
                    "full-stack stereo impulse safety failure");
    }
}

void testRandomizersAndCategories() {
    const auto repeatedA = dm::randomEffectSelection(0x13579bdu, 10, 20);
    const auto repeatedB = dm::randomEffectSelection(0x13579bdu, 10, 20);
    require(repeatedA == repeatedB, "random effect selection was not reproducible for a fixed seed");

    bool sawTen = false;
    bool sawTwenty = false;
    for (uint32_t seed = 0; seed < 3000; ++seed) {
        for (const auto bounds : {std::pair<int, int>{1, 10}, std::pair<int, int>{10, 20}}) {
            const auto selected = dm::randomEffectSelection(seed, bounds.first, bounds.second);
            require(static_cast<int>(selected.size()) >= bounds.first
                    && static_cast<int>(selected.size()) <= bounds.second,
                    "randomizer count fell outside its inclusive bounds");
            auto unique = selected;
            std::sort(unique.begin(), unique.end());
            require(std::adjacent_find(unique.begin(), unique.end()) == unique.end(),
                    "randomizer selected the same effect more than once");
            if (bounds.first == 10) {
                sawTen = sawTen || selected.size() == 10;
                sawTwenty = sawTwenty || selected.size() == 20;
            }
        }
    }
    require(sawTen && sawTwenty, "XTRMRND did not reach both inclusive endpoints");

    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto value = dm::definedRandomAmount(i);
        require(std::isfinite(value) && value >= 0.15f && value <= 0.85f,
                "defined random amount fell outside its documented range");
        require(value == dm::definedRandomAmount(i), "defined random amount was not stable");
        const int category = dm::effectCategoryIndex(i);
        require(category >= 0 && category < static_cast<int>(dm::effectCategories.size()),
                std::string(dm::featureNames[static_cast<size_t>(i)]) + " has no category");
    }
    for (int category = 0; category < static_cast<int>(dm::effectCategories.size()); ++category) {
        bool populated = false;
        for (int i = 0; i < dm::DspEngine::effectCount; ++i)
            populated = populated || dm::effectCategoryIndex(i) == category;
        require(populated, std::string(dm::effectCategories[static_cast<size_t>(category)]) + " is empty");
    }
}

void testCustomPresetStateRoundTrip() {
    dm::CustomPreset preset;
    preset.id = "saved-chain-1";
    preset.name = "Night texture";
    preset.steps = {{"delay", 0.375f}, {"drive", 0.812f}, {"width", 0.25f}};

    juce::ValueTree root("PARAMETERS");
    root.addChild(dm::createCustomPresetTree(preset), -1, nullptr);
    const auto xml = root.createXml();
    require(xml != nullptr, "custom preset state did not serialize to XML");
    const auto restoredRoot = juce::ValueTree::fromXml(*xml);
    dm::CustomPreset restored;
    require(dm::readCustomPresetTree(restoredRoot.getChildWithName("PRESET"), restored),
            "custom preset state did not parse after XML restore");
    require(restored.id == preset.id && restored.name == preset.name,
            "custom preset ID or name did not survive state restore");
    require(restored.steps.size() == preset.steps.size(), "custom preset chain length did not survive restore");
    for (size_t i = 0; i < preset.steps.size(); ++i) {
        require(restored.steps[i].effectId == preset.steps[i].effectId,
                "custom preset effect ID or order did not survive restore");
        require(std::abs(restored.steps[i].amount - preset.steps[i].amount) < 1.0e-6f,
                "custom preset amount did not survive restore");
    }

}

void testFavoriteStateRoundTripAndOrdering() {
    juce::ValueTree root("PARAMETERS");
    juce::ValueTree parameter("PARAM");
    parameter.setProperty("id", "fx0", nullptr);
    root.addChild(parameter, -1, nullptr);
    const std::vector<juce::String> favorites{"vortex", "drive", "delay", "not-a-feature"};
    dm::writeFavoriteEffectIds(root, favorites);
    const auto xml = root.createXml();
    require(xml != nullptr, "favorite state did not serialize to XML");
    const auto restoredRoot = juce::ValueTree::fromXml(*xml);
    require(restoredRoot.getChildWithName("PARAM").isValid(),
            "adding favorites removed existing parameter state");
    const auto restored = dm::readFavoriteEffectIds(restoredRoot);
    require(restored == std::vector<juce::String>{"vortex", "drive", "delay"},
            "favorite IDs did not survive state restore or invalid IDs were not ignored");

    const auto sorted = dm::sortEffectsWithFavoritesFirst({0, 1, 2, 35, 34}, restored);
    require(sorted == std::vector<int>{0, 2, 34, 1, 35},
            "favorite sorting did not preserve catalog order within favorite and regular groups");
    const auto mixed = dm::sortEffectsWithFavoritesFirst({4, 0, 2, 34}, restored);
    require(mixed == std::vector<int>{0, 2, 34, 4},
            "favorite effects were not moved ahead of regular effects");

    dm::writeFavoriteEffectIds(root, {"delay", "delay", "unknown"});
    require(dm::readFavoriteEffectIds(root) == std::vector<juce::String>{"delay"},
            "favorite updates did not remove duplicates and unknown feature IDs");
}
}

int main() {
    testEveryEffect();
    testTargetedEffects();
    testZeroAmountBypass();
    testStutterLoop();
    testTremoloDepth();
    testSafetyAndMono();
    testRandomizersAndCategories();
    testCustomPresetStateRoundTrip();
    testFavoriteStateRoundTripAndOrdering();
    std::cout << "Validated 200 unique effects, randomizer bounds, effect categories, favorite ordering/state round trips, custom-preset state round trips, DSP behavior, mono/stereo operation, and safety ceiling.\n";
    return 0;
}
