#include "Dsp.h"

namespace botanic
{

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    constexpr float pi = juce::MathConstants<float>::pi;

    float hashPhase (int table, int h) noexcept
    {
        auto x = (uint32_t) (h * 2654435761u) ^ (uint32_t) (table * 2246822519u);
        x ^= x >> 13; x *= 1274126177u; x ^= x >> 16;
        return (float) (x & 0xFFFFu) / 65535.0f * twoPi;
    }

    float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

    // Amplitude of harmonic h (1-based) for harmonic-indexed shapes -------------
    float sineAmp (int h)      { return h == 1 ? 1.0f : 0.0f; }
    float triangleAmp (int h)  { return (h % 2) ? (8.0f / (pi * pi * (float) (h * h))) * ((((h - 1) / 2) % 2) == 0 ? 1.0f : -1.0f) : 0.0f; }
    float sawAmp (int h)       { return 1.0f / (float) h; }
    float squareAmp (int h)    { return (h % 2) ? 1.0f / (float) h : 0.0f; }

    float tableAmp (int table, float t, int h)
    {
        switch (table)
        {
            case 0: // Wavetable: sine -> triangle -> saw -> square
            {
                if (t < 0.34f)       return lerp (sineAmp (h), triangleAmp (h), t / 0.34f);
                if (t < 0.67f)       return lerp (triangleAmp (h), sawAmp (h), (t - 0.34f) / 0.33f);
                return lerp (sawAmp (h), squareAmp (h), (t - 0.67f) / 0.33f);
            }
            case 1: // Sine: pure tone morphing into an organ stack
            {
                if (h == 1) return 1.0f;
                if (h <= 8)  return std::pow (t, (float) (h - 1)) * 0.55f;
                return 0.0f;
            }
            case 2: // Triangle: soft odd harmonics, brightening with position
            {
                const auto base = triangleAmp (h);
                const auto bright = sawAmp (h) * 0.7f;
                return lerp (base, bright, t);
            }
            case 3: // Saw: dark to bright tilt
            {
                const auto tilt = std::exp (-(float) h * (1.0f - t) * 0.45f);
                return sawAmp (h) * tilt;
            }
            case 4: // Square with pulse-width morph
            {
                const auto duty = juce::jmap (t, 0.5f, 0.06f);
                const auto pwm = (2.0f / (pi * (float) h)) * std::sin (pi * (float) h * duty);
                return lerp (squareAmp (h), std::abs (pwm), 0.75f);
            }
            case 5: // Glass: sparse shimmering partials
            {
                static const int partials[] = { 1, 3, 5, 8, 11, 15, 21, 29 };
                for (int i = 0; i < 8; ++i)
                    if (partials[i] == h)
                        return std::pow (0.62f, (float) i) * (0.35f + 0.65f * t);
                return 0.0f;
            }
            case 6: // Formant: vowel-like peaks, morphing A -> E -> I -> O -> U
            {
                static const float vowel[5][3] = {
                    {  800.0f, 1150.0f, 2900.0f },
                    {  400.0f, 1700.0f, 2600.0f },
                    {  350.0f, 2100.0f, 2900.0f },
                    {  450.0f,  800.0f, 2830.0f },
                    {  325.0f,  700.0f, 2530.0f }
                };
                const auto fp = juce::jlimit (0.0f, 0.999f, t) * 4.0f;
                const auto vi = (int) fp;
                const auto vf = fp - (float) vi;
                const auto f0 = 110.0f;
                const auto hz = (float) h * f0;
                float gain = 0.0f;
                for (int k = 0; k < 3; ++k)
                {
                    const auto centre = lerp (vowel[vi][k], vowel[vi + 1][k], vf);
                    const auto bw = 90.0f + centre * 0.18f;
                    const auto d = (hz - centre) / bw;
                    gain += (1.0f / (1.0f + k)) * std::exp (-d * d);
                }
                return gain * std::pow ((float) h, -0.55f) + 0.004f / (float) h;
            }
            case 7: // Bells: inharmonic-ish partials
            {
                static const int partials[] = { 1, 2, 3, 4, 5, 7, 9, 11 };
                for (int i = 0; i < 8; ++i)
                    if (partials[i] == h)
                        return std::pow (0.58f, (float) i) * (0.5f + 0.5f * (1.0f - t) * (float) (i % 3 + 1) / 3.0f);
                return 0.0f;
            }
            default: break;
        }
        return 0.0f;
    }
}

//==============================================================================
WavetableBank::WavetableBank()
{
    static const char* names[] = { "Wavetable", "Sine", "Triangle", "Saw", "Square", "Glass", "Formant", "Bells" };

    juce::dsp::FFT fft (11);   // 2048
    std::vector<juce::dsp::Complex<float>> spectrum ((size_t) tableSize), fftOutput ((size_t) tableSize);
    std::vector<float> harmonics ((size_t) tableSize / 2 + 1, 0.0f);

    for (int t = 0; t < numTables; ++t)
    {
        auto& table = tables[(size_t) t];
        table.name = names[t];
        table.data.assign ((size_t) numFrames * numMips * tableSize, 0.0f);

        for (int f = 0; f < numFrames; ++f)
        {
            const auto pos = (float) f / (float) (numFrames - 1);

            for (int h = 1; h <= tableSize / 2; ++h)
                harmonics[(size_t) h] = tableAmp (t, pos, h);

            // peak of the full-bandwidth frame defines the gain for every mip
            float frameGain = 1.0f;
            bool gainKnown = false;

            for (int mip = 0; mip < numMips; ++mip)
            {
                const int limit = juce::jmax (1, (tableSize / 2) >> mip);

                std::fill (spectrum.begin(), spectrum.end(), juce::dsp::Complex<float> {});

                for (int h = 1; h <= limit; ++h)
                {
                    const auto a = harmonics[(size_t) h];
                    if (std::abs (a) < 1.0e-6f)
                        continue;

                    const auto ph = (t <= 4) ? -juce::MathConstants<float>::halfPi : hashPhase (t, h);
                    const auto re = a * std::cos (ph);
                    const auto im = a * std::sin (ph);

                    spectrum[(size_t) h] += juce::dsp::Complex<float> (re, im);
                    spectrum[(size_t) (tableSize - h)] += juce::dsp::Complex<float> (re, -im);
                }

                fft.perform (spectrum.data(), fftOutput.data(), true);

                auto* out = table.data.data() + ((size_t) f * numMips + (size_t) mip) * tableSize;

                float peak = 0.0f;
                for (int i = 0; i < tableSize; ++i)
                {
                    out[i] = fftOutput[(size_t) i].real();
                    peak = juce::jmax (peak, std::abs (out[i]));
                }

                if (! gainKnown)
                {
                    frameGain = peak > 1.0e-6f ? 1.0f / peak : 1.0f;
                    gainKnown = true;
                }

                for (int i = 0; i < tableSize; ++i)
                    out[i] *= frameGain;
            }
        }
    }
}

const WavetableBank& WavetableBank::instance()
{
    static WavetableBank bank;
    return bank;
}

} // namespace botanic
