#include "DspEngine.h"
#include "FeatureNames.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace dm {
namespace {
constexpr float pi = 3.14159265358979323846f;

float clampAmount(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}

float onePoleCoefficient(float frequency, double sampleRate) {
    const float bounded = std::clamp(frequency, 5.0f, static_cast<float>(sampleRate * 0.45));
    return 1.0f - std::exp(-2.0f * pi * bounded / static_cast<float>(sampleRate));
}

bool needsDelay(DspEngine::Kind kind) {
    using K = DspEngine::Kind;
    switch (kind) {
        case K::Chorus: case K::Flanger: case K::Vibrato: case K::Tape:
        case K::Grain: case K::Comb: case K::Rotary: case K::Freeze:
        case K::Swirl: case K::Echo: case K::Slap: case K::Ping:
        case K::Stutter: case K::LongEcho: case K::GhostDelay: case K::Orbit:
        case K::Pluck: case K::Flutter: case K::Smear: case K::Drift:
        case K::Delay:
            return true;
        default:
            return false;
    }
}

bool needsCombs(DspEngine::Kind kind) {
    return kind == DspEngine::Kind::Reverb || kind == DspEngine::Kind::Room
        || kind == DspEngine::Kind::Hall || kind == DspEngine::Kind::BrightPlate
        || kind == DspEngine::Kind::DarkHall || kind == DspEngine::Kind::Smear
        || kind == DspEngine::Kind::Bloom || kind == DspEngine::Kind::Choir
        || kind == DspEngine::Kind::Shimmer || kind == DspEngine::Kind::Stage;
}
}

DspEngine::DspEngine() = default;

void DspEngine::prepare(double rate) {
    sampleRate = std::isfinite(rate) && rate >= 8000.0 && rate <= 768000.0 ? rate : 44100.0;
    reset();
    const auto maxDelay = static_cast<size_t>(std::max(2048.0, sampleRate * 0.8));
    for (int i = 0; i < effectCount; ++i) {
        auto& state = states[static_cast<size_t>(i)];
        const auto kind = kindFor(i);
        if (needsDelay(kind))
            state.delay.assign(maxDelay, {0.0f, 0.0f});
        if (needsCombs(kind)) {
            constexpr std::array<float, 4> seconds{0.0297f, 0.0371f, 0.0411f, 0.0437f};
            for (size_t line = 0; line < seconds.size(); ++line) {
                const float roomScale = 0.9f + static_cast<float>(i) * 0.0005f;
                const auto length = static_cast<size_t>(std::max(31.0f, seconds[line] * roomScale * static_cast<float>(sampleRate)));
                state.comb[line].assign(length, 0.0f);
            }
        }
    }
}

void DspEngine::reset() {
    for (auto& state : states) {
        state.low = {};
        state.low2 = {};
        state.previous = {};
        state.allpass = {};
        state.envelope = {};
        state.held = {};
        state.combPosition = {};
        state.delayPosition = 0;
        state.bufferedSamples = 0;
        state.phase = 0.0f;
        state.mix = 0.0f;
        state.currentAmount = 0.0f;
        state.crusherCounter = 0.0f;
        for (auto& line : state.comb)
            std::fill(line.begin(), line.end(), 0.0f);
        for (auto& frame : state.delay)
            frame = {};
    }
    dcInput = {};
    dcOutput = {};
    limiterGain = 1.0f;
}

