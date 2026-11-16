#include "Widgets.h"
#include "../ModMatrix.h"

namespace ui
{

using namespace botanic;

namespace
{
    constexpr float startAngle = juce::MathConstants<float>::pi * 0.75f;
    constexpr float sweepAngle = juce::MathConstants<float>::pi * 1.5f;

    int modDestForParam (const juce::String& id)
    {
        for (int d = 0; d < numModDests; ++d)
        {
            const auto* pid = modDestParamId (d);
            if (pid != nullptr && id == pid)
                return d;
        }
        return -1;
    }

    void drawChevron (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, float thickness = 1.6f)
    {
        juce::Path p;
        p.startNewSubPath (area.getX(), area.getCentreY() - area.getHeight() * 0.22f);
        p.lineTo (area.getCentreX(), area.getCentreY() + area.getHeight() * 0.24f);
        p.lineTo (area.getRight(), area.getCentreY() - area.getHeight() * 0.22f);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
Knob::Knob (BotanicAudioProcessor& processor, const juce::String& paramId,
            const juce::String& captionText, float diameterRef)
    : proc (processor), paramID (paramId), caption (captionText), diameter (diameterRef)
{
    param = proc.apvts.getParameter (paramId);
    setInterceptsMouseClicks (true, false);
    setTooltip ((param != nullptr ? param->getName (64) : paramId) + "\n"
                + T ("drag to adjust - shift for fine - right click to modulate"));
    refresh();
    setSize ((int) std::ceil (diameter), (int) std::ceil (diameter + labelHeight));
}

float Knob::currentNorm() const
{
    return param != nullptr ? param->getValue() : 0.0f;
}

void Knob::refresh()
{
    if (param == nullptr)
        return;

    const auto n = param->getValue();
    const auto text = param->getCurrentValueAsText();

    if (std::abs (n - normValue) > 1.0e-5f || text != displayText)
    {
        normValue = n;
        displayText = text;
        repaint();
    }
}

void Knob::setDimmed (bool shouldDim)
{
    if (dimmed != shouldDim)
    {
        dimmed = shouldDim;
        repaint();
    }
}

void Knob::setParameter (const juce::String& newId, const juce::String& newCaption)
{
    paramID = newId;
    caption = newCaption;
    param = proc.apvts.getParameter (newId);
    normValue = -1.0f;
    displayText.clear();
    refresh();
    repaint();
}

void Knob::paint (juce::Graphics& g)
{
    const auto c = juce::Point<float> (getWidth() * 0.5f, diameter * 0.5f);
    const auto outerR = diameter * 0.5f - 1.0f;
    const auto ringR = outerR - 3.4f;
    const auto bodyR = outerR - 10.5f;
    const auto alpha = dimmed ? 0.45f : 1.0f;

    //--- value track
    juce::Path track;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, startAngle + sweepAngle, true);
    g.setColour (juce::Colour (0xffd6dad0).withAlpha (0.9f * alpha));
    g.strokePath (track, juce::PathStrokeType (6.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    //--- modulation range
    const auto dest = modDestForParam (paramID);

    if (dest >= 0 && param != nullptr)
    {
        float neg = 0.0f, pos = 0.0f;

        for (const auto& slot : proc.modMatrix.getSlots())
        {
            if (slot.source < 0 || slot.dest != dest)
                continue;
            if (slot.amount > 0.0f) pos += slot.amount;
            else neg += slot.amount;
        }

        if (std::abs (neg) > 0.0001f || std::abs (pos) > 0.0001f)
        {
            const auto a0 = startAngle + juce::jlimit (0.0f, 1.0f, normValue + neg) * sweepAngle;
            const auto a1 = startAngle + juce::jlimit (0.0f, 1.0f, normValue + pos) * sweepAngle;
            juce::Path modPath;
            modPath.addCentredArc (c.x, c.y, ringR + 5.0f, ringR + 5.0f, 0.0f, a0, a1, true);
            g.setColour (th::green.withAlpha (0.55f * alpha));
            g.strokePath (modPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    //--- value arc
    if (normValue > 0.0005f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, startAngle + sweepAngle * normValue, true);
        g.setGradientFill (juce::ColourGradient (th::greenMid.withAlpha (alpha), c.x - ringR, c.y - ringR,
                                                 th::greenDeep.withAlpha (alpha), c.x + ringR, c.y + ringR, false));
        g.strokePath (arc, juce::PathStrokeType (6.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //--- body
    {
        juce::DropShadow ds (juce::Colours::black.withAlpha (0.22f * alpha), 8, { 0, 3 });
        juce::Path body;
        body.addEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        ds.drawForPath (g, body);

        juce::ColourGradient bodyGrad (juce::Colour (0xfffdfdfb).withAlpha (alpha), c.x - bodyR * 0.45f, c.y - bodyR * 0.55f,
                                       juce::Colour (0xffa9aca4).withAlpha (alpha), c.x + bodyR * 0.55f, c.y + bodyR * 0.65f, true);
        bodyGrad.addColour (0.55, juce::Colour (0xffe7e8e3).withAlpha (alpha));
        g.setGradientFill (bodyGrad);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.55f * alpha));
        g.drawEllipse (c.x - bodyR + 0.6f, c.y - bodyR + 0.6f, bodyR * 2.0f - 1.2f, bodyR * 2.0f - 1.2f, 1.0f);

        // brushed metal rings
        for (float rr : { 0.82f, 0.62f, 0.4f })
        {
            g.setColour (juce::Colour (0xff9ea198).withAlpha (0.20f * alpha));
            g.drawEllipse (c.x - bodyR * rr, c.y - bodyR * rr, bodyR * rr * 2.0f, bodyR * rr * 2.0f, 0.7f);
        }

        // indicator
        const auto angle = startAngle + sweepAngle * normValue;
        g.setColour (th::greenDeep.withAlpha (alpha));
        g.drawLine (c.x + std::cos (angle) * bodyR * 0.24f, c.y + std::sin (angle) * bodyR * 0.24f,
                    c.x + std::cos (angle) * bodyR * 0.88f, c.y + std::sin (angle) * bodyR * 0.88f, 2.4f);

        g.setColour (juce::Colour (0xff2f5c0c).withAlpha (alpha));
        g.fillEllipse (c.x + std::cos (angle) * bodyR * 0.88f - 2.0f,
                       c.y + std::sin (angle) * bodyR * 0.88f - 2.0f, 4.0f, 4.0f);
    }

    //--- labels
    g.setFont (th::font (13.0f, 1));
    g.setColour (th::inkSoft.withAlpha (0.95f * alpha));
    g.drawText (caption, juce::Rectangle<float> (0, diameter + 4.0f, (float) getWidth(), 15.0f),
                juce::Justification::centred, false);

    g.setFont (th::font (14.0f, 2));
    g.setColour (th::ink.withAlpha (alpha));
    g.drawText (displayText, juce::Rectangle<float> (0, diameter + 20.0f, (float) getWidth(), 17.0f),
                juce::Justification::centred, false);
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (param == nullptr)
        return;

    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    dragStartNorm = currentNorm();
    dragging = true;
    if (auto* parent = getParentComponent())
        parent->repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (param == nullptr || ! dragging)
        return;

    const auto sensitivity = e.mods.isShiftDown() ? 0.0009f : 0.0038f;
    const auto delta = -(float) e.getDistanceFromDragStartY() * sensitivity;
    const auto n = juce::jlimit (0.0f, 1.0f, dragStartNorm + delta);
    param->setValueNotifyingHost (n);
    refresh();
}

void Knob::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        dragging = false;
        proc.takeUndoSnapshot();
    }
}

void Knob::mouseDoubleClick (const juce::MouseEvent&)
{
    if (param != nullptr)
    {
        param->setValueNotifyingHost (param->getDefaultValue());
        proc.takeUndoSnapshot();
        refresh();
    }
}

void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (param == nullptr)
        return;

    const auto delta = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f) * 0.04f;
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, currentNorm() + delta));
    refresh();
    proc.takeUndoSnapshot();
}

void Knob::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void Knob::mouseExit (const juce::MouseEvent&)  { hovered = false; repaint(); }
void Knob::mouseMove (const juce::MouseEvent&)  {}

void Knob::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (param != nullptr ? param->getName (32) : paramID);
    menu.addItem (1, T ("Reset to default"));
    menu.addSeparator();

