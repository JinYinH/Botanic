#include "MainView.h"

namespace ui
{

using namespace botanic;

namespace
{
    const juce::Colour panelDivider { 0x14000000 };

    juce::Rectangle<float> refRect (float x, float y, float w, float h) { return { x, y, w, h }; }

    void drawLeafMark (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto c = area.getCentre();

        juce::Path big;
        big.startNewSubPath (c.x + area.getWidth() * 0.06f, area.getBottom() - area.getHeight() * 0.06f);
        big.cubicTo (c.x + area.getWidth() * 0.55f, area.getBottom() - area.getHeight() * 0.35f,
                     c.x + area.getWidth() * 0.52f, area.getY() + area.getHeight() * 0.10f,
                     c.x + area.getWidth() * 0.16f, area.getY());
        big.cubicTo (c.x - area.getWidth() * 0.18f, area.getY() + area.getHeight() * 0.30f,
                     c.x - area.getWidth() * 0.05f, area.getBottom() - area.getHeight() * 0.30f,
                     c.x + area.getWidth() * 0.06f, area.getBottom() - area.getHeight() * 0.06f);
        big.closeSubPath();

        juce::ColourGradient gradient (th::greenMid, area.getX(), area.getY(),
                                       th::greenDeep, area.getRight(), area.getBottom(), false);
        gradient.addColour (0.5, th::green);
        g.setGradientFill (gradient);
        g.fillPath (big);

        juce::Path small;
        small.startNewSubPath (c.x - area.getWidth() * 0.02f, area.getBottom() - area.getHeight() * 0.14f);
        small.cubicTo (area.getX() + area.getWidth() * 0.05f, c.y - area.getHeight() * 0.10f,
                       area.getX() + area.getWidth() * 0.02f, c.y + area.getHeight() * 0.15f,
                       area.getX() + area.getWidth() * 0.30f, c.y + area.getHeight() * 0.10f);
        small.cubicTo (area.getX() + area.getWidth() * 0.10f, c.y + area.getHeight() * 0.30f,
                       c.x - area.getWidth() * 0.12f, area.getBottom() - area.getHeight() * 0.22f,
                       c.x - area.getWidth() * 0.02f, area.getBottom() - area.getHeight() * 0.14f);
        small.closeSubPath();
        g.setColour (th::greenLight.withAlpha (0.95f));
        g.fillPath (small);

        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (c.x + area.getWidth() * 0.04f, area.getBottom() - area.getHeight() * 0.12f,
                    c.x + area.getWidth() * 0.20f, area.getY() + area.getHeight() * 0.22f, 1.0f);
    }
}

//==============================================================================
PresetBar::PresetBar (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, false);
    refresh();
}

void PresetBar::refresh()
{
    const auto n = proc.getPresetName();
    const auto idx = proc.getPresetIndex();

    if (n != name || idx != presetIndex)
    {
        name = n;
        presetIndex = idx;
        repaint();
    }
}

void PresetBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);

    const auto iconSize = 18.0f;

    auto drawArrow = [&] (float x, bool forward, bool hover)
    {
        juce::Colour c = hover ? th::greenDark : th::ink;
        th::drawIcon (g, forward ? th::Icon::next : th::Icon::prev,
                      { x - iconSize * 0.5f, r.getCentreY() - iconSize * 0.5f, iconSize, iconSize }, c, 1.4f);
    };

    drawArrow (70.0f, false, hovered == 0);
    drawArrow (306.0f, true, hovered == 1);

    g.setFont (th::font (14.0f, 1));
    g.setColour (th::ink);
    g.drawText (name, r.withTrimmedLeft (96.0f).withTrimmedRight (140.0f), juce::Justification::centred, false);

    const std::array<th::Icon, 4> icons { th::Icon::folder, th::Icon::save, th::Icon::undo, th::Icon::redo };
    const std::array<float, 4> xs { 356.0f, 398.0f, 440.0f, 478.0f };

    for (size_t i = 0; i < xs.size(); ++i)
    {
        const auto hover = hovered == (int) i + 2;
        th::drawIcon (g, icons[i], { xs[i] - 9.0f, r.getCentreY() - 9.0f, 18.0f, 18.0f },
                      hover ? th::greenDark : th::inkSoft, 1.5f);
    }
}

void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    const auto x = e.position.x;

    if (x < 100.0f)
    {
        proc.previousPreset();
    }
    else if (x > 290.0f && x < 320.0f)
    {
        proc.nextPreset();
    }
    else if (x > 336.0f && x < 376.0f)
    {
        if (onBrowse != nullptr)
            onBrowse();
        else
            proc.refreshPresetList();
    }
    else if (x >= 376.0f && x < 418.0f)
    {
        if (onSaveRequested != nullptr)
            onSaveRequested();
    }
    else if (x >= 418.0f && x < 458.0f)
    {
        proc.undo();
    }
    else if (x >= 458.0f && x < 500.0f)
    {
        proc.redo();
    }
    else
    {
        if (onBrowse != nullptr)
            onBrowse();
    }

    refresh();
}

void PresetBar::mouseMove (const juce::MouseEvent& e)
{
    const auto x = e.position.x;
    int h = -1;

    if (x < 100.0f) h = 0;
    else if (x > 290.0f && x < 320.0f) h = 1;
    else if (x > 336.0f && x < 376.0f) h = 2;
    else if (x >= 376.0f && x < 418.0f) h = 3;
    else if (x >= 418.0f && x < 458.0f) h = 4;
    else if (x >= 458.0f && x < 500.0f) h = 5;

    if (h != hovered)
    {
        hovered = h;
        repaint();
    }
}

void PresetBar::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

//==============================================================================
PianoKeyboard::PianoKeyboard (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, false);
}

std::vector<PianoKeyboard::KeyRect> PianoKeyboard::buildKeys() const
{
    std::vector<KeyRect> keys;
    const auto r = getLocalBounds().toFloat();
    const auto whiteCount = [&]
    {
        int count = 0;
        for (int n = lowNote; n <= highNote; ++n)
        {
            const auto pc = n % 12;
            if (pc != 1 && pc != 3 && pc != 6 && pc != 8 && pc != 10)
                ++count;
        }
        return count;
    }();

    const auto whiteW = r.getWidth() / (float) juce::jmax (1, whiteCount);
    const auto blackW = whiteW * 0.62f;
    const auto blackH = r.getHeight() * 0.62f;

    float x = r.getX();
    std::vector<std::pair<int, float>> whitePositions;

    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto pc = n % 12;
        const auto isBlack = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);

        if (! isBlack)
        {
            keys.push_back ({ n, { x, r.getY(), whiteW, r.getHeight() }, false });
            whitePositions.push_back ({ n, x });
            x += whiteW;
        }
    }

    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto pc = n % 12;
        const auto isBlack = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);

        if (! isBlack)
            continue;

        // place black key between the previous and next white key
        for (size_t i = 0; i + 1 < whitePositions.size(); ++i)
        {
            const auto prevNote = whitePositions[i].first;
            if (prevNote + 1 == n)
            {
                const auto centre = whitePositions[i].second + whiteW;
                keys.push_back ({ n, { centre - blackW * 0.5f, r.getY(), blackW, blackH }, true });
                break;
            }
        }
    }

    return keys;
}

int PianoKeyboard::noteAt (juce::Point<float> p) const
{
    const auto keys = buildKeys();

    for (const auto& k : keys)
        if (k.black && k.bounds.contains (p))
            return k.note;

    for (const auto& k : keys)
        if (! k.black && k.bounds.contains (p))
            return k.note;

    return -1;
}

void PianoKeyboard::setHeldNotes (const std::array<juce::uint64, 2>& mask)
{
    for (int n = 0; n < 128; ++n)
        active[(size_t) n] = (mask[(size_t) (n / 64)] & ((juce::uint64) 1 << (n % 64))) != 0;
}

void PianoKeyboard::tick (double deltaSeconds)
{
    const auto dt = (float) juce::jlimit (0.0, 0.1, deltaSeconds);
    bool needsRepaint = false;

    for (int n = 0; n < 128; ++n)
    {
        const auto target = active[(size_t) n] ? 1.0f : 0.0f;
        auto& gv = glow[(size_t) n];

        if (std::abs (gv - target) < 0.002f)
        {
            if (gv != target)
            {
                gv = target;
                needsRepaint = true;
            }

            continue;
        }

        const auto speed = target > gv ? 28.0f : 7.5f;
        gv += (target - gv) * juce::jmin (1.0f, dt * speed);
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

void PianoKeyboard::paint (juce::Graphics& g)
{
    const auto keys = buildKeys();
    const auto r = getLocalBounds().toFloat();

    // white keys
    for (const auto& k : keys)
    {
        if (k.black)
            continue;

        auto kr = k.bounds.reduced (0.6f, 0.0f);
        const auto on = glow[(size_t) k.note];

        juce::ColourGradient grad (juce::Colour (0xffffffff), kr.getX(), kr.getY(),
                                   juce::Colour (0xffdddfd5), kr.getX(), kr.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (kr, 3.0f);

        if (on > 0.01f)
        {
            g.setColour (th::greenMid.withAlpha (0.92f * on));
            g.fillRoundedRectangle (kr, 3.0f);

            g.setColour (th::greenLight.withAlpha (0.5f * on));
            g.drawRoundedRectangle (kr.reduced (0.6f), 3.0f, 1.4f);
        }

        g.setColour (th::ink.withAlpha (0.30f));
        g.drawRoundedRectangle (kr.reduced (0.4f), 3.0f, 1.0f);

        g.setColour (th::ink.withAlpha (0.13f));
        g.fillRect (kr.getRight() - 1.6f, kr.getY() + 2.0f, 1.6f, kr.getHeight() - 4.0f);

        g.setColour (th::ink.withAlpha (0.10f));
        g.fillRect (kr.getX(), kr.getY(), kr.getWidth(), 5.0f);
    }

    // black keys
    for (const auto& k : keys)
    {
        if (! k.black)
            continue;

        const auto on = glow[(size_t) k.note];
        auto kr = k.bounds;

        juce::DropShadow ds (juce::Colours::black.withAlpha (0.35f), 5, { 0, 3 });
        juce::Path p;
        p.addRoundedRectangle (kr, 3.0f);
        ds.drawForPath (g, p);

        const auto lit = juce::jlimit (0.0f, 1.0f, on);
        juce::ColourGradient grad (juce::Colour (0xff2b2f27).interpolatedWith (th::greenDark, lit),
                                   kr.getX(), kr.getY(),
                                   juce::Colour (0xff11140f).interpolatedWith (th::greenDeep, lit),
                                   kr.getX(), kr.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (kr, 3.0f);

        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (kr.getX() + 2.0f, kr.getY() + 4.0f, kr.getRight() - 2.0f, kr.getY() + 4.0f, 1.0f);
    }

    // octave markers
    g.setColour (th::ink.withAlpha (0.35f));
    g.setFont (th::font (8.0f, 1));

    for (const auto& k : keys)
    {
        if (! k.black && k.note % 12 == 0)
        {
            auto labelArea = k.bounds.withTop (k.bounds.getBottom() - 18.0f).reduced (2.0f, 0.0f);
            g.drawText ("C" + juce::String (k.note / 12 - 1), labelArea, juce::Justification::centred, false);
        }
    }

    juce::ignoreUnused (r);
}

void PianoKeyboard::mouseDown (const juce::MouseEvent& e)
{
    const auto note = noteAt (e.position);

    if (note >= 0)
    {
        mouseNote = note;
        active[(size_t) note] = true;
        glow[(size_t) note] = juce::jmax (glow[(size_t) note], 0.35f);
        const auto velocity = juce::jlimit (0.15f, 1.0f, 1.0f - e.position.y / (float) getHeight() * 0.7f);
        proc.keyboardNoteOn (note, velocity);
        repaint();
    }
}

void PianoKeyboard::mouseDrag (const juce::MouseEvent& e)
{
    const auto note = noteAt (e.position);

    if (note != mouseNote)
    {
        if (mouseNote >= 0)
            proc.keyboardNoteOff (mouseNote);

        mouseNote = note;

        if (note >= 0)
            proc.keyboardNoteOn (note, juce::jlimit (0.15f, 1.0f, 1.0f - e.position.y / (float) getHeight() * 0.7f));

        repaint();
    }
}

void PianoKeyboard::mouseUp (const juce::MouseEvent&)
{
    if (mouseNote >= 0)
    {
        active[(size_t) mouseNote] = false;
        proc.keyboardNoteOff (mouseNote);
        mouseNote = -1;
        repaint();
    }
}

void PianoKeyboard::mouseExit (const juce::MouseEvent&)
{
    if (mouseNote >= 0)
    {
        active[(size_t) mouseNote] = false;
        proc.keyboardNoteOff (mouseNote);
        mouseNote = -1;
        repaint();
    }
}

void PianoKeyboard::handleKey (int keyCode, bool isDown)
{
    if (! computerKeys)
        return;

    static const int map[] = { 'a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k', 'o', 'l', 'p', ';' };

    for (int i = 0; i < (int) numElementsInArray (map); ++i)
    {
        if (keyCode == juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) map[i]))
        {
            const auto note = 60 + i;
            if (isDown)
                proc.keyboardNoteOn (note, 0.8f);
            else
                proc.keyboardNoteOff (note);
            return;
        }
    }
}

//==============================================================================
FxChainPanel::FxChainPanel (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, false);
    refresh();
}

void FxChainPanel::refresh()
{
    bool changed = false;

    for (int i = 0; i < 5; ++i)
    {
        const auto e = proc.isFxOn (i);
        if (e != enabled[(size_t) i])
        {
            enabled[(size_t) i] = e;
            changed = true;
        }
    }

    const auto o = proc.getFxOrder();

    for (int i = 0; i < 5; ++i)
        if (o[(size_t) i] != order[(size_t) i])
        {
            order = o;
            changed = true;
        }

    if (changed)
        repaint();
}

juce::Rectangle<float> FxChainPanel::rowBounds (int row) const
{
    return { 34.0f, 30.0f + 41.0f * (float) row, 224.0f, 34.0f };
}

juce::Point<float> FxChainPanel::dotPosition (int row, int column) const
{
    return { 282.0f + 15.0f * (float) column, 47.0f + 41.0f * (float) row };
}

void FxChainPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // list background
    auto list = juce::Rectangle<float> (22.0f, 24.0f, 252.0f, 218.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillRoundedRectangle (list, 14.0f);
    g.setColour (th::ink.withAlpha (0.06f));
    g.drawRoundedRectangle (list.reduced (0.5f), 14.0f, 1.0f);

    const auto names = Params::fxSlotNames();

    for (int row = 0; row < 5; ++row)
    {
        const auto slot = order[(size_t) row];
        auto rb = rowBounds (row);

        if (row == hoverRow)
        {
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.fillRoundedRectangle (rb.reduced (4.0f, 2.0f), 8.0f);
        }

        // radio
        const auto centre = juce::Point<float> (rb.getX() + 16.0f, rb.getCentreY());

        if (enabled[(size_t) slot])
        {
            g.setColour (th::greenMid);
            g.fillEllipse (centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f);
            g.setColour (th::greenDeep.withAlpha (0.6f));
            g.drawEllipse (centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f, 1.2f);
        }
        else
        {
            g.setColour (th::ink.withAlpha (0.25f));
            g.drawEllipse (centre.x - 5.5f, centre.y - 5.5f, 11.0f, 11.0f, 1.4f);
        }

        g.setFont (th::font (13.0f, enabled[(size_t) slot] ? 2 : 1));
        g.setColour (enabled[(size_t) slot] ? th::ink : th::ink.withAlpha (0.45f));
        g.drawText (T (names[slot].toRawUTF8()), rb.withTrimmedLeft (34.0f), juce::Justification::centredLeft, false);

        // connecting line
        g.setColour (th::ink.withAlpha (0.10f));
        g.drawLine (rb.getRight() - 12.0f, rb.getCentreY(), dotPosition (row, 0).x - 8.0f, rb.getCentreY(), 1.0f);
    }

    // routing grid
    for (int row = 0; row < 5; ++row)
    {
        const auto slot = order[(size_t) row];

        for (int col = 0; col < 5; ++col)
        {
            const auto p = dotPosition (row, col);
            const auto isCurrent = proc.getFxOrder()[(size_t) col] == slot;

            if (isCurrent)
            {
                g.setColour (th::greenMid);
                g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
                g.setColour (th::greenDeep.withAlpha (0.5f));
                g.drawEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f, 1.0f);
            }
            else
            {
                g.setColour (th::ink.withAlpha (0.14f));
                g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7.0f, 7.0f);
            }
        }
    }

    juce::ignoreUnused (panelDivider, r);
}

