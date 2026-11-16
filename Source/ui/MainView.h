#pragma once

#include <JuceHeader.h>
#include "Theme.h"
#include "Widgets.h"
#include "Displays.h"
#include "../PluginProcessor.h"

namespace ui
{

//==============================================================================
/** Preset browser bar with prev/next, browse, save, undo and redo. */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void refresh();

    std::function<void()> onBrowse;
    std::function<void()> onSaveRequested;

private:
    BotanicAudioProcessor& proc;
    int hovered = -1;
    juce::String name;
    int presetIndex = -1;
};

//==============================================================================
class PianoKeyboard : public juce::Component
{
public:
    explicit PianoKeyboard (BotanicAudioProcessor& processor);

    void setHeldNotes (const std::array<juce::uint64, 2>& mask);
    void tick (double deltaSeconds);
    void setComputerKeyboardEnabled (bool shouldBeOn) { computerKeys = shouldBeOn; repaint(); }
    bool isComputerKeyboardEnabled() const noexcept { return computerKeys; }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void handleKey (int keyCode, bool isDown);

    int getLowNote() const noexcept { return lowNote; }

private:
    struct KeyRect { int note; juce::Rectangle<float> bounds; bool black; };

    std::vector<KeyRect> buildKeys() const;
    int noteAt (juce::Point<float> p) const;

    BotanicAudioProcessor& proc;
    int lowNote = 36, highNote = 84;
    int mouseNote = -1;
    std::array<bool, 128> active {};
    std::array<float, 128> glow {};
    bool computerKeys = false;
};

//==============================================================================
/** FX chain list + routing matrix. */
class FxChainPanel : public juce::Component
{
public:
    explicit FxChainPanel (BotanicAudioProcessor& processor);

    void refresh();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> rowBounds (int row) const;
    juce::Point<float> dotPosition (int row, int column) const;

    BotanicAudioProcessor& proc;
    std::array<bool, 5> enabled {};
    std::array<int, 5> order {};
    int dragRow = -1, hoverRow = -1;
    juce::Point<float> dragStart;
};

//==============================================================================
class CyclePill : public juce::Component
{
public:
    CyclePill (juce::StringArray items, int initial, std::function<void (int)> callback);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }
    void setIndex (int i) { index = i; repaint(); }
    int getIndex() const noexcept { return index; }

private:
    juce::StringArray items;
    int index = 0;
    bool hovered = false;
    std::function<void (int)> onChange;
};

//==============================================================================
/** Stage pages that replace the artwork when the nav is used. */
class StagePanel : public juce::Component
{
public:
    explicit StagePanel (BotanicAudioProcessor& processor);
    virtual ~StagePanel() = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    virtual void refresh() {}

    std::function<void()> onClose;

protected:
    BotanicAudioProcessor& proc;
    float closeHover = 0.0f;
    juce::Rectangle<float> closeBounds() const { return { (float) getWidth() - 40.0f, 12.0f, 24.0f, 24.0f }; }
    juce::Rectangle<float> contentBounds() const
    {
        return getLocalBounds().toFloat().reduced (20.0f, 0.0f).withTrimmedTop (56.0f).withTrimmedBottom (16.0f);
    }
};

//==============================================================================
class ModMatrixPanel : public StagePanel
{
public:
    explicit ModMatrixPanel (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void refresh() override;

private:
    juce::Rectangle<float> cellBounds (int source, int dest) const;
    int sourceAt (juce::Point<float>) const;
    int destAt (juce::Point<float>) const;

    std::array<std::array<float, botanic::numModDests>, botanic::numModSources> amounts {};
    int dragSource = -1, dragDest = -1;
    float dragStart = 0.0f;
};

//==============================================================================
class ArpPanel : public StagePanel
{
public:
    explicit ArpPanel (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;

private:
    ChoicePill modePill, ratePill;
    ToggleSwitch latchSwitch;
    Knob octavesKnob, gateKnob, swingKnob;
};

//==============================================================================
class SettingsPanel : public StagePanel
{
public:
    explicit SettingsPanel (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh() override;

    std::function<void()> onSaveRequested;

private:
    Knob tuningKnob, voicesKnob, glideKnob, bendKnob;
    juce::Rectangle<float> initButton() const { return { 22.0f, (float) getHeight() - 56.0f, 128.0f, 32.0f }; }
    juce::Rectangle<float> saveButton() const { return { 160.0f, (float) getHeight() - 56.0f, 128.0f, 32.0f }; }
    juce::Rectangle<float> randomiseButton() const { return { 298.0f, (float) getHeight() - 56.0f, 128.0f, 32.0f }; }
    juce::Rectangle<float> panicButton() const { return { 436.0f, (float) getHeight() - 56.0f, 128.0f, 32.0f }; }
};

//==============================================================================
/** Granular + glitch laboratory. */
class TexturePanel : public StagePanel
{
public:
    explicit TexturePanel (BotanicAudioProcessor& processor);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    void mouseDown (const juce::MouseEvent&) override;
    void showTab (int index) { tab = index; tabs.setSelected (index); updateTab(); }

private:
    void updateTab();

