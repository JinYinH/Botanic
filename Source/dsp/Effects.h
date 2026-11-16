#pragma once

#include <JuceHeader.h>
#include <array>

#include "Dsp.h"

namespace botanic
{

struct FxParams
{
    bool revOn = true, dlyOn = true, choOn = true, eqOn = false, compOn = false;

    float revMix = 0.32f, revSize = 0.6f, revDamp = 0.4f, revWidth = 0.85f;
    float dlyMix = 0.25f, dlyTimeSec = 0.33f, dlyFb = 0.35f;
    float choMix = 0.35f, choRate = 0.8f, choDepth = 0.5f;
    float eqLowDb = 0.0f, eqMidDb = 0.0f, eqHighDb = 0.0f;
    float cmpThresh = -18.0f, cmpRatio = 3.0f, cmpMakeup = 0.0f;

    float revMixMod = 0.0f, dlyMixMod = 0.0f;

    std::array<int, 5> order { 0, 1, 2, 3, 4 };   // reverb, delay, chorus, eq, comp
};

/** Master tone stage: tilt EQ, mid/side width and an air shelf. */
class ToneStage
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (float tiltDb, float width, float air);
    void process (float* left, float* right, int numSamples);

private:
    void updateCoefficients();

    juce::dsp::IIR::Filter<float> lowL, lowR, highL, highR, airL, airR;
    double sampleRate = 44100.0;
    float tilt = 1000.0f, airAmount = 1000.0f, width = 1.0f;
};

/** The five-unit stereo effects chain. */
class EffectsChain
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (const FxParams& p);
    void process (float* left, float* right, int numSamples);

private:
    void processReverb (float* l, float* r, int n);
    void processDelay (float* l, float* r, int n);
    void processChorus (float* l, float* r, int n);
    void processEq (float* l, float* r, int n);
    void processComp (float* l, float* r, int n);

    float sampleRate = 44100.0f;
    FxParams params;

    juce::Reverb reverb;
    juce::dsp::Compressor<float> compressor;

    // delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL, delayLineR;
    float delayDampL = 0.0f, delayDampR = 0.0f;
    float lastDelayTime = -1.0f;

    // chorus
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> chorusLineL, chorusLineR;
    float chorusPhase = 0.0f;

    // eq
    struct EqBand
    {
        juce::dsp::IIR::Filter<float> filter;
        float gainDb = 1000.0f;
        int type = 0;
        void update (float db, int kind, double sr);
    };
    EqBand eqL[3], eqR[3];

    juce::AudioBuffer<float> tempBuffer;
};

} // namespace botanic