void FxChainPanel::mouseDown (const juce::MouseEvent& e)
{
    for (int row = 0; row < 5; ++row)
    {
        if (rowBounds (row).contains (e.position))
        {
            const auto slot = order[(size_t) row];

            if (e.position.x < 60.0f)
            {
                proc.setFxOn (slot, ! enabled[(size_t) slot]);
                refresh();
            }
            else
            {
                dragRow = row;
                dragStart = e.position;
            }

            return;
        }

        for (int col = 0; col < 5; ++col)
        {
            if (dotPosition (row, col).getDistanceFrom (e.position) < 9.0f)
            {
                const auto slot = order[(size_t) row];
                auto current = proc.getFxOrder();
                int from = 0;

                for (int i = 0; i < 5; ++i)
                    if (current[(size_t) i] == slot)
                        from = i;

                proc.moveFxSlot (from, col);
                proc.takeUndoSnapshot();
                refresh();
                return;
            }
        }
    }
}

void FxChainPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragRow < 0)
        return;

    if (e.position.y - dragStart.y > 22.0f && dragRow < 4)
    {
        proc.moveFxSlot (dragRow, dragRow + 1);
        proc.takeUndoSnapshot();
        ++dragRow;
        dragStart = e.position;
        refresh();
    }
    else if (e.position.y - dragStart.y < -22.0f && dragRow > 0)
    {
        proc.moveFxSlot (dragRow, dragRow - 1);
        proc.takeUndoSnapshot();
        --dragRow;
        dragStart = e.position;
        refresh();
    }
}

void FxChainPanel::mouseUp (const juce::MouseEvent&)
{
    dragRow = -1;
}

//==============================================================================
CyclePill::CyclePill (juce::StringArray items, int initial, std::function<void (int)> callback)
    : items (std::move (items)), index (initial), onChange (std::move (callback))
{
    setInterceptsMouseClicks (true, false);
}

void CyclePill::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::white.withAlpha (hovered ? 0.75f : 0.5f));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (th::ink.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

    g.setFont (th::font (11.5f, 2));
    g.setColour (th::greenDeep);
    g.drawText (items[index], r.withTrimmedLeft (10.0f).withTrimmedRight (18.0f), juce::Justification::centredLeft, false);

    juce::Path p;
    const auto cx = r.getRight() - 13.0f, cy = r.getCentreY() - 1.0f;
    p.startNewSubPath (cx - 4.0f, cy - 1.5f);
    p.lineTo (cx, cy + 2.5f);
    p.lineTo (cx + 4.0f, cy - 1.5f);
    g.setColour (th::inkSoft.withAlpha (0.8f));
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CyclePill::mouseDown (const juce::MouseEvent&)
{
    index = (index + 1) % juce::jmax (1, items.size());
    repaint();

    if (onChange != nullptr)
        onChange (index);
}

//==============================================================================
StagePanel::StagePanel (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, true);
}

void StagePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    juce::ColourGradient card (juce::Colour (0xfffdfefb), r.getX(), r.getY(),
                               juce::Colour (0xfff4f7ee), r.getRight(), r.getBottom(), false);
    g.setGradientFill (card);
    g.fillRoundedRectangle (r, 18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (r.reduced (0.5f), 18.0f, 1.2f);

    const auto close = closeBounds();
    g.setColour (th::ink.withAlpha (closeHover * 0.08f));
    g.fillEllipse (close.expanded (5.0f));
    th::drawIcon (g, th::Icon::close, close, th::inkSoft, 1.4f);
}

void StagePanel::mouseDown (const juce::MouseEvent& e)
{
    if (closeBounds().contains (e.position) && onClose != nullptr)
        onClose();
}

//==============================================================================
ModMatrixPanel::ModMatrixPanel (BotanicAudioProcessor& processor) : StagePanel (processor)
{
}

juce::Rectangle<float> ModMatrixPanel::cellBounds (int source, int dest) const
{
    auto content = contentBounds();
    const auto labelW = 92.0f;
    const auto cellW = (content.getWidth() - labelW) / (float) numModDests;
    const auto cellH = 30.0f;
    return { content.getX() + labelW + cellW * (float) dest + 2.0f,
             content.getY() + 24.0f + cellH * (float) source + 2.0f,
             cellW - 4.0f, cellH - 4.0f };
}

int ModMatrixPanel::sourceAt (juce::Point<float> p) const
{
    for (int s = 0; s < numModSources; ++s)
        if (cellBounds (s, 0).withWidth (getWidth()).contains (p))
            return s;
    return -1;
}

int ModMatrixPanel::destAt (juce::Point<float> p) const
{
    for (int d = 0; d < numModDests; ++d)
        if (cellBounds (0, d).withHeight (getHeight()).contains (p))
            return d;
    return -1;
}

void ModMatrixPanel::refresh()
{
    bool changed = false;

    for (int s = 0; s < numModSources; ++s)
        for (int d = 0; d < numModDests; ++d)
        {
            const auto v = proc.modMatrix.get (s, d);
            if (std::abs (amounts[(size_t) s][(size_t) d] - v) > 0.0005f)
            {
                amounts[(size_t) s][(size_t) d] = v;
                changed = true;
            }
        }

    if (changed)
        repaint();
}

void ModMatrixPanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    auto content = contentBounds();

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("MODULATION MATRIX"), juce::Rectangle<float> (22.0f, 22.0f, 320.0f, 24.0f), juce::Justification::centredLeft, false);

    auto header = contentBounds();
    const auto labelW = 92.0f;
    const auto cellW = (header.getWidth() - labelW) / (float) numModDests;

    static const char* shortNames[] = { "CUT", "RES", "DRV", "PIT", "O1L", "O2L", "O3L", "O1P", "O2P", "O3P",
                                        "DET", "PAN", "REV", "DLY", "G.POS", "G.MIX", "L.MIX", "L.BIT", "VWL" };
    static_assert (sizeof (shortNames) / sizeof (shortNames[0]) == (size_t) numModDests,
                   "modulation destination labels must match numModDests");

    g.setFont (th::font (8.0f, 1));

    for (int d = 0; d < numModDests; ++d)
    {
        g.setColour (th::ink.withAlpha (0.55f));
        const auto label = d < (int) (sizeof (shortNames) / sizeof (shortNames[0])) ? shortNames[d] : "";
        g.drawText (label, juce::Rectangle<float> (header.getX() + labelW + cellW * (float) d, header.getY() + 8.0f, cellW, 12.0f),
                    juce::Justification::centred, false);
    }

    for (int s = 0; s < numModSources; ++s)
    {
        auto labelArea = juce::Rectangle<float> (header.getX(), header.getY() + 24.0f + 30.0f * (float) s, labelW - 6.0f, 26.0f);
        g.setFont (th::font (10.5f, 1));
        g.setColour (th::inkSoft);
        g.drawText (modSourceName (s), labelArea, juce::Justification::centredLeft, false);
    }

    for (int s = 0; s < numModSources; ++s)
    {
        for (int d = 0; d < numModDests; ++d)
        {
            auto cell = cellBounds (s, d);
            const auto amount = amounts[(size_t) s][(size_t) d];

            g.setColour (th::ink.withAlpha (0.05f));
            g.fillRoundedRectangle (cell, 4.0f);

            if (std::abs (amount) > 0.001f)
            {
                const auto a = juce::jlimit (0.15f, 0.95f, std::abs (amount));
                g.setColour (amount > 0 ? th::green.withAlpha (a) : th::greenDeep.withAlpha (a * 0.55f));
                g.fillRoundedRectangle (cell, 4.0f);

                g.setColour (juce::Colours::white.withAlpha (0.75f));
                g.setFont (th::font (8.0f, 2));
                g.drawText (juce::String (juce::roundToInt (amount * 100.0f)), cell, juce::Justification::centred, false);
            }
            else
            {
                g.setColour (th::ink.withAlpha (0.12f));
                g.fillEllipse (cell.getCentreX() - 1.5f, cell.getCentreY() - 1.5f, 3.0f, 3.0f);
            }
        }
    }

    g.setFont (th::font (9.5f, 1));
    g.setColour (th::ink.withAlpha (0.4f));
    g.drawText (T ("drag a cell vertically to set depth - double click to clear"),
                getLocalBounds().toFloat().removeFromBottom (22.0f).reduced (20.0f, 0.0f),
                juce::Justification::centredLeft, false);
}

void ModMatrixPanel::mouseDown (const juce::MouseEvent& e)
{
    StagePanel::mouseDown (e);

    dragSource = sourceAt (e.position);
    dragDest = destAt (e.position);

    if (dragSource >= 0 && dragDest >= 0)
    {
        dragStart = amounts[(size_t) dragSource][(size_t) dragDest];

        if (e.mods.isPopupMenu())
        {
            proc.modMatrix.set (dragSource, dragDest, 0.0f);
            proc.takeUndoSnapshot();
            refresh();
        }
    }
}

void ModMatrixPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragSource < 0 || dragDest < 0)
        return;

    const auto amount = juce::jlimit (-1.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / 90.0f);
    proc.modMatrix.set (dragSource, dragDest, amount);
    refresh();
}

void ModMatrixPanel::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto s = sourceAt (e.position);
    const auto d = destAt (e.position);

    if (s >= 0 && d >= 0)
    {
        proc.modMatrix.set (s, d, 0.0f);
        proc.takeUndoSnapshot();
        refresh();
    }
}

//==============================================================================
ArpPanel::ArpPanel (BotanicAudioProcessor& processor)
    : StagePanel (processor),
      modePill (processor, Params::arpMode),
      ratePill (processor, Params::arpRate),
      latchSwitch (processor, Params::arpLatch, T ("Latch")),
      octavesKnob (processor, Params::arpOctaves, T ("Octaves"), 58.0f),
      gateKnob (processor, Params::arpGate, T ("Gate"), 58.0f),
      swingKnob (processor, Params::arpSwing, T ("Swing"), 58.0f)
{
    addAndMakeVisible (modePill);
    addAndMakeVisible (ratePill);
    addAndMakeVisible (latchSwitch);
    addAndMakeVisible (octavesKnob);
    addAndMakeVisible (gateKnob);
    addAndMakeVisible (swingKnob);
}

