#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <map>
#include <set>
#include "Themes.h"
#include "Attach.h"

// ============================================================================
//  DREAMSHARE THREAD BOARD
//  A Reddit / 4chan style board that fills the DreamShare centre column.
//    LIST   : sortable thread rows (vote column, thumbnail, title, badges)
//    THREAD : OP + numbered replies, inline images, file chips, plugin chips
//  Threads and replies carry images, .zip, audio and shared plugins as tokens
//  inside the existing text field (see Attach.h) so the Worker is unchanged.
//  Files can be dragged straight onto the board: on the list they start a new
//  thread, inside a thread they attach to your reply.
// ============================================================================
namespace kt
{
struct BoardComment { juce::String id, user, text, themeId; };
struct BoardThread
{
    juce::String id, user, title, text, themeId;
    juce::int64 at = 0;
    int score = 0;
    juce::Array<BoardComment> comments;
};

class ThreadBoard final : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    // ---- callbacks wired by the editor ------------------------------------
    std::function<void(const juce::String&, const juce::String&, const juce::Array<juce::File>&, const juce::String&)> onPost;
    std::function<void(const juce::String&, const juce::String&, const juce::Array<juce::File>&, const juce::String&)> onReply;
    std::function<void(const AttachRef&, std::function<void(juce::Image)>)> fetchImage;
    std::function<void(const AttachRef&, juce::Point<int>)> onFileClick;
    std::function<void(const juce::String&, const juce::String&)> onOpenPlugin;
    std::function<void(std::function<void(juce::String, juce::String)>)> onPickPlugin;
    std::function<void(const juce::String&)> onUpvote;
    std::function<void()> onRefresh;
    std::function<void(const juce::String&)> onStatus;

    ThreadBoard()
    {
        for (auto* e : { &titleEd, &bodyEd, &replyEd })
        {
            e->setMultiLine(e != &titleEd, true);
            e->setReturnKeyStartsNewLine(e != &titleEd);
            e->setScrollbarsShown(false);
            addChildComponent(*e);
        }
        titleEd.setTextToShowWhenEmpty("Thread title", juce::Colour(0x80808080));
        bodyEd.setTextToShowWhenEmpty("Say something. Drop images, .zip, audio or a plugin on this board to attach them.", juce::Colour(0x80808080));
        replyEd.setTextToShowWhenEmpty("Reply (drop files here). >>123 quotes a post, a line starting with > is green.", juce::Colour(0x80808080));
        setWantsKeyboardFocus(false);
    }

    void setTheme(const ThemePalette& t)
    {
        host = t;
        for (auto* e : { &titleEd, &bodyEd, &replyEd })
        {
            e->setColour(juce::TextEditor::backgroundColourId, c(host.bg).brighter(0.03f));
            e->setColour(juce::TextEditor::textColourId, c(host.text));
            e->setColour(juce::TextEditor::outlineColourId, c(host.border));
            e->setColour(juce::TextEditor::focusedOutlineColourId, c(host.accent).withAlpha(0.85f));
            e->setColour(juce::TextEditor::highlightColourId, c(host.accent).withAlpha(0.30f));
            e->applyFontToAllText(dsFont(host, 13.f));
            e->setFont(dsFont(host, 13.f));
        }
        repaint();
    }

    void setThreads(const juce::Array<BoardThread>& t)
    {
        threads = t;
        if (openId.isNotEmpty() && findThread(openId) == nullptr) openId.clear();
        clampScroll();
        repaint();
    }

    void openThreadById(const juce::String& id) { openId = id; scrollY = 0; resized(); repaint(); }
    void closeThread() { openId.clear(); scrollY = 0; resized(); repaint(); }
    bool isThreadOpen() const { return openId.isNotEmpty(); }
    juce::String currentThread() const { return openId; }
    void setBusy(bool b, const juce::String& msg = {}) { busy = b; busyMsg = msg; repaint(); }
    void clearComposer()
    {
        titleEd.clear(); bodyEd.clear(); replyEd.clear();
        staged.clear(); stagedMod = {}; stagedModName = {};
        composerOpen = false;
        resized(); repaint();
    }
    void stageFiles(const juce::Array<juce::File>& files)
    {
        const int cap = openId.isEmpty() ? 4 : 3;
        for (const auto& f : files)
        {
            if (! f.existsAsFile()) continue;
            if (f.getSize() > kAttachMaxBytes)
            {
                if (onStatus) onStatus(f.getFileName() + " is over " + humanBytes(kAttachMaxBytes) + " - too big to attach");
                continue;
            }
            if (staged.size() >= cap) { if (onStatus) onStatus("Up to " + juce::String(cap) + " files per post"); break; }
            staged.addIfNotAlreadyThere(f);
        }
        if (openId.isEmpty() && ! composerOpen) { composerOpen = true; }
        resized(); repaint();
    }

