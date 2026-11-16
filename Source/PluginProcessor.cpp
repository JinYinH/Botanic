#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace botanic;

namespace
{
    inline float softClip (float x) noexcept
    {
        constexpr float knee = 0.90f;
        const auto a = std::abs (x);
        if (a <= knee)
            return x;
        const auto shaped = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
        return x < 0.0f ? -shaped : shaped;
    }

    inline double syncBeats (int divisionIndex)
    {
        static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 1.0 / 3.0, 0.75, 0.25, 1.0 / 6.0, 0.375, 0.125, 0.0625 };
        return beats[(size_t) juce::jlimit (0, 10, divisionIndex)];
    }
}

//==============================================================================
BotanicAudioProcessor::BotanicAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BOTANIC", Params::createParameters())
{
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            paramPointers[ranged->paramID] = apvts.getRawParameterValue (ranged->paramID);

    for (auto& c : ccToParam)
        c.store (-1);

    refreshPresetList();
    events.reserve (512);

    presetIndex.store (0);
    if (! presetNames.isEmpty())
        applyPresetInternal (0, false);

    takeUndoSnapshot (true);
}

BotanicAudioProcessor::~BotanicAudioProcessor() = default;

juce::File BotanicAudioProcessor::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
             .getChildFile ("Botanic")
             .getChildFile ("Presets");
}

void BotanicAudioProcessor::applyPresetInternal (int index, bool pushUndo)
{
    if (presetNames.isEmpty())
        return;

    const auto count = presetNames.size();
    index = ((index % count) + count) % count;

    if (pushUndo)
        takeUndoSnapshot (true);

    const auto numFactory = factoryPresetNames().size();

    if (index < numFactory)
    {
        applyFactoryPreset (index, apvts, modMatrix);
        currentPresetName = presetNames[index];
    }
    else
    {
        const auto fileIndex = index - numFactory;
        if (fileIndex >= 0 && fileIndex < userPresetFiles.size())
        {
            const auto file = userPresetFiles[fileIndex];
            if (auto xml = juce::parseXML (file))
            {
                const auto tree = juce::ValueTree::fromXml (*xml);
                if (tree.isValid())
                    reloadFromState (tree, false);
                currentPresetName = file.getFileNameWithoutExtension();
            }
        }
    }

    presetIndex.store (index);
    requestDspReset();

    if (onPresetChanged != nullptr)
    {
        auto cb = onPresetChanged;
        juce::MessageManager::callAsync ([cb] { if (cb) cb(); });
    }
}

//==============================================================================
void BotanicAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate * 2.0, samplesPerBlock * 2);
    const auto voiceCapacity = juce::jmax (samplesPerBlock * 2, 8192) + 8;
    voiceBufferL.assign ((size_t) voiceCapacity, 0.0f);
    voiceBufferR.assign ((size_t) voiceCapacity, 0.0f);
    decimatorL.reset();
    decimatorR.reset();
    dcBlockL.prepare (sampleRate);
    dcBlockR.prepare (sampleRate);
    limiterGain = 1.0f;
    limiterAttack = 1.0 - std::exp (-1.0 / (0.0005 * sampleRate));
    limiterRelease = 1.0 - std::exp (-1.0 / (0.120 * sampleRate));
    setLatencySamples (botanic::HalfBandDecimator::latencyInBaseSamples);
    effectsChain.prepare (sampleRate, samplesPerBlock);
    arpeggiator.prepare (sampleRate);
    texture.prepare (sampleRate, samplesPerBlock);
    tone.prepare (sampleRate, samplesPerBlock);
    juce::ignoreUnused (WavetableBank::instance());   // build tables off the audio thread
    renderBuffer.setSize (2, juce::jmax (32, samplesPerBlock), false, true, true);
    sustainedNotes.reserve (32);
    physicallyHeld.reserve (32);
    transportWasPlaying = false;
}

void BotanicAudioProcessor::releaseResources()
{
}

//==============================================================================
float BotanicAudioProcessor::paramValue (const juce::String& id) const
{
    auto it = paramPointers.find (id);
    return it != paramPointers.end() ? it->second->load() : 0.0f;
}

juce::RangedAudioParameter* BotanicAudioProcessor::param (const juce::String& id) const
{
    return apvts.getParameter (id);
}

bool BotanicAudioProcessor::isParamModulated (const juce::String& id) const
{
    for (int d = 0; d < numModDests; ++d)
    {
        const auto* pid = modDestParamId (d);
        if (pid == nullptr || id != pid)
            continue;

        for (const auto& slot : modMatrix.getSlots())
            if (slot.source >= 0 && slot.dest == d)
                return true;
    }
    return false;
}

