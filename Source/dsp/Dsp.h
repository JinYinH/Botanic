#pragma once

#include <JuceHeader.h>
#include <array>

/**
    Core DSP building blocks for Botanic: band-limited wavetables, oscillators,
    envelopes, LFOs and the two filter models.
*/
namespace botanic
{

//==============================================================================
/** Builds the eight factory wavetables (16 frames, 9 mip levels each).

    Tables are generated with an inverse FFT from additive harmonic spectra, so
    every frame is band-limited per mip level. Generation happens once, lazily.
*/
class WavetableBank
{
public:
    static constexpr int numTables = 8;
    static constexpr int numFrames = 16;
    static constexpr int tableSize = 2048;
    static constexpr int numMips   = 9;

    struct Table
    {
        juce::String name;
        std::vector<float> data;   // numFrames * numMips * tableSize

        const float* frame (int f, int mip) const noexcept
        {
            return data.data() + ((size_t) f * numMips + (size_t) mip) * tableSize;
        }
    };

    static const WavetableBank& instance();

    const Table& getTable (int index) const noexcept
    {
        return tables[(size_t) juce::jlimit (0, numTables - 1, index)];
    }

    static int mipForFrequency (float freq, float sampleRate) noexcept
    {
        const auto maxHarmonic = juce::jmax (1.0f, (sampleRate * 0.5f) / juce::jmax (1.0f, freq));
        int mip = 0;
        int limit = 1024;
        while (mip < numMips - 1 && (float) limit > maxHarmonic)
        {
            limit >>= 1;
            ++mip;
        }
        return mip;
    }

    /** Reads the morphing frame with linear interpolation in both axes. */
    static float sample (const Table& t, float position, int mip, float phase) noexcept
    {
        const auto fpos = juce::jlimit (0.0f, (float) (numFrames - 1), position * (float) (numFrames - 1));
        const auto f0 = (int) fpos;
        const auto f1 = juce::jmin (f0 + 1, numFrames - 1);
        const auto ft = fpos - (float) f0;

        const auto x = (phase - std::floor (phase)) * (float) tableSize;
        const auto i0 = (int) x & (tableSize - 1);
        const auto i1 = (i0 + 1) & (tableSize - 1);
        const auto xt = x - std::floor (x);

        const auto* a = t.frame (f0, mip);
        const auto* b = t.frame (f1, mip);

        const auto sa = a[i0] + xt * (a[i1] - a[i0]);
        const auto sb = b[i0] + xt * (b[i1] - b[i0]);
        return sa + ft * (sb - sa);
    }

private:
    WavetableBank();
    std::array<Table, numTables> tables;
};

//==============================================================================
/** Simple, band-limited multi-unison wavetable oscillator. */
class WtOscillator
{
public:
    static constexpr int maxUnison = 8;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        resetPhases (0.0f);
    }

    void resetPhases (float phase) noexcept
    {
        for (auto& u : unison)
            u.phase = phase + u.spreadPhase;
    }