    const auto dest = modDestForParam (paramID);

    if (dest >= 0)
    {
        juce::PopupMenu modMenu;

        struct Src { int index; const char* label; };
        const Src sources[] = { { 0, "LFO 1" }, { 1, "LFO 2" }, { 2, "ENV 1" }, { 3, "ENV 2" },
                                { 4, "Velocity" }, { 5, "Mod Wheel" }, { 6, "Key Track" }, { 7, "Random" },
                                { 8, "Aftertouch" }, { 9, "Chaos" } };

        for (const auto& s : sources)
        {
            const auto current = proc.modMatrix.get (s.index, dest);
            const auto assigned = std::abs (current) > 0.0001f;
            modMenu.addItem (100 + s.index, T (s.label) + (assigned ? juce::String (juce::CharPointer_UTF8 ("  \xe7\xa7\xbb\xe9\x99\xa4")) : juce::String ("  +50%")), true, assigned);
        }

        modMenu.addSeparator();
        modMenu.addItem (200, T ("Clear all modulation"));

        menu.addSubMenu (T ("Modulate"), modMenu);
    }
    else
    {
        menu.addItem (300, T ("Not a modulation target"), false);
    }

    menu.addItem (400, T ("MIDI Learn"), true, proc.getLearningParam() == paramID);

