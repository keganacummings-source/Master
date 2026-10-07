#pragma once
#include "PluginProcessor.h"
#include "ChainLevelControls.h"
#include "Themes.h"
#include "Attach.h"
#include "DreamBoard.h"
#include "MachineDesign.h"
#include "ModularParts.h"
#include "WaveDisplay.h"
#include "PluginShells.h"
#include "FlexLayout.h"
#include "BuilderFit.h"
#include <vector>

struct CanvasWidget : public juce::Component
{
    enum class Kind { Dial, Slider, Key, Wave, Stack, Board, Cosmetic, Button, Sound };
    std::function<void(CanvasWidget*, const juce::MouseEvent&)> onRightClick;

    CanvasWidget(KyotoAudioProcessor& p, juce::ValueTree n);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setTheme(const kt::ThemePalette& t);
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    KyotoAudioProcessor& proc;
    juce::ValueTree node;
    Kind kind = Kind::Dial;
    juce::Slider slider;
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::unique_ptr<WaveDisplay> waveDisplay;
    std::function<void()> onSelect;
    bool selected = false;
    kt::ThemePalette theme = kt::kThemes[0];
};

class FxBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    struct CustomItem { juce::String id, name, author, categories; bool remote = false; };

    FxBrowser(KyotoAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setTheme(const kt::ThemePalette& t);
    void setCustomItems(const juce::Array<CustomItem>& items);
    void showCustom(bool custom);
    bool isShowingCustom() const { return customMode; }
    int getSelectedFx() const { return selected; }
    void setSelectedFx(int index);
    std::function<void(int)> onSelect;
    std::function<void(const CustomItem&)> onCustomSelect;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selectedRow) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override;
    void rebuildFilter();

    KyotoAudioProcessor& proc;
    juce::ListBox list;
    juce::TextEditor search;
    juce::ComboBox familyBox;
    juce::TextButton builtInTab { "BUILT-IN" }, customTab { "CUSTOM" };
    juce::String filter;
    juce::Array<int> filtered;
    juce::Array<int> customFiltered;
    juce::Array<CustomItem> customItems;
    int selected = 0;
    int selectedCustom = -1;
    bool customMode = false;
    kt::ThemePalette theme = kt::kThemes[0];
};


class SocialRail : public juce::Component
{
public:
    struct Bubble { juce::String id, user, text, themeId; };
    struct Person { juce::String name, detail, themeId, kind, requestId; bool online = false; };

    std::function<void(const Bubble&, juce::Point<int>)> onBubbleMenu;
    std::function<void(const Person&)> onPersonClick;
    std::function<void(const Person&, juce::Point<int>)> onPersonMenu;
    std::function<void(const kt::AttachRef&)> onFileClick;

    void setBubbles(const juce::Array<Bubble>& next) { bubbles = next; syncSize(); }
    void setPeople(const juce::Array<Person>& next) { people = next; syncSize(); }
    void setMode(int next) { mode = next; syncSize(); }
    void setPhase(float next) { phase = next; repaint(); }
    void setHostTheme(const kt::ThemePalette& t) { host = t; repaint(); }
    void setShowDirectory(bool on) { showDirectory = on; syncSize(); }
    void setSelfUser(const juce::String& name) { selfUser = name; repaint(); }
    int contentHeight() const { return contentHeightFor(getWidth()); }
    int contentHeightFor(int width) const;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void syncSize() { setSize(juce::jmax(220, getWidth()), contentHeight()); repaint(); }
    int rowAt(juce::Point<int> pos, Bubble* bubble, Person* person) const;
    juce::Array<Bubble> bubbles;
    juce::Array<Person> people;
    kt::ThemePalette host = kt::kThemes[0];
    int mode = 0;
    float phase = 0.f;
    bool showDirectory = false;
    juce::String selfUser;
};