//==============================================================================
void BotanicAudioProcessor::buildEngineParams()
{
    EngineParams p;

    float grainMixMacro = 0.0f, grainPosMacro = 0.0f;
    float glitchMixMacro = 0.0f, glitchBitsMacro = 0.0f, revMixMacro = 0.0f;

    p.a4 = paramValue (Params::a4);
    p.polyMode = (int) paramValue (Params::polyMode);
    p.maxVoices = (int) paramValue (Params::maxVoices);
    p.glideMs = paramValue (Params::glide);
    p.bendRange = paramValue (Params::bendRange);
    p.ampVel = paramValue (Params::ampVel);
    p.pitchBend = pitchBendSemis;
    p.modWheel = modWheel;
    p.aftertouch = aftertouch;

    for (int i = 0; i < 3; ++i)
    {
        auto& o = p.osc[i];
        o.on = paramValue (Params::oscId (i, "on")) > 0.5f;
        o.table = (int) paramValue (Params::oscId (i, "table"));
        o.pos = paramValue (Params::oscId (i, "pos"));
        o.tune = paramValue (Params::oscId (i, "tune"));
        o.detune = paramValue (Params::oscId (i, "detune"));
        o.level = paramValue (Params::oscId (i, "level"));
        o.pan = paramValue (Params::oscId (i, "pan"));
        o.unison = (int) paramValue (Params::oscId (i, "uni"));
        o.spread = paramValue (Params::oscId (i, "spread"));
        o.phase = paramValue (Params::oscId (i, "phase"));
    }

    p.filterType = (int) paramValue (Params::filterType);
    p.cutoff = paramValue (Params::cutoff);
    p.resonance = paramValue (Params::resonance);
    p.drive = paramValue (Params::drive);
    p.filterEnv = paramValue (Params::filterEnv);
    p.keyTrack = paramValue (Params::keyTrack);

    p.ampEnv = { paramValue (Params::envId (0, "A")), paramValue (Params::envId (0, "D")),
                 paramValue (Params::envId (0, "S")), paramValue (Params::envId (0, "R")) };
    p.env1 = { paramValue (Params::envId (1, "A")), paramValue (Params::envId (1, "D")),
               paramValue (Params::envId (1, "S")), paramValue (Params::envId (1, "R")) };
    p.env2 = { paramValue (Params::envId (2, "A")), paramValue (Params::envId (2, "D")),
               paramValue (Params::envId (2, "S")), paramValue (Params::envId (2, "R")) };

    const auto bpm = (double) lastBpm.load();

    for (int i = 0; i < 2; ++i)
    {
        auto& l = p.lfo[i];
        const auto sync = paramValue (Params::lfoId (i, "sync")) > 0.5f;
        const auto rate = paramValue (Params::lfoId (i, "rate"));
        const auto div = (int) paramValue (Params::lfoId (i, "div"));

        l.rateHz = sync ? (float) ((bpm / 60.0) / syncBeats (div)) : rate;
        l.depth = paramValue (Params::lfoId (i, "depth"));
        l.phase = paramValue (Params::lfoId (i, "phase"));
        l.retrigger = paramValue (Params::lfoId (i, "retrig")) > 0.5f;
        l.shape = (int) paramValue (Params::lfoId (i, "shape"));
    }

    p.fmtOn = paramValue (Params::fmtOn) > 0.5f;
    p.fmtVowel = paramValue (Params::fmtVowel);
    p.fmtShift = paramValue (Params::fmtShift);
    p.fmtQ = paramValue (Params::fmtQ);
    p.fmtMix = paramValue (Params::fmtMix);
    p.fmtKey = paramValue (Params::fmtKey);

    p.numMods = 0;

    for (const auto& slot : modMatrix.getSlots())
    {
        if (slot.source < 0 || slot.dest < 0 || p.numMods >= maxModSlots)
            continue;
        if (isGlobalModDest (slot.dest))
            continue;

        p.mods[p.numMods].source = slot.source;
        p.mods[p.numMods].dest = slot.dest;
        p.mods[p.numMods].amount = slot.amount;
        ++p.numMods;
    }

    // ---- performance macros
    MacroState macro;

    {
        const auto destX = (int) paramValue (Params::macDestX);
        const auto destY = (int) paramValue (Params::macDestY);
        auto mx = paramValue (Params::macX);
        auto my = paramValue (Params::macY);

        if (paramValue (Params::macMotion) > 0.5f)
        {
            mx = juce::jlimit (0.0f, 1.0f, 0.35f * mx + 0.5f + 0.5f * std::sin (macroPhase));
            my = juce::jlimit (0.0f, 1.0f, 0.35f * my + 0.5f + 0.5f * std::sin (macroPhase * 1.37f + 1.1f));
        }

        macro.destX = destX;
        macro.destY = destY;
        macro.x = (mx - 0.5f) * 2.0f;
        macro.y = (my - 0.5f) * 2.0f;
        macroDisplayX.store (mx);
        macroDisplayY.store (my);
    }

    float macroTilt = paramValue (Params::toneTilt);

    auto applyMacro = [&] (int dest, float amount)
    {
        switch (dest)
        {
            case 1:  p.cutoff = juce::jlimit (20.0f, 20000.0f, p.cutoff * std::exp2 (amount * 3.0f)); break;
            case 2:  p.resonance = juce::jlimit (0.0f, 1.0f, p.resonance + amount * 0.5f); break;
            case 3:  p.drive = juce::jlimit (0.0f, 1.0f, p.drive + amount * 0.5f); break;
            case 4:  p.fmtShift = juce::jlimit (-1200.0f, 1200.0f, p.fmtShift + amount * 1200.0f); break;
            case 5:  grainMixMacro = juce::jlimit (0.0f, 1.0f, grainMixMacro + amount * 0.6f); break;
            case 6:  grainPosMacro = juce::jlimit (0.0f, 1.0f, grainPosMacro + amount * 0.45f); break;
            case 7:  glitchMixMacro = juce::jlimit (0.0f, 1.0f, glitchMixMacro + amount * 0.6f); break;
            case 8:  glitchBitsMacro = juce::jlimit (-8.0f, 8.0f, glitchBitsMacro + amount * 8.0f); break;
            case 9:  revMixMacro = juce::jlimit (-0.5f, 0.5f, revMixMacro + amount * 0.45f); break;
            case 10: macroTilt = juce::jlimit (-12.0f, 12.0f, macroTilt + amount * 12.0f); break;
            default: break;
        }
    };

    applyMacro (macro.destX, macro.x);
    applyMacro (macro.destY, macro.y);

    engine.setParams (p);

    FxParams fx;
    fx.revOn = paramValue (Params::revOn) > 0.5f;
    fx.dlyOn = paramValue (Params::dlyOn) > 0.5f;
    fx.choOn = paramValue (Params::choOn) > 0.5f;
    fx.eqOn = paramValue (Params::eqOn) > 0.5f;
    fx.compOn = paramValue (Params::compOn) > 0.5f;

    fx.revMix = paramValue (Params::revMix);
    fx.revSize = paramValue (Params::revSize);
    fx.revDamp = paramValue (Params::revDamp);
    fx.revWidth = paramValue (Params::revWidth);

    const auto delaySync = paramValue (Params::dlySync) > 0.5f;
    const auto delayDiv = (int) paramValue (Params::dlyDiv);
    fx.dlyTimeSec = delaySync
                      ? (float) (syncBeats (delayDiv) * 60.0 / juce::jmax (20.0, bpm))
                      : paramValue (Params::dlyTime) * 0.001f;
    fx.dlyMix = paramValue (Params::dlyMix);
    fx.dlyFb = paramValue (Params::dlyFb);

    fx.choMix = paramValue (Params::choMix);
    fx.choRate = paramValue (Params::choRate);
    fx.choDepth = paramValue (Params::choDepth);

    fx.eqLowDb = paramValue (Params::eqLow);
    fx.eqMidDb = paramValue (Params::eqMid);
    fx.eqHighDb = paramValue (Params::eqHigh);

    fx.cmpThresh = paramValue (Params::compThresh);
    fx.cmpRatio = paramValue (Params::compRatio);
    fx.cmpMakeup = paramValue (Params::compMakeup);

    float revMod = 0.0f, dlyMod = 0.0f;
    float grainPosMod = 0.0f, grainMixMod = 0.0f, glitchMixMod = 0.0f, glitchBitsMod = 0.0f;

    for (const auto& slot : modMatrix.getSlots())
    {
        if (slot.source < 0 || slot.dest < 0)
            continue;

        const auto value = slot.amount * globalSourceValue (slot.source);
        const auto dest = (ModDest) slot.dest;

        switch (dest)
        {
            case ModDest::reverbMix:    revMod += value; break;
            case ModDest::delayMix:     dlyMod += value; break;
            case ModDest::grainPosition: grainPosMod += value; break;
            case ModDest::grainMix:     grainMixMod += value; break;
            case ModDest::glitchMix:    glitchMixMod += value; break;
            case ModDest::glitchBits:   glitchBitsMod += value; break;
            default: break;
        }
    }

    fx.revMixMod = revMod + revMixMacro;
    fx.dlyMixMod = dlyMod;
    fx.order = fxOrder;

    effectsChain.setParams (fx);

    tone.setParams (macroTilt, paramValue (Params::toneWidth), paramValue (Params::toneAir));

    // ---- granular / glitch texture
    GrainEngine::Params gr;
    gr.on = paramValue (Params::grOn) > 0.5f;
    gr.freeze = paramValue (Params::grFreeze) > 0.5f;
    gr.mix = juce::jlimit (0.0f, 1.0f, paramValue (Params::grMix) + grainMixMod + grainMixMacro);
    gr.sizeMs = paramValue (Params::grSize);
    gr.density = paramValue (Params::grDensity);
    gr.position = juce::jlimit (0.0f, 1.0f, paramValue (Params::grPosition) + grainPosMod + grainPosMacro);
    gr.spray = paramValue (Params::grSpray);
    gr.pitch = paramValue (Params::grPitch);
    gr.pitchRandom = paramValue (Params::grPitchRandom);
    gr.reverse = paramValue (Params::grReverse);
    gr.spread = paramValue (Params::grSpread);

    GlitchEngine::Params gl;
    gl.on = paramValue (Params::glOn) > 0.5f;
    gl.mix = juce::jlimit (0.0f, 1.0f, paramValue (Params::glMix) + glitchMixMod + glitchMixMacro);
    gl.bits = juce::jlimit (2.0f, 16.0f, paramValue (Params::glBits) + glitchBitsMod * 8.0f + glitchBitsMacro);
    gl.rateDiv = paramValue (Params::glRateDiv);
    gl.stepDivision = (int) paramValue (Params::glStep);
    gl.stutterDivision = (int) paramValue (Params::glStutterDiv);
    gl.stutterChance = paramValue (Params::glStutter);
    gl.gateChance = paramValue (Params::glGate);
    gl.dropChance = paramValue (Params::glDrop);
    gl.tapeChance = paramValue (Params::glTape);
    gl.repeats = paramValue (Params::glRepeats);
    gl.chaos = paramValue (Params::glChaos);
    gl.noise = paramValue (Params::glNoise);

    texture.setParams (gr, gl, bpm);
}