    if (proc.getMappedCC (paramID).isNotEmpty())
        menu.addItem (401, T ("Clear MIDI mapping"));

    auto safe = juce::Component::SafePointer<Knob> (this);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safe] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;

                            if (result == 1)
                            {
                                safe->param->setValueNotifyingHost (safe->param->getDefaultValue());
                                safe->proc.takeUndoSnapshot();
                                safe->refresh();
                                return;
                            }

                            if (result >= 100 && result < 200)
                            {
                                const auto dest = modDestForParam (safe->paramID);
                                if (dest < 0)
                                    return;
                                const auto source = result - 100;
                                const auto current = safe->proc.modMatrix.get (source, dest);
                                safe->proc.modMatrix.set (source, dest, std::abs (current) > 0.0001f ? 0.0f : 0.5f);
                                safe->proc.takeUndoSnapshot();
                                safe->repaint();
                                return;
                            }

                            if (result == 200)
                            {
                                const auto dest = modDestForParam (safe->paramID);
                                for (int s = 0; s < numModSources; ++s)
                                    if (dest >= 0)
                                        safe->proc.modMatrix.set (s, dest, 0.0f);
                                safe->proc.takeUndoSnapshot();
                                safe->repaint();
                                return;
                            }

                            if (result == 400)
                                safe->proc.startMidiLearn (safe->paramID);
                            else if (result == 401)
                                safe->proc.clearMidiMapping (safe->paramID);
                        });
}

//==============================================================================
TabStrip::TabStrip (std::vector<juce::String> labels)
    : items (std::move (labels))
{
    setInterceptsMouseClicks (true, false);
}

void TabStrip::setSelected (int index, bool notify)
{
    if (items.empty())
        return;

    index = juce::jlimit (0, (int) items.size() - 1, index);

    if (index != selected)
    {
        selected = index;
        repaint();

        if (notify && onChange != nullptr)
            onChange (index);
    }
}

int TabStrip::indexAt (float x) const
{
    const auto n = (int) items.size();

    if (n <= 0)
        return -1;

    const auto w = (float) getWidth() / (float) n;
    return juce::jlimit (0, n - 1, (int) (x / w));
}

