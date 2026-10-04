#include "PluginEditor.h"
#include "Randomizer.h"
#include "DreamShareCredentialStore.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>

namespace {
constexpr auto workerUrl = "https://dreamshare-api.keganacummings.workers.dev/";
constexpr int allCategory = -1;
constexpr int favoritesCategory = -2;
constexpr int categoryRadioGroup = 1407;

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
    options.applicationName = "DreamMasterLite";
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
            favorite.setButtonText(favorite.getToggleState() ? "★" : "☆");
            favoriteChangedCallback();
        };
        addAndMakeVisible(favorite);

        enabled.setButtonText("OFF");
        enabled.setClickingTogglesState(true);
        enabled.setColour(juce::ToggleButton::textColourId, juce::Colour(0xff9caaa0));
        enabled.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffc8ff33));
        addAndMakeVisible(enabled);
        enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.state, "fx" + juce::String(index), enabled);
        enabled.onStateChange = [this] {
            enabled.setButtonText(enabled.getToggleState() ? "ON" : "OFF");
            repaint();
        };

        amount.setSliderStyle(juce::Slider::LinearHorizontal);
        amount.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amount.setRange(0.0, 1.0, 0.001);
        amount.setScrollWheelEnabled(false);
        addAndMakeVisible(amount);
        amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.state, "amt" + juce::String(index), amount);
        setSize(220, 66);
        updateFavoriteState();
        setPalette(juce::Colour(0xff030805), juce::Colour(0xff100a08), juce::Colour(0xff563526),
                   juce::Colour(0xff65ff83), juce::Colour(0xffc8ff33), juce::Colour(0xffd8ffe0));
    }

    int getEffectIndex() const { return effectIndex; }

    void updateFavoriteState() {
        const bool isFavorite = proc.isEffectFavorite(effectId(effectIndex));
        favorite.setToggleState(isFavorite, juce::dontSendNotification);
        favorite.setButtonText(isFavorite ? "★" : "☆");
    }

    void setPalette(juce::Colour background, juce::Colour panel, juce::Colour border,
                    juce::Colour accent, juce::Colour highlight, juce::Colour text) {
        backgroundColour = background;
        panelColour = panel;
        borderColour = border;
        accentColour = accent;
        highlightColour = highlight;
        name.setColour(juce::Label::textColourId, text);
        favorite.setColour(juce::TextButton::buttonColourId, panel);
        favorite.setColour(juce::TextButton::buttonOnColourId, highlight.withAlpha(0.22f));
        favorite.setColour(juce::TextButton::textColourOffId, accent);
        favorite.setColour(juce::TextButton::textColourOnId, highlight);
        enabled.setColour(juce::ToggleButton::textColourId, text);
        enabled.setColour(juce::ToggleButton::tickColourId, highlight);
        amount.setColour(juce::Slider::trackColourId, accent);
        amount.setColour(juce::Slider::thumbColourId, highlight);
        amount.setColour(juce::Slider::backgroundColourId, background.darker(0.3f));
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const bool isOn = enabled.getToggleState();
        g.setColour(panelColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
        g.setColour(isOn ? accentColour : borderColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, isOn ? 1.7f : 1.0f);
        if (isOn) {
            g.setColour(accentColour.withAlpha(0.8f));
            g.fillRoundedRectangle(1.0f, 6.0f, 3.0f, static_cast<float>(getHeight() - 12), 1.5f);
        }
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(8, 5);
        auto header = bounds.removeFromTop(25);
        favorite.setBounds(header.removeFromRight(27).reduced(1));
        enabled.setBounds(header.removeFromRight(49).reduced(1));
        name.setBounds(header.reduced(1, 0));
        amount.setBounds(bounds.removeFromTop(22).reduced(2, 1));
    }

private:
    DreamMasterLiteProcessor& proc;
    int effectIndex = -1;
    std::function<void()> favoriteChangedCallback;
    juce::Label name;
    juce::TextButton favorite;
    juce::ToggleButton enabled;
    juce::Slider amount;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    juce::Colour backgroundColour, panelColour, borderColour, accentColour, highlightColour;
};

