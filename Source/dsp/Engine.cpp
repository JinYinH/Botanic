#include "Engine.h"

namespace botanic
{

Engine::Engine()
{
    heldNotes.reserve (32);
}

void Engine::prepare (double sr, int)
{
    sampleRate = (float) sr;
    for (auto& v : voices)
        v.prepare (sr);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices)
        v.kill();
    startOrder.fill (0);
    counter = 0;
    nextVoice = 0;
    monoVoice = -1;
    heldNotes.clear();
    outputLevel = 0.0f;
}

int Engine::findFreeVoice()
{
    const auto limit = juce::jlimit (1, maxVoices, params.maxVoices);

    for (int i = 0; i < limit; ++i)
        if (! voices[(size_t) i].isActive())
            return i;

    // steal the oldest / quietest voice
    int best = 0;
    float bestScore = 1.0e9f;

    for (int i = 0; i < limit; ++i)
    {
        const auto age = (float) (counter - startOrder[(size_t) i]);
        const auto score = age * (0.35f + voices[(size_t) i].getLevel());
        if (score < bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    return best;
}

void Engine::startVoice (int note, float velocity)
{
    const auto index = findFreeVoice();
    auto& v = voices[(size_t) index];

    if (v.isActive() && v.isKeyDown())
        v.fastRelease();

    v.noteOn (note, velocity, params);
    startOrder[(size_t) index] = ++counter;
}

void Engine::noteOn (int note, float velocity)
{
    lastNote = note;
    const auto mono = params.polyMode != 0;

    if (mono)
    {
        auto it = std::find (heldNotes.begin(), heldNotes.end(), note);
        if (it == heldNotes.end())
            heldNotes.push_back (note);

        if (monoVoice >= 0 && voices[(size_t) monoVoice].isActive())
        {
            auto& v = voices[(size_t) monoVoice];
            if (params.polyMode == 1)          // mono: retrigger envelopes
                v.noteOn (note, velocity, params);
            else                               // legato: just glide
                v.glideTo (note, params, params.glideMs < 0.5f);

            startOrder[(size_t) monoVoice] = ++counter;
        }
        else
        {
            monoVoice = findFreeVoice();
            voices[(size_t) monoVoice].noteOn (note, velocity, params);
            startOrder[(size_t) monoVoice] = ++counter;
        }

        return;
    }

    // poly: reuse a voice already playing this note
    for (auto& v : voices)
    {
        if (v.isActive() && v.isKeyDown() && v.getNote() == note)
        {
            v.noteOn (note, velocity, params);
            return;
        }
    }

    startVoice (note, velocity);
}

void Engine::noteOff (int note)
{
    const auto mono = params.polyMode != 0;

    if (mono)
    {
        const auto it = std::find (heldNotes.begin(), heldNotes.end(), note);
        if (it != heldNotes.end())
            heldNotes.erase (it);

        if (monoVoice >= 0 && voices[(size_t) monoVoice].isActive())
        {
            if (heldNotes.empty())
                voices[(size_t) monoVoice].noteOff();
            else
                voices[(size_t) monoVoice].glideTo (heldNotes.back(), params, params.glideMs < 0.5f);
        }
        return;
    }

    for (auto& v : voices)
        if (v.isActive() && v.getNote() == note)
            v.noteOff();
}

void Engine::allNotesOff (bool allowTailOff)
{
    heldNotes.clear();
    for (auto& v : voices)
    {
        if (allowTailOff)
            v.noteOff();
        else
            v.kill();
    }
    monoVoice = -1;
}

void Engine::render (float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    for (auto& v : voices)
        if (v.isActive())
            v.render (left, right, numSamples, params);

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax (peak, std::abs (left[i]), std::abs (right[i]));

    outputLevel = peak;
}

int Engine::getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (auto& v : voices)
        if (v.isActive())
            ++count;
    return count;
}

std::array<juce::uint64, 2> Engine::getNoteMask() const noexcept
{
    std::array<juce::uint64, 2> mask {};
    for (auto& v : voices)
        if (v.isActive())
        {
            const auto n = juce::jlimit (0, 127, v.getNote());
            mask[(size_t) (n / 64)] |= ((juce::uint64) 1 << (n % 64));
        }
    return mask;
}

} // namespace botanic