//==============================================================================
void BotanicAudioProcessor::engineNote (int note, float velocity, bool isOn)
{
    if (isOn)
    {
        sustainedNotes.erase (std::remove (sustainedNotes.begin(), sustainedNotes.end(), note), sustainedNotes.end());
        engine.noteOn (note, velocity);
    }
    else
    {
        if (sustainPedal)
        {
            if (std::find (sustainedNotes.begin(), sustainedNotes.end(), note) == sustainedNotes.end())
                sustainedNotes.push_back (note);
        }
        else
        {
            engine.noteOff (note);
        }
    }
}

void BotanicAudioProcessor::handleMidi (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        const auto note = msg.getNoteNumber();
        const auto vel = msg.getFloatVelocity();
        sustainedNotes.erase (std::remove (sustainedNotes.begin(), sustainedNotes.end(), note), sustainedNotes.end());
        physicallyHeld.push_back (note);
        setHeldVisual (note, true);

        if (arpeggiator.isEnabled())
            arpeggiator.noteOn (note, vel);
        else
            engine.noteOn (note, vel);
    }
    else if (msg.isNoteOff())
    {
        const auto note = msg.getNoteNumber();
        physicallyHeld.erase (std::remove (physicallyHeld.begin(), physicallyHeld.end(), note), physicallyHeld.end());
        setHeldVisual (note, false);

        if (arpeggiator.isEnabled())
            arpeggiator.noteOff (note);
        else
            engineNote (note, 0.0f, false);
    }
    else if (msg.isPitchWheel())
    {
        const auto norm = (float) (msg.getPitchWheelValue() - 8192) / 8192.0f;
        pitchBendSemis = norm * paramValue (Params::bendRange);
    }
    else if (msg.isChannelPressure())
    {
        aftertouch = (float) msg.getChannelPressureValue() / 127.0f;
    }
    else if (msg.isAftertouch())
    {
        aftertouch = (float) msg.getAfterTouchValue() / 127.0f;
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        engine.allNotesOff (true);
        arpeggiator.reset();
        sustainedNotes.clear();
        physicallyHeld.clear();
        heldMask[0].store (0);
        heldMask[1].store (0);
        sustainPedal = false;
    }
    else if (msg.isController())
    {
        const auto cc = msg.getControllerNumber();
        const auto value = (float) msg.getControllerValue() / 127.0f;

        if (cc == 1)
        {
            modWheel = value;
        }
        else if (cc == 64)
        {
            const auto on = value > 0.5f;
            if (sustainPedal && ! on)
            {
                for (auto n : sustainedNotes)
                    if (std::find (physicallyHeld.begin(), physicallyHeld.end(), n) == physicallyHeld.end())
                        engine.noteOff (n);
                sustainedNotes.clear();
            }
            sustainPedal = on;
        }
        else if (cc == 123 || cc == 120)
        {
            engine.allNotesOff (true);
            arpeggiator.reset();
            sustainedNotes.clear();
            physicallyHeld.clear();
        }
        else
        {
            const auto learnTarget = learningParamIndex.load();

            if (learnTarget >= 0)
            {
                ccToParam[(size_t) cc].store (learnTarget);
                learningParamIndex.store (-1);
                learnVersion.fetch_add (1);
            }
            else
            {
                const auto target = ccToParam[(size_t) cc].load();
                if (target >= 0)
                {
                    const auto& params = getParameters();
                    if (target < params.size())
                        params[target]->setValueNotifyingHost (value);
                }
            }
        }
    }
}