DreamMasterLiteEditor::DreamMasterLiteEditor(DreamMasterLiteProcessor& processor)
    : AudioProcessorEditor(processor), proc(processor) {
    title.setText("DREAMMASTERLITE", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    title.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title);
    subtitle.setText("INSOMNIA FX RACK  /  200 DISTINCT NAMED MODULES", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(10.5f)));
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    countLabel.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    addAndMakeVisible(countLabel);
    onlineLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    onlineLabel.setText("ONLINE: —", juce::dontSendNotification);
    addAndMakeVisible(onlineLabel);
    accountLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    addAndMakeVisible(accountLabel);

    styleButton(randomButton, juce::Colour(0xff102b17), accentColour, highlightColour);
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
    definedButton.onClick = [this] { randomize(1, 10, true); };
    resetButton.onClick = [this] { resetAll(); };
    websiteButton.onClick = [] { juce::URL("https://dreamdaw.com").launchInDefaultBrowser(); };
    rackPageButton.onClick = [this] { showPage(false); };
    builderPageButton.onClick = [this] { showPage(true); };
    loginButton.onClick = [this] { showLoginPopup(); };
    logoutButton.onClick = [this] { logout(); };
    themeButton.onClick = [this] { showThemeMenu(); };

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
            button->setButtonText("★ FAVORITES");
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

    searchEditor.setTextToShowWhenEmpty("Search effects…", juce::Colours::grey);
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

    styleButton(chooseEffectButton, panelColour, accentColour, highlightColour);
    styleButton(addEffectButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(moveUpButton, panelColour, accentColour, highlightColour);
    styleButton(moveDownButton, panelColour, accentColour, highlightColour);
    styleButton(removeEffectButton, juce::Colour(0xff291410), juce::Colour(0xffff9b83), juce::Colour(0xffffc0a8));
    styleButton(savePresetButton, panelColour, accentColour, highlightColour);
    styleButton(loadPresetButton, panelColour, accentColour, highlightColour);
    styleButton(deletePresetButton, panelColour, mutedColour, highlightColour);
    for (auto* button : {&chooseEffectButton, &addEffectButton, &moveUpButton, &moveDownButton,
                         &removeEffectButton, &savePresetButton, &loadPresetButton, &deletePresetButton})
        builderContent.addAndMakeVisible(*button);
    chooseEffectButton.onClick = [this] { showEffectBrowserMenu(); };
    addEffectButton.onClick = [this] { addBuilderEffect(); };

    builderLockLabel.setText("FX BUILDER LOCKED · LOG IN TO DREAMSHARE", juce::dontSendNotification);
    builderLockLabel.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    builderLockLabel.setJustificationType(juce::Justification::centredLeft);
    builderContent.addAndMakeVisible(builderLockLabel);
    builderAmountSlider.setRange(0.0, 1.0, 0.001);
    builderAmountSlider.setValue(0.22, juce::dontSendNotification);
    builderAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    builderAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 22);
    builderAmountSlider.setScrollWheelEnabled(false);
    builderAmountSlider.onValueChange = [this] { updateBuilderAmount(); };
    builderContent.addAndMakeVisible(builderAmountSlider);

    chainList.setRowHeight(27);
    chainList.setMultipleSelectionEnabled(false);
    chainList.setColour(juce::ListBox::backgroundColourId, panelColour);
    chainList.setColour(juce::ListBox::outlineColourId, borderColour);
    builderContent.addAndMakeVisible(chainList);
    moveUpButton.onClick = [this] { moveSelectedEffect(-1); };
    moveDownButton.onClick = [this] { moveSelectedEffect(1); };
    removeEffectButton.onClick = [this] {
        const int selected = chainList.getSelectedRow();
        if (selected >= 0 && selected < static_cast<int>(builderSteps.size())) {
            builderSteps.erase(builderSteps.begin() + selected);
            updateBuilderChainList();
            if (!builderSteps.empty())
                chainList.selectRow(juce::jmin(selected, static_cast<int>(builderSteps.size()) - 1));
        }
    };
    presetNameEditor.setTextToShowWhenEmpty("Name this FX chain", juce::Colours::grey);
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, panelColour);
    presetNameEditor.setColour(juce::TextEditor::textColourId, textColour);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, borderColour);
    builderContent.addAndMakeVisible(presetNameEditor);
    builderContent.addAndMakeVisible(presetChoiceBox);
    savePresetButton.onClick = [this] { saveBuilderPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    builderStatusLabel.setFont(juce::Font(juce::FontOptions(10.5f)));
    builderContent.addAndMakeVisible(builderStatusLabel);
    builderContent.setVisible(false);

    loginTitle.setText("DREAMSHARE LOGIN", juce::dontSendNotification);
    loginTitle.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    loginTitle.setJustificationType(juce::Justification::centred);
    loginOverlay.addAndMakeVisible(loginTitle);
    loginStatusLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    loginStatusLabel.setJustificationType(juce::Justification::centred);
    loginStatusLabel.setColour(juce::Label::textColourId, mutedColour);
    loginOverlay.addAndMakeVisible(loginStatusLabel);
    usernameEditor.setTextToShowWhenEmpty("Username", juce::Colours::grey);
    passwordEditor.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passwordEditor.setPasswordCharacter(0x2022);
    for (auto* editor : {&usernameEditor, &passwordEditor}) {
        editor->setColour(juce::TextEditor::backgroundColourId, panelColour);
        editor->setColour(juce::TextEditor::textColourId, textColour);
        editor->setColour(juce::TextEditor::outlineColourId, borderColour);
        loginOverlay.addAndMakeVisible(*editor);
    }
    loginOverlay.addAndMakeVisible(loginSubmitButton);
    loginOverlay.addAndMakeVisible(loginCancelButton);
    styleButton(loginSubmitButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(loginCancelButton, panelColour, mutedColour, highlightColour);
    loginSubmitButton.onClick = [this] { login(); };
    loginCancelButton.onClick = [this] { dismissLoginPopup(); };
    passwordEditor.onReturnKey = [this] { login(); };
    addAndMakeVisible(loginOverlay);
    loginOverlay.setVisible(false);

    updateCategoryButtons();
    updateVisibleEffects();
    updateAuthenticationUi();
    updateActiveCount();
    setResizable(true, true);
    setResizeLimits(760, 580, 1500, 1200);
    setSize(1040, 800);

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
    viewport.setViewedComponent(nullptr, false);
}

void DreamMasterLiteEditor::paint(juce::Graphics& g) {
    g.fillAll(backgroundColour);
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient glow(highlightColour.withAlpha(0.07f), bounds.getWidth() * 0.18f, 0.0f,
                              juce::Colours::transparentBlack, bounds.getWidth() * 0.62f, bounds.getHeight(), false);
    g.setGradientFill(glow);
    g.fillRect(bounds);
    for (int x = 12; x < getWidth(); x += 31) {
        for (int y = 7 + ((x * 13) % 43); y < getHeight(); y += 23 + ((x / 31) % 4) * 7) {
            const int glyph = (x * 7 + y * 3) % 4;
            g.setColour(accentColour.withAlpha(glyph == 0 ? 0.14f : 0.045f));
            g.setFont(juce::Font(juce::FontOptions(10.0f)));
            g.drawText(glyph == 0 ? "1" : (glyph == 1 ? "+" : (glyph == 2 ? ":" : "0")),
                       x, y, 9, 11, juce::Justification::centred);
        }
    }

    const auto theme = normalisedThemeId(currentThemePack);
    if (theme == "goonr") {
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        for (int x = 8; x < getWidth(); x += 21) {
            const int phase = (x * 29 + static_cast<int>(animationFrame * 3u)) % juce::jmax(1, getHeight() + 90);
            const int y = phase - 42;
            g.setColour(accentColour.withAlpha(0.22f));
            g.drawText("0", x, y, 12, 12, juce::Justification::centred);
            g.setColour(highlightColour.withAlpha(0.14f));
            g.drawText("1", x, y - 24, 12, 12, juce::Justification::centred);
        }
    } else if (theme == "trippah") {
        for (int i = 0; i < 28; ++i) {
            const float x = static_cast<float>((i * 83 + 19) % juce::jmax(1, getWidth()));
            const float drift = std::sin(static_cast<float>(animationFrame) * 0.025f + i * 1.7f) * 13.0f;
            const float y = static_cast<float>((i * 47 + static_cast<int>(animationFrame * (1u + static_cast<uint32_t>(i % 3))) * 2)
                                               % juce::jmax(1, getHeight()));
            const float diameter = 2.0f + static_cast<float>(i % 3);
            g.setColour((i % 4 == 0 ? highlightColour : accentColour).withAlpha(0.12f));
            g.fillEllipse(x + drift, y, diameter, diameter);
        }
    }

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    for (int y = 0; y < getHeight(); y += 4)
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

    if (loginOverlay.isVisible()) {
        g.setColour(juce::Colours::black.withAlpha(0.76f));
        g.fillRect(getLocalBounds());
        auto panel = getLocalBounds().withSizeKeepingCentre(430, 260).toFloat();
        g.setColour(panelColour);
        g.fillRoundedRectangle(panel, 8.0f);
        g.setColour(accentColour);
        g.drawRoundedRectangle(panel, 8.0f, 1.6f);
    }
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
    const int gap = 8, cardHeight = 68;
    const int cardWidth = juce::jmax(165, (viewport.getWidth() - 27 - gap * (columns - 1)) / columns);
    const int rows = (static_cast<int>(visibleEffects.size()) + columns - 1) / columns;
    content.setSize(juce::jmax(viewport.getWidth() - 16, columns * cardWidth + (columns - 1) * gap),
                    juce::jmax(viewport.getHeight(), rows * (cardHeight + gap)));
    for (int position = 0; position < effectCards.size(); ++position) {
        const int col = position % columns, row = position / columns;
        effectCards[position]->setBounds(col * (cardWidth + gap), row * (cardHeight + gap), cardWidth, cardHeight);
    }

    builderContent.setSize(juce::jmax(500, viewport.getWidth() - 16), 580);
    auto builderBounds = builderContent.getLocalBounds().reduced(12);
    builderLockLabel.setBounds(builderBounds.removeFromTop(30));
    auto chooseRow = builderBounds.removeFromTop(32);
    chooseEffectButton.setBounds(chooseRow.removeFromLeft(250).reduced(2));
    builderAmountSlider.setBounds(chooseRow.removeFromLeft(240).reduced(2));
    addEffectButton.setBounds(chooseRow.removeFromLeft(145).reduced(2));
    chainList.setBounds(builderBounds.removeFromTop(212).reduced(2));
    auto orderRow = builderBounds.removeFromTop(29);
    moveUpButton.setBounds(orderRow.removeFromLeft(72).reduced(2));
    moveDownButton.setBounds(orderRow.removeFromLeft(82).reduced(2));
    removeEffectButton.setBounds(orderRow.removeFromLeft(100).reduced(2));
    auto saveRow = builderBounds.removeFromTop(32);
    presetNameEditor.setBounds(saveRow.removeFromLeft(275).reduced(2));
    savePresetButton.setBounds(saveRow.removeFromLeft(110).reduced(2));
    auto recallRow = builderBounds.removeFromTop(32);
    presetChoiceBox.setBounds(recallRow.removeFromLeft(275).reduced(2));
    loadPresetButton.setBounds(recallRow.removeFromLeft(75).reduced(2));
    deletePresetButton.setBounds(recallRow.removeFromLeft(82).reduced(2));
    builderStatusLabel.setBounds(builderBounds.removeFromTop(24).reduced(4, 1));

    loginOverlay.setBounds(getLocalBounds());
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
    if (isShowing() && (normalisedThemeId(currentThemePack) == "goonr"
                        || normalisedThemeId(currentThemePack) == "trippah"))
        startTimerHz(30);
    else
        startTimer(1000);
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
    }
    proc.setActiveEffectOrder({});
    updateActiveCount();
    countLabel.setText(juce::String(selected.size()) + " ACTIVE · "
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
        styleButton(*categoryButtons[i], category == selected ? juce::Colour(0xff102b17) : panelColour,
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
    g.drawText(juce::String(step.amount, 3), width - 77, 0, 64, height, juce::Justification::centredRight);
}

void DreamMasterLiteEditor::selectedRowsChanged(int selectedRow) {
    if (selectedRow >= 0 && selectedRow < static_cast<int>(builderSteps.size()))
        builderAmountSlider.setValue(builderSteps[static_cast<size_t>(selectedRow)].amount, juce::dontSendNotification);
}

void DreamMasterLiteEditor::showEffectBrowserMenu() {
    juce::PopupMenu menu;
    const auto favorites = proc.getFavoriteEffectIds();
    const auto query = searchEditor.getText().trim().toLowerCase();
    auto matches = [this, &query, &favorites](int index, int category) {
        if (category >= 0 && dm::effectCategoryIndex(index) != category)
            return false;
        if (selectedCategory >= 0 && dm::effectCategoryIndex(index) != selectedCategory)
            return false;
        if (showFavoritesOnly
            && std::find(favorites.begin(), favorites.end(), effectId(index)) == favorites.end())
            return false;
        return query.isEmpty() || effectName(index).toLowerCase().contains(query)
            || effectId(index).toLowerCase().contains(query);
    };
    auto addEffects = [&favorites, &matches](juce::PopupMenu& target, int category) {
        std::vector<int> indices;
        for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
            if ((category < 0 || dm::effectCategoryIndex(i) == category)
                && matches(i, category))
                indices.push_back(i);
        }
        indices = dm::sortEffectsWithFavoritesFirst(std::move(indices), favorites);
        for (const int index : indices) {
            const bool isFavorite = std::find(favorites.begin(), favorites.end(), effectId(index)) != favorites.end();
            target.addItem(index + 1, (isFavorite ? "★  " : "") + effectName(index));
        }
    };
    juce::PopupMenu favoriteMenu;
    for (const auto& id : favorites) {
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), id.toStdString());
        if (found == dm::featureIds.end())
            continue;
        const int index = static_cast<int>(std::distance(dm::featureIds.begin(), found));
        if (matches(index, allCategory))
            favoriteMenu.addItem(index + 1, effectName(index));
    }
    if (favoriteMenu.getNumItems() > 0)
        menu.addSubMenu("★ FAVORITES", favoriteMenu);
    for (int category = 0; category < static_cast<int>(dm::effectCategories.size()); ++category) {
        juce::PopupMenu categoryMenu;
        addEffects(categoryMenu, category);
        if (categoryMenu.getNumItems() > 0)
            menu.addSubMenu(juce::String(dm::effectCategories[static_cast<size_t>(category)].data()), categoryMenu);
    }
    if (menu.getNumItems() == 0)
        return;
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&chooseEffectButton),
        [safeThis](int result) {
            if (safeThis == nullptr || result <= 0)
                return;
            safeThis->selectedBuilderEffect = result - 1;
            safeThis->chooseEffectButton.setButtonText(effectName(safeThis->selectedBuilderEffect));
            if (auto* parameter = safeThis->proc.state.getRawParameterValue(
                    "amt" + juce::String(safeThis->selectedBuilderEffect)))
                safeThis->builderAmountSlider.setValue(parameter->load(std::memory_order_relaxed),
                                                       juce::dontSendNotification);
        });
}

