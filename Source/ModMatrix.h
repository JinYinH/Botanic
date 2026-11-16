#pragma once

#include <JuceHeader.h>
#include <array>

#include "Params.h"

/** Modulation matrix shared by the engine and the UI. */
namespace botanic
{

enum class ModSource
{
    lfo1 = 0, lfo2, env1, env2, velocity, modWheel, keyTrack, random, aftertouch, chaos, numSources
};

enum class ModDest
{
    cutoff = 0, resonance, filterDrive, pitch,
    osc1Level, osc2Level, osc3Level,
    osc1Pos, osc2Pos, osc3Pos,
    osc1Detune, pan, reverbMix, delayMix,
    grainPosition, grainMix, glitchMix, glitchBits, formantShift,
    numDests
};

inline constexpr int numModSources = (int) ModSource::numSources;
inline constexpr int numModDests   = (int) ModDest::numDests;
inline constexpr int maxModSlots   = 24;

inline const char* modSourceName (int i)
{
    static const char* names[] = { "LFO 1", "LFO 2", "ENV 1", "ENV 2", "Velocity", "Mod Wheel", "Key Track", "Random", "Aftertouch", "Chaos" };
    return (i >= 0 && i < numModSources) ? names[i] : "";
}

inline const char* modSourceShort (int i)
{
    static const char* names[] = { "L1", "L2", "E1", "E2", "VEL", "MW", "KEY", "RND", "AT", "CHS" };
    return (i >= 0 && i < numModSources) ? names[i] : "";
}

inline const char* modDestName (int i)
{
    static const char* names[] = {
        "Cutoff", "Resonance", "Drive", "Pitch",
        "OSC1 Level", "OSC2 Level", "OSC3 Level",
        "OSC1 Pos", "OSC2 Pos", "OSC3 Pos",
        "OSC1 Detune", "Pan", "Reverb Mix", "Delay Mix",
        "Grain Position", "Grain Mix", "Glitch Mix", "Glitch Bits", "Vowel Shift"
    };
    return (i >= 0 && i < numModDests) ? names[i] : "";
}

inline const char* modDestParamId (int i)
{
    switch ((ModDest) i)
    {
        case ModDest::cutoff:      return Params::cutoff;
        case ModDest::resonance:   return Params::resonance;
        case ModDest::filterDrive: return Params::drive;
        case ModDest::osc1Level:   return "osc1_level";
        case ModDest::osc2Level:   return "osc2_level";
        case ModDest::osc3Level:   return "osc3_level";
        case ModDest::osc1Pos:     return "osc1_pos";
        case ModDest::osc2Pos:     return "osc2_pos";
        case ModDest::osc3Pos:     return "osc3_pos";
        case ModDest::osc1Detune:  return "osc1_detune";
        case ModDest::reverbMix:   return Params::revMix;
        case ModDest::delayMix:    return Params::dlyMix;
        case ModDest::grainPosition: return Params::grPosition;
        case ModDest::grainMix:      return Params::grMix;
        case ModDest::glitchMix:     return Params::glMix;
        case ModDest::glitchBits:    return Params::glBits;
        case ModDest::formantShift:  return Params::fmtShift;
        default: break;
    }
    return nullptr;
}

inline bool isGlobalModDest (int i) noexcept
{
    switch ((ModDest) i)
    {
        case ModDest::reverbMix:
        case ModDest::delayMix:
        case ModDest::grainPosition:
        case ModDest::grainMix:
        case ModDest::glitchMix:
        case ModDest::glitchBits:
            return true;
        default:
            break;
    }
    return false;
}

struct ModSlot
{
    int source = -1;
    int dest = -1;
    float amount = 0.0f;
};

class ModMatrix
{
public:
    ModMatrix() { slots.fill ({}); }

    void clear()
    {
        slots.fill ({});
        ++revision;
    }

    int find (int source, int dest) const noexcept
    {
        for (int i = 0; i < maxModSlots; ++i)
            if (slots[(size_t) i].source == source && slots[(size_t) i].dest == dest)
                return i;
        return -1;
    }

    /** Adds or updates a routing. Returns false when there is no room left. */
    bool set (int source, int dest, float amount)
    {
        if (source < 0 || dest < 0 || source >= numModSources || dest >= numModDests)
            return false;

        auto index = find (source, dest);

        if (index < 0)
        {
            if (amount == 0.0f)
                return true;

            for (int i = 0; i < maxModSlots; ++i)
            {
                if (slots[(size_t) i].source < 0)
                {
                    index = i;
                    break;
                }
            }
        }

        if (index < 0)
            return false;

        slots[(size_t) index].source = source;
        slots[(size_t) index].dest = dest;
        slots[(size_t) index].amount = juce::jlimit (-1.0f, 1.0f, amount);

        if (std::abs (slots[(size_t) index].amount) < 1.0e-4f)
            slots[(size_t) index] = {};

        ++revision;
        return true;
    }

    float get (int source, int dest) const noexcept
    {
        const auto i = find (source, dest);
        return i >= 0 ? slots[(size_t) i].amount : 0.0f;
    }

    /** Total modulation for a destination, used to draw rings on knobs. */
    float totalForDest (int dest) const noexcept
    {
        float total = 0.0f;
        for (auto& s : slots)
            if (s.source >= 0 && s.dest == dest)
                total += s.amount;
        return total;
    }

    const std::array<ModSlot, maxModSlots>& getSlots() const noexcept { return slots; }

    void toValueTree (juce::ValueTree& parent) const
    {
        juce::ValueTree m ("MODMATRIX");
        for (auto& s : slots)
        {
            if (s.source < 0 || s.dest < 0)
                continue;
            juce::ValueTree child ("SLOT");
            child.setProperty ("source", s.source, nullptr);
            child.setProperty ("dest", s.dest, nullptr);
            child.setProperty ("amount", s.amount, nullptr);
            m.appendChild (child, nullptr);
        }
        parent.appendChild (m, nullptr);
    }

    void fromValueTree (const juce::ValueTree& parent)
    {
        slots.fill ({});
        const auto m = parent.getChildWithName ("MODMATRIX");
        if (! m.isValid())
            return;

        for (const auto& child : m)
        {
            ModSlot s;
            s.source = (int) child.getProperty ("source", -1);
            s.dest = (int) child.getProperty ("dest", -1);
            s.amount = (float) child.getProperty ("amount", 0.0f);
            if (s.source >= 0 && s.dest >= 0 && s.source < numModSources && s.dest < numModDests)
                set (s.source, s.dest, s.amount);
        }
        ++revision;
    }

    int getRevision() const noexcept { return revision; }

private:
    std::array<ModSlot, maxModSlots> slots;
    int revision = 0;
};

} // namespace botanic