// Full-screen viewer surface for Pluggin mode. Defined in PluginViewScreen.h; it installs
// itself as a child of the editor (see parentHierarchyChanged) so Plugin View shows the built
// pluggin live with only a Back and a Geek button, and nothing else, on screen.
class PluginViewScreen;

class KyotoAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget
{
public:
    explicit KyotoAudioProcessorEditor(KyotoAudioProcessor&);
    ~KyotoAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void paintOverChildren(juce::Graphics&) override;
    void parentHierarchyChanged() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool inPluginView() const { return pluginView; }

private:
    struct EditorSnapshot
    {
        juce::MemoryBlock processorState;
        juce::String fxStackJson;
        int selectedFx = -1;
    };

    void timerCallback() override;
    void showTab(int tab);
    void rebuildCanvas();
    void addSeriesStep();
    void armPlacement();
    void placeInSlot(int slot);
    void ensureMotherboard();
    void syncPanelMouse();
    void applyShell(int index);
    bool slotOccupied(int slot) const;
    juce::Point<float> slotAnchor(int slot) const;
    void reflowSeries();
    juce::Rectangle<int> cellFor(int index, const juce::String& kind) const;
    bool findAutoCell(int index, const juce::String& kind, juce::Rectangle<int>& result) const;
    void saveLocal();
    void publish();
    void login();
    void logout();
    void sendChat();
    void refreshFeed();
    void chatUtility(const juce::String& action);
    void refreshCatalog();
    // Chain wiring: new parts connect into the highlighted part (or the end of the chain).
    int chainParentWidget() const;
    int nearestFreeBay(const juce::String& style, juce::Point<float> localPos, int preferred) const;
    void shiftDspUp(int from, int to);
    void repairParents();
    void refreshCurrentCenter();
    void layoutCenterHolder();
    void applyDsScale();
    int dropTargetAt(int x, int y) const;
    void stageAttachment(const juce::File& file, int target);
    void clearAttachment();
    void sendWithAttachment(int target, const juce::String& text);
    void saveAttachmentAs(const kt::AttachRef& ref);
    void deleteCatalogId(const juce::String& id);
    void loadCatalogId(const juce::String& id, const juce::String& name);
    void applyTheme(const juce::String& id);
    void loadWav();
    void saveEffect();
    void publishEffect();
    void shareEffectToChat();
    void shareEffectToThread();
    void setCenterMode(int mode);
    void setRailMode(int mode);
    void rebuildCenter();
    void rebuildThreadBoard();
    void showCatalogCardMenu(const juce::String& id, const juce::String& name, const juce::String& author, juce::Point<int> screenPos);
    void approveCatalogId(const juce::String& id);
    void denyCatalogId(const juce::String& id);
    void tagCatalogId(const juce::String& id, const juce::String& tags);
    void refreshMyModules();
    void refreshPending();
    void refreshSocial();
    void openWavRequest(const juce::String& name);
    void openDirectMessage(const juce::String& name);
    void showPersonMenu(const SocialRail::Person& person, juce::Point<int> screenPos);
    void showBubbleMenu(const SocialRail::Bubble& bubble, juce::Point<int> screenPos);
    void openThread(const juce::String& id);
    // DreamShare thread board (Reddit / 4chan style)
    kt::ThreadBoard threadBoard;
    void wireThreadBoard();
    void submitBoardPost(const juce::String& threadId, const juce::String& title, const juce::String& body,
                         const juce::Array<juce::File>& files, const juce::String& modToken);
    void showBoardFileMenu(const kt::AttachRef& ref, juce::Point<int> pos);
    void useAttachmentAsSample(const kt::AttachRef& ref);
    void closeThread();
    void postThreadComment(const juce::String& text);
    void reactTo(const juce::String& kind, const juce::String& id, const juce::String& emoji);
    void rebuildThreadDetail();
    float scaledFont(float baseSize) const;
    void addSpecialChainStep(int type, const juce::String& name);
    void refreshEffectBox();
    void updateFxControls();
    void selectFxStep(int index);
    void writeFxStepFromControls();
    void randomizeFxControls();
    void removeSelectedChainStep();
    void removeSelectedFxStep();
    void undoLast();
    void captureSnapshot();
    void restoreSnapshot(const EditorSnapshot& snapshot);
    juce::String serializeFxStack() const;
    void restoreFxStack(const juce::String& json);
    void renderEffectLinks(const juce::String& text);
    void clearEffectLinks();
    juce::String effectShareText() const;
    juce::File sessionFile() const;
    juce::File moduleDir() const;
    juce::File effectDir() const;
    void setLoggedIn(bool on);
    void setPluginView(bool on);
    void startNewMachine(int playgroundMode);
    void randomizeMachine();
    void randomizeTemplate();
    void persistEditorSession();
    void restoreEditorSession();
    void showSlotMenu(int slot, juce::Point<int> screenPos);
    void editEffectPopup(int widgetIndex);
    void placeKindInSlot(const juce::String& kind, int slot, int fxIndex, const juce::String& label);
    void syncMachineDesignToUi();
    void enterBuilderWizard();
    void advanceBuilderWizard();
    void applyPlaygroundTheme(const juce::String& id);
    juce::String deriveCategoriesFromStack() const;

