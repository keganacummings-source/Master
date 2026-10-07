#pragma once
#include "PluginProcessor.h"

// Plugin Builder only: the FX Builder retains its own separate stack controls.
class ChainLevelControls final : public juce::Component
{
public:
    explicit ChainLevelControls(KyotoAudioProcessor& p) : proc(p)
    {
        addAndMakeVisible(enabled);
        addAndMakeVisible(chain);
        addAndMakeVisible(level);
        enabled.setClickingTogglesState(true);
        enabled.setTooltip("Per-chain volumes sum directly, without automatic balancing. Off preserves the legacy balanced mix.");
        chain.setTooltip("BREAK starts the next chain. Volumes belong to chain numbers.");
        level.setSliderStyle(juce::Slider::LinearHorizontal);
        level.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 20);
        level.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 1) + "%"; };
        level.valueFromTextFunction = [](const juce::String& v) { return v.getDoubleValue() / 100.0; };
        level.setTooltip("Chain output volume: 0% mutes, 100% is unity. Separate from each effect's wet/dry mix.");
        modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "perChainLevels", enabled);
        chain.onChange = [this] { bindLevel(); };
        refresh();
    }

    void refresh()
    {
        int count = 1;
        for (int i = 0; i < proc.slotCount(); ++i)
        {
            const auto prefix = "s" + juce::String(i + 1).paddedLeft('0', 2);
            if (proc.apvts.getRawParameterValue(prefix + "on")->load() >= 0.5f
                && (int) proc.apvts.getRawParameterValue(prefix + "type")->load() == KyotoAudioProcessor::kBreakType)
                ++count;
        }
        if (count != chain.getNumItems())
        {
            const int selected = juce::jlimit(1, count, chain.getSelectedId());
            chain.clear(juce::dontSendNotification);
            for (int i = 1; i <= count; ++i) chain.addItem("Chain " + juce::String(i), i);
            chain.setSelectedId(selected, juce::dontSendNotification);
            bindLevel();
        }
        level.setEnabled(proc.apvts.getRawParameterValue("perChainLevels")->load() >= 0.5f);
    }

    void resized() override
    {
        auto row = getLocalBounds();
        enabled.setBounds(row.removeFromLeft(126));
        row.removeFromLeft(6);
        chain.setBounds(row.removeFromLeft(92));
        row.removeFromLeft(6);
        level.setBounds(row);
    }

private:
    void bindLevel()
    {
        levelAttachment.reset();
        levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, KyotoAudioProcessor::chainLevelId(chain.getSelectedId() - 1), level);
    }

    KyotoAudioProcessor& proc;
    juce::TextButton enabled { "PER-CHAIN LEVELS" };
    juce::ComboBox chain;
    juce::Slider level;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;
};