    void setUnison (int count, float detuneCents, float spreadAmount) noexcept
    {
        const auto n = juce::jlimit (1, maxUnison, count);
        const auto changed = (n != unisonCount) || (detuneCents != lastDetune) || (spreadAmount != lastSpread);

        unisonCount = n;
        lastDetune = detuneCents;
        lastSpread = spreadAmount;

        if (! changed)
            return;

        for (int i = 0; i < maxUnison; ++i)
        {
            const auto t = (n > 1) ? (float) i / (float) (n - 1) : 0.5f;
            const auto cents = (t - 0.5f) * 2.0f * detuneCents;
            const auto p = (t - 0.5f) * 2.0f * spreadAmount;

            unison[(size_t) i].ratio = std::pow (2.0f, cents / 1200.0f);
            unison[(size_t) i].gainL = std::cos ((p * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
            unison[(size_t) i].gainR = std::sin ((p * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi);
            unison[(size_t) i].spreadPhase = 0.6180339887f * (float) i;
        }
    }

    void render (const WavetableBank::Table& t, float freq, float position, float& outL, float& outR) noexcept
    {
        const auto phaseInc = freq / sampleRate;
        const auto mip = WavetableBank::mipForFrequency (freq, sampleRate);
        float l = 0.0f, r = 0.0f;

        for (int i = 0; i < unisonCount; ++i)
        {
            auto& u = unison[(size_t) i];
            u.phase += phaseInc * u.ratio;
            if (u.phase >= 1.0f) u.phase -= std::floor (u.phase);

            const auto s = WavetableBank::sample (t, position, mip, u.phase);
            l += s * u.gainL;
            r += s * u.gainR;
        }

        const auto norm = 1.0f / std::sqrt ((float) unisonCount);
        outL = l * norm;
        outR = r * norm;
    }

private:
    struct Uni { float phase = 0.0f, ratio = 1.0f, gainL = 0.7071f, gainR = 0.7071f, spreadPhase = 0.0f; };

    float sampleRate = 44100.0f;
    std::array<Uni, maxUnison> unison {};
    int unisonCount = 1;
    float lastDetune = 0.0f, lastSpread = 0.35f;
};

//==============================================================================
/** Analog-style ADSR with exponential segments. */
class AdsrEnv
{
public:
    void prepare (double sr) { sampleRate = (float) sr; update(); }

    void setParams (float a, float d, float s, float r) noexcept
    {
        if (a == attackTime && d == decayTime && s == sustainLevel && r == releaseTime)
            return;
        attackTime = a; decayTime = d; sustainLevel = s; releaseTime = r;
        update();
    }

    void noteOn() noexcept  { stage = Stage::attack; }
    void noteOff() noexcept { if (stage != Stage::idle) stage = Stage::release; }
    void reset() noexcept   { stage = Stage::idle; level = 0.0f; }

    void fastRelease() noexcept
    {
        if (stage != Stage::idle)
        {
            stage = Stage::release;
            releaseCoef = coefficient (0.005f, sampleRate);
        }
    }
    bool isActive() const noexcept { return stage != Stage::idle; }

    float next() noexcept
    {
        switch (stage)
        {
            case Stage::attack:
                level += (attackTarget - level) * attackCoef;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::decay; }
                break;
            case Stage::decay:
                level += (sustainLevel - level) * decayCoef;
                if (std::abs (level - sustainLevel) < 0.001f) { level = sustainLevel; stage = Stage::sustain; }
                break;
            case Stage::sustain:
                level += (sustainLevel - level) * decayCoef;
                break;
            case Stage::release:
                level += (releaseTarget - level) * releaseCoef;
                if (level < 0.0001f) { level = 0.0f; stage = Stage::idle; }
                break;
            case Stage::idle:
            default:
                level = 0.0f;
                break;
        }
        return level;
    }

    float getLevel() const noexcept { return level; }

private:
    enum class Stage { idle, attack, decay, sustain, release };

    static float coefficient (float timeSec, float sr) noexcept
    {
        if (timeSec <= 0.0001f) return 1.0f;
        return 1.0f - std::exp (-1.0f / (timeSec * sr));
    }

    void update() noexcept
    {
        attackCoef = coefficient (attackTime * 0.6f, sampleRate);
        decayCoef = coefficient (decayTime * 0.5f, sampleRate);
        releaseCoef = coefficient (releaseTime * 0.5f, sampleRate);
        attackTarget = 1.25f;
        releaseTarget = -0.02f;
    }

    float sampleRate = 44100.0f;
    float attackTime = 0.01f, decayTime = 0.4f, sustainLevel = 0.7f, releaseTime = 0.3f;
    float attackCoef = 0.1f, decayCoef = 0.0001f, releaseCoef = 0.0001f;
    float attackTarget = 1.25f, releaseTarget = -0.02f;
    float level = 0.0f;
    Stage stage = Stage::idle;
};

//==============================================================================
/** Global-shape LFO with sync support. */
class Lfo
{
public:
    enum Shape { triangle, sine, sawUp, sawDown, square, sampleHold, random, chaos, pulse };

    void prepare (double sr) { sampleRate = (float) sr; }

    void setRate (float hz) noexcept { rateHz = juce::jmax (0.001f, hz); }
    void setShape (int s) noexcept   { shape = (Shape) juce::jlimit (0, 8, s); }
    void setPhase (float p) noexcept
    {
        if (retriggerPhase != p) { retriggerPhase = p; }
    }
    void setRetrigger (bool r) noexcept { retrigger = r; }

    void reset (float startPhase) noexcept
    {
        phase = startPhase;
        heldValue = 0.0f;
        lastShapePhase = -1.0f;
    }

    float next (juce::Random& rnd) noexcept
    {
        const auto prevPhase = phase;
        phase += rateHz / sampleRate;
        if (phase >= 1.0f) phase -= std::floor (phase);

        const auto wrapped = phase < prevPhase;

        switch (shape)
        {
            case triangle:  return 4.0f * std::abs (phase - 0.5f) - 1.0f;
            case sine:      return std::sin (phase * juce::MathConstants<float>::twoPi);
            case sawUp:     return 2.0f * phase - 1.0f;
            case sawDown:   return 1.0f - 2.0f * phase;
            case square:    return phase < 0.5f ? 1.0f : -1.0f;
            case sampleHold:
                if (wrapped || lastShapePhase < 0.0f) heldValue = rnd.nextFloat() * 2.0f - 1.0f;
                return heldValue;
            case random:
            default:
                if (wrapped || lastShapePhase < 0.0f)
                {
                    target = rnd.nextFloat() * 2.0f - 1.0f;
                    heldValue = target;
                }
                return heldValue;
            case chaos:
                chaosValue += (rnd.nextFloat() * 2.0f - 1.0f) * 0.02f;
                chaosValue = juce::jlimit (-1.0f, 1.0f, chaosValue * 0.999f);
                return chaosValue;
            case pulse:
                return phase < 0.18f ? 1.0f - phase * 4.5f : -0.15f;
        }
    }

    bool retrigger = true;

private:
    float sampleRate = 44100.0f;
    float rateHz = 1.0f;
    float phase = 0.0f;
    float retriggerPhase = 0.0f;
    float heldValue = 0.0f, target = 0.0f, lastShapePhase = -1.0f, chaosValue = 0.0f;
    Shape shape = triangle;
};

//==============================================================================
/** Two-pole TPT state variable filter. */
class SvfFilter
{
public:
    enum Mode { lowPass, bandPass, highPass, notch };

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    void set (float cutoffHz, float res01, float sr) noexcept
    {
        const auto f = juce::jlimit (15.0f, sr * 0.45f, cutoffHz);
        g = std::tan (juce::MathConstants<float>::pi * f / sr);
        k = 2.0f - 1.95f * juce::jlimit (0.0f, 1.0f, res01);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float x, Mode mode) noexcept
    {
        const auto v3 = x - ic2;
        const auto v1 = a1 * ic1 + a2 * v3;
        const auto v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        switch (mode)
        {
            case lowPass:  return v2;
            case bandPass: return v1;
            case highPass: return x - k * v1 - v2;
            case notch:    return x - k * v1;
        }
        return v2;
    }

private:
    float g = 0.1f, k = 1.0f, a1 = 1.0f, a2 = 0.1f, a3 = 0.01f;
    float ic1 = 0.0f, ic2 = 0.0f;
};

//==============================================================================
/** Four-pole ladder low-pass with feedback saturation (zero-delay structure). */
class LadderFilter
{
public:
    void reset() noexcept { s1 = s2 = s3 = s4 = 0.0f; }

    void set (float cutoffHz, float res01, float sr) noexcept
    {
        const auto f = juce::jlimit (15.0f, sr * 0.42f, cutoffHz);
        const auto g = std::tan (juce::MathConstants<float>::pi * f / sr);
        G = g / (1.0f + g);
        k = 4.1f * juce::jlimit (0.0f, 0.995f, res01);
    }

    float process (float x, float driveAmount) noexcept
    {
        const auto A1 = G,           B1 = s1 * (1.0f - G);
        const auto A2 = G * A1,      B2 = G * (B1 - s2) + s2;
        const auto A3 = G * A2,      B3 = G * (B2 - s3) + s3;
        const auto A4 = G * A3,      B4 = G * (B3 - s4) + s4;

        auto u = (x - k * B4) / (1.0f + k * A4);

        if (driveAmount > 0.0005f)
        {
            // saturate with level compensation so turning drive up adds harmonics
            // instead of just making everything quieter
            const auto g = 1.0f + driveAmount * 3.0f;
            const auto makeup = 1.0f / std::sqrt (g);
            const auto saturated = std::tanh (u * g) * makeup;
            u = u + (saturated - u) * juce::jmin (1.0f, driveAmount * 2.0f);
        }

        const auto y1 = G * (u - s1) + s1;
        const auto y2 = G * (y1 - s2) + s2;
        const auto y3 = G * (y2 - s3) + s3;
        const auto y4 = G * (y3 - s4) + s4;

        s1 = 2.0f * y1 - s1;
        s2 = 2.0f * y2 - s2;
        s3 = 2.0f * y3 - s3;
        s4 = 2.0f * y4 - s4;

        return y4 * (1.0f + k * 0.85f);
    }

private:
    float G = 0.1f, k = 2.0f;
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
};

//==============================================================================
/** Three band formant (vowel) filter with morphing between A E I O U.

    A time domain alternative to spectral formant shifting: three resonant
    band passes are placed on the vowel formants and summed, so the spectral
    envelope can be reshaped without touching the pitch.
*/
class FormantBank
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = (float) sr;
        reset();
    }

