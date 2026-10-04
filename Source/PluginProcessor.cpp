#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

DreamMasterLiteProcessor::DreamMasterLiteProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "PARAMETERS", createParams()) {
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto index = juce::String(i);
        enabledParameters[static_cast<size_t>(i)] = state.getRawParameterValue("fx" + index);
        amountParameters[static_cast<size_t>(i)] = state.getRawParameterValue("amt" + index);
        processingOrder[static_cast<size_t>(i)].store(i, std::memory_order_relaxed);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout DreamMasterLiteProcessor::createParams() {
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    for (int i = 0; i < 200; ++i) {
        const auto idx = juce::String(i);
        const auto* name = dm::featureNames[static_cast<size_t>(i)].data();
        params.add(std::make_unique<juce::AudioParameterBool>("fx" + idx, juce::String(name), false));
        params.add(std::make_unique<juce::AudioParameterFloat>("amt" + idx, juce::String(name) + " Amount", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.22f));
    }
    return params;
}

void DreamMasterLiteProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
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
    std::array<bool, dm::DspEngine::effectCount> enabled{};
    std::array<float, dm::DspEngine::effectCount> amounts{};
    std::array<int, dm::DspEngine::effectCount> order{};
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto slot = static_cast<size_t>(i);
        const auto* enabledParameter = enabledParameters[slot];
        const auto* amountParameter = amountParameters[slot];
        enabled[slot] = enabledParameter != nullptr && enabledParameter->load(std::memory_order_relaxed) >= 0.5f;
        amounts[slot] = amountParameter != nullptr ? amountParameter->load(std::memory_order_relaxed) : 0.0f;
        order[slot] = processingOrder[slot].load(std::memory_order_relaxed);
    }
    engine.process(buffer.getWritePointer(0),
                   channels > 1 ? buffer.getWritePointer(1) : nullptr,
                   channels, samples, enabled, amounts, &order);
}

void DreamMasterLiteProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}
void DreamMasterLiteProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType())) {
            auto restoredState = juce::ValueTree::fromXml(*xml);
            state.replaceState(restoredState);
            std::vector<dm::PresetStep> order;
            const auto storedOrder = restoredState.getProperty("activeEffectOrder").toString();
            for (const auto& effectId : juce::StringArray::fromTokens(storedOrder, ",", {}))
                order.push_back({effectId, 0.22f});
            setProcessingOrder(order, false);
        }
}

std::vector<dm::CustomPreset> DreamMasterLiteProcessor::getCustomPresets() {
    std::vector<dm::CustomPreset> presets;
    const auto root = state.copyState();
    const auto container = root.getChildWithName("CUSTOM_PRESETS");
    for (int i = 0; i < container.getNumChildren(); ++i) {
        dm::CustomPreset preset;
        if (dm::readCustomPresetTree(container.getChild(i), preset))
            presets.push_back(std::move(preset));
    }
    return presets;
}

juce::String DreamMasterLiteProcessor::saveCustomPreset(const juce::String& name,
                                                        const std::vector<dm::PresetStep>& steps) {
    const auto trimmedName = name.trim();
    if (trimmedName.isEmpty() || steps.empty() || steps.size() > static_cast<size_t>(dm::DspEngine::effectCount))
        return {};

    dm::CustomPreset preset;
    preset.id = juce::Uuid().toString();
    preset.name = trimmedName;
    std::array<bool, dm::DspEngine::effectCount> seen{};
    for (const auto& step : steps) {
        const auto effectId = step.effectId.toStdString();
        int index = -1;
        for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
            if (effectId == dm::featureIds[static_cast<size_t>(i)]) {
                index = i;
                break;
            }
        }
        if (index < 0 || seen[static_cast<size_t>(index)] || !std::isfinite(step.amount))
            return {};
        seen[static_cast<size_t>(index)] = true;
        preset.steps.push_back({step.effectId, juce::jlimit(0.0f, 1.0f, step.amount)});
    }

    auto root = state.copyState();
    auto container = root.getChildWithName("CUSTOM_PRESETS");
    if (!container.isValid()) {
        container = juce::ValueTree("CUSTOM_PRESETS");
        root.addChild(container, -1, nullptr);
    }
    container.addChild(dm::createCustomPresetTree(preset), -1, nullptr);
    state.replaceState(root);
    return preset.id;
}

