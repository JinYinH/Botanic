#include "Effects.h"

namespace botanic
{

void EffectsChain::EqBand::update (float db, int kind, double sr)
{
    if (std::abs (db - gainDb) < 0.001f)
        return;

    gainDb = db;
    type = kind;

    switch (kind)
    {
        case 0: filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 220.0, 0.7, juce::Decibels::decibelsToGain (db)); break;
        case 1: filter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1000.0, 1.1, juce::Decibels::decibelsToGain (db)); break;
        default: filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 4200.0, 0.7, juce::Decibels::decibelsToGain (db)); break;
    }
}

//==============================================================================
void ToneStage::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, maxBlockSize), 1 };

    for (auto* f : { &lowL, &lowR, &highL, &highR, &airL, &airR })
        f->prepare (spec);

    tilt = 1000.0f;
    airAmount = 1000.0f;
    reset();
}

void ToneStage::reset()
{
    for (auto* f : { &lowL, &lowR, &highL, &highR, &airL, &airR })
        f->reset();
}

void ToneStage::setParams (float tiltDb, float widthValue, float air)
{
    width = juce::jlimit (0.0f, 2.0f, widthValue);

    if (std::abs (tiltDb - tilt) > 0.01f || std::abs (air - airAmount) > 0.005f)
    {
        tilt = tiltDb;
        airAmount = air;
        updateCoefficients();
    }
}

void ToneStage::updateCoefficients()
{
    const auto lowGain = juce::Decibels::decibelsToGain (-tilt * 0.5f);
    const auto highGain = juce::Decibels::decibelsToGain (tilt * 0.5f);
    const auto airGain = juce::Decibels::decibelsToGain (airAmount * 12.0f);

    auto lowCoef = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, 320.0, 0.6, lowGain);
    auto highCoef = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 3200.0, 0.6, highGain);
    auto airCoef = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 9000.0, 0.7, airGain);

    for (auto* f : { &lowL, &lowR }) f->coefficients = lowCoef;
    for (auto* f : { &highL, &highR }) f->coefficients = highCoef;
    for (auto* f : { &airL, &airR }) f->coefficients = airCoef;
}

void ToneStage::process (float* left, float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        auto l = highL.processSample (lowL.processSample (left[i]));
        auto r = highR.processSample (lowR.processSample (right[i]));

        l = airL.processSample (l);
        r = airR.processSample (r);

        if (std::abs (width - 1.0f) > 0.001f)
        {
            const auto mid = (l + r) * 0.5f;
            const auto side = (l - r) * 0.5f * width;
            l = mid + side;
            r = mid - side;
        }

        left[i] = l;
        right[i] = r;
    }
}

void EffectsChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = (float) sr;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, maxBlockSize), 2 };

    reverb.setSampleRate (sr);

    compressor.prepare (spec);
    compressor.setAttack (5.0f);
    compressor.setRelease (140.0f);

    delayLineL.setMaximumDelayInSamples ((int) (sr * 2.05));
    delayLineR.setMaximumDelayInSamples ((int) (sr * 2.05));
    delayLineL.prepare (spec);
    delayLineR.prepare (spec);

    chorusLineL.setMaximumDelayInSamples ((int) (sr * 0.06));
    chorusLineR.setMaximumDelayInSamples ((int) (sr * 0.06));
    chorusLineL.prepare (spec);
    chorusLineR.prepare (spec);

    for (int i = 0; i < 3; ++i)
    {
        eqL[i].filter.prepare (spec);
        eqR[i].filter.prepare (spec);
        eqL[i].gainDb = 1000.0f;
        eqR[i].gainDb = 1000.0f;
    }

    tempBuffer.setSize (2, juce::jmax (32, maxBlockSize));
    reset();
}

void EffectsChain::reset()
{
    reverb.reset();
    compressor.reset();
    delayLineL.reset();
    delayLineR.reset();
    chorusLineL.reset();
    chorusLineR.reset();
    delayDampL = delayDampR = 0.0f;
    chorusPhase = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        eqL[i].filter.reset();
        eqR[i].filter.reset();
    }
}

void EffectsChain::setParams (const FxParams& p)
{
    params = p;

    juce::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, p.revSize);
    rp.damping = juce::jlimit (0.0f, 1.0f, p.revDamp);
    rp.width = juce::jlimit (0.0f, 1.0f, p.revWidth);
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    compressor.setThreshold (p.cmpThresh);
    compressor.setRatio (juce::jmax (1.0f, p.cmpRatio));

    for (int i = 0; i < 3; ++i)
    {
        const auto db = i == 0 ? p.eqLowDb : (i == 1 ? p.eqMidDb : p.eqHighDb);
        eqL[i].update (db, i, (double) sampleRate);
        eqR[i].update (db, i, (double) sampleRate);
    }
}

void EffectsChain::process (float* left, float* right, int numSamples)
{
    for (auto slot : params.order)
    {
        switch (slot)
        {
            case 0: if (params.revOn)  processReverb (left, right, numSamples); break;
            case 1: if (params.dlyOn)  processDelay (left, right, numSamples); break;
            case 2: if (params.choOn)  processChorus (left, right, numSamples); break;
            case 3: if (params.eqOn)   processEq (left, right, numSamples); break;
            case 4: if (params.compOn) processComp (left, right, numSamples); break;
            default: break;
        }
    }
}

