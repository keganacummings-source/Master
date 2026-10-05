#include "PluginEditor.h"
#include "Randomizer.h"
#include "DreamShareCredentialStore.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <stdexcept>

namespace {
constexpr auto workerUrl = "https://dreamshare-api.keganacummings.workers.dev/";
constexpr int allCategory = -1;
constexpr int favoritesCategory = -2;
constexpr int categoryRadioGroup = 1407;
constexpr auto defaultThemeId = "default";
constexpr auto defaultBackground = 0xff090909;
constexpr auto defaultPanel = 0xff141414;
constexpr auto defaultAccent = 0xfff12828;
constexpr auto defaultHighlight = 0xffff4444;
constexpr auto defaultText = 0xffe8e0d4;
constexpr auto defaultMuted = 0xff9a8980;
constexpr auto defaultBorder = 0xff3a2824;

juce::ThreadPool& dreamShareWorkerPool() {
    static juce::ThreadPool pool(2);
    return pool;
}

void styleButton(juce::TextButton& button, juce::Colour fill, juce::Colour ink, juce::Colour highlight) {
    button.setColour(juce::TextButton::buttonColourId, fill);
    button.setColour(juce::TextButton::buttonOnColourId, highlight.withAlpha(0.24f));
    button.setColour(juce::TextButton::textColourOffId, ink);
    button.setColour(juce::TextButton::textColourOnId, highlight);
}

bool responseIsOk(const juce::var& response) {
    if (auto* object = response.getDynamicObject()) {
        const auto value = object->getProperty("ok").toString();
        return value == "true" || value == "1";
    }
    return false;
}

bool valueIsTrue(const juce::var& value) {
    const auto text = value.toString();
    return text == "true" || text == "1";
}

juce::Colour colourFromCss(juce::String value, juce::Colour fallback) {
    value = value.trim();
    if (!value.startsWithChar('#'))
        return fallback;
    auto hex = value.substring(1);
    if (hex.length() == 3) {
        hex = juce::String::charToString(hex[0]) + juce::String::charToString(hex[0])
            + juce::String::charToString(hex[1]) + juce::String::charToString(hex[1])
            + juce::String::charToString(hex[2]) + juce::String::charToString(hex[2]);
    }
    if (hex.length() != 6 && hex.length() != 8)
        return fallback;
    return juce::Colour::fromString(hex.length() == 6 ? "ff" + hex : hex);
}

juce::String effectName(int index) {
    return juce::String(dm::featureNames[static_cast<size_t>(index)].data());
}

juce::String effectId(int index) {
    return juce::String(dm::featureIds[static_cast<size_t>(index)].data());
}

juce::String normalisedThemeId(const juce::var& themePack) {
    if (auto* pack = themePack.getDynamicObject())
        return pack->getProperty("id").toString().toLowerCase();
    return {};
}

void updateLoginPreferenceFile(bool write, bool dismissed, bool* result) {
    juce::PropertiesFile::Options options;
    options.applicationName = "INXOMNIA";
    options.folderName = "Dreamdaw";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    juce::PropertiesFile preferences(options);
    if (write) {
        preferences.setValue("loginPopupDismissed", dismissed);
        preferences.saveIfNeeded();
    } else if (result != nullptr) {
        *result = preferences.getBoolValue("loginPopupDismissed", false);
    }
}

class WorkerRequestJob final : public juce::ThreadPoolJob {
public:
    using Reply = DreamMasterLiteEditor::WorkerReply;

    WorkerRequestJob(juce::var requestData, bool useGet, std::function<void(Reply)> completion)
        : ThreadPoolJob("DreamShare HTTPS request"),
          request(std::move(requestData)), get(useGet), callback(std::move(completion)) {}

    JobStatus runJob() override {
        Reply reply;
        juce::URL url(workerUrl);
        const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(3000)
            .withNumRedirectsToFollow(0)
            .withStatusCode(&reply.statusCode);
        std::unique_ptr<juce::InputStream> stream;
        if (get) {
            stream = url.createInputStream(options.withHttpRequestCmd("GET"));
        } else {
            url = url.withPOSTData(juce::JSON::toString(request));
            stream = url.createInputStream(options.withHttpRequestCmd("POST")
                .withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n"));
        }

        if (stream == nullptr) {
            reply.error = "DreamShare Worker is unavailable.";
        } else {
            const auto responseBody = stream->readEntireStreamAsString();
            if (reply.statusCode < 200 || reply.statusCode >= 300) {
                reply.error = "DreamShare Worker returned HTTP " + juce::String(reply.statusCode) + ".";
            } else {
                reply.payload = juce::JSON::parse(responseBody);
                if (!reply.payload.isObject())
                    reply.error = "DreamShare Worker returned an invalid JSON response.";
            }
        }

        auto completion = std::move(callback);
        juce::MessageManager::callAsync([completion = std::move(completion), reply = std::move(reply)]() mutable {
            completion(std::move(reply));
        });
        return jobHasFinished;
    }

private:
    juce::var request;
    bool get = false;
    std::function<void(Reply)> callback;
};
}

class DreamMasterLiteEditor::EffectCard final : public juce::Component {
public:
    EffectCard(DreamMasterLiteProcessor& processor, int index, std::function<void()> favoriteChanged)
        : proc(processor), effectIndex(index), favoriteChangedCallback(std::move(favoriteChanged)) {
        name.setText(effectName(index), juce::dontSendNotification);
        name.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        name.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(name);

        favorite.setClickingTogglesState(true);
        favorite.setTooltip("Add to or remove from favorites");
        favorite.onClick = [this] {
            const auto id = effectId(effectIndex);
            proc.setEffectFavorite(id, favorite.getToggleState());
            favoriteState = favorite.getToggleState();
            favorite.setButtonText(favorite.getToggleState() ? "FAV *" : "FAV +");
            favoriteChangedCallback();
        };
        addAndMakeVisible(favorite);

        enabled.setButtonText("OFF");
        enabled.setClickingTogglesState(true);
        addAndMakeVisible(enabled);
        enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state, "fx" + juce::String(index), enabled);
        enabled.onStateChange = [this] { enabled.setButtonText(enabled.getToggleState() ? "ON" : "OFF"); repaint(); };

        const std::array<const char*, 5> ids{{"amt", "tone", "motion", "mix", "shape"}};
        const std::array<float, 5> defaults{{0.22f, 0.50f, 0.50f, 0.65f, 0.50f}};
        for (int i = 0; i < 5; ++i) {
            sliders[static_cast<size_t>(i)].setSliderStyle(juce::Slider::LinearHorizontal);
            sliders[static_cast<size_t>(i)].setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 18);
            sliders[static_cast<size_t>(i)].setRange(0.0, 1.0, 0.001);
            sliders[static_cast<size_t>(i)].setScrollWheelEnabled(false);
            sliders[static_cast<size_t>(i)].setTooltip(juce::String(ids[i]).toUpperCase());
            addAndMakeVisible(sliders[static_cast<size_t>(i)]);
            attachments[static_cast<size_t>(i)] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                proc.state, juce::String(ids[i]) + juce::String(index), sliders[static_cast<size_t>(i)]);
            sliders[static_cast<size_t>(i)].setValue(defaults[static_cast<size_t>(i)], juce::dontSendNotification);
        }
        setSize(260, 142);
        updateFavoriteState();
        setPalette(juce::Colour(0xff030805), juce::Colour(0xff100a08), juce::Colour(0xff563526),
                   juce::Colour(0xff65ff83), juce::Colour(0xffc8ff33), juce::Colour(0xffd8ffe0));
    }

    int getEffectIndex() const { return effectIndex; }
    void updateFavoriteState() {
        favoriteState = proc.isEffectFavorite(effectId(effectIndex));
        favorite.setToggleState(favoriteState, juce::dontSendNotification);
        favorite.setButtonText(favoriteState ? "FAV *" : "FAV +");
    }
    void setPalette(juce::Colour background, juce::Colour panel, juce::Colour border,
                    juce::Colour accent, juce::Colour highlight, juce::Colour text) {
        backgroundColour = background; panelColour = panel; borderColour = border;
        accentColour = accent; highlightColour = highlight;
        name.setColour(juce::Label::textColourId, text);
        favorite.setColour(juce::TextButton::buttonColourId, panel);
        favorite.setColour(juce::TextButton::buttonOnColourId, highlight.withAlpha(0.22f));
        favorite.setColour(juce::TextButton::textColourOffId, accent);
        favorite.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffce68));
        enabled.setColour(juce::ToggleButton::textColourId, text);
        enabled.setColour(juce::ToggleButton::tickColourId, highlight);
        for (auto& slider : sliders) {
            slider.setColour(juce::Slider::trackColourId, accent);
            slider.setColour(juce::Slider::thumbColourId, highlight);
            slider.setColour(juce::Slider::textBoxTextColourId, text);
            slider.setColour(juce::Slider::textBoxBackgroundColourId, background.darker(0.3f));
            slider.setColour(juce::Slider::textBoxOutlineColourId, border);
        }
        repaint();
    }
    void paint(juce::Graphics& g) override {
        const bool isOn = enabled.getToggleState();
        const auto outline = favoriteState ? juce::Colour(0xffffce68) : (isOn ? accentColour : borderColour);
        g.setColour(panelColour); g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
        g.setColour(outline); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, favoriteState || isOn ? 1.8f : 1.0f);
    }
    void resized() override {
        auto b = getLocalBounds().reduced(8, 5);
        auto header = b.removeFromTop(24);
        favorite.setBounds(header.removeFromRight(55).reduced(1));
        enabled.setBounds(header.removeFromRight(49).reduced(1));
        name.setBounds(header.reduced(1, 0));
        const int rowH = juce::jmax(17, b.getHeight() / 5);
        for (int i = 0; i < 5; ++i)
            sliders[static_cast<size_t>(i)].setBounds(b.removeFromTop(rowH).reduced(1, 0));
    }
