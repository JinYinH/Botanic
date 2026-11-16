#include <JuceHeader.h>

#define LOG(...) do { std::printf (__VA_ARGS__); std::fflush (stdout); } while (0)

/**
    Loads the built VST3 into JUCE's plugin hosting layer, exactly like a DAW does,
    renders a chord, and reports whether the plugin responds with audio.
*/
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    if (argc < 2)
    {
        LOG ("usage: BotanicHostTest <path to Botanic.vst3>\n");
        return 2;
    }

    const juce::File pluginFile { juce::String (argv[1]) };
    LOG ("loading %s\n", pluginFile.getFullPathName().toRawUTF8());

    if (! pluginFile.exists())
    {
        LOG ("file does not exist\n");
        return 1;
    }

    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    juce::OwnedArray<juce::PluginDescription> descriptions;

    for (auto* format : formatManager.getFormats())
    {
        if (format->getName() != "VST3")
            continue;

        juce::OwnedArray<juce::PluginDescription> found;
        format->findAllTypesForFile (found, pluginFile.getFullPathName());

        for (auto* d : found)
        {
            LOG ("  found: %s (%s) by %s, category=%s\n",
                         d->name.toRawUTF8(), d->descriptiveName.toRawUTF8(),
                         d->manufacturerName.toRawUTF8(), d->category.toRawUTF8());
            descriptions.add (new juce::PluginDescription (*d));
        }
    }

    if (descriptions.isEmpty())
    {
        LOG ("FAILED: no plugin types found\n");
        return 1;
    }

    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> instance;

    try
    {
        instance.reset (formatManager.createPluginInstance (*descriptions[0], 48000.0, 128, error).release());
    }
    catch (const std::exception& e)
    {
        LOG ("exception during instantiate: %s\n", e.what());
        return 1;
    }
    catch (...)
    {
        LOG ("unknown exception during instantiate\n");
        return 1;
    }

    if (instance == nullptr)
    {
        LOG ("FAILED to instantiate: %s\n", error.toRawUTF8());
        return 1;
    }

    LOG ("instantiated '%s'\n", instance->getName().toRawUTF8());
    LOG ("  inputs=%d outputs=%d midiIn=%d midiOut=%d\n",
                 instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels(),
                 (int) instance->acceptsMidi(), (int) instance->producesMidi());
    LOG ("  programs=%d\n", instance->getNumPrograms());

    for (int i = 0; i < juce::jmin (instance->getNumPrograms(), 20); ++i)
        LOG ("    %2d: %s\n", i, instance->getProgramName (i).toRawUTF8());

    instance->enableAllBuses();
    instance->setPlayConfigDetails (0, 2, 48000.0, 128);
    instance->prepareToPlay (48000.0, 128);

    juce::AudioBuffer<float> buffer (2, 128);
    juce::MidiBuffer midi;

    float peak = 0.0f;
    bool nan = false;

    for (int block = 0; block < 1000; ++block)
    {
        buffer.clear();
        midi.clear();

        if (block == 0)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.9f), 0);
        }

        instance->processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto v = buffer.getSample (ch, i);

                if (! std::isfinite (v))
                    nan = true;

                peak = juce::jmax (peak, std::abs (v));
            }
    }

    LOG ("rendered %d blocks, peak=%.4f%s\n", 1000, peak, nan ? " (NaN detected!)" : "");

    // editor
    if (instance->hasEditor())
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (instance->createEditorIfNeeded());

        if (editor != nullptr)
        {
            LOG ("editor created: %d x %d\n", editor->getWidth(), editor->getHeight());
        }
        else
        {
            LOG ("FAILED to create editor\n");
        }
    }

    // state round trip
    juce::MemoryBlock state;
    instance->getStateInformation (state);
    LOG ("state size = %d bytes\n", (int) state.getSize());

    if (state.getSize() > 0)
    {
        instance->setStateInformation (state.getData(), (int) state.getSize());
        juce::MemoryBlock second;
        instance->getStateInformation (second);
        LOG ("state after restore = %d bytes\n", (int) second.getSize());
    }

    instance->releaseResources();

    const auto ok = peak > 0.001f && ! nan && state.getSize() > 0;

    LOG ("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