void EffectsChain::processReverb (float* l, float* r, int n)
{
    const auto mix = juce::jlimit (0.0f, 1.0f, params.revMix + params.revMixMod);
    if (mix <= 0.0005f)
        return;

    if (tempBuffer.getNumSamples() < n)
        tempBuffer.setSize (2, n, false, false, true);

    // keep the decay musical: the JUCE reverb gets very long near the top of
    // the range, which turns pads into endless washes
    juce::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, params.revSize * 0.82f);
    rp.damping = juce::jlimit (0.0f, 1.0f, params.revDamp);
    rp.width = juce::jlimit (0.0f, 1.0f, params.revWidth);
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    tempBuffer.copyFrom (0, 0, l, n);
    tempBuffer.copyFrom (1, 0, r, n);

    reverb.processStereo (tempBuffer.getWritePointer (0), tempBuffer.getWritePointer (1), n);

    for (int i = 0; i < n; ++i)
    {
        l[i] = l[i] * (1.0f - mix) + tempBuffer.getSample (0, i) * mix;
        r[i] = r[i] * (1.0f - mix) + tempBuffer.getSample (1, i) * mix;
    }
}

void EffectsChain::processDelay (float* l, float* r, int n)
{
    const auto mix = juce::jlimit (0.0f, 1.0f, params.dlyMix + params.dlyMixMod);
    if (mix <= 0.0005f)
        return;

    const auto delaySamples = juce::jlimit (1.0f, sampleRate * 2.0f, params.dlyTimeSec * sampleRate);
    delayLineL.setDelay (delaySamples);
    delayLineR.setDelay (delaySamples * 1.02f);

    const auto fb = juce::jlimit (0.0f, 0.95f, params.dlyFb);
    const auto dampCoef = 0.35f;

    for (int i = 0; i < n; ++i)
    {
        const auto dl = delayLineL.popSample (0);
        const auto dr = delayLineR.popSample (0);

        delayDampL += (dl - delayDampL) * dampCoef;
        delayDampR += (dr - delayDampR) * dampCoef;

        delayLineL.pushSample (0, l[i] + delayDampL * fb);
        delayLineR.pushSample (0, r[i] + delayDampR * fb);

        l[i] = l[i] * (1.0f - mix * 0.5f) + dl * mix;
        r[i] = r[i] * (1.0f - mix * 0.5f) + dr * mix;
    }
}

void EffectsChain::processChorus (float* l, float* r, int n)
{
    const auto mix = juce::jlimit (0.0f, 1.0f, params.choMix);
    if (mix <= 0.0005f)
        return;

    const auto rate = juce::jlimit (0.02f, 8.0f, params.choRate);
    const auto depth = juce::jlimit (0.0f, 1.0f, params.choDepth);
    const auto inc = rate / sampleRate;
    const auto baseSamples = sampleRate * 0.012f;
    const auto depthSamples = sampleRate * 0.009f * depth;

    for (int i = 0; i < n; ++i)
    {
        chorusPhase += inc;
        if (chorusPhase >= 1.0f)
            chorusPhase -= 1.0f;

        const auto modL = std::sin (chorusPhase * juce::MathConstants<float>::twoPi);
        const auto modR = std::sin ((chorusPhase + 0.25f) * juce::MathConstants<float>::twoPi);

        chorusLineL.setDelay (baseSamples + depthSamples * (modL + 1.0f));
        chorusLineR.setDelay (baseSamples + depthSamples * (modR + 1.0f));

        const auto wetL = chorusLineL.popSample (0);
        const auto wetR = chorusLineR.popSample (0);

        chorusLineL.pushSample (0, l[i]);
        chorusLineR.pushSample (0, r[i]);

        l[i] = l[i] * (1.0f - mix) + wetL * mix;
        r[i] = r[i] * (1.0f - mix) + wetR * mix;
    }
}

void EffectsChain::processEq (float* l, float* r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        float sl = l[i], sr = r[i];
        for (int b = 0; b < 3; ++b)
        {
            sl = eqL[b].filter.processSample (sl);
            sr = eqR[b].filter.processSample (sr);
        }
        l[i] = sl;
        r[i] = sr;
    }
}

void EffectsChain::processComp (float* l, float* r, int n)
{
    if (tempBuffer.getNumSamples() < n)
        tempBuffer.setSize (2, n, false, false, true);

    // keep the decay musical: the JUCE reverb gets very long near the top of
    // the range, which turns pads into endless washes
    juce::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, params.revSize * 0.82f);
    rp.damping = juce::jlimit (0.0f, 1.0f, params.revDamp);
    rp.width = juce::jlimit (0.0f, 1.0f, params.revWidth);
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    tempBuffer.copyFrom (0, 0, l, n);
    tempBuffer.copyFrom (1, 0, r, n);

    juce::dsp::AudioBlock<float> block (tempBuffer.getArrayOfWritePointers(), 2, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    compressor.process (ctx);

    const auto makeup = dbToGain (params.cmpMakeup);
    for (int i = 0; i < n; ++i)
    {
        l[i] = tempBuffer.getSample (0, i) * makeup;
        r[i] = tempBuffer.getSample (1, i) * makeup;
    }
}

} // namespace botanic
