#include "PluginEditor.h"
#include <algorithm>
#include <numeric>
#include <vector>
#include <random>

namespace {
const juce::Colour bg(0xff030805), panel(0xff0b120d), acid(0xffc8ff33), green(0xff65ff83), text(0xffd8ffe0), dim(0xffb5a18a), border(0xff69412e);
juce::PropertiesFile& getFavoritesFile() {
    static juce::PropertiesFile file([] {
        juce::PropertiesFile::Options options;
        options.applicationName = "DreamMasterLite";
        options.filenameSuffix = "settings";
        options.folderName = "Dreamdaw";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers = false;
        options.ignoreCaseOfKeyNames = true;
        options.millisecondsBeforeSaving = -1;
        return options;
    }());
    return file;
}
juce::String getFeatureName(int index) {
    const auto name = dm::featureNames[(size_t)index];
    return juce::String::fromUTF8(name.data(), static_cast<int>(name.size()));
}
juce::String getFavoriteKey(int index) {
    const auto id = dm::featureIds[(size_t)index];
    return "favorite_" + juce::String::fromUTF8(id.data(), static_cast<int>(id.size()));
}
bool loadFavorite(int index) {
    auto& file = getFavoritesFile();
    const auto key = getFavoriteKey(index);
    if (file.containsKey(key))
        return file.getBoolValue(key, false);

    const auto legacyKey = "favorite_" + juce::String(index);
    const bool favorite = file.getBoolValue(legacyKey, false);
    if (favorite) {
        file.setValue(key, true);
        if (!file.saveIfNeeded())
            juce::Logger::writeToLog("DreamMasterLite: could not migrate saved favorites to the current settings file.");
    }
    return favorite;
}
void styleButton(juce::TextButton& button, juce::Colour fill = panel, juce::Colour ink = green) {
    button.setColour(juce::TextButton::buttonColourId, fill);
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff153c20));
    button.setColour(juce::TextButton::textColourOffId, ink);
    button.setColour(juce::TextButton::textColourOnId, acid);
}
class EffectCard final : public juce::Component {
public:
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(panel.withAlpha(0.96f)); g.fillRoundedRectangle(b, 3.0f);
        g.setColour(border); g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);
        g.setColour(green.withAlpha(0.12f)); g.drawLine(7.0f, 25.0f, b.getWidth()-7.0f, 25.0f, 1.0f);
    }
};
void getControlNames(const std::string& id, juce::String& a, juce::String& b, juce::String& c) {
    auto has = [&](const char* s) { return id.find(s) != std::string::npos; };
    if (has("delay") || has("echo") || has("slap") || has("predelay") || has("reverb") || has("room") || has("hall") || has("plate") || has("halo") || has("ripple") || has("orbit") || has("seance") || has("choir") || has("comb")) { a="Mix"; b="Time"; c="Feedback"; }
    else if (has("lowpass") || has("lpf") || has("highpass") || has("hpf") || has("haze") || has("fog") || has("notch") || has("mud")) { a="Cutoff"; b="Resonance"; c="Mix"; }
    else if (has("width") || has("widen") || has("stereo") || has("mono") || has("pan") || has("image")) { a="Width"; b="Center"; c="Mix"; }
    else if (has("chorus") || has("flanger") || has("phaser") || has("vibrato") || has("trem") || has("pulse") || has("worm") || has("vortex") || has("rotary")) { a="Depth"; b="Rate"; c="Mix"; }
    else if (has("drive") || has("grit") || has("sat") || has("clip") || has("rot") || has("melt") || has("warm") || has("tape") || has("ink") || has("grind")) { a="Drive"; b="Tone"; c="Mix"; }
    else if (has("limit") || has("ceiling") || has("loud") || has("glue") || has("compress") || has("polish")) { a="Amount"; b="Threshold"; c="Release"; }
    else if (has("gate") || has("stutter") || has("chop") || has("duck")) { a="Amount"; b="Rate"; c="Smooth"; }
    else if (has("grain") || has("crush") || has("lofi") || has("radio") || has("phone") || has("bit")) { a="Amount"; b="Bits"; c="Sample"; }
    else if (has("air") || has("sheen") || has("sparkle") || has("presence") || has("edge")) { a="Amount"; b="Frequency"; c="Mix"; }
    else if (has("bass") || has("sub") || has("lowend") || has("body") || has("punch")) { a="Amount"; b="Frequency"; c="Mix"; }
    else { a="Amount"; b="Character"; c="Mix"; }
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
    countLabel.setText("0 ACTIVE - RANDOM PICK MAX 10", juce::dontSendNotification);
    countLabel.setColour(juce::Label::textColourId, acid); countLabel.setFont(juce::Font(10.5f, juce::Font::bold)); addAndMakeVisible(countLabel);
    styleButton(randomButton, juce::Colour(0xff102b17), green); styleButton(resetButton, juce::Colour(0xff20130e), text); styleButton(websiteButton, juce::Colour(0xff102b17), acid);
    for (auto* b : { &randomButton, &resetButton, &websiteButton }) addAndMakeVisible(*b);
    randomButton.onClick = [this] { randomize(); }; resetButton.onClick = [this] { resetAll(); };
    websiteButton.onClick = [] { juce::URL("https://dreamdaw.com").launchInDefaultBrowser(); };
    viewport.setViewedComponent(&content, false); viewport.setScrollBarsShown(true, false);
    viewport.setScrollOnDragEnabled(false);
    viewport.setColour(juce::ScrollBar::thumbColourId, green.withAlpha(0.8f)); viewport.setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff061009)); addAndMakeVisible(viewport);

    for (int i = 0; i < 200; ++i) {
        favoriteStates[(size_t)i] = loadFavorite(i);
        auto* card = effectCards.add(new EffectCard()); content.addAndMakeVisible(card);
        const auto name = getFeatureName(i);
        auto* label = effectLabels.add(new juce::Label({}, name));
        label->setColour(juce::Label::textColourId, text); label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centredLeft); content.addAndMakeVisible(label);
        auto* button = effectButtons.add(new juce::ToggleButton("ON"));
        button->setClickingTogglesState(true); button->setColour(juce::ToggleButton::textColourId, green);
        button->setColour(juce::ToggleButton::tickColourId, acid); content.addAndMakeVisible(button);
        buttonAttachments.add(new juce::AudioProcessorValueTreeState::ButtonAttachment(proc.state, "fx" + juce::String(i), *button));
        auto* favorite = favoriteButtons.add(new juce::TextButton("FAV"));
        favorite->setClickingTogglesState(true);
        favorite->setColour(juce::TextButton::buttonColourId, panel);
        favorite->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff33420d));
        favorite->setColour(juce::TextButton::textColourOffId, dim);
        favorite->setColour(juce::TextButton::textColourOnId, acid);
        favorite->setTitle("Favorite " + name);
        favorite->setToggleState(favoriteStates[(size_t)i], juce::dontSendNotification);
        favorite->setTooltip("Favorite this effect. Favorites are saved on this computer.");
        favorite->onClick = [this, favorite, i] {
            const bool on = favorite->getToggleState();
            if (!saveFavorite(i, on))
                favorite->setTooltip("Favorite is active for this session, but could not be saved to disk.");
            else
                favorite->setTooltip("Favorite this effect. Favorites are saved on this computer.");
            resized();
        };
        content.addAndMakeVisible(favorite);

        juce::String c1, c2, c3; getControlNames(std::string(dm::featureIds[(size_t)i]), c1, c2, c3);
        auto makeLabel = [&](juce::OwnedArray<juce::Label>& collection, const juce::String& t) {
            auto* l = collection.add(new juce::Label({}, t)); l->setColour(juce::Label::textColourId, dim); l->setFont(juce::Font(9.0f)); l->setJustificationType(juce::Justification::centredLeft); content.addAndMakeVisible(l); return l;
        };
        makeLabel(control1Labels, c1); makeLabel(control2Labels, c2); makeLabel(control3Labels, c3);
        auto makeSlider = [&](juce::OwnedArray<juce::Slider>& collection, const juce::String& param) {
            auto* s = collection.add(new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
            s->setRange(0.0, 1.0, 0.001); s->setColour(juce::Slider::trackColourId, green); s->setColour(juce::Slider::thumbColourId, acid);
            s->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff484848));
            // Critical: scrolling the viewport over a control must never alter its value.
            s->setScrollWheelEnabled(false); content.addAndMakeVisible(s);
            sliderAttachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(proc.state, param + juce::String(i), *s)); return s;
        };
        makeSlider(amountSliders, "amt"); makeSlider(control2Sliders, "ctrl2"); makeSlider(control3Sliders, "ctrl3");
    }
    setResizable(true, true); setResizeLimits(650, 520, 1600, 1200); setSize(1080, 820);
}
DreamMasterLiteEditor::~DreamMasterLiteEditor() { viewport.setViewedComponent(nullptr, false); }
bool DreamMasterLiteEditor::saveFavorite(int index, bool enabled) {
    favoriteStates[(size_t)index] = enabled;
    auto& file = getFavoritesFile();
    file.setValue(getFavoriteKey(index), enabled);
    if (file.saveIfNeeded())
        return true;

    juce::Logger::writeToLog("DreamMasterLite: could not save favorites to " + file.getFile().getFullPathName());
    return false;
}

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
    const int columns = getWidth() >= 1050 ? 4 : 3, gap = 9, cardHeight = 116;
    const int cardWidth = juce::jmax(174, (r.getWidth() - gap * (columns - 1)) / columns);
    const int rows = (200 + columns - 1) / columns; content.setSize(columns * cardWidth + (columns - 1) * gap, rows * (cardHeight + gap));
    std::array<int, 200> displayOrder;
    std::iota(displayOrder.begin(), displayOrder.end(), 0);
    std::sort(displayOrder.begin(), displayOrder.end(), [this](int left, int right) {
        const bool leftIsFavorite = favoriteStates[(size_t)left];
        const bool rightIsFavorite = favoriteStates[(size_t)right];
        if (leftIsFavorite != rightIsFavorite)
            return leftIsFavorite;
        if (leftIsFavorite) {
            const int nameOrder = getFeatureName(left).compareIgnoreCase(getFeatureName(right));
            if (nameOrder != 0)
                return nameOrder < 0;
        }
        return left < right;
    });
    for (int slot = 0; slot < 200; ++slot) {
        const int i = displayOrder[(size_t)slot];
        const int col = slot % columns, row = slot / columns, x = col * (cardWidth + gap), y = row * (cardHeight + gap);
        effectCards[i]->setBounds(x, y, cardWidth, cardHeight);
        effectLabels[i]->setBounds(x + 7, y + 3, cardWidth - 110, 22);
        effectButtons[i]->setBounds(x + cardWidth - 88, y + 2, 48, 21);
        favoriteButtons[i]->setBounds(x + cardWidth - 38, y + 2, 32, 21);
        const int labelWidth = 58, sliderWidth = cardWidth - labelWidth - 18;
        control1Labels[i]->setBounds(x + 8, y + 29, labelWidth, 20); amountSliders[i]->setBounds(x + labelWidth + 8, y + 28, sliderWidth, 20);
        control2Labels[i]->setBounds(x + 8, y + 54, labelWidth, 20); control2Sliders[i]->setBounds(x + labelWidth + 8, y + 53, sliderWidth, 20);
        control3Labels[i]->setBounds(x + 8, y + 79, labelWidth, 20); control3Sliders[i]->setBounds(x + labelWidth + 8, y + 78, sliderWidth, 20);
    }
}
void DreamMasterLiteEditor::randomize() {
    juce::Random rng; std::vector<int> indices(200); for (int i = 0; i < 200; ++i) indices[(size_t)i] = i;
    for (int i = 199; i > 0; --i) std::swap(indices[(size_t)i], indices[(size_t)rng.nextInt(i + 1)]);
    const int count = 1 + rng.nextInt(10);
    for (int i = 0; i < 200; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(i))) p->setValueNotifyingHost(0.0f);
    for (int i = 0; i < count; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(indices[(size_t)i]))) p->setValueNotifyingHost(1.0f);
    countLabel.setText(juce::String(count) + " ACTIVE - RANDOM PICK MAX 10", juce::dontSendNotification);
}
void DreamMasterLiteEditor::resetAll() {
    for (int i = 0; i < 200; ++i) if (auto* p = proc.state.getParameter("fx" + juce::String(i))) p->setValueNotifyingHost(0.0f);
    countLabel.setText("0 ACTIVE - RANDOM PICK MAX 10", juce::dontSendNotification);
}
juce::AudioProcessorEditor* DreamMasterLiteProcessor::createEditor() { return new DreamMasterLiteEditor(*this); }