    void reset() noexcept
    {
        for (auto& b : bands)
        {
            b.l.reset();
            b.r.reset();
        }
    }

    /** vowel 0..1 morphs A -> E -> I -> O -> U, shift is in cents. */
    void set (float vowel, float shiftCents, float resonance, float keyTrackOctaves) noexcept
    {
        const auto pos = juce::jlimit (0.0f, 0.999f, vowel) * 4.0f;
        const auto v0 = (int) pos;
        const auto v1 = juce::jmin (4, v0 + 1);
        const auto t = pos - (float) v0;
        const auto semitones = shiftCents * 0.01f + keyTrackOctaves * 12.0f;
        const auto shift = std::pow (2.0f, semitones / 12.0f);

        for (int i = 0; i < 3; ++i)
        {
            const auto f = juce::jmap (t, vowelTable[v0][i], vowelTable[v1][i]) * shift;
            const auto g = juce::jmap (t, vowelGain[v0][i], vowelGain[v1][i]);
            auto& b = bands[(size_t) i];
            b.frequency = juce::jlimit (60.0f, sampleRate * 0.45f, f);
            b.gain = g;
            b.l.set (b.frequency, juce::jlimit (0.0f, 0.995f, resonance), sampleRate);
            b.r.set (b.frequency, juce::jlimit (0.0f, 0.995f, resonance), sampleRate);
        }
    }