private:
    DreamMasterLiteProcessor& proc;
    int effectIndex = -1;
    std::function<void()> favoriteChangedCallback;
    juce::Label name;
    juce::TextButton favorite;
    juce::ToggleButton enabled;
    std::array<juce::Slider, 5> sliders;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> attachments;
    bool favoriteState = false;
    juce::Colour backgroundColour, panelColour, borderColour, accentColour, highlightColour;
};

class DreamMasterLiteEditor::EffectBrowserModel final : public juce::ListBoxModel {
public:
    explicit EffectBrowserModel(DreamMasterLiteEditor& owner) : editor(owner) {}

    int getNumRows() override { return static_cast<int>(editor.builderBrowserEffects.size()); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override {
        if (row < 0 || row >= static_cast<int>(editor.builderBrowserEffects.size()))
            return;
        const auto index = editor.builderBrowserEffects[static_cast<size_t>(row)];
        const bool favorite = editor.proc.isEffectFavorite(effectId(index));
        g.fillAll(selected ? editor.accentColour.withAlpha(0.2f) : editor.panelColour);
        g.setColour(selected ? editor.highlightColour : editor.borderColour.withAlpha(0.5f));
        g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));
        g.setColour(favorite ? juce::Colour(0xffffce68) : editor.textColour);
        g.setFont(juce::Font(juce::FontOptions(11.0f, favorite ? juce::Font::bold : juce::Font::plain)));
        g.drawText((favorite ? "*  " : "   ") + effectName(index),
                   9, 0, width - 18, height, juce::Justification::centredLeft);
    }

    void selectedRowsChanged(int row) override { editor.selectBuilderBrowserRow(row); }

private:
    DreamMasterLiteEditor& editor;
};