DspEngine::Kind DspEngine::kindFor(int index) {
    using K = Kind;
    // The table is intentionally indexed in the same order as featureIds. No
    // substring guesses or hash-derived pseudo-algorithms are used at runtime.
    static constexpr std::array<K, effectCount> recipes{{
        K::Drive, K::Chorus, K::Delay, K::Reverb, K::Width, K::HighPass, K::LowPass, K::Tremolo, K::AutoPan, K::Flanger,
        K::Phaser, K::Crush, K::Vibrato, K::Slap, K::Telephone, K::Tape, K::Stutter, K::Formant, K::BrightPlate, K::Comb,
        K::Compressor, K::Warmth, K::Delay, K::Fade, K::Grain, K::Saturate, K::Chorus, K::Rotary, K::Echo, K::Delay,
        K::Air, K::Grit, K::Haze, K::Grit, K::Phaser, K::Choir, K::Delay, K::Reverb, K::Pulse, K::Haze,
        K::Tape, K::Ink, K::Limiter, K::Saturate, K::Tilt, K::Bass, K::Air, K::Presence, K::Haze, K::Notch,
        K::Notch, K::Width, K::MonoBass, K::Room, K::Hall, K::Exciter, K::Warmth, K::Haze, K::Duck, K::Shimmer,
        K::Chorus, K::Focus, K::Gain, K::Width, K::Sheen, K::Saturate, K::Grit, K::Bloom, K::Swirl, K::Ping,
        K::Drift, K::Snap, K::Body, K::AirLift, K::SubBoost, K::Notch, K::RingMod, K::DeepPhase, K::LongEcho, K::Freeze,
        K::Vinyl, K::Radio, K::Underwater, K::Glass, K::Pulse, K::AutoPan, K::Cream, K::Reverb, K::Compressor, K::Limiter,
        K::Drive, K::Air, K::Bass, K::Tilt, K::Width, K::MonoBass, K::LowPass, K::HighPass, K::Sparkle, K::Warmth,
        K::Compressor, K::Warmth, K::Bass, K::Reverb, K::Width, K::Limiter, K::Tilt, K::Clean, K::Stage, K::Wire,
        K::Compressor, K::Shelf, K::Gain, K::SubBoost, K::Presence, K::Notch, K::Notch, K::Air, K::SleepGate, K::Room,
        K::Acid, K::GhostDelay, K::BoneEQ, K::Warmth, K::Cinder, K::Shelf, K::Glass, K::Drive, K::Hall, K::Room,
        K::Echo, K::Tape, K::Reverb, K::Delay, K::Orbit, K::Choir, K::Pluck, K::DrumGlue, K::Bass, K::HatAir,
        K::Snap, K::Room, K::DarkHall, K::BrightPlate, K::Flanger, K::Tremolo, K::Chorus, K::TightComp, K::OpenComp, K::DeEss,
        K::MonoBass, K::SideAir, K::Crush, K::Tape, K::Flutter, K::Gate16, K::Shimmer, K::OctaveDown, K::OctaveUp, K::Resonant,
        K::Smear, K::Dust, K::Tape, K::Limiter, K::Saturate, K::Air, K::Width, K::Hall, K::Room, K::Delay,
        K::Phaser, K::Chorus, K::Vibrato, K::AutoPan, K::Tremolo, K::LowPass, K::HighPass, K::Drive, K::Grit, K::Haze,
        K::Rotary, K::Saturate, K::Chorus, K::Echo, K::Delay, K::Air, K::Grit, K::Phaser, K::Choir, K::Haze,
        K::Ink, K::Tape, K::Reverb, K::Pulse, K::Delay, K::Orbit, K::Choir, K::BrightPlate, K::Comb, K::Formant
    }};
    return recipes[static_cast<size_t>(std::clamp(index, 0, effectCount - 1))];
}

float DspEngine::sanitize(float value) {
    return std::isfinite(value) ? value : 0.0f;
}

float DspEngine::softClip(float value, float drive) {
    const float normalization = std::tanh(std::max(1.0f, drive));
    return std::tanh(value * drive) / normalization;
}