//==============================================================================
void BotanicAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto cpuStart = juce::Time::getHighResolutionTicks();

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    buffer.clear();

    if (numSamples <= 0)
        return;

    for (int ch = 2; ch < numChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (paramValue (Params::power) < 0.5f)
    {
        engine.allNotesOff (false);
        meterLevel.store (0.0f);
        peakL.store (0.0f);
        peakR.store (0.0f);
        activeVoices.store (0);
        return;
    }

    if (hardPanic.exchange (false))
    {
        engine.allNotesOff (false);
        effectsChain.reset();
        texture.reset();
        tone.reset();
        decimatorL.reset();
        decimatorR.reset();
        dcBlockL.reset();
        dcBlockR.reset();
        limiterGain = 1.0f;
        silentBlocks = 0;
    }

    if (const auto requested = resetRequest.load(); requested != lastResetSeen)
    {
        lastResetSeen = requested;
        engine.allNotesOff (true);
        effectsChain.reset();
        texture.reset();
        tone.reset();
        decimatorL.reset();
        decimatorR.reset();
        dcBlockL.reset();
        dcBlockR.reset();
        limiterGain = 1.0f;
        nonFiniteCount = 0;
    }

    double bpm = 120.0;
    bool playing = false;

    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            if (const auto b = pos->getBpm())
                bpm = *b;
            playing = pos->getIsPlaying();
        }
    }

    bpm = juce::jlimit (20.0, 300.0, bpm);
    lastBpm.store ((float) bpm);

    if (playing && ! transportWasPlaying)
    {
        arpeggiator.reset();
        globalLfoPhase[0] = globalLfoPhase[1] = 0.0f;
    }
    transportWasPlaying = playing;

    globalChaos += (randomSource.nextFloat() * 2.0f - 1.0f) * 0.06f * (float) juce::jmax (1, numSamples) / 256.0f;
    globalChaos = juce::jlimit (-1.0f, 1.0f, globalChaos * 0.999f);

    buildEngineParams();

    arpeggiator.setParams (paramValue (Params::arpOn) > 0.5f,
                           (int) paramValue (Params::arpMode),
                           (int) paramValue (Params::arpRate),
                           (int) paramValue (Params::arpOctaves),
                           paramValue (Params::arpGate),
                           paramValue (Params::arpSwing),
                           paramValue (Params::arpLatch) > 0.5f,
                           bpm);

    // ---- notes coming from the on screen / computer keyboard
    {
        auto read = queueRead.load();
        const auto write = queueWrite.load();

        if (read != write)
        {
            QueuedNote pending[noteQueueSize];
            int pendingCount = 0;

            while (read != write && pendingCount < noteQueueSize)
            {
                pending[pendingCount++] = noteQueue[(size_t) (read % noteQueueSize)];
                ++read;
            }

            queueRead.store (read);

            for (int pi = 0; pi < pendingCount; ++pi)
            {
                const auto& n = pending[pi];
                if (n.isOn != 0)
                    handleMidi (juce::MidiMessage::noteOn (1, n.note, n.velocity));
                else
                    handleMidi (juce::MidiMessage::noteOff (1, n.note));
            }
        }
    }

    events.clear();

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();

        if (msg.isNoteOnOrOff())
        {
            if (arpeggiator.isEnabled())
                handleMidi (msg);          // feeds the arp chord memory
            else
                events.push_back ({ juce::jlimit (0, numSamples, meta.samplePosition), msg });
        }
        else
        {
            events.push_back ({ juce::jlimit (0, numSamples, meta.samplePosition), msg });
        }
    }

    arpeggiator.process (numSamples, [this, numSamples] (int offset, int note, float velocity, bool isOn)
    {
        const auto msg = isOn ? juce::MidiMessage::noteOn (1, note, velocity)
                              : juce::MidiMessage::noteOff (1, note);
        events.push_back ({ juce::jlimit (0, juce::jmax (0, numSamples - 1), offset), msg });
    });

    std::stable_sort (events.begin(), events.end(),
                      [] (const TimedEvent& a, const TimedEvent& b) { return a.offset < b.offset; });

    auto* left = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : left;

    auto* voiceL = voiceBufferL.data();
    auto* voiceR = voiceBufferR.data();
    const auto voiceSamples = numSamples * 2;

    if ((int) voiceBufferL.size() < voiceSamples + 2)
    {
        voiceBufferL.assign ((size_t) voiceSamples + 8, 0.0f);
        voiceBufferR.assign ((size_t) voiceSamples + 8, 0.0f);
        voiceL = voiceBufferL.data();
        voiceR = voiceBufferR.data();
    }

    std::fill (voiceL, voiceL + voiceSamples, 0.0f);
    std::fill (voiceR, voiceR + voiceSamples, 0.0f);

    int pos = 0;

    for (const auto& ev : events)
    {
        const auto offset = juce::jlimit (0, numSamples, ev.offset);

        if (offset > pos)
        {
            engine.render (voiceL + pos * 2, voiceR + pos * 2, (offset - pos) * 2);
            pos = offset;
        }

        if (ev.message.isNoteOn())
        {
            engine.noteOn (ev.message.getNoteNumber(), ev.message.getFloatVelocity());
            visNotePulse.fetch_add (1);
        }
        else if (ev.message.isNoteOff())
            engineNote (ev.message.getNoteNumber(), 0.0f, false);
        else
            handleMidi (ev.message);
    }

    if (pos < numSamples)
        engine.render (voiceL + pos * 2, voiceR + pos * 2, (numSamples - pos) * 2);

    // band limit back to the host rate
    decimatorL.process (voiceL, left, numSamples);
    decimatorR.process (voiceR, right, numSamples);

    // if everything has died away for a while, flush the remaining tails so a
    // huge reverb or frozen grain cloud can never ring forever
    if (engine.getActiveVoiceCount() == 0 && blockPeak < 0.0015f)
        ++silentBlocks;
    else
        silentBlocks = 0;

    if (silentBlocks > (int) (getSampleRate() * 2.0 / juce::jmax (1, numSamples)))
    {
        silentBlocks = 0;
        effectsChain.reset();
        texture.reset();
        tone.reset();
        limiterGain = 1.0f;
    }

    {
        const auto srNow = getSampleRate();
        const auto rateHz = paramValue (Params::macRate);

        if (srNow > 0.0)
        {
            macroPhase += (float) (juce::MathConstants<double>::twoPi * (double) rateHz * (double) numSamples / srNow);

            if (macroPhase > juce::MathConstants<float>::twoPi * 64.0f)
                macroPhase = std::fmod (macroPhase, juce::MathConstants<float>::twoPi);
        }
    }

    texture.process (left, right, numSamples);
    textureActivity.store (texture.getActivity());

    effectsChain.process (left, right, numSamples);
    tone.process (left, right, numSamples);
    applyMasterAndLimiter (left, right, numSamples);

    // ---- visual signals for the animated artwork
    {
        double energy = 0.0, delta = 0.0;
        float prev = visPrevSample;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto v = left[i];
            energy += (double) v * v;
            const auto d = v - prev;
            delta += (double) d * d;
            prev = v;
        }

        visPrevSample = prev;

        const auto ratio = energy > 1.0e-9 ? (delta / energy) : 0.0;
        const auto bright = (float) juce::jlimit (0.0, 1.0, std::sqrt (ratio) * 0.85);
        visBright.store (visBright.load() * 0.85f + bright * 0.15f);
        visTexture.store (juce::jmax (texture.getActivity(), visTexture.load() * 0.9f));
    }

    if (const auto srCpu = getSampleRate(); srCpu > 0.0 && numSamples > 0)
    {
        const auto elapsed = juce::Time::highResolutionTicksToSeconds (
            juce::Time::getHighResolutionTicks() - cpuStart);
        const auto ratio = (float) (elapsed / ((double) numSamples / srCpu));
        cpuLoad.store (cpuLoad.load() * 0.9f + juce::jlimit (0.0f, 4.0f, ratio) * 0.1f);
    }

    const auto sr = getSampleRate();

    if (sr > 0.0)
    {
        const auto bpmNow = lastBpm.load();

        for (int i = 0; i < 2; ++i)
        {
            const auto sync = paramValue (Params::lfoId (i, "sync")) > 0.5f;
            const auto div = (int) paramValue (Params::lfoId (i, "div"));
            const auto rate = sync ? (float) ((bpmNow / 60.0) / syncBeats (div))
                                   : paramValue (Params::lfoId (i, "rate"));

            globalLfoPhase[i] += (float) ((double) rate * (double) numSamples / sr);
            globalLfoPhase[i] -= std::floor (globalLfoPhase[i]);
        }
    }

    float peakLv = 0.0f, peakRv = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        peakLv = juce::jmax (peakLv, std::abs (left[i]));
        peakRv = juce::jmax (peakRv, std::abs (right[i]));
    }

    meterLevel.store (juce::jmax (peakLv, peakRv));
    peakL.store (peakLv);
    peakR.store (peakRv);
    activeVoices.store (engine.getActiveVoiceCount());
}

