#pragma once

#include <JuceHeader.h>
#include "Theme.h"
#include "../PluginProcessor.h"

namespace ui
{

//==============================================================================
/** Morphing wavetable view with a draggable position node. */
class WavetableDisplay : public juce::Component
{
public:
    explicit WavetableDisplay (BotanicAudioProcessor& processor);

    void setOscillator (int index);
    void refresh();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void applyPosition (float newPos);
    void paintStatic (juce::Graphics&);
    juce::Rectangle<float> plotArea() const;

    BotanicAudioProcessor& proc;
    int oscIndex = 0;
    int tableIndex = 0;
    float position = 0.5f;
    juce::RangedAudioParameter* posParam = nullptr;
    juce::Image staticCache;
    bool cacheDirty = true;
};

//==============================================================================
/** Filter response curve with draggable cutoff / resonance node. */
class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (BotanicAudioProcessor& processor);

    void refresh();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void applyFromPoint (juce::Point<float> p);

    BotanicAudioProcessor& proc;
    int type = 0;
    float cutoff = 1200.0f, resonance = 0.32f;
    juce::Rectangle<float> plotArea() const;
};

//==============================================================================
/** ADSR editor with draggable nodes. */
class EnvelopeDisplay : public juce::Component
{
public:
    explicit EnvelopeDisplay (BotanicAudioProcessor& processor);

    void setEnvelope (int index);
    void setHighlightStage (int stage);
    void refresh();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    struct Node { juce::Point<float> p; };

    std::array<juce::Point<float>, 5> computeNodes() const;
    int hitNode (juce::Point<float> p) const;

    BotanicAudioProcessor& proc;
    int envIndex = 0;
    int highlight = 0;
    float a = 0.01f, d = 0.42f, s = 0.78f, r = 0.32f;
    int draggedNode = -1;
    std::array<juce::Point<float>, 5> lastNodes {};
};

//==============================================================================
/** Animated LFO shape view. */
class LfoDisplay : public juce::Component
{
public:
    explicit LfoDisplay (BotanicAudioProcessor& processor);

    void setLfo (int index);
    void refresh();
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void paintStatic (juce::Graphics&);

    BotanicAudioProcessor& proc;
    int lfoIndex = 0;
    int shape = 0;
    float rate = 1.0f, depth = 0.5f, phase = 0.0f;
    float dragStart = 0.0f;
    juce::Image staticCache;
    bool cacheDirty = true;
};

//==============================================================================
/** Oscilloscope of the synth output, used on the stage page. */
class ScopeDisplay : public juce::Component
{
public:
    explicit ScopeDisplay (BotanicAudioProcessor& processor);
    void pushSamples (const float* left, const float* right, int numSamples);
    void paint (juce::Graphics&) override;

private:
    BotanicAudioProcessor& proc;
    static constexpr int size = 512;
    std::array<float, size> ring {};
    int writePos = 0;
};

} // namespace ui
