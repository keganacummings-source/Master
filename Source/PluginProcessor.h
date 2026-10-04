#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "DspEngine.h"
#include "FeatureNames.h"

class DreamMasterLiteProcessor : public juce::AudioProcessor {
public:
    DreamMasterLiteProcessor();
    ~DreamMasterLiteProcessor() override = default;
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "DreamMasterLite"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
private:
    dm::DspEngine engine;
    std::array<std::atomic<float>*, 200> enabledParameters{};
    std::array<std::atomic<float>*, 200> amountParameters{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteProcessor)
};
