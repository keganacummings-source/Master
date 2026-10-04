#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "DspEngine.h"
#include "EffectFavorites.h"
#include "FeatureNames.h"
#include "CustomPresetState.h"

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
    std::vector<dm::CustomPreset> getCustomPresets();
    juce::String saveCustomPreset(const juce::String& name, const std::vector<dm::PresetStep>& steps);
    bool deleteCustomPreset(const juce::String& id);
    bool loadCustomPreset(const juce::String& id);
    void setActiveEffectOrder(const std::vector<dm::PresetStep>& steps);
    std::vector<juce::String> getFavoriteEffectIds();
    bool isEffectFavorite(const juce::String& effectId);
    void setEffectFavorite(const juce::String& effectId, bool favorite);
    bool setDreamShareSession(const juce::String& token, const juce::String& user, const juce::String& theme);
    void clearDreamShareSession();
    bool hasDreamShareSession() const { return dreamShareToken.isNotEmpty(); }
    const juce::String& getDreamShareToken() const { return dreamShareToken; }
    const juce::String& getDreamShareUser() const { return dreamShareUser; }
    const juce::String& getDreamShareTheme() const { return dreamShareTheme; }
    void setDreamShareTheme(const juce::String& theme) { dreamShareTheme = theme; }
private:
    dm::DspEngine engine;
    std::array<std::atomic<float>*, 200> enabledParameters{};
    std::array<std::atomic<float>*, 200> amountParameters{};
    std::array<std::atomic<int>, dm::DspEngine::effectCount> processingOrder{};
    juce::String dreamShareToken;
    juce::String dreamShareUser;
    juce::String dreamShareTheme;
    void setProcessingOrder(const std::vector<dm::PresetStep>& steps, bool persistToState);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteProcessor)
};
