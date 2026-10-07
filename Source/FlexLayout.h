#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Flexible row layout helpers for auto-adjusting toolbars.
 * Components are distributed across the available width, shrinking or
 * growing to fit — no hardcoded pixel widths that overflow on narrow windows.
 */
namespace flx
{
    /** A single flex item: component + relative weight + minimum width. */
    struct Item
    {
        juce::Component* comp = nullptr;
        float flex = 1.0f;
        int minWidth = 50;
        bool fixed = false;   // if true, width = minWidth (no flex grow)
    };

    /**
     * Lay out a row of items inside `area`.
     * Fixed-width items get exactly minWidth; flex items share the remaining
     * space proportionally to their flex weights. Gaps are inserted between items.
     */
    inline void row(juce::Rectangle<int> area, int gap, const juce::Array<Item>& items)
    {
        const int n = items.size();
        if (n == 0) return;

        int fixedTotal = 0;
        float flexTotal = 0.f;
        for (const auto& it : items)
        {
            fixedTotal += it.minWidth;
            if (it.fixed)
                flexTotal += 0.f;
            else
                flexTotal += it.flex;
        }
        const int totalGaps = (n - 1) * gap;
        const int available = juce::jmax(0, area.getWidth() - totalGaps);

        int flexBudget = available - fixedTotal;
        if (flexBudget < 0) flexBudget = 0;

        int x = area.getX();
        const int y = area.getY();
        const int h = area.getHeight();

        for (const auto& it : items)
        {
            int w;
            if (it.fixed || flexTotal <= 0.f)
                w = it.minWidth;
            else
                w = juce::jmax(it.minWidth, (int) juce::roundToInt(flexBudget * it.flex / flexTotal));

            if (it.comp)
                it.comp->setBounds(x, y, w, h);
            x += w + gap;
        }
    }

    /** Convenience overload with an initializer list. */
    inline void row(juce::Rectangle<int> area, int gap, std::initializer_list<Item> items)
    {
        juce::Array<Item> arr;
        for (const auto& it : items) arr.add(it);
        row(area, gap, arr);
    }

    /** Spacer item (invisible, fixed width). */
    inline Item spacer(int width) { return Item { nullptr, 0.f, width, true }; }
}