void ArpPanel::resized()
{
    modePill.setBounds (22, 66, 120, 28);
    ratePill.setBounds (152, 66, 96, 28);
    latchSwitch.setBounds (268, 66, 150, 28);

    octavesKnob.setBounds (30, 140, 58, 58 + 40);
    gateKnob.setBounds (110, 140, 58, 58 + 40);
    swingKnob.setBounds (190, 140, 58, 58 + 40);
}

void ArpPanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("ARPEGGIATOR"), juce::Rectangle<float> (22.0f, 22.0f, 300.0f, 24.0f), juce::Justification::centredLeft, false);

    // step preview
    auto r = juce::Rectangle<float> (22.0f, 260.0f, 552.0f, 120.0f);
    g.setColour (th::ink.withAlpha (0.05f));
    g.fillRoundedRectangle (r, 10.0f);

    const auto steps = 16;
    const auto w = r.getWidth() / (float) steps;

    for (int i = 0; i < steps; ++i)
    {
        const auto h = 0.25f + 0.7f * std::abs (std::sin ((float) i * 0.7f));
        auto bar = juce::Rectangle<float> (r.getX() + w * (float) i + 3.0f, r.getBottom() - 10.0f - h * (r.getHeight() - 26.0f),
                                           w - 6.0f, h * (r.getHeight() - 26.0f));
        g.setColour (i % 4 == 0 ? th::greenDark : th::greenMid.withAlpha (0.75f));
        g.fillRoundedRectangle (bar, 3.0f);
    }

    g.setFont (th::font (10.5f, 1));
    g.setColour (th::inkSoft.withAlpha (0.7f));
    g.drawText (T ("steps follow host tempo"), r.removeFromTop (0.0f).withTrimmedTop (r.getHeight() - 14.0f),
                juce::Justification::centredLeft, false);
}

void ArpPanel::refresh()
{
    modePill.refresh();
    ratePill.refresh();
    latchSwitch.refresh();
    octavesKnob.refresh();
    gateKnob.refresh();
    swingKnob.refresh();
}

//==============================================================================
SettingsPanel::SettingsPanel (BotanicAudioProcessor& processor)
    : StagePanel (processor),
      tuningKnob (processor, Params::a4, T ("Tuning A4"), 58.0f),
      voicesKnob (processor, Params::maxVoices, T ("Polyphony"), 58.0f),
      glideKnob (processor, Params::glide, T ("Glide"), 58.0f),
      bendKnob (processor, Params::bendRange, T ("Bend"), 58.0f)
{
    addAndMakeVisible (tuningKnob);
    addAndMakeVisible (voicesKnob);
    addAndMakeVisible (glideKnob);
    addAndMakeVisible (bendKnob);
}

void SettingsPanel::resized()
{
    tuningKnob.setBounds (26, 70, 58, 98);
    voicesKnob.setBounds (110, 70, 58, 98);
    glideKnob.setBounds (194, 70, 58, 98);
    bendKnob.setBounds (278, 70, 58, 98);
}

void SettingsPanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("SETTINGS"), juce::Rectangle<float> (22.0f, 22.0f, 300.0f, 24.0f), juce::Justification::centredLeft, false);

    auto info = juce::Rectangle<float> (22.0f, 196.0f, 552.0f, 66.0f);
    g.setColour (th::ink.withAlpha (0.05f));
    g.fillRoundedRectangle (info, 10.0f);

    g.setFont (th::font (10.5f, 1));
    g.setColour (th::inkSoft);
    g.drawFittedText (T ("Right-click any control to assign modulation or MIDI learn.\n"
                      "Drag the centre artwork away with the nav: Modulation opens the\n"
                      "matrix, Arpeggiator and Settings open their pages."),
                      info.reduced (12.0f, 8.0f).toNearestInt(), juce::Justification::centredLeft, 4);

    for (auto b : { initButton(), saveButton(), randomiseButton(), panicButton() })
    {
        g.setColour (juce::Colour (0xffeef5e3));
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (th::green.withAlpha (0.45f));
        g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.2f);
    }
    g.setFont (th::font (11.0f, 2));
    g.setColour (th::greenDeep);
    g.drawText (T ("Init patch"), initButton(), juce::Justification::centred, false);
    g.drawText (T ("Save as preset..."), saveButton(), juce::Justification::centred, false);
    g.drawText (T ("Randomise..."), randomiseButton(), juce::Justification::centred, false);
    g.drawText (T ("Panic"), panicButton(), juce::Justification::centred, false);

    g.setFont (th::font (9.0f, 1));
    g.setColour (th::ink.withAlpha (0.35f));
    g.setFont (th::font (9.5f, 1));
    g.setColour (th::ink.withAlpha (0.5f));
    g.drawText ("CPU " + juce::String (juce::roundToInt (proc.getCpuLoad() * 100.0f)) + "%",
                juce::Rectangle<float> (480.0f, getHeight() - 22.0f, 96.0f, 14.0f),
                juce::Justification::centredRight, false);
    g.setColour (th::ink.withAlpha (0.45f));
    g.setFont (th::font (9.5f, 1));
    g.drawText (T ("Botanic 1.4  |  hybrid wavetable synthesizer"), juce::Rectangle<float> (22.0f, getHeight() - 22.0f, 400.0f, 14.0f),
                juce::Justification::centredLeft, false);
}

void SettingsPanel::mouseDown (const juce::MouseEvent& e)
{
    StagePanel::mouseDown (e);

    if (initButton().contains (e.position))
        proc.initPatch();
    else if (randomiseButton().contains (e.position))
    {
        proc.randomize (0);
        proc.takeUndoSnapshot();
    }
    else if (panicButton().contains (e.position))
    {
        proc.panic();
        proc.clearHeldVisual();
        proc.takeUndoSnapshot (true);
    }
    else if (saveButton().contains (e.position))
    {
        if (onSaveRequested != nullptr)
            onSaveRequested();
    }
}

void SettingsPanel::refresh()
{
    tuningKnob.refresh();
    voicesKnob.refresh();
    glideKnob.refresh();
    bendKnob.refresh();
}

//==============================================================================
TexturePanel::TexturePanel (BotanicAudioProcessor& processor)
    : StagePanel (processor),
      tabs ({ T ("GRAIN"), T ("GLITCH"), T ("VOWEL") }),
      grainOn (processor, Params::grOn, T ("Grain On")),
      freezeToggle (processor, Params::grFreeze, T ("Freeze")),
      glitchOn (processor, Params::glOn, T ("Glitch On")),
      formantOn (processor, Params::fmtOn, T ("Vowel On")),
      glitchStep (processor, Params::glStep),
      glitchSize (processor, Params::glStutterDiv)
{
    addAndMakeVisible (tabs);
    addAndMakeVisible (grainOn);
    addAndMakeVisible (freezeToggle);
    addChildComponent (glitchOn);
    addChildComponent (glitchStep);
    addChildComponent (glitchSize);
    addChildComponent (formantOn);

    const std::pair<const char*, juce::String> grainSpecs[] = {
        { Params::grMix, T ("Mix") }, { Params::grSize, T ("Size") }, { Params::grDensity, T ("Density") },
        { Params::grPosition, T ("Position") }, { Params::grSpray, T ("Spray") }, { Params::grPitch, T ("Pitch") },
        { Params::grPitchRandom, T ("Pitch Rnd") }, { Params::grReverse, T ("Reverse") }, { Params::grSpread, T ("Spread") }
    };

    for (const auto& s : grainSpecs)
    {
        auto k = std::make_unique<Knob> (processor, s.first, s.second, 54.0f);
        addAndMakeVisible (*k);
        grainKnobs.push_back (std::move (k));
    }

    const std::pair<const char*, juce::String> glitchSpecs[] = {
        { Params::glMix, T ("Mix") }, { Params::glBits, T ("Bits") }, { Params::glRateDiv, T ("Rate") },
        { Params::glStutter, T ("Stutter") }, { Params::glGate, T ("Gate") }, { Params::glDrop, T ("Dropout") },
        { Params::glTape, T ("Tape") }, { Params::glRepeats, T ("Repeats") }, { Params::glChaos, T ("Chaos") },
        { Params::glNoise, T ("Noise") }
    };

    for (const auto& s : glitchSpecs)
    {
        auto k = std::make_unique<Knob> (processor, s.first, s.second, 54.0f);
        addChildComponent (*k);
        glitchKnobs.push_back (std::move (k));
    }

    const std::pair<const char*, juce::String> vowelSpecs[] = {
        { Params::fmtVowel, T ("Vowel") }, { Params::fmtShift, T ("Shift") },
        { Params::fmtQ, T ("Q") }, { Params::fmtMix, T ("Mix") }, { Params::fmtKey, T ("Key") }
    };

    for (const auto& s : vowelSpecs)
    {
        auto k = std::make_unique<Knob> (processor, s.first, s.second, 54.0f);
        addChildComponent (*k);
        vowelKnobs.push_back (std::move (k));
    }

    tabs.onChange = [this] (int index) { tab = index; updateTab(); };
    tabs.setSelected (0);

    triggerButton.setText (T ("TRIGGER"));
    randomButton.setText (T ("RANDOM"));
    triggerButton.onClick = [this] { proc.triggerGlitch(); };
    randomButton.onClick = [this] { proc.randomize (5); proc.takeUndoSnapshot(); };

    addAndMakeVisible (triggerButton);
    addAndMakeVisible (randomButton);

    updateTab();
}

void TexturePanel::updateTab()
{
    const auto grain = (tab == 0);
    const auto glitch = (tab == 1);
    const auto vowel = (tab == 2);

    grainOn.setVisible (grain);
    freezeToggle.setVisible (grain);
    glitchOn.setVisible (glitch);
    glitchStep.setVisible (glitch);
    glitchSize.setVisible (glitch);
    formantOn.setVisible (vowel);

    for (auto& k : grainKnobs)
        k->setVisible (grain);

    for (auto& k : glitchKnobs)
        k->setVisible (glitch);

    for (auto& k : vowelKnobs)
        k->setVisible (vowel);

    repaint();
}

void TexturePanel::resized()
{
    tabs.setBounds (22, 52, 236, 26);
    grainOn.setBounds (26, 92, 132, 24);
    freezeToggle.setBounds (170, 92, 120, 24);
    glitchOn.setBounds (26, 92, 132, 24);

    triggerButton.setBounds (330, 50, 104, 28);
    randomButton.setBounds (444, 50, 104, 28);

    for (int i = 0; i < (int) grainKnobs.size(); ++i)
    {
        const auto col = i % 4;
        const auto row = i / 4;
        grainKnobs[(size_t) i]->setBounds (26 + col * 132 + 38, 130 + row * 100, 56, 96);
    }

    for (int i = 0; i < (int) glitchKnobs.size(); ++i)
    {
        const auto col = i % 4;
        const auto row = i / 4;
        glitchKnobs[(size_t) i]->setBounds (26 + col * 132 + 38, 130 + row * 100, 56, 96);
    }

    glitchStep.setBounds (26 + 2 * 132 + 10, 130 + 2 * 100 + 34, 108, 28);
    glitchSize.setBounds (26 + 3 * 132 + 10, 130 + 2 * 100 + 34, 108, 28);
    formantOn.setBounds (26, 92, 132, 24);

    for (int i = 0; i < (int) vowelKnobs.size(); ++i)
    {
        const auto col = i % 4;
        const auto row = i / 4;
        vowelKnobs[(size_t) i]->setBounds (26 + col * 132 + 38, 130 + row * 100, 56, 96);
    }
}

void TexturePanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("TEXTURE LAB"), juce::Rectangle<float> (22.0f, 20.0f, 300.0f, 24.0f), juce::Justification::centredLeft, false);

    if (tab == 1)
    {
        g.setFont (th::font (10.0f, 1));
        g.setColour (th::inkSoft.withAlpha (0.8f));

        g.drawText ("Step", juce::Rectangle<float> (26.0f + 2 * 132.0f + 10.0f, 130.0f + 2 * 100.0f + 14.0f, 108.0f, 16.0f),
                    juce::Justification::centredLeft, false);
        g.drawText ("Stutter", juce::Rectangle<float> (26.0f + 3 * 132.0f + 10.0f, 130.0f + 2 * 100.0f + 14.0f, 108.0f, 16.0f),
                    juce::Justification::centredLeft, false);
    }

    if (tab == 2)
    {
        // live vowel response so the morph is visible
        auto well = juce::Rectangle<float> (26.0f + 2 * 132.0f + 10.0f, 130.0f + 100.0f, 236.0f, 96.0f);
        th::drawInnerWell (g, well, 10.0f);

        const auto vowel = proc.paramValue (Params::fmtVowel);
        static const float formants[5][3] = {
            { 800.0f, 1150.0f, 2900.0f }, { 400.0f, 1700.0f, 2600.0f },
            { 350.0f, 2100.0f, 2900.0f }, { 450.0f, 800.0f, 2830.0f },
            { 325.0f, 700.0f, 2530.0f }
        };
        const auto pos = juce::jlimit (0.0f, 0.999f, vowel) * 4.0f;
        const auto v0 = (int) pos;
        const auto v1 = juce::jmin (4, v0 + 1);
        const auto t = pos - (float) v0;

        juce::Path curve;

        for (int x = 0; x <= (int) well.getWidth(); ++x)
        {
            const auto norm = (float) x / well.getWidth();
            const auto freq = 150.0f * std::pow (6000.0f / 150.0f, norm);
            float amp = 0.0f;

            for (int f = 0; f < 3; ++f)
            {
                const auto centre = juce::jmap (t, formants[v0][f], formants[v1][f]);
                const auto d = (freq - centre) / (centre * 0.22f + 60.0f);
                amp += std::exp (-d * d) * (f == 0 ? 1.0f : 0.6f);
            }

            const auto px = well.getX() + (float) x;
            const auto py = well.getBottom() - juce::jlimit (0.0f, 1.0f, amp * 0.8f) * well.getHeight();

            if (x == 0)
                curve.startNewSubPath (px, py);
            else
                curve.lineTo (px, py);
        }

        g.setColour (th::green.withAlpha (0.85f));
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setFont (th::font (9.5f, 1));
    g.setColour (th::ink.withAlpha (0.4f));
    const auto hint = tab == 0 ? T ("grains of the synth bus, frozen in time - position drags through the buffer")
                    : tab == 1 ? T ("bit crush + tempo locked accidents - chaos scales how often they happen")
                               : T ("three formant bands morph between A E I O U without changing pitch");
    g.drawText (hint, juce::Rectangle<float> (22.0f, (float) getHeight() - 26.0f, 540.0f, 16.0f),
                juce::Justification::centredLeft, false);
}