void DreamMasterLiteEditor::addBuilderEffect() {
    const int index = selectedBuilderEffect;
    if (index < 0 || index >= dm::DspEngine::effectCount)
        return;
    if (std::any_of(builderSteps.begin(), builderSteps.end(),
                    [index](const auto& step) { return step.effectIndex == index; })) {
        builderStatusLabel.setText("Each module can appear only once in a chain.", juce::dontSendNotification);
        return;
    }
    builderSteps.push_back({index, static_cast<float>(builderAmountSlider.getValue())});
    updateBuilderChainList();
    chainList.selectRow(static_cast<int>(builderSteps.size()) - 1);
    builderStatusLabel.setText("Chain order is the DSP processing order.", juce::dontSendNotification);
}

void DreamMasterLiteEditor::saveBuilderPreset() {
    if (!authenticated || builderSteps.empty()) {
        builderStatusLabel.setText(builderSteps.empty() ? "Add at least one module before saving." : "Log in to save presets.",
                                   juce::dontSendNotification);
        return;
    }
    std::vector<dm::PresetStep> steps;
    steps.reserve(builderSteps.size());
    for (const auto& step : builderSteps)
        steps.push_back({effectId(step.effectIndex), step.amount});
    const auto id = proc.saveCustomPreset(presetNameEditor.getText(), steps);
    if (id.isEmpty()) {
        builderStatusLabel.setText("Preset could not be saved; enter a name and check the chain.", juce::dontSendNotification);
        return;
    }
    updatePresetList();
    for (int i = 0; i < static_cast<int>(availablePresets.size()); ++i) {
        if (availablePresets[static_cast<size_t>(i)].id == id) {
            presetChoiceBox.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    }
    loadSelectedPreset();
    builderStatusLabel.setText("Saved and loaded in this plugin instance's state.", juce::dontSendNotification);
}

void DreamMasterLiteEditor::loadSelectedPreset() {
    const int selected = presetChoiceBox.getSelectedId() - 1;
    if (!authenticated || selected < 0 || selected >= static_cast<int>(availablePresets.size()))
        return;
    const auto preset = availablePresets[static_cast<size_t>(selected)];
    if (!proc.loadCustomPreset(preset.id)) {
        builderStatusLabel.setText("Preset data is invalid and was not loaded.", juce::dontSendNotification);
        return;
    }
    builderSteps.clear();
    for (const auto& step : preset.steps) {
        const auto found = std::find(dm::featureIds.begin(), dm::featureIds.end(), step.effectId.toStdString());
        if (found != dm::featureIds.end())
            builderSteps.push_back({static_cast<int>(std::distance(dm::featureIds.begin(), found)), step.amount});
    }
    updateBuilderChainList();
    chainList.deselectAllRows();
    builderStatusLabel.setText("Loaded chain; modules run in the saved order.", juce::dontSendNotification);
    updateActiveCount();
}

void DreamMasterLiteEditor::deleteSelectedPreset() {
    const int selected = presetChoiceBox.getSelectedId() - 1;
    if (!authenticated || selected < 0 || selected >= static_cast<int>(availablePresets.size()))
        return;
    if (!proc.deleteCustomPreset(availablePresets[static_cast<size_t>(selected)].id)) {
        builderStatusLabel.setText("Preset could not be deleted.", juce::dontSendNotification);
        return;
    }
    updatePresetList();
    builderStatusLabel.setText("Deleted saved preset from this plugin state.", juce::dontSendNotification);
}

void DreamMasterLiteEditor::moveSelectedEffect(int direction) {
    const int selected = chainList.getSelectedRow();
    const int destination = selected + direction;
    if (selected < 0 || destination < 0 || destination >= static_cast<int>(builderSteps.size()))
        return;
    std::swap(builderSteps[static_cast<size_t>(selected)], builderSteps[static_cast<size_t>(destination)]);
    updateBuilderChainList();
    chainList.selectRow(destination);
}

void DreamMasterLiteEditor::updateBuilderAmount() {
    const int selected = chainList.getSelectedRow();
    if (selected < 0 || selected >= static_cast<int>(builderSteps.size()))
        return;
    builderSteps[static_cast<size_t>(selected)].amount = static_cast<float>(builderAmountSlider.getValue());
    chainList.repaintRow(selected);
}

void DreamMasterLiteEditor::updateBuilderChainList() {
    chainList.updateContent();
    chainList.repaint();
}

void DreamMasterLiteEditor::updatePresetList() {
    availablePresets = proc.getCustomPresets();
    presetChoiceBox.clear(juce::dontSendNotification);
    for (int i = 0; i < static_cast<int>(availablePresets.size()); ++i)
        presetChoiceBox.addItem(availablePresets[static_cast<size_t>(i)].name, i + 1);
}

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
    themeButton.setEnabled(authenticated && !themeIds.isEmpty());
    builderLockLabel.setVisible(!builderAvailable);
    builderLockLabel.setText(!authenticated ? "FX BUILDER LOCKED · LOG IN TO DREAMSHARE"
        : (builderAvailable ? "FX BUILDER · CHAIN ORDER IS DSP ORDER" : "FX BUILDER ACCESS UNAVAILABLE"),
        juce::dontSendNotification);
    const std::array<juce::Component*, 12> builderControls{{
        &chooseEffectButton, &builderAmountSlider, &addEffectButton, &chainList, &moveUpButton, &moveDownButton,
        &removeEffectButton, &presetNameEditor, &presetChoiceBox, &savePresetButton, &loadPresetButton,
        &deletePresetButton
    }};
    for (auto* control : builderControls)
        control->setEnabled(builderAvailable);
    if (authenticated) {
        accountLabel.setText("SIGNED IN · " + proc.getDreamShareUser(), juce::dontSendNotification);
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
    loginOverlay.setVisible(true);
    loginOverlay.toFront(true);
    usernameEditor.grabKeyboardFocus();
    resized();
    repaint();
}

void DreamMasterLiteEditor::dismissLoginPopup() {
    loginOverlay.setVisible(false);
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
    loginStatusLabel.setText("CONNECTING SECURELY…", juce::dontSendNotification);
    loginSubmitButton.setEnabled(false);
    loginSubmitButton.setButtonText("SIGNING IN…");
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
        safeThis->loginOverlay.setVisible(false);
        safeThis->authenticated = true;
        safeThis->populateThemes(response->getProperty("themes"));
        safeThis->applyThemePack(response->getProperty("themePack"));
        safeThis->updateAuthenticationUi();
        safeThis->refreshCapabilities();
        safeThis->sendPresenceHeartbeat();
        safeThis->accountLabel.setText("SIGNED IN · " + safeThis->proc.getDreamShareUser()
            + (savedSecurely ? " · SESSION SAVED" : " · SESSION MEMORY-ONLY"), juce::dontSendNotification);
    });
}

