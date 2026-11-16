#pragma once

#include <JuceHeader.h>

/**
    Parameter definitions for Botanic.

    All parameter IDs live here so the DSP, the UI and the preset system stay in sync.
*/
namespace Params
{
    enum class Fmt
    {
        Plain,
        Percent,
        Hertz,          // 20 Hz / 1.2 kHz
        Cents,          // +7 ct
        Semitones,      // 0 st
        Decibels,       // -1.5 dB
        Seconds,        // 0.42
        Milliseconds,   // 120 ms
        Ratio,          // 3.0:1
        Pan,            // C / 35% L
        BipolarPercent  // +35%
    };

    inline juce::String formatValue (Fmt fmt, float v, int decimals = 2)
    {
        switch (fmt)
        {
            case Fmt::Percent:      return juce::String (juce::roundToInt (v * 100.0f)) + "%";
            case Fmt::BipolarPercent:
            {
                const auto p = juce::roundToInt (v * 100.0f);
                return (p > 0 ? "+" : "") + juce::String (p) + "%";
            }
            case Fmt::Hertz:
                if (v < 10.0f)   return juce::String (v, 2) + " Hz";
                if (v < 1000.0f) return juce::String (juce::roundToInt (v)) + " Hz";
                return juce::String (v / 1000.0f, 1) + " kHz";
            case Fmt::Cents:        return (v >= 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
            case Fmt::Semitones:    return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " st";
            case Fmt::Decibels:     return juce::String (v, 1) + " dB";
            case Fmt::Seconds:      return juce::String (v, decimals);
            case Fmt::Milliseconds: return juce::String (juce::roundToInt (v)) + " ms";
            case Fmt::Ratio:        return juce::String (v, 1) + ":1";
            case Fmt::Pan:
            {
                if (std::abs (v) < 0.005f) return "C";
                return juce::String (juce::roundToInt (std::abs (v) * 100.0f)) + (v < 0 ? "% L" : "% R");
            }
            case Fmt::Plain:
            default: break;
        }

        return juce::String (v, decimals);
    }

    //==============================================================================
    // Identifiers - global
    inline constexpr auto power         = "power";
    inline constexpr auto master        = "master";
    inline constexpr auto output        = "output";
    inline constexpr auto a4            = "a4";
    inline constexpr auto polyMode      = "polyMode";
    inline constexpr auto maxVoices     = "maxVoices";
    inline constexpr auto glide         = "glide";
    inline constexpr auto bendRange     = "bendRange";
    inline constexpr auto ampVel        = "ampVel";

    // Filter
    inline constexpr auto filterType    = "fltType";
    inline constexpr auto cutoff        = "fltCutoff";
    inline constexpr auto resonance     = "fltReso";
    inline constexpr auto drive         = "fltDrive";
    inline constexpr auto filterEnv     = "fltEnv";
    inline constexpr auto keyTrack      = "fltKey";

    // Effects
    inline constexpr auto revMix        = "revMix";
    inline constexpr auto revSize       = "revSize";
    inline constexpr auto revDamp       = "revDamp";
    inline constexpr auto revWidth      = "revWidth";
    inline constexpr auto revOn         = "revOn";
    inline constexpr auto dlyMix        = "dlyMix";
    inline constexpr auto dlyTime       = "dlyTime";
    inline constexpr auto dlyFb         = "dlyFb";
    inline constexpr auto dlySync       = "dlySync";
    inline constexpr auto dlyDiv        = "dlyDiv";
    inline constexpr auto dlyOn         = "dlyOn";
    inline constexpr auto choMix        = "choMix";
    inline constexpr auto choRate       = "choRate";
    inline constexpr auto choDepth      = "choDepth";
    inline constexpr auto choOn         = "choOn";
    inline constexpr auto eqLow         = "eqLow";
    inline constexpr auto eqMid         = "eqMid";
    inline constexpr auto eqHigh        = "eqHigh";
    inline constexpr auto eqOn          = "eqOn";
    inline constexpr auto compThresh    = "cmpThresh";
    inline constexpr auto compRatio     = "cmpRatio";
    inline constexpr auto compMakeup    = "cmpMakeup";
    inline constexpr auto compOn        = "cmpOn";

    // Formant / vowel filter
    inline constexpr auto fmtOn        = "fmtOn";
    inline constexpr auto fmtVowel     = "fmtVowel";
    inline constexpr auto fmtShift     = "fmtShift";
    inline constexpr auto fmtQ         = "fmtQ";
    inline constexpr auto fmtMix       = "fmtMix";
    inline constexpr auto fmtKey       = "fmtKey";

    // Tone section (tilt / width / air)
    inline constexpr auto toneTilt     = "toneTilt";
    inline constexpr auto toneWidth    = "toneWidth";
    inline constexpr auto toneAir      = "toneAir";

    // Performance macros (XY pad)
    inline constexpr auto macX         = "macX";
    inline constexpr auto macY         = "macY";
    inline constexpr auto macDestX     = "macDestX";
    inline constexpr auto macDestY     = "macDestY";
    inline constexpr auto macMotion    = "macMotion";
    inline constexpr auto macRate      = "macRate";

    // Granular / glitch texture engine
    inline constexpr auto grOn          = "grOn";
    inline constexpr auto grFreeze      = "grFreeze";
    inline constexpr auto grMix         = "grMix";
    inline constexpr auto grSize        = "grSize";
    inline constexpr auto grDensity     = "grDensity";
    inline constexpr auto grPosition    = "grPosition";
    inline constexpr auto grSpray       = "grSpray";
    inline constexpr auto grPitch       = "grPitch";
    inline constexpr auto grPitchRandom = "grPitchRand";
    inline constexpr auto grReverse     = "grReverse";
    inline constexpr auto grSpread      = "grSpread";

    inline constexpr auto glOn          = "glOn";
    inline constexpr auto glMix         = "glMix";
    inline constexpr auto glBits        = "glBits";
    inline constexpr auto glRateDiv     = "glRateDiv";
    inline constexpr auto glStep        = "glStep";
    inline constexpr auto glStutterDiv  = "glStutterDiv";
    inline constexpr auto glStutter     = "glStutter";
    inline constexpr auto glGate        = "glGate";
    inline constexpr auto glDrop        = "glDrop";
    inline constexpr auto glTape        = "glTape";
    inline constexpr auto glRepeats     = "glRepeats";
    inline constexpr auto glChaos       = "glChaos";
    inline constexpr auto glNoise       = "glNoise";

    // Arpeggiator
    inline constexpr auto arpOn         = "arpOn";
    inline constexpr auto arpMode       = "arpMode";
    inline constexpr auto arpRate       = "arpRate";
    inline constexpr auto arpOctaves    = "arpOct";
    inline constexpr auto arpGate       = "arpGate";
    inline constexpr auto arpSwing      = "arpSwing";
    inline constexpr auto arpLatch      = "arpLatch";

    // Indexed helpers -----------------------------------------------------------
    inline juce::String oscId   (int i, const juce::String& field) { return "osc" + juce::String (i + 1) + "_" + field; }
    inline juce::String envId   (int i, const juce::String& field) { return i == 0 ? "amp" + field : "env" + juce::String (i) + field; }
    inline juce::String lfoId   (int i, const juce::String& field) { return "lfo" + juce::String (i + 1) + "_" + field; }

    inline juce::StringArray oscFields();
    inline juce::StringArray envFields();   // "A","D","S","R" (suffix)
    inline juce::StringArray lfoFields();

    //==============================================================================
    inline juce::StringArray wavetableNames()
    {
        return { "Wavetable", "Sine", "Triangle", "Saw", "Square", "Glass", "Formant", "Bells" };
    }

    inline juce::StringArray filterTypeNames()
    {
        return { "LP 24dB", "LP 12dB", "BP 12dB", "HP 12dB", "Notch" };
    }

    inline juce::StringArray lfoShapeNames()
    {
        return { "Triangle", "Sine", "Saw Up", "Saw Down", "Square", "S & H", "Random", "Chaos", "Pulse" };
    }

    inline juce::StringArray macroDestNames()
    {
        return { "Off", "Filter Cutoff", "Resonance", "Filter Drive", "Vowel Shift",
                 "Grain Mix", "Grain Position", "Glitch Mix", "Glitch Bits", "Reverb Mix", "Tilt" };
    }

    inline juce::StringArray glitchDivNames()
    {
        return { "1/2", "1/4", "1/8", "1/16", "1/16 T", "1/32", "1/64" };
    }

    inline juce::StringArray syncDivisionNames()
    {
        return { "1/1", "1/2", "1/4", "1/8", "1/8 T", "1/8 D", "1/16", "1/16 T", "1/16 D", "1/32", "1/64" };
    }

    inline juce::StringArray polyModeNames()
    {
        return { "Poly", "Mono", "Legato" };
    }

    inline juce::StringArray arpModeNames()
    {
        return { "Up", "Down", "Up-Down", "Random", "As Played", "Chord" };
    }

    inline juce::StringArray arpRateNames()
    {
        return { "1/4", "1/8", "1/8 T", "1/16", "1/16 T", "1/32" };
    }

    inline juce::StringArray fxSlotNames()
    {
        return { "Reverb", "Delay", "Chorus", "EQ", "Compressor" };
    }

    //==============================================================================
    /** Static descriptions used both for creating parameters and for the UI. */
    struct FloatSpec
    {
        juce::String id;
        juce::String name;
        juce::NormalisableRange<float> range;
        float def;
        Fmt fmt;
        int decimals = 2;
    };

    struct ChoiceSpec
    {
        juce::String id;
        juce::String name;
        juce::StringArray choices;
        int def;
    };

    inline juce::NormalisableRange<float> lin (float lo, float hi, float step = 0.0f)
    {
        return juce::NormalisableRange<float> (lo, hi, step);
    }

    inline juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }

    /** The full parameter table. */
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameters()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout p;

        auto addFloat = [&p] (const FloatSpec& s)
        {
            auto attrs = juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([f = s.fmt, d = s.decimals] (float v, int) { return formatValue (f, v, d); });
            p.add (std::make_unique<juce::AudioParameterFloat> (s.id, s.name, s.range, s.def, attrs));
        };

        auto addChoice = [&p] (const ChoiceSpec& s)
        {
            p.add (std::make_unique<juce::AudioParameterChoice> (s.id, s.name, s.choices, s.def));
        };

        auto addBool = [&p] (const char* id, const char* name, bool def)
        {
            p.add (std::make_unique<juce::AudioParameterBool> (id, name, def));
        };

        // ---- global
        addBool  (power, "Power", true);
        addFloat ({ master,    "Master",     lin (-24.0f, 6.0f),  -0.3f,  Fmt::Decibels, 1 });
        addFloat ({ output,    "Volume",     lin (-60.0f, 6.0f),  -1.5f,  Fmt::Decibels, 1 });
        addFloat ({ a4,        "Tuning A4",  lin (415.0f, 465.0f, 0.1f), 440.0f, Fmt::Hertz, 0 });
        addChoice ({ polyMode, "Voice Mode", polyModeNames(), 0 });
        addFloat ({ maxVoices, "Voices",     lin (1.0f, 32.0f, 1.0f), 16.0f, Fmt::Plain, 0 });
        addFloat ({ glide,     "Glide",      lin (0.0f, 1000.0f), 0.0f, Fmt::Milliseconds, 0 });
        addFloat ({ bendRange, "Bend Range", lin (0.0f, 24.0f, 1.0f), 2.0f, Fmt::Semitones, 0 });
        addFloat ({ ampVel,    "Vel > Amp",  lin (0.0f, 1.0f), 0.6f, Fmt::Percent });