float BotanicAudioProcessor::globalLfoValue (int index) const
{
    const auto phase = globalLfoPhase[juce::jlimit (0, 1, index)];
    const auto shape = (int) paramValue (Params::lfoId (index, "shape"));

    switch (shape)
    {
        case 1:  return std::sin (phase * juce::MathConstants<float>::twoPi);
        case 2:  return 2.0f * phase - 1.0f;
        case 3:  return 1.0f - 2.0f * phase;
        case 4:  return phase < 0.5f ? 1.0f : -1.0f;
        case 7:  return std::sin (phase * 37.0f) * std::cos (phase * 11.0f);
        case 8:  return phase < 0.18f ? 1.0f - phase * 4.5f : -0.15f;
        case 0:
        default: return 4.0f * std::abs (phase - 0.5f) - 1.0f;
    }
}

float BotanicAudioProcessor::globalSourceValue (int source) const
{
    switch ((ModSource) source)
    {
        case ModSource::lfo1:       return globalLfoValue (0) * paramValue (Params::lfoId (0, "depth"));
        case ModSource::lfo2:       return globalLfoValue (1) * paramValue (Params::lfoId (1, "depth"));
        case ModSource::modWheel:   return modWheel;
        case ModSource::aftertouch: return aftertouch;
        case ModSource::chaos:      return globalChaos;
        case ModSource::velocity:   return 0.85f;
        case ModSource::random:     return globalChaos * 0.6f;
        default:                    return 0.0f;
    }
}

