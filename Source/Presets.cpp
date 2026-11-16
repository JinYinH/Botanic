#include "Presets.h"

namespace botanic
{

namespace
{
    struct PresetDef
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
        struct Mod { int source; int dest; float amount; };
        std::vector<Mod> mods;
    };

    // helper aliases to keep the tables readable
    constexpr int LFO1 = (int) ModSource::lfo1;
    constexpr int LFO2 = (int) ModSource::lfo2;
    constexpr int ENV1 = (int) ModSource::env1;
    constexpr int ENV2 = (int) ModSource::env2;
    constexpr int VEL  = (int) ModSource::velocity;
    constexpr int MW   = (int) ModSource::modWheel;

    constexpr int D_CUT   = (int) ModDest::cutoff;
    constexpr int D_RES   = (int) ModDest::resonance;
    constexpr int D_PITCH = (int) ModDest::pitch;
    constexpr int D_L1    = (int) ModDest::osc1Level;
    constexpr int D_L2    = (int) ModDest::osc2Level;
    constexpr int D_P1    = (int) ModDest::osc1Pos;
    constexpr int D_P2    = (int) ModDest::osc2Pos;
    constexpr int D_DET   = (int) ModDest::osc1Detune;
    constexpr int D_PAN   = (int) ModDest::pan;
    constexpr int D_REV   = (int) ModDest::reverbMix;

    const std::vector<PresetDef>& presetTable()
    {
        static const std::vector<PresetDef> presets = {
            { "Velvet Keys",
              {
                  { "osc1_on", 1 }, { "osc1_table", 1 }, { "osc1_pos", 0.35f }, { "osc1_level", 0.85f },
                  { "osc1_uni", 2 }, { "osc1_spread", 0.4f }, { "osc1_detune", 5.0f },
                  { "osc2_on", 1 }, { "osc2_table", 7 }, { "osc2_pos", 0.45f }, { "osc2_tune", 12.0f },
                  { "osc2_level", 0.32f }, { "osc2_pan", 0.12f },
                  { "fltType", 0 }, { "fltCutoff", 3200.0f }, { "fltReso", 0.10f }, { "fltDrive", 0.06f },
                  { "fltEnv", 0.28f }, { "fltKey", 0.45f },
                  { "ampA", 0.006f }, { "ampD", 1.5f }, { "ampS", 0.32f }, { "ampR", 0.55f }, { "ampVel", 0.55f },
                  { "env1A", 0.002f }, { "env1D", 0.9f }, { "env1S", 0.12f }, { "env1R", 0.5f },
                  { "choOn", 1 }, { "choMix", 0.26f }, { "choRate", 0.55f }, { "choDepth", 0.45f },
                  { "revOn", 1 }, { "revMix", 0.22f }, { "revSize", 0.45f }, { "revDamp", 0.55f },
                  { "dlyOn", 1 }, { "dlyMix", 0.10f }, { "dlyDiv", 4 }, { "dlyFb", 0.22f },
                  { "toneTilt", 1.5f }, { "toneWidth", 1.1f },
                  { "maxVoices", 24.0f }, { "master", -1.5f }
              },
              { { VEL, D_CUT, 0.35f }, { MW, D_REV, 0.2f } } },

            { "Soft Pluck",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.42f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 2 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.28f },
                  { "fltType", 0 }, { "fltCutoff", 950.0f }, { "fltReso", 0.24f }, { "fltDrive", 0.1f },
                  { "fltEnv", 0.55f }, { "fltKey", 0.3f },
                  { "ampA", 0.002f }, { "ampD", 0.5f }, { "ampS", 0.0f }, { "ampR", 0.32f }, { "ampVel", 0.6f },
                  { "env1A", 0.001f }, { "env1D", 0.32f }, { "env1S", 0.0f }, { "env1R", 0.25f },
                  { "dlyOn", 1 }, { "dlyMix", 0.26f }, { "dlyDiv", 4 }, { "dlyFb", 0.3f },
                  { "revOn", 1 }, { "revMix", 0.24f }, { "revSize", 0.5f }, { "revDamp", 0.55f },
                  { "choOn", 1 }, { "choMix", 0.16f },
                  { "toneAir", 0.15f },
                  { "master", -2.0f }
              },
              { { VEL, D_CUT, 0.4f } } },

            { "Warm Bass",
              {
                  { "osc1_on", 1 }, { "osc1_table", 1 }, { "osc1_pos", 0.2f }, { "osc1_level", 1.0f },
                  { "osc2_on", 1 }, { "osc2_table", 3 }, { "osc2_level", 0.28f }, { "osc2_tune", -12.0f },
                  { "fltType", 0 }, { "fltCutoff", 430.0f }, { "fltReso", 0.18f }, { "fltDrive", 0.22f },
                  { "fltEnv", 0.35f }, { "fltKey", 0.2f },
                  { "ampA", 0.004f }, { "ampD", 0.5f }, { "ampS", 0.78f }, { "ampR", 0.16f }, { "ampVel", 0.5f },
                  { "env1A", 0.001f }, { "env1D", 0.35f }, { "env1S", 0.2f }, { "env1R", 0.2f },
                  { "revOn", 1 }, { "revMix", 0.05f }, { "revSize", 0.35f },
                  { "dlyOn", 0 }, { "choOn", 0 },
                  { "toneTilt", -1.0f },
                  { "master", -2.5f }
              },
              { { VEL, D_CUT, 0.4f } } },

            { "Morning Pad",
              {
                  { "osc1_on", 1 }, { "osc1_table", 0 }, { "osc1_pos", 0.45f }, { "osc1_level", 0.8f },
                  { "osc1_uni", 4 }, { "osc1_spread", 0.55f }, { "osc1_detune", 8.0f },
                  { "osc2_on", 1 }, { "osc2_table", 2 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.38f },
                  { "osc2_pan", -0.15f },
                  { "fltType", 1 }, { "fltCutoff", 2400.0f }, { "fltReso", 0.12f }, { "fltDrive", 0.05f },
                  { "fltEnv", 0.2f }, { "fltKey", 0.3f },
                  { "ampA", 0.9f }, { "ampD", 1.8f }, { "ampS", 0.75f }, { "ampR", 2.2f }, { "ampVel", 0.3f },
                  { "env1A", 1.2f }, { "env1D", 2.0f }, { "env1S", 0.5f }, { "env1R", 2.0f },
                  { "lfo1_rate", 0.12f }, { "lfo1_depth", 0.4f }, { "lfo1_shape", 0 },
                  { "revOn", 1 }, { "revMix", 0.3f }, { "revSize", 0.62f }, { "revDamp", 0.45f },
                  { "dlyOn", 0 },
                  { "choOn", 1 }, { "choMix", 0.32f }, { "choRate", 0.35f },
                  { "toneTilt", 1.0f }, { "toneWidth", 1.15f },
                  { "master", -3.0f }
              },
              { { LFO1, D_CUT, 0.35f } } },

