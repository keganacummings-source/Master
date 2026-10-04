#include "PluginEditor.h"
#include <algorithm>
#include <vector>
#include <random>

namespace {
const juce::Colour bg(0xff030805), panel(0xff100a08), acid(0xffc8ff33), green(0xff65ff83), text(0xffd8ffe0), dim(0xffb5a18a), border(0xff563526);
void styleButton(juce::TextButton& button, juce::Colour fill = panel, juce::Colour ink = green) {
    button.setColour(juce::TextButton::buttonColourId, fill);
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff153c20));
    button.setColour(juce::TextButton::textColourOffId, ink);
    button.setColour(juce::TextButton::textColourOnId, acid);
}
}

DreamMasterLiteEditor::DreamMasterLiteEditor(DreamMasterLiteProcessor& processor)
    : AudioProcessorEditor(processor), proc(processor) {
    title.setText("DREAMMASTERLITE", juce::dontSendNotification);
    title.setFont(juce::Font(27.0f, juce::Font::bold)); title.setColour(juce::Label::textColourId, green);
    title.setJustificationType(juce::Justification::centred); addAndMakeVisible(title);
    subtitle.setText("INSOMNIA FX RACK  /  MASTER POLISH  /  200 DISTINCT NAMED MODULES", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, dim); subtitle.setFont(juce::Font(10.5f));
    subtitle.setJustificationType(juce::Justification::centred); addAndMakeVisible(subtitle);
    countLabel.setText("0 ACTIVE · RANDOM PICK MAX 10", juce::dontSendNotification);
    countLabel.setColour(juce::Label::textColourId, acid); countLabel.setFont(juce::Font(10.5f, juce::Font::bold)); addAndMakeVisible(countLabel);
    styleButton(randomButton, juce::Colour(0xff102b17), green); styleButton(resetButton, juce::Colour(0xff20130e), text); styleButton(websiteButton, juce::Colour(0xff102b17), acid);
    for (auto* b : { &randomButton, &resetButton, &websiteButton }) addAndMakeVisible(*b);
    randomButton.onClick = [this] { randomize(); }; resetButton.onClick = [this] { resetAll(); };
    websiteButton.onClick = [] { juce::URL("https://dreamdaw.com").launchInDefaultBrowser(); };
    viewport.setViewedComponent(&content, false); viewport.setScrollBarsShown(true, true);
    viewport.setColour(juce::ScrollBar::thumbColourId, green.withAlpha(0.8f)); viewport.setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff061009)); addAndMakeVisible(viewport);
    for (int i = 0; i < 200; ++i) {
        const auto name = juce::String(dm::featureNames[static_cast<size_t>(i)].data());
        auto* label = effectLabels.add(new juce::Label({}, name));
        label->setColour(juce::Label::textColourId, text); label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centredLeft); content.addAndMakeVisible(label);
        auto* button = effectButtons.add(new juce::ToggleButton("ON"));
        button->setClickingTogglesState(true); button->setColour(juce::ToggleButton::textColourId, green);
        button->setColour(juce::ToggleButton::tickColourId, acid); content.addAndMakeVisible(button);
        buttonAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(proc.state, "fx" + juce::String(i), *button));
        auto* slider = amountSliders.add(new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
        slider->setRange(0.0, 1.0, 0.001); slider->setValue(0.22, juce::dontSendNotification);
        slider->setColour(juce::Slider::trackColourId, green); slider->setColour(juce::Slider::thumbColourId, acid);
        slider->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff484848)); content.addAndMakeVisible(slider);
        sliderAttachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.state, "amt" + juce::String(i), *slider));
    }
    setResizable(true, true); setResizeLimits(650, 520, 1500, 1100); setSize(1040, 800);
}
DreamMasterLiteEditor::~DreamMasterLiteEditor() { viewport.setViewedComponent(nullptr, false); }