void TexturePanel::refresh()
{
    grainOn.refresh();
    freezeToggle.refresh();
    glitchOn.refresh();
    glitchStep.refresh();
    glitchSize.refresh();
    formantOn.refresh();

    for (auto& k : vowelKnobs)
        k->refresh();

    for (auto& k : grainKnobs)
        k->refresh();

    for (auto& k : glitchKnobs)
        k->refresh();
}

void TexturePanel::mouseDown (const juce::MouseEvent& e)
{
    StagePanel::mouseDown (e);
}

//==============================================================================
MacroPanel::MacroPanel (BotanicAudioProcessor& processor)
    : StagePanel (processor),
      destX (processor, Params::macDestX),
      destY (processor, Params::macDestY),
      motion (processor, Params::macMotion, T ("MOTION")),
      rateKnob (processor, Params::macRate, T ("Rate"), 52.0f)
{
    addAndMakeVisible (destX);
    addAndMakeVisible (destY);
    addAndMakeVisible (motion);
    addAndMakeVisible (rateKnob);
}

void MacroPanel::resized()
{
    destX.setBounds (346, 76, 222, 28);
    destY.setBounds (346, 120, 222, 28);
    motion.setBounds (346, 172, 222, 26);
    rateKnob.setBounds (350, 214, 52, 52 + 40);
}

void MacroPanel::setFromPoint (juce::Point<float> p)
{
    const auto pad = padArea();
    const auto nx = juce::jlimit (0.0f, 1.0f, (p.x - pad.getX()) / pad.getWidth());
    const auto ny = juce::jlimit (0.0f, 1.0f, 1.0f - (p.y - pad.getY()) / pad.getHeight());

    if (auto* px = proc.apvts.getParameter (Params::macX))
        px->setValueNotifyingHost (px->convertTo0to1 (nx));

    if (auto* py = proc.apvts.getParameter (Params::macY))
        py->setValueNotifyingHost (py->convertTo0to1 (ny));

    x = nx;
    y = ny;
    repaint();
}

void MacroPanel::mouseDown (const juce::MouseEvent& e)
{
    StagePanel::mouseDown (e);

    if (padArea().contains (e.position))
        setFromPoint (e.position);
}

void MacroPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (padArea().expanded (20.0f).contains (e.position))
        setFromPoint (e.position);
}

void MacroPanel::refresh()
{
    destX.refresh();
    destY.refresh();
    motion.refresh();
    rateKnob.refresh();

    const auto nx = proc.getMacroDisplayX();
    const auto ny = proc.getMacroDisplayY();

    if (std::abs (nx - x) > 0.001f || std::abs (ny - y) > 0.001f)
    {
        x = nx;
        y = ny;
        repaint();
    }
}

void MacroPanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("PERFORM"), juce::Rectangle<float> (22.0f, 20.0f, 300.0f, 24.0f), juce::Justification::centredLeft, false);

    auto pad = padArea();

    th::drawInnerWell (g, pad, 14.0f);

    // grid
    g.setColour (th::ink.withAlpha (0.07f));

    for (int i = 1; i < 4; ++i)
    {
        const auto fx = pad.getX() + pad.getWidth() * (float) i / 4.0f;
        const auto fy = pad.getY() + pad.getHeight() * (float) i / 4.0f;
        g.drawVerticalLine (juce::roundToInt (fx), pad.getY(), pad.getBottom());
        g.drawHorizontalLine (juce::roundToInt (fy), pad.getX(), pad.getRight());
    }

    // motion trail
    if (proc.paramValue (Params::macMotion) > 0.5f)
    {
        juce::Path trail;

        for (int i = 0; i <= 64; ++i)
        {
            const auto t = (float) i / 64.0f * juce::MathConstants<float>::twoPi * 2.0f;
            const auto px = pad.getX() + pad.getWidth() * (0.5f + 0.42f * std::sin (t));
            const auto py = pad.getY() + pad.getHeight() * (0.5f + 0.42f * std::sin (t * 1.37f + 1.1f));

            if (i == 0)
                trail.startNewSubPath (px, py);
            else
                trail.lineTo (px, py);
        }

        g.setColour (th::green.withAlpha (0.18f));
        g.strokePath (trail, juce::PathStrokeType (1.4f));
    }

    const auto px = pad.getX() + pad.getWidth() * x;
    const auto py = pad.getBottom() - pad.getHeight() * y;

    g.setColour (th::green.withAlpha (0.35f));
    g.drawVerticalLine (juce::roundToInt (px), pad.getY(), pad.getBottom());
    g.drawHorizontalLine (juce::roundToInt (py), pad.getX(), pad.getRight());

    juce::ColourGradient glow (th::greenMid.withAlpha (0.55f), px, py,
                               th::greenMid.withAlpha (0.0f), px + 46.0f, py + 46.0f, true);
    g.setGradientFill (glow);
    g.fillEllipse (px - 46.0f, py - 46.0f, 92.0f, 92.0f);

    g.setColour (juce::Colours::white);
    g.fillEllipse (px - 9.0f, py - 9.0f, 18.0f, 18.0f);
    g.setColour (th::greenDeep);
    g.drawEllipse (px - 9.0f, py - 9.0f, 18.0f, 18.0f, 2.0f);

    // axis captions
    g.setFont (th::font (10.5f, 2));
    g.setColour (th::greenDeep.withAlpha (0.85f));
    g.drawText ("X  " + destX.getDisplayText(), pad.withTrimmedTop (pad.getHeight() + 2.0f).withHeight (16.0f),
                juce::Justification::centredLeft, false);
    g.drawText ("Y  " + destY.getDisplayText(), pad.withTrimmedTop (pad.getHeight() + 2.0f).withHeight (16.0f),
                juce::Justification::centredRight, false);

    g.setFont (th::font (9.5f, 1));
    g.setColour (th::ink.withAlpha (0.4f));
    g.drawText (T ("drag the pad to move both macros - right column assigns what they control"),
                juce::Rectangle<float> (22.0f, (float) getHeight() - 26.0f, 540.0f, 16.0f),
                juce::Justification::centredLeft, false);
}

//==============================================================================
ParamPage::ParamPage (BotanicAudioProcessor& processor, juce::String pageTitle,
                      std::vector<Spec> specList, juce::StringArray rowLabels)
    : StagePanel (processor), title (std::move (pageTitle)), rows (std::move (rowLabels)), specs (std::move (specList))
{
    for (const auto& s : specs)
    {
        juce::Component* c = nullptr;

        switch (s.type)
        {
            case Spec::Type::choice: c = new ChoicePill (processor, s.paramId); break;
            case Spec::Type::toggle: c = new ToggleSwitch (processor, s.paramId, s.caption); break;
            case Spec::Type::knob:
            default:                 c = new Knob (processor, s.paramId, s.caption, s.size); break;
        }

        controls.add (c);
        addAndMakeVisible (c);
    }
}

void ParamPage::resized()
{
    auto content = contentBounds();

    if (rows.isEmpty())
        content = content.withTrimmedTop (14.0f);

    const auto labelW = rows.isEmpty() ? 0.0f : 66.0f;
    const auto columns = 4;
    const auto colW = (content.getWidth() - labelW) / (float) columns;
    const auto rowCount = juce::jmax (1, (int) std::ceil ((float) specs.size() / (float) columns));
    const auto rowH = juce::jmin (112.0f, content.getHeight() / (float) rowCount);

    for (int i = 0; i < specs.size(); ++i)
    {
        auto* c = controls[i];
        const auto col = i % columns;
        const auto row = i / columns;
        const auto cellX = content.getX() + labelW + colW * (float) col;
        const auto cellY = content.getY() + rowH * (float) row + (rowH - 96.0f) * 0.5f;

        if (dynamic_cast<Knob*> (c) != nullptr)
            c->setBounds ((int) (cellX + (colW - 56.0f) * 0.5f), (int) cellY, 56, 96);
        else if (dynamic_cast<ToggleSwitch*> (c) != nullptr)
            c->setBounds ((int) (cellX + 8.0f), (int) (cellY + 40.0f), (int) (colW - 16.0f), 26);
        else
            c->setBounds ((int) (cellX + 10.0f), (int) (cellY + 38.0f), (int) (colW - 20.0f), 28);
    }
}

void ParamPage::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (title, juce::Rectangle<float> (22.0f, 22.0f, 340.0f, 24.0f), juce::Justification::centredLeft, false);

    if (rows.isEmpty())
        return;

    auto content = contentBounds();
    const auto rowH = juce::jmin (112.0f, content.getHeight() / (float) rows.size());

    g.setFont (th::font (11.0f, 2));

    for (int r = 0; r < rows.size(); ++r)
    {
        g.setColour (th::greenDeep.withAlpha (0.85f));
        g.drawText (rows[r], juce::Rectangle<float> (content.getX(), content.getY() + rowH * (float) r + 40.0f, 60.0f, 20.0f),
                    juce::Justification::centredLeft, false);

        if (r > 0)
        {
            g.setColour (th::ink.withAlpha (0.06f));
            g.drawHorizontalLine ((int) (content.getY() + rowH * (float) r), content.getX(), content.getRight());
        }
    }
}

void ParamPage::refresh()
{
    for (auto* c : controls)
    {
        if (auto* k = dynamic_cast<Knob*> (c)) k->refresh();
        else if (auto* t = dynamic_cast<ToggleSwitch*> (c)) t->refresh();
        else if (auto* p = dynamic_cast<ChoicePill*> (c)) p->refresh();
    }
}

//==============================================================================
PresetPanel::PresetPanel (BotanicAudioProcessor& processor) : StagePanel (processor)
{
    refresh();
}

void PresetPanel::refresh()
{
    const auto n = proc.getPresetNames();
    const auto idx = proc.getPresetIndex();

    if (n != names || idx != current)
    {
        names = n;
        current = idx;
        repaint();
    }
}

void PresetPanel::paint (juce::Graphics& g)
{
    StagePanel::paint (g);

    g.setFont (th::font (15.0f, 2));
    g.setColour (th::ink);
    g.drawText (T ("PRESETS"), juce::Rectangle<float> (22.0f, 22.0f, 300.0f, 24.0f), juce::Justification::centredLeft, false);

    auto list = listBounds();
    g.setColour (th::ink.withAlpha (0.05f));
    g.fillRoundedRectangle (list, 12.0f);

    const auto rowH = 26.0f;
    const auto numRows = (int) (list.getHeight() / rowH);

    for (int i = 0; i < juce::jmin (numRows, names.size()); ++i)
    {
        auto row = juce::Rectangle<float> (list.getX() + 6.0f, list.getY() + 6.0f + rowH * (float) i,
                                           list.getWidth() - 12.0f, rowH - 2.0f);

        if (i == current)
        {
            g.setColour (th::greenMid.withAlpha (0.85f));
            g.fillRoundedRectangle (row, 8.0f);
        }
        else if (i == hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.fillRoundedRectangle (row, 8.0f);
        }

        g.setFont (th::font (11.5f, i == current ? 2 : 1));
        g.setColour (i == current ? juce::Colours::white : th::ink);
        g.drawText (names[i], row.withTrimmedLeft (12.0f), juce::Justification::centredLeft, false);
    }
}

void PresetPanel::mouseDown (const juce::MouseEvent& e)
{
    StagePanel::mouseDown (e);

    auto list = listBounds();
    const auto rowH = 26.0f;
    const auto index = (int) ((e.position.y - list.getY() - 6.0f) / rowH);

    if (index >= 0 && index < names.size() && list.contains (e.position))
    {
        proc.setPresetIndex (index);
        refresh();
    }
}

void PresetPanel::mouseMove (const juce::MouseEvent& e)
{
    auto list = listBounds();
    const auto rowH = 26.0f;
    const auto index = (int) ((e.position.y - list.getY() - 6.0f) / rowH);

    if (index != hovered)
    {
        hovered = index;
        repaint();
    }
}

