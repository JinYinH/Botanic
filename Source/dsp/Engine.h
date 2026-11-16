#pragma once

#include "Voice.h"

namespace botanic
{

/** Voice allocation, mono/legato handling and block rendering. */
class Engine
{
public:
    static constexpr int maxVoices = 32;

    Engine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setParams (const EngineParams& newParams) { params = newParams; }
    const EngineParams& getParams() const noexcept { return params; }

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff (bool allowTailOff);

    void render (float* left, float* right, int numSamples);

    int getActiveVoiceCount() const noexcept;
    std::array<juce::uint64, 2> getNoteMask() const noexcept;
    float getOutputLevel() const noexcept { return outputLevel; }

    int getLastNote() const noexcept { return lastNote; }

private:
    int findFreeVoice();
    void startVoice (int note, float velocity);

    std::array<Voice, maxVoices> voices;
    std::array<juce::uint64, maxVoices> startOrder {};
    juce::uint64 counter = 0;
    int nextVoice = 0;

    EngineParams params;
    std::vector<int> heldNotes;
    int monoVoice = -1;
    int lastNote = 60;
    float outputLevel = 0.0f;
    float sampleRate = 44100.0f;
};

} // namespace botanic