        // ---- oscillators
        for (int i = 0; i < 3; ++i)
        {
            const auto n = juce::String (i + 1);
            addBool (oscId (i, "on").toRawUTF8(), ("OSC" + n + " On").toRawUTF8(), i == 0);
            addChoice ({ oscId (i, "table"), "OSC" + n + " Table", wavetableNames(), 0 });
            addFloat ({ oscId (i, "pos"),     "OSC" + n + " Position", lin (0.0f, 1.0f), 0.5f, Fmt::Percent });
            addFloat ({ oscId (i, "tune"),    "OSC" + n + " Tune",     lin (-36.0f, 36.0f, 1.0f), 0.0f, Fmt::Semitones, 0 });
            addFloat ({ oscId (i, "detune"),  "OSC" + n + " Detune",   lin (-50.0f, 50.0f), 0.0f, Fmt::Cents, 0 });
            addFloat ({ oscId (i, "level"),   "OSC" + n + " Level",    lin (0.0f, 1.0f), 1.0f, Fmt::Percent });
            addFloat ({ oscId (i, "pan"),     "OSC" + n + " Pan",      lin (-1.0f, 1.0f), 0.0f, Fmt::Pan });
            addFloat ({ oscId (i, "uni"),     "OSC" + n + " Unison",   lin (1.0f, 8.0f, 1.0f), 1.0f, Fmt::Plain, 0 });
            addFloat ({ oscId (i, "spread"),  "OSC" + n + " Spread",   lin (0.0f, 1.0f), 0.35f, Fmt::Percent });
            addFloat ({ oscId (i, "phase"),   "OSC" + n + " Phase",    lin (0.0f, 1.0f), 0.0f, Fmt::Plain, 2 });
        }

