#pragma once

#include <JuceHeader.h>
#include "BinaryData.h"
#include "Loc.h"

/** Visual language of Botanic: colours, fonts and reusable painting helpers. */
namespace th
{
    //==============================================================================
    inline const juce::Colour ink        { 0xff161c11 };
    inline const juce::Colour inkSoft    { 0xff39412f };
    inline const juce::Colour muted      { 0xff6c7563 };
    inline const juce::Colour greenDeep  { 0xff2f5c0c };
    inline const juce::Colour greenDark  { 0xff3f7a12 };
    inline const juce::Colour green      { 0xff5f9f1c };
    inline const juce::Colour greenMid   { 0xff87c33c };
    inline const juce::Colour greenLight { 0xffc3e59c };
    inline const juce::Colour greenPale  { 0xffe7f3d6 };
    inline const juce::Colour paper      { 0xfff3f4ee };
    inline const juce::Colour cardFill   { 0xffffffff };

    //==============================================================================
    inline juce::Typeface::Ptr typeface (int weight)
    {
        static juce::Typeface::Ptr regular, medium, semi, bold, logo;

        switch (weight)
        {
            case 1: if (medium == nullptr) medium = juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, (size_t) BinaryData::InterMedium_ttfSize); return medium;
            case 2: if (semi   == nullptr) semi   = juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf, (size_t) BinaryData::InterSemiBold_ttfSize); return semi;
            case 3: if (bold   == nullptr) bold   = juce::Typeface::createSystemTypefaceFor (BinaryData::InterBold_ttf, (size_t) BinaryData::InterBold_ttfSize); return bold;
            case 4: if (logo   == nullptr) logo   = juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsBold_ttf, (size_t) BinaryData::PoppinsBold_ttfSize); return logo;
            default:
                if (regular == nullptr) regular = juce::Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf, (size_t) BinaryData::InterRegular_ttfSize);
                return regular;
        }
    }

    /** weight: 0 regular, 1 medium, 2 semibold, 3 bold, 4 logo (Poppins) */
    inline juce::Font font (float size, int weight = 0)
    {
        if (weight == 4)
        {
            juce::Font logoFont (typeface (4));
            logoFont.setHeight (size);
            return logoFont;
        }

        // CJK glyphs are not in the embedded latin fonts, fall back to a system family
        if (ui::Locale::isChinese())
        {
            juce::Font cjk (juce::Font (juce::String::fromUTF8 ("\xe5\xbe\xae\xe8\xbd\xaf\xe9\x9b\x85\xe9\xbb\x91"),
                                        size, weight >= 2 ? juce::Font::bold : juce::Font::plain));

            if (cjk.getTypefaceName().isNotEmpty())
                return cjk;
        }

        juce::Font f (typeface (weight));
        f.setHeight (size);
        return f;
    }

    inline juce::Font tracked (float size, int weight = 1) { return font (size, weight); }

    /** Picks a family that can actually render the given text. */
    inline juce::Font fontFor (const juce::String& text, float size, int weight = 0)
    {
        bool nonAscii = false;

        for (auto c : text)
            if ((int) c > 127) { nonAscii = true; break; }

        if (nonAscii)
        {
            juce::Font cjk (juce::Font (juce::String::fromUTF8 ("\xe5\xbe\xae\xe8\xbd\xaf\xe9\x9b\x85\xe9\xbb\x91"),
                                        size, weight >= 2 ? juce::Font::bold : juce::Font::plain));

            if (cjk.getTypefaceName().isNotEmpty())
                return cjk;
        }

        return font (size, weight);
    }

    //==============================================================================
    inline void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Font f,
                                 juce::Colour colour, juce::Rectangle<float> area,
                                 float tracking = 0.5f, juce::Justification just = juce::Justification::centredLeft)
    {
        g.setFont (f);
        g.setColour (colour);

        const auto widths = [&]
        {
            std::vector<float> w;
            float total = 0.0f;
            for (auto c : text)
            {
                const auto cw = f.getStringWidthFloat (juce::String::charToString (c));
                w.push_back (cw);
                total += cw + tracking;
            }
            total -= tracking;
            return std::pair<std::vector<float>, float> { w, total };
        }();

        const auto total = widths.second;
        auto x = area.getX();

        if ((just.getFlags() & juce::Justification::horizontallyCentred) != 0)
            x = area.getCentreX() - total * 0.5f;
        else if ((just.getFlags() & juce::Justification::right) != 0)
            x = area.getRight() - total;

        const auto baseline = area.getCentreY() + f.getAscent() * 0.5f - f.getDescent() * 0.5f;

        int index = 0;
        for (auto c : text)
        {
            g.drawSingleLineText (juce::String::charToString (c), juce::roundToInt (x), juce::roundToInt (baseline));
            x += widths.first[(size_t) index++] + tracking;
        }
    }

    //==============================================================================
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 18.0f, float alpha = 0.60f, bool shadow = true)
    {
        if (shadow)
        {
            juce::DropShadow ds (juce::Colours::black.withAlpha (0.10f), 22, { 0, 9 });
            juce::Path p;
            p.addRoundedRectangle (r, radius);
            ds.drawForPath (g, p);
        }

        juce::ColourGradient fill (cardFill.withAlpha (alpha * 1.05f), r.getX(), r.getY(),
                                   cardFill.withAlpha (alpha * 0.78f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    inline void drawInnerWell (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.fillRoundedRectangle (r, radius);
        g.setColour (th::ink.withAlpha (0.07f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    /** Green gradient used by active tabs and the power pill. */
    inline juce::ColourGradient activeGradient (juce::Rectangle<float> r)
    {
        juce::ColourGradient cg (greenMid, r.getX(), r.getY(), greenDark, r.getRight(), r.getBottom(), false);
        return cg;
    }

    inline void drawPill (juce::Graphics& g, juce::Rectangle<float> r, bool active, float inactiveAlpha = 0.55f)
    {
        const auto radius = r.getHeight() * 0.5f;

        if (active)
        {
            juce::DropShadow ds (green.withAlpha (0.35f), 12, { 0, 3 });
            juce::Path p;
            p.addRoundedRectangle (r, radius);
            ds.drawForPath (g, p);

            g.setGradientFill (activeGradient (r));
            g.fillRoundedRectangle (r, radius);

            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (inactiveAlpha * 0.75f));
            g.fillRoundedRectangle (r, radius);
            g.setColour (th::ink.withAlpha (0.08f));
            g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        }
    }

    inline void drawPillText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, bool active, float size = 13.0f)
    {
        g.setFont (font (size, active ? 2 : 1));
        g.setColour (active ? juce::Colours::white : inkSoft);
        g.drawText (text, r, juce::Justification::centred, false);
    }

    //==============================================================================
    /** Small line icons drawn to fit the given square. */
    enum class Icon
    {
        leaf, gear, wave, filter, knob, effects, arp, home, target, folder, save, undo, redo,
        prev, next, play, keyboard, close, plus, minus, power, matrix, search, dice, grain
    };

    inline void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour, float stroke = 1.6f)
    {
        g.setColour (colour);
        const auto c = area.getCentre();
        const auto s = juce::jmin (area.getWidth(), area.getHeight());
        juce::Path p;

        switch (icon)
        {
            case Icon::gear:
            {
                const auto inner = s * 0.26f;
                const auto outer = s * 0.44f;
                g.drawEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f, stroke);
                for (int i = 0; i < 8; ++i)
                {
                    const auto a = (float) i * juce::MathConstants<float>::twoPi / 8.0f;
                    g.drawLine (c.x + std::cos (a) * inner, c.y + std::sin (a) * inner,
                                c.x + std::cos (a) * outer, c.y + std::sin (a) * outer, stroke);
                }
                break;
            }
            case Icon::power:
                g.drawEllipse (area.reduced (s * 0.16f), stroke);
                g.drawLine (c.x, area.getY() + s * 0.10f, c.x, c.y, stroke);
                break;
            case Icon::prev:
                p.startNewSubPath (c.x + s * 0.15f, c.y - s * 0.24f);
                p.lineTo (c.x + s * 0.15f, c.y + s * 0.24f);
                p.lineTo (c.x - s * 0.18f, c.y);
                p.closeSubPath();
                g.fillPath (p);
                break;
            case Icon::next:
                p.startNewSubPath (c.x - s * 0.15f, c.y - s * 0.24f);
                p.lineTo (c.x - s * 0.15f, c.y + s * 0.24f);
                p.lineTo (c.x + s * 0.18f, c.y);
                p.closeSubPath();
                g.fillPath (p);
                break;
            case Icon::folder:
                p.startNewSubPath (area.getX() + s * 0.08f, area.getY() + s * 0.28f);
                p.lineTo (area.getX() + s * 0.08f, area.getBottom() - s * 0.22f);
                p.lineTo (area.getRight() - s * 0.1f, area.getBottom() - s * 0.22f);
                p.lineTo (area.getRight() - s * 0.1f, area.getY() + s * 0.28f);
                p.closeSubPath();
                g.strokePath (p, juce::PathStrokeType (stroke));
                break;
            case Icon::save:
                g.drawRoundedRectangle (area.reduced (s * 0.12f), 2.0f, stroke);
                g.fillRect (area.getCentreX() - s * 0.16f, area.getY() + s * 0.18f, s * 0.32f, s * 0.22f);
                break;
            case Icon::undo:
            case Icon::redo:
            {
                const auto flip = icon == Icon::redo;
                juce::Path arc;
                arc.addCentredArc (c.x, c.y + s * 0.06f, s * 0.27f, s * 0.27f, 0.0f,
                                   flip ? -2.4f : -3.9f, flip ? 0.35f : 0.35f - 1.5f, true);
                g.strokePath (arc, juce::PathStrokeType (stroke));
                const auto tipX = flip ? c.x + s * 0.24f : c.x - s * 0.24f;
                p.startNewSubPath (tipX, c.y - s * 0.24f);
                p.lineTo (tipX + (flip ? s * 0.1f : -s * 0.1f), c.y - s * 0.06f);
                p.lineTo (tipX + (flip ? -s * 0.02f : s * 0.02f), c.y + s * 0.02f);
                g.strokePath (p, juce::PathStrokeType (stroke));
                break;
            }
            case Icon::home:
                p.startNewSubPath (area.getX() + s * 0.14f, c.y);
                p.lineTo (c.x, area.getY() + s * 0.16f);
                p.lineTo (area.getRight() - s * 0.14f, c.y);
                g.strokePath (p, juce::PathStrokeType (stroke));
                g.drawRect (area.getX() + s * 0.24f, c.y - s * 0.02f, s * 0.52f, s * 0.42f, stroke);
                break;
            case Icon::wave:
                p.startNewSubPath (area.getX() + s * 0.1f, c.y);
                p.cubicTo (area.getX() + s * 0.24f, c.y - s * 0.36f, area.getX() + s * 0.3f, c.y - s * 0.36f, c.x, c.y);
                p.cubicTo (area.getX() + s * 0.72f, c.y + s * 0.36f, area.getX() + s * 0.78f, c.y + s * 0.36f, area.getRight() - s * 0.1f, c.y);
                g.strokePath (p, juce::PathStrokeType (stroke));
                break;
            case Icon::filter:
                p.startNewSubPath (area.getX() + s * 0.14f, area.getY() + s * 0.22f);
                p.lineTo (area.getRight() - s * 0.14f, area.getY() + s * 0.22f);
                p.lineTo (c.x + s * 0.06f, c.y + s * 0.1f);
                p.lineTo (c.x + s * 0.06f, area.getBottom() - s * 0.14f);
                p.lineTo (c.x - s * 0.06f, area.getBottom() - s * 0.14f);
                p.lineTo (c.x - s * 0.06f, c.y + s * 0.1f);
                p.closeSubPath();
                g.strokePath (p, juce::PathStrokeType (stroke));
                break;
            case Icon::knob:
                g.drawEllipse (area.reduced (s * 0.2f), stroke);
                g.drawLine (c.x, area.getY() + s * 0.2f, c.x, c.y - s * 0.06f, stroke);
                break;
            case Icon::effects:
                g.drawEllipse (area.reduced (s * 0.18f), stroke);
                g.fillEllipse (c.x - s * 0.1f, c.y - s * 0.1f, s * 0.2f, s * 0.2f);
                break;
            case Icon::arp:
            {
                for (int i = 0; i < 3; ++i)
                {
                    const auto x = area.getX() + s * (0.22f + 0.22f * (float) i);
                    const auto h = s * (0.34f + 0.16f * (float) i);
                    g.drawLine (x, area.getBottom() - s * 0.2f, x, area.getBottom() - s * 0.2f - h, stroke);
                }
                break;
            }
            case Icon::matrix:
            {
                for (int y = 0; y < 3; ++y)
                    for (int x = 0; x < 3; ++x)
                        g.fillEllipse (area.getX() + s * (0.2f + 0.28f * (float) x) - s * 0.06f,
                                       area.getY() + s * (0.2f + 0.28f * (float) y) - s * 0.06f,
                                       s * 0.12f, s * 0.12f);
                break;
            }
            case Icon::keyboard:
                g.drawRoundedRectangle (area.reduced (s * 0.08f, s * 0.2f), 2.0f, stroke);
                for (int i = 1; i < 5; ++i)
                {
                    const auto x = area.getX() + s * 0.08f + (area.getWidth() - s * 0.16f) * (float) i / 5.0f;
                    g.drawLine (x, area.getY() + s * 0.22f, x, area.getBottom() - s * 0.22f, stroke * 0.7f);
                }
                break;
            case Icon::target:
                g.drawEllipse (c.x - s * 0.16f, c.y - s * 0.16f, s * 0.32f, s * 0.32f, stroke * 0.8f);
                g.drawLine (c.x, area.getY(), c.x, c.y - s * 0.2f, stroke * 0.8f);
                g.drawLine (c.x, c.y + s * 0.2f, c.x, area.getBottom(), stroke * 0.8f);
                g.drawLine (area.getX(), c.y, c.x - s * 0.2f, c.y, stroke * 0.8f);
                g.drawLine (c.x + s * 0.2f, c.y, area.getRight(), c.y, stroke * 0.8f);
                break;
            case Icon::close:
                g.drawLine (area.getX() + s * 0.22f, area.getY() + s * 0.22f, area.getRight() - s * 0.22f, area.getBottom() - s * 0.22f, stroke);
                g.drawLine (area.getRight() - s * 0.22f, area.getY() + s * 0.22f, area.getX() + s * 0.22f, area.getBottom() - s * 0.22f, stroke);
                break;
            case Icon::plus:
                g.drawLine (c.x - s * 0.25f, c.y, c.x + s * 0.25f, c.y, stroke);
                g.drawLine (c.x, c.y - s * 0.25f, c.x, c.y + s * 0.25f, stroke);
                break;
            case Icon::minus:
                g.drawLine (c.x - s * 0.25f, c.y, c.x + s * 0.25f, c.y, stroke);
                break;
            case Icon::leaf:
            {
                juce::Path l;
                l.startNewSubPath (c.x, area.getY() + s * 0.12f);
                l.cubicTo (area.getRight() - s * 0.1f, c.y - s * 0.3f, area.getRight() - s * 0.16f, area.getBottom() - s * 0.12f, c.x, area.getBottom() - s * 0.14f);
                l.cubicTo (area.getX() + s * 0.16f, area.getBottom() - s * 0.12f, area.getX() + s * 0.1f, c.y - s * 0.3f, c.x, area.getY() + s * 0.12f);
                g.strokePath (l, juce::PathStrokeType (stroke));
                break;
            }
            case Icon::dice:
            {
                g.drawRoundedRectangle (area.reduced (s * 0.10f), s * 0.18f, stroke);
                const auto dot = s * 0.10f;
                g.fillEllipse (c.x - dot * 0.5f - s * 0.16f, c.y - dot * 0.5f - s * 0.16f, dot, dot);
                g.fillEllipse (c.x - dot * 0.5f + s * 0.16f, c.y - dot * 0.5f + s * 0.16f, dot, dot);
                g.fillEllipse (c.x - dot * 0.5f, c.y - dot * 0.5f, dot, dot);
                break;
            }
            case Icon::grain:
            {
                static const float pts[] = { 0.5f, 0.5f, 0.26f, 0.28f, 0.74f, 0.3f, 0.3f, 0.72f, 0.72f, 0.74f, 0.5f, 0.18f, 0.18f, 0.5f, 0.82f, 0.5f };
                for (int i = 0; i < 8; ++i)
                {
                    const auto px = area.getX() + s * pts[i * 2];
                    const auto py = area.getY() + s * pts[i * 2 + 1];
                    const auto r = s * (i == 0 ? 0.10f : 0.055f);
                    g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
                }
                break;
            }
            case Icon::search:
                g.drawEllipse (c.x - s * 0.24f, c.y - s * 0.28f, s * 0.36f, s * 0.36f, stroke);
                g.drawLine (c.x + s * 0.1f, c.y + s * 0.08f, c.x + s * 0.28f, c.y + s * 0.28f, stroke);
                break;
            default:
                break;
        }
    }
}