float DspEngine::processEffect(Kind kind, EffectState& st, int index, float amount,
                               float left, float right, int, float& rightOutput) {
    const float profile = 0.72f + static_cast<float>(index) * 0.0014f;
    const float sr = static_cast<float>(sampleRate);
    const float rate = kind == Kind::Gate16 || kind == Kind::Stutter
        ? 8.0f + static_cast<float>(index % 7) * 1.15f
        : index == 145 ? 7.5f + amount * 2.0f
        : 0.12f + static_cast<float>((index * 11) % 17) * 0.105f;
    const float phaseL = st.phase;
    const float phaseR = std::fmod(phaseL + 0.5f, 1.0f);
    const float sinL = std::sin(2.0f * pi * phaseL);
    const float sinR = std::sin(2.0f * pi * phaseR);
    const float cutoff = 120.0f + static_cast<float>(index) * 14.0f + (1.0f - amount) * 1600.0f;
    const float alpha = onePoleCoefficient(cutoff, sampleRate);
    float outL = left;
    float outR = right;

    switch (kind) {
        case Kind::Drive: case Kind::Saturate: case Kind::Warmth:
        case Kind::Grit: case Kind::Acid: case Kind::Cinder: case Kind::Ink:
        case Kind::Snap: case Kind::Dust: case Kind::Vinyl: case Kind::Cream:
        {
            const float drive = 1.0f + amount * profile * (kind == Kind::Grit || kind == Kind::Acid ? 5.0f : 2.8f);
            if (kind == Kind::Ink) {
                const float bias = amount * 0.14f;
                const float biasOutput = softClip(bias, drive);
                outL = softClip(left + bias, drive) - biasOutput;
                outR = softClip(right + bias, drive) - biasOutput;
            } else {
                outL = softClip(left, drive);
                outR = softClip(right, drive);
            }
            if (kind == Kind::Warmth || kind == Kind::Vinyl) {
                st.low[0] += onePoleCoefficient(180.0f, sampleRate) * (left - st.low[0]);
                st.low[1] += onePoleCoefficient(180.0f, sampleRate) * (right - st.low[1]);
                outL = 0.82f * outL + 0.18f * st.low[0];
                outR = 0.82f * outR + 0.18f * st.low[1];
            }
            break;
        }
        case Kind::HighPass: case Kind::Telephone: case Kind::Radio: case Kind::Clean:
            st.low[0] += alpha * (left - st.low[0]);
            st.low[1] += alpha * (right - st.low[1]);
            outL = left - st.low[0];
            outR = right - st.low[1];
            if (kind == Kind::Telephone || kind == Kind::Radio) {
                st.low2[0] += onePoleCoefficient(2800.0f, sampleRate) * (outL - st.low2[0]);
                st.low2[1] += onePoleCoefficient(2800.0f, sampleRate) * (outR - st.low2[1]);
                outL = st.low2[0];
                outR = st.low2[1];
                st.low[0] += onePoleCoefficient(420.0f, sampleRate) * (left - st.low[0]);
                st.low[1] += onePoleCoefficient(420.0f, sampleRate) * (right - st.low[1]);
                outL = st.low2[0] - st.low[0] * 0.72f;
                outR = st.low2[1] - st.low[1] * 0.72f;
            }
            break;
        case Kind::LowPass: case Kind::Haze: case Kind::Underwater:
        case Kind::Fog:
            st.low[0] += alpha * (left - st.low[0]);
            st.low[1] += alpha * (right - st.low[1]);
            outL = st.low[0];
            outR = st.low[1];
            break;
        case Kind::Air: case Kind::AirLift: case Kind::HatAir:
        case Kind::SideAir: case Kind::Wire: case Kind::Sheen:
        case Kind::Sparkle:
            st.low[0] += onePoleCoefficient(1800.0f, sampleRate) * (left - st.low[0]);
            st.low[1] += onePoleCoefficient(1800.0f, sampleRate) * (right - st.low[1]);
            outL = left + (left - st.low[0]) * amount * 1.1f;
            outR = right + (right - st.low[1]) * amount * 1.1f;
            break;
        case Kind::Bass: case Kind::SubBoost: case Kind::Body: case Kind::Focus:
            st.low[0] += onePoleCoefficient(110.0f + index % 90, sampleRate) * (left - st.low[0]);
            st.low[1] += onePoleCoefficient(110.0f + index % 90, sampleRate) * (right - st.low[1]);
            if (kind == Kind::MonoBass)
                st.low[0] = st.low[1] = 0.5f * (st.low[0] + st.low[1]);
            outL = left + st.low[0] * amount * (kind == Kind::SubBoost ? 1.05f : 0.65f);
            outR = right + st.low[1] * amount * (kind == Kind::SubBoost ? 1.05f : 0.65f);
            break;
        case Kind::Notch: case Kind::Formant: case Kind::BoneEQ:
        case Kind::DeEss: case Kind::Glass:
            st.low[0] += alpha * (left - st.low[0]);
            st.low[1] += alpha * (right - st.low[1]);
            st.low2[0] += onePoleCoefficient(kind == Kind::DeEss ? 6500.0f : cutoff * 1.8f, sampleRate) * (left - st.low2[0]);
            st.low2[1] += onePoleCoefficient(kind == Kind::DeEss ? 6500.0f : cutoff * 1.8f, sampleRate) * (right - st.low2[1]);
            if (kind == Kind::DeEss) {
                const float sibilanceL = left - st.low2[0];
                const float sibilanceR = right - st.low2[1];
                outL = left - sibilanceL * amount * (std::abs(sibilanceL) > 0.06f ? 0.72f : 0.2f);
                outR = right - sibilanceR * amount * (std::abs(sibilanceR) > 0.06f ? 0.72f : 0.2f);
            } else {
                const float bandL = st.low2[0] - st.low[0];
                const float bandR = st.low2[1] - st.low[1];
                const float depth = amount * (kind == Kind::Formant ? 0.85f : kind == Kind::BoneEQ ? 0.35f : 0.55f);
                outL = kind == Kind::Glass ? left + bandL * depth : left - bandL * depth;
                outR = kind == Kind::Glass ? right + bandR * depth : right - bandR * depth;
            }
            break;
        case Kind::Tilt: case Kind::Shelf: case Kind::Presence:
            st.low[0] += onePoleCoefficient(950.0f, sampleRate) * (left - st.low[0]);
            st.low[1] += onePoleCoefficient(950.0f, sampleRate) * (right - st.low[1]);
            outL = left + (st.low[0] * 2.0f - left) * amount * 0.22f;
            outR = right + (st.low[1] * 2.0f - right) * amount * 0.22f;
            break;
        case Kind::Chorus: case Kind::Flanger: case Kind::Vibrato:
        case Kind::Tape: case Kind::Flutter: case Kind::Rotary:
        case Kind::Orbit: case Kind::Swirl: case Kind::Drift:
        case Kind::Grain: case Kind::Smear:
            if (!st.delay.empty()) {
                const float depthMs = (kind == Kind::Flanger ? 1.2f + amount * 4.0f
                    : kind == Kind::Vibrato ? 8.0f + amount * 28.0f
                    : 5.0f + amount * 14.0f) * profile;
                const float centerMs = (kind == Kind::Flanger ? 4.0f : kind == Kind::Tape ? 18.0f : 16.0f) * profile;
                const float offsetL = (centerMs + depthMs * (0.5f + 0.5f * sinL)) * sr * 0.001f;
                const float offsetR = (centerMs + depthMs * (0.5f + 0.5f * sinR)) * sr * 0.001f;
                const auto p = st.delayPosition;
                st.delay[p] = {left, right};
                const auto read = [&](float delay) {
                    const float bounded = std::clamp(delay, 1.0f, static_cast<float>(st.delay.size() - 2));
                    const float pos = static_cast<float>(p) - bounded;
                    const auto base = static_cast<long long>(std::floor(pos));
                    const float frac = pos - static_cast<float>(base);
                    const auto wrap = [&](long long n) {
                        const auto size = static_cast<long long>(st.delay.size());
                        return static_cast<size_t>((n % size + size) % size);
                    };
                    const float a = st.delay[wrap(base)][0];
                    const float b = st.delay[wrap(base + 1)][0];
                    return a + frac * (b - a);
                };
                const auto readR = [&](float delay) {
                    const float bounded = std::clamp(delay, 1.0f, static_cast<float>(st.delay.size() - 2));
                    const float pos = static_cast<float>(p) - bounded;
                    const auto base = static_cast<long long>(std::floor(pos));
                    const float frac = pos - static_cast<float>(base);
                    const auto wrap = [&](long long n) {
                        const auto size = static_cast<long long>(st.delay.size());
                        return static_cast<size_t>((n % size + size) % size);
                    };
                    const float a = st.delay[wrap(base)][1];
                    const float b = st.delay[wrap(base + 1)][1];
                    return a + frac * (b - a);
                };
                const float wetL = read(offsetL);
                const float wetR = readR(offsetR);
                const float wetMix = kind == Kind::Vibrato ? 0.9f : 0.52f;
                outL = left * (1.0f - wetMix) + wetL * wetMix;
                outR = right * (1.0f - wetMix) + wetR * wetMix;
                st.delayPosition = (p + 1) % st.delay.size();
            }
            break;
        case Kind::Delay: case Kind::Echo: case Kind::Slap: case Kind::Ping:
        case Kind::LongEcho: case Kind::GhostDelay: case Kind::Pluck:
        case Kind::Freeze: case Kind::Comb:
            if (!st.delay.empty()) {
                const float seconds = (kind == Kind::Comb ? 0.014f
                    : kind == Kind::Slap || kind == Kind::Pluck ? 0.055f
                    : kind == Kind::LongEcho || kind == Kind::GhostDelay ? 0.42f
                    : kind == Kind::Freeze ? 0.085f : 0.19f) * profile;
                const size_t lag = std::clamp(static_cast<size_t>(seconds * sr), size_t(1), st.delay.size() - 1);
                const size_t read = (st.delayPosition + st.delay.size() - lag) % st.delay.size();
                const auto delayed = st.delay[read];
                const float feedback = kind == Kind::Freeze ? 0.72f : kind == Kind::LongEcho || kind == Kind::GhostDelay ? 0.47f : 0.28f;
                st.delay[st.delayPosition] = {
                    sanitize(left + delayed[0] * feedback),
                    sanitize(right + delayed[1] * feedback)
                };
                outL = left * 0.68f + delayed[0] * 0.32f;
                outR = right * 0.68f + delayed[1] * 0.32f;
                if (kind == Kind::Comb) {
                    outL = softClip(left + delayed[0] * amount * 0.7f, 1.2f);
                    outR = softClip(right + delayed[1] * amount * 0.7f, 1.2f);
                }
                st.delayPosition = (st.delayPosition + 1) % st.delay.size();
            }
            break;
        case Kind::Reverb: case Kind::Room: case Kind::Hall:
        case Kind::BrightPlate: case Kind::DarkHall: case Kind::Stage:
        case Kind::Bloom: case Kind::Choir: case Kind::Shimmer:
            if (needsCombs(kind)) {
                float wetL = 0.0f;
                float wetR = 0.0f;
                for (size_t line = 0; line < st.comb.size(); ++line) {
                    auto& comb = st.comb[line];
                    if (comb.empty()) continue;
                    const size_t p = st.combPosition[line];
                    const float delayedL = comb[p];
                    const float delayedR = comb[p];
                    const float feedback = 0.56f + profile * 0.08f - static_cast<float>(line) * 0.045f;
                    float feed = 0.5f * (left + right);
                    if (kind == Kind::DarkHall) {
                        st.low[0] += onePoleCoefficient(3200.0f, sampleRate) * (left - st.low[0]);
                        st.low[1] += onePoleCoefficient(3200.0f, sampleRate) * (right - st.low[1]);
                        feed = 0.5f * (st.low[0] + st.low[1]);
                    } else if (kind == Kind::Shimmer || kind == Kind::BrightPlate) {
                        st.low[0] += onePoleCoefficient(2400.0f, sampleRate) * (feed - st.low[0]);
                        const float brightness = kind == Kind::Shimmer ? 1.25f : 0.82f;
                        feed = st.low[0] * 0.38f + (feed - st.low[0]) * brightness;
                    }
                    comb[p] = sanitize(feed + delayedL * feedback);
                    st.combPosition[line] = (p + 1) % comb.size();
                    wetL += delayedL * (line % 2 == 0 ? 0.30f : 0.20f);
                    wetR += delayedR * (line % 2 == 0 ? 0.20f : 0.30f);
                }
                const float wet = kind == Kind::Hall || kind == Kind::DarkHall ? 0.44f : 0.34f;
                outL = left * (1.0f - wet) + wetL * wet;
                outR = right * (1.0f - wet) + wetR * wet;
            }
            break;
        case Kind::Phaser: case Kind::DeepPhase: case Kind::Resonant:
            for (int ch = 0; ch < 2; ++ch) {
                const float in = ch == 0 ? left : right;
                float y = in;
                const float sweep = 0.15f + 0.72f * (0.5f + 0.5f * (ch == 0 ? sinL : sinR));
                const float coeff = std::clamp((0.15f + amount * 0.68f) * sweep * profile, 0.08f, 0.92f);
                for (int stage = 0; stage < 4; ++stage) {
                    const float stageCoeff = std::clamp(coeff * (0.75f + stage * 0.055f), 0.05f, 0.94f);
                    const size_t stateIndex = static_cast<size_t>(ch * 4 + stage);
                    const float next = -stageCoeff * y + st.allpass[stateIndex];
                    st.allpass[stateIndex] = y + stageCoeff * next;
                    y = next;
                }
                if (ch == 0) outL = 0.58f * in + 0.42f * y;
                else outR = 0.58f * in + 0.42f * y;
            }
            break;
        case Kind::Stutter:
            if (!st.delay.empty()) {
                const size_t period = std::clamp(static_cast<size_t>(sr / rate), size_t(1), st.delay.size() - 1);
                if (st.bufferedSamples < period) {
                    st.delay[st.bufferedSamples++] = {left, right};
                } else {
                    const size_t readPosition = std::min(static_cast<size_t>(phaseL * static_cast<float>(period)), period - 1);
                    const auto repeated = st.delay[readPosition];
                    const float phaseDistance = std::min(phaseL, 1.0f - phaseL);
                    const float seam = std::clamp(phaseDistance / 0.045f, 0.0f, 1.0f);
                    const float repeatMix = (0.36f + amount * 0.42f) * seam;
                    outL = left + (repeated[0] - left) * repeatMix;
                    outR = right + (repeated[1] - right) * repeatMix;
                }
            }
            break;
        case Kind::Tremolo: case Kind::Pulse: case Kind::Gate16:
        case Kind::SleepGate: case Kind::Duck: case Kind::Gate:
        case Kind::Fade:
            if (kind == Kind::Duck) {
                const float detector = std::max(std::abs(left), std::abs(right));
                const float coefficient = detector > st.envelope[0]
                    ? onePoleCoefficient(120.0f, sampleRate) : onePoleCoefficient(5.0f, sampleRate);
                st.envelope[0] += coefficient * (detector - st.envelope[0]);
                const float gain = 1.0f - amount * std::min(0.68f, st.envelope[0] * 1.8f);
                outL *= gain;
                outR *= gain;
            } else if (kind == Kind::Gate) {
                const float detector = std::max(std::abs(left), std::abs(right));
                const float coefficient = detector > st.envelope[0]
                    ? onePoleCoefficient(100.0f, sampleRate) : onePoleCoefficient(8.0f, sampleRate);
                st.envelope[0] += coefficient * (detector - st.envelope[0]);
                const float threshold = 0.01f + amount * 0.08f;
                const float gate = std::clamp((st.envelope[0] - threshold * 0.45f) / (threshold * 0.55f), 0.0f, 1.0f);
                const float gain = 1.0f - amount * 0.92f * (1.0f - gate);
                outL *= gain;
                outR *= gain;
            } else {
                float modulation = 0.5f + 0.5f * sinL;
                if (kind == Kind::Gate16 || kind == Kind::SleepGate) {
                    const float local = std::fmod(phaseL, 1.0f);
                    const float edge = 0.025f;
                    const float gate = local < 0.5f - edge ? 1.0f
                        : local < 0.5f + edge ? 0.5f + 0.5f * std::cos(pi * (local - (0.5f - edge)) / (2.0f * edge))
                        : 0.0f;
                    modulation = kind == Kind::SleepGate ? std::max(0.24f, gate) : gate;
                }
                const float gain = 1.0f - amount * profile * (kind == Kind::Gate16 || kind == Kind::SleepGate ? 0.96f : 0.78f) * (1.0f - modulation);
                outL *= gain;
                outR *= gain;
            }
            break;
        case Kind::AutoPan:
            outL *= 0.5f + 0.5f * sinL * amount * profile;
            outR *= 0.5f - 0.5f * sinL * amount * profile;
            break;
        case Kind::Width: case Kind::Image:
            {
                const float mid = 0.5f * (left + right);
                const float side = 0.5f * (left - right) * (1.0f + amount * profile);
                outL = mid + side;
                outR = mid - side;
            }
            break;
        case Kind::Crush:
            {
                const float holdPeriod = 1.0f + amount * (1.0f + static_cast<float>(index % 10));
                st.crusherCounter += 1.0f;
                if (st.crusherCounter >= holdPeriod) {
                    st.crusherCounter = 0.0f;
                    const float steps = 12.0f + (1.0f - amount) * 500.0f + (1.0f - profile) * 20.0f;
                    st.held[0] = std::round(left * steps) / steps;
                    st.held[1] = std::round(right * steps) / steps;
                }
                outL = st.held[0];
                outR = st.held[1];
            }
            break;
        case Kind::Compressor: case Kind::DrumGlue: case Kind::TightComp:
        case Kind::OpenComp: case Kind::Limiter:
            {
                const float threshold = kind == Kind::Limiter
                    ? 0.36f + (1.0f - profile) * 0.18f
                    : (kind == Kind::TightComp ? 0.25f : kind == Kind::OpenComp ? 0.43f : 0.34f)
                      - amount * (kind == Kind::OpenComp ? 0.13f : 0.24f) + (1.0f - profile) * 0.08f;
                const float attackFrequency = kind == Kind::Limiter ? 180.0f
                    : kind == Kind::TightComp ? 35.0f : 18.0f;
                const float releaseFrequency = kind == Kind::DrumGlue ? 2.2f
                    : kind == Kind::OpenComp ? 3.5f : kind == Kind::TightComp ? 8.0f : 5.0f;
                const float attack = onePoleCoefficient(attackFrequency, sampleRate);
                const float release = onePoleCoefficient(releaseFrequency, sampleRate);
                const float level = std::max(std::abs(left), std::abs(right));
                auto& envelope = st.envelope[0];
                envelope += (level > envelope ? attack : release) * (level - envelope);
                const float ratio = kind == Kind::Limiter ? 8.0f
                    : kind == Kind::TightComp ? 5.0f : kind == Kind::OpenComp ? 1.8f
                    : kind == Kind::DrumGlue ? 3.4f : 2.4f + amount * 3.0f;
                const float compressed = envelope > threshold ? threshold + (envelope - threshold) / ratio : envelope;
                const float gain = envelope > 1.0e-6f ? compressed / envelope : 1.0f;
                outL = left * gain;
                outR = right * gain;
            }
            break;
        case Kind::RingMod: case Kind::OctaveDown: case Kind::OctaveUp:
            {
                const float carrier = std::sin(2.0f * pi * st.carrierPhase);
                st.carrierPhase += (70.0f + static_cast<float>(index) * 2.7f) / sr;
                if (st.carrierPhase >= 1.0f)
                    st.carrierPhase -= 1.0f;
                const float mix = amount * 0.75f * profile;
                outL = left * (1.0f - mix) + left * carrier * mix;
                outR = right * (1.0f - mix) + right * carrier * mix;
            }
            break;
        case Kind::Exciter:
            st.low[0] += onePoleCoefficient(1200.0f, sampleRate) * (left - st.low[0]);
            st.low[1] += onePoleCoefficient(1200.0f, sampleRate) * (right - st.low[1]);
            outL = left + softClip(left - st.low[0], 2.5f) * amount * 0.45f;
            outR = right + softClip(right - st.low[1], 2.5f) * amount * 0.45f;
            break;
        case Kind::Gain: case Kind::Trim:
            outL = left * (1.0f + (amount - 0.5f) * 0.28f);
            outR = right * (1.0f + (amount - 0.5f) * 0.28f);
            break;
        case Kind::Balance:
            outL = left * (1.0f - amount * 0.35f);
            outR = right * (1.0f + amount * 0.35f);
            break;
        default:
            outL = softClip(left + (left - st.previous[0]) * amount * 0.18f, 1.2f);
            outR = softClip(right + (right - st.previous[1]) * amount * 0.18f, 1.2f);
            break;
    }

    const float phaseAdvance = rate / sr;
    st.phase += phaseAdvance;
    if (st.phase >= 1.0f)
        st.phase -= std::floor(st.phase);
    st.previous = {left, right};
    rightOutput = sanitize(outR);
    return sanitize(outL);
}

