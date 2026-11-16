#pragma once

#include <JuceHeader.h>
#include "Theme.h"
#include "../PluginProcessor.h"

namespace ui
{

//==============================================================================
/** Rotary control bound to a parameter, with modulation ring and context menu. */
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (BotanicAudioProcessor& processor, const juce::String& paramId,
          const juce::String& captionText, float diameterRef);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;

    void refresh();
    void setDimmed (bool shouldDim);
    void setCaption (const juce::String& c) { caption = c; repaint(); }
    void setParameter (const juce::String& newId, const juce::String& newCaption);
    const juce::String& getParamId() const noexcept { return paramID; }

    static constexpr float labelHeight = 40.0f;

private:
    void showContextMenu();
    float currentNorm() const;

    BotanicAudioProcessor& proc;
    juce::String paramID, caption;
    juce::RangedAudioParameter* param = nullptr;
    juce::String displayText;
    float diameter = 60.0f;
    float normValue = 0.0f;
    float dragStartNorm = 0.0f;
    bool dimmed = false, hovered = false, dragging = false;
};

//==============================================================================
/** Row of pill-shaped tabs. */
class TabStrip : public juce::Component
{
public:
    explicit TabStrip (std::vector<juce::String> labels);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setSelected (int index, bool notify = false);
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

private:
    std::vector<juce::String> items;
    int selected = 0, hovered = -1;
    int indexAt (float x) const;
};

//==============================================================================
/** Pill-shaped dropdown bound to a choice parameter (or a custom value list). */
class ChoicePill : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChoicePill (BotanicAudioProcessor& processor, const juce::String& paramId, float corner = 14.0f);
    ChoicePill (BotanicAudioProcessor& processor, const juce::String& paramId,
                juce::StringArray customLabels, std::vector<float> customValues);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void refresh();
    juce::String getDisplayText() const { return displayText; }

private:
    void showMenu();

    BotanicAudioProcessor& proc;
    juce::String paramID;
    juce::RangedAudioParameter* param = nullptr;
    juce::StringArray labels;
    std::vector<float> values;
    bool customValues = false;
    bool hovered = false;
    float radius = 14.0f;
    juce::String displayText;
};

//==============================================================================
/** Small on/off switch bound to a boolean parameter, with optional side label. */
class ToggleSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    ToggleSwitch (BotanicAudioProcessor& processor, const juce::String& paramId, const juce::String& labelText = {});

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();
    void setParameter (const juce::String& newId, const juce::String& newLabel);

private:
    BotanicAudioProcessor& proc;
    juce::String paramID, label;
    juce::RangedAudioParameter* param = nullptr;
    bool state = false;
};

//==============================================================================
/** The top-right ON pill. */
class PowerSwitch : public juce::Component
{
public:
    explicit PowerSwitch (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void refresh();

private:
    BotanicAudioProcessor& proc;
    bool on = true, hovered = false;
    float anim = 1.0f;
};

//==============================================================================
class IconButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    IconButton (th::Icon icon, juce::Colour colour = th::inkSoft, float iconScale = 0.62f);
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void()> onClick;
    void setIcon (th::Icon i) { icon = i; repaint(); }
    void setColour (juce::Colour c) { colour = c; repaint(); }

private:
    th::Icon icon;
    juce::Colour colour;
    float scale;
    bool hovered = false, pressed = false;
};

//==============================================================================
/** Stereo peak meter. */
class LevelMeter : public juce::Component
{
public:
    void setLevels (float peakLeft, float peakRight);
    void paint (juce::Graphics&) override;

private:
    void paintBar (juce::Graphics&, juce::Rectangle<float>, float level);

    float smoothedL = 0.0f, smoothedR = 0.0f;
    float holdL = 0.0f, holdR = 0.0f;
};

//==============================================================================
/** Vertical fader used by the envelope panel and the master volume strip. */
class VerticalFader : public juce::Component, public juce::SettableTooltipClient
{
public:
    VerticalFader (BotanicAudioProcessor& processor, const juce::String& paramId,
                   const juce::String& captionText, bool showLabels);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void refresh();
    void setDimmed (bool d) { dimmed = d; }
    void setParameter (const juce::String& newId, const juce::String& newCaption);
    const juce::String& getParamId() const noexcept { return paramID; }

private:
    juce::Rectangle<float> trackArea() const;

    BotanicAudioProcessor& proc;
    juce::String paramID, caption;
    juce::RangedAudioParameter* param = nullptr;
    float norm = 0.0f;
    float dragStart = 0.0f;
    bool showLabels = true;
    bool dimmed = false;
};

//==============================================================================
/** Rounded text button used on the experimental pages. */
class TextPillButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    TextPillButton() = default;
    TextPillButton (juce::String buttonText, juce::Colour accentColour = th::greenDark);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setText (const juce::String& newText) { text = newText; repaint(); }
    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void setIcon (th::Icon newIcon) { icon = newIcon; hasIcon = true; repaint(); }

    std::function<void()> onClick;
    std::function<void()> onSecondaryClick;

private:
    juce::String text;
    juce::Colour accent { th::greenDark };
    th::Icon icon = th::Icon::dice;
    bool hasIcon = false;
    bool hovered = false, pressed = false, suppressClick = false;
    float flash = 0.0f;
};

//==============================================================================
class NavRail : public juce::Component
{
public:
    explicit NavRail (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void setSelected (int index);
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onSelect;

    static juce::StringArray itemNames();
    static constexpr int numItems = 9;

private:
    BotanicAudioProcessor& proc;
    int selected = 0, hovered = -1;
    float hoverT = 0.0f;
    juce::Rectangle<float> itemBounds (int index) const;
};

} // namespace ui