        // ---- filter
        addChoice ({ filterType, "Filter Type", filterTypeNames(), 0 });
        addFloat ({ cutoff,     "Cutoff",     logRange (20.0f, 20000.0f), 1200.0f, Fmt::Hertz });
        addFloat ({ resonance,  "Resonance",  lin (0.0f, 1.0f), 0.32f, Fmt::Percent });
        addFloat ({ drive,      "Drive",      lin (0.0f, 1.0f), 0.12f, Fmt::Percent });
        addFloat ({ filterEnv,  "Filter Env", lin (-1.0f, 1.0f), 0.25f, Fmt::BipolarPercent });
        addFloat ({ keyTrack,   "Key Track",  lin (0.0f, 1.0f), 0.0f, Fmt::Percent });

        // ---- envelopes: 0 = amp, 1 = env1, 2 = env2
        for (int e = 0; e < 3; ++e)
        {
            const auto prefix = e == 0 ? juce::String ("Amp") : ("Env" + juce::String (e));
            addFloat ({ envId (e, "A"), prefix + " Attack",  lin (0.001f, 8.0f), e == 0 ? 0.01f : 0.05f, Fmt::Seconds });
            addFloat ({ envId (e, "D"), prefix + " Decay",   lin (0.001f, 8.0f), 0.42f, Fmt::Seconds });
            addFloat ({ envId (e, "S"), prefix + " Sustain", lin (0.0f, 1.0f), 0.78f, Fmt::Seconds });
            addFloat ({ envId (e, "R"), prefix + " Release", lin (0.001f, 12.0f), 0.32f, Fmt::Seconds });
        }

        // ---- LFOs
        for (int i = 0; i < 2; ++i)
        {
            const auto n = juce::String (i + 1);
            addFloat ({ lfoId (i, "rate"),   "LFO" + n + " Rate",  logRange (0.01f, 40.0f), 1.0f, Fmt::Hertz });
            addFloat ({ lfoId (i, "depth"),  "LFO" + n + " Depth", lin (0.0f, 1.0f), 0.5f, Fmt::Percent });
            addFloat ({ lfoId (i, "phase"),  "LFO" + n + " Phase", lin (0.0f, 1.0f), 0.0f, Fmt::Percent });
            addBool  (lfoId (i, "sync").toRawUTF8(),  ("LFO" + n + " Sync").toRawUTF8(), false);
            addChoice ({ lfoId (i, "shape"), "LFO" + n + " Shape", lfoShapeNames(), 0 });
            addChoice ({ lfoId (i, "div"),   "LFO" + n + " Div",   syncDivisionNames(), 2 });
            addBool  (lfoId (i, "retrig").toRawUTF8(), ("LFO" + n + " Retrigger").toRawUTF8(), true);
        }

