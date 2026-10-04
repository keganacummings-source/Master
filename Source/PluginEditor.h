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
    juce::OwnedArray<juce::ToggleButton> effectButtons;
    juce::OwnedArray<juce::Slider> amountSliders;
    juce::OwnedArray<juce::Label> effectLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;
    juce::TextButton randomButton{"RANDOM FX PICK (MAX 10)"}, resetButton{"ALL OFF"}, websiteButton{"DREAMDAW.COM ↗"};
    juce::Label title, subtitle, countLabel;
    void randomize();
    void resetAll();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteEditor)
};