    TabStrip tabs;
    ToggleSwitch grainOn, freezeToggle, glitchOn, formantOn;
    ChoicePill glitchStep, glitchSize;
    std::vector<std::unique_ptr<Knob>> grainKnobs, glitchKnobs, vowelKnobs;
    TextPillButton triggerButton;
    TextPillButton randomButton;
    int tab = 0;
};

//==============================================================================
/** Assignable XY performance pad with automatic motion. */
class MacroPanel : public StagePanel
{
public:
    explicit MacroPanel (BotanicAudioProcessor& processor);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> padArea() const { return { 24.0f, 62.0f, 300.0f, 300.0f }; }
    void setFromPoint (juce::Point<float> p);

    ChoicePill destX, destY;
    ToggleSwitch motion;
    Knob rateKnob;
    float x = 0.5f, y = 0.5f;
};

//==============================================================================
/** Generic page that lays out a grid of parameter controls. */
class ParamPage : public StagePanel
{
public:
    struct Spec
    {
        enum class Type { knob, choice, toggle };
        Type type = Type::knob;
        juce::String paramId;
        juce::String caption;
        float size = 54.0f;
    };

    ParamPage (BotanicAudioProcessor& processor, juce::String pageTitle,
               std::vector<Spec> specs, juce::StringArray rowLabels = {});

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;

private:
    juce::String title;
    juce::StringArray rows;
    std::vector<Spec> specs;
    juce::OwnedArray<juce::Component> controls;
};

//==============================================================================
class PresetPanel : public StagePanel
{
public:
    explicit PresetPanel (BotanicAudioProcessor& processor);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void refresh() override;

    juce::Rectangle<float> listBounds() const
    {
        return getLocalBounds().toFloat().reduced (18.0f, 0.0f).withTrimmedTop (50.0f).withTrimmedBottom (14.0f);
    }

private:
    juce::StringArray names;
    int current = -1, hovered = -1;
    float scroll = 0.0f;
};

//==============================================================================
/** The living centre artwork: parallax, pollen particles and a clickable bloom. */
class StageArt : public juce::Component
{
public:
    explicit StageArt (BotanicAudioProcessor& processor);

    void animate (double deltaSeconds);
    void paint (juce::Graphics&) override;
    void paintLayer (juce::Graphics&);
    void paintOverlay (juce::Graphics&);
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    struct Particle
    {
        float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
        float life = 0.0f, maxLife = 1.0f, size = 1.0f, warm = 0.0f;
    };

    void spawn (juce::Point<float> at, int count, float power);
    void addRipple();

    struct Ripple
    {
        float x = 0.5f, y = 0.42f, radius = 0.0f, life = 0.0f, maxLife = 1.0f;
    };

    BotanicAudioProcessor& proc;
    juce::Image image;
    juce::Image artLayer;
    juce::Image tintedImage;
    double lastTintTime = -1.0;
    float lastTintBright = -1.0f;
    double lastLayerTime = -1.0;
    double repaintTimer = 0.0;
    std::array<Particle, 72> particles {};
    juce::Random rnd { 0x2f6b1c };
    double time = 0.0;
    float mouseX = 0.5f, mouseY = 0.5f, hover = 0.0f;
    float dragOffset = 0.0f, dragVelocity = 0.0f;
    float level = 0.0f, smoothLevel = 0.0f;
    float smoothBright = 0.35f, smoothTexture = 0.0f;
    float burst = 0.0f;
    int lastNotePulse = 0;
    std::array<Ripple, 8> ripples {};
    int rippleIndex = 0;
    bool dragging = false;
    float dragStartX = 0.0f, lastDragX = 0.0f;
};

//==============================================================================
/** The whole plugin face. Fixed 1672x941 design space, transformed by the editor. */
class MainView : public juce::Component
{
public:
    static constexpr int designWidth = 1672;
    static constexpr int designHeight = 941;