    void process (float& left, float& right) noexcept
    {
        float accL = 0.0f, accR = 0.0f;

        for (auto& b : bands)
        {
            accL += b.l.process (left, SvfFilter::bandPass) * b.gain;
            accR += b.r.process (right, SvfFilter::bandPass) * b.gain;
        }

        left = accL;
        right = accR;
    }

private:
    struct Band
    {
        SvfFilter l, r;
        float frequency = 1000.0f;
        float gain = 1.0f;
    };

    static constexpr float vowelTable[5][3] = {
        { 800.0f, 1150.0f, 2900.0f },   // A
        { 400.0f, 1700.0f, 2600.0f },   // E
        { 350.0f, 2100.0f, 2900.0f },   // I
        { 450.0f,  800.0f, 2830.0f },   // O
        { 325.0f,  700.0f, 2530.0f }    // U
    };

    static constexpr float vowelGain[5][3] = {
        { 1.0f, 0.62f, 0.22f },
        { 1.0f, 0.50f, 0.20f },
        { 1.0f, 0.45f, 0.22f },
        { 1.0f, 0.38f, 0.16f },
        { 1.0f, 0.30f, 0.14f }
    };

    std::array<Band, 3> bands {};
    float sampleRate = 44100.0f;
};

//==============================================================================
inline float midiToFreq (float note, float a4) noexcept
{
    return a4 * std::exp2 ((note - 69.0f) / 12.0f);
}

inline float dbToGain (float db) noexcept
{
    return std::pow (10.0f, db / 20.0f);
}

} // namespace botanic