void TabStrip::paint (juce::Graphics& g)
{
    const auto n = (int) items.size();

    if (n <= 0)
        return;

    const auto w = (float) getWidth() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> ((float) i * w + 2.0f, 0.0f, w - 4.0f, (float) getHeight());
        const auto active = i == selected;

        th::drawPill (g, r, active);
        th::drawPillText (g, items[(size_t) i], r, active, 12.5f);

        if (! active && hovered == i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);
        }
    }
}

void TabStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto i = indexAt (e.position.x);

    if (i >= 0 && i != selected)
    {
        selected = i;
        repaint();

        if (onChange != nullptr)
            onChange (i);
    }
}

void TabStrip::mouseMove (const juce::MouseEvent& e)
{
    const auto i = indexAt (e.position.x);

    if (i != hovered)
    {
        hovered = i;
        repaint();
    }
}

void TabStrip::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

//==============================================================================
ChoicePill::ChoicePill (BotanicAudioProcessor& processor, const juce::String& paramId, float corner)
    : proc (processor), paramID (paramId), radius (corner)
{
    param = proc.apvts.getParameter (paramId);
    setInterceptsMouseClicks (true, false);
    refresh();
}

ChoicePill::ChoicePill (BotanicAudioProcessor& processor, const juce::String& paramId,
                        juce::StringArray customLabels, std::vector<float> customValues)
    : proc (processor), paramID (paramId), labels (std::move (customLabels)),
      values (std::move (customValues)), customValues (true)
{
    param = proc.apvts.getParameter (paramId);
    setInterceptsMouseClicks (true, false);
    refresh();
}

void ChoicePill::refresh()
{
    if (param == nullptr)
        return;

    juce::String text;

    if (customValues)
    {
        const auto v = param->convertFrom0to1 (param->getValue());
        int best = 0;
        for (int i = 0; i < (int) values.size(); ++i)
            if (std::abs (values[(size_t) i] - v) < std::abs (values[(size_t) best] - v))
                best = i;
        text = labels[best];
    }
    else
    {
        text = param->getCurrentValueAsText();
        const auto translated = T (text.toRawUTF8());

        if (translated != text)
            text = translated;
    }

    if (text != displayText)
    {
        displayText = text;
        repaint();
    }
}

void ChoicePill::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    g.setColour (juce::Colours::white.withAlpha (hovered ? 0.72f : 0.52f));
    g.fillRoundedRectangle (r, radius);
    g.setColour (th::ink.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

    g.setFont (th::font (13.0f, 1));
    g.setColour (th::ink);
    g.drawText (displayText, r.withTrimmedLeft (14.0f).withTrimmedRight (26.0f), juce::Justification::centredLeft, false);

    drawChevron (g, juce::Rectangle<float> (r.getRight() - 22.0f, r.getCentreY() - 5.0f, 11.0f, 10.0f),
                 th::inkSoft.withAlpha (0.75f), 1.5f);
}

void ChoicePill::showMenu()
{
    if (param == nullptr)
        return;

    juce::PopupMenu menu;

    if (customValues)
    {
        const auto current = param->convertFrom0to1 (param->getValue());
        int best = 0;
        for (int i = 0; i < (int) values.size(); ++i)
            if (std::abs (values[(size_t) i] - current) < std::abs (values[(size_t) best] - current))
                best = i;

        for (int i = 0; i < (int) values.size(); ++i)
            menu.addItem (i + 1, labels[i], true, i == best);
    }
    else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        const auto current = choice->getIndex();

        for (int i = 0; i < choice->choices.size(); ++i)
            menu.addItem (i + 1, choice->choices[i], true, i == current);
    }

    auto safe = juce::Component::SafePointer<ChoicePill> (this);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe] (int result)
                        {
                            if (safe == nullptr || result <= 0)
                                return;

                            if (safe->customValues)
                            {
                                const auto index = juce::jlimit (0, (int) safe->values.size() - 1, result - 1);
                                if (safe->param != nullptr)
                                    safe->param->setValueNotifyingHost (safe->param->convertTo0to1 (safe->values[(size_t) index]));
                            }
                            else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (safe->param))
                            {
                                choice->setValueNotifyingHost (choice->convertTo0to1 ((float) (result - 1)));
                            }

                            safe->proc.takeUndoSnapshot();
                            safe->refresh();
                        });
}

