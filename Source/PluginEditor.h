#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

class DreamMasterLiteEditor : public juce::AudioProcessorEditor,
                              private juce::Timer,
                              private juce::ListBoxModel {
public:
    struct WorkerReply {
        juce::var payload;
        juce::String error;
        int statusCode = 0;
    };

    explicit DreamMasterLiteEditor(DreamMasterLiteProcessor&);
    ~DreamMasterLiteEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    class EffectCard;
    class EffectBrowserModel;
    struct BuilderStep {
        int effectIndex = 0;
        std::array<float, 5> controls { 0.22f, 0.50f, 0.50f, 0.65f, 0.50f };
    };
    DreamMasterLiteProcessor& proc;
    juce::Viewport viewport;
    juce::Component content, builderContent;
    class LoginOverlay;
    std::unique_ptr<LoginOverlay> loginOverlay;
    juce::OwnedArray<EffectCard> effectCards;
    juce::OwnedArray<juce::TextButton> categoryButtons;
    juce::ListBox effectBrowserList;
    juce::TextButton randomButton{"RANDOM FX"}, extremeButton{"XTRMRND"}, definedButton{"RANDOM VALUES"};
    juce::TextButton resetButton{"ALL OFF"}, websiteButton{"DREAMDAW.COM"};
    juce::TextButton rackPageButton{"FX RACK"}, builderPageButton{"FX BUILDER"};
    juce::TextButton loginButton{"LOG IN"}, logoutButton{"LOG OUT"}, themeButton{"THEMES"};
    juce::TextButton addEffectButton{"ADD COMPONENT"};
    juce::TextButton moveUpButton{"UP"}, moveDownButton{"DOWN"}, removeEffectButton{"REMOVE"};
    juce::TextButton setFolderButton{"SET FX FOLDER"};
    juce::TextButton savePresetButton{"SAVE FX"}, loadPresetButton{"LOAD"}, deletePresetButton{"DELETE"};
    juce::TextButton loginSubmitButton{"SIGN IN"}, loginCancelButton{"NOT NOW"};
    juce::Label title, subtitle, countLabel, onlineLabel, accountLabel, builderLockLabel, builderStatusLabel;
    juce::Label loginTitle, loginStatusLabel;
    juce::Label effectBrowserHeading, chainHeading, chosenEffectLabel;
    juce::TextEditor usernameEditor, passwordEditor, presetNameEditor, searchEditor;
    juce::Slider builderAmountSlider, builderToneSlider, builderMotionSlider, builderMixSlider, builderShapeSlider;
    juce::Label builderAmountLabel, builderToneLabel, builderMotionLabel, builderMixLabel, builderShapeLabel;
    juce::ComboBox presetChoiceBox;
    juce::ListBox chainList{"Builder chain", this};
    std::unique_ptr<EffectBrowserModel> effectBrowserModel;
    std::vector<int> visibleEffects;
    std::vector<int> builderBrowserEffects;
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
    bool sessionValidationPending = false;
    bool loginRequestPending = false;
    bool onlineRequestPending = false;
    bool heartbeatPending = false;
    bool showFavoritesOnly = false;
    bool loginOverlayDismissed = false;
    bool reducedMotion = false;
    bool glitchActive = false;
    int selectedCategory = -1;
    int selectedBuilderEffect = 0;
    float pendingBuilderAmount = 0.22f;
    juce::String customFxFolder;
    std::unique_ptr<juce::FileChooser> customFolderChooser;
    juce::uint32 lastCountUpdateMs = 0;
    juce::uint32 lastHeartbeatMs = 0;
    juce::uint32 lastOnlinePollMs = 0;
    juce::uint32 animationFrame = 0;

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void timerCallback() override;
    void randomize(int minimum, int maximum, bool randomAmounts);
    void randomizePickedValues();
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
    void updateBuilderControlsFromSelection();
    void chooseCustomFxFolder();
    void loadCustomEffectFiles();
    juce::File customFxDirectory() const;
    void showLoginPopup();
    void dismissLoginPopup();
    void login();
    void logout();
    void validateSession();
    void refreshCapabilities();
    void refreshOnlineCount();
    void sendPresenceHeartbeat();
    void selectTheme(int selectedIndex);
    void showThemeMenu();
    void populateThemes(const juce::var& themes);
    void applyThemePack(const juce::var& themePack);
    void applyPalette();
    void updateThemeButtonLabel();
    void updateAnimationTimer();
    void requestWorkerAsync(const juce::var* request, bool get,
                            std::function<void(WorkerReply)> callback);
    void setCategory(int category);
    void updateCategoryButtons();
    void updateBuilderChainList();
    void updateEffectBrowserList();
    void selectBuilderBrowserRow(int row);
    void updateLoginPreference(bool dismissed);
    bool readLoginPreference() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamMasterLiteEditor)
};