void DreamMasterLiteEditor::paint(juce::Graphics& g) {
    g.fillAll(bg); auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient glow(acid.withAlpha(0.06f), bounds.getWidth() * 0.18f, 0.0f, juce::Colours::transparentBlack, bounds.getWidth() * 0.62f, bounds.getHeight(), false);
    g.setGradientFill(glow); g.fillRect(bounds);
    for (int x = 12; x < getWidth(); x += 31) for (int y = 7 + ((x * 13) % 43); y < getHeight(); y += 23 + ((x / 31) % 4) * 7) {
        const int glyph = (x * 7 + y * 3) % 4; g.setColour(green.withAlpha(glyph == 0 ? 0.14f : 0.045f)); g.setFont(juce::Font(10.0f));
        g.drawText(glyph == 0 ? "1" : (glyph == 1 ? "+" : (glyph == 2 ? ":" : "0")), x, y, 9, 11, juce::Justification::centred);
    }
    g.setColour(juce::Colours::black.withAlpha(0.22f)); for (int y = 0; y < getHeight(); y += 4) g.fillRect(0, y, getWidth(), 1);
    const float cy = 27.0f, eyeW = 34.0f, eyeH = 22.0f, mid = getWidth() * 0.5f;
    for (int side : { -1, 1 }) { juce::Rectangle<float> eye(mid + side * 150.0f - eyeW * 0.5f, cy - eyeH * 0.5f, eyeW, eyeH);
        g.setColour(green.withAlpha(0.85f)); g.drawEllipse(eye, 2.0f); g.setColour(juce::Colour(0xff0c2112)); g.fillEllipse(eye.reduced(2.0f));
        g.setColour(acid); g.fillEllipse(eye.getCentreX() - 5.0f, eye.getCentreY() - 5.0f, 10.0f, 10.0f); g.setColour(bg); g.fillEllipse(eye.getCentreX() - 2.0f, eye.getCentreY() - 2.0f, 4.0f, 4.0f); }
    g.setColour(border); g.drawRect(getLocalBounds().reduced(2), 2); g.setColour(acid.withAlpha(0.45f)); g.drawRect(getLocalBounds().reduced(5), 1);
}

void DreamMasterLiteEditor::resized() {
    auto r = getLocalBounds().reduced(10); title.setBounds(r.removeFromTop(36)); subtitle.setBounds(r.removeFromTop(20)); auto top = r.removeFromTop(34);
    randomButton.setBounds(top.removeFromLeft(205).reduced(2)); resetButton.setBounds(top.removeFromLeft(85).reduced(2)); websiteButton.setBounds(top.removeFromLeft(145).reduced(2)); countLabel.setBounds(top.reduced(5, 2)); viewport.setBounds(r);
    const int columns = getWidth() >= 1050 ? 4 : 3, gap = 8, rowHeight = 66;
    const int cardWidth = juce::jmax(150, (r.getWidth() - gap * (columns - 1)) / columns);
    const int rows = (200 + columns - 1) / columns; content.setSize(columns * cardWidth + (columns - 1) * gap, rows * (rowHeight + gap));
    for (int i = 0; i < 200; ++i) {
        const int col = i % columns, row = i / columns, x = col * (cardWidth + gap), y = row * (rowHeight + gap);
        effectLabels[i]->setBounds(x + 7, y + 4, cardWidth - 57, 20);
        effectButtons[i]->setBounds(x + cardWidth - 48, y + 2, 44, 22);
        amountSliders[i]->setBounds(x + 8, y + 30, cardWidth - 16, 25);
    }
}

void DreamMasterLiteEditor::randomize() {
    // Pick 1–10 distinct modules; clear the rest so this button can never enable more than ten.
    juce::Random rng; std::vector<int> indices(200); for (int i = 0; i < 200; ++i) indices[(size_t)i] = i;
    for (int i = 199; i > 0; --i) std::swap(indices[(size_t)i], indices[(size_t)rng.nextInt(i + 1)]);
    const int count = 1 + rng.nextInt(10);
    for (int i = 0; i < 200; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(i))) p->setValueNotifyingHost(0.0f);
    for (int i = 0; i < count; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(indices[(size_t)i]))) p->setValueNotifyingHost(1.0f);
    countLabel.setText(juce::String(count) + " ACTIVE · RANDOM PICK MAX 10", juce::dontSendNotification);
}
void DreamMasterLiteEditor::resetAll() {
    for (int i = 0; i < 200; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(i))) p->setValueNotifyingHost(0.0f);
    countLabel.setText("0 ACTIVE · RANDOM PICK MAX 10", juce::dontSendNotification);
}
juce::AudioProcessorEditor* DreamMasterLiteProcessor::createEditor() { return new DreamMasterLiteEditor(*this); }
