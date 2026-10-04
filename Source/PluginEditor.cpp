#include "PluginEditor.h"
#include "Randomizer.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
constexpr auto workerUrl = "https://dreamshare-api.keganacummings.workers.dev/";

void styleButton(juce::TextButton& button, juce::Colour fill, juce::Colour ink, juce::Colour highlight) {
    button.setColour(juce::TextButton::buttonColourId, fill);
    button.setColour(juce::TextButton::buttonOnColourId, highlight.withAlpha(0.22f));
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
    if (hex.length() < 6)
        return fallback;
    return juce::Colour::fromString("ff" + hex.substring(0, 6));
}
}

DreamMasterLiteEditor::DreamMasterLiteEditor(DreamMasterLiteProcessor& processor)
    : AudioProcessorEditor(processor), proc(processor) {
    title.setText("DREAMMASTERLITE", juce::dontSendNotification);
    title.setFont(juce::Font(27.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title);

    subtitle.setText("INSOMNIA FX RACK  /  200 DISTINCT NAMED MODULES", juce::dontSendNotification);
    subtitle.setFont(juce::Font(10.5f));
    subtitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(subtitle);

    countLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(countLabel);
    onlineLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    onlineLabel.setText("ONLINE: —", juce::dontSendNotification);
    addAndMakeVisible(onlineLabel);
    accountLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(accountLabel);

    usernameEditor.setTextToShowWhenEmpty("DreamShare username", juce::Colours::grey);
    passwordEditor.setTextToShowWhenEmpty("Password", juce::Colours::grey);
    passwordEditor.setPasswordCharacter(0x2022);
    for (auto* editor : {&usernameEditor, &passwordEditor}) {
        editor->setColour(juce::TextEditor::backgroundColourId, panelColour);
        editor->setColour(juce::TextEditor::textColourId, textColour);
        editor->setColour(juce::TextEditor::outlineColourId, borderColour);
        addAndMakeVisible(*editor);
    }
    addAndMakeVisible(loginButton);
    addAndMakeVisible(logoutButton);
    loginButton.onClick = [this] { login(); };
    logoutButton.onClick = [this] { logout(); };
    passwordEditor.onReturnKey = [this] { login(); };

    styleButton(randomButton, panelColour, accentColour, highlightColour);
    styleButton(extremeButton, juce::Colour(0xff241018), juce::Colour(0xffff7272), juce::Colour(0xffffb047));
    styleButton(definedButton, panelColour, highlightColour, accentColour);
    styleButton(resetButton, juce::Colour(0xff20130e), textColour, highlightColour);
    styleButton(websiteButton, panelColour, highlightColour, accentColour);
    styleButton(rackPageButton, juce::Colour(0xff102b17), accentColour, highlightColour);
    styleButton(builderPageButton, panelColour, accentColour, highlightColour);
    for (auto* button : {&randomButton, &extremeButton, &definedButton, &resetButton, &websiteButton,
                         &rackPageButton, &builderPageButton})
        addAndMakeVisible(*button);
    randomButton.onClick = [this] { randomize(1, 10, false); };
    extremeButton.onClick = [this] { randomize(10, 20, false); };
    definedButton.onClick = [this] { randomize(1, 10, true); };
    resetButton.onClick = [this] { resetAll(); };
    websiteButton.onClick = [] { juce::URL("https://dreamdaw.com").launchInDefaultBrowser(); };
    rackPageButton.onClick = [this] { showPage(false); };
    builderPageButton.onClick = [this] { showPage(true); };

    categoryBox.addItem("All categories", 1);
    for (int i = 0; i < static_cast<int>(dm::effectCategories.size()); ++i)
        categoryBox.addItem(juce::String(dm::effectCategories[static_cast<size_t>(i)].data()), i + 2);
    categoryBox.setSelectedId(1, juce::dontSendNotification);
    categoryBox.onChange = [this] { updateVisibleEffects(); };
    addAndMakeVisible(categoryBox);

    themeBox.addItem("DreamShare themes", 1);
    themeBox.setSelectedId(1, juce::dontSendNotification);
    themeBox.onChange = [this] { selectTheme(); };
    addAndMakeVisible(themeBox);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, true);
    viewport.setColour(juce::ScrollBar::thumbColourId, accentColour.withAlpha(0.8f));
    viewport.setColour(juce::ScrollBar::trackColourId, backgroundColour);
    addAndMakeVisible(viewport);

    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto name = juce::String(dm::featureNames[static_cast<size_t>(i)].data());
        auto* label = effectLabels.add(new juce::Label({}, name));
        label->setColour(juce::Label::textColourId, textColour);
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centredLeft);
        content.addAndMakeVisible(label);

        auto* button = effectButtons.add(new juce::ToggleButton("ON"));
        button->setClickingTogglesState(true);
        button->setColour(juce::ToggleButton::textColourId, accentColour);
        button->setColour(juce::ToggleButton::tickColourId, highlightColour);
        content.addAndMakeVisible(button);
        buttonAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(
            proc.state, "fx" + juce::String(i), *button));

        auto* slider = amountSliders.add(new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
        slider->setRange(0.0, 1.0, 0.001);
        slider->setColour(juce::Slider::trackColourId, accentColour);
        slider->setColour(juce::Slider::thumbColourId, highlightColour);
        slider->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff484848));
        content.addAndMakeVisible(slider);
        sliderAttachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(
            proc.state, "amt" + juce::String(i), *slider));
    }

    builderLockLabel.setText("FX BUILDER LOCKED · LOG IN TO DREAMSHARE", juce::dontSendNotification);
    builderLockLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    builderLockLabel.setJustificationType(juce::Justification::centred);
    builderContent.addAndMakeVisible(builderLockLabel);
    auto addBuilderControl = [this](juce::Component& component) {
        builderContent.addAndMakeVisible(component);
        component.setEnabled(false);
    };

    for (int i = 0; i < dm::DspEngine::effectCount; ++i)
        effectChoiceBox.addItem(juce::String(dm::featureNames[static_cast<size_t>(i)].data()), i + 1);
    effectChoiceBox.setSelectedId(1, juce::dontSendNotification);
    builderAmountSlider.setRange(0.0, 1.0, 0.001);
    builderAmountSlider.setValue(0.22, juce::dontSendNotification);
    builderAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    builderAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 24);
    builderAmountSlider.onValueChange = [this] { updateBuilderAmount(); };
    effectChoiceBox.onChange = [this] {
        if (auto* parameter = proc.state.getRawParameterValue("amt" + juce::String(effectChoiceBox.getSelectedId() - 1)))
            builderAmountSlider.setValue(parameter->load(std::memory_order_relaxed), juce::dontSendNotification);
    };
    addBuilderControl(effectChoiceBox);
    addBuilderControl(builderAmountSlider);
    addBuilderControl(addEffectButton);
    addEffectButton.onClick = [this] { addBuilderEffect(); };

    chainList.setRowHeight(30);
    chainList.setMultipleSelectionEnabled(false);
    chainList.setColour(juce::ListBox::backgroundColourId, panelColour);
    chainList.setColour(juce::ListBox::outlineColourId, borderColour);
    addBuilderControl(chainList);
    for (auto* button : {&moveUpButton, &moveDownButton, &removeEffectButton, &savePresetButton,
                         &loadPresetButton, &deletePresetButton}) {
        addBuilderControl(*button);
        styleButton(*button, panelColour, accentColour, highlightColour);
    }
    moveUpButton.onClick = [this] { moveSelectedEffect(-1); };
    moveDownButton.onClick = [this] { moveSelectedEffect(1); };
    removeEffectButton.onClick = [this] {
        const int selected = chainList.getSelectedRow();
        if (selected >= 0 && selected < static_cast<int>(builderSteps.size())) {
            builderSteps.erase(builderSteps.begin() + selected);
            chainList.updateContent();
            chainList.selectRow(juce::jmin(selected, static_cast<int>(builderSteps.size()) - 1));
        }
    };
    presetNameEditor.setTextToShowWhenEmpty("Name this FX chain", juce::Colours::grey);
    presetNameEditor.setColour(juce::TextEditor::backgroundColourId, panelColour);
    presetNameEditor.setColour(juce::TextEditor::textColourId, textColour);
    presetNameEditor.setColour(juce::TextEditor::outlineColourId, borderColour);
    addBuilderControl(presetNameEditor);
    addBuilderControl(presetChoiceBox);
    savePresetButton.onClick = [this] { saveBuilderPreset(); };
    loadPresetButton.onClick = [this] { loadSelectedPreset(); };
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };
    builderStatusLabel.setFont(juce::Font(10.5f));
    builderContent.addAndMakeVisible(builderStatusLabel);

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
    juce::ColourGradient glow(highlightColour.withAlpha(0.06f), bounds.getWidth() * 0.18f, 0.0f,
                              juce::Colours::transparentBlack, bounds.getWidth() * 0.62f, bounds.getHeight(), false);
    g.setGradientFill(glow);
    g.fillRect(bounds);
    for (int x = 12; x < getWidth(); x += 31) {
        for (int y = 7 + ((x * 13) % 43); y < getHeight(); y += 23 + ((x / 31) % 4) * 7) {
            const int glyph = (x * 7 + y * 3) % 4;
            g.setColour(accentColour.withAlpha(glyph == 0 ? 0.14f : 0.045f));
            g.setFont(juce::Font(10.0f));
            g.drawText(glyph == 0 ? "1" : (glyph == 1 ? "+" : (glyph == 2 ? ":" : "0")),
                       x, y, 9, 11, juce::Justification::centred);
        }
    }
    g.setColour(juce::Colours::black.withAlpha(0.22f));
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
    g.setColour(highlightColour.withAlpha(0.45f));
    g.drawRect(getLocalBounds().reduced(5), 1);
}

