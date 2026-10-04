#include "PluginProcessor.h"
#include "PluginEditor.h"

DreamMasterLiteProcessor::DreamMasterLiteProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "PARAMETERS", createParams()) {
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto index = juce::String(i);
        enabledParameters[static_cast<size_t>(i)] = state.getRawParameterValue("fx" + index);
        amountParameters[static_cast<size_t>(i)] = state.getRawParameterValue("amt" + index);
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
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto slot = static_cast<size_t>(i);
        const auto* enabledParameter = enabledParameters[slot];
        const auto* amountParameter = amountParameters[slot];
        enabled[slot] = enabledParameter != nullptr && enabledParameter->load(std::memory_order_relaxed) >= 0.5f;
        amounts[slot] = amountParameter != nullptr ? amountParameter->load(std::memory_order_relaxed) : 0.0f;
    }
    engine.process(buffer.getWritePointer(0),
                   channels > 1 ? buffer.getWritePointer(1) : nullptr,
                   channels, samples, enabled, amounts);
}

void DreamMasterLiteProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DreamMasterLiteProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DreamMasterLiteProcessor(); }