void DreamMasterLiteEditor::logout() {
    proc.clearDreamShareSession();
    authenticated = false;
    builderAvailable = false;
    updateLoginPreference(true);
    updateAuthenticationUi();
    accountLabel.setText("SIGNED OUT · LOG IN WHEN NEEDED", juce::dontSendNotification);
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
            safeThis->accountLabel.setText("SESSION CHECK FAILED · " + reply.error, juce::dontSendNotification);
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
                safeThis->themeButton.setEnabled(safeThis->authenticated && !safeThis->themeIds.isEmpty());
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
    themeButton.setEnabled(authenticated && !themeIds.isEmpty());
}

void DreamMasterLiteEditor::updateThemeButtonLabel() {
    const int index = themeIds.indexOf(proc.getDreamShareTheme());
    themeButton.setButtonText(index >= 0 ? "THEME · " + themeNames[index] : "THEMES");
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
    if (isShowing() && (normalisedThemeId(currentThemePack) == "goonr"
                        || normalisedThemeId(currentThemePack) == "trippah"))
        startTimerHz(30);
    else
        startTimer(1000);
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
                         &themeButton, &chooseEffectButton, &moveUpButton, &moveDownButton, &savePresetButton,
                         &loadPresetButton, &deletePresetButton})
        styleButton(*button, panelColour, accentColour, highlightColour);
    styleButton(randomButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(extremeButton, juce::Colour(0xff241018), juce::Colour(0xffff7272), juce::Colour(0xffffb047));
    styleButton(resetButton, juce::Colour(0xff20130e), textColour, highlightColour);
    styleButton(addEffectButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(removeEffectButton, juce::Colour(0xff291410), juce::Colour(0xffff9b83), juce::Colour(0xffffc0a8));
    styleButton(loginSubmitButton, juce::Colour(0xff102b17), accentColour, highlightColour);
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
    builderAmountSlider.setColour(juce::Slider::trackColourId, accentColour);
    builderAmountSlider.setColour(juce::Slider::thumbColourId, highlightColour);
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
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&themeButton), [safeThis](int result) {
        if (safeThis == nullptr || result <= 0 || result > safeThis->themeIds.size())
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
    accountLabel.setText("CHANGING THEME…", juce::dontSendNotification);
    juce::Component::SafePointer<DreamMasterLiteEditor> safeThis(this);
    requestWorkerAsync(&body, false, [safeThis, previousTheme](WorkerReply reply) {
        if (safeThis == nullptr)
            return;
        if (reply.error.isNotEmpty() || !responseIsOk(reply.payload)) {
            safeThis->accountLabel.setText(reply.error.isNotEmpty() ? reply.error : "THEME CHANGE FAILED",
                                           juce::dontSendNotification);
            const int previous = safeThis->themeIds.indexOf(previousTheme);
            if (previous >= 0)
                safeThis->themeButton.setButtonText("THEME · " + safeThis->themeNames[previous]);
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
    animationFrame = now / 33u;
    const auto theme = normalisedThemeId(currentThemePack);
    if (theme == "goonr" || theme == "trippah")
        repaint();
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