void ChoicePill::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        showMenu();
}

void ChoicePill::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void ChoicePill::mouseExit (const juce::MouseEvent&)  { hovered = false; repaint(); }

//==============================================================================
ToggleSwitch::ToggleSwitch (BotanicAudioProcessor& processor, const juce::String& paramId, const juce::String& labelText)
    : proc (processor), paramID (paramId), label (labelText)
{
    param = proc.apvts.getParameter (paramId);
    setInterceptsMouseClicks (true, false);
    refresh();
}

void ToggleSwitch::refresh()
{
    if (param == nullptr)
        return;

    const auto s = param->getValue() > 0.5f;

    if (s != state)
    {
        state = s;
        repaint();
    }
}

void ToggleSwitch::setParameter (const juce::String& newId, const juce::String& newLabel)
{
    paramID = newId;
    label = newLabel;
    param = proc.apvts.getParameter (newId);
    refresh();
    repaint();
}

void ToggleSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto switchW = 44.0f;
    auto switchR = juce::Rectangle<float> (r.getRight() - switchW, r.getCentreY() - 11.0f, switchW, 22.0f);

    if (label.isNotEmpty())
    {
        g.setFont (th::font (13.0f, 1));
        g.setColour (th::inkSoft);
        g.drawText (label, r.withTrimmedRight (switchW + 6.0f), juce::Justification::centredLeft, false);
    }

    g.setColour (state ? th::greenDark : juce::Colour (0xffc9ccc4));
    g.fillRoundedRectangle (switchR, 11.0f);

    const auto knobX = state ? switchR.getRight() - 11.0f : switchR.getX() + 11.0f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (knobX - 8.5f, switchR.getCentreY() - 8.5f, 17.0f, 17.0f);
}

void ToggleSwitch::mouseDown (const juce::MouseEvent&)
{
    if (param != nullptr)
    {
        param->setValueNotifyingHost (state ? 0.0f : 1.0f);
        refresh();
        proc.takeUndoSnapshot();
    }
}

//==============================================================================
PowerSwitch::PowerSwitch (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, false);
    refresh();
}

void PowerSwitch::refresh()
{
    if (auto* p = proc.apvts.getParameter (Params::power))
    {
        const auto s = p->getValue() > 0.5f;

        if (s != on)
        {
            on = s;
            repaint();
        }
    }
}

void PowerSwitch::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto radius = r.getHeight() * 0.5f;

    if (on)
    {
        juce::DropShadow ds (th::green.withAlpha (0.4f), 12, { 0, 2 });
        juce::Path p;
        p.addRoundedRectangle (r, radius);
        ds.drawForPath (g, p);
    }

    g.setColour (on ? th::greenDark : juce::Colour (0xff4a4f45).withAlpha (0.55f));
    g.fillRoundedRectangle (r, radius);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    const auto knobX = on ? r.getRight() - radius - 3.0f : r.getX() + radius + 3.0f;
    g.fillEllipse (knobX - radius + 5.0f, r.getCentreY() - radius + 5.0f, (radius - 5.0f) * 2.0f, (radius - 5.0f) * 2.0f);

    g.setFont (th::font (12.0f, 2));
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.drawText ("ON", r.withTrimmedLeft (on ? 6.0f : 24.0f).withTrimmedRight (on ? 24.0f : 6.0f),
                juce::Justification::centred, false);

    if (hovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.2f);
    }
}

void PowerSwitch::mouseDown (const juce::MouseEvent&)
{
    if (auto* p = proc.apvts.getParameter (Params::power))
    {
        p->setValueNotifyingHost (on ? 0.0f : 1.0f);
        refresh();
    }
}