//==============================================================================
StageArt::StageArt (BotanicAudioProcessor& processor) : proc (processor)
{
    image = juce::ImageFileFormat::loadFrom (BinaryData::stage_png, (size_t) BinaryData::stage_pngSize);
    setInterceptsMouseClicks (true, false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void StageArt::spawn (juce::Point<float> at, int count, float power)
{
    for (int i = 0; i < count; ++i)
    {
        Particle* p = nullptr;

        for (auto& candidate : particles)
            if (candidate.life <= 0.0f) { p = &candidate; break; }

        if (p == nullptr)
            break;

        const auto angle = rnd.nextFloat() * juce::MathConstants<float>::twoPi;
        const auto speed = (0.3f + rnd.nextFloat()) * power;

        p->x = at.x + (rnd.nextFloat() - 0.5f) * 18.0f;
        p->y = at.y + (rnd.nextFloat() - 0.5f) * 18.0f;
        p->vx = std::cos (angle) * speed * 26.0f;
        p->vy = std::sin (angle) * speed * 26.0f - 12.0f;
        p->maxLife = 1.4f + rnd.nextFloat() * 2.2f;
        p->life = p->maxLife;
        p->size = 1.2f + rnd.nextFloat() * 3.2f;
        p->warm = rnd.nextFloat();
    }
}

void StageArt::addRipple()
{
    auto& r = ripples[(size_t) (rippleIndex++ % (int) ripples.size())];
    r.x = 0.5f + (rnd.nextFloat() - 0.5f) * 0.18f;
    r.y = 0.42f + (rnd.nextFloat() - 0.5f) * 0.16f;
    r.radius = 0.02f;
    r.maxLife = 1.1f + rnd.nextFloat() * 0.5f;
    r.life = r.maxLife;
}

void StageArt::animate (double deltaSeconds)
{
    const auto dt = (float) juce::jlimit (0.0, 0.1, deltaSeconds);
    time += dt;

    level = proc.getMeterLevel();
    smoothLevel += (level - smoothLevel) * juce::jmin (1.0f, dt * 6.0f);
    burst *= std::pow (0.02f, dt);

    // the artwork follows the patch: brightness from the filter / harmonics,
    // texture from the grain, glitch and reverb settings
    const auto cut = proc.paramValue (Params::cutoff);
    const auto cutNorm = juce::jlimit (0.0f, 1.0f, std::log (juce::jmax (20.0f, cut) / 90.0f)
                                                 / std::log (20000.0f / 90.0f));
    const auto brightTarget = juce::jlimit (0.0f, 1.0f,
                                            cutNorm * 0.55f + proc.getVisualBrightness() * 0.7f);
    smoothBright += (brightTarget - smoothBright) * juce::jmin (1.0f, dt * 3.0f);

    const auto grAmt = proc.paramValue (Params::grOn) > 0.5f ? proc.paramValue (Params::grMix) : 0.0f;
    const auto glAmt = proc.paramValue (Params::glOn) > 0.5f ? proc.paramValue (Params::glMix) : 0.0f;
    const auto revAmt = proc.paramValue (Params::revOn) > 0.5f ? proc.paramValue (Params::revMix) : 0.0f;
    const auto texTarget = juce::jlimit (0.0f, 1.0f,
                                         grAmt * 0.55f + glAmt * 0.85f + revAmt * 0.2f
                                         + proc.getVisualTexture() * 0.55f);
    smoothTexture += (texTarget - smoothTexture) * juce::jmin (1.0f, dt * 2.5f);

    if (const auto pulse = proc.getNotePulse(); pulse != lastNotePulse)
    {
        const auto fired = juce::jlimit (1, 4, pulse - lastNotePulse);
        lastNotePulse = pulse;

        for (int i = 0; i < fired; ++i)
            addRipple();
    }

    for (auto& r : ripples)
    {
        if (r.life <= 0.0f)
            continue;

        r.life -= dt;
        r.radius += dt * (0.35f + level * 0.8f);
    }
    hover += ((mouseX > 0.0f ? 1.0f : 0.0f) - hover) * juce::jmin (1.0f, dt * 5.0f);

    // bounce the artwork back after a drag
    if (! dragging)
    {
        dragOffset += dragVelocity * dt;
        dragVelocity *= std::pow (0.02f, dt);
        dragOffset *= std::pow (0.25f, dt);

        if (std::abs (dragOffset) < 0.05f)
            dragOffset = 0.0f;
    }

    const auto w = (float) getWidth(), h = (float) getHeight();

    for (auto& p : particles)
    {
        if (p.life <= 0.0f)
            continue;

        p.life -= dt;
        p.vy -= 14.0f * dt;                     // drift upwards
        p.vx *= std::pow (0.35f, dt);
        p.vy *= std::pow (0.6f, dt);

        // gentle attraction towards the pointer
        if (hover > 0.05f)
        {
            const auto dx = mouseX * w - p.x;
            const auto dy = mouseY * h - p.y;
            const auto dist = juce::jmax (24.0f, std::sqrt (dx * dx + dy * dy));
            const auto pull = (1.0f - juce::jlimit (0.0f, 1.0f, dist / 260.0f)) * 90.0f * hover;
            p.vx += dx / dist * pull * dt;
            p.vy += dy / dist * pull * dt;
        }

        p.x += p.vx * dt * 8.0f;
        p.y += p.vy * dt * 8.0f;

        if (p.life <= 0.0f || p.y < -20.0f)
            p.life = 0.0f;
    }

    // ambient pollen keeps the scene alive, denser when the patch is gritty
    if (rnd.nextFloat() < dt * (6.0f + smoothTexture * 16.0f + smoothLevel * 12.0f))
        spawn ({ rnd.nextFloat() * w, h * (0.55f + rnd.nextFloat() * 0.45f) }, 1, 0.6f);

    repaintTimer += dt;

    if (repaintTimer >= 1.0 / 18.0)
    {
        repaintTimer = 0.0;
        repaint();
    }
}

void StageArt::paint (juce::Graphics& g)
{
    if (! image.isValid())
        return;

    auto bounds = getLocalBounds().toFloat();

    if (! artLayer.isValid() || artLayer.getWidth() != getWidth() || artLayer.getHeight() != getHeight()
        || time - lastLayerTime > (smoothTexture > 0.15f ? 0.055 : 0.085))
    {
        artLayer = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true);
        juce::Graphics lg (artLayer);
        paintLayer (lg);
        lastLayerTime = time;
    }

    g.drawImageAt (artLayer, 0, 0);

    paintOverlay (g);
}

void StageArt::paintLayer (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto w = bounds.getWidth(), h = bounds.getHeight();

    // bright patches turn lime, dark ones stay deep green
    const auto grade = th::greenMid.interpolatedWith (juce::Colour (0xffa9d94f), smoothBright * 0.85f);

    // bake the colour grade into a cached copy so the per frame work stays cheap
    if (! tintedImage.isValid() || tintedImage.getWidth() != image.getWidth()
        || tintedImage.getHeight() != image.getHeight()
        || std::abs (smoothBright - lastTintBright) > 0.012f
        || time - lastTintTime > 0.2)
    {
        tintedImage = image.createCopy();

        {
            juce::Graphics tg (tintedImage);
            const auto tw = (float) tintedImage.getWidth(), th2 = (float) tintedImage.getHeight();
            const auto src = juce::Rectangle<float> (0.0f, 0.0f, tw, th2);

            juce::ColourGradient tint (grade.withAlpha (0.07f + smoothBright * 0.20f), 0.0f, 0.0f,
                                       th::greenDeep.withAlpha (0.10f + smoothLevel * 0.08f), tw, th2, false);
            tg.setGradientFill (tint);
            tg.fillRect (src);

            const auto darken = juce::jmax (0.0f, 0.5f - smoothBright) * 0.36f;
            const auto lift = juce::jmax (0.0f, smoothBright - 0.5f) * 0.18f;

            if (darken > 0.002f)
            {
                tg.setColour (th::greenDeep.withAlpha (darken));
                tg.fillRect (src);
            }
            else if (lift > 0.002f)
            {
                tg.setColour (juce::Colours::white.withAlpha (lift));
                tg.fillRect (src);
            }
        }

        lastTintBright = smoothBright;
        lastTintTime = time;
    }

    const auto parallaxX = (mouseX - 0.5f) * 10.0f * hover;
    const auto parallaxY = (mouseY - 0.5f) * 8.0f * hover;
    const auto bobX = (2.5f + smoothBright * 3.0f) * std::sin ((float) time * (0.43f + smoothBright * 0.2f)) + dragOffset;
    const auto bobY = 2.0f * std::sin ((float) time * 0.61f + 1.2f) + smoothLevel * 10.0f;
    const auto wobble = 1.0f + smoothLevel * 7.0f + burst * 5.0f + smoothBright * 1.5f;

    // draw the artwork as thin horizontal bands that wave with the sound: cheap
    // enough for 60 fps and gives the flower a liquid, breathing movement
    {
        const auto bands = 18;
        const auto bandH = h / (float) bands;
        const auto texKick = smoothTexture > 0.06f ? smoothTexture : 0.0f;

        for (int b = 0; b < bands; ++b)
        {
            const auto y = bandH * (float) b;
            const auto phase = (float) time * 2.3f + (float) b * 0.42f;
            const auto dy = std::sin (phase) * wobble;
            const auto dx = bobX + parallaxX
                              + std::sin (phase * 0.63f + 1.1f) * wobble * 0.55f
                              + (texKick > 0.0f ? (rnd.nextFloat() * 2.0f - 1.0f) * texKick * 15.0f : 0.0f);

            const auto srcY = (int) (y * (float) tintedImage.getHeight() / h);
            const auto srcH = (int) (bandH * (float) tintedImage.getHeight() / h) + 2;

            g.drawImage (tintedImage,
                         juce::roundToInt (dx), juce::roundToInt (y + dy + bobY + parallaxY),
                         juce::roundToInt (w), juce::roundToInt (bandH + 2.0f),
                         0, srcY, tintedImage.getWidth(), srcH,
                         false);
        }
    }


    // bloom behind the flower, breathing with the audio
    const auto bloom = 0.10f + smoothLevel * 0.5f + burst * 0.25f + smoothBright * 0.12f;
    const auto bloomSize = 430.0f + smoothLevel * 90.0f + smoothBright * 40.0f;
    juce::ColourGradient glow (grade.brighter (0.35f).withAlpha (juce::jlimit (0.0f, 0.6f, bloom)),
                               w * 0.5f, h * 0.36f,
                               grade.withAlpha (0.0f), w * 0.5f, h * 0.36f, true);
    g.setGradientFill (glow);
    g.fillEllipse (w * 0.5f - bloomSize * 0.5f, h * 0.36f - bloomSize * 0.5f, bloomSize, bloomSize);

    // hologram sweep across the wordmark area
    const auto sweepX = w * (0.15f + 0.7f * (float) std::fmod (time * 0.11, 1.0));
    juce::ColourGradient sweep (juce::Colours::transparentWhite, sweepX - 90.0f, 0.0f,
                                juce::Colours::transparentWhite, sweepX + 90.0f, 0.0f, false);
    sweep.addColour (0.5, juce::Colours::white.withAlpha (0.10f));
    g.setGradientFill (sweep);
    g.fillRect (bounds.withTop (h * 0.62f).withBottom (h * 0.95f));

    juce::ignoreUnused (bounds);
}

void StageArt::paintOverlay (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto w = bounds.getWidth(), h = bounds.getHeight();

    // pointer light
    if (hover > 0.02f)
    {
        const auto mx = mouseX * w, my = mouseY * h;
        juce::ColourGradient spot (juce::Colours::white.withAlpha (0.15f * hover), mx, my,
                                   juce::Colours::white.withAlpha (0.0f), mx + 130.0f, my + 120.0f, true);
        g.setGradientFill (spot);
        g.fillEllipse (mx - 130.0f, my - 120.0f, 260.0f, 240.0f);
    }

    // note ripples blooming out of the flower
    for (const auto& r : ripples)
    {
        if (r.life <= 0.0f)
            continue;

        const auto f = juce::jlimit (0.0f, 1.0f, r.life / r.maxLife);
        const auto radius = (0.05f + r.radius) * juce::jmin (w, h);
        const auto alpha = 0.30f * f * f;
        const auto centre = juce::Point<float> (r.x * w, r.y * h);

        g.setColour (th::greenMid.withAlpha (alpha));
        g.drawEllipse (juce::Rectangle<float> (centre.x - radius, centre.y - radius * 0.82f,
                                               radius * 2.0f, radius * 1.64f), 1.6f + 2.4f * f);
    }

    // pollen
    for (const auto& p : particles)
    {
        if (p.life <= 0.0f)
            continue;

        const auto age = 1.0f - p.life / p.maxLife;
        const auto alpha = std::sin (juce::jlimit (0.0f, 1.0f, age) * juce::MathConstants<float>::pi);
        const auto radius = p.size * (0.6f + age * 1.4f);
        const auto colour = p.warm > 0.55f ? th::greenMid : juce::Colours::white;

        g.setColour (colour.withAlpha (0.55f * alpha));
        g.fillEllipse (p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour (colour.withAlpha (0.18f * alpha));
        g.fillEllipse (p.x - radius * 2.4f, p.y - radius * 2.4f, radius * 4.8f, radius * 4.8f);
    }

    juce::ignoreUnused (bounds, w, h);
}

void StageArt::mouseMove (const juce::MouseEvent& e)
{
    mouseX = juce::jlimit (0.0f, 1.0f, e.position.x / (float) juce::jmax (1, getWidth()));
    mouseY = juce::jlimit (0.0f, 1.0f, e.position.y / (float) juce::jmax (1, getHeight()));
}

void StageArt::mouseExit (const juce::MouseEvent&)
{
    mouseX = -1.0f;
    mouseY = -1.0f;
    hover = 0.0f;
}

void StageArt::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    dragStartX = e.position.x;
    lastDragX = e.position.x;
    spawn (e.position, 26, 1.6f);
    burst = 1.0f;
    proc.triggerGlitch();
}

void StageArt::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    const auto delta = e.position.x - dragStartX;
    dragOffset = juce::jlimit (-26.0f, 26.0f, delta * 0.35f);
    dragVelocity = -(e.position.x - lastDragX) * 4.0f;
    lastDragX = e.position.x;
    mouseX = juce::jlimit (0.0f, 1.0f, e.position.x / (float) juce::jmax (1, getWidth()));
    mouseY = juce::jlimit (0.0f, 1.0f, e.position.y / (float) juce::jmax (1, getHeight()));
}

