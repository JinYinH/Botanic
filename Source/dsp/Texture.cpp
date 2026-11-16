#include "Texture.h"

namespace botanic
{

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    inline float softLimit (float x) noexcept
    {
        return std::tanh (x * 1.15f) * 0.87f;
    }
}

//==============================================================================
// Grain engine
//==============================================================================
void GrainEngine::prepare (double sr)
{
    sampleRate = (float) sr;
    bufferSize = (int) (sr * 2.0) + 256;
    bufferL.assign ((size_t) bufferSize, 0.0f);
    bufferR.assign ((size_t) bufferSize, 0.0f);
    reset();
}

void GrainEngine::reset()
{
    for (auto& g : grains)
        g = {};

    writePos = 0;
    samplesUntilNext = 0.0;
    activity = 0.0f;
    lowState.fill (0.0f);
    std::fill (bufferL.begin(), bufferL.end(), 0.0f);
    std::fill (bufferR.begin(), bufferR.end(), 0.0f);
}

float GrainEngine::readInterpolated (const std::vector<float>& buf, float pos) const
{
    auto p = pos;

    while (p < 0.0f)
        p += (float) bufferSize;

    while (p >= (float) bufferSize)
        p -= (float) bufferSize;

    const auto i0 = (int) p;
    const auto i1 = (i0 + 1) % bufferSize;
    const auto frac = p - (float) i0;
    return buf[(size_t) i0] + frac * (buf[(size_t) i1] - buf[(size_t) i0]);
}