void PowerSwitch::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void PowerSwitch::mouseExit (const juce::MouseEvent&)  { hovered = false; repaint(); }

//==============================================================================
IconButton::IconButton (th::Icon i, juce::Colour c, float iconScale)
    : icon (i), colour (c), scale (iconScale)
{
    setInterceptsMouseClicks (true, true);
}

void IconButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    if (hovered || pressed)
    {
        g.setColour (juce::Colours::white.withAlpha (pressed ? 0.65f : 0.45f));
        g.fillEllipse (r.getCentreX() - r.getHeight() * 0.5f, r.getCentreY() - r.getHeight() * 0.5f,
                       r.getHeight(), r.getHeight());
    }

    const auto s = r.getHeight() * scale;
    th::drawIcon (g, icon, r.withSizeKeepingCentre (s, s), colour, 1.5f);
}

void IconButton::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void IconButton::mouseExit (const juce::MouseEvent&)  { hovered = false; pressed = false; repaint(); }
void IconButton::mouseDown (const juce::MouseEvent&)  { pressed = true; repaint(); }

void IconButton::mouseUp (const juce::MouseEvent& e)
{
    pressed = false;
    repaint();

    if (hovered && getLocalBounds().toFloat().contains (e.position) && onClick != nullptr)
        onClick();
}

//==============================================================================
void LevelMeter::setLevels (float peakLeft, float peakRight)
{
    const auto decay = 0.86f;
    smoothedL = peakLeft > smoothedL ? peakLeft : smoothedL * decay;
    smoothedR = peakRight > smoothedR ? peakRight : smoothedR * decay;
    holdL = juce::jmax (holdL * 0.995f, smoothedL);
    holdR = juce::jmax (holdR * 0.995f, smoothedR);

    if (smoothedL > 0.0005f || smoothedR > 0.0005f || holdL > 0.0005f)
        repaint();
}

void LevelMeter::paintBar (juce::Graphics& g, juce::Rectangle<float> bar, float level)
{
    g.setColour (th::ink.withAlpha (0.08f));
    g.fillRoundedRectangle (bar, 2.0f);

    const auto norm = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels (level, -60.0f) / 60.0f + 1.0f);

    if (norm > 0.001f)
    {
        auto filled = bar.withWidth (bar.getWidth() * norm);
        juce::ColourGradient grad (th::greenMid, bar.getX(), bar.getY(), th::greenDeep, bar.getRight(), bar.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (filled, 2.0f);
    }
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto barH = (r.getHeight() - 2.0f) * 0.5f;
    paintBar (g, r.withHeight (barH), smoothedL);
    paintBar (g, r.withTrimmedTop (barH + 2.0f), smoothedR);
}

//==============================================================================
VerticalFader::VerticalFader (BotanicAudioProcessor& processor, const juce::String& paramId,
                              const juce::String& captionText, bool labels)
    : proc (processor), paramID (paramId), caption (captionText), showLabels (labels)
{
    param = proc.apvts.getParameter (paramId);
    setInterceptsMouseClicks (true, false);
    refresh();
}

juce::Rectangle<float> VerticalFader::trackArea() const
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();
    const auto labelSpace = showLabels ? 34.0f : 0.0f;
    return { w * 0.5f - 4.0f, 8.0f, 8.0f, juce::jmax (20.0f, h - labelSpace - 16.0f) };
}

void VerticalFader::setParameter (const juce::String& newId, const juce::String& newCaption)
{
    paramID = newId;
    caption = newCaption;
    param = proc.apvts.getParameter (newId);
    norm = -1.0f;
    refresh();
    repaint();
}

void VerticalFader::refresh()
{
    if (param == nullptr)
        return;

    const auto n = param->getValue();

    if (std::abs (n - norm) > 1.0e-5f)
    {
        norm = n;
        repaint();
    }
}

