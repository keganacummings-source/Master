#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include <string>

namespace {
enum class FxType { Gain, LowPass, HighPass, Air, Bass, Saturation, Limiter, Delay, Width, Modulation, LoFi, Gate, Tilt, Color };
FxType classify(const std::string& id) {
    auto has = [&](const char* s) { return id.find(s) != std::string::npos; };
    if (has("hpf") || has("highpass") || has("polishhp") || has("clean")) return FxType::HighPass;
    if (has("lpf") || has("lowpass") || has("haze") || has("fog") || has("underwater") || has("under")) return FxType::LowPass;
    if (has("air") || has("sheen") || has("sparkle") || has("wire") || has("presence") || has("edge")) return FxType::Air;
    if (has("bass") || has("sub") || has("lowend") || has("body") || has("punch") || has("mass")) return FxType::Bass;
    if (has("delay") || has("echo") || has("reverb") || has("verb") || has("room") || has("hall") || has("plate") || has("halo") || has("bloom") || has("ripple") || has("ping") || has("stage") || has("orbit") || has("seance") || has("choir") || has("comb") || has("slap") || has("predelay") || has("shimmer")) return FxType::Delay;
    if (has("width") || has("widen") || has("mono") || has("pan") || has("image") || has("balance") || has("stereo")) return FxType::Width;
    if (has("chorus") || has("flanger") || has("phaser") || has("vibrato") || has("trem") || has("pulse") || has("worm") || has("drift") || has("swirl") || has("rotary") || has("vortex") || has("wriggle")) return FxType::Modulation;
    if (has("crush") || has("lofi") || has("radio") || has("phone") || has("telephone") || has("bit")) return FxType::LoFi;
    if (has("gate") || has("stutter") || has("chop") || has("duck")) return FxType::Gate;
    if (has("tilt") || has("curve") || has("mud") || has("debox") || has("notch") || has("silk") || has("carve") || has("shelf") || has("formant")) return FxType::Tilt;
    if (has("drive") || has("grit") || has("sat") || has("clip") || has("rot") || has("melt") || has("cream") || has("velvet") || has("warm") || has("tape") || has("ink") || has("infection") || has("devil") || has("grind")) return FxType::Saturation;
    if (has("limit") || has("ceiling") || has("loud") || has("glue") || has("compress") || has("polish")) return FxType::Limiter;
    if (has("trim") || has("fade") || has("gain") || has("final")) return FxType::Gain;
    return FxType::Color;
}
}

DreamMasterLiteProcessor::DreamMasterLiteProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "PARAMETERS", createParams()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DreamMasterLiteProcessor::createParams() {
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    for (int i = 0; i < 200; ++i) {
        const auto idx = juce::String(i);
        const auto name = juce::String::fromUTF8(dm::featureNames[(size_t)i].data(), static_cast<int>(dm::featureNames[(size_t)i].size()));
        params.add(std::make_unique<juce::AudioParameterBool>("fx" + idx, name, false));
        params.add(std::make_unique<juce::AudioParameterFloat>("amt" + idx, name + " Amount", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.22f));
        params.add(std::make_unique<juce::AudioParameterFloat>("ctrl2" + idx, name + " Control 2", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.50f));
        params.add(std::make_unique<juce::AudioParameterFloat>("ctrl3" + idx, name + " Control 3", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.50f));
    }
    return params;
}

void DreamMasterLiteProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    filterState = {}; previousState = {}; delayPositions = {};
    for (int i = 0; i < 200; ++i) {
        const auto id = std::string(dm::featureIds[(size_t)i]);
        const auto type = classify(id);
        if (type == FxType::Delay) delayBuffers[(size_t)i].assign((size_t)juce::jlimit(2048, 24000, (int)(currentSampleRate * 0.24)), {0.0f, 0.0f});
        else delayBuffers[(size_t)i].clear();
    }
}
void DreamMasterLiteProcessor::releaseResources() {}