DreamMasterLiteEditor::DreamMasterLiteEditor(DreamMasterLiteProcessor& processor)
    : AudioProcessorEditor(processor), proc(processor),
      loginOverlay(std::make_unique<LoginOverlay>()) {
    backgroundColour = juce::Colour(defaultBackground);
    panelColour = juce::Colour(defaultPanel);
    accentColour = juce::Colour(defaultAccent);
    highlightColour = juce::Colour(defaultHighlight);
    textColour = juce::Colour(defaultText);
    mutedColour = juce::Colour(defaultMuted);
    borderColour = juce::Colour(defaultBorder);
    auto* initialTheme = new juce::DynamicObject();
    initialTheme->setProperty("id", defaultThemeId);
    currentThemePack = juce::var(initialTheme);
    title.setText("INXOMNIA", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    title.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title);
    subtitle.setText("INXOMNIA FX RACK  /  200 DISTINCT NAMED MODULES / COMPOSITE FX BUILDER", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(10.5f)));
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    countLabel.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    addAndMakeVisible(countLabel);
    onlineLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    onlineLabel.setText("ONLINE: --", juce::dontSendNotification);
    addAndMakeVisible(onlineLabel);
    accountLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    addAndMakeVisible(accountLabel);

    styleButton(randomButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(extremeButton, juce::Colour(0xff241018), juce::Colour(0xffff7272), juce::Colour(0xffffb047));
    styleButton(definedButton, panelColour, highlightColour, accentColour);
    styleButton(resetButton, juce::Colour(0xff20130e), textColour, highlightColour);
    styleButton(websiteButton, panelColour, highlightColour, accentColour);
    styleButton(rackPageButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(builderPageButton, panelColour, accentColour, highlightColour);
    for (auto* button : {&randomButton, &extremeButton, &definedButton, &resetButton,
                         &websiteButton, &rackPageButton, &builderPageButton,
                         &loginButton, &logoutButton, &themeButton})
        addAndMakeVisible(*button);
    randomButton.onClick = [this] { randomize(1, 10, false); };
    extremeButton.onClick = [this] { randomize(10, 20, true); };
    definedButton.onClick = [this] { randomizePickedValues(); };
    resetButton.onClick = [this] { resetAll(); };
    websiteButton.onClick = [] { juce::URL("https://dreamdaw.com").launchInDefaultBrowser(); };
    rackPageButton.onClick = [this] { showPage(false); };
    builderPageButton.onClick = [this] { showPage(true); };
    loginButton.onClick = [this] { showLoginPopup(); };
    logoutButton.onClick = [this] { logout(); };
    themeButton.setButtonText("THEME: DEFAULT");
    themeButton.onClick = [this] {
        if (authenticated)
            showThemeMenu();
        else
            showLoginPopup();
    };

    for (int i = 0; i < 10; ++i) {
        auto* button = categoryButtons.add(new juce::TextButton());
        button->setClickingTogglesState(true);
        button->setRadioGroupId(categoryRadioGroup);
        button->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        if (i == 0) {
            button->setButtonText("ALL EFFECTS");
            button->setTooltip("Show all effect categories");
            button->onClick = [this] { setCategory(allCategory); };
        } else if (i == 1) {
            button->setButtonText("FAVORITES *");
            button->setTooltip("Show favorites from every category");
            button->onClick = [this] { setCategory(favoritesCategory); };
        } else {
            const int category = i - 2;
            button->setButtonText(juce::String(dm::effectCategories[static_cast<size_t>(category)].data()));
            button->setTooltip(button->getButtonText());
            button->onClick = [this, category] { setCategory(category); };
        }
        addAndMakeVisible(*button);
    }

    searchEditor.setTextToShowWhenEmpty("Search effects...", juce::Colours::grey);
    searchEditor.setColour(juce::TextEditor::backgroundColourId, panelColour);
    searchEditor.setColour(juce::TextEditor::textColourId, textColour);
    searchEditor.setColour(juce::TextEditor::outlineColourId, borderColour);
    searchEditor.onTextChange = [this] { updateVisibleEffects(); };
    addAndMakeVisible(searchEditor);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, true);
    viewport.setColour(juce::ScrollBar::thumbColourId, accentColour.withAlpha(0.8f));
    viewport.setColour(juce::ScrollBar::trackColourId, backgroundColour);
    addAndMakeVisible(viewport);

    styleButton(addEffectButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(moveUpButton, panelColour, accentColour, highlightColour);
    styleButton(moveDownButton, panelColour, accentColour, highlightColour);
    styleButton(removeEffectButton, juce::Colour(0xff291410), juce::Colour(0xffff9b83), juce::Colour(0xffffc0a8));
    styleButton(savePresetButton, panelColour, accentColour, highlightColour);
    styleButton(loadPresetButton, panelColour, accentColour, highlightColour);
    styleButton(deletePresetButton, panelColour, mutedColour, highlightColour);
    for (auto* button : {&addEffectButton, &moveUpButton, &moveDownButton,
                         &removeEffectButton, &setFolderButton, &savePresetButton, &loadPresetButton, &deletePresetButton})
        builderContent.addAndMakeVisible(*button);
    moveUpButton.setVisible(false);
    moveDownButton.setVisible(false);
    setFolderButton.onClick = [this] { chooseCustomFxFolder(); };
    addEffectButton.onClick = [this] { addBuilderEffect(); };

    builderLockLabel.setText("FX BUILDER LOCKED - LOG IN TO DREAMSHARE", juce::dontSendNotification);
    builderLockLabel.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    builderLockLabel.setJustificationType(juce::Justification::centredLeft);
    builderContent.addAndMakeVisible(builderLockLabel);
    effectBrowserHeading.setText("1. EFFECT LIBRARY", juce::dontSendNotification);
    effectBrowserHeading.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    effectBrowserHeading.setJustificationType(juce::Justification::centredLeft);
    builderContent.addAndMakeVisible(effectBrowserHeading);
    chainHeading.setText("2. COMPOSITE PARTS", juce::dontSendNotification);
    chainHeading.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    chainHeading.setJustificationType(juce::Justification::centredLeft);
    builderContent.addAndMakeVisible(chainHeading);
    chosenEffectLabel.setText("Select a module, tune all five controls, then ADD COMPONENT.", juce::dontSendNotification);
    chosenEffectLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    chosenEffectLabel.setJustificationType(juce::Justification::centredLeft);
    builderContent.addAndMakeVisible(chosenEffectLabel);
    effectBrowserModel = std::make_unique<EffectBrowserModel>(*this);
    effectBrowserList.setModel(effectBrowserModel.get());
    effectBrowserList.setRowHeight(25);
    effectBrowserList.setMultipleSelectionEnabled(false);
    effectBrowserList.setColour(juce::ListBox::backgroundColourId, panelColour);
    effectBrowserList.setColour(juce::ListBox::outlineColourId, borderColour);
    builderContent.addAndMakeVisible(effectBrowserList);
    builderAmountSlider.setRange(0.0, 1.0, 0.001);
    builderAmountSlider.setValue(0.22, juce::dontSendNotification);
    builderAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    builderAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 22);
    builderAmountSlider.setScrollWheelEnabled(false);
    builderAmountSlider.onValueChange = [this] {
        pendingBuilderAmount = static_cast<float>(builderAmountSlider.getValue());
        updateBuilderAmount();
    };
    builderContent.addAndMakeVisible(builderAmountSlider);
    const std::array<juce::Slider*, 4> extraBuilderSliders{{&builderToneSlider, &builderMotionSlider, &builderMixSlider, &builderShapeSlider}};
    const std::array<float, 4> extraDefaults{{0.50f, 0.50f, 0.65f, 0.50f}};
    for (int i = 0; i < 4; ++i) {
        auto* slider = extraBuilderSliders[static_cast<size_t>(i)];
        slider->setRange(0.0, 1.0, 0.001);
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 22);
        slider->setScrollWheelEnabled(false);
        slider->setValue(extraDefaults[static_cast<size_t>(i)], juce::dontSendNotification);
        builderContent.addAndMakeVisible(*slider);
        slider->onValueChange = [this] { updateBuilderAmount(); };
    }
    const std::array<juce::Label*, 5> builderLabels{{&builderAmountLabel, &builderToneLabel, &builderMotionLabel, &builderMixLabel, &builderShapeLabel}};
    const std::array<const char*, 5> labelText{{"INTENSITY", "TONE", "MOTION", "MIX", "SHAPE"}};
    for (int i = 0; i < 5; ++i) { builderLabels[static_cast<size_t>(i)]->setText(labelText[i], juce::dontSendNotification); builderLabels[static_cast<size_t>(i)]->setColour(juce::Label::textColourId, mutedColour); builderContent.addAndMakeVisible(*builderLabels[static_cast<size_t>(i)]); }

    chainList.setRowHeight(27);
    chainList.setMultipleSelectionEnabled(false);
    chainList.setColour(juce::ListBox::backgroundColourId, panelColour);
    chainList.setColour(juce::ListBox::outlineColourId, borderColour);
    builderContent.addAndMakeVisible(chainList);
    moveUpButton.onClick = [this] {};
    moveDownButton.onClick = [this] {};
    removeEffectButton.onClick = [this] {
        const int selected = chainList.getSelectedRow();
        if (selected >= 0 && selected < static_cast<int>(builderSteps.size())) {
            builderSteps.erase(builderSteps.begin() + selected);
            updateBuilderChainList();
            if (!builderSteps.empty())
                chainList.selectRow(juce::jmin(selected, static_cast<int>(builderSteps.size()) - 1));
        }
    };
    presetNameEditor.setTextToShowWhenEmpty("Name this custom FX", juce::Colours::grey);
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, panelColour);
    presetNameEditor.setColour(juce::TextEditor::textColourId, textColour);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, borderColour);
    builderContent.addAndMakeVisible(presetNameEditor);
    builderContent.addAndMakeVisible(presetChoiceBox);
    savePresetButton.onClick = [this] { saveBuilderPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    builderStatusLabel.setFont(juce::Font(juce::FontOptions(10.5f)));
    builderStatusLabel.setText("Build one named composite effect from multiple modules. No chain ordering is used.",
                               juce::dontSendNotification);
    builderContent.addAndMakeVisible(builderStatusLabel);
    builderContent.setVisible(false);

    loginTitle.setText("DREAMSHARE LOGIN", juce::dontSendNotification);
    loginTitle.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    loginTitle.setJustificationType(juce::Justification::centred);
    loginOverlay->addAndMakeVisible(loginTitle);
    loginStatusLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    loginStatusLabel.setJustificationType(juce::Justification::centred);
    loginStatusLabel.setColour(juce::Label::textColourId, mutedColour);
    loginOverlay->addAndMakeVisible(loginStatusLabel);
    usernameEditor.setTextToShowWhenEmpty("Username", juce::Colours::grey);
    passwordEditor.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passwordEditor.setPasswordCharacter(0x2022);
    for (auto* editor : {&usernameEditor, &passwordEditor}) {
        editor->setColour(juce::TextEditor::backgroundColourId, panelColour);
        editor->setColour(juce::TextEditor::textColourId, textColour);
        editor->setColour(juce::TextEditor::outlineColourId, borderColour);
        loginOverlay->addAndMakeVisible(*editor);
    }
    loginOverlay->addAndMakeVisible(loginSubmitButton);
    loginOverlay->addAndMakeVisible(loginCancelButton);
    styleButton(loginSubmitButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(loginCancelButton, panelColour, mutedColour, highlightColour);
    loginSubmitButton.onClick = [this] { login(); };
    loginCancelButton.onClick = [this] { dismissLoginPopup(); };
    passwordEditor.onReturnKey = [this] { login(); };
    addAndMakeVisible(*loginOverlay);
    loginOverlay->setVisible(false);

    {
        juce::PropertiesFile::Options options; options.applicationName = "INXOMNIA"; options.folderName = "Dreamdaw";
        options.filenameSuffix = "settings"; options.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile prefs(options); customFxFolder = prefs.getValue("customFxFolder", "");
    }
    updateCategoryButtons();
    loginOverlay->setPalette(panelColour, borderColour, accentColour);
    updateBuilderChainList();
    updateVisibleEffects();
    updateAuthenticationUi();
    updateActiveCount();
    setResizable(true, true);
    setResizeLimits(760, 580, 1500, 1200);
    setSize(1040, 800);
    applyPalette();

    if (proc.hasDreamShareSession()) {
        authenticated = true;
        updateAuthenticationUi();
        validateSession();
    } else if (!readLoginPreference()) {
        showLoginPopup();
    }
    refreshOnlineCount();
    lastOnlinePollMs = juce::Time::getMillisecondCounter();
    startTimer(1000);
}

DreamMasterLiteEditor::~DreamMasterLiteEditor() {
    stopTimer();
    effectBrowserList.setModel(nullptr);
    viewport.setViewedComponent(nullptr, false);
}

void DreamMasterLiteEditor::paint(juce::Graphics& g) {
    g.fillAll(backgroundColour);
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient glow(highlightColour.withAlpha(0.055f), bounds.getWidth() * 0.18f, 0.0f,
                              juce::Colours::transparentBlack, bounds.getWidth() * 0.62f, bounds.getHeight(), false);
    g.setGradientFill(glow);
    g.fillRect(bounds);

    const auto theme = normalisedThemeId(currentThemePack);
    const bool warmTheme = theme == "amber" || theme == "bloodmoon" || theme == "cherry"
        || theme == "cinder" || theme == "copper" || theme == "ember" || theme == "honey"
        || theme == "rust" || theme == "sulfur" || theme == "wine";
    const bool coolTheme = theme == "abyss" || theme == "ice" || theme == "lagoon"
        || theme == "neon" || theme == "void" || theme == "violet";
    const bool organicTheme = theme == "mint" || theme == "moss" || theme == "olive"
        || theme == "pine" || theme == "goonr";
    const int animationHeight = juce::jmax(1, getHeight());
    const int animationWidth = juce::jmax(1, getWidth());
    const float motion = reducedMotion ? 0.0f : static_cast<float>(animationFrame) * 0.012f;

    if (theme == "goonr") {
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        for (int x = 18, column = 0; x < getWidth(); x += 64, ++column) {
            const int phase = (column * 47 + static_cast<int>(animationFrame)) % (animationHeight + 72);
            const int y = phase - 36;
            g.setColour(accentColour.withAlpha(0.13f));
            g.drawText("0", x, y, 11, 11, juce::Justification::centred);
            g.setColour(highlightColour.withAlpha(0.085f));
            g.drawText("1", x, y - 21, 11, 11, juce::Justification::centred);
            g.setColour(accentColour.withAlpha(0.055f));
            g.drawText("0", x, y - 42, 11, 11, juce::Justification::centred);
        }
    } else if (theme == "trippah") {
        for (int i = 0; i < 10; ++i) {
            const float x = static_cast<float>((i * 109 + 37) % animationWidth)
                + std::sin(motion + i * 1.4f) * 9.0f;
            const float y = static_cast<float>((i * 67 + static_cast<int>(animationFrame * (1u + static_cast<uint32_t>(i % 2))) * 2)
                                               % (animationHeight + 100)) - 50.0f;
            const float size = 5.0f + static_cast<float>(i % 3);
            g.setColour((i % 2 == 0 ? accentColour : highlightColour).withAlpha(0.17f));
            if (i % 2 == 0) {
                // Small mushroom caps and stems are the only non-pill floating motif.
                g.fillRoundedRectangle(x + size * 0.36f, y + size * 0.66f, size * 0.3f, size * 0.9f, 1.4f);
                g.fillEllipse(x, y, size, size * 0.68f);
                g.setColour(textColour.withAlpha(0.24f));
                g.fillEllipse(x + size * 0.25f, y + size * 0.2f, size * 0.12f, size * 0.12f);
            } else {
                juce::Rectangle<float> pill(x, y, size * 1.65f, size * 0.68f);
                g.fillRoundedRectangle(pill, size * 0.34f);
                g.setColour(backgroundColour.withAlpha(0.42f));
                g.drawLine(pill.getCentreX(), pill.getY() + 1.0f, pill.getCentreX(),
                           pill.getBottom() - 1.0f, 0.8f);
            }
        }
    } else {
        // Keep the ambient layer deliberately small: ten theme-coloured primitives,
        // animated with position math and no per-frame allocation or child components.
        for (int i = 0; i < 10; ++i) {
            const float baseX = static_cast<float>((i * 97 + 29) % animationWidth);
            const float x = baseX + std::sin(motion + i * 1.31f) * (warmTheme ? 7.0f : 11.0f);
            const float travel = reducedMotion ? 0.0f : std::fmod(motion * (18.0f + (i % 4) * 5.0f), 72.0f);
            const float y = static_cast<float>((i * 59 + 17) % animationHeight) + travel - 36.0f;
            const float size = 4.0f + static_cast<float>(i % 4);
            const auto colour = (i % 3 == 0 ? highlightColour : accentColour)
                .withAlpha(organicTheme ? 0.095f : 0.075f);
            g.setColour(colour);

            if (coolTheme) {
                g.drawEllipse(x, y, size * 1.55f, size * 1.55f, 1.0f);
                g.fillEllipse(x + size * 0.58f, y + size * 0.58f, size * 0.4f, size * 0.4f);
            } else if (organicTheme) {
                juce::Path leaf;
                leaf.startNewSubPath(x, y + size);
                leaf.quadraticTo(x + size * 0.9f, y - size * 0.2f, x + size * 1.6f, y + size * 0.45f);
                leaf.quadraticTo(x + size * 0.9f, y + size * 1.5f, x, y + size);
                g.strokePath(leaf, juce::PathStrokeType(1.0f));
                g.drawLine(x + 1.0f, y + size * 0.9f, x + size * 1.4f, y + size * 0.42f, 0.7f);
            } else if (warmTheme) {
                juce::Path ember;
                ember.startNewSubPath(x + size * 0.5f, y);
                ember.lineTo(x + size, y + size * 0.5f);
                ember.lineTo(x + size * 0.5f, y + size);
                ember.lineTo(x, y + size * 0.5f);
                ember.closeSubPath();
                g.strokePath(ember, juce::PathStrokeType(1.0f));
            } else {
                g.drawEllipse(x, y, size, size, 1.0f);
                g.drawLine(x + size * 0.5f, y - 1.5f, x + size * 0.5f, y + size + 1.5f, 0.8f);
                g.drawLine(x - 1.5f, y + size * 0.5f, x + size + 1.5f, y + size * 0.5f, 0.8f);
            }
        }
    }

    if (glitchActive) {
        g.saveState();
        const float slice = 2.0f + static_cast<float>(animationFrame % 5u);
        g.setColour(highlightColour.withAlpha(0.10f));
        g.fillRect(0.0f, slice * 18.0f, static_cast<float>(getWidth()), slice);
        g.setColour(accentColour.withAlpha(0.08f));
        g.fillRect(8.0f, slice * 19.0f, static_cast<float>(getWidth() - 16), 1.0f);
        g.restoreState();
    }

    g.setColour(juce::Colours::black.withAlpha(0.10f));
    for (int y = 0; y < getHeight(); y += 6)
        g.fillRect(0, y, getWidth(), 1);
    const float cy = 27.0f, eyeW = 34.0f, eyeH = 22.0f, mid = getWidth() * 0.5f;
    for (int side : {-1, 1}) {
        juce::Rectangle<float> eye(mid + side * 150.0f - eyeW * 0.5f, cy - eyeH * 0.5f, eyeW, eyeH);
        g.setColour(accentColour.withAlpha(0.85f));
        g.drawEllipse(eye, 2.0f);
        g.setColour(panelColour);
        g.fillEllipse(eye.reduced(2.0f));
        g.setColour(highlightColour);
        g.fillEllipse(eye.getCentreX() - 5.0f, eye.getCentreY() - 5.0f, 10.0f, 10.0f);
        g.setColour(backgroundColour);
        g.fillEllipse(eye.getCentreX() - 2.0f, eye.getCentreY() - 2.0f, 4.0f, 4.0f);
    }
    g.setColour(borderColour);
    g.drawRect(getLocalBounds().reduced(2), 2);
    g.setColour(highlightColour.withAlpha(0.38f));
    g.drawRect(getLocalBounds().reduced(5), 1);
}

void DreamMasterLiteEditor::resized() {
    auto bounds = getLocalBounds().reduced(11);
    title.setBounds(bounds.removeFromTop(32));
    subtitle.setBounds(bounds.removeFromTop(15));

    auto accountRow = bounds.removeFromTop(28);
    loginButton.setBounds(accountRow.removeFromRight(74).reduced(2));
    logoutButton.setBounds(accountRow.removeFromRight(74).reduced(2));
    onlineLabel.setBounds(accountRow.removeFromRight(115).reduced(3, 2));
    accountLabel.setBounds(accountRow.reduced(3, 2));

    auto actionRow = bounds.removeFromTop(34);
    randomButton.setBounds(actionRow.removeFromLeft(123).reduced(2));
    extremeButton.setBounds(actionRow.removeFromLeft(92).reduced(2));
    definedButton.setBounds(actionRow.removeFromLeft(133).reduced(2));
    resetButton.setBounds(actionRow.removeFromLeft(76).reduced(2));
    countLabel.setBounds(actionRow.reduced(6, 2));

    auto navigationRow = bounds.removeFromTop(31);
    rackPageButton.setBounds(navigationRow.removeFromLeft(96).reduced(2));
    builderPageButton.setBounds(navigationRow.removeFromLeft(115).reduced(2));
    websiteButton.setBounds(navigationRow.removeFromLeft(143).reduced(2));
    themeButton.setBounds(navigationRow.removeFromRight(145).reduced(2));

    const int categoryGap = 4;
    const int categoryWidth = (bounds.getWidth() - categoryGap * 4) / 5;
    for (int i = 0; i < categoryButtons.size(); ++i) {
        const int row = i / 5, column = i % 5;
        categoryButtons[i]->setBounds(bounds.getX() + column * (categoryWidth + categoryGap),
                                      bounds.getY() + row * 25, categoryWidth, 23);
    }
    bounds.removeFromTop(51);
    searchEditor.setBounds(bounds.removeFromTop(26).reduced(2));
    bounds.removeFromTop(3);
    viewport.setBounds(bounds);

    const int columns = getWidth() >= 1050 ? 4 : 3;
    const int gap = 8, cardHeight = 148;
    const int cardWidth = juce::jmax(210, (viewport.getWidth() - 27 - gap * (columns - 1)) / columns);
    const int rows = (static_cast<int>(visibleEffects.size()) + columns - 1) / columns;
    content.setSize(juce::jmax(viewport.getWidth() - 16, columns * cardWidth + (columns - 1) * gap),
                    juce::jmax(viewport.getHeight(), rows * (cardHeight + gap)));
    for (int position = 0; position < effectCards.size(); ++position) {
        const int col = position % columns, row = position / columns;
        effectCards[position]->setBounds(col * (cardWidth + gap), row * (cardHeight + gap), cardWidth, cardHeight);
    }

    const int builderWidth = juce::jmax(500, viewport.getWidth() - 16);
    const int builderHeight = juce::jmax(460, viewport.getHeight() - 16);
    builderContent.setSize(builderWidth, builderHeight);
    auto builderBounds = builderContent.getLocalBounds().reduced(12);
    builderLockLabel.setBounds(builderBounds.removeFromTop(24));
    builderBounds.removeFromTop(4);
    const int paneGap = 12;
    auto leftPane = builderBounds.removeFromLeft((builderBounds.getWidth() - paneGap) / 2);
    builderBounds.removeFromLeft(paneGap);
    auto rightPane = builderBounds;
    effectBrowserHeading.setBounds(leftPane.removeFromTop(23));
    chainHeading.setBounds(rightPane.removeFromTop(23));
    leftPane.removeFromTop(3);
    rightPane.removeFromTop(3);
    const int pickerControlsHeight = 170;
    effectBrowserList.setBounds(leftPane.removeFromTop(juce::jmax(100, leftPane.getHeight() - pickerControlsHeight)).reduced(1));
    leftPane.removeFromTop(3);
    chosenEffectLabel.setBounds(leftPane.removeFromTop(19));
    auto pickerRow = leftPane.removeFromTop(29);
    builderAmountLabel.setBounds(pickerRow.removeFromLeft(58));
    builderAmountSlider.setBounds(pickerRow.reduced(1));
    for (auto* label : {&builderToneLabel, &builderMotionLabel, &builderMixLabel, &builderShapeLabel}) { (void) label; }
    auto controlRows = leftPane.removeFromTop(116);
    const std::array<std::pair<juce::Label*, juce::Slider*>, 4> rows{{
        {&builderToneLabel, &builderToneSlider}, {&builderMotionLabel, &builderMotionSlider},
        {&builderMixLabel, &builderMixSlider}, {&builderShapeLabel, &builderShapeSlider}}};
    for (const auto& row : rows) { auto line = controlRows.removeFromTop(28); row.first->setBounds(line.removeFromLeft(58)); row.second->setBounds(line.reduced(1)); }
    addEffectButton.setBounds(leftPane.removeFromTop(28).reduced(2));

    const int bottomControlsHeight = 158;
    chainList.setBounds(rightPane.removeFromTop(juce::jmax(90, rightPane.getHeight() - bottomControlsHeight)).reduced(1));
    rightPane.removeFromTop(3);
    auto orderRow = rightPane.removeFromTop(28);
    removeEffectButton.setBounds(orderRow.removeFromLeft(96).reduced(1));
    setFolderButton.setBounds(orderRow.removeFromLeft(126).reduced(1));
    auto saveRow = rightPane.removeFromTop(29);
    presetNameEditor.setBounds(saveRow.removeFromLeft(juce::jmax(110, saveRow.getWidth() - 96)).reduced(1));
    savePresetButton.setBounds(saveRow.reduced(1));
    auto recallRow = rightPane.removeFromTop(29);
    presetChoiceBox.setBounds(recallRow.removeFromLeft(juce::jmax(110, recallRow.getWidth() - 164)).reduced(1));
    loadPresetButton.setBounds(recallRow.removeFromLeft(72).reduced(1));
    deletePresetButton.setBounds(recallRow.reduced(1));
    builderStatusLabel.setBounds(rightPane.removeFromTop(38).reduced(2, 1));

    loginOverlay->setBounds(getLocalBounds());
    const auto popup = getLocalBounds().withSizeKeepingCentre(430, 260).reduced(28);
    auto loginBounds = juce::Rectangle<int>(popup.getX(), popup.getY(), popup.getWidth(), popup.getHeight());
    loginTitle.setBounds(loginBounds.removeFromTop(34));
    loginBounds.removeFromTop(4);
    usernameEditor.setBounds(loginBounds.removeFromTop(32).reduced(1));
    loginBounds.removeFromTop(5);
    passwordEditor.setBounds(loginBounds.removeFromTop(32).reduced(1));
    loginBounds.removeFromTop(3);
    loginStatusLabel.setBounds(loginBounds.removeFromTop(20).reduced(1));
    loginBounds.removeFromTop(5);
    loginSubmitButton.setBounds(loginBounds.removeFromLeft(loginBounds.getWidth() / 2 - 3)
                                    .withHeight(30).reduced(1));
    loginCancelButton.setBounds(loginBounds.withHeight(30).reduced(1));
}

void DreamMasterLiteEditor::visibilityChanged() {
    updateAnimationTimer();
}

void DreamMasterLiteEditor::randomizePickedValues() {
    juce::Random& rng = juce::Random::getSystemRandom();
    int changed = 0;
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        auto* enabled = proc.state.getRawParameterValue("fx" + juce::String(i));
        if (enabled == nullptr || enabled->load(std::memory_order_relaxed) < 0.5f)
            continue;
        const std::array<const char*, 5> ids{{"amt", "tone", "motion", "mix", "shape"}};
        for (const auto* id : ids)
            if (auto* parameter = proc.state.getParameter(juce::String(id) + juce::String(i)))
                parameter->setValueNotifyingHost(0.08f + rng.nextFloat() * 0.84f);
        ++changed;
    }
    countLabel.setText(juce::String(changed) + " PICKED FX | RANDOM VALUES", juce::dontSendNotification);
    updateVisibleEffects();
}

