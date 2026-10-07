#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class KyotoAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kMaxSlots = 32;
    static constexpr int kMixType = 200;
    static constexpr int kBreakType = 201;
    static constexpr int kMaxChains = kMaxSlots + 1;

    static juce::String chainLevelId(int chain);
    juce::var exportChainLevels() const;
    void restoreChainLevels(const juce::var& state);

    KyotoAudioProcessor();
    ~KyotoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isFx() const;
    int slotCount() const { return isFx() ? 32 : 16; }

    void setHardwareColour(int fxType, float amount);
    void noteOn(int note, float vel);
    void noteOff(int note);
    void loadSample(juce::AudioBuffer<float> buffer, double fileRate);
    void triggerSample();
    double hostBpm() const;
    void setSlotOvermax(int slot, float over);
    float getSlotOvermax(int slot) const;
    void copyScope(float* dest, int n) const;

    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "ui" };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout(bool fx);

private:
    struct Voice
    {
        bool on = false;
        int note = 60;
        float phase = 0.f, sub = 0.f, env = 0.f, vel = 0.8f;
        int stage = 3;
    };

    struct SlotDsp
    {
        float lp[2] {}, hp[2] {}, bp[2] {}, lfo = 0.f;
        std::vector<float> delay[2];
        int write = 0;
    };

    struct SlotParams
    {
        std::atomic<float>* on = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* amount = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* motion = nullptr;
        std::atomic<float>* mix = nullptr;
        std::atomic<float>* shape = nullptr;
    };

    struct BlockSlotConfig
    {
        bool on = false;
        int type = 0;
        float amount = 0.45f, tone = 0.5f, motion = 0.35f, mix = 0.4f, shape = 0.5f;
    };

    static juce::String slotId(int i, const char* tail);
    void cacheParameters();
    float renderVoice(Voice& v);
    void applySlotStereo(int slot, float& left, float& right);
    void processChain(float& left, float& right, float original);
    void rebuildActiveSlots() noexcept;

    Voice voices[8];
    SlotDsp slotDsp[kMaxSlots];
    SlotParams slotParams[kMaxSlots];
    BlockSlotConfig blockConfig[kMaxSlots];
    // Compact ordered list of slots that process audio this block.
    // Built once per block so the per-sample path never walks empty slots.
    int activeSlots[kMaxSlots] {};
    int activeSlotCount = 0;
    bool anyActiveSlot = false;
    std::atomic<float>* perChainLevelsParam = nullptr;
    std::atomic<float>* chainLevelParams[kMaxChains] {};
    float blockChainLevels[kMaxChains] {};
    bool blockPerChainLevels = true;

    std::atomic<float>* oscParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* subParam = nullptr;
    std::atomic<float>* noiseParam = nullptr;

    double sampleRateHz = 44100.0;
    int maxDelaySamples = 0;

    static constexpr int scopeN = 256;
    float scope[scopeN] {};
    std::atomic<int> scopeWrite { 0 };

    std::vector<float> sample;
    double sampleRateFile = 44100.0;
    std::atomic<int> samplePos { -1 };
    juce::CriticalSection sampleLock;
    juce::Random noiseRng;
    std::atomic<int> hardwareFx { -1 };
    std::atomic<float> hardwareAmt { 0.f };
    float slotOvermax[kMaxSlots] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessor)
};