void DreamMasterLiteEditor::resized() {
    auto r = getLocalBounds().reduced(10);
    title.setBounds(r.removeFromTop(33));
    subtitle.setBounds(r.removeFromTop(17));

    auto authRow = r.removeFromTop(28);
    usernameEditor.setBounds(authRow.removeFromLeft(145).reduced(2));
    passwordEditor.setBounds(authRow.removeFromLeft(125).reduced(2));
    loginButton.setBounds(authRow.removeFromLeft(72).reduced(2));
    logoutButton.setBounds(authRow.removeFromLeft(70).reduced(2));
    onlineLabel.setBounds(authRow.removeFromRight(112).reduced(2));
    accountLabel.setBounds(authRow.reduced(4, 2));

    auto actionRow = r.removeFromTop(30);
    randomButton.setBounds(actionRow.removeFromLeft(137).reduced(2));
    extremeButton.setBounds(actionRow.removeFromLeft(97).reduced(2));
    definedButton.setBounds(actionRow.removeFromLeft(130).reduced(2));
    resetButton.setBounds(actionRow.removeFromLeft(75).reduced(2));
    categoryBox.setBounds(actionRow.removeFromRight(165).reduced(2));
    countLabel.setBounds(actionRow.reduced(4, 2));

    auto navigationRow = r.removeFromTop(29);
    rackPageButton.setBounds(navigationRow.removeFromLeft(95).reduced(2));
    builderPageButton.setBounds(navigationRow.removeFromLeft(110).reduced(2));
    websiteButton.setBounds(navigationRow.removeFromLeft(145).reduced(2));
    themeBox.setBounds(navigationRow.removeFromRight(190).reduced(2));
    viewport.setBounds(r);

    const int columns = getWidth() >= 1050 ? 4 : 3, gap = 8, rowHeight = 66;
    const int cardWidth = juce::jmax(150, (viewport.getWidth() - 25 - gap * (columns - 1)) / columns);
    const int rows = (static_cast<int>(visibleEffects.size()) + columns - 1) / columns;
    content.setSize(columns * cardWidth + (columns - 1) * gap, rows * (rowHeight + gap));
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        const auto found = std::find(visibleEffects.begin(), visibleEffects.end(), i);
        if (found == visibleEffects.end()) {
            effectLabels[i]->setVisible(false);
            effectButtons[i]->setVisible(false);
            amountSliders[i]->setVisible(false);
            continue;
        }
        const int position = static_cast<int>(std::distance(visibleEffects.begin(), found));
        const int col = position % columns, row = position / columns;
        const int x = col * (cardWidth + gap), y = row * (rowHeight + gap);
        effectLabels[i]->setVisible(true);
        effectButtons[i]->setVisible(true);
        amountSliders[i]->setVisible(true);
        effectLabels[i]->setBounds(x + 7, y + 4, cardWidth - 57, 20);
        effectButtons[i]->setBounds(x + cardWidth - 48, y + 2, 44, 22);
        amountSliders[i]->setBounds(x + 8, y + 30, cardWidth - 16, 25);
    }

    builderContent.setSize(viewport.getWidth() - 25, 610);
    auto b = builderContent.getLocalBounds().reduced(12);
    builderLockLabel.setBounds(b.removeFromTop(44));
    auto chooseRow = b.removeFromTop(34);
    effectChoiceBox.setBounds(chooseRow.removeFromLeft(285).reduced(2));
    builderAmountSlider.setBounds(chooseRow.removeFromLeft(245).reduced(2));
    addEffectButton.setBounds(chooseRow.removeFromLeft(125).reduced(2));
    chainList.setBounds(b.removeFromTop(235).reduced(2));
    auto orderingRow = b.removeFromTop(31);
    moveUpButton.setBounds(orderingRow.removeFromLeft(95).reduced(2));
    moveDownButton.setBounds(orderingRow.removeFromLeft(105).reduced(2));
    removeEffectButton.setBounds(orderingRow.removeFromLeft(105).reduced(2));
    auto saveRow = b.removeFromTop(34);
    presetNameEditor.setBounds(saveRow.removeFromLeft(300).reduced(2));
    savePresetButton.setBounds(saveRow.removeFromLeft(125).reduced(2));
    auto recallRow = b.removeFromTop(34);
    presetChoiceBox.setBounds(recallRow.removeFromLeft(300).reduced(2));
    loadPresetButton.setBounds(recallRow.removeFromLeft(110).reduced(2));
    deletePresetButton.setBounds(recallRow.removeFromLeft(120).reduced(2));
    builderStatusLabel.setBounds(b.removeFromTop(25).reduced(4, 2));
}