void DreamMasterLiteEditor::randomize(int minimum, int maximum, bool randomAmounts) {
    const auto seed = static_cast<uint32_t>(juce::Random::getSystemRandom().nextInt());
    const auto selected = dm::randomEffectSelection(seed, minimum, maximum);
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (auto* parameter = proc.state.getParameter("fx" + juce::String(i)))
            parameter->setValueNotifyingHost(0.0f);
    }
    juce::Random& rng = juce::Random::getSystemRandom();
    for (const int index : selected) {
        if (auto* parameter = proc.state.getParameter("fx" + juce::String(index)))
            parameter->setValueNotifyingHost(1.0f);
        const float amount = randomAmounts
            ? 0.1f + rng.nextFloat() * 0.8f
            : dm::definedRandomAmount(index);
        if (auto* parameter = proc.state.getParameter("amt" + juce::String(index)))
            parameter->setValueNotifyingHost(amount);
        const std::array<std::pair<const char*, float>, 4> details{{{"tone", 0.50f}, {"motion", 0.50f}, {"mix", 0.65f}, {"shape", 0.50f}}};
        for (const auto& detail : details)
            if (auto* parameter = proc.state.getParameter(juce::String(detail.first) + juce::String(index)))
                parameter->setValueNotifyingHost(randomAmounts ? 0.1f + rng.nextFloat() * 0.8f : detail.second);
    }
    proc.setActiveEffectOrder({});
    updateActiveCount();
    countLabel.setText(juce::String(selected.size()) + " ACTIVE | "
        + (randomAmounts ? "RANDOM AMOUNTS" : "STABLE AMOUNTS"),
                       juce::dontSendNotification);
}

