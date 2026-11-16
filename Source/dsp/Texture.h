#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

#include "Dsp.h"

namespace botanic
{

//==============================================================================
/** Granular ("particle") processor.

    Records the incoming synth bus into a two second ring buffer and rebuilds it
    from a cloud of windowed grains. Position / spray / density turn the synth
    into a shifting, almost aleatoric texture; freeze locks a moment in time.
*/
class GrainEngine
{
public:
    struct Params
    {
        bool on = false;
        bool freeze = false;
        float mix = 0.55f;
        float sizeMs = 120.0f;
        float density = 16.0f;
        float position = 0.35f;
        float spray = 0.3f;
        float pitch = 0.0f;
        float pitchRandom = 0.0f;
        float reverse = 0.25f;
        float spread = 0.6f;
        float brightness = 1.0f;
    };

    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { params = p; }
    void process (float* left, float* right, int numSamples);

    float getActivity() const noexcept { return activity; }

private:
    struct Grain
    {
        bool active = false;
        int readPos = 0;
        int length = 1;
        int age = 0;
        float rate = 1.0f;
        float gainL = 0.707f, gainR = 0.707f;
        float pan = 0.0f;
        bool reverse = false;
        float tilt = 0.0f;
    };

    void spawnGrain();
    float readInterpolated (const std::vector<float>& buf, float pos) const;

    static constexpr int maxGrains = 48;
    std::array<Grain, maxGrains> grains {};
    std::vector<float> bufferL, bufferR;
    int bufferSize = 1;
    int writePos = 0;

    Params params;
    float sampleRate = 44100.0f;
    double samplesUntilNext = 0.0;
    float activity = 0.0f;
    juce::Random rnd { 0x9e3779b9 };
    std::array<float, 2> lowState {};
};

//==============================================================================
/** Digital failure / glitch processor.

    Bit crushing and sample rate reduction run continuously, while a tempo locked
    sequencer keeps throwing stutters, tape stops, gates and dropouts at the
    incoming signal. Chaos scales how often the accidents happen.
*/
class GlitchEngine
{
public:
    struct Params
    {
        bool on = false;
        float mix = 0.65f;
        float bits = 12.0f;
        float rateDiv = 1.0f;
        int stepDivision = 3;        // index into the sync table
        int stutterDivision = 4;
        float stutterChance = 0.25f;
        float gateChance = 0.10f;
        float dropChance = 0.08f;
        float tapeChance = 0.06f;
        float repeats = 3.0f;
        float chaos = 0.5f;
        float noise = 0.10f;
    };

    void prepare (double sampleRate);
    void reset();
    void setParams (const Params& p) { params = p; }
    void setTempo (double bpm);
    void trigger();                          // manual stutter
    void process (float* left, float* right, int numSamples);

    float getActivity() const noexcept { return activity; }

private:
    enum class Mode { normal, capture, repeat, tape };

    static double divisionBeats (int index);
    void beginStutter (int lengthSamples);
    void onStep();

    Params params;
    float sampleRate = 44100.0f;
    double bpm = 120.0;
    double stepCounter = 0.0;
    int stepLength = 1;

    Mode mode = Mode::normal;
    int modeCounter = 0;
    int captureLength = 1;
    int repeatLeft = 0;
    int tapeLength = 1;

    std::vector<float> loopL, loopR;
    int loopWrite = 0;
    float loopRead = 0.0f;

    std::vector<float> tapeL, tapeR;
    int tapeWrite = 0;
    float tapeRead = 0.0f;

    float gateValue = 1.0f;
    float holdL = 0.0f, holdR = 0.0f;
    float debrisL = 0.0f, debrisR = 0.0f;
    float lpL = 0.0f, lpR = 0.0f, lp2L = 0.0f, lp2R = 0.0f;
    float preL = 0.0f, preR = 0.0f;
    float lpCoefficient = 0.7f;
    float inputEnv = 0.0f;
    int silentBlocks = 0;
    Mode lastMode = Mode::normal;
    float transFade = 1.0f, transStep = 0.01f, transFromL = 0.0f, transFromR = 0.0f;
    float lastOutL = 0.0f, lastOutR = 0.0f;
    int holdCount = 0;
    int gateRamp = 0;
    bool gateOpen = true;
    int dropLeft = 0;
    float activity = 0.0f;

    juce::Random rnd { 0x51ed270b };
};

//==============================================================================
/** Glitch followed by grain, sitting between the voices and the effect chain. */
class TextureProcessor
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (const GrainEngine::Params& grain, const GlitchEngine::Params& glitch, double bpm);
    void process (float* left, float* right, int numSamples);
    void trigger() { glitch.trigger(); }

    float getActivity() const noexcept { return juce::jmax (grain.getActivity(), glitch.getActivity()); }

private:
    GrainEngine grain;
    GlitchEngine glitch;
};

} // namespace botanic