void DreamMasterLiteEditor::randomize(int minimum, int maximum, bool definedValues) {
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
        const float amount = definedValues
            ? dm::definedRandomAmount(index)
            : 0.1f + rng.nextFloat() * 0.8f;
        if (auto* parameter = proc.state.getParameter("amt" + juce::String(index)))
            parameter->setValueNotifyingHost(amount);
    }
    proc.setActiveEffectOrder({});
    countLabel.setText(juce::String(selected.size()) + " ACTIVE · " + (definedValues ? "DEFINED VALUES" : "RANDOM AMOUNTS"),
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

void DreamMasterLiteEditor::updateVisibleEffects() {
    visibleEffects.clear();
    const int selectedCategory = categoryBox.getSelectedId() - 2;
    for (int i = 0; i < dm::DspEngine::effectCount; ++i) {
        if (categoryBox.getSelectedId() == 1 || dm::effectCategoryIndex(i) == selectedCategory)
            visibleEffects.push_back(i);
    }
    resized();
    content.repaint();
}

void DreamMasterLiteEditor::showPage(bool builder) {
    builderPageActive = builder;
    viewport.setViewedComponent(builder ? &builderContent : &content, false);
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
    g.fillAll(rowIsSelected ? accentColour.withAlpha(0.18f) : panelColour);
    const auto& step = builderSteps[static_cast<size_t>(row)];
    const auto name = juce::String(dm::featureNames[static_cast<size_t>(step.effectIndex)].data());
    g.setColour(textColour);
    g.setFont(juce::Font(12.0f));
    g.drawText(juce::String(row + 1) + ". " + name, 9, 0, width - 100, height, juce::Justification::centredLeft);
    g.setColour(highlightColour);
    g.drawText(juce::String(step.amount, 3), width - 78, 0, 65, height, juce::Justification::centredRight);
}

void DreamMasterLiteEditor::selectedRowsChanged(int selectedRow) {
    if (selectedRow >= 0 && selectedRow < static_cast<int>(builderSteps.size()))
        builderAmountSlider.setValue(builderSteps[static_cast<size_t>(selectedRow)].amount, juce::dontSendNotification);
}

void DreamMasterLiteEditor::addBuilderEffect() {
    const int index = effectChoiceBox.getSelectedId() - 1;
    if (index < 0 || index >= dm::DspEngine::effectCount)
        return;
    if (std::any_of(builderSteps.begin(), builderSteps.end(),
                    [index](const auto& step) { return step.effectIndex == index; })) {
        builderStatusLabel.setText("Each module can appear only once in a chain.", juce::dontSendNotification);
        return;
    }
    builderSteps.push_back({index, static_cast<float>(builderAmountSlider.getValue())});
    chainList.updateContent();
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
        steps.push_back({juce::String(dm::featureIds[static_cast<size_t>(step.effectIndex)].data()), step.amount});
    const auto id = proc.saveCustomPreset(presetNameEditor.getText(), steps);
    if (id.isEmpty()) {
        builderStatusLabel.setText("Preset could not be saved; enter a name and check the chain.", juce::dontSendNotification);
        return;
    }
    updatePresetList();
    for (int i = 0; i < static_cast<int>(availablePresets.size()); ++i) {
        if (availablePresets[static_cast<size_t>(i)].id == id) {
            presetChoiceBox.setSelectedId(i + 1, juce::sendNotificationSync);
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
    chainList.updateContent();
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
    chainList.updateContent();
    chainList.selectRow(destination);
}

void DreamMasterLiteEditor::updateBuilderAmount() {
    const int selected = chainList.getSelectedRow();
    if (selected < 0 || selected >= static_cast<int>(builderSteps.size()))
        return;
    builderSteps[static_cast<size_t>(selected)].amount = static_cast<float>(builderAmountSlider.getValue());
    chainList.repaintRow(selected);
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
    loginButton.setEnabled(!authenticated);
    logoutButton.setEnabled(authenticated);
    usernameEditor.setEnabled(!authenticated);
    passwordEditor.setEnabled(!authenticated);
    themeBox.setEnabled(authenticated && themeIds.size() > 0);
    builderLockLabel.setVisible(!builderAvailable);
    builderLockLabel.setText(!authenticated ? "FX BUILDER LOCKED · LOG IN TO DREAMSHARE"
        : (builderAvailable ? "FX BUILDER · CHAIN ORDER IS DSP ORDER" : "FX BUILDER ACCESS UNAVAILABLE"),
        juce::dontSendNotification);
    const std::array<juce::Component*, 12> builderControls{{
        &effectChoiceBox, &builderAmountSlider, &addEffectButton, &chainList, &moveUpButton, &moveDownButton,
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
    }
}

DreamMasterLiteEditor::WorkerReply DreamMasterLiteEditor::requestWorker(const juce::var* request, bool get) {
    WorkerReply reply;
    juce::URL url(workerUrl);
    const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(3000)
        .withNumRedirectsToFollow(0)
        .withStatusCode(&reply.statusCode);
    std::unique_ptr<juce::InputStream> stream;
    if (get) {
        stream = url.createInputStream(options.withHttpRequestCmd("GET"));
    } else {
        if (request == nullptr) {
            reply.error = "Request data is missing.";
            return reply;
        }
        url = url.withPOSTData(juce::JSON::toString(*request));
        stream = url.createInputStream(options.withHttpRequestCmd("POST")
            .withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n"));
    }

    if (stream == nullptr) {
        reply.error = "DreamShare Worker is unavailable.";
        return reply;
    }
    const auto body = stream->readEntireStreamAsString();
    if (reply.statusCode < 200 || reply.statusCode >= 300) {
        reply.error = "DreamShare Worker returned HTTP " + juce::String(reply.statusCode) + ".";
        return reply;
    }
    reply.payload = juce::JSON::parse(body);
    if (!reply.payload.isObject()) {
        reply.error = "DreamShare Worker returned an invalid JSON response.";
        return reply;
    }
    return reply;
}

void DreamMasterLiteEditor::login() {
    const auto username = usernameEditor.getText().trim();
    const auto password = passwordEditor.getText();
    if (username.isEmpty() || password.isEmpty()) {
        accountLabel.setText("ENTER USERNAME AND PASSWORD", juce::dontSendNotification);
        return;
    }
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "login");
    request->setProperty("user", username);
    request->setProperty("pass", password);
    const juce::var body(request.get());
    const auto reply = requestWorker(&body);
    passwordEditor.clear();
    if (reply.error.isNotEmpty()) {
        accountLabel.setText(reply.error, juce::dontSendNotification);
        return;
    }
    auto* response = reply.payload.getDynamicObject();
    const auto token = response->getProperty("token").toString();
    const auto user = response->getProperty("user").toString();
    const auto theme = response->getProperty("theme").toString();
    if (!responseIsOk(reply.payload) || token.isEmpty() || user.isEmpty()) {
        accountLabel.setText("DREAMSHARE LOGIN FAILED", juce::dontSendNotification);
        return;
    }
    proc.setDreamShareSession(token, user, theme);
    authenticated = true;
    populateThemes(response->getProperty("themes"));
    applyThemePack(response->getProperty("themePack"));
    updateAuthenticationUi();
    refreshCapabilities();
    sendPresenceHeartbeat();
    accountLabel.setText("SIGNED IN · " + proc.getDreamShareUser(), juce::dontSendNotification);
}

void DreamMasterLiteEditor::logout() {
    proc.clearDreamShareSession();
    authenticated = false;
    builderAvailable = false;
    updateAuthenticationUi();
    accountLabel.setText("SIGNED OUT", juce::dontSendNotification);
}

void DreamMasterLiteEditor::validateSession() {
    if (!proc.hasDreamShareSession())
        return;
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "session");
    request->setProperty("token", proc.getDreamShareToken());
    const juce::var body(request.get());
    const auto reply = requestWorker(&body);
    if (reply.error.isNotEmpty()) {
        accountLabel.setText("SESSION CHECK FAILED · " + reply.error, juce::dontSendNotification);
        return;
    }
    auto* response = reply.payload.getDynamicObject();
    if (!responseIsOk(reply.payload)) {
        proc.clearDreamShareSession();
        updateAuthenticationUi();
        accountLabel.setText("DREAMSHARE SESSION EXPIRED", juce::dontSendNotification);
        return;
    }
    const auto theme = response->getProperty("theme").toString();
    if (theme.isNotEmpty())
        proc.setDreamShareTheme(theme);
    populateThemes(response->getProperty("themes"));
    applyThemePack(response->getProperty("themePack"));
    authenticated = true;
    updateAuthenticationUi();
    refreshCapabilities();
    sendPresenceHeartbeat();
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
    const auto reply = requestWorker(&body);
    if (reply.error.isNotEmpty() || !responseIsOk(reply.payload)) {
        builderAvailable = false;
        updateAuthenticationUi();
        builderStatusLabel.setText("FX Builder capability is unavailable; DreamShare login and themes remain active.",
                                   juce::dontSendNotification);
        return;
    }
    auto* response = reply.payload.getDynamicObject();
    auto* capabilities = response->getProperty("capabilities").getDynamicObject();
    builderAvailable = capabilities != nullptr && valueIsTrue(capabilities->getProperty("fxBuilder"));
    if (capabilities != nullptr) {
        const auto allowedThemes = capabilities->getProperty("themes");
        if (const auto* ids = allowedThemes.getArray()) {
            juce::StringArray permitted;
            for (const auto& item : *ids)
                permitted.add(item.toString());
            themeBox.clear(juce::dontSendNotification);
            for (int i = themeIds.size() - 1; i >= 0; --i) {
                if (!permitted.contains(themeIds[i])) {
                    themeIds.remove(i);
                    themeNames.remove(i);
                }
            }
            for (int i = 0; i < themeIds.size(); ++i)
                themeBox.addItem(themeNames[i], i + 1);
            const int currentTheme = themeIds.indexOf(proc.getDreamShareTheme());
            if (currentTheme >= 0)
                themeBox.setSelectedId(currentTheme + 1, juce::dontSendNotification);
        }
    }
    updateAuthenticationUi();
    if (!builderAvailable)
        builderStatusLabel.setText("FX Builder is not enabled for this account.", juce::dontSendNotification);
}

void DreamMasterLiteEditor::refreshOnlineCount() {
    const auto reply = requestWorker(nullptr, true);
    if (reply.error.isNotEmpty()) {
        onlineLabel.setText("ONLINE: UNAVAILABLE", juce::dontSendNotification);
        return;
    }
    auto* response = reply.payload.getDynamicObject();
    const auto online = response->getProperty("online");
    if (const auto* users = online.getArray()) {
        onlineLabel.setText("ONLINE: " + juce::String(users->size()), juce::dontSendNotification);
    } else {
        onlineLabel.setText("ONLINE: UNAVAILABLE", juce::dontSendNotification);
    }
}

void DreamMasterLiteEditor::sendPresenceHeartbeat() {
    if (!proc.hasDreamShareSession())
        return;
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "presence");
    request->setProperty("token", proc.getDreamShareToken());
    const juce::var body(request.get());
    const auto reply = requestWorker(&body);
    lastHeartbeatMs = juce::Time::getMillisecondCounter();
    if (reply.statusCode == 401 || (reply.error.isEmpty() && !responseIsOk(reply.payload))) {
        proc.clearDreamShareSession();
        updateAuthenticationUi();
        accountLabel.setText("DREAMSHARE SESSION EXPIRED", juce::dontSendNotification);
    }
}

void DreamMasterLiteEditor::populateThemes(const juce::var& themes) {
    const auto* summaries = themes.getArray();
    if (summaries == nullptr)
        return;
    themeBox.clear(juce::dontSendNotification);
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
        themeBox.addItem(name, themeIds.size());
    }
    const int themeIndex = themeIds.indexOf(proc.getDreamShareTheme());
    if (themeIndex >= 0)
        themeBox.setSelectedId(themeIndex + 1, juce::dontSendNotification);
    else if (!themeIds.isEmpty())
        themeBox.setSelectedId(1, juce::dontSendNotification);
    themeBox.setEnabled(authenticated && !themeIds.isEmpty());
}

void DreamMasterLiteEditor::applyThemePack(const juce::var& themePack) {
    auto* pack = themePack.getDynamicObject();
    if (pack == nullptr)
        return;
    currentThemePack = themePack;
    const auto themeId = pack->getProperty("id").toString();
    if (themeId.isNotEmpty())
        proc.setDreamShareTheme(themeId);

    const auto vars = pack->getProperty("vars");
    auto* variables = vars.getDynamicObject();
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
    title.setColour(juce::Label::textColourId, accentColour);
    subtitle.setColour(juce::Label::textColourId, mutedColour);
    countLabel.setColour(juce::Label::textColourId, highlightColour);
    accountLabel.setColour(juce::Label::textColourId, mutedColour);
    builderLockLabel.setColour(juce::Label::textColourId, accentColour);
    for (auto* label : effectLabels)
        label->setColour(juce::Label::textColourId, textColour);
    for (auto* button : effectButtons) {
        button->setColour(juce::ToggleButton::textColourId, accentColour);
        button->setColour(juce::ToggleButton::tickColourId, highlightColour);
    }
    for (auto* slider : amountSliders) {
        slider->setColour(juce::Slider::trackColourId, accentColour);
        slider->setColour(juce::Slider::thumbColourId, highlightColour);
    }
    for (auto* button : {&randomButton, &definedButton, &websiteButton, &rackPageButton, &builderPageButton,
                         &addEffectButton, &moveUpButton, &moveDownButton, &removeEffectButton, &savePresetButton,
                         &loadPresetButton, &deletePresetButton})
        styleButton(*button, panelColour, accentColour, highlightColour);
    styleButton(extremeButton, juce::Colour(0xff241018), juce::Colour(0xffff7272), juce::Colour(0xffffb047));
    styleButton(resetButton, juce::Colour(0xff20130e), textColour, highlightColour);
    for (auto* editor : {&usernameEditor, &passwordEditor, &presetNameEditor}) {
        editor->setColour(juce::TextEditor::backgroundColourId, panelColour);
        editor->setColour(juce::TextEditor::textColourId, textColour);
        editor->setColour(juce::TextEditor::outlineColourId, borderColour);
    }
    chainList.setColour(juce::ListBox::backgroundColourId, panelColour);
    chainList.setColour(juce::ListBox::outlineColourId, borderColour);
    viewport.setColour(juce::ScrollBar::thumbColourId, accentColour.withAlpha(0.8f));
    viewport.setColour(juce::ScrollBar::trackColourId, backgroundColour);
    repaint();
}

void DreamMasterLiteEditor::selectTheme() {
    if (!authenticated)
        return;
    const int selected = themeBox.getSelectedId() - 1;
    if (selected < 0 || selected >= themeIds.size())
        return;
    const auto theme = themeIds[selected];
    juce::DynamicObject::Ptr request = new juce::DynamicObject();
    request->setProperty("action", "set_theme");
    request->setProperty("token", proc.getDreamShareToken());
    request->setProperty("theme", theme);
    const juce::var body(request.get());
    const auto reply = requestWorker(&body);
    if (reply.error.isNotEmpty() || !responseIsOk(reply.payload)) {
        accountLabel.setText(reply.error.isNotEmpty() ? reply.error : "THEME CHANGE FAILED", juce::dontSendNotification);
        const int previous = themeIds.indexOf(proc.getDreamShareTheme());
        if (previous >= 0)
            themeBox.setSelectedId(previous + 1, juce::dontSendNotification);
        return;
    }
    auto* response = reply.payload.getDynamicObject();
    proc.setDreamShareTheme(response->getProperty("theme").toString());
    populateThemes(response->getProperty("themes"));
    applyThemePack(response->getProperty("themePack"));
    accountLabel.setText("THEME: " + proc.getDreamShareTheme(), juce::dontSendNotification);
}

void DreamMasterLiteEditor::timerCallback() {
    updateActiveCount();
    const auto now = juce::Time::getMillisecondCounter();
    if (now - lastOnlinePollMs >= 8000u) {
        refreshOnlineCount();
        lastOnlinePollMs = now;
    }
    if (authenticated && now - lastHeartbeatMs >= 20000u)
        sendPresenceHeartbeat();
}

juce::AudioProcessorEditor* DreamMasterLiteProcessor::createEditor() {
    return new DreamMasterLiteEditor(*this);
}