void DreamMasterLiteEditor::resetAll() {
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (auto* parameter = proc.state.getParameter("fx" + juce::String(i)))
            parameter->setValueNotifyingHost(0.0f);
    }
    proc.setActiveEffectOrder({});
    updateActiveCount();
}

void DreamMasterLiteEditor::setCategory(int category) {
    showFavoritesOnly = category == favoritesCategory;
    selectedCategory = category >= 0 ? category : allCategory;
    updateCategoryButtons();
    updateVisibleEffects();
}

void DreamMasterLiteEditor::updateCategoryButtons() {
    const int selected = showFavoritesOnly ? favoritesCategory : selectedCategory;
    for (int i = 0; i < categoryButtons.size(); ++i) {
        const int category = i == 0 ? allCategory : (i == 1 ? favoritesCategory : i - 2);
        categoryButtons[i]->setToggleState(category == selected, juce::dontSendNotification);
        styleButton(*categoryButtons[i], category == selected
                        ? panelColour.interpolatedWith(accentColour, 0.16f) : panelColour,
                    category == selected ? accentColour : mutedColour,
                    category == selected ? highlightColour : accentColour);
    }
}

void DreamMasterLiteEditor::updateVisibleEffects() {
    visibleEffects.clear();
    const auto favorites = proc.getFavoriteEffectIds();
    const auto query = searchEditor.getText().trim().toLowerCase();
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (selectedCategory != allCategory && dm::effectCategoryIndex(i) != selectedCategory)
            continue;
        const auto id = effectId(i);
        const bool favorite = std::find(favorites.begin(), favorites.end(), id) != favorites.end();
        if (showFavoritesOnly && !favorite)
            continue;
        if (query.isNotEmpty() && !effectName(i).toLowerCase().contains(query)
            && !id.toLowerCase().contains(query))
            continue;
        visibleEffects.push_back(i);
    }
    visibleEffects = dm::sortEffectsWithFavoritesFirst(std::move(visibleEffects), favorites);
    updateEffectBrowserList();

    effectCards.clear();
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    for (const int index : visibleEffects) {
        auto* card = effectCards.add(new EffectCard(proc, index, [safeThis] {
            juce::MessageManager::callAsync([safeThis] {
                if (safeThis != nullptr)
                    safeThis->updateVisibleEffects();
            });
        }));
        card->setPalette(backgroundColour, panelColour, borderColour, accentColour, highlightColour, textColour);
        content.addAndMakeVisible(card);
    }
    resized();
    content.repaint();
}

void DreamMasterLiteEditor::updateEffectBrowserList() {
    builderBrowserEffects.clear();
    const auto favorites = proc.getFavoriteEffectIds();
    const auto query = searchEditor.getText().trim().toLowerCase();
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (selectedCategory >= 0 && dm::effectCategoryIndex(i) != selectedCategory)
            continue;
        const auto id = effectId(i);
        const bool favorite = std::find(favorites.begin(), favorites.end(), id) != favorites.end();
        if (showFavoritesOnly && !favorite)
            continue;
        if (query.isNotEmpty() && !effectName(i).toLowerCase().contains(query)
            && !id.toLowerCase().contains(query))
            continue;
        builderBrowserEffects.push_back(i);
    }
    builderBrowserEffects = dm::sortEffectsWithFavoritesFirst(std::move(builderBrowserEffects), favorites);
    effectBrowserHeading.setText("1. EFFECT LIBRARY (" + juce::String(builderBrowserEffects.size()) + ")",
                                 juce::dontSendNotification);
    effectBrowserList.updateContent();
    effectBrowserList.repaint();
}

void DreamMasterLiteEditor::selectBuilderBrowserRow(int row) {
    if (row < 0 || row >= static_cast<int>(builderBrowserEffects.size())) return;
    selectedBuilderEffect = builderBrowserEffects[static_cast<size_t>(row)];
    chosenEffectLabel.setText("New component: " + effectName(selectedBuilderEffect), juce::dontSendNotification);
    builderStatusLabel.setText("Tune all five controls, then ADD COMPONENT.", juce::dontSendNotification);
    chainList.deselectAllRows();
    const std::array<std::pair<juce::Slider*, const char*>, 5> controls{{
        {&builderAmountSlider, "amt"}, {&builderToneSlider, "tone"}, {&builderMotionSlider, "motion"},
        {&builderMixSlider, "mix"}, {&builderShapeSlider, "shape"}}};
    const std::array<float, 5> defaults{{0.22f, 0.50f, 0.50f, 0.65f, 0.50f}};
    for (int i = 0; i < 5; ++i) {
        float value = defaults[static_cast<size_t>(i)];
        if (auto* parameter = proc.state.getRawParameterValue(juce::String(controls[static_cast<size_t>(i)].second) + juce::String(selectedBuilderEffect)))
            value = parameter->load(std::memory_order_relaxed);
        controls[static_cast<size_t>(i)].first->setValue(value, juce::dontSendNotification);
    }
}

void DreamMasterLiteEditor::showPage(bool builder) {
    if (builder && !authenticated) {
        showLoginPopup();
        return;
    }
    builderPageActive = builder;
    viewport.setViewedComponent(builder ? &builderContent : &content, false);
    content.setVisible(!builder);
    builderContent.setVisible(builder);
    rackPageButton.setToggleState(!builder, juce::dontSendNotification);
    builderPageButton.setToggleState(builder, juce::dontSendNotification);
    resized();
}

int DreamMasterLiteEditor::getNumRows() {
    return static_cast<int>(builderSteps.size());
}

void DreamMasterLiteEditor::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (row < 0 || row >= static_cast<int>(builderSteps.size()))
        return;
    g.fillAll(rowIsSelected ? accentColour.withAlpha(0.16f) : panelColour);
    g.setColour(rowIsSelected ? accentColour : borderColour.withAlpha(0.55f));
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));
    const auto& step = builderSteps[static_cast<size_t>(row)];
    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(juce::String(row + 1) + ". " + effectName(step.effectIndex),
               9, 0, width - 98, height, juce::Justification::centredLeft);
    g.setColour(highlightColour);
    g.drawText(juce::String(step.controls[0], 2) + " / " + juce::String(step.controls[3], 2), width - 77, 0, 64, height, juce::Justification::centredRight);
}

void DreamMasterLiteEditor::selectedRowsChanged(int selectedRow) {
    if (selectedRow >= 0 && selectedRow < static_cast<int>(builderSteps.size())) {
        const auto& part = builderSteps[static_cast<size_t>(selectedRow)];
        builderAmountSlider.setValue(part.controls[0], juce::dontSendNotification);
        builderToneSlider.setValue(part.controls[1], juce::dontSendNotification);
        builderMotionSlider.setValue(part.controls[2], juce::dontSendNotification);
        builderMixSlider.setValue(part.controls[3], juce::dontSendNotification);
        builderShapeSlider.setValue(part.controls[4], juce::dontSendNotification);
        chosenEffectLabel.setText("Editing component " + juce::String(selectedRow + 1) + ": " + effectName(part.effectIndex), juce::dontSendNotification);
        builderStatusLabel.setText("Tune Intensity / Tone / Motion / Mix / Shape. Components are blended into ONE effect.", juce::dontSendNotification);
    }
}

