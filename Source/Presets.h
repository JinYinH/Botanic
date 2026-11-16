#pragma once

#include <JuceHeader.h>

#include "Params.h"
#include "ModMatrix.h"

namespace botanic
{
    juce::StringArray factoryPresetNames();

    /** Restores defaults then applies the factory preset with the given index. */
    void applyFactoryPreset (int index, juce::AudioProcessorValueTreeState& apvts, ModMatrix& matrix);

    /** Builds a full state for a factory preset (defaults + overrides + mod matrix). */
    juce::ValueTree buildFactoryPresetState (int index);
}