void GrainEngine::spawnGrain()
{
    Grain* slot = nullptr;

    for (auto& g : grains)
    {
        if (! g.active)
        {
            slot = &g;
            break;
        }
    }

    if (slot == nullptr)
        return;

    const auto sizeSamples = juce::jlimit (24.0f, sampleRate * 0.48f, params.sizeMs * 0.001f * sampleRate);
    const auto length = (int) (sizeSamples * juce::jmap (rnd.nextFloat(), 0.55f, 1.45f));

    if (length < 16)
        return;

    const auto maxOffset = juce::jmax (8, bufferSize - length - 8);

    int offset = 0;

    if (params.freeze)
    {
        offset = (int) (juce::jlimit (0.0f, 1.0f, params.position) * (float) maxOffset);
    }
    else
    {
        const auto latency = length + 64;
        const auto range = juce::jmax (1, maxOffset - latency);
        const auto pos = juce::jlimit (0.0f, 1.0f, params.position);
        offset = latency + (int) (pos * pos * (float) range);
    }

    offset += (int) ((rnd.nextFloat() * 2.0f - 1.0f) * params.spray * sampleRate * 0.45f);
    offset = juce::jlimit (1, maxOffset, offset);

    const auto semis = params.pitch + (rnd.nextFloat() * 2.0f - 1.0f) * params.pitchRandom;
    const auto rate = std::pow (2.0f, semis / 12.0f);
    const auto reverse = rnd.nextFloat() < params.reverse;

    const auto pan = (rnd.nextFloat() * 2.0f - 1.0f) * juce::jlimit (0.0f, 1.0f, params.spread);

    slot->active = true;
    slot->length = length;
    slot->age = 0;
    slot->rate = reverse ? -rate : rate;
    slot->reverse = reverse;
    slot->pan = pan;
    slot->gainL = std::cos ((pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
    slot->gainR = std::sin ((pan * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
    slot->tilt = juce::jmap (rnd.nextFloat(), 0.6f, 1.4f);

    auto start = (float) writePos - (float) offset;

    while (start < 0.0f)
        start += (float) bufferSize;

    slot->readPos = start;
}

void GrainEngine::process (float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    const auto wet = juce::jlimit (0.0f, 1.0f, params.mix);
    const auto active = params.on && wet > 0.0005f;
    const auto spawnInterval = (double) sampleRate / (double) juce::jlimit (0.5f, 150.0f, params.density);
    const auto brightness = juce::jlimit (0.05f, 1.0f, params.brightness);
    const auto lpCoef = 1.0f - std::exp (-twoPi * (200.0f + brightness * 11000.0f) / sampleRate);

    float peakActivity = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto inL = left[i];
        const auto inR = right[i];

        if (! params.freeze)
        {
            bufferL[(size_t) writePos] = inL;
            bufferR[(size_t) writePos] = inR;
        }

        float outL = 0.0f, outR = 0.0f;
        int grainCount = 0;

        if (active)
        {
            samplesUntilNext -= 1.0;

            if (samplesUntilNext <= 0.0)
            {
                spawnGrain();
                samplesUntilNext += spawnInterval;
            }

            for (auto& g : grains)
            {
                if (! g.active)
                    continue;

                const auto phase = (float) g.age / (float) g.length;
                const auto env = 0.5f - 0.5f * std::cos (twoPi * juce::jlimit (0.0f, 1.0f, phase));

                const auto sampleL = readInterpolated (bufferL, g.readPos);
                const auto sampleR = readInterpolated (bufferR, g.readPos);

                const auto gain = env * 0.55f * g.tilt;
                outL += sampleL * gain * g.gainL;
                outR += sampleR * gain * g.gainR;

                g.readPos += g.rate;

                if (g.readPos >= (float) bufferSize)
                    g.readPos -= (float) bufferSize;
                else if (g.readPos < 0.0f)
                    g.readPos += (float) bufferSize;

                ++g.age;

                if (g.age >= g.length)
                    g.active = false;

                ++grainCount;
            }

            lowState[0] += (outL - lowState[0]) * lpCoef;
            lowState[1] += (outR - lowState[1]) * lpCoef;

            outL = lowState[0];
            outR = lowState[1];
        }

        if (! params.freeze)
            if (++writePos >= bufferSize)
                writePos = 0;

        if (active)
        {
            left[i] = inL * (1.0f - wet) + outL * wet;
            right[i] = inR * (1.0f - wet) + outR * wet;
        }

        peakActivity = juce::jmax (peakActivity, (float) grainCount / (float) maxGrains);
    }

    activity += (juce::jlimit (0.0f, 1.0f, peakActivity * 2.5f) - activity) * 0.15f;
}

//==============================================================================
// Glitch engine
//==============================================================================
double GlitchEngine::divisionBeats (int index)
{
    static const double beats[] = { 2.0, 1.0, 0.5, 0.25, 0.1875, 0.125, 0.0625 };
    return beats[(size_t) juce::jlimit (0, 6, index)];
}

void GlitchEngine::prepare (double sr)
{
    sampleRate = (float) sr;
    lpCoefficient = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 9000.0 / sr));
    transStep = (float) (1.0 / juce::jmax (1.0, sr * 0.002));
    const auto maxLen = (int) (sr * 2.0) + 8;

    loopL.assign ((size_t) maxLen, 0.0f);
    loopR.assign ((size_t) maxLen, 0.0f);
    tapeL.assign ((size_t) (sr * 0.4) + 8, 0.0f);
    tapeR.assign ((size_t) (sr * 0.4) + 8, 0.0f);

    setTempo (bpm);
    reset();
}

void GlitchEngine::reset()
{
    mode = Mode::normal;
    modeCounter = 0;
    repeatLeft = 0;
    loopWrite = 0;
    loopRead = 0.0f;
    tapeWrite = 0;
    tapeRead = 0.0f;
    holdL = holdR = 0.0f;
    holdCount = 0;
    gateRamp = 0;
    gateValue = 1.0f;
    gateOpen = true;
    dropLeft = 0;
    activity = 0.0f;
    stepCounter = 0.0;
    debrisL = debrisR = 0.0f;
    std::fill (loopL.begin(), loopL.end(), 0.0f);
    std::fill (loopR.begin(), loopR.end(), 0.0f);
}

void GlitchEngine::setTempo (double newBpm)
{
    bpm = juce::jlimit (20.0, 300.0, newBpm);
    stepLength = juce::jmax (32, (int) (60.0 / bpm * divisionBeats (params.stepDivision) * (double) sampleRate));
}

void GlitchEngine::trigger()
{
    if (mode == Mode::normal)
        beginStutter ((int) (60.0 / bpm * divisionBeats (params.stutterDivision) * (double) sampleRate));
}

void GlitchEngine::beginStutter (int lengthSamples)
{
    if (mode != Mode::normal)
        return;

    captureLength = juce::jlimit (32, (int) loopL.size() - 1, lengthSamples);
    mode = Mode::capture;
    modeCounter = 0;
    loopWrite = 0;
    loopRead = 0.0f;
}

void GlitchEngine::onStep()
{
    const auto chaos = juce::jlimit (0.0f, 1.0f, params.chaos);
    const auto scale = 0.25f + chaos * 1.5f;

    // rhythmic gate
    if (rnd.nextFloat() < params.gateChance * scale)
    {
        gateOpen = ! gateOpen;
        gateRamp = 0;
    }
    else if (! gateOpen && rnd.nextFloat() < 0.6f)
    {
        gateOpen = true;
    }

    if (mode != Mode::normal)
        return;

    if (rnd.nextFloat() < params.stutterChance * scale)
    {
        beginStutter ((int) (60.0 / bpm * divisionBeats (params.stutterDivision) * (double) sampleRate));
        return;
    }

    if (rnd.nextFloat() < params.dropChance * scale)
    {
        dropLeft = (int) (sampleRate * (0.008f + rnd.nextFloat() * 0.05f));
    }

    if (rnd.nextFloat() < params.tapeChance * scale)
    {
        mode = Mode::tape;
        tapeLength = juce::jmax (64, stepLength);
        modeCounter = 0;
        tapeRead = 0.0f;
        tapeWrite = 0;
    }
}

void GlitchEngine::process (float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    const auto wet = juce::jlimit (0.0f, 1.0f, params.mix);
    const auto active = params.on && wet > 0.0005f;

    if (! active)
    {
        if (mode != Mode::normal)
            reset();

        activity *= 0.9f;
        return;
    }

    // how loud is the incoming signal? everything below this is treated as silence
    float inPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
        inPeak = juce::jmax (inPeak, std::abs (left[i]), std::abs (right[i]));

    inputEnv += (inPeak - inputEnv) * (inPeak > inputEnv ? 0.25f : 0.004f);

    if (inputEnv < 0.00002f)
    {
        ++silentBlocks;

        if (silentBlocks > 8 && mode != Mode::normal)
            reset();

        activity *= 0.9f;
        debrisL = debrisR = 0.0f;
        return;
    }

    silentBlocks = 0;

    const auto quantLevels = std::pow (2.0f, juce::jlimit (4.0f, 15.0f, params.bits) - 1.0f);
    const auto invQuant = 1.0f / quantLevels;
    const auto rateDiv = juce::jlimit (1.0f, 48.0f, params.rateDiv);
    const auto gateRampSamples = juce::jmax (1.0f, sampleRate * 0.0015f);
    const auto chaos = juce::jlimit (0.0f, 1.0f, params.chaos);
    const auto repeats = juce::jlimit (1.0f, 12.0f, params.repeats);

    // band limit before the sample and hold, otherwise the rate reduction folds
    // the whole spectrum back as harsh digital hash
    const auto preCutoff = juce::jmin (14000.0f, sampleRate / (2.4f * rateDiv));
    const auto preCoefficient = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi
                                                          * (double) preCutoff / (double) sampleRate));

    float peakActivity = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- sequencer
        stepCounter -= 1.0;

        if (stepCounter <= 0.0)
        {
            onStep();
            stepCounter += (double) juce::jmax (32, stepLength);
        }

        float inL = left[i];
        float inR = right[i];

        // ---- antialias then sample rate reduction
        preL += (inL - preL) * preCoefficient;
        preR += (inR - preR) * preCoefficient;
        inL = preL;
        inR = preR;

        holdCount += 1;

        if ((float) holdCount >= rateDiv)
        {
            holdCount = 0;
            holdL = inL;
            holdR = inR;
        }

        inL = holdL;
        inR = holdR;

        // ---- bit crush
        inL = std::round (inL * quantLevels) * invQuant;
        inR = std::round (inR * quantLevels) * invQuant;

        float outL = inL, outR = inR;

        // ---- stutter / tape state machine
        switch (mode)
        {
            case Mode::capture:
            {
                if (loopWrite < captureLength)
                {
                    loopL[(size_t) loopWrite] = inL;
                    loopR[(size_t) loopWrite] = inR;
                    ++loopWrite;
                }

                if (++modeCounter >= captureLength)
                {
                    mode = Mode::repeat;
                    loopRead = 0.0f;
                    repeatLeft = (int) repeats;
                }

                break;
            }
            case Mode::repeat:
            {
                const auto idx = juce::jlimit (0, captureLength - 1, (int) loopRead);
                outL = loopL[(size_t) idx];
                outR = loopR[(size_t) idx];

                loopRead += 1.0f;

                if (loopRead >= (float) captureLength)
                {
                    loopRead -= (float) captureLength;

                    if (--repeatLeft <= 0)
                        mode = Mode::normal;
                }

                break;
            }
            case Mode::tape:
            {
                const auto progress = juce::jlimit (0.0f, 1.0f, (float) modeCounter / (float) tapeLength);
                const auto speed = juce::jmap (progress, 1.0f, 0.02f);

                tapeL[(size_t) tapeWrite] = inL;
                tapeR[(size_t) tapeWrite] = inR;

                if (++tapeWrite >= (int) tapeL.size())
                    tapeWrite = 0;

                tapeRead += speed;

                if (tapeRead >= (float) tapeL.size())
                    tapeRead -= (float) tapeL.size();

                auto readIdx = (float) tapeWrite - tapeRead;

                while (readIdx < 0.0f)
                    readIdx += (float) tapeL.size();

                const auto i0 = (int) readIdx % (int) tapeL.size();
                const auto i1 = (i0 + 1) % (int) tapeL.size();
                const auto frac = readIdx - std::floor (readIdx);

                outL = tapeL[(size_t) i0] + frac * (tapeL[(size_t) i1] - tapeL[(size_t) i0]);
                outR = tapeR[(size_t) i0] + frac * (tapeR[(size_t) i1] - tapeR[(size_t) i0]);

                // fade the tape stop out
                const auto fade = juce::jlimit (0.0f, 1.0f, 1.0f - progress * 1.6f);
                outL *= fade;
                outR *= fade;

                if (++modeCounter >= tapeLength)
                    mode = Mode::normal;

                break;
            }
            case Mode::normal:
            default:
                break;
        }

        // ---- gate
        const auto target = gateOpen ? 1.0f : 0.0f;
        const auto gateStep = 1.0f / gateRampSamples;

        if (gateValue < target) gateValue = juce::jmin (target, gateValue + gateStep);
        else if (gateValue > target) gateValue = juce::jmax (target, gateValue - gateStep);

        outL *= gateValue;
        outR *= gateValue;

        // ---- dropouts and digital debris
        if (dropLeft > 0)
        {
            --dropLeft;
            outL = 0.0f;
            outR = 0.0f;
        }
        else if (params.noise > 0.0005f && inputEnv > 0.002f)
        {
            const auto chance = params.noise * (0.15f + chaos) * 0.00035f;

            if (rnd.nextFloat() < chance)
            {
                // a short filtered tick instead of a full scale single sample click
                const auto amp = 0.16f + rnd.nextFloat() * 0.30f;
                debrisL += (rnd.nextFloat() * 2.0f - 1.0f) * amp;
                debrisR += (rnd.nextFloat() * 2.0f - 1.0f) * amp;
            }
        }

        if (std::abs (debrisL) > 1.0e-5f || std::abs (debrisR) > 1.0e-5f)
        {
            outL += debrisL;
            outR += debrisR;
            debrisL *= 0.93f;
            debrisR *= 0.93f;
        }

        if (mode != lastMode)
        {
            lastMode = mode;
            transFade = 0.0f;
            transFromL = lastOutL;
            transFromR = lastOutR;
        }

        if (transFade < 1.0f)
        {
            transFade = juce::jmin (1.0f, transFade + transStep);
            outL = transFromL + (outL - transFromL) * transFade;
            outR = transFromR + (outR - transFromR) * transFade;
        }

        lastOutL = outL;
        lastOutR = outR;

        outL = softLimit (outL);
        outR = softLimit (outR);

        left[i] = left[i] * (1.0f - wet) + outL * wet;
        right[i] = right[i] * (1.0f - wet) + outR * wet;

        peakActivity = juce::jmax (peakActivity, mode == Mode::normal ? 0.0f : 1.0f);
    }

    activity += (juce::jlimit (0.0f, 1.0f, peakActivity) - activity) * 0.2f;
}

//==============================================================================
void TextureProcessor::prepare (double sampleRate, int)
{
    grain.prepare (sampleRate);
    glitch.prepare (sampleRate);
}

void TextureProcessor::reset()
{
    grain.reset();
    glitch.reset();
}

void TextureProcessor::setParams (const GrainEngine::Params& g, const GlitchEngine::Params& gl, double bpm)
{
    grain.setParams (g);
    glitch.setParams (gl);
    glitch.setTempo (bpm);
}

void TextureProcessor::process (float* left, float* right, int numSamples)
{
    glitch.process (left, right, numSamples);
    grain.process (left, right, numSamples);
}

} // namespace botanic