void DspEngine::process(float* left, float* right, int channels, int samples,
                        const std::array<bool, effectCount>& enabled,
                        const std::array<float, effectCount>& amounts,
                        const std::array<float, effectCount>& tone,
                        const std::array<float, effectCount>& motion,
                        const std::array<float, effectCount>& mix,
                        const std::array<float, effectCount>& shape,
                        const std::array<int, effectCount>* effectOrder) {
    if (left == nullptr || channels < 1 || samples <= 0)
        return;
    const bool stereo = channels > 1 && right != nullptr;
    const float mixCoefficient = onePoleCoefficient(1.0f / (0.012f * 2.0f * pi), sampleRate);
    const float amountCoefficient = onePoleCoefficient(1.0f / (0.02f * 2.0f * pi), sampleRate);
    const float dcCoefficient = std::exp(-2.0f * pi * 5.0f / static_cast<float>(sampleRate));
    const float limiterRelease = onePoleCoefficient(8.0f, sampleRate);

    for (int sample = 0; sample < samples; ++sample) {
        float l = sanitize(left[sample]);
        float r = stereo ? sanitize(right[sample]) : l;

        for (int orderIndex = 0; orderIndex < effectCount; ++orderIndex) {
            const int i = effectOrder != nullptr ? (*effectOrder)[static_cast<size_t>(orderIndex)] : orderIndex;
            if (i < 0 || i >= effectCount)
                continue;
            auto& state = states[static_cast<size_t>(i)];
            const float targetMix = enabled[static_cast<size_t>(i)] ? 1.0f : 0.0f;
            state.mix += (targetMix - state.mix) * mixCoefficient;
            const auto slot = static_cast<size_t>(i);
            const float targetAmount = clampAmount(amounts[slot]);
            state.currentAmount += (targetAmount - state.currentAmount) * amountCoefficient;
            if (state.mix < 1.0e-5f)
                continue;
            const float toneValue = clampAmount(tone[slot]);
            const float motionValue = clampAmount(motion[slot]);
            const float mixValue = clampAmount(mix[slot]);
            const float shapeValue = clampAmount(shape[slot]);
            l = sanitize(l);
            r = sanitize(r);
            const float toneGain = 0.72f + toneValue * 0.56f;
            const float motionGain = 0.78f + motionValue * 0.44f;
            const float shapeDrive = 1.0f + (shapeValue - 0.5f) * 0.42f;
            const float preL = l * toneGain * shapeDrive;
            const float preR = r * toneGain * shapeDrive;
            state.phase += (motionGain - 1.0f) * 0.00013f;
            if (state.phase >= 1.0f) state.phase -= 1.0f;
            if (state.phase < 0.0f) state.phase += 1.0f;
            float wetRight = preR;
            const float wetLeft = processEffect(kindFor(i), state, i, state.currentAmount, preL, preR, 0, wetRight);
            const float wet = state.mix * state.currentAmount
                * (0.84f + 0.16f * (0.72f + static_cast<float>(i) * 0.0014f))
                * std::clamp(0.15f + mixValue * 0.85f, 0.05f, 1.0f);
            l = sanitize(l + (wetLeft - l) * wet);
            r = sanitize(r + (wetRight - r) * wet);
        }

        const float dcL = l - dcInput[0] + dcCoefficient * dcOutput[0];
        const float dcR = r - dcInput[1] + dcCoefficient * dcOutput[1];
        dcInput = {l, r};
        dcOutput = {sanitize(dcL), sanitize(dcR)};
        const float peak = std::max(std::abs(dcL), std::abs(dcR));
        const float targetGain = peak > 0.96f ? 0.96f / peak : 1.0f;
        if (targetGain < limiterGain)
            limiterGain = targetGain;
        else
            limiterGain += (targetGain - limiterGain) * limiterRelease;
        left[sample] = sanitize(dcL * limiterGain);
        if (stereo)
            right[sample] = sanitize(dcR * limiterGain);
    }
}

} // namespace dm