    explicit MainView (BotanicAudioProcessor& processor);
    ~MainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshAll();
    void animate();
    void setStagePage (int page);
    int getStagePage() const noexcept { return stagePage; }

    std::function<void()> onOpenPresets;
    std::function<void()> onToggleLanguage;
    bool isComputerKeyboardEnabled() const { return keyboard.isComputerKeyboardEnabled(); }
    void setTextureTab (int index) { texturePanel.showTab (index); }
    void openRandomMenu() { showRandomMenu(); }
    void toggleLanguage() { if (onToggleLanguage != nullptr) onToggleLanguage(); }
    void openSaveDialogPublic() { openSaveDialog(); }

private:
    void updateOscillatorPage (int osc);
    void updateEnvelopePage (int env);
    void updateModPage (int tab);
    void updateFxPage (int tab);
    void drawBackground (juce::Graphics&);
    void drawPanels (juce::Graphics&);
    void drawHeaders (juce::Graphics&);
    void rebuildChrome();
    void showRandomMenu();
    void openSaveDialog();
    void switchAB (int index);
    void updateABButtons();

    BotanicAudioProcessor& proc;

    juce::Image backgroundCache;
    juce::Image chromeCache;
    juce::Image stageImage;
    juce::Image mountainImage;
    juce::Image textureImage;

    NavRail nav;
    PresetBar presetBar;
    PowerSwitch power;
    Knob masterKnob;
    IconButton settingsButton { th::Icon::gear, th::inkSoft, 0.85f };

    // oscillator
    TabStrip oscTabs;
    WavetableDisplay wtDisplay;
    ChoicePill tablePill;
    Knob oscKnob1, oscKnob2, oscKnob3;

    // filter
    ChoicePill filterPill;
    FilterDisplay filterDisplay;
    Knob filtKnob1, filtKnob2, filtKnob3;

    // envelope
    CyclePill envTarget;
    TabStrip envTabs;
    EnvelopeDisplay envDisplay;
    VerticalFader envFader1, envFader2, envFader3, envFader4;

    // modulation
    TabStrip modTabs;
    LfoDisplay lfoDisplay;
    EnvelopeDisplay modEnvDisplay;
    ToggleSwitch syncSwitch;
    ChoicePill shapePill;
    Knob modKnob1, modKnob2, modKnob3, modKnob4;

    // effects
    TabStrip fxTabs;
    Knob fxKnob1, fxKnob2, fxKnob3;
    FxChainPanel chainPanel;
    VerticalFader volumeFader;
    Knob tiltKnob, widthKnob, airKnob;
    LevelMeter levelMeter;
    TextPillButton abButtonA, abButtonB;
    class SaveDialog;
    std::unique_ptr<SaveDialog> saveDialog;
    std::unique_ptr<juce::ValueTree> abSlot[2];
    int abActive = 0;

    // bottom
    ChoicePill modePill, voicesPill, tuningPill;
    IconButton kbdButton { th::Icon::keyboard, th::inkSoft, 0.8f };
    PianoKeyboard keyboard;

    // stage
    ModMatrixPanel matrixPanel;
    ArpPanel arpPanel;
    SettingsPanel settingsPanel;
    PresetPanel presetPanel;
    ParamPage oscPage, filterPage, fxPage;
    TexturePanel texturePanel;
    MacroPanel macroPanel;
    StageArt stageArt;
    TextPillButton randomButton;
    TextPillButton languageButton;

    double lastAnimTime = 0.0;
    int refreshCounter = 0;
    int stagePage = 0;      // 0 artwork, 1 matrix, 2 arp, 3 settings, 4 presets, 5 osc, 6 filter, 7 effects
    int navFocus = 0;
    int currentOsc = 0, currentEnv = 0, currentModTab = 0, currentFxTab = 0;

    juce::Image noiseTexture;
};

} // namespace ui