    // ---- FileDragAndDropTarget -----------------------------------------------
    bool isInterestedInFileDrag(const juce::StringArray& f) override { return f.size() > 0; }
    void fileDragEnter(const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped(const juce::StringArray& f, int, int) override
    {
        dragOver = false;
        juce::Array<juce::File> files;
        for (auto& s : f) files.add(juce::File(s));
        stageFiles(files);
        if (onStatus) onStatus(openId.isEmpty() ? "Files staged on a new thread - add a title and POST"
                                                : "Files staged on your reply - press REPLY");
    }

    // ---- layout ----------------------------------------------------------------
    void resized() override
    {
        const int w = getWidth();
        const int pad = 8;
        const bool list = openId.isEmpty();
        titleEd.setVisible(list && composerOpen);
        bodyEd.setVisible(list && composerOpen);
        replyEd.setVisible(! list);
        if (list && composerOpen)
        {
            titleEd.setBounds(pad + 4, kTop + 8, w - pad * 2 - 8, 28);
            bodyEd.setBounds(pad + 4, kTop + 42, w - pad * 2 - 8, 64);
        }
        if (! list)
            replyEd.setBounds(pad + 4, getHeight() - kReplyBar + 8, w - pad * 2 - 8, 50);
        clampScroll();
    }

    void paint(juce::Graphics& g) override
    {
        hits.clear();
        g.fillAll(c(host.bg));
        const bool list = openId.isEmpty();
        auto view = viewport();
        {
            juce::Graphics::ScopedSaveState s(g);
            g.reduceClipRegion(view);
            contentH = list ? paintList(&g, view) : paintThread(&g, view);
        }
        paintTopBar(g, list);
        if (list && composerOpen) paintComposer(g);
        if (! list) paintReplyBar(g);
        if (busy)
        {
            g.setColour(c(host.accent).withAlpha(0.9f));
            g.setFont(dsFont(host, 11.f, true));
            g.drawText(busyMsg.isEmpty() ? juce::String("Uploading...") : busyMsg, getLocalBounds().removeFromBottom(kReplyBar + 4).removeFromTop(16).reduced(14, 0), juce::Justification::centredRight);
        }
        if (dragOver)
        {
            auto r = getLocalBounds().toFloat().reduced(3.f);
            g.setColour(c(host.bg).withAlpha(0.74f));
            g.fillRoundedRectangle(r, 12.f);
            g.setColour(c(host.accent).withAlpha(0.2f));
            g.fillRoundedRectangle(r, 12.f);
            g.setColour(c(host.accent));
            g.drawRoundedRectangle(r.reduced(3.f), 10.f, 2.5f);
            g.setFont(dsFont(host, 16.f, true));
            g.drawFittedText(list ? "DROP TO START A NEW THREAD WITH THESE FILES" : "DROP TO ATTACH TO YOUR REPLY",
                             getLocalBounds().reduced(24), juce::Justification::centred, 3);
        }
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& d) override
    {
        scrollY -= (int) (d.deltaY * 520.f);
        clampScroll();
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const auto p = e.getPosition();
        for (int i = hits.size(); --i >= 0;)
        {
            const auto& h = hits.getReference(i);
            if (! h.r.contains(p)) continue;
            act(h, e);
            return;
        }
    }

private:
    static constexpr int kTop = 40;        // top bar height
    static constexpr int kComposerH = 150; // new-thread composer
    static constexpr int kReplyBar = 112;  // reply bar in a thread

    enum class Act { Sort, New, Refresh, Back, Open, Up, PickFile, PickPlugin, Post, Cancel, Reply,
                     File, Plugin, ClearFile, ClearPlugin, Quote, Expand };
    struct Hit { juce::Rectangle<int> r; Act act; juce::String a, b; int n = 0; AttachRef ref; };

    ThemePalette host = kThemes[0];
    juce::Array<BoardThread> threads;
    juce::String openId, stagedMod, stagedModName, busyMsg;
    juce::Array<juce::File> staged;
    juce::TextEditor titleEd, bodyEd, replyEd;
    juce::Array<Hit> hits;
    std::map<juce::String, juce::Image> images;
    std::set<juce::String> pending, expanded;
    std::shared_ptr<juce::FileChooser> chooser;
    int sortMode = 0, scrollY = 0, contentH = 0;
    bool composerOpen = false, dragOver = false, busy = false;

    const BoardThread* findThread(const juce::String& id) const
    {
        for (const auto& t : threads) if (t.id == id) return &t;
        return nullptr;
    }

    juce::Rectangle<int> viewport() const
    {
        const bool list = openId.isEmpty();
        int top = kTop + (list && composerOpen ? kComposerH + 6 : 0);
        int bottom = getHeight() - (list ? 0 : kReplyBar);
        return { 0, top, getWidth(), juce::jmax(0, bottom - top) };
    }

    void clampScroll()
    {
        const int maxS = juce::jmax(0, contentH - viewport().getHeight());
        scrollY = juce::jlimit(0, maxS, scrollY);
    }

    static juce::String postNo(const juce::String& id)
    {
        const auto digits = id.retainCharacters("0123456789");
        return digits.isEmpty() ? id.substring(juce::jmax(0, id.length() - 6)) : digits.substring(juce::jmax(0, digits.length() - 7));
    }

    static juce::String ago(juce::int64 at)
    {
        if (at <= 0) return {};
        const auto s = (juce::Time::currentTimeMillis() - at) / 1000;
        if (s < 60) return "just now";
        if (s < 3600) return juce::String(s / 60) + "m ago";
        if (s < 86400) return juce::String(s / 3600) + "h ago";
        return juce::String(s / 86400) + "d ago";
    }

    static juce::Colour kindColour(const juce::String& k)
    {
        if (k == "image") return juce::Colour(0xff4fa3ff);
        if (k == "zip") return juce::Colour(0xffe0a020);
        if (k == "audio") return juce::Colour(0xff40d090);
        return juce::Colour(0xffa0a0b0);
    }

    // Image is fetched lazily the first time a post that shows it is painted.
    juce::Image imageFor(const AttachRef& r)
    {
        auto it = images.find(r.upload);
        if (it != images.end()) return it->second;
        if (fetchImage && pending.find(r.upload) == pending.end() && r.bytes <= 6 * 1024 * 1024)
        {
            pending.insert(r.upload);
            juce::Component::SafePointer<ThreadBoard> safe(this);
            const auto id = r.upload;
            fetchImage(r, [safe, id](juce::Image img) {
                if (safe == nullptr) return;
                safe->images[id] = img;
                safe->repaint();
            });
        }
        return {};
    }

    void addHit(juce::Rectangle<int> r, Act a, juce::String s1 = {}, juce::String s2 = {}, int n = 0, AttachRef ref = {})
    {
        Hit h; h.r = r; h.act = a; h.a = s1; h.b = s2; h.n = n; h.ref = ref;
        hits.add(h);
    }

    void drawBtn(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& txt, bool on, Act a, juce::String s1 = {}, int n = 0)
    {
        auto f = r.toFloat();
        g.setColour(on ? c(host.accent).withAlpha(0.32f) : c(host.panel).brighter(0.08f));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(on ? c(host.accent) : c(host.border));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setColour(c(host.text));
        g.setFont(dsFont(host, 10.5f, true));
        g.drawText(txt, r, juce::Justification::centred, true);
        addHit(r, a, s1, {}, n);
    }

    void paintTopBar(juce::Graphics& g, bool list)
    {
        auto bar = juce::Rectangle<int>(0, 0, getWidth(), kTop);
        g.setColour(c(host.bg));
        g.fillRect(bar);
        g.setColour(c(host.border).withAlpha(0.7f));
        g.drawLine(0.f, (float) kTop - 0.5f, (float) getWidth(), (float) kTop - 0.5f);
        auto r = bar.reduced(8, 6);
        if (list)
        {
            const char* names[] = { "NEW", "ACTIVE", "FILES" };
            for (int i = 0; i < 3; ++i)
            {
                drawBtn(g, r.removeFromLeft(70), names[i], sortMode == i, Act::Sort, {}, i);
                r.removeFromLeft(6);
            }
            drawBtn(g, r.removeFromRight(124), composerOpen ? "CLOSE POST BOX" : "+ NEW THREAD", composerOpen, Act::New);
            r.removeFromRight(6);
            drawBtn(g, r.removeFromRight(70), "REFRESH", false, Act::Refresh);
        }
        else
        {
            drawBtn(g, r.removeFromLeft(96), "< THREADS", false, Act::Back);
            r.removeFromLeft(10);
            drawBtn(g, r.removeFromRight(70), "REFRESH", false, Act::Refresh);
            if (auto* t = findThread(openId))
            {
                g.setColour(c(host.accent));
                g.setFont(dsFont(host, 13.f, true));
                g.drawText(t->title.isEmpty() ? juce::String("Thread") : t->title, r, juce::Justification::centredLeft, true);
            }
        }
    }

    void paintStagedRow(juce::Graphics& g, juce::Rectangle<int> row, bool reply)
    {
        // staged chips + plugin chip, left to right
        for (int i = 0; i < staged.size(); ++i)
        {
            auto chip = row.removeFromLeft(juce::jmin(150, juce::jmax(80, row.getWidth() / 3)));
            row.removeFromLeft(4);
            const auto kind = fileKindOf(staged[i].getFileName());
            g.setColour(kindColour(kind).withAlpha(0.2f));
            g.fillRoundedRectangle(chip.toFloat(), 6.f);
            g.setColour(kindColour(kind));
            g.drawRoundedRectangle(chip.toFloat(), 6.f, 1.f);
            g.setColour(c(host.text));
            g.setFont(dsFont(host, 10.f));
            g.drawText(kind.toUpperCase() + " " + staged[i].getFileName() + "  x", chip.reduced(6, 0), juce::Justification::centredLeft, true);
            addHit(chip, Act::ClearFile, {}, {}, i);
        }
        if (stagedMod.isNotEmpty())
        {
            auto chip = row.removeFromLeft(juce::jmin(160, juce::jmax(90, row.getWidth() / 2)));
            g.setColour(c(host.accent).withAlpha(0.22f));
            g.fillRoundedRectangle(chip.toFloat(), 6.f);
            g.setColour(c(host.accent));
            g.drawRoundedRectangle(chip.toFloat(), 6.f, 1.f);
            g.setFont(dsFont(host, 10.f, true));
            g.drawText("PLUGIN " + stagedModName + "  x", chip.reduced(6, 0), juce::Justification::centredLeft, true);
            addHit(chip, Act::ClearPlugin);
        }
        juce::ignoreUnused(reply);
    }

    void paintComposer(juce::Graphics& g)
    {
        auto r = juce::Rectangle<int>(8, kTop + 2, getWidth() - 16, kComposerH).toFloat();
        g.setColour(c(host.panel));
        g.fillRoundedRectangle(r, 10.f);
        g.setColour(c(host.accent).withAlpha(0.6f));
        g.drawRoundedRectangle(r, 10.f, 1.2f);
        auto row = juce::Rectangle<int>(16, kTop + 112, getWidth() - 32, 28);
        auto right = row.removeFromRight(190);
        drawBtn(g, right.removeFromRight(92), "POST", true, Act::Post);
        right.removeFromRight(6);
        drawBtn(g, right, "CANCEL", false, Act::Cancel);
        drawBtn(g, row.removeFromLeft(78), "+ FILE", false, Act::PickFile);
        row.removeFromLeft(6);
        drawBtn(g, row.removeFromLeft(92), "+ PLUGIN", false, Act::PickPlugin);
        row.removeFromLeft(8);
        paintStagedRow(g, row, false);
    }

    void paintReplyBar(juce::Graphics& g)
    {
        auto bar = juce::Rectangle<int>(0, getHeight() - kReplyBar, getWidth(), kReplyBar);
        g.setColour(c(host.panel));
        g.fillRect(bar);
        g.setColour(c(host.accent).withAlpha(0.6f));
        g.drawLine(0.f, (float) bar.getY() + 0.5f, (float) getWidth(), (float) bar.getY() + 0.5f);
        auto row = juce::Rectangle<int>(12, getHeight() - 44, getWidth() - 24, 28);
        auto right = row.removeFromRight(100);
        drawBtn(g, right, "REPLY", true, Act::Reply);
        drawBtn(g, row.removeFromLeft(78), "+ FILE", false, Act::PickFile);
        row.removeFromLeft(6);
        drawBtn(g, row.removeFromLeft(92), "+ PLUGIN", false, Act::PickPlugin);
        row.removeFromLeft(8);
        paintStagedRow(g, row, true);
    }

    // ---- shared helpers ----------------------------------------------------------
    static juce::AttributedString styledText(const juce::String& body, const ThemePalette& pal)
    {
        juce::AttributedString as;
        const auto lines = juce::StringArray::fromLines(body);
        for (int i = 0; i < lines.size(); ++i)
        {
            const auto& ln = lines[i];
            juce::Colour col = c(pal.text);
            if (ln.trimStart().startsWith(">>")) col = c(pal.accent);
            else if (ln.trimStart().startsWith(">")) col = juce::Colour(0xff789922); // greentext
            as.append(ln + (i + 1 < lines.size() ? "\n" : ""), dsFont(pal, 12.f), col);
        }
        as.setWordWrap(juce::AttributedString::byWord);
        return as;
    }

    static int textHeight(const juce::String& body, const ThemePalette& pal, float width)
    {
        if (body.isEmpty()) return 0;
        juce::TextLayout tl;
        tl.createLayout(styledText(body, pal), juce::jmax(40.f, width));
        return (int) std::ceil(tl.getHeight());
    }

    // Attachments of a post inside the thread view. Returns the height used.
    int paintAttachments(juce::Graphics* g, const ParsedPost& post, int x, int y, int w, const ThemePalette& pal)
    {
        int used = 0;
        for (const auto& f : post.files)
        {
            const auto kind = fileKindOf(f.name);
            if (kind == "image")
            {
                auto img = imageFor(f);
                const bool big = expanded.count(f.upload) > 0;
                const int maxW = big ? w : juce::jmin(w, 220);
                int iw = maxW, ih = 120;
                if (img.isValid())
                {
                    const float sc = juce::jmin(1.f, (float) maxW / (float) img.getWidth(), (big ? 700.f : 220.f) / (float) img.getHeight());
                    iw = juce::jmax(40, (int) (img.getWidth() * sc));
                    ih = juce::jmax(30, (int) (img.getHeight() * sc));
                }
                juce::Rectangle<int> r(x, y + used, iw, ih);
                if (g != nullptr)
                {
                    g->setColour(c(pal.bg));
                    g->fillRoundedRectangle(r.toFloat(), 6.f);
                    if (img.isValid()) g->drawImage(img, r.toFloat(), juce::RectanglePlacement::centred);
                    else
                    {
                        g->setColour(c(pal.muted));
                        g->setFont(dsFont(pal, 11.f));
                        g->drawText(pending.count(f.upload) ? "loading image..." : "image", r, juce::Justification::centred);
                    }
                    g->setColour(c(pal.border));
                    g->drawRoundedRectangle(r.toFloat(), 6.f, 1.f);
                    addHit(r, Act::Expand, f.upload);
                }
                used += ih + 4;
                auto cap = juce::Rectangle<int>(x, y + used, w, 16);
                if (g != nullptr)
                {
                    g->setColour(c(pal.muted));
                    g->setFont(dsFont(pal, 10.f));
                    g->drawText(f.name + "  (" + humanBytes(f.bytes) + ")  click image to expand  -  ", cap.withWidth(juce::jmin(w, 360)), juce::Justification::centredLeft, true);
                    auto save = juce::Rectangle<int>(x + juce::jmin(w, 360) - 60, y + used, 50, 16);
                    g->setColour(c(pal.accent));
                    g->drawText("SAVE", save, juce::Justification::centredLeft);
                    addHit(save, Act::File, {}, {}, 0, f);
                }
                used += 20;
            }
            else
            {
                juce::Rectangle<int> r(x, y + used, juce::jmin(w, 420), 30);
                if (g != nullptr)
                {
                    const auto col = kindColour(kind);
                    g->setColour(col.withAlpha(0.16f));
                    g->fillRoundedRectangle(r.toFloat(), 7.f);
                    g->setColour(col);
                    g->drawRoundedRectangle(r.toFloat(), 7.f, 1.f);
                    auto in = r.reduced(9, 0);
                    g->setFont(dsFont(pal, 9.f, true));
                    g->drawText(kind == "zip" ? "ZIP" : kind == "audio" ? "AUDIO" : "FILE", in.removeFromLeft(42), juce::Justification::centredLeft);
                    g->setColour(c(pal.text));
                    g->setFont(dsFont(pal, 11.f, true));
                    auto rt = in.removeFromRight(juce::jmin(150, in.getWidth() / 2));
                    g->drawText(f.name, in, juce::Justification::centredLeft, true);
                    g->setColour(c(pal.muted));
                    g->setFont(dsFont(pal, 10.f));
                    g->drawText(humanBytes(f.bytes) + (kind == "audio" ? "  -  PLAY/SAVE" : "  -  SAVE"), rt, juce::Justification::centredRight, true);
                    addHit(r, Act::File, {}, {}, 0, f);
                }
                used += 36;
            }
        }
        if (post.modId.isNotEmpty())
        {
            juce::Rectangle<int> r(x, y + used, juce::jmin(w, 420), 34);
            if (g != nullptr)
            {
                g->setColour(c(pal.accent).withAlpha(0.2f));
                g->fillRoundedRectangle(r.toFloat(), 7.f);
                g->setColour(c(pal.accent));
                g->drawRoundedRectangle(r.toFloat(), 7.f, 1.4f);
                auto in = r.reduced(10, 0);
                g->setFont(dsFont(pal, 9.f, true));
                g->drawText("PLUGIN", in.removeFromLeft(52), juce::Justification::centredLeft);
                g->setColour(c(pal.text));
                g->setFont(dsFont(pal, 12.f, true));
                auto rt = in.removeFromRight(96);
                g->drawText(post.modName, in, juce::Justification::centredLeft, true);
                g->setColour(c(pal.accent));
                g->setFont(dsFont(pal, 10.f, true));
                g->drawText("OPEN IN BUILDER", rt, juce::Justification::centredRight);
                addHit(r, Act::Plugin, post.modId, post.modName);
            }
            used += 40;
        }
        return used;
    }

    // One numbered post (OP or reply). Returns its height.
    int paintPost(juce::Graphics* g, const juce::String& id, const juce::String& user, const juce::String& themeId,
                  const juce::String& title, const juce::String& rawText, juce::int64 at, bool op,
                  int y, int w, int scroll)
    {
        const auto pal = themeById(themeId.isEmpty() ? juce::String("trippah") : themeId);
        const auto post = parsePost(rawText);
        const int x = 8, inner = w - 16 - 24;
        const int th = textHeight(post.text, pal, (float) inner);
        // measure attachments with a null graphics pass
        const int ah = paintAttachments(nullptr, post, 0, 0, inner, pal);
        const int headH = 22 + (op && title.isNotEmpty() ? 22 : 0);
        const int h = 10 + headH + (ah > 0 ? ah + 4 : 0) + (th > 0 ? th + 4 : 0) + 12;
        if (g != nullptr)
        {
            const int sy = y - scroll;
            if (sy + h >= viewport().getY() && sy <= viewport().getBottom())
            {
                auto card = juce::Rectangle<int>(x, sy, w - 16, h).toFloat();
                g->setColour(c(pal.panel));
                g->fillRoundedRectangle(card, 8.f);
                g->setColour(c(pal.accent).withAlpha(op ? 0.9f : 0.5f));
                g->fillRoundedRectangle(card.getX(), card.getY(), 4.f, card.getHeight(), 2.f);
                g->setColour(c(pal.accent).withAlpha(0.35f));
                g->drawRoundedRectangle(card, 8.f, 1.f);
                int cy = sy + 8;
                if (op && title.isNotEmpty())
                {
                    g->setColour(c(pal.accent));
                    g->setFont(dsFont(pal, 14.f, true));
                    g->drawText(title, x + 14, cy, inner, 22, juce::Justification::centredLeft, true);
                    cy += 22;
                }
                const auto no = postNo(id);
                g->setFont(dsFont(pal, 10.5f, true));
                g->setColour(juce::Colour(0xff4fa86f));
                const int uw = juce::jmin(inner / 2, juce::GlyphArrangement::getStringWidthInt(dsFont(pal, 10.5f, true), user) + 10);
                g->drawText(user, x + 14, cy, uw, 18, juce::Justification::centredLeft, true);
                g->setColour(c(pal.muted));
                g->setFont(dsFont(pal, 10.f));
                g->drawText(ago(at) + "  " + juce::String(pal.name), x + 14 + uw, cy, inner / 3, 18, juce::Justification::centredLeft, true);
                auto noR = juce::Rectangle<int>(x + w - 16 - 120, cy, 108, 18);
                g->setColour(c(pal.accent));
                g->setFont(dsFont(pal, 10.5f, true));
                g->drawText("No." + no, noR, juce::Justification::centredRight);
                addHit(noR, Act::Quote, no);
                cy += 22;
                if (ah > 0) { paintAttachments(g, post, x + 14, cy, inner, pal); cy += ah + 4; }
                if (th > 0)
                {
                    styledText(post.text, pal).draw(*g, juce::Rectangle<float>((float) x + 14.f, (float) cy, (float) inner, (float) th + 4.f));
                }
            }
        }
        return h;
    }

    int paintThread(juce::Graphics* g, juce::Rectangle<int> view)
    {
        auto* t = findThread(openId);
        if (t == nullptr) return 0;
        const int w = view.getWidth();
        int cy = 6;
        cy += paintPost(g, t->id, t->user, t->themeId, t->title, t->text, t->at, true, view.getY() + cy, w, scrollY) + 8;
        for (const auto& cm : t->comments)
            cy += paintPost(g, cm.id, cm.user, cm.themeId, juce::String(), cm.text, (juce::int64) 0, false, view.getY() + cy, w, scrollY) + 8;
        return cy + 8;
    }

    int paintList(juce::Graphics* g, juce::Rectangle<int> view)
    {
        juce::Array<const BoardThread*> rows;
        for (const auto& t : threads)
        {
            if (sortMode == 2)
            {
                const auto p = parsePost(t.text);
                if (p.files.isEmpty() && p.modId.isEmpty()) continue;
            }
            rows.add(&t);
        }
        std::stable_sort(rows.begin(), rows.end(), [this](const BoardThread* a, const BoardThread* b) {
            if (sortMode == 1) return (a->score + a->comments.size() * 2) > (b->score + b->comments.size() * 2);
            return a->at > b->at;
        });
        if (rows.isEmpty())
        {
            if (g != nullptr)
            {
                g->setColour(c(host.muted));
                g->setFont(dsFont(host, 13.f));
                g->drawFittedText(threads.isEmpty() ? "No threads yet. Press + NEW THREAD, or drop an image, .zip, audio file or plugin anywhere on this board."
                                                    : "Nothing with files or plugins yet.",
                                  view.reduced(30, 20), juce::Justification::centredTop, 4);
            }
            return 0;
        }
        const int rowH = (int) std::ceil(104.f + 8.f * dsScale());
        int cy = 6;
        for (auto* t : rows)
        {
            const int sy = view.getY() + cy - scrollY;
            if (g != nullptr && sy + rowH >= view.getY() && sy <= view.getBottom())
                paintRow(*g, *t, juce::Rectangle<int>(8, sy, view.getWidth() - 16, rowH - 6));
            cy += rowH;
        }
        return cy + 6;
    }

    void paintRow(juce::Graphics& g, const BoardThread& t, juce::Rectangle<int> r)
    {
        const auto pal = themeById(t.themeId.isEmpty() ? juce::String("trippah") : t.themeId);
        const auto post = parsePost(t.text);
        g.setColour(c(pal.panel));
        g.fillRoundedRectangle(r.toFloat(), 9.f);
        g.setColour(c(pal.accent).withAlpha(0.4f));
        g.drawRoundedRectangle(r.toFloat(), 9.f, 1.f);
        g.setColour(c(pal.accent).withAlpha(0.9f));
        g.fillRoundedRectangle((float) r.getX(), (float) r.getY(), 4.f, (float) r.getHeight(), 2.f);
        auto in = r.reduced(10, 8);
        // vote column
        auto vote = in.removeFromLeft(40);
        g.setColour(c(pal.accent));
        juce::Path tri;
        tri.addTriangle((float) vote.getCentreX(), (float) vote.getY() + 2.f, (float) vote.getCentreX() - 10.f, (float) vote.getY() + 18.f, (float) vote.getCentreX() + 10.f, (float) vote.getY() + 18.f);
        g.fillPath(tri);
        g.setFont(dsFont(pal, 13.f, true));
        g.drawText(juce::String(t.score), vote.withTrimmedTop(20).removeFromTop(20), juce::Justification::centred);
        g.setColour(c(pal.muted));
        g.setFont(dsFont(pal, 9.f));
        g.drawText(juce::String(t.comments.size()) + " re", vote.withTrimmedTop(40).removeFromTop(14), juce::Justification::centred);
        addHit(vote, Act::Up, t.id);
        in.removeFromLeft(6);
        // thumbnail
        const int th = in.getHeight();
        if (! post.files.isEmpty() || post.modId.isNotEmpty())
        {
            auto thumb = in.removeFromLeft(th);
            in.removeFromLeft(8);
            const AttachRef* firstImg = nullptr;
            for (const auto& f : post.files) if (fileKindOf(f.name) == "image") { firstImg = &f; break; }
            g.setColour(c(pal.bg));
            g.fillRoundedRectangle(thumb.toFloat(), 6.f);
            juce::Image img = firstImg != nullptr ? imageFor(*firstImg) : juce::Image();
            if (img.isValid()) g.drawImage(img, thumb.toFloat().reduced(1.f), juce::RectanglePlacement::centred);
            else
            {
                const juce::String kind = post.modId.isNotEmpty() ? "plugin" : fileKindOf(post.files[0].name);
                const auto col = kind == "plugin" ? c(pal.accent) : kindColour(kind);
                g.setColour(col.withAlpha(0.25f));
                g.fillRoundedRectangle(thumb.toFloat().reduced(6.f), 6.f);
                g.setColour(col);
                g.setFont(dsFont(pal, 12.f, true));
                g.drawText(kind.toUpperCase(), thumb, juce::Justification::centred);
            }
            g.setColour(c(pal.border));
            g.drawRoundedRectangle(thumb.toFloat(), 6.f, 1.f);
        }
        // badges (right)
        int nImg = 0, nZip = 0, nAud = 0, nOther = 0;
        for (const auto& f : post.files)
        {
            const auto k = fileKindOf(f.name);
            if (k == "image") ++nImg; else if (k == "zip") ++nZip; else if (k == "audio") ++nAud; else ++nOther;
        }
        juce::StringArray badges;
        if (nImg) badges.add("IMG x" + juce::String(nImg));
        if (nZip) badges.add("ZIP x" + juce::String(nZip));
        if (nAud) badges.add("AUDIO x" + juce::String(nAud));
        if (nOther) badges.add("FILE x" + juce::String(nOther));
        if (post.modId.isNotEmpty()) badges.add("PLUGIN");
        g.setColour(c(pal.accent).withAlpha(0.9f));
        g.setFont(dsFont(pal, 9.f, true));
        g.drawText(badges.joinIntoString("  "), in.removeFromTop(14), juce::Justification::centredRight, true);
        in.expand(0, 0);
        g.setColour(c(pal.accent));
        g.setFont(dsFont(pal, 14.f, true));
        g.drawText(t.title.isEmpty() ? juce::String("Thread") : t.title, in.removeFromTop((int) (20.f * dsScale())), juce::Justification::centredLeft, true);
        g.setColour(c(pal.muted));
        g.setFont(dsFont(pal, 10.f));
        g.drawText("No." + postNo(t.id) + "  by " + t.user + "  -  " + ago(t.at) + "  -  " + pal.name, in.removeFromTop((int) (15.f * dsScale())), juce::Justification::centredLeft, true);
        if (post.text.isNotEmpty())
        {
            g.saveState();
            g.reduceClipRegion(in);
            styledText(post.text, pal).draw(g, in.toFloat().withHeight(200.f));
            g.restoreState();
        }
        addHit(r, Act::Open, t.id);
    }

    // ---- actions ---------------------------------------------------------------------
    void pickFiles()
    {
        chooser = std::make_shared<juce::FileChooser>("Attach files (images, .zip, audio)", juce::File(), "*");
        juce::Component::SafePointer<ThreadBoard> safe(this);
        auto keep = chooser;
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
            [safe, keep](const juce::FileChooser& fc) {
                if (safe == nullptr) return;
                juce::Array<juce::File> files;
                for (const auto& f : fc.getResults()) files.add(f);
                safe->stageFiles(files);
            });
    }

