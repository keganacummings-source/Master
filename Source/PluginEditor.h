#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class DreamMasterLiteEditor : public juce::AudioProcessorEditor {
public:
    explicit DreamMasterLiteEditor(DreamMasterLiteProcessor&);
    ~DreamMasterLiteEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    DreamMasterLiteProcessor& proc;
    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<juce::Component> effectCards;
    juce::OwnedArray<juce::ToggleButton> effectButtons, favoriteButtons;
    juce::OwnedArray<juce::Slider> amountSliders, control2Sliders, control3Sliders;
    juce::OwnedArray<juce::Label> effectLabels, control1Labels, control2Labels, control3Labels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;
    juce::TextButton randomButton{"RANDOM FX PICK (MAX 10)"}, resetButton{"ALL OFF"}, websiteButton{"DREAMDAW.COM ↗"};
    juce::Label title, subtitle, countLabel;
    std::unique_ptr<juce::PropertiesFile> favoritesFile;
    void randomize();
    void resetAll();
    void saveFavorite(int index, bool enabled);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteEditor)
};