void VerticalFader::paint (juce::Graphics& g)
{
    auto track = trackArea();
    const auto alpha = dimmed ? 0.45f : 1.0f;

    g.setColour (juce::Colours::white.withAlpha (0.55f * alpha));
    g.fillRoundedRectangle (track, 4.0f);
    g.setColour (th::ink.withAlpha (0.09f * alpha));
    g.drawRoundedRectangle (track.reduced (0.5f), 4.0f, 1.0f);

    const auto knobY = track.getBottom() - norm * track.getHeight();
    auto fill = track.withTop (knobY);

    g.setGradientFill (juce::ColourGradient (th::greenMid.withAlpha (alpha), track.getX(), track.getY(),
                                             th::greenDark.withAlpha (alpha), track.getX(), track.getBottom(), false));
    g.fillRoundedRectangle (fill, 4.0f);

    juce::DropShadow ds (juce::Colours::black.withAlpha (0.3f * alpha), 6, { 0, 2 });
    juce::Path p;
    p.addEllipse (track.getCentreX() - 7.5f, knobY - 7.5f, 15.0f, 15.0f);
    ds.drawForPath (g, p);

    juce::ColourGradient knob (juce::Colour (0xff3c7415).withAlpha (alpha), track.getCentreX() - 5.0f, knobY - 6.0f,
                               juce::Colour (0xff8cc63f).withAlpha (alpha), track.getCentreX() + 5.0f, knobY + 6.0f, false);
    g.setGradientFill (knob);
    g.fillEllipse (track.getCentreX() - 7.5f, knobY - 7.5f, 15.0f, 15.0f);
    g.setColour (juce::Colours::white.withAlpha (0.6f * alpha));
    g.drawEllipse (track.getCentreX() - 7.0f, knobY - 7.0f, 14.0f, 14.0f, 1.0f);

    if (showLabels && param != nullptr)
    {
        const auto w = (float) getWidth();
        const auto labelY = track.getBottom() + 4.0f;

        g.setFont (th::font (11.5f, 1));
        g.setColour (th::inkSoft);
        g.drawText (caption, juce::Rectangle<float> (0, labelY, w, 15.0f), juce::Justification::centred, false);

        g.setFont (th::font (12.5f, 2));
        g.setColour (th::ink);
        g.drawText (param->getCurrentValueAsText(), juce::Rectangle<float> (0, labelY + 15.0f, w, 16.0f),
                    juce::Justification::centred, false);
    }
}

void VerticalFader::mouseDown (const juce::MouseEvent& e)
{
    if (param == nullptr)
        return;

    dragStart = param->getValue();

    if (! e.mods.isPopupMenu())
        mouseDrag (e);
}

void VerticalFader::mouseDrag (const juce::MouseEvent& e)
{
    if (param == nullptr)
        return;

    const auto track = trackArea();
    const auto n = juce::jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / track.getHeight());
    param->setValueNotifyingHost (n);
    refresh();
}

void VerticalFader::mouseDoubleClick (const juce::MouseEvent&)
{
    if (param != nullptr)
    {
        param->setValueNotifyingHost (param->getDefaultValue());
        refresh();
        proc.takeUndoSnapshot();
    }
}

//==============================================================================
TextPillButton::TextPillButton (juce::String buttonText, juce::Colour accentColour)
    : text (std::move (buttonText)), accent (accentColour)
{
    setInterceptsMouseClicks (true, false);
}

void TextPillButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto alpha = pressed ? 0.95f : (hovered ? 0.85f : 0.65f);

    g.setColour (th::greenPale.withAlpha (alpha));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (accent.withAlpha (hovered ? 0.8f : 0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.2f);

    auto textFont = th::fontFor (text, 11.0f, 2);
    const auto textW = textFont.getStringWidthFloat (text);
    const auto iconSize = hasIcon ? 15.0f : 0.0f;
    const auto gap = hasIcon ? 6.0f : 0.0f;
    const auto total = iconSize + gap + textW;
    auto x = r.getCentreX() - total * 0.5f;

    if (hasIcon)
    {
        th::drawIcon (g, icon, { x, r.getCentreY() - iconSize * 0.5f, iconSize, iconSize }, accent, 1.4f);
        x += iconSize + gap;
    }

    g.setFont (textFont);
    g.setColour (accent);
    g.drawText (text, juce::Rectangle<float> (x, r.getY(), textW + 2.0f, r.getHeight()),
                juce::Justification::centredLeft, false);
}

void TextPillButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown())
    {
        suppressClick = true;

        if (onSecondaryClick != nullptr)
            onSecondaryClick();

        return;
    }

    suppressClick = false;
    pressed = true;
    repaint();
}

void TextPillButton::mouseUp (const juce::MouseEvent& e)
{
    pressed = false;
    repaint();

    if (suppressClick)
    {
        suppressClick = false;
        return;
    }

    if (getLocalBounds().toFloat().contains (e.position) && onClick != nullptr)
        onClick();
}

void TextPillButton::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void TextPillButton::mouseExit (const juce::MouseEvent&)  { hovered = false; pressed = false; repaint(); }

//==============================================================================
juce::StringArray NavRail::itemNames()
{
    return { T ("Home"), T ("Oscillator"), T ("Filter"), T ("Modulation"),
             T ("Effects"), T ("Texture"), T ("Perform"), T ("Arpeggiator"), T ("Settings") };
}

NavRail::NavRail (BotanicAudioProcessor& processor) : proc (processor)
{
    setInterceptsMouseClicks (true, false);
}

juce::Rectangle<float> NavRail::itemBounds (int index) const
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();
    const auto top = 14.0f;
    const auto usable = h - 26.0f;
    const auto itemH = usable / (float) numItems;
    return { 8.0f, top + itemH * (float) index, w - 16.0f, juce::jmin (42.0f, itemH - 6.0f) };
}

void NavRail::setSelected (int index)
{
    if (index != selected)
    {
        selected = index;
        repaint();
    }
}

void NavRail::paint (juce::Graphics& g)
{
    th::drawPanel (g, getLocalBounds().toFloat(), 20.0f, 0.62f);

    const auto names = itemNames();
    static const th::Icon icons[] = { th::Icon::home, th::Icon::wave, th::Icon::filter, th::Icon::knob,
                                      th::Icon::effects, th::Icon::grain, th::Icon::target,
                                      th::Icon::arp, th::Icon::gear };

    for (int i = 0; i < names.size(); ++i)
    {
        auto r = itemBounds (i);
        const auto active = i == selected;

        if (active)
        {
            juce::DropShadow ds (th::green.withAlpha (0.45f), 14, { 0, 4 });
            juce::Path p;
            p.addRoundedRectangle (r, r.getHeight() * 0.5f);
            ds.drawForPath (g, p);

            g.setGradientFill (th::activeGradient (r));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        }
        else if (hovered == i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        }

        const auto iconR = juce::Rectangle<float> (r.getX() + 12.0f, r.getCentreY() - 9.0f, 18.0f, 18.0f);
        th::drawIcon (g, icons[i], iconR, active ? juce::Colours::white : th::inkSoft.withAlpha (0.85f), 1.5f);

        g.setFont (th::font (13.0f, active ? 2 : 1));
        g.setColour (active ? juce::Colours::white : th::inkSoft);
        g.drawText (names[i], r.withTrimmedLeft (38.0f), juce::Justification::centredLeft, false);
    }
}

void NavRail::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < numItems; ++i)
    {
        if (itemBounds (i).contains (e.position))
        {
            setSelected (i);

            if (onSelect != nullptr)
                onSelect (i);

            return;
        }
    }
}

void NavRail::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;

    for (int i = 0; i < numItems; ++i)
        if (itemBounds (i).contains (e.position))
            h = i;

    if (h != hovered)
    {
        hovered = h;
        repaint();
    }

    juce::ignoreUnused (proc);
}

void NavRail::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

} // namespace ui