void StageArt::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
}

//==============================================================================
MainView::MainView (BotanicAudioProcessor& processor)
    : proc (processor),
      nav (processor),
      presetBar (processor),
      power (processor),
      masterKnob (processor, Params::master, T ("MASTER"), 86.0f),
      oscTabs ({ "OSC 1", "OSC 2", "OSC 3" }),
      wtDisplay (processor),
      tablePill (processor, Params::oscId (0, "table")),
      oscKnob1 (processor, Params::oscId (0, "tune"), T ("Tune"), 58.0f),
      oscKnob2 (processor, Params::oscId (0, "detune"), T ("Detune"), 58.0f),
      oscKnob3 (processor, Params::oscId (0, "level"), T ("Level"), 58.0f),
      filterPill (processor, Params::filterType),
      filterDisplay (processor),
      filtKnob1 (processor, Params::cutoff, T ("Cutoff"), 54.0f),
      filtKnob2 (processor, Params::resonance, T ("Resonance"), 54.0f),
      filtKnob3 (processor, Params::drive, T ("Drive"), 54.0f),
      envTarget ({ T ("AMP"), T ("ENV 1"), T ("ENV 2") }, 0, [this] (int index) { updateEnvelopePage (index); }),
      envTabs ({ "A", "D", "S", "R" }),
      envDisplay (processor),
      envFader1 (processor, Params::envId (0, "A"), "A", true),
      envFader2 (processor, Params::envId (0, "D"), "D", true),
      envFader3 (processor, Params::envId (0, "S"), "S", true),
      envFader4 (processor, Params::envId (0, "R"), "R", true),
      modTabs ({ "LFO 1", "LFO 2", T ("ENV 1"), T ("ENV 2") }),
      lfoDisplay (processor),
      modEnvDisplay (processor),
      syncSwitch (processor, Params::lfoId (0, "sync"), T ("Sync")),
      shapePill (processor, Params::lfoId (0, "shape")),
      modKnob1 (processor, Params::lfoId (0, "rate"), T ("Rate"), 44.0f),
      modKnob2 (processor, Params::lfoId (0, "depth"), T ("Depth"), 44.0f),
      modKnob3 (processor, Params::lfoId (0, "phase"), T ("Phase"), 44.0f),
      modKnob4 (processor, Params::envId (1, "A"), T ("Attack"), 44.0f),
      fxTabs ({ T ("Reverb"), T ("Delay"), T ("Chorus"), "EQ", T ("Comp") }),
      fxKnob1 (processor, Params::revMix, T ("Mix"), 48.0f),
      fxKnob2 (processor, Params::revSize, T ("Size"), 48.0f),
      fxKnob3 (processor, Params::revDamp, T ("Damp"), 48.0f),
      chainPanel (processor),
      volumeFader (processor, Params::output, "", false),
      tiltKnob (processor, Params::toneTilt, T ("Tilt"), 40.0f),
      widthKnob (processor, Params::toneWidth, T ("Width"), 40.0f),
      airKnob (processor, Params::toneAir, T ("Air"), 40.0f),
      modePill (processor, Params::polyMode),
      voicesPill (processor, Params::maxVoices, { "1", "2", "4", "8", "12", "16", "24", "32" }, { 1, 2, 4, 8, 12, 16, 24, 32 }),
      tuningPill (processor, Params::a4, { "415 Hz", "432 Hz", "440 Hz", "442 Hz", "444 Hz" }, { 415, 432, 440, 442, 444 }),
      keyboard (processor),
      matrixPanel (processor),
      arpPanel (processor),
      settingsPanel (processor),
      presetPanel (processor),
      oscPage (processor, "OSCILLATOR DETAILS",
               { { ParamPage::Spec::Type::knob, Params::oscId (0, "uni"),    "Unison", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (0, "spread"), "Spread", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (0, "pan"),    "Pan",    50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (0, "phase"),  "Phase",  50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (1, "uni"),    "Unison", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (1, "spread"), "Spread", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (1, "pan"),    "Pan",    50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (1, "phase"),  "Phase",  50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (2, "uni"),    "Unison", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (2, "spread"), "Spread", 50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (2, "pan"),    "Pan",    50.0f },
                 { ParamPage::Spec::Type::knob, Params::oscId (2, "phase"),  "Phase",  50.0f } },
               { "OSC 1", "OSC 2", "OSC 3" }),
      filterPage (processor, "FILTER & VOICE",
                  { { ParamPage::Spec::Type::knob,   Params::filterEnv, "Env Amt",   50.0f },
                    { ParamPage::Spec::Type::knob,   Params::keyTrack,  "Key Track", 50.0f },
                    { ParamPage::Spec::Type::choice, Params::filterType, "Type" },
                    { ParamPage::Spec::Type::knob,   Params::resonance, "Resonance", 50.0f },
                    { ParamPage::Spec::Type::knob,   Params::glide,     "Glide",     50.0f },
                    { ParamPage::Spec::Type::knob,   Params::bendRange, "Bend",      50.0f },
                    { ParamPage::Spec::Type::knob,   Params::maxVoices, "Polyphony", 50.0f },
                    { ParamPage::Spec::Type::knob,   Params::a4,        "Tuning",    50.0f } }),
      fxPage (processor, "EFFECT CHAIN DETAILS",
              { { ParamPage::Spec::Type::toggle, Params::revOn,  "Reverb" },
                { ParamPage::Spec::Type::knob,   Params::revWidth, "Width", 46.0f },
                { ParamPage::Spec::Type::toggle, Params::dlyOn,  "Delay" },
                { ParamPage::Spec::Type::knob,   Params::dlyTime, "Time", 46.0f },
                { ParamPage::Spec::Type::toggle, Params::dlySync, "Sync" },
                { ParamPage::Spec::Type::choice, Params::dlyDiv,  "Division" },
                { ParamPage::Spec::Type::knob,   Params::dlyFb,  "Feedback", 46.0f },
                { ParamPage::Spec::Type::toggle, Params::choOn,  "Chorus" },
                { ParamPage::Spec::Type::knob,   Params::choRate, "Rate", 46.0f },
                { ParamPage::Spec::Type::knob,   Params::choDepth, "Depth", 46.0f },
                { ParamPage::Spec::Type::toggle, Params::eqOn,   "EQ" },
                { ParamPage::Spec::Type::knob,   Params::eqLow,  "Low", 46.0f },
                { ParamPage::Spec::Type::knob,   Params::eqHigh, "High", 46.0f },
                { ParamPage::Spec::Type::toggle, Params::compOn, "Comp" },
                { ParamPage::Spec::Type::knob,   Params::compThresh, "Thresh", 46.0f },
                { ParamPage::Spec::Type::knob,   Params::compRatio,  "Ratio", 46.0f } }),
      texturePanel (processor),
      macroPanel (processor),
      stageArt (processor)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (stageArt);
    addAndMakeVisible (nav);
    addAndMakeVisible (presetBar);
    addAndMakeVisible (power);
    addAndMakeVisible (masterKnob);
    addAndMakeVisible (settingsButton);

    addAndMakeVisible (oscTabs);
    addAndMakeVisible (wtDisplay);
    addAndMakeVisible (tablePill);
    addAndMakeVisible (oscKnob1);
    addAndMakeVisible (oscKnob2);
    addAndMakeVisible (oscKnob3);

    addAndMakeVisible (filterPill);
    addAndMakeVisible (filterDisplay);
    addAndMakeVisible (filtKnob1);
    addAndMakeVisible (filtKnob2);
    addAndMakeVisible (filtKnob3);

    addAndMakeVisible (envTarget);
    addAndMakeVisible (envTabs);
    addAndMakeVisible (envDisplay);
    addAndMakeVisible (envFader1);
    addAndMakeVisible (envFader2);
    addAndMakeVisible (envFader3);
    addAndMakeVisible (envFader4);

    addAndMakeVisible (modTabs);
    addAndMakeVisible (lfoDisplay);
    addChildComponent (modEnvDisplay);
    addAndMakeVisible (syncSwitch);
    addAndMakeVisible (shapePill);
    addAndMakeVisible (modKnob1);
    addAndMakeVisible (modKnob2);
    addAndMakeVisible (modKnob3);
    addChildComponent (modKnob4);

    addAndMakeVisible (fxTabs);
    addAndMakeVisible (fxKnob1);
    addAndMakeVisible (fxKnob2);
    addAndMakeVisible (fxKnob3);
    addAndMakeVisible (chainPanel);
    addAndMakeVisible (volumeFader);
    addAndMakeVisible (tiltKnob);
    addAndMakeVisible (widthKnob);
    addAndMakeVisible (airKnob);
    addAndMakeVisible (levelMeter);
    addAndMakeVisible (abButtonA);
    addAndMakeVisible (abButtonB);

    addAndMakeVisible (modePill);
    addAndMakeVisible (voicesPill);
    addAndMakeVisible (tuningPill);
    addAndMakeVisible (kbdButton);
    addAndMakeVisible (keyboard);
    keyboard.setComputerKeyboardEnabled (true);
    kbdButton.setColour (th::greenDark);

    addChildComponent (matrixPanel);
    addChildComponent (arpPanel);
    addChildComponent (settingsPanel);
    addChildComponent (presetPanel);
    addChildComponent (oscPage);
    addChildComponent (filterPage);
    addChildComponent (fxPage);
    addChildComponent (texturePanel);
    addChildComponent (macroPanel);
    addAndMakeVisible (randomButton);

    oscTabs.setSelected (0);
    oscTabs.onChange = [this] (int i) { updateOscillatorPage (i); };
    envTabs.onChange = [this] (int i) { envDisplay.setHighlightStage (i); };
    envTabs.setSelected (0);
    modTabs.onChange = [this] (int i) { updateModPage (i); };
    modTabs.setSelected (0);
    fxTabs.onChange = [this] (int i) { updateFxPage (i); };
    fxTabs.setSelected (0);

    nav.onSelect = [this] (int index)
    {
        navFocus = index;

        switch (index)
        {
            case 1: setStagePage (5); break;
            case 2: setStagePage (6); break;
            case 3: setStagePage (1); break;
            case 4: setStagePage (7); break;
            case 5: setStagePage (8); break;
            case 6: setStagePage (9); break;
            case 7: setStagePage (2); break;
            case 8: setStagePage (3); break;
            default: setStagePage (0); break;
        }
    };

    for (StagePanel* p : std::initializer_list<StagePanel*> { &matrixPanel, &arpPanel, &settingsPanel, &presetPanel, &oscPage, &filterPage, &fxPage, &texturePanel, &macroPanel })
        p->onClose = [this] { setStagePage (0); nav.setSelected (0); navFocus = 0; };

    presetBar.onBrowse = [this]
    {
        setStagePage (4);
    };

    settingsButton.onClick = [this]
    {
        nav.setSelected (8);
        navFocus = 8;
        setStagePage (3);
    };

    languageButton.setText (ui::Locale::isChinese() ? juce::String ("EN") : juce::String (juce::CharPointer_UTF8 ("\xe4\xb8\xad \xe6\x96\x87")));
    languageButton.onClick = [this] { if (onToggleLanguage != nullptr) onToggleLanguage(); };
    addAndMakeVisible (languageButton);

    randomButton.setText (T ("Random"));
    randomButton.setIcon (th::Icon::dice);
    randomButton.onClick = [this]
    {
        proc.randomize (0);
        proc.takeUndoSnapshot();
    };
    randomButton.onSecondaryClick = [this] { showRandomMenu(); };

    abButtonA.setText ("A");
    abButtonB.setText ("B");
    abButtonA.setAccent (th::greenDeep);
    abButtonB.setAccent (th::muted);
    abButtonA.setTooltip (T ("Compare slot A"));
    abButtonB.setTooltip (T ("Compare slot B"));
    abButtonA.onClick = [this] { switchAB (0); };
    abButtonB.onClick = [this] { switchAB (1); };

    kbdButton.onClick = [this]
    {
        keyboard.setComputerKeyboardEnabled (! keyboard.isComputerKeyboardEnabled());
        kbdButton.setColour (keyboard.isComputerKeyboardEnabled() ? th::greenDark : th::inkSoft);
        grabKeyboardFocus();
    };

    stageImage = juce::ImageFileFormat::loadFrom (BinaryData::stage_png, (size_t) BinaryData::stage_pngSize);
    mountainImage = juce::ImageFileFormat::loadFrom (BinaryData::mountains_png, (size_t) BinaryData::mountains_pngSize);
    textureImage = juce::ImageFileFormat::loadFrom (BinaryData::texture_png, (size_t) BinaryData::texture_pngSize);

    // subtle procedural grain so large flat areas do not look plastic
    noiseTexture = juce::Image (juce::Image::ARGB, 256, 256, true);
    {
        juce::Random rnd (0x5eed1234);
        juce::Image::BitmapData data (noiseTexture, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < 256; ++y)
        {
            for (int x = 0; x < 256; ++x)
            {
                const auto v = (juce::uint8) (96 + rnd.nextInt (64));
                data.setPixelColour (x, y, juce::Colour (v, v, v, (juce::uint8) 70));
            }
        }
    }

    updateOscillatorPage (0);
    updateEnvelopePage (0);
    updateModPage (0);
    updateFxPage (0);

    setSize (designWidth, designHeight);
}