bool DreamMasterLiteProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void DreamMasterLiteProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    for (int i = 0; i < 200; ++i) {
        auto* enabled = state.getRawParameterValue("fx" + juce::String(i));
        auto* amountParam = state.getRawParameterValue("amt" + juce::String(i));
        auto* control2Param = state.getRawParameterValue("ctrl2" + juce::String(i));
        auto* control3Param = state.getRawParameterValue("ctrl3" + juce::String(i));
        if (enabled == nullptr || amountParam == nullptr || control2Param == nullptr || control3Param == nullptr || enabled->load() < 0.5f) continue;
        const float amount = juce::jlimit(0.0f, 1.0f, amountParam->load());
        const float control2 = juce::jlimit(0.0f, 1.0f, control2Param->load());
        const float control3 = juce::jlimit(0.0f, 1.0f, control3Param->load());
        const auto id = std::string(dm::featureIds[(size_t)i]);
        const FxType type = classify(id);
        const float unique = float((std::hash<std::string>{}(id) % 97) + 3) / 100.0f;
        auto& st = filterState[(size_t)i];
        auto& prev = previousState[(size_t)i];
        auto& ring = delayBuffers[(size_t)i];
        const float cutoff = 0.001f + control2 * 0.22f;
        for (int s = 0; s < samples; ++s) {
            float l = buffer.getSample(0, s);
            float r = channels > 1 ? buffer.getSample(1, s) : l;
            const float inL = l, inR = r;
            switch (type) {
                case FxType::Gain: { const float g = 1.0f + (amount - 0.22f) * 0.7f; const float trim = 0.75f + control2 * 0.5f; l *= g * trim; r *= g * trim; break; }
                case FxType::LowPass: {
                    st[0] += cutoff * (l - st[0]); st[1] += cutoff * (r - st[1]);
                    const float resonance = control3 * 0.12f;
                    l = l * (1.0f - amount * 0.75f) + st[0] * amount * 0.75f + (l - st[0]) * resonance;
                    r = r * (1.0f - amount * 0.75f) + st[1] * amount * 0.75f + (r - st[1]) * resonance;
                    break;
                }
                case FxType::HighPass:
                    st[0] += cutoff * (l - st[0]); st[1] += cutoff * (r - st[1]); l -= st[0] * amount * (0.2f + control3 * 0.65f); r -= st[1] * amount * (0.2f + control3 * 0.65f); break;
                case FxType::Air: {
                    st[0] += 0.22f * (l - st[0]); st[1] += 0.22f * (r - st[1]);
                    const float tone = 0.04f + control2 * 0.35f; l += (l - st[0]) * amount * tone * (0.4f + control3); r += (r - st[1]) * amount * tone * (0.4f + control3); break;
                }
                case FxType::Bass: {
                    st[0] += 0.025f * (l - st[0]); st[1] += 0.025f * (r - st[1]);
                    const float bassAmount = amount * (0.15f + control3 * 0.55f);
                    l += st[0] * bassAmount; r += st[1] * bassAmount;
                    break;
                }
                case FxType::Saturation: {
                    const float drive = 1.0f + amount * (1.0f + control2 * 7.0f);
                    const float norm = std::tanh(drive);
                    const float mix = control3 * 0.65f;
                    l = (1.0f - mix) * l + mix * (std::tanh(l * drive) / norm);
                    r = (1.0f - mix) * r + mix * (std::tanh(r * drive) / norm); break;
                }
                case FxType::Limiter: {
                    const float threshold = 0.99f - amount * (0.04f + control2 * 0.42f);
                    const float limitedL = std::tanh(l / threshold) * threshold, limitedR = std::tanh(r / threshold) * threshold;
                    l = l * (1.0f - control3) + limitedL * control3; r = r * (1.0f - control3) + limitedR * control3; break;
                }
                case FxType::Delay: {
                    if (!ring.empty()) {
                        const size_t p = delayPositions[(size_t)i];
                        const size_t lag = (size_t)juce::jlimit(1, (int)ring.size() - 1, (int)(ring.size() * (0.02f + control2 * 0.70f)));
                        const size_t rp = (p + ring.size() - lag) % ring.size();
                        const float dl = ring[rp][0], dr = ring[rp][1];
                        ring[p] = {inL + dl * control3 * 0.65f, inR + dr * control3 * 0.65f};
                        delayPositions[(size_t)i] = (p + 1) % ring.size();
                        const float mix = amount * 0.7f;
                        l = inL * (1.0f - mix) + dl * mix;
                        r = inR * (1.0f - mix) + dr * mix;
                    } break;
                }
                case FxType::Width: {
                    const float mid = (l + r) * 0.5f, side = (l - r) * 0.5f * (0.25f + control2 * 2.0f);
                    const float widenedL = mid + side, widenedR = mid - side;
                    l = l * (1.0f - amount * control3) + widenedL * amount * control3; r = r * (1.0f - amount * control3) + widenedR * amount * control3; break;
                }
                case FxType::Modulation: {
                    const float phase = float(s) * (0.002f + control2 * 0.12f) + unique * 6.28f;
                    const float mod = 1.0f - amount * control3 * 0.45f * (0.5f + 0.5f * std::sin(phase));
                    l *= mod; r *= mod; break;
                }
                case FxType::LoFi: {
                    const float bits = 4.0f + control2 * 12.0f;
                    const float steps = std::pow(2.0f, bits);
                    const float mix = amount * control3;
                    l = l * (1.0f - mix) + (std::round(l * steps) / steps) * mix; r = r * (1.0f - mix) + (std::round(r * steps) / steps) * mix; break;
                }
                case FxType::Gate: {
                    const float threshold = 0.001f + control2 * 0.08f;
                    const float attenuation = 1.0f - amount * control3 * 0.95f;
                    if (std::abs(l) < threshold) l *= attenuation;
                    if (std::abs(r) < threshold) r *= attenuation;
                    break;
                }
                case FxType::Tilt: {
                    st[0] += 0.015f * (l - st[0]); st[1] += 0.015f * (r - st[1]);
                    l += (st[0] - (l - st[0])) * amount * 0.12f; r += (st[1] - (r - st[1])) * amount * 0.12f; break;
                }
                case FxType::Color:
                default: {
                    st[0] += cutoff * (l - st[0]); st[1] += cutoff * (r - st[1]);
                    const float toneCutoff = 0.002f + control2 * 0.20f;
                    st[0] += toneCutoff * (l - st[0]); st[1] += toneCutoff * (r - st[1]);
                    const float blend = amount * control3 * 0.35f;
                    l = l * (1.0f - blend) + std::tanh(st[0] * (1.0f + amount * 2.0f)) * blend;
                    r = r * (1.0f - blend) + std::tanh(st[1] * (1.0f + amount * 2.0f)) * blend; break;
                }
            }
            // Conservative per-stage guard prevents runaway peaks when several effects are stacked.
            buffer.setSample(0, s, juce::jlimit(-1.25f, 1.25f, l));
            if (channels > 1) buffer.setSample(1, s, juce::jlimit(-1.25f, 1.25f, r));
            prev[0] = inL; prev[1] = inR;
        }
    }
    // Final safety ceiling. This prevents accidental digital overs, not loudness mastering.
    for (int ch = 0; ch < channels; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int s = 0; s < samples; ++s) data[s] = juce::jlimit(-0.98f, 0.98f, data[s]);
    }
}

void DreamMasterLiteProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DreamMasterLiteProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DreamMasterLiteProcessor(); }