void BotanicAudioProcessor::applyMasterAndLimiter (float* left, float* right, int numSamples)
{
    const auto gain = dbToGain (paramValue (Params::master)) * dbToGain (paramValue (Params::output));
    constexpr float ceiling = 0.95f;
    blockPeak = 0.0f;
    constexpr float hardLimit = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        auto l = left[i] * gain;
        auto r = right[i] * gain;

        // never let a non-finite value poison the state (it would stick forever)
        if (! std::isfinite (l) || ! std::isfinite (r))
        {
            l = 0.0f;
            r = 0.0f;

            if (++nonFiniteCount < 512)
                dcBlockL.reset(), dcBlockR.reset(), limiterGain = 1.0f;
        }
        else
        {
            nonFiniteCount = 0;
        }

        l = dcBlockL.process (l);
        r = dcBlockR.process (r);

        const auto peak = juce::jmax (std::abs (l), std::abs (r));
        const auto target = peak > ceiling ? ceiling / peak : 1.0f;
        const auto coeff = target < limiterGain ? limiterAttack : limiterRelease;
        limiterGain += (target - limiterGain) * coeff;

        if (! std::isfinite (limiterGain) || limiterGain <= 0.0f)
            limiterGain = 1.0f;

        limiterGain = juce::jlimit (0.05f, 1.0f, limiterGain);

        left[i] = juce::jlimit (-hardLimit, hardLimit, softClip (l * limiterGain));
        right[i] = juce::jlimit (-hardLimit, hardLimit, softClip (r * limiterGain));
        blockPeak = juce::jmax (blockPeak, std::abs (left[i]), std::abs (right[i]));
    }
}

//==============================================================================
std::array<juce::uint64, 2> BotanicAudioProcessor::getNoteMask() const
{
    return engine.getNoteMask();
}

void BotanicAudioProcessor::setHeldVisual (int note, bool isOn) noexcept
{
    if (note < 0 || note > 127)
        return;

    const auto index = (size_t) (note / 64);
    const auto bit = (juce::uint64) 1 << (note % 64);
    auto current = heldMask[index].load();

    while (true)
    {
        const auto next = isOn ? (current | bit) : (current & ~bit);

        if (heldMask[index].compare_exchange_weak (current, next))
            break;
    }
}

void BotanicAudioProcessor::keyboardNoteOn (int note, float velocity)
{
    // instant visual feedback, audio thread picks the note up on the next block
    setHeldVisual (note, true);

    const auto write = queueWrite.load();
    noteQueue[(size_t) (write % noteQueueSize)] = { note, velocity, 1 };
    queueWrite.store (write + 1);
}

void BotanicAudioProcessor::keyboardNoteOff (int note)
{
    setHeldVisual (note, false);

    const auto write = queueWrite.load();
    noteQueue[(size_t) (write % noteQueueSize)] = { note, 0.0f, 0 };
    queueWrite.store (write + 1);
}

//==============================================================================
std::array<int, 5> BotanicAudioProcessor::getFxOrder() const
{
    return fxOrder;
}

void BotanicAudioProcessor::moveFxSlot (int from, int to)
{
    if (from < 0 || from > 4 || to < 0 || to > 4 || from == to)
        return;

    if (from < to)
        std::rotate (fxOrder.begin() + from, fxOrder.begin() + from + 1, fxOrder.begin() + to + 1);
    else
        std::rotate (fxOrder.begin() + to, fxOrder.begin() + from, fxOrder.begin() + from + 1);
}

bool BotanicAudioProcessor::isFxOn (int slot) const
{
    switch (slot)
    {
        case 0: return paramValue (Params::revOn) > 0.5f;
        case 1: return paramValue (Params::dlyOn) > 0.5f;
        case 2: return paramValue (Params::choOn) > 0.5f;
        case 3: return paramValue (Params::eqOn) > 0.5f;
        case 4: return paramValue (Params::compOn) > 0.5f;
        default: break;
    }
    return false;
}

void BotanicAudioProcessor::setFxOn (int slot, bool shouldBeOn)
{
    const char* id = nullptr;

    switch (slot)
    {
        case 0: id = Params::revOn; break;
        case 1: id = Params::dlyOn; break;
        case 2: id = Params::choOn; break;
        case 3: id = Params::eqOn; break;
        case 4: id = Params::compOn; break;
        default: break;
    }

    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (shouldBeOn ? 1.0f : 0.0f);
}

//==============================================================================
void BotanicAudioProcessor::captureStateInto (juce::ValueTree& state) const
{
    state.removeChild (state.getChildWithName ("MODMATRIX"), nullptr);
    modMatrix.toValueTree (state);

    for (int i = 0; i < 5; ++i)
        state.setProperty ("fxSlot" + juce::String (i), fxOrder[(size_t) i], nullptr);

    state.setProperty ("language", language.load(), nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("presetIndex", presetIndex.load(), nullptr);
    state.setProperty ("version", 1, nullptr);
}

void BotanicAudioProcessor::reloadFromState (const juce::ValueTree& state, bool updateName)
{
    apvts.replaceState (state);
    modMatrix.fromValueTree (state);

    const auto defaultOrder = std::array<int, 5> { 0, 1, 2, 3, 4 };
    fxOrder = defaultOrder;

    for (int i = 0; i < 5; ++i)
    {
        const auto v = (int) state.getProperty ("fxSlot" + juce::String (i), i);
        if (v >= 0 && v <= 4)
            fxOrder[(size_t) i] = v;
    }

    if (updateName)
        currentPresetName = state.getProperty ("presetName", currentPresetName).toString();

    language.store ((int) state.getProperty ("language", 0));

    const auto idx = (int) state.getProperty ("presetIndex", -1);
    if (idx >= 0 && idx < presetNames.size())
        presetIndex.store (idx);

    requestDspReset();
}

void BotanicAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    captureStateInto (state);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BotanicAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            reloadFromState (state, true);
            takeUndoSnapshot (true);
        }
    }
}

//==============================================================================
juce::uint64 BotanicAudioProcessor::stateHash() const
{
    // cheap change detector so the 2 Hz snapshot timer does not serialise XML
    juce::uint64 h = 1469598103934665603ull;

    auto mix = [&h] (juce::uint32 v)
    {
        h ^= v;
        h *= 1099511628211ull;
    };

    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            mix ((juce::uint32) (rp->getValue() * 1.0e7f));

    for (const auto& slot : modMatrix.getSlots())
    {
        mix ((juce::uint32) (slot.source * 131 + slot.dest * 17 + 1));
        mix ((juce::uint32) (slot.amount * 1.0e6f + 100000.0f));
    }

    for (int i = 0; i < 5; ++i)
        mix ((juce::uint32) (fxOrder[(size_t) i] * 7 + i));

    mix ((juce::uint32) (presetIndex.load() * 13));
    return h;
}

