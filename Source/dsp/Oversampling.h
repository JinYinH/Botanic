#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

namespace botanic
{

/**
    Linear phase half-band decimator (2:1).

    Used to run the voice engine at twice the host sample rate so the ladder
    filter's saturation and the wavetable morphing stay clean, then band-limit
    the result before it goes back to the host rate.
*/
class HalfBandDecimator
{
public:
    static constexpr int numTaps = 63;   // odd, symmetric around the centre tap
    static constexpr int latencyInBaseSamples = 16;

    HalfBandDecimator()
    {
        std::array<double, numTaps> h {};
        double sum = 0.0;

        for (int i = 0; i < numTaps; ++i)
        {
            const auto k = i - (numTaps - 1) / 2;
            double v = 0.0;

            if (k == 0)
                v = 0.5;
            else if ((k % 2) != 0)
                v = std::sin (juce::MathConstants<double>::pi * k * 0.5)
                      / (juce::MathConstants<double>::pi * k);

            const auto x = (double) i / (double) (numTaps - 1);
            const auto window = 0.42 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * x)
                                     + 0.08 * std::cos (4.0 * juce::MathConstants<double>::pi * x);

            h[(size_t) i] = v * window;
            sum += h[(size_t) i];
        }

        for (int i = 0; i < numTaps; ++i)
            taps[(size_t) i] = (float) (h[(size_t) i] / sum);

        reset();
    }

    void reset() noexcept
    {
        history.fill (0.0f);
        writeIndex = 0;
    }

    /** Consumes 2 * numOutputSamples input samples, writes numOutputSamples. */
    void process (const float* input, float* output, int numOutputSamples) noexcept
    {
        for (int n = 0; n < numOutputSamples; ++n)
        {
            history[(size_t) writeIndex] = input[n * 2];
            if (++writeIndex >= numTaps) writeIndex = 0;

            history[(size_t) writeIndex] = input[n * 2 + 1];
            if (++writeIndex >= numTaps) writeIndex = 0;

            float acc = 0.0f;
            int index = writeIndex;

            for (int i = 0; i < numTaps; ++i)
            {
                if (--index < 0) index = numTaps - 1;
                acc += taps[(size_t) i] * history[(size_t) index];
            }

            output[n] = acc;
        }
    }

private:
    std::array<float, numTaps> taps {};
    std::array<float, numTaps> history {};
    int writeIndex = 0;
};

/** One pole DC blocker used after the saturating stages. */
class DcBlocker
{
public:
    void prepare (double sampleRate) noexcept
    {
        const auto cutoff = 8.0;
        coefficient = (float) std::exp (-juce::MathConstants<double>::twoPi * cutoff / sampleRate);
        reset();
    }

    void reset() noexcept { x1 = 0.0f; y1 = 0.0f; }

    float process (float x) noexcept
    {
        const auto y = x - x1 + coefficient * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    float coefficient = 0.999f;
    float x1 = 0.0f, y1 = 0.0f;
};

} // namespace botanic