MainView::~MainView() = default;

void MainView::setStagePage (int page)
{
    stagePage = page;

    matrixPanel.setVisible (page == 1);
    arpPanel.setVisible (page == 2);
    settingsPanel.setVisible (page == 3);
    presetPanel.setVisible (page == 4);
    oscPage.setVisible (page == 5);
    filterPage.setVisible (page == 6);
    fxPage.setVisible (page == 7);
    texturePanel.setVisible (page == 8);
    macroPanel.setVisible (page == 9);

    if (page == 4)
        presetPanel.refresh();

    repaint();
}

void MainView::updateOscillatorPage (int osc)
{
    currentOsc = osc;
    wtDisplay.setOscillator (osc);
    tablePill.setVisible (true);

    oscKnob1.setParameter (Params::oscId (osc, "tune"), T ("Tune"));
    oscKnob2.setParameter (Params::oscId (osc, "detune"), T ("Detune"));
    oscKnob3.setParameter (Params::oscId (osc, "level"), T ("Level"));
}

void MainView::updateEnvelopePage (int env)
{
    currentEnv = env;

    static const char* fields[] = { "A", "D", "S", "R" };
    static const char* captions[] = { "A", "D", "S", "R" };

    std::array<VerticalFader*, 4> faders { &envFader1, &envFader2, &envFader3, &envFader4 };

    for (int i = 0; i < 4; ++i)
        faders[(size_t) i]->setParameter (Params::envId (env, fields[i]), captions[i]);

    envDisplay.setEnvelope (env);
}

void MainView::updateModPage (int tab)
{
    currentModTab = tab;
    const auto isLfo = tab < 2;

    lfoDisplay.setVisible (isLfo);
    syncSwitch.setVisible (isLfo);
    shapePill.setVisible (isLfo);
    modEnvDisplay.setVisible (! isLfo);
    modKnob4.setVisible (! isLfo);

    if (isLfo)
    {
        lfoDisplay.setLfo (tab);
        syncSwitch.setParameter (Params::lfoId (tab, "sync"), T ("Sync"));
        syncSwitch.setBounds (452, 644, 118, 24);
        syncSwitch.setVisible (true);

        modKnob1.setParameter (Params::lfoId (tab, "rate"), T ("Rate"));
        modKnob2.setParameter (Params::lfoId (tab, "depth"), T ("Depth"));
        modKnob3.setParameter (Params::lfoId (tab, "phase"), T ("Phase"));

        modKnob1.setBounds (117, 732, 46, 46 + 40);
        modKnob2.setBounds (217, 732, 46, 46 + 40);
        modKnob3.setBounds (317, 732, 46, 46 + 40);
    }
    else
    {
        const auto env = tab - 1;   // ENV1 -> 1, ENV2 -> 2
        modEnvDisplay.setEnvelope (env);

        modKnob1.setParameter (Params::envId (env, "A"), T ("Attack"));
        modKnob2.setParameter (Params::envId (env, "D"), T ("Decay"));
        modKnob3.setParameter (Params::envId (env, "S"), T ("Sustain"));
        modKnob4.setParameter (Params::envId (env, "R"), T ("Release"));

        modKnob1.setBounds (77, 732, 46, 46 + 40);
        modKnob2.setBounds (167, 732, 46, 46 + 40);
        modKnob3.setBounds (257, 732, 46, 46 + 40);
        modKnob4.setBounds (347, 732, 46, 46 + 40);
    }

    repaint();
}

void MainView::updateFxPage (int tab)
{
    currentFxTab = tab;

    switch (tab)
    {
        case 0:
            fxKnob1.setParameter (Params::revMix, T ("Mix"));
            fxKnob2.setParameter (Params::revSize, T ("Size"));
            fxKnob3.setParameter (Params::revDamp, T ("Damp"));
            break;
        case 1:
            fxKnob1.setParameter (Params::dlyMix, T ("Mix"));
            fxKnob2.setParameter (Params::dlyTime, T ("Time"));
            fxKnob3.setParameter (Params::dlyFb, T ("Feedback"));
            break;
        case 2:
            fxKnob1.setParameter (Params::choMix, T ("Mix"));
            fxKnob2.setParameter (Params::choRate, T ("Rate"));
            fxKnob3.setParameter (Params::choDepth, T ("Depth"));
            break;
        case 3:
            fxKnob1.setParameter (Params::eqLow, T ("Low"));
            fxKnob2.setParameter (Params::eqMid, T ("Mid"));
            fxKnob3.setParameter (Params::eqHigh, T ("High"));
            break;
        default:
            fxKnob1.setParameter (Params::compThresh, T ("Threshold"));
            fxKnob2.setParameter (Params::compRatio, T ("Ratio"));
            fxKnob3.setParameter (Params::compMakeup, T ("Makeup"));
            break;
    }
}

void MainView::resized()
{
    nav.setBounds (14, 140, 138, 420);
    presetBar.setBounds (585, 42, 505, 42);
    power.setBounds (1306, 48, 76, 32);
    masterKnob.setBounds (1417, 12, 90, 90 + 40);
    settingsButton.setBounds (1626, 50, 26, 26);
    randomButton.setBounds (1126, 50, 104, 30);
    languageButton.setBounds (1546, 54, 60, 26);
    stageArt.setBounds (516, 86, 596, 466);

    // oscillator card
    oscTabs.setBounds (193, 186, 292, 26);
    wtDisplay.setBounds (188, 226, 308, 122);
    tablePill.setBounds (190, 356, 143, 28);
    oscKnob1.setBounds (203, 395, 60, 60 + 40);
    oscKnob2.setBounds (314, 395, 60, 60 + 40);
    oscKnob3.setBounds (425, 395, 60, 60 + 40);

    // filter card
    filterPill.setBounds (1168, 196, 132, 28);
    filterDisplay.setBounds (1122, 232, 220, 158);
    filtKnob1.setBounds (1132, 397, 56, 56 + 40);
    filtKnob2.setBounds (1212, 397, 56, 56 + 40);
    filtKnob3.setBounds (1292, 397, 56, 56 + 40);

    // envelope card
    envTarget.setBounds (1560, 150, 80, 24);
    envTabs.setBounds (1408, 237, 224, 26);
    envDisplay.setBounds (1390, 275, 250, 130);
    envFader1.setBounds (1396, 412, 44, 118);
    envFader2.setBounds (1461, 412, 44, 118);
    envFader3.setBounds (1526, 412, 44, 118);
    envFader4.setBounds (1591, 412, 44, 118);

    // modulation card
    modTabs.setBounds (170, 600, 316, 26);
    lfoDisplay.setBounds (40, 628, 395, 80);
    modEnvDisplay.setBounds (40, 628, 395, 80);
    syncSwitch.setBounds (452, 644, 118, 24);
    shapePill.setBounds (455, 690, 115, 26);

    // effects card
    fxTabs.setBounds (660, 597, 400, 24);
    fxKnob1.setBounds (676, 730, 48, 48 + 40);
    fxKnob2.setBounds (776, 730, 48, 48 + 40);
    fxKnob3.setBounds (876, 730, 48, 48 + 40);

    chainPanel.setBounds (1105, 555, 350, 262);
    volumeFader.setBounds (1532, 588, 58, 108);
    levelMeter.setBounds (1478, 574, 158, 8);
    tiltKnob.setBounds (1468, 726, 44, 44 + 40);
    widthKnob.setBounds (1528, 726, 44, 44 + 40);
    airKnob.setBounds (1588, 726, 44, 44 + 40);
    abButtonA.setBounds (1240, 50, 30, 30);
    abButtonB.setBounds (1274, 50, 30, 30);

    // bottom bar
    modePill.setBounds (55, 858, 78, 30);
    voicesPill.setBounds (150, 858, 82, 30);
    tuningPill.setBounds (248, 858, 86, 30);
    kbdButton.setBounds (338, 860, 28, 26);
    keyboard.setBounds (400, 848, 1040, 93);

    // stage pages
    const juce::Rectangle<int> stage (516, 86, 596, 466);
    matrixPanel.setBounds (stage);
    arpPanel.setBounds (stage);
    settingsPanel.setBounds (stage);
    presetPanel.setBounds (stage);
    oscPage.setBounds (stage);
    filterPage.setBounds (stage);
    fxPage.setBounds (stage);
    texturePanel.setBounds (stage);
    macroPanel.setBounds (stage);

    backgroundCache = {};
    chromeCache = {};
}

void MainView::drawBackground (juce::Graphics& g)
{
    if (! backgroundCache.isValid())
    {
        backgroundCache = juce::Image (juce::Image::ARGB, designWidth, designHeight, true);
        juce::Graphics bg (backgroundCache);

        const juce::Rectangle<float> full (0.0f, 0.0f, (float) designWidth, (float) designHeight);

        bg.setGradientFill (juce::ColourGradient (juce::Colour (0xfffdfefb), 0.0f, 0.0f,
                                                  juce::Colour (0xfff0f2e9), 0.0f, (float) designHeight, false));
        bg.fillRect (full);

        auto wash = [&bg] (float x, float y, float radius, juce::Colour colour)
        {
            juce::ColourGradient grad (colour, x, y, colour.withAlpha (0.0f), x + radius, y + radius, true);
            bg.setGradientFill (grad);
            bg.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        };

        wash (40.0f, 60.0f, 520.0f, th::greenLight.withAlpha (0.20f));
        wash (40.0f, 940.0f, 460.0f, th::green.withAlpha (0.09f));
        wash (1650.0f, 940.0f, 520.0f, th::greenMid.withAlpha (0.13f));
        wash (1660.0f, 40.0f, 380.0f, th::greenLight.withAlpha (0.14f));
        wash (860.0f, 930.0f, 380.0f, th::greenLight.withAlpha (0.12f));

        // large translucent leaves in the corners
        for (int i = 0; i < 2; ++i)
        {
            bg.saveState();
            bg.addTransform (juce::AffineTransform::rotation (i == 0 ? -0.5f : 0.7f, 90.0f, 880.0f));
            juce::Path leaf;
            leaf.startNewSubPath (90.0f, 1040.0f);
            leaf.cubicTo (40.0f, 800.0f, 220.0f, 690.0f, 320.0f, 700.0f);
            leaf.cubicTo (250.0f, 860.0f, 190.0f, 980.0f, 90.0f, 1040.0f);
            leaf.closeSubPath();
            bg.setColour (th::green.withAlpha (i == 0 ? 0.045f : 0.03f));
            bg.fillPath (leaf);
            bg.restoreState();
        }

        for (int i = 0; i < 2; ++i)
        {
            bg.saveState();
            bg.addTransform (juce::AffineTransform::rotation (i == 0 ? 2.4f : 2.1f, 1600.0f, 880.0f));
            juce::Path leaf;
            leaf.startNewSubPath (1600.0f, 1080.0f);
            leaf.cubicTo (1560.0f, 820.0f, 1760.0f, 700.0f, 1860.0f, 700.0f);
            leaf.cubicTo (1790.0f, 880.0f, 1720.0f, 1000.0f, 1600.0f, 1080.0f);
            leaf.closeSubPath();
            bg.setColour (th::greenMid.withAlpha (i == 0 ? 0.07f : 0.045f));
            bg.fillPath (leaf);
            bg.restoreState();
        }

        // specimen construction lines
        bg.setColour (th::ink.withAlpha (0.05f));
        bg.drawHorizontalLine (101, 0.0f, (float) designWidth);
        bg.drawVerticalLine (520, 86.0f, 552.0f);
        bg.drawVerticalLine (1112, 86.0f, 552.0f);

        bg.setColour (th::ink.withAlpha (0.10f));
        for (int i = 0; i < 7; ++i)
        {
            const auto x = 60.0f + 8.0f * (float) i;
            bg.drawVerticalLine (juce::roundToInt (x), 96.0f, 106.0f);
        }

        // very light paper grain
        if (noiseTexture.isValid())
        {
            bg.setOpacity (0.05f);

            for (float y = 0.0f; y < (float) designHeight; y += 256.0f)
                for (float x = 0.0f; x < (float) designWidth; x += 256.0f)
                    bg.drawImageAt (noiseTexture, (int) x, (int) y);

            bg.setOpacity (1.0f);
        }

        // vignette
        juce::ColourGradient vignette (juce::Colours::transparentBlack, (float) designWidth * 0.5f, (float) designHeight * 0.45f,
                                       juce::Colours::black.withAlpha (0.05f), (float) designWidth * 0.5f, 0.0f, true);
        bg.setGradientFill (vignette);
        bg.fillRect (full);
    }

    g.drawImageAt (backgroundCache, 0, 0);
}

