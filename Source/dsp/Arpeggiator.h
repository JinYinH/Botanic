#pragma once

#include <JuceHeader.h>
#include <vector>
#include <functional>

namespace botanic
{

/** Tempo-synced arpeggiator.

    Notes are collected while enabled and re-emitted with sample-accurate offsets
    so the host hears proper timing.
*/
class Arpeggiator
{
public:
    using NoteCallback = std::function<void (int sampleOffset, int note, float velocity, bool isOn)>;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        reset();
    }

    void reset()
    {
        held.clear();
        playedOrder.clear();
        stepCounter = 0;
        nextStepIn = 0.0f;
        gateLeft = 0.0f;
        gateNote = -1;
        pendingOff = -1;
        latchHeld = false;
    }

    void setParams (bool isOn, int mode, int rateIndex, int octaves, float gate, float swing, bool latch, double bpm)
    {
        enabled = isOn;
        arpModeValue = juce::jlimit (0, 5, mode);
        rateIdx = juce::jlimit (0, 5, rateIndex);
        octaveCount = juce::jlimit (1, 4, octaves);
        gateAmount = juce::jlimit (0.05f, 1.0f, gate);
        swingAmount = juce::jlimit (0.0f, 0.6f, swing);
        latchMode = latch;
        tempo = bpm > 1.0 ? bpm : 120.0;

        if (! enabled && previousEnabled)
        {
            if (gateNote >= 0)
            {
                pendingOff = gateNote;
                gateNote = -1;
            }
            held.clear();
            playedOrder.clear();
            latchHeld = false;
        }

        previousEnabled = enabled;
    }

    void noteOn (int note, float velocity)
    {
        if (! enabled)
            return;

        lastVelocity = velocity;

        if (latchMode && latchHeld && held.empty())
        {
            playedOrder.clear();
            latchHeld = false;
        }

        if (std::find (held.begin(), held.end(), note) == held.end())
        {
            held.push_back (note);
            playedOrder.push_back (note);
        }
    }

    void noteOff (int note)
    {
        if (! enabled)
            return;

        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        playedOrder.erase (std::remove (playedOrder.begin(), playedOrder.end(), note), playedOrder.end());

        if (latchMode && held.empty())
            latchHeld = true;
    }

    bool isEnabled() const noexcept { return enabled; }
    bool hasNotes() const noexcept { return ! held.empty() || latchHeld; }

    /** Emits note events for this block. Call once per block. */
    void process (int numSamples, const NoteCallback& cb)
    {
        if (pendingOff >= 0)
        {
            cb (0, pendingOff, 0.0f, false);
            pendingOff = -1;
        }

        if (! enabled)
            return;

        if (held.empty() && ! latchHeld)
        {
            if (gateNote >= 0)
            {
                cb (0, gateNote, 0.0f, false);
                gateNote = -1;
            }
            nextStepIn = 0.0f;
            stepCounter = 0;
            return;
        }

        const auto stepLen = stepLengthSamples();
        float t = 0.0f;
        int safety = 0;

        while (t < (float) numSamples && ++safety < 4096)
        {
            const auto nextStepAt = t + juce::jmax (0.0f, nextStepIn);
            const auto gateAt = gateNote >= 0 ? t + juce::jmax (0.0f, gateLeft) : 1.0e9f;

            if (gateAt <= nextStepAt)
            {
                if (gateAt >= (float) numSamples)
                {
                    gateLeft -= (float) numSamples - t;
                    nextStepIn -= (float) numSamples - t;
                    t = (float) numSamples;
                    break;
                }

                t = gateAt;
                cb ((int) juce::jlimit (0.0f, (float) (numSamples - 1), t), gateNote, 0.0f, false);
                gateNote = -1;
                gateLeft = 0.0f;
                continue;
            }

            if (nextStepAt >= (float) numSamples)
            {
                nextStepIn -= (float) numSamples - t;
                if (gateNote >= 0)
                    gateLeft -= (float) numSamples - t;
                t = (float) numSamples;
                break;
            }

            t = nextStepAt;
            nextStepIn = 0.0f;

            if (gateNote >= 0)
            {
                cb ((int) t, gateNote, 0.0f, false);
                gateNote = -1;
            }

            const auto note = pickNote();
            if (note >= 0)
            {
                const auto offset = (int) juce::jlimit (0.0f, (float) (numSamples - 1), t);
                cb (offset, note, lastVelocity, true);
                gateNote = note;
                gateLeft = (stepLen * gateAmount);
            }

            const auto swingMul = (stepCounter % 2 == 0) ? (1.0f + swingAmount) : (1.0f - swingAmount);
            nextStepIn = juce::jmax (1.0f, stepLen * swingMul);
            ++stepCounter;
        }

        if (gateNote >= 0 && gateLeft > 0.0f)
            gateLeft = juce::jmax (0.0f, gateLeft);
    }

private:
    float stepLengthSamples() const
    {
        static const double beats[] = { 1.0, 0.5, 0.5 * 2.0 / 3.0, 0.25, 0.25 * 2.0 / 3.0, 0.125 };
        const auto secondsPerBeat = 60.0 / juce::jlimit (20.0, 300.0, tempo);
        return (float) (secondsPerBeat * beats[rateIdx] * (double) sampleRate);
    }

    int pickNote()
    {
        if (held.empty())
            return -1;

        std::vector<int> notes = (arpModeValue == 4) ? playedOrder : held;
        if (notes.empty())
            notes = held;

        if (arpModeValue != 4)
            std::sort (notes.begin(), notes.end());

        const auto noteCount = (int) notes.size();
        const auto count = noteCount * octaveCount;
        if (count <= 0)
            return -1;

        int index = 0;

        if (arpModeValue == 2) // up-down
        {
            const auto period = juce::jmax (1, count * 2 - 2);
            const auto local = stepCounter % period;
            index = local < count ? local : period - local;
        }
        else if (arpModeValue == 3) // random
        {
            index = juce::Random::getSystemRandom().nextInt (count);
        }
        else
        {
            index = stepCounter % count;
        }

        const auto octave = index / noteCount;
        return juce::jlimit (0, 127, notes[(size_t) (index % noteCount)] + octave * 12);
    }

    float sampleRate = 44100.0f;
    bool enabled = false, previousEnabled = false, latchMode = false, latchHeld = false;
    int arpModeValue = 0, rateIdx = 3, octaveCount = 1;
    float gateAmount = 0.5f, swingAmount = 0.0f;
    double tempo = 120.0;

    std::vector<int> held, playedOrder;
    int stepCounter = 0, gateNote = -1, pendingOff = -1;
    float nextStepIn = 0.0f, gateLeft = 0.0f;
    float lastVelocity = 0.8f;
};

} // namespace botanic