void BotanicAudioProcessor::takeUndoSnapshot (bool force)
{
    const auto hash = stateHash();

    if (! force && hash == lastUndoHash)
        return;

    lastUndoHash = hash;

    auto state = apvts.copyState();
    captureStateInto (state);

    const auto xml = state.toXmlString();

    if (! force && xml == lastUndoXml)
        return;

    lastUndoXml = xml;
    undoStack.push_back (state);

    while (undoStack.size() > 40)
        undoStack.pop_front();

    redoStack.clear();
}

void BotanicAudioProcessor::undo()
{
    if (undoStack.size() < 2)
        return;

    redoStack.push_back (undoStack.back());
    undoStack.pop_back();

    const auto state = undoStack.back();
    reloadFromState (state, true);
    lastUndoXml = state.toXmlString();
}

void BotanicAudioProcessor::redo()
{
    if (redoStack.empty())
        return;

    auto state = apvts.copyState();
    captureStateInto (state);
    undoStack.push_back (state);

    const auto next = redoStack.back();
    redoStack.pop_back();

    reloadFromState (next, true);
    lastUndoXml = next.toXmlString();
}

//==============================================================================
void BotanicAudioProcessor::refreshPresetList()
{
    presetNames = factoryPresetNames();
    userPresetFiles.clear();

    const auto dir = getUserPresetDirectory();

    if (dir.isDirectory())
    {
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.botanic"))
        {
            userPresetFiles.add (f);
            presetNames.add (f.getFileNameWithoutExtension());
        }
    }
}

juce::StringArray BotanicAudioProcessor::getPresetNames() const
{
    return presetNames;
}

juce::String BotanicAudioProcessor::getPresetName() const
{
    return currentPresetName;
}

void BotanicAudioProcessor::setPresetIndex (int index)
{
    applyPresetInternal (index, true);
}

void BotanicAudioProcessor::initPatch()
{
    takeUndoSnapshot (true);

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    modMatrix.clear();
    currentPresetName = "Init";
    presetIndex.store (-1);

    if (onPresetChanged != nullptr)
    {
        auto cb = onPresetChanged;
        juce::MessageManager::callAsync ([cb] { if (cb) cb(); });
    }
}

bool BotanicAudioProcessor::savePresetAs (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());

    if (clean.isEmpty())
        return false;

    const auto dir = getUserPresetDirectory();
    dir.createDirectory();

    auto state = apvts.copyState();
    captureStateInto (state);
    state.setProperty ("presetName", clean, nullptr);

    auto xml = state.createXml();

    if (xml == nullptr)
        return false;

    const auto file = dir.getChildFile (clean + ".botanic");

    if (! xml->writeTo (file))
        return false;

    refreshPresetList();
    currentPresetName = clean;

    for (int i = 0; i < presetNames.size(); ++i)
        if (presetNames[i] == clean)
            presetIndex.store (i);

    if (onPresetChanged != nullptr)
    {
        auto cb = onPresetChanged;
        juce::MessageManager::callAsync ([cb] { if (cb) cb(); });
    }

    return true;
}

//==============================================================================
void BotanicAudioProcessor::startMidiLearn (const juce::String& paramId)
{
    const auto& params = getParameters();

    for (int i = 0; i < params.size(); ++i)
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[i]);
        if (ranged != nullptr && ranged->paramID == paramId)
        {
            learningParamIndex.store (i);
            learningName = paramId;
            return;
        }
    }
}

juce::String BotanicAudioProcessor::getMappedCC (const juce::String& paramId) const
{
    const auto& params = getParameters();
    int index = -1;

    for (int i = 0; i < params.size(); ++i)
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[i]);
        if (ranged != nullptr && ranged->paramID == paramId)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return {};

    for (int cc = 0; cc < 128; ++cc)
        if (ccToParam[(size_t) cc].load() == index)
            return "CC " + juce::String (cc);

    return {};
}

void BotanicAudioProcessor::clearMidiMapping (const juce::String& paramId)
{
    const auto& params = getParameters();
    int index = -1;

    for (int i = 0; i < params.size(); ++i)
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[i]);
        if (ranged != nullptr && ranged->paramID == paramId)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return;

    for (int cc = 0; cc < 128; ++cc)
        if (ccToParam[(size_t) cc].load() == index)
            ccToParam[(size_t) cc].store (-1);

    learnVersion.fetch_add (1);
}

//==============================================================================
juce::ValueTree BotanicAudioProcessor::getFullState()
{
    auto state = apvts.copyState();
    captureStateInto (state);
    return state;
}

void BotanicAudioProcessor::setFullState (const juce::ValueTree& state)
{
    if (state.isValid())
        reloadFromState (state, true);
}

void BotanicAudioProcessor::setLanguage (int value)
{
    language.store (juce::jlimit (0, 1, value));
}