void MainView::drawPanels (juce::Graphics& g)
{
    th::drawPanel (g, { 176.0f, 145.0f, 336.0f, 400.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 1112.0f, 145.0f, 240.0f, 390.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 1372.0f, 145.0f, 278.0f, 390.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 28.0f, 555.0f, 558.0f, 262.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 608.0f, 555.0f, 490.0f, 262.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 1105.0f, 555.0f, 350.0f, 262.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 1465.0f, 555.0f, 185.0f, 262.0f }, 20.0f, 0.68f);
    th::drawPanel (g, { 24.0f, 846.0f, 372.0f, 56.0f }, 20.0f, 0.55f);

    juce::ignoreUnused (navFocus);

    // fx artwork strip
    if (mountainImage.isValid())
    {
        auto art = juce::Rectangle<float> (622.0f, 628.0f, 452.0f, 95.0f);
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (art, 6.0f);
        g.reduceClipRegion (clip);
        g.drawImage (mountainImage, art, juce::RectanglePlacement::stretchToFit, false);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.30f), art.getX(), art.getY(),
                                                 juce::Colour (0xffdfe7d6).withAlpha (0.20f), art.getRight(), art.getBottom(), false));
        g.fillRect (art);
    }
}

void MainView::drawHeaders (juce::Graphics& g)
{
    struct Header { th::Icon icon; float x, y; juce::String title; float titleX; };
    const Header headers[] = {
        { th::Icon::wave,     188.0f, 156.0f, T ("OSCILLATOR"), 212.0f },
        { th::Icon::filter,   1124.0f, 156.0f, T ("FILTER"),     1148.0f },
        { th::Icon::knob,     1384.0f, 156.0f, T ("ENV"),        1408.0f },
        { th::Icon::matrix,   40.0f, 562.0f, T ("MODULATION"),   64.0f },
        { th::Icon::effects,  620.0f, 562.0f, T ("EFFECTS"),     644.0f },
        { th::Icon::gear,     1119.0f, 562.0f, T ("FX CHAIN"),   1143.0f },
        { th::Icon::knob,     1482.0f, 562.0f, T ("VOLUME"),     1506.0f }
    };

    for (const auto& h : headers)
    {
        th::drawIcon (g, h.icon, { h.x, h.y, 17.0f, 17.0f }, th::ink.withAlpha (0.85f), 1.5f);
        th::drawTrackedText (g, h.title, th::font (12.5f, 3), th::ink,
                             { h.titleX, h.y, 240.0f, 17.0f }, 1.2f, juce::Justification::centredLeft);
    }

    // small target crosses
    th::drawIcon (g, th::Icon::target, { 462.0f, 152.0f, 18.0f, 18.0f }, th::ink.withAlpha (0.55f), 1.2f);
    th::drawIcon (g, th::Icon::target, { 1610.0f, 152.0f, 18.0f, 18.0f }, th::ink.withAlpha (0.35f), 1.2f);

    // logo block
    drawLeafMark (g, { 58.0f, 20.0f, 78.0f, 66.0f });

    g.setFont (th::font (44.0f, 4));
    g.setColour (th::ink);
    g.drawText ("Botanic", juce::Rectangle<float> (150.0f, 26.0f, 360.0f, 58.0f), juce::Justification::centredLeft, false);

    g.setFont (th::font (16.0f, 1));
    g.setColour (th::inkSoft.withAlpha (0.9f));
    g.drawText ("Hybrid Synthesizer", juce::Rectangle<float> (152.0f, 82.0f, 300.0f, 22.0f), juce::Justification::centredLeft, false);

    th::drawTrackedText (g, "NATURE INSPIRES", th::font (9.5f, 1), th::inkSoft.withAlpha (0.8f),
                         { 424.0f, 54.0f, 140.0f, 12.0f }, 0.9f);
    th::drawTrackedText (g, "SOUND", th::font (9.5f, 1), th::inkSoft.withAlpha (0.8f),
                         { 424.0f, 68.0f, 140.0f, 12.0f }, 0.9f);

    th::drawTrackedText (g, "INSPIRED", th::font (9.5f, 1), th::inkSoft.withAlpha (0.75f),
                         { 1468.0f, 852.0f, 120.0f, 12.0f }, 0.9f);
    th::drawTrackedText (g, "BY NATURE", th::font (9.5f, 1), th::inkSoft.withAlpha (0.75f),
                         { 1468.0f, 866.0f, 120.0f, 12.0f }, 0.9f);

}

void MainView::rebuildChrome()
{
    chromeCache = juce::Image (juce::Image::ARGB, designWidth, designHeight, true);
    juce::Graphics cg (chromeCache);

    drawBackground (cg);
    drawPanels (cg);
    drawHeaders (cg);
}

void MainView::paint (juce::Graphics& g)
{
    if (! chromeCache.isValid())
        rebuildChrome();

    g.drawImageAt (chromeCache, 0, 0);

    g.setFont (th::font (11.5f, 1));
    g.setColour (th::inkSoft);
    g.drawText (Params::formatValue (Params::Fmt::Decibels, proc.paramValue (Params::output), 1),
                juce::Rectangle<float> (1532.0f, 700.0f, 58.0f, 16.0f), juce::Justification::centred, false);
}

//==============================================================================
/** In-app save dialog so the text is always drawn with our own font and colours. */
class MainView::SaveDialog : public juce::Component
{
public:
    SaveDialog (BotanicAudioProcessor& processor, std::function<void()> closeCallback)
        : proc (processor), close (std::move (closeCallback))
    {
        setInterceptsMouseClicks (true, true);

        editor.setText (proc.getPresetName());
        editor.setSelectAllWhenFocused (true);
        editor.setFont (th::font (15.0f, 1));
        editor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
        editor.setColour (juce::TextEditor::textColourId, th::ink);
        editor.setColour (juce::TextEditor::outlineColourId, th::green.withAlpha (0.45f));
        editor.setColour (juce::TextEditor::focusedOutlineColourId, th::greenDeep);
        editor.setColour (juce::TextEditor::highlightColourId, th::green.withAlpha (0.25f));
        editor.onReturnKey = [this] { commit(); };
        editor.onEscapeKey = [this] { close(); };
        addAndMakeVisible (editor);

        saveButton.setText (T ("Save"));
        saveButton.onClick = [this] { commit(); };
        addAndMakeVisible (saveButton);

        cancelButton.setText (T ("Cancel"));
        cancelButton.setAccent (th::inkSoft);
        cancelButton.onClick = [this] { close(); };
        addAndMakeVisible (cancelButton);
    }

    juce::Rectangle<float> cardBounds() const
    {
        return getLocalBounds().toFloat().withSizeKeepingCentre (520.0f, 250.0f);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff10160c).withAlpha (0.35f));

        auto card = cardBounds();
        g.setColour (juce::Colour (0xfffdfefb));
        g.fillRoundedRectangle (card, 20.0f);
        g.setColour (th::green.withAlpha (0.35f));
        g.drawRoundedRectangle (card.reduced (0.5f), 20.0f, 1.4f);

        g.setFont (th::fontFor (T ("Save preset"), 18.0f, 2));
        g.setColour (th::ink);
        g.drawText (T ("Save preset"), card.withTrimmedTop (24.0f).withHeight (26.0f).reduced (28.0f, 0.0f),
                    juce::Justification::centredLeft, false);

        g.setFont (th::font (11.5f, 1));
        g.setColour (th::inkSoft);
        g.drawText (T ("Enter a name for this patch"),
                    card.withTrimmedTop (54.0f).withHeight (20.0f).reduced (28.0f, 0.0f),
                    juce::Justification::centredLeft, false);

        g.setFont (th::font (10.5f, 1));
        g.setColour (th::ink.withAlpha (0.55f));
        g.drawText (T ("Name"), card.withTrimmedTop (86.0f).withHeight (18.0f).reduced (28.0f, 0.0f),
                    juce::Justification::centredLeft, false);

        if (errorText.isNotEmpty())
        {
            g.setFont (th::fontFor (errorText, 11.0f, 1));
            g.setColour (juce::Colour (0xffb3413a));
            g.drawText (errorText, card.withTrimmedTop (158.0f).withHeight (20.0f).reduced (28.0f, 0.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    void resized() override
    {
        auto card = cardBounds();
        editor.setBounds (card.withTrimmedTop (106.0f).withHeight (38.0f).reduced (28.0f, 0.0f).toNearestInt());
        saveButton.setBounds (card.getRight() - 260.0f, card.getBottom() - 58.0f, 110.0f, 34.0f);
        cancelButton.setBounds (card.getRight() - 140.0f, card.getBottom() - 58.0f, 110.0f, 34.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! cardBounds().contains (e.position))
            close();
    }

    void visibilityChanged() override
    {
        if (isVisible())
            editor.grabKeyboardFocus();
    }

private:
    void commit()
    {
        auto name = editor.getText().trim();

        if (name.isEmpty())
        {
            errorText = T ("Please enter a name");
            repaint();
            return;
        }

        if (! proc.savePresetAs (name))
        {
            errorText = T ("A preset with that name already exists");
            repaint();
            return;
        }

        close();
    }

    BotanicAudioProcessor& proc;
    std::function<void()> close;
    juce::TextEditor editor;
    TextPillButton saveButton, cancelButton;
    juce::String errorText;
};

void MainView::openSaveDialog()
{
    if (saveDialog != nullptr)
        return;

    saveDialog = std::make_unique<SaveDialog> (proc, [this]
    {
        saveDialog = nullptr;
    });

    addAndMakeVisible (*saveDialog);
    saveDialog->setBounds (getLocalBounds());
    saveDialog->toFront (true);
}

void MainView::updateABButtons()
{
    abButtonA.setAccent (abActive == 0 ? th::greenDeep : th::muted);
    abButtonB.setAccent (abActive == 1 ? th::greenDeep : th::muted);
}

void MainView::switchAB (int index)
{
    if (index == abActive)
        return;

    if (abSlot[abActive] == nullptr)
        abSlot[abActive] = std::make_unique<juce::ValueTree> (proc.getFullState());
    else
        *abSlot[abActive] = proc.getFullState();

    if (abSlot[index] == nullptr || ! abSlot[index]->isValid())
        abSlot[index] = std::make_unique<juce::ValueTree> (proc.getFullState());
    else
        proc.setFullState (*abSlot[index]);

    abActive = index;
    updateABButtons();
}

void MainView::showRandomMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (T ("Generative randomiser"));
    menu.addItem (1, T ("Everything"));
    menu.addSeparator();
    menu.addItem (2, T ("Oscillators"));
    menu.addItem (3, T ("Filter + envelopes"));
    menu.addItem (4, T ("Modulation"));
    menu.addItem (5, T ("Effects"));
    menu.addItem (6, T ("Texture (grain / glitch)"));
    menu.addItem (7, T ("Arpeggiator"));

    juce::Component::SafePointer<MainView> safe (this);
    auto options = juce::PopupMenu::Options()
                     .withTargetComponent (&randomButton)
                     .withMinimumWidth (190)
                     .withStandardItemHeight (26);

    menu.showMenuAsync (options, [safe] (int result)
    {
        if (safe == nullptr || result <= 0)
            return;

        safe->proc.randomize (result - 1);
        safe->proc.takeUndoSnapshot();
    });
}

void MainView::animate()
{
    const auto now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const auto dt = lastAnimTime > 0.0 ? juce::jlimit (0.0, 0.1, now - lastAnimTime) : 0.016;
    lastAnimTime = now;

    if (stagePage == 0)
        stageArt.animate (dt);

    keyboard.tick (dt);
}

void MainView::refreshAll()
{
    masterKnob.refresh();
    presetBar.refresh();
    power.refresh();

    ++refreshCounter;
    wtDisplay.refresh();
    oscKnob1.refresh();
    oscKnob2.refresh();
    oscKnob3.refresh();
    tablePill.refresh();

    filterPill.refresh();
    filterDisplay.refresh();
    filtKnob1.refresh();
    filtKnob2.refresh();
    filtKnob3.refresh();

    envDisplay.refresh();
    envFader1.refresh();
    envFader2.refresh();
    envFader3.refresh();
    envFader4.refresh();

    lfoDisplay.refresh();
    modEnvDisplay.refresh();
    syncSwitch.refresh();
    shapePill.refresh();
    modKnob1.refresh();
    modKnob2.refresh();
    modKnob3.refresh();
    modKnob4.refresh();

    fxKnob1.refresh();
    fxKnob2.refresh();
    fxKnob3.refresh();
    fxKnob2.setDimmed (currentFxTab == 1 && proc.paramValue (Params::dlySync) > 0.5f);
    chainPanel.refresh();
    volumeFader.refresh();
    tiltKnob.refresh();
    widthKnob.refresh();
    airKnob.refresh();
    levelMeter.setLevels (proc.getPeakL(), proc.getPeakR());

    modePill.refresh();
    voicesPill.refresh();
    tuningPill.refresh();

    keyboard.setHeldNotes (proc.getHeldNotes());

    if (matrixPanel.isVisible())   matrixPanel.refresh();
    if (arpPanel.isVisible())      arpPanel.refresh();
    if (settingsPanel.isVisible()) settingsPanel.refresh();
    if (presetPanel.isVisible())   presetPanel.refresh();
    if (oscPage.isVisible())       oscPage.refresh();
    if (filterPage.isVisible())    filterPage.refresh();
    if (fxPage.isVisible())        fxPage.refresh();
    if (texturePanel.isVisible())  texturePanel.refresh();
    if (macroPanel.isVisible())    macroPanel.refresh();

    if (refreshCounter % 2 == 0)
        wtDisplay.repaint();

    if (refreshCounter % 2 == 0)
        lfoDisplay.repaint();
}

} // namespace ui