void DreamMasterLiteEditor::addBuilderEffect() {
    const int index = selectedBuilderEffect;
    if (index < 0 || index >= dm::DspEngine::effectCount)
        return;
    if (std::any_of(builderSteps.begin(), builderSteps.end(), [index](const auto& step) { return step.effectIndex == index; })) {
        builderStatusLabel.setText("That module is already a component of this custom effect.", juce::dontSendNotification);
        return;
    }
    BuilderStep part;
    part.effectIndex = index;
    part.controls = { static_cast<float>(builderAmountSlider.getValue()), static_cast<float>(builderToneSlider.getValue()),
                      static_cast<float>(builderMotionSlider.getValue()), static_cast<float>(builderMixSlider.getValue()),
                      static_cast<float>(builderShapeSlider.getValue()) };
    builderSteps.push_back(part);
    updateBuilderChainList();
    chainList.selectRow(static_cast<int>(builderSteps.size()) - 1);
    builderStatusLabel.setText("Added as a component. No chain ordering is stored.", juce::dontSendNotification);
}

juce::File DreamMasterLiteEditor::customFxDirectory() const {
    if (customFxFolder.trim().isEmpty()) return {};
    const juce::File folder(customFxFolder);
    return folder.isDirectory() ? folder : juce::File();
}

void DreamMasterLiteEditor::chooseCustomFxFolder() {
    customFolderChooser = std::make_unique<juce::FileChooser>("Choose INXOMNIA Custom FX Folder", juce::File(customFxFolder), "*");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    customFolderChooser->launchAsync(flags, [safeThis](const juce::FileChooser& chooser) {
        if (safeThis == nullptr) return;
        const auto folder = chooser.getResult();
        if (!folder.isDirectory()) return;
        safeThis->customFxFolder = folder.getFullPathName();
        juce::PropertiesFile::Options options;
        options.applicationName = "INXOMNIA"; options.folderName = "Dreamdaw";
        options.filenameSuffix = "settings"; options.osxLibrarySubFolder = "Application Support";
        juce::PropertiesFile prefs(options);
        prefs.setValue("customFxFolder", safeThis->customFxFolder); prefs.saveIfNeeded();
        safeThis->loadCustomEffectFiles();
        safeThis->builderStatusLabel.setText("CUSTOM FX FOLDER: " + safeThis->customFxFolder, juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::loadCustomEffectFiles() {
    availablePresets.clear();
    presetChoiceBox.clear(juce::dontSendNotification);
    const auto folder = customFxDirectory();
    if (!folder.isDirectory()) return;
    try {
        juce::Array<juce::File> files;
        folder.findChildFiles(files, juce::File::findFiles, false, "*.dmefx");
        for (const auto& file : files) {
            const auto lines = juce::StringArray::fromLines(file.loadFileAsString());
            juce::String name = file.getFileNameWithoutExtension();
            for (const auto& line : lines) if (line.startsWithIgnoreCase("name=")) name = line.fromFirstOccurrenceOf("=", false, false).trim();
            dm::CustomPreset preset; preset.id = file.getFullPathName(); preset.name = name;
            for (const auto& line : lines) {
                if (!line.startsWithIgnoreCase("part=")) continue;
                const auto fields = juce::StringArray::fromTokens(line.fromFirstOccurrenceOf("=", false, false), "|", "");
                if (fields.size() < 6) continue;
                const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), fields[0].toStdString());
                if (found == dm::featureIds.end()) continue;
                dm::CustomEffectPart part; part.effectId = fields[0];
                for (int i = 0; i < 5; ++i) part.controls[static_cast<size_t>(i)] = juce::jlimit(0.0f, 1.0f, fields[i + 1].getFloatValue());
                preset.parts.push_back(part);
            }
            if (!preset.parts.empty()) { availablePresets.push_back(std::move(preset)); presetChoiceBox.addItem(name, presetChoiceBox.getNumItems() + 1); }
        }
    } catch (...) {
        builderStatusLabel.setText("Custom FX folder could not be read safely.", juce::dontSendNotification);
    }
}

void DreamMasterLiteEditor::saveBuilderPreset() {
    if (!authenticated || builderSteps.empty()) {
        builderStatusLabel.setText(builderSteps.empty() ? "Add at least one component before saving." : "Log in to use the FX Builder.", juce::dontSendNotification);
        return;
    }
    if (!customFxDirectory().isDirectory()) {
        chooseCustomFxFolder();
        builderStatusLabel.setText("Choose a folder once; future custom FX saves go there automatically.", juce::dontSendNotification);
        return;
    }
    auto name = presetNameEditor.getText().trim();
    if (name.isEmpty()) { builderStatusLabel.setText("Name the custom effect first.", juce::dontSendNotification); return; }
    name = name.replaceCharacters("\\/:*?\"<>|", "_________").trim();
    const auto file = customFxDirectory().getChildFile(name + ".dmefx");
    juce::String code = "DMEFX1\nname=" + name + "\nkind=composite\nparts=" + juce::String(builderSteps.size()) + "\n";
    for (const auto& part : builderSteps) {
        code += "part=" + effectId(part.effectIndex);
        for (auto value : part.controls) code += "|" + juce::String(value, 4);
        code += "\n";
    }
    code += "end\n";
    try {
        if (!file.replaceWithText(code, false, false)) throw std::runtime_error("write failed");
        loadCustomEffectFiles();
        for (int i = 0; i < availablePresets.size(); ++i) if (availablePresets[static_cast<size_t>(i)].id == file.getFullPathName()) presetChoiceBox.setSelectedId(i + 1, juce::dontSendNotification);
        builderStatusLabel.setText("Saved lightweight String FX code: " + file.getFileName(), juce::dontSendNotification);
    } catch (...) { builderStatusLabel.setText("Custom FX could not be saved; the builder state was left intact.", juce::dontSendNotification); }
}

void DreamMasterLiteEditor::loadSelectedPreset() {
    const int selected = presetChoiceBox.getSelectedId() - 1;
    if (selected < 0 || selected >= static_cast<int>(availablePresets.size())) return;
    const auto preset = availablePresets[static_cast<size_t>(selected)];
    if (preset.parts.empty()) { builderStatusLabel.setText("Custom FX file contains no valid components.", juce::dontSendNotification); return; }
    builderSteps.clear();
    for (int i = 0; i < dm::DspEngine::effectCount; ++i)
        if (auto* p = proc.state.getParameter("fx" + juce::String(i))) p->setValueNotifyingHost(0.0f);
    for (const auto& part : preset.parts) {
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), part.effectId.toStdString());
        if (found == dm::featureIds.end()) continue;
        const int index = static_cast<int>(std::distance(dm::featureIds.begin(), found));
        BuilderStep step; step.effectIndex = index; step.controls = part.controls; builderSteps.push_back(step);
        if (auto* p = proc.state.getParameter("fx" + juce::String(index))) p->setValueNotifyingHost(1.0f);
        const std::array<const char*, 5> ids{{"amt", "tone", "motion", "mix", "shape"}};
        for (int c = 0; c < 5; ++c) if (auto* p = proc.state.getParameter(juce::String(ids[c]) + juce::String(index))) p->setValueNotifyingHost(part.controls[static_cast<size_t>(c)]);
    }
    proc.setActiveEffectOrder({});
    updateBuilderChainList();
    if (!builderSteps.empty()) chainList.selectRow(0);
    builderStatusLabel.setText("Loaded one composite FX from its local String code file.", juce::dontSendNotification);
    updateActiveCount();
}

void DreamMasterLiteEditor::deleteSelectedPreset() {
    const int selected = presetChoiceBox.getSelectedId() - 1;
    if (selected < 0 || selected >= static_cast<int>(availablePresets.size())) return;
    const auto file = juce::File(availablePresets[static_cast<size_t>(selected)].id);
    try { if (file.existsAsFile()) file.deleteFile(); } catch (...) {}
    loadCustomEffectFiles();
    builderStatusLabel.setText("Deleted local custom FX file.", juce::dontSendNotification);
}

void DreamMasterLiteEditor::moveSelectedEffect(int) {}

void DreamMasterLiteEditor::updateBuilderAmount() {
    const int selected = chainList.getSelectedRow();
    const std::array<juce::Slider*, 5> sliders{{&builderAmountSlider, &builderToneSlider, &builderMotionSlider, &builderMixSlider, &builderShapeSlider}};
    if (selected < 0 || selected >= static_cast<int>(builderSteps.size())) return;
    for (int i = 0; i < 5; ++i) builderSteps[static_cast<size_t>(selected)].controls[static_cast<size_t>(i)] = static_cast<float>(sliders[static_cast<size_t>(i)]->getValue());
    chainList.repaintRow(selected);
}

void DreamMasterLiteEditor::updateBuilderControlsFromSelection() { selectedRowsChanged(chainList.getSelectedRow()); }

void DreamMasterLiteEditor::updateBuilderChainList() {
    chainHeading.setText("2. COMPOSITE PARTS (" + juce::String(builderSteps.size()) + ")", juce::dontSendNotification);
    chainList.updateContent(); chainList.repaint();
}

void DreamMasterLiteEditor::updatePresetList() { loadCustomEffectFiles(); }

void DreamMasterLiteEditor::updateActiveCount() {
    int active = 0;
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (auto* parameter = proc.state.getRawParameterValue("fx" + juce::String(i)))
            active += parameter->load(std::memory_order_relaxed) >= 0.5f ? 1 : 0;
    }
    countLabel.setText(juce::String(active) + " ACTIVE", juce::dontSendNotification);
}

