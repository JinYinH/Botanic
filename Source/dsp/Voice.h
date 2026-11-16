#pragma once

#include "Dsp.h"
#include "../ModMatrix.h"

namespace botanic
{

//==============================================================================
struct OscParams
{
    bool on = false;
    int table = 0;
    float pos = 0.5f, tune = 0.0f, detune = 0.0f, level = 1.0f, pan = 0.0f, spread = 0.35f, phase = 0.0f;
    int unison = 1;
};

struct EnvParams { float a = 0.01f, d = 0.4f, s = 0.7f, r = 0.3f; };

struct LfoParams
{
    float rateHz = 1.0f, depth = 0.5f, phase = 0.0f;
    bool retrigger = true;
    int shape = 0;
};

struct ActiveMod
{
    float amount = 0.0f;
    int source = 0;
    int dest = 0;
};

/** A plain snapshot of every parameter the engine needs for one block. */
struct EngineParams
{
    float a4 = 440.0f;
    int polyMode = 0;
    int maxVoices = 16;
    float glideMs = 0.0f, bendRange = 2.0f, ampVel = 0.6f;
    float pitchBend = 0.0f;
    float modWheel = 0.0f, aftertouch = 0.0f;

    OscParams osc[3];

    int filterType = 0;
    float cutoff = 1200.0f, resonance = 0.32f, drive = 0.12f, filterEnv = 0.25f, keyTrack = 0.0f;

    EnvParams ampEnv, env1, env2;
    LfoParams lfo[2];

    bool fmtOn = false;
    float fmtVowel = 0.15f, fmtShift = 0.0f, fmtQ = 0.55f, fmtMix = 0.65f, fmtKey = 0.35f;

    ActiveMod mods[maxModSlots];
    int numMods = 0;