void BotanicAudioProcessor::randomize (int scope)
{
    takeUndoSnapshot (true);

    auto& r = randomSource;
    auto randF = [&r] (float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); };
    auto pickIndex = [&r] (int count) { return r.nextInt (juce::jmax (1, count)); };

    auto setF = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    auto setC = [this] (const juce::String& id, int idx)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
    };

    auto setB = [this] (const juce::String& id, bool b)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (b ? 1.0f : 0.0f);
    };

    const auto all = (scope == 0);

    if (all || scope == 1)
    {
        for (int i = 0; i < 3; ++i)
        {
            setB (Params::oscId (i, "on"), i == 0 || r.nextFloat() < 0.6f);
            setC (Params::oscId (i, "table"), pickIndex (8));
            setF (Params::oscId (i, "pos"), randF (0.0f, 1.0f));
            setF (Params::oscId (i, "tune"), (float) juce::jlimit (-24, 24, (int) (randF (-13.0f, 13.0f)) * 12 / 12));
            setF (Params::oscId (i, "detune"), randF (-28.0f, 28.0f));
            setF (Params::oscId (i, "level"), i == 0 ? randF (0.7f, 1.0f) : randF (0.2f, 0.85f));
            setF (Params::oscId (i, "uni"), (float) (1 + r.nextInt (6)));
            setF (Params::oscId (i, "spread"), randF (0.0f, 1.0f));
            setF (Params::oscId (i, "pan"), randF (-0.7f, 0.7f));
            setF (Params::oscId (i, "phase"), r.nextFloat());
        }
    }

    if (all || scope == 2)
    {
        setC (Params::filterType, pickIndex (5));
        setF (Params::cutoff, std::exp (randF (std::log (320.0f), std::log (7500.0f))));
        setF (Params::resonance, randF (0.0f, 0.62f));
        setF (Params::drive, randF (0.0f, 0.5f));
        setF (Params::filterEnv, randF (-0.15f, 0.8f));
        setF (Params::keyTrack, randF (0.0f, 0.8f));

        for (int e = 0; e < 3; ++e)
        {
            setF (Params::envId (e, "A"), randF (0.001f, 1.1f));
            setF (Params::envId (e, "D"), randF (0.05f, 1.8f));
            setF (Params::envId (e, "S"), randF (e == 0 ? 0.25f : 0.0f, 1.0f));
            setF (Params::envId (e, "R"), randF (0.1f, 2.6f));
        }
    }

    if (all || scope == 3)
    {
        modMatrix.clear();

        const int count = 2 + r.nextInt (3);
        static const int sourcePool[] = { 0, 0, 1, 1, 2, 3, 4, 5, 7, 9, 9 };
        static const int destPool[] = { 0, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17 };

        for (int i = 0; i < count; ++i)
        {
            const auto source = sourcePool[pickIndex ((int) numElementsInArray (sourcePool))];
            const auto dest = destPool[pickIndex ((int) numElementsInArray (destPool))];
            modMatrix.set (source, dest, randF (-0.75f, 0.9f));
        }
    }

    if (all || scope == 4)
    {
        setB (Params::revOn, r.nextFloat() < 0.8f);
        setF (Params::revMix, randF (0.05f, 0.6f));
        setF (Params::revSize, randF (0.35f, 1.0f));
        setF (Params::revDamp, r.nextFloat());
        setF (Params::revWidth, randF (0.5f, 1.0f));

        setB (Params::dlyOn, r.nextFloat() < 0.7f);
        setF (Params::dlyMix, randF (0.0f, 0.45f));
        setF (Params::dlyFb, randF (0.1f, 0.62f));
        setC (Params::dlyDiv, pickIndex (11));

        setB (Params::choOn, r.nextFloat() < 0.7f);
        setF (Params::choMix, randF (0.0f, 0.55f));
        setF (Params::choRate, randF (0.1f, 2.5f));
        setF (Params::choDepth, r.nextFloat());

        setB (Params::eqOn, r.nextFloat() < 0.35f);
        setF (Params::eqLow, randF (-4.0f, 4.0f));
        setF (Params::eqMid, randF (-4.0f, 4.0f));
        setF (Params::eqHigh, randF (-4.0f, 4.0f));

        setB (Params::compOn, r.nextFloat() < 0.3f);
        setF (Params::compThresh, randF (-32.0f, -8.0f));
        setF (Params::compRatio, randF (1.5f, 6.0f));
    }

    if (all || scope == 5)
    {
        setB (Params::grOn, r.nextFloat() < 0.7f);
        setB (Params::grFreeze, r.nextFloat() < 0.25f);
        setF (Params::grMix, randF (0.25f, 0.85f));
        setF (Params::grSize, randF (25.0f, 320.0f));
        setF (Params::grDensity, randF (3.0f, 45.0f));
        setF (Params::grPosition, r.nextFloat());
        setF (Params::grSpray, r.nextFloat());
        setF (Params::grPitch, (float) (12 * (int) randF (-2.0f, 2.0f)));
        setF (Params::grPitchRandom, randF (0.0f, 5.0f));
        setF (Params::grReverse, r.nextFloat() * 0.6f);
        setF (Params::grSpread, randF (0.3f, 1.0f));

        setB (Params::glOn, r.nextFloat() < 0.55f);
        setF (Params::glMix, randF (0.3f, 0.85f));
        setF (Params::glBits, (float) (4 + r.nextInt (9)));
        setF (Params::glRateDiv, (float) (1 + r.nextInt (6)));
        setC (Params::glStep, r.nextInt (7));
        setC (Params::glStutterDiv, pickIndex (7));
        setF (Params::glStutter, r.nextFloat() * 0.65f);
        setF (Params::glGate, r.nextFloat() * 0.3f);
        setF (Params::glDrop, r.nextFloat() * 0.25f);
        setF (Params::glTape, r.nextFloat() * 0.22f);
        setF (Params::glRepeats, (float) (1 + r.nextInt (6)));
        setF (Params::glChaos, randF (0.35f, 1.0f));
        setF (Params::glNoise, r.nextFloat() * 0.12f);
    }

    if (all || scope == 6)
    {
        setB (Params::arpOn, r.nextFloat() < 0.4f);
        setC (Params::arpMode, pickIndex (6));
        setC (Params::arpRate, pickIndex (6));
        setF (Params::arpOctaves, (float) (1 + r.nextInt (3)));
        setF (Params::arpGate, randF (0.2f, 0.9f));
        setF (Params::arpSwing, r.nextFloat() * 0.4f);
        setB (Params::arpLatch, r.nextFloat() < 0.3f);
    }

    if (onPresetChanged != nullptr)
    {
        auto cb = onPresetChanged;
        juce::MessageManager::callAsync ([cb] { if (cb) cb(); });
    }
}

//==============================================================================
juce::AudioProcessorEditor* BotanicAudioProcessor::createEditor()
{
    return new BotanicAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BotanicAudioProcessor();
}