    void act(const Hit& h, const juce::MouseEvent& e)
    {
        switch (h.act)
        {
            case Act::Sort: sortMode = h.n; scrollY = 0; repaint(); break;
            case Act::New: composerOpen = ! composerOpen; resized(); repaint(); break;
            case Act::Refresh: if (onRefresh) onRefresh(); break;
            case Act::Back: closeThread(); break;
            case Act::Open: openThreadById(h.a); break;
            case Act::Up:
                for (auto& t : threads) if (t.id == h.a) ++t.score;
                if (onUpvote) onUpvote(h.a);
                repaint();
                break;
            case Act::PickFile: pickFiles(); break;
            case Act::PickPlugin:
                if (onPickPlugin)
                {
                    juce::Component::SafePointer<ThreadBoard> safe(this);
                    onPickPlugin([safe](juce::String id, juce::String name) {
                        if (safe == nullptr) return;
                        safe->stagedMod = id; safe->stagedModName = name;
                        if (safe->openId.isEmpty()) safe->composerOpen = true;
                        safe->resized(); safe->repaint();
                    });
                }
                break;
            case Act::ClearFile: if (h.n >= 0 && h.n < staged.size()) staged.remove(h.n); repaint(); break;
            case Act::ClearPlugin: stagedMod = {}; stagedModName = {}; repaint(); break;
            case Act::Cancel: clearComposer(); break;
            case Act::Post:
            {
                const auto title = titleEd.getText().trim();
                if (title.isEmpty() && bodyEd.getText().trim().isEmpty() && staged.isEmpty() && stagedMod.isEmpty())
                { if (onStatus) onStatus("Write a title or attach something first"); break; }
                if (onPost) onPost(title, bodyEd.getText().trim(), staged, stagedMod.isEmpty() ? juce::String() : modToken(stagedMod, stagedModName));
                break;
            }
            case Act::Reply:
            {
                if (replyEd.getText().trim().isEmpty() && staged.isEmpty() && stagedMod.isEmpty())
                { if (onStatus) onStatus("Write a reply or attach something first"); break; }
                if (onReply) onReply(openId, replyEd.getText().trim(), staged, stagedMod.isEmpty() ? juce::String() : modToken(stagedMod, stagedModName));
                break;
            }
            case Act::File: if (onFileClick) onFileClick(h.ref, e.getScreenPosition()); break;
            case Act::Plugin: if (onOpenPlugin) onOpenPlugin(h.a, h.b); break;
            case Act::Quote:
                if (openId.isNotEmpty())
                {
                    replyEd.moveCaretToEnd();
                    replyEd.insertTextAtCaret(">>" + h.a + "\n");
                    replyEd.grabKeyboardFocus();
                }
                break;
            case Act::Expand:
                if (expanded.count(h.a)) expanded.erase(h.a); else expanded.insert(h.a);
                repaint();
                break;
        }
    }
};
} // namespace kt
