#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <deque>
#include <unordered_map>

#include "Params.h"
#include "ModMatrix.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "dsp/Effects.h"
#include "dsp/Arpeggiator.h"
#include "dsp/Texture.h"
#include "dsp/Oversampling.h"

class BotanicAudioProcessor : public juce::AudioProcessor
{
public:
    BotanicAudioProcessor();
    ~BotanicAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Botanic"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return juce::jmax (1, getPresetNames().size()); }
    int getCurrentProgram() override { return juce::jmax (0, getPresetIndex()); }
    void setCurrentProgram (int index) override { setPresetIndex (index); }
    const juce::String getProgramName (int index) override
    {
        const auto names = getPresetNames();
        return juce::isPositiveAndBelow (index, names.size()) ? names[index] : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    botanic::ModMatrix modMatrix;

    botanic::Engine engine;
    botanic::EffectsChain effectsChain;
    botanic::Arpeggiator arpeggiator;
    botanic::TextureProcessor texture;
    botanic::ToneStage tone;

    float paramValue (const juce::String& id) const;
    juce::RangedAudioParameter* param (const juce::String& id) const;
    bool isParamModulated (const juce::String& id) const;

    //==============================================================================
    juce::StringArray getPresetNames() const;
    int getPresetIndex() const noexcept { return presetIndex.load(); }
    juce::String getPresetName() const;
    void setPresetIndex (int index);
    void nextPreset() { setPresetIndex (getPresetIndex() + 1); }
    void previousPreset() { setPresetIndex (getPresetIndex() - 1); }
    void initPatch();
    bool savePresetAs (const juce::String& name);
    void refreshPresetList();
    juce::File getUserPresetDirectory() const;

    //==============================================================================
    std::array<juce::uint64, 2> getNoteMask() const;
    int getActiveVoiceCount() const noexcept { return activeVoices.load(); }
    float getMeterLevel() const noexcept { return meterLevel.load(); }
    float getPeakL() const noexcept { return peakL.load(); }
    float getPeakR() const noexcept { return peakR.load(); }
    float getTempo() const noexcept { return lastBpm.load(); }

    /** UI-thread friendly note trigger: the event is queued for the audio thread. */
    void keyboardNoteOn (int note, float velocity);
    void keyboardNoteOff (int note);

    /** Notes physically held down (screen keyboard, computer keyboard, MIDI in). */
    std::array<juce::uint64, 2> getHeldNotes() const noexcept
    {
        return { heldMask[0].load(), heldMask[1].load() };
    }

    void setHeldVisual (int note, bool isOn) noexcept;
    void clearHeldVisual() noexcept { heldMask[0].store (0); heldMask[1].store (0); }

    //==============================================================================
    int getLanguage() const noexcept { return language.load(); }
    void setLanguage (int value);

    void setModAmount (int source, int dest, float amount) { modMatrix.set (source, dest, amount); }

    /** Full plugin state, used by the A/B compare buttons. */
    juce::ValueTree getFullState();
    void setFullState (const juce::ValueTree& state);

    /** Clears all internal DSP tails on the audio thread (used on program changes). */
    void requestDspReset() { resetRequest.fetch_add (1); }

    /** Hard stop: kills every voice and clears all tails immediately. */
    void panic() { hardPanic.store (true); }
    float getModAmount (int source, int dest) const { return modMatrix.get (source, dest); }

    /** Manual glitch trigger used by the texture page. */
    void triggerGlitch() { texture.trigger(); }

    /** Generative randomisation. scope: 0 = all, 1 = osc, 2 = filter/env, 3 = mod, 4 = fx, 5 = texture, 6 = arp */
    void randomize (int scope = 0);
    float getTextureActivity() const noexcept { return textureActivity.load(); }
    float getCpuLoad() const noexcept { return cpuLoad.load(); }
    float getVisualBrightness() const noexcept { return visBright.load(); }
    float getVisualTexture() const noexcept { return visTexture.load(); }
    int getNotePulse() const noexcept { return visNotePulse.load(); }
    float getMacroDisplayX() const noexcept { return macroDisplayX.load(); }
    float getMacroDisplayY() const noexcept { return macroDisplayY.load(); }

    //==============================================================================
    std::array<int, 5> getFxOrder() const;
    void moveFxSlot (int from, int to);
    bool isFxOn (int slot) const;
    void setFxOn (int slot, bool shouldBeOn);

    //==============================================================================
    void takeUndoSnapshot (bool force = false);
    juce::uint64 stateHash() const;
    void undo();
    void redo();
    bool canUndo() const noexcept { return undoStack.size() > 1; }
    bool canRedo() const noexcept { return ! redoStack.empty(); }
    int getUndoDepth() const noexcept { return (int) undoStack.size(); }

    //==============================================================================
    void startMidiLearn (const juce::String& paramId);
    void cancelMidiLearn() { learningParamIndex.store (-1); }
    juce::String getLearningParam() const { return learningName; }
    juce::String getMappedCC (const juce::String& paramId) const;
    void clearMidiMapping (const juce::String& paramId);
    int getLearnVersion() const noexcept { return learnVersion.load(); }

    std::function<void()> onPresetChanged;

private:
    void applyPresetInternal (int index, bool pushUndo);
    void buildEngineParams();
    void handleMidi (const juce::MidiMessage& msg);
    void handleIncomingNote (const juce::MidiMessage& msg);
    void engineNote (int note, float velocity, bool isOn);
    void applyMasterAndLimiter (float* left, float* right, int numSamples);
    float globalLfoValue (int index) const;
    float globalSourceValue (int source) const;
    void captureStateInto (juce::ValueTree& state) const;
    void reloadFromState (const juce::ValueTree& state, bool updateName);

    std::unordered_map<juce::String, std::atomic<float>*> paramPointers;

    struct TimedEvent { int offset; juce::MidiMessage message; };
    std::vector<TimedEvent> events;

    float globalLfoPhase[2] { 0.0f, 0.0f };

    std::atomic<int> activeVoices { 0 };
    std::atomic<float> meterLevel { 0.0f }, peakL { 0.0f }, peakR { 0.0f };
    std::atomic<int> presetIndex { 0 };
    std::atomic<float> lastBpm { 120.0f };
    std::atomic<float> textureActivity { 0.0f };
    std::atomic<float> macroDisplayX { 0.5f }, macroDisplayY { 0.5f };
    std::atomic<float> cpuLoad { 0.0f };
    std::atomic<float> visBright { 0.35f }, visTexture { 0.0f };
    std::atomic<int> visNotePulse { 0 };
    std::atomic<int> resetRequest { 0 };
    std::atomic<bool> hardPanic { false };
    float blockPeak = 0.0f;
    int silentBlocks = 0;
    float visPrevSample = 0.0f;
    int lastResetSeen = 0;
    int nonFiniteCount = 0;
    std::atomic<juce::uint64> heldMask[2] { { 0 }, { 0 } };
    std::atomic<int> language { 0 };

    struct QueuedNote { int note = 0; float velocity = 0.0f; int isOn = 0; };
    static constexpr int noteQueueSize = 64;
    std::array<QueuedNote, noteQueueSize> noteQueue {};
    std::atomic<int> queueWrite { 0 }, queueRead { 0 };
    float globalChaos = 0.0f;
    float macroPhase = 0.0f;

    struct MacroState { int destX = 0, destY = 0; float x = 0.0f, y = 0.0f; };
    juce::Random randomSource;

    juce::StringArray presetNames;
    juce::Array<juce::File> userPresetFiles;

    std::array<int, 5> fxOrder { 0, 1, 2, 3, 4 };

    std::deque<juce::ValueTree> undoStack, redoStack;
    juce::String lastUndoXml;
    juce::uint64 lastUndoHash = 0;
    juce::String currentPresetName { "Lush Atmosphere" };

    float pitchBendSemis = 0.0f;
    float modWheel = 0.0f, aftertouch = 0.0f;
    bool sustainPedal = false;
    std::vector<int> sustainedNotes;      // notes held by the pedal
    std::vector<int> physicallyHeld;      // notes currently down (for sustain logic)

    std::atomic<int> learningParamIndex { -1 };
    std::atomic<int> learnVersion { 0 };
    int lastLearnVersion = 0;
    juce::String learningName;
    std::array<std::atomic<int>, 128> ccToParam;

    bool transportWasPlaying = false;

    juce::AudioBuffer<float> renderBuffer;

    // the voice engine runs at twice the host rate, then gets band limited
    std::vector<float> voiceBufferL, voiceBufferR;
    botanic::HalfBandDecimator decimatorL, decimatorR;
    botanic::DcBlocker dcBlockL, dcBlockR;
    float limiterGain = 1.0f;
    double limiterAttack = 0.0, limiterRelease = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BotanicAudioProcessor)
};