        // ---- effects
        addBool (revOn, "Reverb On", true);
        addFloat ({ revMix,   "Reverb Mix",   lin (0.0f, 1.0f), 0.32f, Fmt::Percent });
        addFloat ({ revSize,  "Reverb Size",  lin (0.0f, 1.0f), 0.6f, Fmt::Percent });
        addFloat ({ revDamp,  "Reverb Damp",  lin (0.0f, 1.0f), 0.4f, Fmt::Percent });
        addFloat ({ revWidth, "Reverb Width", lin (0.0f, 1.0f), 0.85f, Fmt::Percent });

        addBool (dlyOn, "Delay On", true);
        addFloat ({ dlyMix,  "Delay Mix",      lin (0.0f, 1.0f), 0.25f, Fmt::Percent });
        addFloat ({ dlyTime, "Delay Time",     lin (10.0f, 2000.0f), 330.0f, Fmt::Milliseconds });
        addFloat ({ dlyFb,   "Delay Feedback", lin (0.0f, 0.95f), 0.35f, Fmt::Percent });
        addBool  (dlySync, "Delay Sync", true);
        addChoice ({ dlyDiv, "Delay Division", syncDivisionNames(), 3 });

        addBool (choOn, "Chorus On", true);
        addFloat ({ choMix,   "Chorus Mix",   lin (0.0f, 1.0f), 0.35f, Fmt::Percent });
        addFloat ({ choRate,  "Chorus Rate",  logRange (0.05f, 8.0f), 0.8f, Fmt::Hertz });
        addFloat ({ choDepth, "Chorus Depth", lin (0.0f, 1.0f), 0.5f, Fmt::Percent });

        addBool (eqOn, "EQ On", false);
        addFloat ({ eqLow,  "EQ Low",  lin (-12.0f, 12.0f), 0.0f, Fmt::Decibels, 1 });
        addFloat ({ eqMid,  "EQ Mid",  lin (-12.0f, 12.0f), 0.0f, Fmt::Decibels, 1 });
        addFloat ({ eqHigh, "EQ High", lin (-12.0f, 12.0f), 0.0f, Fmt::Decibels, 1 });

        addBool (compOn, "Compressor On", false);
        addFloat ({ compThresh, "Comp Threshold", lin (-60.0f, 0.0f), -18.0f, Fmt::Decibels, 0 });
        addFloat ({ compRatio,  "Comp Ratio",     lin (1.0f, 20.0f), 3.0f, Fmt::Ratio, 1 });
        addFloat ({ compMakeup, "Comp Makeup",    lin (0.0f, 12.0f), 0.0f, Fmt::Decibels, 1 });

        // ---- formant / vowel filter
        addBool  (fmtOn, "Formant On", false);
        addFloat ({ fmtVowel, "Vowel",        lin (0.0f, 1.0f), 0.15f, Fmt::Percent });
        addFloat ({ fmtShift, "Vowel Shift",  lin (-1200.0f, 1200.0f), 0.0f, Fmt::Cents, 0 });
        addFloat ({ fmtQ,     "Vowel Q",      lin (0.0f, 1.0f), 0.55f, Fmt::Percent });
        addFloat ({ fmtMix,   "Vowel Mix",    lin (0.0f, 1.0f), 0.65f, Fmt::Percent });
        addFloat ({ fmtKey,   "Vowel Key",    lin (0.0f, 1.0f), 0.35f, Fmt::Percent });

        // ---- tone section
        addFloat ({ toneTilt,  "Tilt",  lin (-12.0f, 12.0f), 0.0f, Fmt::Decibels, 1 });
        addFloat ({ toneWidth, "Width", lin (0.0f, 2.0f), 1.0f, Fmt::Percent });
        addFloat ({ toneAir,   "Air",   lin (0.0f, 1.0f), 0.0f, Fmt::Percent });

        // ---- performance macros
        addFloat ({ macX, "Macro X", lin (0.0f, 1.0f), 0.5f, Fmt::Percent });
        addFloat ({ macY, "Macro Y", lin (0.0f, 1.0f), 0.5f, Fmt::Percent });
        addChoice ({ macDestX, "Macro X Target", macroDestNames(), 1 });
        addChoice ({ macDestY, "Macro Y Target", macroDestNames(), 6 });
        addBool  (macMotion, "Macro Motion", false);
        addFloat ({ macRate, "Motion Rate", logRange (0.02f, 4.0f), 0.15f, Fmt::Hertz });