void DreamMasterLiteEditor::updateAuthenticationUi() {
    authenticated = proc.hasDreamShareSession();
    loginButton.setVisible(!authenticated);
    logoutButton.setVisible(authenticated);
    themeButton.setEnabled(true);
    builderLockLabel.setVisible(!builderAvailable);
    builderLockLabel.setText(!authenticated ? "FX BUILDER LOCKED - LOG IN TO DREAMSHARE"
        : (builderAvailable ? "FX BUILDER - COMPOSITE EFFECT MODE" : "FX BUILDER ACCESS UNAVAILABLE"),
        juce::dontSendNotification);
    const std::array<juce::Component*, 15> builderControls{{
        &effectBrowserList, &builderAmountSlider, &builderToneSlider, &builderMotionSlider, &builderMixSlider, &builderShapeSlider,
        &addEffectButton, &chainList, &removeEffectButton, &setFolderButton, &presetNameEditor, &presetChoiceBox,
        &savePresetButton, &loadPresetButton, &deletePresetButton
    }};
    for (auto* control : builderControls)
        control->setEnabled(builderAvailable);
    if (authenticated) {
        accountLabel.setText("SIGNED IN | " + proc.getDreamShareUser(), juce::dontSendNotification);
        updatePresetList();
    } else {
        accountLabel.setText("SIGNED OUT", juce::dontSendNotification);
        availablePresets.clear();
        presetChoiceBox.clear(juce::dontSendNotification);
        if (builderPageActive) {
            builderPageActive = false;
            viewport.setViewedComponent(&content, false);
            content.setVisible(true);
            builderContent.setVisible(false);
        }
    }
    resized();
}

void DreamMasterLiteEditor::requestWorkerAsync(const juce::var* request, bool get,
                                              std::function<void(WorkerReply)> callback) {
    const juce::var requestCopy = request == nullptr ? juce::var() : *request;
    dreamShareWorkerPool().addJob(new WorkerRequestJob(requestCopy, get, std::move(callback)), true);
}

void DreamMasterLiteEditor::showLoginPopup() {
    if (authenticated)
        return;
    loginStatusLabel.setText({}, juce::dontSendNotification);
    loginOverlay->setVisible(true);
    loginOverlay->toFront(true);
    usernameEditor.grabKeyboardFocus();
    resized();
    repaint();
}

void DreamMasterLiteEditor::dismissLoginPopup() {
    loginOverlay->setVisible(false);
    updateLoginPreference(true);
    repaint();
}

void DreamMasterLiteEditor::login() {
    if (loginRequestPending)
        return;
    const auto username = usernameEditor.getText().trim();
    const auto password = passwordEditor.getText();
    if (username.isEmpty() || password.isEmpty()) {
        loginStatusLabel.setText("ENTER USERNAME AND PASSWORD", juce::dontSendNotification);
        return;
    }
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "login");
    request->setProperty("user", username);
    request->setProperty("pass", password);
    const juce::var body(request.get());
    passwordEditor.clear();
    loginRequestPending = true;
    loginStatusLabel.setText("CONNECTING SECURELY...", juce::dontSendNotification);
    loginSubmitButton.setEnabled(false);
    loginSubmitButton.setButtonText("SIGNING IN...");
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        safeThis->loginRequestPending = false;
        safeThis->loginSubmitButton.setEnabled(true);
        safeThis->loginSubmitButton.setButtonText("SIGN IN");
        if (reply.error.isNotEmpty()) {
            safeThis->loginStatusLabel.setText(reply.error, juce::dontSendNotification);
            return;
        }
        auto* response = reply.payload.getDynamicObject();
        if (response == nullptr) {
            safeThis->loginStatusLabel.setText("DREAMSHARE LOGIN FAILED", juce::dontSendNotification);
            return;
        }
        const auto token = response->getProperty("token").toString();
        const auto user = response->getProperty("user").toString();
        const auto theme = response->getProperty("theme").toString();
        if (!responseIsOk(reply.payload) || token.isEmpty() || user.isEmpty()) {
            safeThis->loginStatusLabel.setText("DREAMSHARE LOGIN FAILED", juce::dontSendNotification);
            return;
        }
        const bool savedSecurely = safeThis->proc.setDreamShareSession(token, user, theme);
        safeThis->updateLoginPreference(true);
        safeThis->loginOverlay->setVisible(false);
        safeThis->authenticated = true;
        safeThis->populateThemes(response->getProperty("themes"));
        safeThis->applyThemePack(response->getProperty("themePack"));
        safeThis->updateAuthenticationUi();
        safeThis->refreshCapabilities();
        safeThis->sendPresenceHeartbeat();
        safeThis->accountLabel.setText("SIGNED IN | " + safeThis->proc.getDreamShareUser()
            + (savedSecurely ? " | SESSION SAVED" : " | SESSION MEMORY-ONLY"), juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::logout() {
    proc.clearDreamShareSession();
    authenticated = false;
    builderAvailable = false;
    updateLoginPreference(true);
    updateAuthenticationUi();
    accountLabel.setText("SIGNED OUT | LOG IN WHEN NEEDED", juce::dontSendNotification);
}

void DreamMasterLiteEditor::validateSession() {
    if (!proc.hasDreamShareSession())
        return;
    sessionValidationPending = true;
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "session");
    request->setProperty("token", proc.getDreamShareToken());
    const juce::var body(request.get());
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        safeThis->sessionValidationPending = false;
        if (reply.error.isNotEmpty()) {
            safeThis->accountLabel.setText("SESSION CHECK FAILED | " + reply.error, juce::dontSendNotification);
            return;
        }
        auto* response = reply.payload.getDynamicObject();
        if (response == nullptr || !responseIsOk(reply.payload)) {
            safeThis->proc.clearDreamShareSession();
            safeThis->builderAvailable = false;
            safeThis->updateAuthenticationUi();
            safeThis->accountLabel.setText("DREAMSHARE SESSION EXPIRED", juce::dontSendNotification);
            return;
        }
        const auto refreshedToken = response->getProperty("token").toString();
        const auto theme = response->getProperty("theme").toString();
        safeThis->proc.setDreamShareSession(refreshedToken.isEmpty() ? safeThis->proc.getDreamShareToken() : refreshedToken,
                                            safeThis->proc.getDreamShareUser(),
                                            theme.isEmpty() ? safeThis->proc.getDreamShareTheme() : theme);
        safeThis->populateThemes(response->getProperty("themes"));
        safeThis->applyThemePack(response->getProperty("themePack"));
        safeThis->authenticated = true;
        safeThis->updateAuthenticationUi();
        safeThis->refreshCapabilities();
        safeThis->sendPresenceHeartbeat();
    });
}