    float globalDest[2] = { 0.0f, 0.0f };   // reverb mix, delay mix offsets
};

//==============================================================================
/** One polyphonic voice: three wavetable oscillators -> stereo filter -> amp. */
class Voice
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        for (auto& o : oscs) o.prepare (sr);
        formant.prepare (sr);
        ampEnv.prepare (sr);
        env1.prepare (sr);
        env2.prepare (sr);
        for (auto& l : lfos) l.prepare (sr);
        rnd.setSeed (juce::Random::getSystemRandom().nextInt());
    }

    void noteOn (int midiNote, float velocity, const EngineParams& p)
    {
        const auto wasActive = active;
        note = midiNote;
        vel = velocity;
        active = true;
        keyDown = true;
        finished = false;

        const auto freq = midiToFreq ((float) midiNote + p.pitchBend, p.a4);
        targetFreq = freq;
        if (p.glideMs < 0.5f || ! wasActive)
            currentFreq = freq;

        randomValue = rnd.nextFloat();
        chaosValue = (rnd.nextFloat() * 2.0f - 1.0f) * 0.5f;

        for (int i = 0; i < 3; ++i)
        {
            oscs[(size_t) i].setUnison (p.osc[i].unison, p.osc[i].detune, p.osc[i].spread);
            auto startPhase = p.osc[i].phase + randomValue * 0.13f;
            startPhase -= std::floor (startPhase);
            oscs[(size_t) i].resetPhases (startPhase);
        }

        ampEnv.setParams (p.ampEnv.a, p.ampEnv.d, p.ampEnv.s, p.ampEnv.r);
        env1.setParams (p.env1.a, p.env1.d, p.env1.s, p.env1.r);
        env2.setParams (p.env2.a, p.env2.d, p.env2.s, p.env2.r);

        ampEnv.noteOn();
        env1.noteOn();
        env2.noteOn();

        for (int i = 0; i < 2; ++i)
        {
            lfos[(size_t) i].setRate (p.lfo[i].rateHz);
            lfos[(size_t) i].setShape (p.lfo[i].shape);
            if (p.lfo[i].retrigger)
                lfos[(size_t) i].reset (p.lfo[i].phase);
        }

        ladderL.reset(); ladderR.reset();
        svfL.reset(); svfR.reset();
        formant.reset();
    }

    /** Retunes a running voice without retriggering (mono legato + glide). */
    void glideTo (int midiNote, const EngineParams& p, bool immediate)
    {
        note = midiNote;
        targetFreq = midiToFreq ((float) midiNote + p.pitchBend, p.a4);
        if (immediate)
            currentFreq = targetFreq;
    }

    void noteOff()
    {
        keyDown = false;
        ampEnv.noteOff();
        env1.noteOff();
        env2.noteOff();
    }

    void fastRelease()
    {
        keyDown = false;
        ampEnv.fastRelease();
        env1.fastRelease();
        env2.fastRelease();
    }

    void kill()
    {
        active = false;
        keyDown = false;
        finished = true;
        ampEnv.reset();
        env1.reset();
        env2.reset();
    }

    bool isActive() const noexcept   { return active; }
    bool isKeyDown() const noexcept  { return keyDown; }
    bool isFinished() const noexcept { return finished; }
    int getNote() const noexcept     { return note; }
    float getLevel() const noexcept  { return ampEnv.getLevel(); }

    void render (float* left, float* right, int numSamples, const EngineParams& p)
    {
        if (! active)
            return;

        const auto& bank = WavetableBank::instance();
        const WavetableBank::Table* tables[3] { p.osc[0].on ? &bank.getTable (p.osc[0].table) : nullptr,
                                                p.osc[1].on ? &bank.getTable (p.osc[1].table) : nullptr,
                                                p.osc[2].on ? &bank.getTable (p.osc[2].table) : nullptr };

        const auto glideCoef = p.glideMs > 0.5f
                                 ? 1.0f - std::exp (-1000.0f / (p.glideMs * sampleRate))
                                 : 1.0f;

        const auto velGain = (1.0f - p.ampVel) + p.ampVel * vel;
        const auto keyTrackOct = p.keyTrack * ((float) note - 60.0f) / 12.0f;
        const auto filterMode = juce::jlimit (0, 4, p.filterType);

        for (int i = 0; i < numSamples; ++i)
        {
            currentFreq += (targetFreq - currentFreq) * glideCoef;

            const auto a = ampEnv.next();
            const auto e1 = env1.next();
            const auto e2 = env2.next();

            float modAcc[numModDests] = {};

            if (p.numMods > 0)
            {
                const auto l1 = lfos[0].next (rnd) * p.lfo[0].depth;
                const auto l2 = lfos[1].next (rnd) * p.lfo[1].depth;

                for (int m = 0; m < p.numMods; ++m)
                {
                    const auto& slot = p.mods[m];
                    float value = 0.0f;

                    switch ((ModSource) slot.source)
                    {
                        case ModSource::lfo1:       value = l1; break;
                        case ModSource::lfo2:       value = l2; break;
                        case ModSource::env1:       value = e1; break;
                        case ModSource::env2:       value = e2; break;
                        case ModSource::velocity:   value = vel; break;
                        case ModSource::modWheel:   value = p.modWheel; break;
                        case ModSource::keyTrack:   value = juce::jlimit (-1.0f, 1.0f, ((float) note - 60.0f) / 48.0f); break;
                        case ModSource::random:     value = randomValue * 2.0f - 1.0f; break;
                        case ModSource::aftertouch: value = p.aftertouch; break;
                        case ModSource::chaos:
                            chaosValue += (rnd.nextFloat() * 2.0f - 1.0f) * 0.0015f;
                            chaosValue = juce::jlimit (-1.0f, 1.0f, chaosValue * 0.9995f);
                            value = chaosValue;
                            break;
                        default: break;
                    }

                    modAcc[slot.dest] += slot.amount * value;
                }

                for (int d = 0; d < numModDests; ++d)
                    modAcc[d] = juce::jlimit (-2.0f, 2.0f, modAcc[d]);
            }

            // ---- pitch
            const auto pitchSemis = modAcc[(int) ModDest::pitch] * 12.0f;
            const auto pitchMul = std::exp2 (pitchSemis * (1.0f / 12.0f));

            // ---- filter
            const auto cutoffOct = modAcc[(int) ModDest::cutoff] * 5.0f
                                     + p.filterEnv * e1 * 6.0f
                                     + keyTrackOct;
            const auto cutoffHz = juce::jlimit (16.0f, sampleRate * 0.45f,
                                                p.cutoff * std::exp2 (cutoffOct));
            const auto reso = juce::jlimit (0.0f, 1.0f, p.resonance + modAcc[(int) ModDest::resonance]);
            const auto dr = juce::jlimit (0.0f, 1.0f, p.drive + modAcc[(int) ModDest::filterDrive]);

            float monoL = 0.0f, monoR = 0.0f;

            for (int o = 0; o < 3; ++o)
            {
                if (tables[o] == nullptr)
                    continue;

                const auto& op = p.osc[o];
                const auto detune = op.detune + ((o == 0) ? modAcc[(int) ModDest::osc1Detune] * 50.0f : 0.0f);
                const auto freq = currentFreq * pitchMul
                                    * std::exp2 (op.tune * (1.0f / 12.0f))
                                    * std::exp2 (detune * (1.0f / 1200.0f));

                if (freq < 8.0f || freq > sampleRate * 0.49f)
                    continue;

                oscs[(size_t) o].setUnison (op.unison, op.detune, op.spread);

                const auto pos = juce::jlimit (0.0f, 1.0f, op.pos + modAcc[(int) ModDest::osc1Pos + o]);
                const auto level = juce::jlimit (0.0f, 1.0f, op.level + modAcc[(int) ModDest::osc1Level + o]);
                const auto pan = juce::jlimit (-1.0f, 1.0f, op.pan + modAcc[(int) ModDest::pan]);

                float ol = 0.0f, orr = 0.0f;
                oscs[(size_t) o].render (*tables[o], freq, pos, ol, orr);

                const auto gainL = level * (pan <= 0.0f ? 1.0f : 1.0f - pan);
                const auto gainR = level * (pan >= 0.0f ? 1.0f : 1.0f + pan);

                monoL += ol * gainL;
                monoR += orr * gainR;
            }

            float fl, fr;

            if (filterMode == 0)
            {
                ladderL.set (cutoffHz, reso, sampleRate);
                ladderR.set (cutoffHz, reso, sampleRate);
                fl = ladderL.process (monoL, dr);
                fr = ladderR.process (monoR, dr);
            }
            else
            {
                const auto mode = filterMode == 1 ? SvfFilter::lowPass
                                : filterMode == 2 ? SvfFilter::bandPass
                                : filterMode == 3 ? SvfFilter::highPass
                                : SvfFilter::notch;
                svfL.set (cutoffHz, reso, sampleRate);
                svfR.set (cutoffHz, reso, sampleRate);
                fl = svfL.process (monoL, mode);
                fr = svfR.process (monoR, mode);
            }

            if (! std::isfinite (fl) || ! std::isfinite (fr))
            {
                fl = 0.0f;
                fr = 0.0f;
                ladderL.reset(); ladderR.reset();
                svfL.reset(); svfR.reset();
                formant.reset();
            }

            if (p.fmtOn && p.fmtMix > 0.001f)
            {
                if ((i & 15) == 0)
                {
                    const auto keyOct = p.fmtKey * ((float) note - 60.0f) / 12.0f;
                    formant.set (p.fmtVowel,
                                 p.fmtShift + modAcc[(int) ModDest::formantShift] * 1200.0f,
                                 p.fmtQ, keyOct);
                }

                auto wl = fl, wr = fr;
                formant.process (wl, wr);
                const auto wet = p.fmtMix;
                const auto makeup = 1.0f + wet * 0.55f;
                fl = fl * (1.0f - wet) + wl * wet * makeup;
                fr = fr * (1.0f - wet) + wr * wet * makeup;
            }

            const auto out = a * velGain * 0.34f;
            left[i] += fl * out;
            right[i] += fr * out;

            if (! ampEnv.isActive() && ! env1.isActive() && ! env2.isActive())
            {
                active = false;
                finished = true;
                break;
            }
        }
    }

private:
    float sampleRate = 44100.0f;
    int note = 60;
    float vel = 1.0f;
    bool active = false, keyDown = false, finished = false;
    float currentFreq = 440.0f, targetFreq = 440.0f;
    float randomValue = 0.5f;
    float chaosValue = 0.0f;

    WtOscillator oscs[3];
    AdsrEnv ampEnv, env1, env2;
    Lfo lfos[2];
    LadderFilter ladderL, ladderR;
    SvfFilter svfL, svfR;
    FormantBank formant;
    juce::Random rnd;
};

} // namespace botanic