            { "Glass Bell",
              {
                  { "osc1_on", 1 }, { "osc1_table", 7 }, { "osc1_pos", 0.55f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.25f },
                  { "fltType", 0 }, { "fltCutoff", 5200.0f }, { "fltReso", 0.08f }, { "fltEnv", 0.3f }, { "fltKey", 0.5f },
                  { "ampA", 0.002f }, { "ampD", 1.7f }, { "ampS", 0.0f }, { "ampR", 1.2f }, { "ampVel", 0.6f },
                  { "env1A", 0.001f }, { "env1D", 0.8f }, { "env1S", 0.0f }, { "env1R", 0.8f },
                  { "revOn", 1 }, { "revMix", 0.3f }, { "revSize", 0.55f }, { "revDamp", 0.4f },
                  { "dlyOn", 1 }, { "dlyMix", 0.14f }, { "dlyDiv", 5 }, { "dlyFb", 0.25f },
                  { "choOn", 0 },
                  { "master", -2.0f }
              },
              { { VEL, D_CUT, 0.3f } } },

            { "Analog Lead",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.3f }, { "osc1_level", 0.9f },
                  { "osc1_uni", 2 }, { "osc1_detune", 6.0f }, { "osc1_spread", 0.3f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_level", 0.3f }, { "osc2_tune", -12.0f },
                  { "fltType", 0 }, { "fltCutoff", 2800.0f }, { "fltReso", 0.28f }, { "fltDrive", 0.18f },
                  { "fltEnv", 0.3f }, { "fltKey", 0.4f },
                  { "ampA", 0.01f }, { "ampD", 0.6f }, { "ampS", 0.82f }, { "ampR", 0.4f },
                  { "glide", 60.0f }, { "bendRange", 2.0f },
                  { "env1A", 0.01f }, { "env1D", 0.45f }, { "env1S", 0.6f }, { "env1R", 0.35f },
                  { "lfo1_rate", 5.2f }, { "lfo1_depth", 0.12f }, { "lfo1_shape", 0 },
                  { "dlyOn", 1 }, { "dlyMix", 0.2f }, { "dlyDiv", 4 }, { "dlyFb", 0.35f },
                  { "revOn", 1 }, { "revMix", 0.2f }, { "revSize", 0.5f },
                  { "choOn", 1 }, { "choMix", 0.2f },
                  { "master", -3.0f }
              },
              { { LFO1, D_PITCH, 0.04f }, { MW, D_CUT, 0.4f } } },

            { "River Arp",
              {
                  { "osc1_on", 1 }, { "osc1_table", 2 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.8f },
                  { "osc1_uni", 2 }, { "osc1_spread", 0.4f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.3f },
                  { "fltType", 0 }, { "fltCutoff", 1900.0f }, { "fltReso", 0.2f }, { "fltEnv", 0.45f }, { "fltKey", 0.3f },
                  { "ampA", 0.002f }, { "ampD", 0.38f }, { "ampS", 0.05f }, { "ampR", 0.3f },
                  { "env1A", 0.001f }, { "env1D", 0.3f }, { "env1S", 0.0f }, { "env1R", 0.25f },
                  { "arpOn", 1 }, { "arpMode", 0 }, { "arpRate", 3 }, { "arpOctaves", 2.0f },
                  { "arpGate", 0.45f }, { "arpSwing", 0.0f },
                  { "dlyOn", 1 }, { "dlyMix", 0.28f }, { "dlyDiv", 4 }, { "dlyFb", 0.34f },
                  { "revOn", 1 }, { "revMix", 0.26f }, { "revSize", 0.52f },
                  { "choOn", 1 }, { "choMix", 0.2f },
                  { "master", -1.0f }
              },
              { { VEL, D_CUT, 0.3f } } },

            { "Air Choir",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.7f },
                  { "osc1_uni", 3 }, { "osc1_spread", 0.5f }, { "osc1_detune", 7.0f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.35f },
                  { "fltType", 1 }, { "fltCutoff", 2200.0f }, { "fltReso", 0.14f }, { "fltEnv", 0.15f }, { "fltKey", 0.3f },
                  { "ampA", 0.8f }, { "ampD", 1.5f }, { "ampS", 0.8f }, { "ampR", 2.4f }, { "ampVel", 0.3f },
                  { "env1A", 1.0f }, { "env1D", 1.8f }, { "env1S", 0.6f }, { "env1R", 1.8f },
                  { "fmtOn", 1 }, { "fmtVowel", 0.55f }, { "fmtShift", 0.0f }, { "fmtQ", 0.5f },
                  { "fmtMix", 0.45f }, { "fmtKey", 0.4f },
                  { "lfo2_rate", 0.09f }, { "lfo2_depth", 0.3f }, { "lfo2_shape", 0 },
                  { "revOn", 1 }, { "revMix", 0.34f }, { "revSize", 0.66f }, { "revDamp", 0.42f },
                  { "dlyOn", 0 },
                  { "choOn", 1 }, { "choMix", 0.3f }, { "choRate", 0.3f },
                  { "master", -4.0f }
              },
              { { LFO2, (int) ModDest::formantShift, 0.35f } } },

            { "Lo-Fi Tape",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.5f }, { "osc1_level", 0.85f },
                  { "osc1_uni", 2 }, { "osc1_detune", 10.0f }, { "osc1_spread", 0.45f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_level", 0.3f }, { "osc2_tune", -12.0f },
                  { "fltType", 1 }, { "fltCutoff", 1500.0f }, { "fltReso", 0.2f }, { "fltDrive", 0.2f },
                  { "fltEnv", 0.2f }, { "fltKey", 0.25f },
                  { "ampA", 0.01f }, { "ampD", 0.9f }, { "ampS", 0.6f }, { "ampR", 0.5f },
                  { "env1A", 0.01f }, { "env1D", 0.6f }, { "env1S", 0.4f }, { "env1R", 0.4f },
                  { "choOn", 1 }, { "choMix", 0.25f }, { "choRate", 0.25f }, { "choDepth", 0.6f },
                  { "dlyOn", 1 }, { "dlyMix", 0.14f }, { "dlyDiv", 4 }, { "dlyFb", 0.25f },
                  { "revOn", 1 }, { "revMix", 0.2f }, { "revSize", 0.45f },
                  { "toneTilt", -2.0f }, { "toneWidth", 0.9f }, { "toneAir", 0.05f },
                  { "lfo1_rate", 0.9f }, { "lfo1_depth", 0.06f }, { "lfo1_shape", 0 },
                  { "master", -2.5f }
              },
              { { LFO1, D_PITCH, 0.02f } } },

            { "Bright Stab",
              {
                  { "osc1_on", 1 }, { "osc1_table", 4 }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 3 }, { "osc2_level", 0.6f }, { "osc2_detune", 8.0f },
                  { "fltType", 0 }, { "fltCutoff", 3800.0f }, { "fltReso", 0.25f }, { "fltDrive", 0.15f },
                  { "fltEnv", 0.5f }, { "fltKey", 0.3f },
                  { "ampA", 0.002f }, { "ampD", 0.5f }, { "ampS", 0.15f }, { "ampR", 0.3f },
                  { "env1A", 0.001f }, { "env1D", 0.35f }, { "env1S", 0.0f }, { "env1R", 0.25f },
                  { "choOn", 1 }, { "choMix", 0.18f },
                  { "dlyOn", 1 }, { "dlyMix", 0.18f }, { "dlyDiv", 4 }, { "dlyFb", 0.28f },
                  { "revOn", 1 }, { "revMix", 0.22f }, { "revSize", 0.5f },
                  { "toneTilt", 1.0f },
                  { "master", -2.0f }
              },
              { { VEL, D_CUT, 0.35f } } },

            { "Lush Atmosphere",
              {
                  { "osc1_on", 1 }, { "osc1_table", 0 }, { "osc1_pos", 0.62f }, { "osc1_level", 0.85f },
                  { "osc1_uni", 5 }, { "osc1_spread", 0.65f }, { "osc1_detune", 12.0f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_pos", 0.35f }, { "osc2_tune", -12.0f },
                  { "osc2_level", 0.55f }, { "osc2_pan", -0.25f }, { "osc2_uni", 3 },
                  { "osc3_on", 1 }, { "osc3_table", 5 }, { "osc3_level", 0.22f }, { "osc3_tune", 12.0f },
                  { "osc3_pan", 0.3f }, { "osc3_pos", 0.8f },
                  { "fltType", 0 }, { "fltCutoff", 780.0f }, { "fltReso", 0.18f }, { "fltDrive", 0.08f },
                  { "fltEnv", 0.34f }, { "fltKey", 0.25f },
                  { "ampA", 0.65f }, { "ampD", 1.4f }, { "ampS", 0.82f }, { "ampR", 2.6f }, { "ampVel", 0.35f },
                  { "env1A", 0.9f }, { "env1D", 1.2f }, { "env1S", 0.55f }, { "env1R", 1.8f },
                  { "lfo1_rate", 0.18f }, { "lfo1_depth", 0.62f }, { "lfo1_shape", 1 },
                  { "lfo2_rate", 0.32f }, { "lfo2_depth", 0.3f }, { "lfo2_shape", 0 },
                  { "revMix", 0.52f }, { "revSize", 0.86f }, { "revDamp", 0.35f }, { "revWidth", 1.0f },
                  { "dlyMix", 0.24f }, { "dlyFb", 0.42f }, { "dlyDiv", 4 }, { "dlySync", 1 },
                  { "choMix", 0.4f }, { "choRate", 0.45f },
                  { "maxVoices", 24.0f }, { "glide", 0.0f }, { "master", -2.0f }
              },
              { { LFO1, D_CUT, 0.55f }, { ENV2, D_P2, 0.35f }, { MW, D_REV, 0.3f } } },

            { "Glass Bloom",
              {
                  { "osc1_on", 1 }, { "osc1_table", 5 }, { "osc1_pos", 0.3f }, { "osc1_level", 0.9f },
                  { "osc1_uni", 3 }, { "osc1_spread", 0.4f },
                  { "osc2_on", 1 }, { "osc2_table", 7 }, { "osc2_level", 0.45f }, { "osc2_tune", 12.0f },
                  { "osc2_detune", 6.0f },
                  { "fltType", 0 }, { "fltCutoff", 2400.0f }, { "fltReso", 0.3f }, { "fltEnv", 0.55f }, { "fltKey", 0.5f },
                  { "ampA", 0.004f }, { "ampD", 1.1f }, { "ampS", 0.0f }, { "ampR", 1.4f },
                  { "env1A", 0.002f }, { "env1D", 0.55f }, { "env1S", 0.0f }, { "env1R", 0.8f },
                  { "lfo1_rate", 0.6f }, { "lfo1_depth", 0.3f },
                  { "revMix", 0.44f }, { "revSize", 0.72f }, { "revDamp", 0.25f },
                  { "dlyMix", 0.3f }, { "dlyDiv", 3 }, { "dlyFb", 0.38f },
                  { "choMix", 0.22f }, { "master", -2.5f }
              },
              { { LFO1, D_P1, 0.22f }, { VEL, D_CUT, 0.3f }, { ENV2, D_RES, 0.15f } } },

            { "Pollen Pluck",
              {
                  { "osc1_on", 1 }, { "osc1_table", 4 }, { "osc1_pos", 0.35f }, { "osc1_level", 0.95f },
                  { "osc1_uni", 2 }, { "osc1_detune", 8.0f },
                  { "osc2_on", 1 }, { "osc2_table", 2 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.35f },
                  { "fltType", 1 }, { "fltCutoff", 1500.0f }, { "fltReso", 0.36f }, { "fltEnv", 0.62f }, { "fltKey", 0.35f },
                  { "ampA", 0.002f }, { "ampD", 0.42f }, { "ampS", 0.0f }, { "ampR", 0.32f },
                  { "env1A", 0.001f }, { "env1D", 0.28f }, { "env1S", 0.0f }, { "env1R", 0.25f },
                  { "revMix", 0.28f }, { "revSize", 0.5f },
                  { "dlyMix", 0.28f }, { "dlyDiv", 3 }, { "dlyFb", 0.3f },
                  { "master", -2.0f }
              },
              { { VEL, D_CUT, 0.45f }, { ENV1, D_P1, 0.2f } } },

            { "Moss Pad",
              {
                  { "osc1_on", 1 }, { "osc1_table", 2 }, { "osc1_pos", 0.2f }, { "osc1_level", 0.8f },
                  { "osc1_uni", 6 }, { "osc1_spread", 0.8f }, { "osc1_detune", 16.0f },
                  { "osc2_on", 1 }, { "osc2_table", 6 }, { "osc2_pos", 0.55f }, { "osc2_level", 0.5f },
                  { "osc2_tune", -12.0f }, { "osc2_uni", 2 },
                  { "fltType", 0 }, { "fltCutoff", 520.0f }, { "fltReso", 0.22f }, { "fltEnv", 0.25f }, { "fltKey", 0.4f },
                  { "ampA", 1.2f }, { "ampD", 2.0f }, { "ampS", 0.9f }, { "ampR", 3.5f },
                  { "env1A", 1.6f }, { "env1D", 2.0f }, { "env1S", 0.4f }, { "env1R", 2.5f },
                  { "lfo1_rate", 0.12f }, { "lfo1_depth", 0.5f }, { "lfo1_shape", 1 },
                  { "revMix", 0.6f }, { "revSize", 0.95f }, { "revDamp", 0.5f }, { "choMix", 0.5f },
                  { "maxVoices", 16.0f }, { "master", -3.0f }
              },
              { { LFO1, D_CUT, 0.4f }, { LFO1, D_PAN, 0.25f }, { ENV2, D_L2, 0.3f } } },

            { "Rain Keys",
              {
                  { "osc1_on", 1 }, { "osc1_table", 1 }, { "osc1_pos", 0.55f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_level", 0.3f }, { "osc2_pan", -0.4f },
                  { "osc3_on", 1 }, { "osc3_table", 7 }, { "osc3_level", 0.25f }, { "osc3_pan", 0.4f }, { "osc3_tune", 7.0f },
                  { "fltType", 1 }, { "fltCutoff", 3200.0f }, { "fltReso", 0.2f }, { "fltEnv", 0.3f }, { "fltKey", 0.6f },
                  { "ampA", 0.01f }, { "ampD", 0.9f }, { "ampS", 0.25f }, { "ampR", 0.7f },
                  { "env1A", 0.005f }, { "env1D", 0.6f }, { "env1S", 0.1f }, { "env1R", 0.6f },
                  { "lfo1_rate", 4.5f }, { "lfo1_depth", 0.15f }, { "lfo1_shape", 1 },
                  { "revMix", 0.36f }, { "revSize", 0.62f }, { "dlyMix", 0.32f }, { "dlyDiv", 5 },
                  { "master", -2.6f }
              },
              { { LFO1, D_CUT, 0.18f }, { VEL, D_L2, 0.3f } } },

            { "Fern Bass",
              {
                  { "polyMode", 1 }, { "glide", 60.0f }, { "osc1_on", 1 }, { "osc1_table", 3 },
                  { "osc1_pos", 0.25f }, { "osc1_level", 1.0f }, { "osc1_uni", 2 }, { "osc1_spread", 0.2f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.6f }, { "osc2_pos", 0.1f },
                  { "fltType", 0 }, { "fltCutoff", 380.0f }, { "fltReso", 0.26f }, { "fltDrive", 0.35f },
                  { "fltEnv", 0.5f }, { "fltKey", 0.15f },
                  { "ampA", 0.004f }, { "ampD", 0.55f }, { "ampS", 0.55f }, { "ampR", 0.25f },
                  { "env1A", 0.002f }, { "env1D", 0.35f }, { "env1S", 0.15f }, { "env1R", 0.3f },
                  { "revMix", 0.08f }, { "dlyMix", 0.0f }, { "revOn", 1 }, { "choMix", 0.1f },
                  { "master", -1.5f }
              },
              { { VEL, D_CUT, 0.5f } } },

            { "Nectar Lead",
              {
                  { "polyMode", 2 }, { "glide", 35.0f }, { "bendRange", 12.0f },
                  { "osc1_on", 1 }, { "osc1_table", 4 }, { "osc1_pos", 0.55f }, { "osc1_uni", 4 },
                  { "osc1_detune", 18.0f }, { "osc1_spread", 0.5f },
                  { "osc2_on", 1 }, { "osc2_table", 2 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.4f },
                  { "fltType", 0 }, { "fltCutoff", 2200.0f }, { "fltReso", 0.4f }, { "fltDrive", 0.3f },
                  { "fltEnv", 0.4f }, { "fltKey", 0.5f },
                  { "ampA", 0.01f }, { "ampD", 0.6f }, { "ampS", 0.75f }, { "ampR", 0.4f },
                  { "env1A", 0.01f }, { "env1D", 0.5f }, { "env1S", 0.3f }, { "env1R", 0.4f },
                  { "lfo1_rate", 5.2f }, { "lfo1_depth", 0.0f },
                  { "revMix", 0.22f }, { "dlyMix", 0.26f }, { "dlyDiv", 4 }, { "choMix", 0.28f },
                  { "master", -2.2f }
              },
              { { LFO1, D_PITCH, 0.03f }, { MW, D_CUT, 0.35f } } },

            { "Crystal Bells",
              {
                  { "osc1_on", 1 }, { "osc1_table", 7 }, { "osc1_pos", 0.15f }, { "osc1_level", 0.9f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_pos", 0.7f }, { "osc2_level", 0.5f }, { "osc2_tune", 19.0f },
                  { "fltType", 1 }, { "fltCutoff", 5200.0f }, { "fltReso", 0.15f }, { "fltEnv", 0.25f }, { "fltKey", 0.7f },
                  { "ampA", 0.002f }, { "ampD", 2.2f }, { "ampS", 0.0f }, { "ampR", 2.6f },
                  { "env1A", 0.002f }, { "env1D", 1.1f }, { "env1S", 0.0f }, { "env1R", 1.2f },
                  { "revMix", 0.5f }, { "revSize", 0.8f }, { "dlyMix", 0.22f }, { "dlyDiv", 6 },
                  { "master", -3.0f }
              },
              { { ENV1, D_P1, 0.3f }, { LFO2, D_P2, 0.2f } } },

            { "Roots And Soil",
              {
                  { "osc1_on", 1 }, { "osc1_table", 6 }, { "osc1_pos", 0.15f }, { "osc1_level", 0.7f },
                  { "osc1_tune", -12.0f }, { "osc1_uni", 3 },
                  { "osc2_on", 1 }, { "osc2_table", 3 }, { "osc2_tune", -24.0f }, { "osc2_level", 0.55f },
                  { "fltType", 0 }, { "fltCutoff", 300.0f }, { "fltReso", 0.45f }, { "fltDrive", 0.5f }, { "fltEnv", 0.15f },
                  { "ampA", 1.8f }, { "ampD", 3.0f }, { "ampS", 0.95f }, { "ampR", 4.0f },
                  { "env1A", 2.5f }, { "env1D", 3.0f }, { "env1S", 0.6f }, { "env1R", 3.0f },
                  { "lfo1_rate", 0.07f }, { "lfo1_depth", 0.7f }, { "lfo1_shape", 0 },
                  { "lfo2_rate", 0.15f }, { "lfo2_depth", 0.4f }, { "lfo2_shape", 5 },
                  { "revMix", 0.55f }, { "revSize", 1.0f }, { "choMix", 0.55f }, { "choRate", 0.25f },
                  { "master", -4.0f }
              },
              { { LFO1, D_CUT, 0.45f }, { LFO2, D_PAN, 0.35f }, { ENV2, D_P1, 0.25f } } },

            { "Wind Chimes",
              {
                  { "arpOn", 1 }, { "arpMode", 2 }, { "arpRate", 3 }, { "arpOct", 2.0f }, { "arpGate", 0.45f },
                  { "osc1_on", 1 }, { "osc1_table", 7 }, { "osc1_pos", 0.45f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_level", 0.35f }, { "osc2_tune", 12.0f },
                  { "fltType", 1 }, { "fltCutoff", 4200.0f }, { "fltReso", 0.2f }, { "fltEnv", 0.4f }, { "fltKey", 0.6f },
                  { "ampA", 0.002f }, { "ampD", 0.85f }, { "ampS", 0.05f }, { "ampR", 0.9f },
                  { "env1A", 0.001f }, { "env1D", 0.4f }, { "env1S", 0.0f }, { "env1R", 0.5f },
                  { "revMix", 0.45f }, { "revSize", 0.75f }, { "dlyMix", 0.3f }, { "dlyDiv", 5 }, { "dlyFb", 0.45f },
                  { "master", -3.2f }
              },
              { { VEL, D_CUT, 0.35f }, { LFO2, D_P2, 0.25f } } },

            { "Data Rot",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.32f }, { "osc1_level", 0.9f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_pos", 0.6f }, { "osc2_level", 0.42f },
                  { "osc2_tune", -12.0f }, { "osc1_uni", 2 },
                  { "fltType", 0 }, { "fltCutoff", 420.0f }, { "fltReso", 0.38f }, { "fltDrive", 0.5f },
                  { "fltEnv", 0.35f }, { "fltKey", 0.2f },
                  { "ampA", 0.006f }, { "ampD", 0.5f }, { "ampS", 0.78f }, { "ampR", 0.55f },
                  { "env1A", 0.004f }, { "env1D", 0.3f }, { "env1S", 0.15f }, { "env1R", 0.4f },
                  { "glOn", 1 }, { "glMix", 0.8f }, { "glBits", 7.0f }, { "glRateDiv", 3.0f },
                  { "glStep", 3 }, { "glStutterDiv", 4 }, { "glStutter", 0.45f }, { "glGate", 0.2f },
                  { "glDrop", 0.25f }, { "glTape", 0.12f }, { "glRepeats", 4.0f }, { "glChaos", 0.75f },
                  { "glNoise", 0.08f },
                  { "revMix", 0.26f }, { "revSize", 0.55f }, { "dlyMix", 0.22f }, { "dlyDiv", 5 },
                  { "master", -3.5f }
              },
              { { LFO1, D_CUT, 0.35f }, { (int) ModSource::chaos, (int) ModDest::pitch, 0.08f },
                { (int) ModSource::chaos, (int) ModDest::glitchMix, 0.35f } } },

            { "Particle Bloom",
              {
                  { "osc1_on", 1 }, { "osc1_table", 1 }, { "osc1_pos", 0.8f }, { "osc1_level", 0.85f },
                  { "osc1_uni", 3 }, { "osc1_spread", 0.6f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_level", 0.35f }, { "osc2_tune", 12.0f },
                  { "fltType", 1 }, { "fltCutoff", 3200.0f }, { "fltReso", 0.22f }, { "fltEnv", 0.2f }, { "fltKey", 0.4f },
                  { "ampA", 1.1f }, { "ampD", 2.0f }, { "ampS", 0.85f }, { "ampR", 4.0f },
                  { "env1A", 1.4f }, { "env1D", 2.0f }, { "env1S", 0.6f }, { "env1R", 3.0f },
                  { "grOn", 1 }, { "grMix", 0.72f }, { "grSize", 180.0f }, { "grDensity", 26.0f },
                  { "grPosition", 0.3f }, { "grSpray", 0.55f }, { "grPitchRand", 2.5f }, { "grReverse", 0.4f },
                  { "grSpread", 0.9f },
                  { "revMix", 0.58f }, { "revSize", 0.9f }, { "revDamp", 0.3f }, { "choMix", 0.3f },
                  { "lfo1_rate", 0.06f }, { "lfo1_depth", 0.8f }, { "lfo2_rate", 0.13f }, { "lfo2_depth", 0.45f },
                  { "maxVoices", 16.0f }, { "master", -3.0f }
              },
              { { LFO1, (int) ModDest::grainPosition, 0.4f }, { LFO2, D_PAN, 0.3f },
                { ENV2, (int) ModDest::grainMix, 0.2f } } },

            { "Circuit Cemetery",
              {
                  { "osc1_on", 1 }, { "osc1_table", 7 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_level", 0.5f }, { "osc2_tune", 7.0f },
                  { "osc3_on", 1 }, { "osc3_table", 6 }, { "osc3_level", 0.3f }, { "osc3_tune", -12.0f },
                  { "fltType", 2 }, { "fltCutoff", 1800.0f }, { "fltReso", 0.5f }, { "fltDrive", 0.3f },
                  { "fltEnv", 0.3f }, { "fltKey", 0.3f },
                  { "ampA", 0.02f }, { "ampD", 1.6f }, { "ampS", 0.2f }, { "ampR", 2.4f },
                  { "env1A", 0.01f }, { "env1D", 0.8f }, { "env1S", 0.1f }, { "env1R", 1.2f },
                  { "glOn", 1 }, { "glMix", 0.9f }, { "glBits", 7.0f }, { "glRateDiv", 6.0f },
                  { "glStep", 2 }, { "glStutterDiv", 2 }, { "glStutter", 0.55f }, { "glGate", 0.35f },
                  { "glDrop", 0.4f }, { "glTape", 0.3f }, { "glRepeats", 5.0f }, { "glChaos", 1.0f },
                  { "glNoise", 0.15f },
                  { "revMix", 0.55f }, { "revSize", 1.0f }, { "revDamp", 0.2f }, { "choMix", 0.2f },
                  { "lfo1_rate", 0.12f }, { "lfo2_rate", 0.9f },
                  { "master", -4.0f }
              },
              { { (int) ModSource::chaos, (int) ModDest::glitchBits, 0.6f },
                { LFO2, D_CUT, 0.4f }, { (int) ModSource::chaos, D_PAN, 0.4f } } },

            { "Pollen Storm",
              {
                  { "arpOn", 1 }, { "arpMode", 3 }, { "arpRate", 4 }, { "arpOct", 2.0f }, { "arpGate", 0.35f },
                  { "arpSwing", 0.12f },
                  { "osc1_on", 1 }, { "osc1_table", 5 }, { "osc1_pos", 0.5f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 7 }, { "osc2_level", 0.4f }, { "osc2_tune", 12.0f },
                  { "fltType", 1 }, { "fltCutoff", 3000.0f }, { "fltReso", 0.25f }, { "fltEnv", 0.4f }, { "fltKey", 0.5f },
                  { "ampA", 0.004f }, { "ampD", 0.45f }, { "ampS", 0.0f }, { "ampR", 0.6f },
                  { "env1A", 0.002f }, { "env1D", 0.3f }, { "env1S", 0.0f }, { "env1R", 0.4f },
                  { "grOn", 1 }, { "grMix", 0.55f }, { "grSize", 70.0f }, { "grDensity", 40.0f },
                  { "grPosition", 0.25f }, { "grSpray", 0.7f }, { "grPitchRand", 7.0f }, { "grReverse", 0.35f },
                  { "grSpread", 1.0f },
                  { "glOn", 1 }, { "glMix", 0.35f }, { "glBits", 10.0f }, { "glStutter", 0.2f },
                  { "glGate", 0.12f }, { "glChaos", 0.8f },
                  { "revMix", 0.45f }, { "dlyMix", 0.3f }, { "dlyDiv", 4 }, { "dlyFb", 0.4f },
                  { "lfo2_rate", 0.4f }, { "lfo2_depth", 0.5f },
                  { "master", -4.0f }
              },
              { { (int) ModSource::chaos, (int) ModDest::grainPosition, 0.55f },
                { (int) ModSource::chaos, (int) ModDest::pitch, 0.05f }, { VEL, D_CUT, 0.3f } } },

            { "Quantum Fern",
              {
                  { "osc1_on", 1 }, { "osc1_table", 6 }, { "osc1_pos", 0.2f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 3 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.45f },
                  { "fltType", 0 }, { "fltCutoff", 700.0f }, { "fltReso", 0.5f }, { "fltEnv", 0.2f }, { "fltKey", 0.25f },
                  { "ampA", 0.4f }, { "ampD", 1.2f }, { "ampS", 0.7f }, { "ampR", 3.0f },
                  { "env1A", 0.6f }, { "env1D", 1.5f }, { "env1S", 0.4f }, { "env1R", 2.0f },
                  { "env2A", 2.0f }, { "env2D", 3.0f }, { "env2S", 0.8f }, { "env2R", 4.0f },
                  { "grOn", 1 }, { "grMix", 0.85f }, { "grSize", 320.0f }, { "grDensity", 8.0f },
                  { "grPosition", 0.6f }, { "grSpray", 0.15f }, { "grPitch", -12.0f }, { "grPitchRand", 1.0f },
                  { "grReverse", 0.6f }, { "grSpread", 1.0f },
                  { "lfo1_rate", 0.08f }, { "lfo1_depth", 0.9f }, { "lfo1_shape", 7 },
                  { "revMix", 0.5f }, { "revSize", 0.9f }, { "dlyMix", 0.3f }, { "dlyDiv", 6 },
                  { "master", -4.5f }
              },
              { { LFO1, (int) ModDest::grainPosition, 0.6f }, { ENV2, (int) ModDest::grainMix, 0.3f },
                { (int) ModSource::chaos, D_PAN, 0.35f } } },

            { "Signal Decay",
              {
                  { "osc1_on", 1 }, { "osc1_table", 0 }, { "osc1_pos", 0.72f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_level", 0.3f }, { "osc2_tune", 12.0f },
                  { "fltType", 0 }, { "fltCutoff", 1200.0f }, { "fltReso", 0.3f }, { "fltEnv", 0.45f }, { "fltKey", 0.4f },
                  { "ampA", 0.25f }, { "ampD", 2.2f }, { "ampS", 0.6f }, { "ampR", 6.0f },
                  { "env1A", 0.1f }, { "env1D", 1.5f }, { "env1S", 0.3f }, { "env1R", 3.0f },
                  { "glOn", 1 }, { "glMix", 0.55f }, { "glBits", 9.0f }, { "glRateDiv", 12.0f },
                  { "glStep", 4 }, { "glStutter", 0.3f }, { "glGate", 0.1f }, { "glDrop", 0.15f },
                  { "glTape", 0.35f }, { "glChaos", 0.6f }, { "glNoise", 0.08f },
                  { "revMix", 0.62f }, { "revSize", 0.95f }, { "revDamp", 0.45f },
                  { "dlyMix", 0.25f }, { "dlyDiv", 6 }, { "dlyFb", 0.45f },
                  { "lfo2_rate", 0.2f }, { "lfo2_depth", 0.4f },
                  { "master", -3.5f }
              },
              { { LFO2, (int) ModDest::glitchMix, 0.3f }, { MW, D_REV, 0.25f } } },

            { "Botanic Glitch",
              {
                  { "osc1_on", 1 }, { "osc1_table", 5 }, { "osc1_pos", 0.6f }, { "osc1_level", 0.9f },
                  { "osc1_uni", 3 }, { "osc1_spread", 0.55f },
                  { "osc2_on", 1 }, { "osc2_table", 0 }, { "osc2_level", 0.42f }, { "osc2_tune", 12.0f },
                  { "osc3_on", 1 }, { "osc3_table", 6 }, { "osc3_level", 0.26f }, { "osc3_tune", -12.0f },
                  { "fltType", 0 }, { "fltCutoff", 2200.0f }, { "fltReso", 0.3f }, { "fltDrive", 0.2f },
                  { "fltEnv", 0.35f }, { "fltKey", 0.35f },
                  { "ampA", 0.12f }, { "ampD", 1.2f }, { "ampS", 0.75f }, { "ampR", 2.2f },
                  { "env1A", 0.3f }, { "env1D", 1.0f }, { "env1S", 0.4f }, { "env1R", 1.6f },
                  { "grOn", 1 }, { "grMix", 0.35f }, { "grSize", 90.0f }, { "grDensity", 14.0f },
                  { "grPosition", 0.4f }, { "grSpray", 0.4f }, { "grReverse", 0.2f }, { "grSpread", 0.7f },
                  { "glOn", 1 }, { "glMix", 0.42f }, { "glBits", 11.0f }, { "glStutter", 0.25f },
                  { "glGate", 0.15f }, { "glTape", 0.1f }, { "glChaos", 0.65f }, { "glNoise", 0.08f },
                  { "revMix", 0.5f }, { "revSize", 0.85f }, { "dlyMix", 0.28f }, { "dlyDiv", 4 },
                  { "choMix", 0.3f }, { "lfo1_rate", 0.15f }, { "lfo1_depth", 0.5f },
                  { "master", -3.5f }
              },
              { { LFO1, D_CUT, 0.4f }, { (int) ModSource::chaos, (int) ModDest::grainPosition, 0.35f },
                { MW, (int) ModDest::glitchMix, 0.4f } } },

            { "Tape Termite",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.9f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.35f },
                  { "fltType", 0 }, { "fltCutoff", 900.0f }, { "fltReso", 0.3f }, { "fltDrive", 0.25f },
                  { "fltEnv", 0.35f }, { "fltKey", 0.25f },
                  { "ampA", 0.008f }, { "ampD", 0.7f }, { "ampS", 0.72f }, { "ampR", 0.8f },
                  { "glOn", 1 }, { "glMix", 0.75f }, { "glBits", 10.0f }, { "glRateDiv", 2.0f },
                  { "glStep", 2 }, { "glStutterDiv", 4 }, { "glStutter", 0.35f }, { "glGate", 0.1f },
                  { "glDrop", 0.2f }, { "glTape", 0.55f }, { "glRepeats", 3.0f }, { "glChaos", 0.7f },
                  { "glNoise", 0.10f },
                  { "revMix", 0.28f }, { "revSize", 0.6f }, { "dlyMix", 0.2f }, { "dlyDiv", 5 },
                  { "lfo1_rate", 0.1f }, { "lfo1_depth", 0.6f },
                  { "master", -3.5f }
              },
              { { LFO1, (int) ModDest::glitchMix, 0.35f }, { (int) ModSource::chaos, D_CUT, 0.25f } } },

            { "Mycelium",
              {
                  { "osc1_on", 1 }, { "osc1_table", 6 }, { "osc1_pos", 0.35f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 0 }, { "osc2_pos", 0.1f }, { "osc2_tune", 12.0f },
                  { "osc2_level", 0.35f },
                  { "osc3_on", 1 }, { "osc3_table", 5 }, { "osc3_level", 0.22f }, { "osc3_tune", 19.0f },
                  { "fltType", 1 }, { "fltCutoff", 1400.0f }, { "fltReso", 0.3f }, { "fltEnv", 0.2f }, { "fltKey", 0.35f },
                  { "ampA", 0.8f }, { "ampD", 1.8f }, { "ampS", 0.8f }, { "ampR", 3.5f },
                  { "env1A", 1.2f }, { "env1D", 2.0f }, { "env1S", 0.5f }, { "env1R", 2.4f },
                  { "env2A", 2.5f }, { "env2D", 3.0f }, { "env2S", 0.7f }, { "env2R", 4.0f },
                  { "grOn", 1 }, { "grMix", 0.62f }, { "grSize", 220.0f }, { "grDensity", 9.0f },
                  { "grPosition", 0.45f }, { "grSpray", 0.35f }, { "grPitch", -12.0f }, { "grPitchRand", 1.5f },
                  { "grReverse", 0.5f }, { "grSpread", 1.0f },
                  { "lfo1_rate", 0.05f }, { "lfo1_depth", 0.85f },
                  { "revMix", 0.6f }, { "revSize", 0.95f }, { "choMix", 0.35f }, { "choRate", 0.3f },
                  { "master", -4.0f }
              },
              { { LFO1, (int) ModDest::grainPosition, 0.5f }, { ENV2, (int) ModDest::grainMix, 0.25f },
                { (int) ModSource::chaos, D_PAN, 0.3f } } },

            { "Broken Radio",
              {
                  { "osc1_on", 1 }, { "osc1_table", 4 }, { "osc1_pos", 0.5f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 2 }, { "osc2_level", 0.45f }, { "osc2_tune", 7.0f },
                  { "fltType", 2 }, { "fltCutoff", 1500.0f }, { "fltReso", 0.55f }, { "fltDrive", 0.45f },
                  { "fltEnv", 0.25f }, { "fltKey", 0.3f },
                  { "ampA", 0.02f }, { "ampD", 0.9f }, { "ampS", 0.45f }, { "ampR", 0.5f },
                  { "glOn", 1 }, { "glMix", 0.85f }, { "glBits", 8.0f }, { "glRateDiv", 3.0f },
                  { "glStep", 4 }, { "glStutterDiv", 5 }, { "glStutter", 0.5f }, { "glGate", 0.4f },
                  { "glDrop", 0.5f }, { "glTape", 0.15f }, { "glRepeats", 5.0f }, { "glChaos", 0.9f },
                  { "glNoise", 0.10f },
                  { "revMix", 0.25f }, { "dlyMix", 0.3f }, { "dlyDiv", 4 }, { "dlyFb", 0.45f },
                  { "master", -4.5f }
              },
              { { (int) ModSource::chaos, (int) ModDest::glitchBits, 0.35f }, { LFO2, D_CUT, 0.3f } } },

            { "Fern Fractal",
              {
                  { "arpOn", 1 }, { "arpMode", 2 }, { "arpRate", 3 }, { "arpOct", 2.0f }, { "arpGate", 0.4f },
                  { "arpSwing", 0.15f },
                  { "osc1_on", 1 }, { "osc1_table", 2 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 5 }, { "osc2_level", 0.4f }, { "osc2_tune", 12.0f },
                  { "fltType", 1 }, { "fltCutoff", 2600.0f }, { "fltReso", 0.25f }, { "fltEnv", 0.4f }, { "fltKey", 0.5f },
                  { "ampA", 0.003f }, { "ampD", 0.5f }, { "ampS", 0.15f }, { "ampR", 0.6f },
                  { "env1A", 0.002f }, { "env1D", 0.35f }, { "env1S", 0.1f }, { "env1R", 0.4f },
                  { "grOn", 1 }, { "grMix", 0.42f }, { "grSize", 60.0f }, { "grDensity", 30.0f },
                  { "grPosition", 0.3f }, { "grSpray", 0.6f }, { "grPitchRand", 5.0f }, { "grReverse", 0.3f },
                  { "glOn", 1 }, { "glMix", 0.3f }, { "glBits", 12.0f }, { "glStutter", 0.15f },
                  { "glChaos", 0.75f },
                  { "revMix", 0.4f }, { "dlyMix", 0.28f }, { "dlyDiv", 3 }, { "dlyFb", 0.4f },
                  { "lfo2_rate", 0.3f }, { "lfo2_depth", 0.5f },
                  { "master", -4.0f }
              },
              { { (int) ModSource::chaos, (int) ModDest::grainPosition, 0.6f }, { LFO2, D_CUT, 0.35f } } },

            { "Spore Drift",
              {
                  { "osc1_on", 1 }, { "osc1_table", 1 }, { "osc1_pos", 0.6f }, { "osc1_level", 0.8f },
                  { "osc2_on", 1 }, { "osc2_table", 6 }, { "osc2_pos", 0.4f }, { "osc2_level", 0.35f },
                  { "osc2_tune", 7.0f },
                  { "fltType", 0 }, { "fltCutoff", 800.0f }, { "fltReso", 0.35f }, { "fltEnv", 0.1f }, { "fltKey", 0.3f },
                  { "ampA", 1.5f }, { "ampD", 2.5f }, { "ampS", 0.9f }, { "ampR", 5.0f },
                  { "env1A", 2.0f }, { "env1D", 3.0f }, { "env1S", 0.6f }, { "env1R", 4.0f },
                  { "env2A", 3.0f }, { "env2D", 4.0f }, { "env2S", 0.8f }, { "env2R", 5.0f },
                  { "grOn", 1 }, { "grMix", 0.9f }, { "grSize", 380.0f }, { "grDensity", 6.0f },
                  { "grPosition", 0.55f }, { "grSpray", 0.2f }, { "grPitchRand", 0.8f }, { "grReverse", 0.7f },
                  { "grSpread", 1.0f },
                  { "lfo1_rate", 0.04f }, { "lfo1_depth", 0.9f },
                  { "revMix", 0.55f }, { "revSize", 1.0f }, { "revDamp", 0.5f }, { "dlyMix", 0.2f },
                  { "maxVoices", 12.0f }, { "master", -4.5f }
              },
              { { LFO1, (int) ModDest::grainPosition, 0.7f }, { ENV2, (int) ModDest::grainMix, 0.2f } } },

            { "Neural Rot",
              {
                  { "osc1_on", 1 }, { "osc1_table", 7 }, { "osc1_pos", 0.3f }, { "osc1_level", 0.85f },
                  { "osc2_on", 1 }, { "osc2_table", 3 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.5f },
                  { "fltType", 0 }, { "fltCutoff", 1100.0f }, { "fltReso", 0.4f }, { "fltDrive", 0.55f },
                  { "fltEnv", 0.3f }, { "fltKey", 0.3f },
                  { "ampA", 0.05f }, { "ampD", 1.4f }, { "ampS", 0.5f }, { "ampR", 2.2f },
                  { "glOn", 1 }, { "glMix", 0.9f }, { "glBits", 7.0f }, { "glRateDiv", 9.0f },
                  { "glStep", 5 }, { "glStutterDiv", 6 }, { "glStutter", 0.55f }, { "glGate", 0.25f },
                  { "glDrop", 0.35f }, { "glTape", 0.25f }, { "glRepeats", 6.0f }, { "glChaos", 1.0f },
                  { "glNoise", 0.14f },
                  { "revMix", 0.5f }, { "revSize", 0.9f }, { "dlyMix", 0.25f }, { "dlyDiv", 6 },
                  { "master", -5.0f }
              },
              { { (int) ModSource::chaos, (int) ModDest::glitchMix, 0.45f },
                { (int) ModSource::chaos, (int) ModDest::pitch, 0.06f } } },

            { "Botanic Bass",
              {
                  { "polyMode", 1 }, { "glide", 45.0f },
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.2f }, { "osc1_level", 1.0f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_tune", -12.0f }, { "osc2_level", 0.7f },
                  { "osc2_pos", 0.1f },
                  { "fltType", 0 }, { "fltCutoff", 320.0f }, { "fltReso", 0.22f }, { "fltDrive", 0.4f },
                  { "fltEnv", 0.45f }, { "fltKey", 0.2f },
                  { "ampA", 0.003f }, { "ampD", 0.4f }, { "ampS", 0.6f }, { "ampR", 0.2f },
                  { "env1A", 0.001f }, { "env1D", 0.3f }, { "env1S", 0.1f }, { "env1R", 0.3f },
                  { "revMix", 0.05f }, { "dlyMix", 0.0f }, { "choMix", 0.0f },
                  { "master", -2.0f }
              },
              { { VEL, D_CUT, 0.45f } } },

            { "Vowel Garden",
              {
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.35f }, { "osc1_level", 0.75f },
                  { "osc1_uni", 3 }, { "osc1_spread", 0.5f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_pos", 0.6f }, { "osc2_level", 0.35f },
                  { "osc2_tune", 12.0f },
                  { "fltType", 1 }, { "fltCutoff", 2600.0f }, { "fltReso", 0.2f }, { "fltEnv", 0.25f }, { "fltKey", 0.4f },
                  { "ampA", 0.5f }, { "ampD", 1.6f }, { "ampS", 0.8f }, { "ampR", 3.0f },
                  { "env1A", 0.8f }, { "env1D", 1.8f }, { "env1S", 0.5f }, { "env1R", 2.4f },
                  { "fmtOn", 1 }, { "fmtVowel", 0.3f }, { "fmtShift", 0.0f }, { "fmtQ", 0.62f },
                  { "fmtMix", 0.85f }, { "fmtKey", 0.45f },
                  { "toneTilt", 1.5f }, { "toneWidth", 1.25f },
                  { "lfo1_rate", 0.09f }, { "lfo1_depth", 0.7f }, { "lfo2_rate", 0.04f }, { "lfo2_depth", 0.5f },
                  { "revMix", 0.55f }, { "revSize", 0.9f }, { "dlyMix", 0.22f }, { "dlyDiv", 5 },
                  { "master", -4.0f }
              },
              { { LFO2, (int) ModDest::formantShift, 0.45f }, { LFO1, D_CUT, 0.3f },
                { ENV2, (int) ModDest::formantShift, 0.2f } } },

            { "Talking Bass",
              {
                  { "polyMode", 1 }, { "glide", 30.0f },
                  { "osc1_on", 1 }, { "osc1_table", 3 }, { "osc1_pos", 0.25f }, { "osc1_level", 0.95f },
                  { "osc2_on", 1 }, { "osc2_table", 4 }, { "osc2_level", 0.4f }, { "osc2_tune", -12.0f },
                  { "fltType", 0 }, { "fltCutoff", 900.0f }, { "fltReso", 0.35f }, { "fltDrive", 0.35f },
                  { "fltEnv", 0.55f }, { "fltKey", 0.25f },
                  { "ampA", 0.004f }, { "ampD", 0.5f }, { "ampS", 0.62f }, { "ampR", 0.3f },
                  { "env1A", 0.002f }, { "env1D", 0.28f }, { "env1S", 0.15f }, { "env1R", 0.3f },
                  { "env2A", 0.05f }, { "env2D", 0.5f }, { "env2S", 0.35f }, { "env2R", 0.4f },
                  { "fmtOn", 1 }, { "fmtVowel", 0.1f }, { "fmtShift", -300.0f }, { "fmtQ", 0.68f },
                  { "fmtMix", 0.7f }, { "fmtKey", 0.6f },
                  { "toneTilt", -1.0f },
                  { "revMix", 0.08f }, { "dlyMix", 0.0f }, { "choMix", 0.0f },
                  { "master", -3.0f }
              },
              { { ENV2, (int) ModDest::formantShift, 0.4f }, { VEL, D_CUT, 0.4f } } },

            { "Petal Choir",
              {
                  { "osc1_on", 1 }, { "osc1_table", 6 }, { "osc1_pos", 0.4f }, { "osc1_level", 0.8f },
                  { "osc1_uni", 4 }, { "osc1_spread", 0.7f }, { "osc1_detune", 9.0f },
                  { "osc2_on", 1 }, { "osc2_table", 1 }, { "osc2_tune", 12.0f }, { "osc2_level", 0.3f },
                  { "fltType", 1 }, { "fltCutoff", 3200.0f }, { "fltReso", 0.18f }, { "fltEnv", 0.2f }, { "fltKey", 0.35f },
                  { "ampA", 1.2f }, { "ampD", 2.2f }, { "ampS", 0.9f }, { "ampR", 4.5f },
                  { "env1A", 1.5f }, { "env1D", 2.5f }, { "env1S", 0.6f }, { "env1R", 3.5f },
                  { "fmtOn", 1 }, { "fmtVowel", 0.6f }, { "fmtShift", 200.0f }, { "fmtQ", 0.5f },
                  { "fmtMix", 0.55f }, { "fmtKey", 0.3f },
                  { "grOn", 1 }, { "grMix", 0.32f }, { "grSize", 180.0f }, { "grDensity", 10.0f },
                  { "grPosition", 0.5f }, { "grSpray", 0.25f }, { "grSpread", 0.9f },
                  { "toneTilt", 2.0f }, { "toneWidth", 1.35f }, { "toneAir", 0.25f },
                  { "lfo1_rate", 0.06f }, { "lfo1_depth", 0.6f },
                  { "revMix", 0.62f }, { "revSize", 0.95f }, { "revDamp", 0.35f }, { "choMix", 0.35f },
                  { "maxVoices", 12.0f }, { "master", -4.5f }
              },
              { { LFO1, (int) ModDest::grainPosition, 0.4f }, { LFO1, (int) ModDest::formantShift, 0.25f },
                { MW, D_REV, 0.3f } } },

            { "Botanic Init",
              {
                  { "osc1_on", 1 }, { "osc1_table", 0 }, { "osc1_pos", 0.5f },
                  { "fltCutoff", 3000.0f }, { "ampA", 0.01f }, { "ampD", 0.4f }, { "ampS", 0.7f }, { "ampR", 0.4f },
                  { "revMix", 0.18f }, { "dlyMix", 0.0f }, { "choMix", 0.0f }, { "master", -1.5f }
              },
              {} }
        };

        return presets;
    }

    void writeParams (juce::AudioProcessorValueTreeState& apvts, const std::vector<std::pair<const char*, float>>& values)
    {
        for (const auto& v : values)
            if (auto* p = apvts.getParameter (v.first))
                p->setValueNotifyingHost (p->convertTo0to1 (v.second));
    }
}

juce::StringArray factoryPresetNames()
{
    juce::StringArray names;
    for (const auto& p : presetTable())
        names.add (p.name);
    return names;
}

void applyFactoryPreset (int index, juce::AudioProcessorValueTreeState& apvts, ModMatrix& matrix)
{
    const auto& table = presetTable();
    if (index < 0 || index >= (int) table.size())
        return;

    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    matrix.clear();

    const auto& preset = table[(size_t) index];
    writeParams (apvts, preset.values);

    for (const auto& m : preset.mods)
        matrix.set (m.source, m.dest, m.amount);
}

juce::ValueTree buildFactoryPresetState (int index)
{
    const auto& table = presetTable();
    if (index < 0 || index >= (int) table.size())
        return {};

    // Build a plain tree of parameter values (used by the preset preview / docs).
    juce::ValueTree state ("BOTANIC_PRESET");
    state.setProperty ("name", table[(size_t) index].name, nullptr);

    for (const auto& v : table[(size_t) index].values)
        state.setProperty (v.first, v.second, nullptr);

    juce::ValueTree mods ("MODMATRIX");
    for (const auto& m : table[(size_t) index].mods)
    {
        juce::ValueTree slot ("SLOT");
        slot.setProperty ("source", m.source, nullptr);
        slot.setProperty ("dest", m.dest, nullptr);
        slot.setProperty ("amount", m.amount, nullptr);
        mods.appendChild (slot, nullptr);
    }
    state.appendChild (mods, nullptr);
    return state;
}

} // namespace botanic