void DreamMasterLiteEditor::refreshCapabilities() {
    if (!proc.hasDreamShareSession()) {
        builderAvailable = false;
        updateAuthenticationUi();
        return;
    }
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "plugin_capabilities");
    request->setProperty("token", proc.getDreamShareToken());
    const juce::var body(request.get());
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        if (reply.error.isNotEmpty() || !responseIsOk(reply.payload)) {
            safeThis->builderAvailable = false;
            safeThis->updateAuthenticationUi();
            safeThis->builderStatusLabel.setText(
                "FX Builder capability is unavailable; DreamShare login and themes remain active.",
                juce::dontSendNotification);
            return;
        }
        auto* response = reply.payload.getDynamicObject();
        auto* capabilities = response == nullptr ? nullptr
            : response->getProperty("capabilities").getDynamicObject();
        safeThis->builderAvailable = capabilities != nullptr
            && valueIsTrue(capabilities->getProperty("fxBuilder"));
        if (capabilities != nullptr) {
            const auto allowedThemes = capabilities->getProperty("themes");
            if (const auto* ids = allowedThemes.getArray()) {
                juce::StringArray permitted;
                for (const auto& item : *ids)
                    permitted.add(item.toString());
                for (int i = safeThis->themeIds.size() - 1; i >= 0; --i) {
                    if (!permitted.contains(safeThis->themeIds[i])) {
                        safeThis->themeIds.remove(i);
                        safeThis->themeNames.remove(i);
                    }
                }
                safeThis->themeButton.setEnabled(true);
                safeThis->updateThemeButtonLabel();
            }
        }
        safeThis->updateAuthenticationUi();
        if (!safeThis->builderAvailable)
            safeThis->builderStatusLabel.setText("FX Builder is not enabled for this account.", juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::refreshOnlineCount() {
    if (onlineRequestPending)
        return;
    onlineRequestPending = true;
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(nullptr, true, [safeThis](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        safeThis->onlineRequestPending = false;
        if (reply.error.isNotEmpty()) {
            safeThis->onlineLabel.setText("ONLINE: UNAVAILABLE", juce::dontSendNotification);
            return;
        }
        auto* response = reply.payload.getDynamicObject();
        const auto online = response == nullptr ? juce::var() : response->getProperty("online");
        if (const auto* users = online.getArray())
            safeThis->onlineLabel.setText("ONLINE: " + juce::String(users->size()), juce::dontSendNotification);
        else
            safeThis->onlineLabel.setText("ONLINE: UNAVAILABLE", juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::sendPresenceHeartbeat() {
    if (!proc.hasDreamShareSession() || heartbeatPending)
        return;
    heartbeatPending = true;
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "presence");
    request->setProperty("token", proc.getDreamShareToken());
    const juce::var body(request.get());
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        safeThis->heartbeatPending = false;
        safeThis->lastHeartbeatMs = juce::Time::getMillisecondCounter();
        if (reply.statusCode == 401 || (reply.error.isEmpty() && !responseIsOk(reply.payload))) {
            safeThis->proc.clearDreamShareSession();
            safeThis->builderAvailable = false;
            safeThis->updateAuthenticationUi();
            safeThis->accountLabel.setText("DREAMSHARE SESSION EXPIRED", juce::dontSendNotification);
        }
    });
}

void DreamMasterLiteEditor::populateThemes(const juce::var& themes) {
    const auto* summaries = themes.getArray();
    if (summaries == nullptr)
        return;
    themeIds.clear();
    themeNames.clear();
    for (const auto& item : *summaries) {
        auto* summary = item.getDynamicObject();
        if (summary == nullptr)
            continue;
        const auto id = summary->getProperty("id").toString();
        const auto name = summary->getProperty("name").toString();
        if (id.isEmpty() || name.isEmpty() || themeIds.contains(id))
            continue;
        themeIds.add(id);
        themeNames.add(name);
    }
    if (themeIds.indexOf(proc.getDreamShareTheme()) < 0 && !themeIds.isEmpty())
        proc.setDreamShareTheme(themeIds[0]);
    updateThemeButtonLabel();
    themeButton.setEnabled(true);
}

void DreamMasterLiteEditor::updateThemeButtonLabel() {
    const int index = themeIds.indexOf(proc.getDreamShareTheme());
    themeButton.setButtonText(index >= 0 ? "THEME: " + themeNames[index] : "THEME: DEFAULT");
}

void DreamMasterLiteEditor::applyThemePack(const juce::var& themePack) {
    auto* pack = themePack.getDynamicObject();
    if (pack == nullptr)
        return;
    currentThemePack = themePack;
    const auto themeId = pack->getProperty("id").toString();
    if (themeId.isNotEmpty())
        proc.setDreamShareTheme(themeId);
    auto* variables = pack->getProperty("vars").getDynamicObject();
    if (variables != nullptr) {
        auto findColour = [variables](std::initializer_list<const char*> keys, juce::Colour fallback) {
            for (const auto* key : keys) {
                const auto value = variables->getProperty(key).toString();
                if (value.isNotEmpty())
                    return colourFromCss(value, fallback);
            }
            return fallback;
        };
        backgroundColour = findColour({"--bg", "--background", "--page-bg"}, backgroundColour);
        panelColour = findColour({"--panel", "--surface", "--card"}, panelColour);
        textColour = findColour({"--text", "--fg", "--foreground"}, textColour);
        accentColour = findColour({"--accent", "--primary", "--neon"}, accentColour);
        highlightColour = findColour({"--accent-2", "--accent2", "--highlight", "--glow"}, highlightColour);
        borderColour = findColour({"--border", "--line"}, borderColour);
        mutedColour = findColour({"--muted", "--text-muted"}, mutedColour);
    }
    applyPalette();
    updateThemeButtonLabel();
    updateAnimationTimer();
    repaint();
}

void DreamMasterLiteEditor::applyPalette() {
    title.setColour(juce::Label::textColourId, accentColour);
    subtitle.setColour(juce::Label::textColourId, mutedColour);
    countLabel.setColour(juce::Label::textColourId, highlightColour);
    accountLabel.setColour(juce::Label::textColourId, mutedColour);
    builderLockLabel.setColour(juce::Label::textColourId, accentColour);
    builderStatusLabel.setColour(juce::Label::textColourId, mutedColour);
    for (auto* card : effectCards)
        card->setPalette(backgroundColour, panelColour, borderColour, accentColour, highlightColour, textColour);
    for (auto* button : {&randomButton, &definedButton, &websiteButton, &rackPageButton, &builderPageButton,
                         &themeButton, &moveUpButton, &moveDownButton, &setFolderButton, &savePresetButton,
                         &loadPresetButton, &deletePresetButton})
        styleButton(*button, panelColour, accentColour, highlightColour);
    styleButton(randomButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(extremeButton, juce::Colour(0xff241018), juce::Colour(0xffff7272), juce::Colour(0xffffb047));
    styleButton(resetButton, juce::Colour(0xff20130e), textColour, highlightColour);
    styleButton(addEffectButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(removeEffectButton, juce::Colour(0xff291410), juce::Colour(0xffff9b83), juce::Colour(0xffffc0a8));
    styleButton(loginSubmitButton, panelColour.darker(0.12f), accentColour, highlightColour);
    styleButton(loginCancelButton, panelColour, mutedColour, highlightColour);
    for (auto* button : categoryButtons)
        button->setColour(juce::TextButton::buttonColourId, panelColour);
    for (auto* editor : {&usernameEditor, &passwordEditor, &presetNameEditor, &searchEditor}) {
        editor->setColour(juce::TextEditor::backgroundColourId, panelColour);
        editor->setColour(juce::TextEditor::textColourId, textColour);
        editor->setColour(juce::TextEditor::outlineColourId, borderColour);
    }
    loginTitle.setColour(juce::Label::textColourId, accentColour);
    loginStatusLabel.setColour(juce::Label::textColourId, mutedColour);
    loginOverlay->setPalette(panelColour, borderColour, accentColour);
    for (auto* slider : {&builderAmountSlider, &builderToneSlider, &builderMotionSlider, &builderMixSlider, &builderShapeSlider}) {
        slider->setColour(juce::Slider::trackColourId, accentColour);
        slider->setColour(juce::Slider::thumbColourId, highlightColour);
        slider->setColour(juce::Slider::textBoxTextColourId, textColour);
        slider->setColour(juce::Slider::textBoxBackgroundColourId, panelColour);
        slider->setColour(juce::Slider::textBoxOutlineColourId, borderColour);
    }
    for (auto* label : {&builderAmountLabel, &builderToneLabel, &builderMotionLabel, &builderMixLabel, &builderShapeLabel}) label->setColour(juce::Label::textColourId, mutedColour);
    chainList.setColour(juce::ListBox::backgroundColourId, panelColour);
    chainList.setColour(juce::ListBox::outlineColourId, borderColour);
    viewport.setColour(juce::ScrollBar::thumbColourId, accentColour.withAlpha(0.8f));
    viewport.setColour(juce::ScrollBar::trackColourId, backgroundColour);
    updateCategoryButtons();
    repaint();
}

void DreamMasterLiteEditor::showThemeMenu() {
    if (!authenticated)
        return;
    juce::PopupMenu menu;
    for (int i = 0; i < themeIds.size(); ++i)
        menu.addItem(i + 1, themeNames[i], true, themeIds[i] == proc.getDreamShareTheme());
    if (!themeIds.isEmpty())
        menu.addSeparator();
    const int reducedMotionItem = themeIds.size() + 1;
    menu.addItem(reducedMotionItem, "Reduce background motion", true, reducedMotion);
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&themeButton),
                       [safeThis, reducedMotionItem](int result) {
        if (safeThis == nullptr || result <= 0)
            return;
        if (result == reducedMotionItem) {
            safeThis->reducedMotion = !safeThis->reducedMotion;
            safeThis->updateAnimationTimer();
            safeThis->repaint();
            return;
        }
        if (result > safeThis->themeIds.size())
            return;
        safeThis->selectTheme(result - 1);
    });
}

void DreamMasterLiteEditor::selectTheme(int selectedIndex) {
    if (!authenticated || selectedIndex < 0 || selectedIndex >= themeIds.size())
        return;
    const auto theme = themeIds[selectedIndex];
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "set_theme");
    request->setProperty("token", proc.getDreamShareToken());
    request->setProperty("theme", theme);
    const juce::var body(request.get());
    const auto previousTheme = proc.getDreamShareTheme();
    accountLabel.setText("CHANGING THEME...", juce::dontSendNotification);
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis, previousTheme](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        if (reply.error.isNotEmpty() || !responseIsOk(reply.payload)) {
            safeThis->accountLabel.setText(reply.error.isNotEmpty() ? reply.error : "THEME CHANGE FAILED",
                                           juce::dontSendNotification);
            const int previous = safeThis->themeIds.indexOf(previousTheme);
            if (previous >= 0)
                safeThis->themeButton.setButtonText("THEME: " + safeThis->themeNames[previous]);
            return;
        }
        auto* response = reply.payload.getDynamicObject();
        safeThis->proc.setDreamShareTheme(response->getProperty("theme").toString());
        safeThis->populateThemes(response->getProperty("themes"));
        safeThis->applyThemePack(response->getProperty("themePack"));
        safeThis->accountLabel.setText("THEME: " + safeThis->proc.getDreamShareTheme(), juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::timerCallback() {
    if (!isShowing()) {
        startTimer(1000);
        return;
    }
    const auto now = juce::Time::getMillisecondCounter();
    const auto theme = normalisedThemeId(currentThemePack);
    const bool animatedTheme = theme == "goonr" || theme == "trippah";
    if (!reducedMotion) {
        animationFrame = now / 66u;
        // GOONR's native visual cadence is the reference. INXOMNIA glitches at half
        // that event rate, while the glitch itself is theme-independent.
        const auto cadence = animationFrame / 120u;
        glitchActive = (cadence % 2u) == 0u && (animationFrame % 120u) < 3u;
        if (animatedTheme || glitchActive) repaint();
    } else {
        glitchActive = false;
    }
    if (now - lastCountUpdateMs >= 1000u) {
        updateActiveCount();
        lastCountUpdateMs = now;
    }
    if (now - lastOnlinePollMs >= 60000u) {
        refreshOnlineCount();
        lastOnlinePollMs = now;
    }
    if (authenticated && !sessionValidationPending && !heartbeatPending && now - lastHeartbeatMs >= 25000u)
        sendPresenceHeartbeat();
}

void DreamMasterLiteEditor::updateAnimationTimer() {
    const auto theme = normalisedThemeId(currentThemePack);
    if (isShowing() && !reducedMotion)
        startTimerHz(15);
    else
        startTimer(1000);
}

void DreamMasterLiteEditor::updateLoginPreference(bool dismissed) {
    updateLoginPreferenceFile(true, dismissed, nullptr);
}

bool DreamMasterLiteEditor::readLoginPreference() const {
    bool dismissed = false;
    updateLoginPreferenceFile(false, false, &dismissed);
    return dismissed;
}

juce::AudioProcessorEditor* DreamMasterLiteProcessor::createEditor() {
    return new DreamMasterLiteEditor(*this);
}