    KyotoAudioProcessor& proc;
    juce::TooltipWindow tooltipWindow { this, 500 };
    int tab = 0;
    bool loggedIn = false;
    bool isAdmin = false;
    bool pluginView = false;
    bool geekMode = false;
    float geekReveal = 0.f;
    int geekHotPart = -1;
    bool proMode = false;
    int builderWizardStep = 0; // 0=builder, 1=shell, 2=theme, 3=pick/place FX, 4=controls
    kt::ThemePalette playgroundTheme = kt::kThemes[0];
    MachineDesign machineDesign;
    float animPhase = 0.f;
    juce::String token, account;
    juce::String lastPublishedEffectId;
    kt::ThemePalette theme = kt::kThemes[0];

    juce::TextButton shareBtn { "DREAMSHARE" }, chainBtn { "PLUGIN BUILDER" }, fxBtn { "FX BUILDER" }, logoutBtn { "LOG OUT" };
    juce::TextButton pluginViewBtn { "PLUGIN VIEW" }, pluginBackBtn { "< BACK" }, newMachineBtn { "ASPECT RATIO" }, randomMachineBtn { "RANDOMIZE MACHINE" };
    juce::TextButton proToggleBtn { "PRO  -  OFF" }, wizardNextBtn { "NEXT >" }, wizardSkipBtn { "SKIP TO BUILDER" };
    juce::ComboBox shellBox, playgroundThemeBox;
    juce::Label shellLabel, playgroundThemeLabel;
    juce::TextButton chatRefreshBtn { "CHAT" }, threadsBtn { "THREADS" }, socialBtn { "FRIENDS" }, dmBtn { "DM" }, adminDeleteBtn { "REMOVE" }, utilityGoBtn { "GO" };
    juce::TextEditor utilityBox;
    juce::ComboBox utilityActionBox;
    juce::Label status, whoLabel;

