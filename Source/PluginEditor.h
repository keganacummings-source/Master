#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <vector>

class DreamMasterLiteEditor : public juce::AudioProcessorEditor,
                              private juce::Timer,
                              private juce::ListBoxModel {
public:
    explicit DreamMasterLiteEditor(DreamMasterLiteProcessor&);
    ~DreamMasterLiteEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct BuilderStep {
        int effectIndex = 0;
        float amount = 0.22f;
    };
    struct WorkerReply {
        juce::var payload;
        juce::String error;
        int statusCode = 0;
    };

    DreamMasterLiteProcessor& proc;
    juce::Viewport viewport;
    juce::Component content, builderContent;
    juce::OwnedArray<juce::ToggleButton> effectButtons;
    juce::OwnedArray<juce::Slider> amountSliders;
    juce::OwnedArray<juce::Label> effectLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;
    juce::TextButton randomButton{"RANDOM FX"}, extremeButton{"XTRMRND"}, definedButton{"DEFINED VALUES"};
    juce::TextButton resetButton{"ALL OFF"}, websiteButton{"DREAMDAW.COM"};
    juce::TextButton rackPageButton{"FX RACK"}, builderPageButton{"FX BUILDER"};
    juce::TextButton loginButton{"LOG IN"}, logoutButton{"LOG OUT"};
    juce::Label title, subtitle, countLabel, onlineLabel, accountLabel, builderLockLabel, builderStatusLabel;
    juce::TextEditor usernameEditor, passwordEditor, presetNameEditor;
    juce::ComboBox categoryBox, themeBox, effectChoiceBox, presetChoiceBox;
    juce::Slider builderAmountSlider;
    juce::TextButton addEffectButton{"ADD MODULE"}, moveUpButton{"MOVE UP"}, moveDownButton{"MOVE DOWN"};
    juce::TextButton removeEffectButton{"REMOVE"}, savePresetButton{"SAVE PRESET"};
    juce::TextButton loadPresetButton{"LOAD PRESET"}, deletePresetButton{"DELETE PRESET"};
    juce::ListBox chainList{"Builder chain", this};
    std::vector<int> visibleEffects;
    std::vector<BuilderStep> builderSteps;
    std::vector<dm::CustomPreset> availablePresets;
    juce::StringArray themeIds, themeNames;
    juce::var currentThemePack;
    juce::Colour backgroundColour{0xff030805}, panelColour{0xff100a08}, accentColour{0xff65ff83};
    juce::Colour highlightColour{0xffc8ff33}, textColour{0xffd8ffe0}, mutedColour{0xffb5a18a};
    juce::Colour borderColour{0xff563526};
    bool builderPageActive = false;
    bool authenticated = false;
    bool builderAvailable = false;
    juce::uint32 lastHeartbeatMs = 0;
    juce::uint32 lastOnlinePollMs = 0;

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void timerCallback() override;
    void randomize(int minimum, int maximum, bool definedValues);
    void resetAll();
    void updateVisibleEffects();
    void showPage(bool builder);
    void updateAuthenticationUi();
    void updateActiveCount();
    void updatePresetList();
    void addBuilderEffect();
    void saveBuilderPreset();
    void loadSelectedPreset();
    void deleteSelectedPreset();
    void moveSelectedEffect(int direction);
    void updateBuilderAmount();
    void login();
    void logout();
    void validateSession();
    void refreshCapabilities();
    void refreshOnlineCount();
    void sendPresenceHeartbeat();
    void selectTheme();
    void populateThemes(const juce::var& themes);
    void applyThemePack(const juce::var& themePack);
    WorkerReply requestWorker(const juce::var* request, bool get = false);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteEditor)
};