bool DreamMasterLiteProcessor::deleteCustomPreset(const juce::String& id) {
    auto root = state.copyState();
    auto container = root.getChildWithName("CUSTOM_PRESETS");
    for (int i = container.getNumChildren() - 1; i >= 0; --i) {
        if (container.getChild(i).getProperty("id").toString() == id) {
            container.removeChild(i, nullptr);
            state.replaceState(root);
            return true;
        }
    }
    return false;
}

bool DreamMasterLiteProcessor::loadCustomPreset(const juce::String& id) {
    const auto presets = getCustomPresets();
    const auto preset = std::find_if(presets.begin(), presets.end(),
                                     [&id](const auto& candidate) { return candidate.id == id; });
    if (preset == presets.end())
        return false;

    std::array<bool, dm::DspEngine::effectCount> seen{};
    for (const auto& step : preset->steps) {
        const auto effectId = step.effectId.toStdString();
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), effectId);
        if (found == dm::featureIds.end())
            return false;
        const auto index = static_cast<size_t>(std::distance(dm::featureIds.begin(), found));
        if (seen[index])
            return false;
        seen[index] = true;
    }

    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (auto* parameter = state.getParameter("fx" + juce::String(i)))
            parameter->setValueNotifyingHost(0.0f);
    }
    for (const auto& step : preset->steps) {
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), step.effectId.toStdString());
        const auto index = static_cast<int>(std::distance(dm::featureIds.begin(), found));
        if (auto* parameter = state.getParameter("fx" + juce::String(index)))
            parameter->setValueNotifyingHost(1.0f);
        if (auto* parameter = state.getParameter("amt" + juce::String(index)))
            parameter->setValueNotifyingHost(step.amount);
    }
    setProcessingOrder(preset->steps, true);
    return true;
}

void DreamMasterLiteProcessor::setActiveEffectOrder(const std::vector<dm::PresetStep>& steps) {
    setProcessingOrder(steps, true);
}

void DreamMasterLiteProcessor::setProcessingOrder(const std::vector<dm::PresetStep>& steps, bool persistToState) {
    std::array<int, dm::DspEngine::effectCount> order{};
    std::array<bool, dm::DspEngine::effectCount> seen{};
    int next = 0;
    for (const auto& step : steps) {
        const auto effectId = step.effectId.toStdString();
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), effectId);
        if (found == dm::featureIds.end())
            continue;
        const int index = static_cast<int>(std::distance(dm::featureIds.begin(), found));
        if (!seen[static_cast<size_t>(index)]) {
            order[static_cast<size_t>(next++)] = index;
            seen[static_cast<size_t>(index)] = true;
        }
    }
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (!seen[static_cast<size_t>(i)])
            order[static_cast<size_t>(next++)] = i;
    }
    for (int i = 0; i < dm::DspEngine::effectCount; ++i)
        processingOrder[static_cast<size_t>(i)].store(order[static_cast<size_t>(i)], std::memory_order_relaxed);
    if (persistToState) {
        juce::StringArray orderedIds;
        for (const auto& step : steps)
            orderedIds.add(step.effectId);
        auto root = state.copyState();
        root.setProperty("activeEffectOrder", orderedIds.joinIntoString(","), nullptr);
        state.replaceState(root);
    }
}

void DreamMasterLiteProcessor::setDreamShareSession(const juce::String& token,
                                                     const juce::String& user,
                                                     const juce::String& theme) {
    dreamShareToken = token;
    dreamShareUser = user;
    dreamShareTheme = theme;
}

void DreamMasterLiteProcessor::clearDreamShareSession() {
    dreamShareToken.clear();
    dreamShareUser.clear();
    dreamShareTheme.clear();
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DreamMasterLiteProcessor(); }