    juce::TextEditor userBox, passBox, msgBox, logBox;
    juce::TextButton loginBtn { "LOG IN" }, sendBtn { "SEND" }, feedBtn { "REFRESH" };
    juce::ComboBox themeBox;
    juce::Viewport catalogView;
    juce::Component catalogHolder;
    juce::Component threadHolder;
    juce::Viewport chatView;
    SocialRail socialRail;
    juce::TextButton catalogModeBtn { "CATALOG" }, threadsModeBtn { "THREADS" }, railChatBtn { "CHAT" }, railOnlineBtn { "SOCIALS" };
    juce::TextButton pluginsTabBtn { "PLUGINS" }, effectsTabBtn { "EFFECTS" }, myPluginsBtn { "MY PLUGINS" }, pendingBtn { "PENDING" };
    juce::TextEditor tagSearchBox;
    juce::TextButton textMinusBtn { "A-" }, textPlusBtn { "A+" }, attachChip { "" };
    juce::File pendingAttach;
    int pendingAttachTarget = 0; // 0 none, 1 live chat, 2 open thread
    int dragTarget = 0;
    int searchTick = 0, catalogSeq = 0;
    int centerMode = 3; // 0=community plugins, 1=effects, 2=my plugins, 3=threads, 4=pending
    int railMode = 0;
    bool scrollChatOnRefresh = true;
    bool threadOpen = false;
    juce::String selectedThreadId;
    struct ThreadComment { juce::String id, user, text, themeId; };
    struct ThreadItem { juce::String id, user, title, text, themeId; juce::int64 at = 0; int score = 0; int comments = 0; juce::Array<ThreadComment> commentList; };
    juce::TextButton threadBackBtn { "< THREADS" }, threadReactBtn { "REACT" }, threadShareFxBtn { "SHARE FX" }, threadSharePluginBtn { "SHARE PLUGIN" };
    juce::Array<ThreadItem> threads;
    juce::OwnedArray<juce::TextButton> feedEffectButtons;

    juce::TextEditor nameBox, effectNameBox;
    juce::ComboBox gridStyleBox, presetBox, kindBox, effectBox;
    juce::ComboBox paramBox, pieceBox;
    const kt::ModPiece* pendingPiece = nullptr;
    juce::TextButton addBtn { "PLACE" }, saveBtn { "SAVE" }, upBtn { "PUBLISH" }, wavBtn { "WAV" }, chainBreakBtn { "BREAK" }, chainMixBtn { "MIX" }, chainRemoveBtn { "REMOVE" }, chainUndoBtn { "UNDO" }, randomTemplateBtn { "RANDOMIZE" };
    juce::TextButton fxAddBtn { "ADD FX" }, fxSaveBtn { "SAVE" }, fxUpBtn { "PUBLISH" }, fxShareChatBtn { "CHAT" }, fxShareThreadBtn { "THREAD" }, fxRemoveBtn { "REMOVE" }, fxUndoBtn { "UNDO" };
    juce::Slider fxAmount, fxTone, fxMotion, fxMix, fxShape;
    juce::Label fxAmountLabel, fxToneLabel, fxMotionLabel, fxMixLabel, fxShapeLabel, stackLabel;
    juce::TextButton fxBreakBtn { "BREAK" }, fxMixBtn { "MASTER MIX" }, fxRandomBtn { "RANDOM" }, fxClearBtn { "CLEAR" };
    int selectedFxStep = -1;
    int selectedChainWidget = -1;
    bool placing = false;
    juce::String armedStyle { "dial" };
    int shellIndex = 0;
    int pendingFx = 0;
    bool pendingSpecial = false;
    int pendingSpecialType = 0;
    juce::String pendingLabel;
    ChainLevelControls chainLevels { proc };
    pb::BuilderCanvas panel;
    std::unique_ptr<FxBrowser> fxBrowser;
    std::unique_ptr<WaveDisplay> waveDisplay;
    juce::OwnedArray<CanvasWidget> widgets;
    juce::ValueTree fxStack { "fxstack" };
    PluginViewScreen* viewScreen = nullptr;

    friend class PluginViewScreen;

    struct CatalogItem { juce::String id, name, face, author, status, tags; };
    juce::String selectedCatalogId;
    juce::Array<CatalogItem> catalog;
    std::vector<EditorSnapshot> undoStack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KyotoAudioProcessorEditor)
};

// Installs the Pluggin viewer overlay once the editor is hosted. Included at the bottom so
// PluginViewScreen sees the complete editor class while both include guards stay valid.
#include "PluginViewScreen.h"