#include "DspEngine.h"
#include "FeatureNames.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
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
}

int main() {
    testEveryEffect();
    testTargetedEffects();
    testZeroAmountBypass();
    testStutterLoop();
    testTremoloDepth();
    testSafetyAndMono();
    std::cout << "Validated 200 unique effects, targeted modulation, finite output, mono/stereo operation, and safety ceiling.\n";
    return 0;
}