        // ---- granular texture
        addBool  (grOn, "Grain On", false);
        addBool  (grFreeze, "Grain Freeze", false);
        addFloat ({ grMix,      "Grain Mix",       lin (0.0f, 1.0f), 0.55f, Fmt::Percent });
        addFloat ({ grSize,     "Grain Size",      logRange (5.0f, 500.0f), 120.0f, Fmt::Milliseconds, 0 });
        addFloat ({ grDensity,  "Grain Density",   logRange (1.0f, 80.0f), 16.0f, Fmt::Plain, 1 });
        addFloat ({ grPosition, "Grain Position",  lin (0.0f, 1.0f), 0.35f, Fmt::Percent });
        addFloat ({ grSpray,    "Grain Spray",     lin (0.0f, 1.0f), 0.30f, Fmt::Percent });
        addFloat ({ grPitch,    "Grain Pitch",     lin (-24.0f, 24.0f, 1.0f), 0.0f, Fmt::Semitones, 0 });
        addFloat ({ grPitchRandom, "Grain Pitch Random", lin (0.0f, 12.0f), 0.0f, Fmt::Semitones, 1 });
        addFloat ({ grReverse,  "Grain Reverse",   lin (0.0f, 1.0f), 0.25f, Fmt::Percent });
        addFloat ({ grSpread,   "Grain Spread",    lin (0.0f, 1.0f), 0.60f, Fmt::Percent });

        // ---- glitch
        addBool  (glOn, "Glitch On", false);
        addFloat ({ glMix,      "Glitch Mix",      lin (0.0f, 1.0f), 0.65f, Fmt::Percent });
        addFloat ({ glBits,     "Glitch Bits",     lin (2.0f, 16.0f, 1.0f), 12.0f, Fmt::Plain, 0 });
        addFloat ({ glRateDiv,  "Glitch Rate",     lin (1.0f, 48.0f, 1.0f), 1.0f, Fmt::Plain, 0 });
        addChoice ({ glStep,     "Glitch Step",    glitchDivNames(), 3 });
        addChoice ({ glStutterDiv, "Stutter Size", glitchDivNames(), 3 });
        addFloat ({ glStutter,  "Stutter Chance",  lin (0.0f, 1.0f), 0.25f, Fmt::Percent });
        addFloat ({ glGate,     "Gate Chance",     lin (0.0f, 1.0f), 0.10f, Fmt::Percent });
        addFloat ({ glDrop,     "Dropout Chance",  lin (0.0f, 1.0f), 0.08f, Fmt::Percent });
        addFloat ({ glTape,     "Tape Chance",     lin (0.0f, 1.0f), 0.06f, Fmt::Percent });
        addFloat ({ glRepeats,  "Stutter Repeats", lin (1.0f, 12.0f, 1.0f), 3.0f, Fmt::Plain, 0 });
        addFloat ({ glChaos,    "Chaos",           lin (0.0f, 1.0f), 0.50f, Fmt::Percent });
        addFloat ({ glNoise,    "Digital Noise",   lin (0.0f, 1.0f), 0.10f, Fmt::Percent });

        // ---- arpeggiator
        addBool (arpOn, "Arp On", false);
        addChoice ({ arpMode, "Arp Mode", arpModeNames(), 0 });
        addChoice ({ arpRate, "Arp Rate", arpRateNames(), 3 });
        addFloat ({ arpOctaves, "Arp Octaves", lin (1.0f, 4.0f, 1.0f), 1.0f, Fmt::Plain, 0 });
        addFloat ({ arpGate,    "Arp Gate",    lin (0.05f, 1.0f), 0.5f, Fmt::Percent });
        addFloat ({ arpSwing,   "Arp Swing",   lin (0.0f, 0.6f), 0.0f, Fmt::Percent });
        addBool (arpLatch, "Arp Latch", false);

        return p;
    }
}
