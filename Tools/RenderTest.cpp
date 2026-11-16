#include <JuceHeader.h>

#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"
#include <set>

/**
    Offline render harness.

    Renders a few seconds for every factory preset, checks for NaN / clipping and
    optionally writes the result as a WAV file next to the executable.
*/
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    if (argc > 1 && juce::String (argv[1]) == "--dump")
    {
        const auto& bank = botanic::WavetableBank::instance();
        juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile ("table_dump.txt");
        out.deleteFile();
        juce::StringArray lines;

        for (int t = 0; t < 8; ++t)
        {
            const auto& table = bank.getTable (t);
            for (int mip : { 0, 4 })
            {
                juce::String line ("T" + juce::String (t) + "M" + juce::String (mip) + ":");
                const auto* f = table.frame (8, mip);
                for (int i = 0; i < 2048; ++i)
                    line << " " << juce::String (f[i], 5);
                lines.add (line);
            }
        }

        out.replaceWithText (lines.joinIntoString ("\n"));
        std::printf ("dumped to %s\n", out.getFullPathName().toRawUTF8());
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--worst")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor wp;
        wp.setPlayConfigDetails (0, 2, sRate, sBlock);
        wp.prepareToPlay (sRate, sBlock);
        wp.initPatch();

        auto set = [&wp] (const juce::String& id, float v)
        {
            if (auto* p = wp.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        for (int i = 0; i < 3; ++i)
        {
            set (Params::oscId (i, "on"), 1.0f);
            set (Params::oscId (i, "uni"), 8.0f);
            set (Params::oscId (i, "level"), 0.8f);
        }

        set (Params::maxVoices, 16.0f);
        set (Params::fmtOn, 1.0f);
        set (Params::fmtMix, 0.8f);
        set (Params::grOn, 1.0f);
        set (Params::grMix, 0.5f);
        set (Params::glOn, 1.0f);
        set (Params::glMix, 0.5f);
        set (Params::revOn, 1.0f);
        set (Params::dlyOn, 1.0f);
        set (Params::choOn, 1.0f);

        juce::AudioBuffer<float> block (2, sBlock);
        juce::MidiBuffer m;

        for (int k = 0; k < 12; ++k)
            m.addEvent (juce::MidiMessage::noteOn (1, 36 + k * 3, 0.9f), 0);

        double maxMs = 0.0, sumMs = 0.0;
        int n = 0;

        for (int b = 0; b < 600; ++b)
        {
            block.clear();
            const auto t0 = juce::Time::getHighResolutionTicks();
            wp.processBlock (block, m);
            const auto ms = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
            m.clear();

            if (b > 100)
            {
                maxMs = juce::jmax (maxMs, ms);
                sumMs += ms;
                ++n;
            }
        }

        const auto budget = (double) sBlock / sRate * 1000.0;
        std::printf ("worst case (3 osc x 8 unison, 12 notes, formant+grain+glitch+fx):\n");
        std::printf ("  max %.2f ms  avg %.2f ms  budget %.2f ms  => max %.0f%% / avg %.0f%%\n",
                     maxMs, sumMs / juce::jmax (1, n), budget, maxMs / budget * 100.0, sumMs / juce::jmax (1, n) / budget * 100.0);
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--silence")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor zp;
        zp.setPlayConfigDetails (0, 2, sRate, sBlock);
        zp.prepareToPlay (sRate, sBlock);

        juce::AudioBuffer<float> block (2, sBlock);
        const auto names = botanic::factoryPresetNames();
        int noisy = 0;

        for (int preset = 0; preset < names.size(); ++preset)
        {
            zp.setPresetIndex (preset);

            double peak = 0.0;
            juce::MidiBuffer none;

            for (int b = 0; b < (int) (sRate * 1.5 / sBlock); ++b)
            {
                block.clear();
                zp.processBlock (block, none);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block.getNumSamples(); ++i)
                        peak = juce::jmax (peak, (double) std::abs (block.getSample (ch, i)));
            }

            if (peak > 1.0e-4)
            {
                ++noisy;
                std::printf ("%-18s SILENT OUTPUT peak %.5f\n", names[preset].toRawUTF8(), peak);
            }
        }

        std::printf ("presets making noise with no notes: %d / %d\n", noisy, names.size());
        return noisy == 0 ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--clicks")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor kp;
        kp.setPlayConfigDetails (0, 2, sRate, sBlock);
        kp.prepareToPlay (sRate, sBlock);

        juce::AudioBuffer<float> block (2, sBlock);
        const auto names = botanic::factoryPresetNames();

        for (int preset = 0; preset < names.size(); ++preset)
        {
            kp.setPresetIndex (preset);

            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, 45, 0.9f), 0);
            m.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);

            double peak = 0.0;
            int clicks = 0;
            float prevL = 0.0f, prevR = 0.0f;

            for (int b = 0; b < (int) (sRate * 6.0 / sBlock); ++b)
            {
                block.clear();
                kp.processBlock (block, m);
                m.clear();

                for (int i = 0; i < sBlock; ++i)
                {
                    const auto l = block.getSample (0, i);
                    const auto r = block.getSample (1, i);
                    peak = juce::jmax (peak, (double) std::abs (l));

                    if (std::abs (l - prevL) > 0.12f || std::abs (r - prevR) > 0.12f)
                        ++clicks;

                    prevL = l;
                    prevR = r;
                }
            }

            std::printf ("%-18s peak %.2f  discontinuities %d\n", names[preset].toRawUTF8(), peak, clicks);
        }

        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--cpu")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor cp;
        cp.setPlayConfigDetails (0, 2, sRate, sBlock);
        cp.prepareToPlay (sRate, sBlock);

        juce::AudioBuffer<float> block (2, sBlock);
        const auto names = botanic::factoryPresetNames();
        double worstRatio = 0.0;
        juce::String worstName;

        for (int preset = 0; preset < names.size(); ++preset)
        {
            cp.setPresetIndex (preset);

            juce::AudioBuffer<float> warm (2, sBlock);
            juce::MidiBuffer m;

            for (int k = 0; k < 6; ++k)
                m.addEvent (juce::MidiMessage::noteOn (1, 40 + k * 4, 0.9f), 0);

            double maxMs = 0.0, sumMs = 0.0;
            const auto firstBlockMs = [&]
            {
                juce::AudioBuffer<float> one (2, sBlock);
                juce::MidiBuffer mm;
                mm.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);
                const auto t0 = juce::Time::getHighResolutionTicks();
                cp.processBlock (one, mm);
                return juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
            }();
            juce::ignoreUnused (firstBlockMs);
            int measured = 0;

            for (int b = 0; b < 500; ++b)
            {
                warm.clear();
                const auto t0 = juce::Time::getHighResolutionTicks();
                cp.processBlock (warm, m);
                const auto t1 = juce::Time::getHighResolutionTicks();
                m.clear();
                const auto ms = juce::Time::highResolutionTicksToSeconds (t1 - t0) * 1000.0;

                if (b >= 100)
                {
                    maxMs = juce::jmax (maxMs, ms);
                    sumMs += ms;
                    ++measured;
                }
            }

            const auto ratio = maxMs / ((double) sBlock / sRate * 1000.0);

            if (ratio > worstRatio)
            {
                worstRatio = ratio;
                worstName = names[preset];
            }

            std::printf ("%-18s max %.2f avg %.2f first %.2f ms = %2.0f%% of budget\n",
                         names[preset].toRawUTF8(), maxMs, sumMs / juce::jmax (1, measured), firstBlockMs,
                         ratio * 100.0);
        }

        std::printf ("\nworst: %s at %.0f%% of the audio budget (block %.1f ms)\n",
                     worstName.toRawUTF8(), worstRatio * 100.0, (double) sBlock / sRate * 1000.0);
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--tailwav")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;
        const int preset = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;

        BotanicAudioProcessor tp;
        tp.setPlayConfigDetails (0, 2, sRate, sBlock);
        tp.prepareToPlay (sRate, sBlock);
        tp.setPresetIndex (preset);

        const auto total = (int) (sRate * 8.0);
        juce::AudioBuffer<float> full (2, total);
        full.clear();
        juce::AudioBuffer<float> block (2, sBlock);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 45, 0.95f), 0);
        m.addEvent (juce::MidiMessage::noteOn (1, 57, 0.95f), 0);

        int pos = 0;

        while (pos < total)
        {
            block.clear();

            if (pos >= (int) (sRate * 1.5) && pos < (int) (sRate * 1.5) + sBlock)
                m.addEvent (juce::MidiMessage::allNotesOff (1), 0);

            tp.processBlock (block, m);
            m.clear();

            const auto n = juce::jmin (sBlock, total - pos);
            full.copyFrom (0, pos, block, 0, 0, n);
            full.copyFrom (1, pos, block, 1, 0, n);
            pos += n;
        }

        juce::WavAudioFormat wav;
        juce::File file ("tail.wav");
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

        if (stream != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sRate, 2, 24, {}, 0));

            if (writer != nullptr)
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (full, 0, total);
            }
        }

        std::printf ("wrote tail.wav for preset %d\n", preset);
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--denormal")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor dp;
        dp.setPlayConfigDetails (0, 2, sRate, sBlock);
        dp.prepareToPlay (sRate, sBlock);
        dp.setPresetIndex (0);

        auto set = [&dp] (const juce::String& id, float v)
        {
            if (auto* p = dp.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        set (Params::revOn, 1.0f);
        set (Params::revMix, 1.0f);
        set (Params::revSize, 1.0f);
        set (Params::revDamp, 0.0f);
        set (Params::dlyOn, 1.0f);
        set (Params::dlyMix, 0.5f);
        set (Params::choOn, 0.0f);

        juce::AudioBuffer<float> block (2, sBlock);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 48, 0.9f), 0);

        for (int b = 0; b < (int) (sRate * 1.0 / sBlock); ++b)
        {
            block.clear();
            dp.processBlock (block, m);
            m.clear();
        }

        juce::MidiBuffer stop;
        stop.addEvent (juce::MidiMessage::allNotesOff (1), 0);

        std::printf ("tail  seconds : ");
        double windowMs = 0.0;
        int windowBlocks = 0;

        for (int b = 0; b < (int) (sRate * 40.0 / sBlock); ++b)
        {
            block.clear();
            const auto t0 = juce::Time::getHighResolutionTicks();
            dp.processBlock (block, stop);
            const auto ms = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0) * 1000.0;
            windowMs += ms;
            ++windowBlocks;

            if (b == 0)
                stop.clear();

            if ((b + 1) % (int) (sRate * 2.0 / sBlock) == 0)
            {
                double rms = 0.0;

                for (int i = 0; i < sBlock; ++i)
                    rms += (double) block.getSample (0, i) * block.getSample (0, i);

                std::printf ("%2ds rms %.0f dB  avg %.2f ms  ",
                             (b + 1) / (int) (sRate / sBlock),
                             juce::Decibels::gainToDecibels ((float) std::sqrt (rms / sBlock), -140.0f),
                             windowMs / juce::jmax (1, windowBlocks));
                windowMs = 0.0;
                windowBlocks = 0;
            }
        }

        std::printf ("\n");
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--tailprofile")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor tp;
        tp.setPlayConfigDetails (0, 2, sRate, sBlock);
        tp.prepareToPlay (sRate, sBlock);

        auto set = [&tp] (const juce::String& id, float v)
        {
            if (auto* p = tp.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        juce::AudioBuffer<float> block (2, sBlock);

        for (int preset : { 0, 3, 16, 23 })
        {
            tp.setPresetIndex (preset);

            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, 45, 0.95f), 0);
            m.addEvent (juce::MidiMessage::noteOn (1, 57, 0.95f), 0);

            for (int b = 0; b < (int) (sRate * 1.5 / sBlock); ++b)
            {
                block.clear();
                tp.processBlock (block, m);
                m.clear();
            }

            juce::MidiBuffer stop;
            stop.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            block.clear();
            tp.processBlock (block, stop);

            std::printf ("preset %d %s:", preset, botanic::factoryPresetNames()[preset].toRawUTF8());

            for (int w = 0; w < 16; ++w)
            {
                double rms = 0.0;

                for (int b = 0; b < (int) (sRate * 0.25 / sBlock); ++b)
                {
                    block.clear();
                    juce::MidiBuffer none;
                    tp.processBlock (block, none);

                    for (int i = 0; i < sBlock; ++i)
                        rms += (double) block.getSample (0, i) * block.getSample (0, i);
                }

                rms = std::sqrt (rms / (sRate * 0.25));
                std::printf (" %.0f", juce::Decibels::gainToDecibels ((float) rms, -100.0f));
            }

            std::printf (" dB\n");
        }

        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--tails")
    {
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor tp;
        tp.setPlayConfigDetails (0, 2, sRate, sBlock);
        tp.prepareToPlay (sRate, sBlock);

        juce::AudioBuffer<float> block (2, sBlock);
        juce::Random rng (7);
        int badTails = 0, clickRuns = 0;

        auto renderSeconds = [&] (double seconds, const juce::MidiBuffer& first, double* outPeak,
                                  int* clicks, double* tailRms)
        {
            juce::MidiBuffer m = first;
            const int blocks = (int) (sRate * seconds / sBlock);
            double peak = 0.0, rms = 0.0;
            int cl = 0;
            float prevL = 0.0f;

            for (int b = 0; b < blocks; ++b)
            {
                block.clear();
                tp.processBlock (block, m);
                m.clear();

                for (int i = 0; i < sBlock; ++i)
                {
                    const auto v = block.getSample (0, i);
                    peak = juce::jmax (peak, (double) std::abs (v));

                    if (std::abs (v - prevL) > 0.25f)
                        ++cl;

                    prevL = v;

                    if (b > blocks - (int) (sRate * 0.25 / sBlock))
                        rms += (double) v * v;
                }
            }

            if (outPeak) *outPeak = peak;
            if (clicks) *clicks = cl;
            if (tailRms) *tailRms = std::sqrt (rms / juce::jmax (1.0, sRate * 0.25));
        };

        for (int iter = 0; iter < 30; ++iter)
        {
            const auto preset = rng.nextInt (botanic::factoryPresetNames().size());
            tp.setPresetIndex (preset);

            juce::MidiBuffer start;
            start.addEvent (juce::MidiMessage::noteOn (1, 45, 0.95f), 0);
            start.addEvent (juce::MidiMessage::noteOn (1, 57, 0.95f), 0);
            start.addEvent (juce::MidiMessage::noteOn (1, 64, 0.95f), 0);

            double peakA = 0.0, peakB = 0.0, tail = 0.0;
            int clicksA = 0, clicksB = 0;
            renderSeconds (1.5, start, &peakA, &clicksA, nullptr);

            juce::MidiBuffer stop;
            stop.addEvent (juce::MidiMessage::allNotesOff (1), 0);

            // switch to the quiet init patch while the tail rings, then listen to the tail
            tp.setPresetIndex (botanic::factoryPresetNames().size() - 1);
            renderSeconds (0.2, stop, nullptr, nullptr, nullptr);
            renderSeconds (3.0, juce::MidiBuffer(), &peakB, &clicksB, &tail);

            const auto tailDb = juce::Decibels::gainToDecibels ((float) tail, -100.0f);

            if (tailDb > -55.0f)
                ++badTails;

            if (clicksB > 12)
                ++clickRuns;

            if (tailDb > -55.0f || clicksB > 12)
                std::printf ("iter %2d preset %-18s peakA %.2f tail %.0f dB clicks %d/%d\n",
                             iter, botanic::factoryPresetNames()[preset].toRawUTF8(),
                             peakA, tailDb, clicksA, clicksB);
        }

        std::printf ("\nstuck tails: %d / 30   click bursts: %d / 30\n", badTails, clickRuns);
        return (badTails == 0 && clickRuns == 0) ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--stress")
    {
        const double sRate = 44100.0;
        const int sBlock = 64;

        BotanicAudioProcessor sp;
        sp.setPlayConfigDetails (0, 2, sRate, sBlock);
        sp.prepareToPlay (sRate, sBlock);

        juce::AudioBuffer<float> block (2, sBlock);
        juce::MidiBuffer midi;
        juce::Random rnd (42);
        double worst = 0.0;
        int nan = 0, over = 0;
        const auto seconds = argc > 2 ? juce::String (argv[2]).getIntValue() : 40;
        const int blocks = (int) (sRate * seconds / sBlock);
        auto nextSwitch = 0;
        auto nextNote = 0;

        for (int b = 0; b < blocks; ++b)
        {
            block.clear();
            midi.clear();

            if (b >= nextSwitch)
            {
                sp.setPresetIndex (rnd.nextInt (botanic::factoryPresetNames().size()));
                nextSwitch = b + (int) (sRate * 0.15 / sBlock);
            }

            if (b >= nextNote)
            {
                const auto note = 36 + rnd.nextInt (40);
                midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.7f + rnd.nextFloat() * 0.3f), 0);
                midi.addEvent (juce::MidiMessage::noteOff (1, note), sBlock / 2);
                nextNote = b + (int) (sRate * 0.09 / sBlock);
            }

            sp.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                {
                    const auto v = block.getSample (ch, i);
                    if (! std::isfinite (v)) ++nan;
                    if (std::abs (v) > 1.0001f) ++over;
                    worst = juce::jmax (worst, (double) std::abs (v));
                }
        }

        std::printf ("stress %ds @44.1k/64: worst peak %.3f, non-finite %d, over-unity %d : %s\n",
                     seconds, worst, nan, over, (nan == 0 && over == 0) ? "PASS" : "FAIL");

        // hosts are allowed to hand us bigger blocks than were prepared
        {
            BotanicAudioProcessor vp;
            vp.setPlayConfigDetails (0, 2, 48000.0, 128);
            vp.prepareToPlay (48000.0, 128);
            vp.setPresetIndex (0);

            juce::AudioBuffer<float> big (2, 1024);
            juce::MidiBuffer bm;
            double bworst = 0.0;
            int bnan = 0;

            for (int b = 0; b < 200; ++b)
            {
                big.clear();
                bm.clear();

                if (b % 20 == 0)
                    bm.addEvent (juce::MidiMessage::noteOn (1, 40 + b % 30, 0.9f), 0);

                vp.processBlock (big, bm);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < big.getNumSamples(); ++i)
                    {
                        const auto v = big.getSample (ch, i);
                        if (! std::isfinite (v)) ++bnan;
                        bworst = juce::jmax (bworst, (double) std::abs (v));
                    }
            }

            std::printf ("block size mismatch (prepared 128, fed 1024): peak %.3f, non-finite %d : %s\n",
                         bworst, bnan, bnan == 0 ? "PASS" : "FAIL");
            nan += bnan;
        }

        return (nan == 0 && over == 0) ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--scan")
    {
        // play a chord, walk through every preset while it sounds, and watch for
        // non-finite samples, runaway levels and render time spikes
        const double sRate = 48000.0;
        const int sBlock = 128;

        BotanicAudioProcessor sp;
        sp.setPlayConfigDetails (0, 2, sRate, sBlock);
        sp.prepareToPlay (sRate, sBlock);

        const auto names = botanic::factoryPresetNames();
        juce::AudioBuffer<float> block (2, sBlock);
        juce::MidiBuffer midi;
        double worstPeak = 0.0;
        int nanCount = 0;
        int overCount = 0;

        for (int preset = 0; preset < names.size(); ++preset)
        {
            sp.setPresetIndex (preset);

            midi.clear();
            midi.addEvent (juce::MidiMessage::noteOn (1, 40, 1.0f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 52, 1.0f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 59, 1.0f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 0);

            const auto started = juce::Time::getMillisecondCounterHiRes();
            double peak = 0.0;
            int nan = 0;
            int over = 0;

            for (int b = 0; b < 120; ++b)   // ~0.32 s per preset
            {
                block.clear();
                sp.processBlock (block, midi);
                midi.clear();

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block.getNumSamples(); ++i)
                    {
                        const auto v = block.getSample (ch, i);

                        if (! std::isfinite (v))
                            ++nan;

                        if (std::abs (v) > 1.0001f)
                            ++over;

                        peak = juce::jmax (peak, (double) std::abs (v));
                    }
            }

            const auto ms = (juce::Time::getMillisecondCounterHiRes() - started) / 120.0;
            worstPeak = juce::jmax (worstPeak, peak);
            nanCount += nan;
            overCount += over;

            std::printf ("%-18s peak=%6.3f nan=%d over=%d  %.3f ms/block\n",
                         names[preset].toRawUTF8(), peak, nan, over, ms);
        }

        // leave a long tail running and check it settles
        juce::MidiBuffer stop;
        stop.addEvent (juce::MidiMessage::allNotesOff (1), 0);

        for (int b = 0; b < 400; ++b)
        {
            block.clear();
            sp.processBlock (block, stop);
            stop.clear();

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                    if (! std::isfinite (block.getSample (ch, i)))
                        ++nanCount;
        }

        std::printf ("\nworst peak %.3f, non-finite samples %d, over-unity samples %d\n",
                     worstPeak, nanCount, overCount);
        return (nanCount == 0 && overCount == 0) ? 0 : 1;
    }

    if (argc > 1 && juce::String (argv[1]) == "--alias")
    {
        const double ar = 48000.0;
        const int ab = 128;

        BotanicAudioProcessor ap;
        ap.setPlayConfigDetails (0, 2, ar, ab);
        ap.prepareToPlay (ar, ab);
        ap.initPatch();

        auto set = [&ap] (const juce::String& id, float v)
        {
            if (auto* p = ap.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        set (Params::oscId (0, "table"), 3.0f);
        set (Params::oscId (0, "pos"), 0.2f);
        set (Params::cutoff, 20000.0f);
        set (Params::resonance, 0.1f);
        set (Params::drive, 0.9f);
        set (Params::filterEnv, 0.0f);
        set (Params::keyTrack, 0.0f);
        set (Params::envId (0, "A"), 0.002f);
        set (Params::envId (0, "S"), 1.0f);
        set (Params::revOn, 0.0f); set (Params::dlyOn, 0.0f); set (Params::choOn, 0.0f);

        const auto total = (int) (ar * 1.0);
        juce::AudioBuffer<float> full (2, total), blk (2, ab);
        full.clear();
        juce::MidiBuffer midi;
        int pos = 0;

        while (pos < total)
        {
            const auto n = juce::jmin (ab, total - pos);
            blk.setSize (2, n, false, false, true);
            blk.clear();
            midi.clear();

            if (pos == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 96, 1.0f), 0);

            ap.processBlock (blk, midi);
            full.copyFrom (0, pos, blk, 0, 0, n);
            full.copyFrom (1, pos, blk, 1, 0, n);
            pos += n;
        }

        juce::WavAudioFormat wav;
        juce::File file ("alias.wav");
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

        if (stream != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), ar, 2, 24, {}, 0));

            if (writer != nullptr)
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (full, 0, total);
            }
        }

        std::printf ("wrote alias.wav\n");
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "--glitch")
    {
        const double gsRate = 48000.0;
        const int gsBlock = 128;

        BotanicAudioProcessor gp;
        gp.setPlayConfigDetails (0, 2, gsRate, gsBlock);
        gp.prepareToPlay (gsRate, gsBlock);
        gp.initPatch();

        auto set = [&gp] (const juce::String& id, float v)
        {
            if (auto* p = gp.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        set (Params::oscId (0, "table"), 3.0f);
        set (Params::oscId (0, "pos"), 0.2f);
        set (Params::cutoff, 12000.0f);
        set (Params::envId (0, "A"), 0.004f);
        set (Params::envId (0, "S"), 0.0f);
        set (Params::envId (0, "D"), 0.18f);
        set (Params::envId (0, "R"), 0.1f);
        set (Params::revOn, 0.0f);
        set (Params::dlyOn, 0.0f);
        set (Params::choOn, 0.0f);

        set (Params::glOn, 1.0f);
        set (Params::glMix, 1.0f);
        set (Params::glBits, 16.0f);
        set (Params::glRateDiv, 1.0f);
        set (Params::glStutter, 1.0f);
        set (Params::glGate, 0.0f);
        set (Params::glDrop, 0.0f);
        set (Params::glTape, 0.0f);
        set (Params::glRepeats, 4.0f);
        set (Params::glChaos, 0.0f);
        set (Params::glNoise, 0.0f);

        if (auto* p = gp.apvts.getParameter (Params::glStep))
            p->setValueNotifyingHost (p->convertTo0to1 (2.0f));      // 1/8 steps

        if (auto* p = gp.apvts.getParameter (Params::glStutterDiv))
            p->setValueNotifyingHost (p->convertTo0to1 (4.0f));      // 1/16 T stutter

        const auto total = (int) (gsRate * 3.0);
        juce::AudioBuffer<float> full (2, total), block (2, gsBlock);
        full.clear();
        juce::MidiBuffer midi;

        int pos = 0;

        while (pos < total)
        {
            const auto n = juce::jmin (gsBlock, total - pos);
            block.setSize (2, n, false, false, true);
            block.clear();
            midi.clear();

            if (pos == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);

            gp.processBlock (block, midi);
            full.copyFrom (0, pos, block, 0, 0, n);
            full.copyFrom (1, pos, block, 1, 0, n);
            pos += n;
        }

        juce::WavAudioFormat wav;
        juce::File file ("glitchtest.wav");
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

        if (stream != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), gsRate, 2, 24, {}, 0));

            if (writer != nullptr)
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (full, 0, total);
            }
        }

        std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
        return 0;
    }

    const auto writeWav = argc > 1 && juce::String (argv[1]) == "--wav";
    const auto sampleRate = 48000.0;
    const auto blockSize = 128;
    const auto seconds = 3.0;

    juce::File outputDir = juce::File::getCurrentWorkingDirectory().getChildFile ("renders");
    outputDir.createDirectory();

    BotanicAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, (int) blockSize);

    const auto names = botanic::factoryPresetNames();
    int failures = 0;

    for (int preset = 0; preset < names.size(); ++preset)
    {
        proc.setPresetIndex (preset);

        const auto totalSamples = (int) (sampleRate * seconds);
        juce::AudioBuffer<float> full (2, totalSamples);
        full.clear();

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        int pos = 0;
        int blockIndex = 0;

        while (pos < totalSamples)
        {
            const auto n = juce::jmin (blockSize, totalSamples - pos);
            block.setSize (2, n, false, false, true);
            block.clear();
            midi.clear();

            if (blockIndex == 0)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.85f), 0);
                midi.addEvent (juce::MidiMessage::noteOn (1, 55, 0.8f), 0);
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
            }

            if (pos + n >= (int) (sampleRate * 1.8) && pos < (int) (sampleRate * 1.8))
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
                midi.addEvent (juce::MidiMessage::noteOff (1, 55), 0);
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            }

            proc.processBlock (block, midi);

            full.copyFrom (0, pos, block, 0, 0, n);
            full.copyFrom (1, pos, block, 1, 0, n);

            pos += n;
            ++blockIndex;
        }

        float peak = 0.0f;
        double sumSq = 0.0;
        bool bad = false;

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* data = full.getReadPointer (ch);

            for (int i = 0; i < totalSamples; ++i)
            {
                const auto v = data[i];

                if (! std::isfinite (v))
                    bad = true;

                peak = juce::jmax (peak, std::abs (v));
                sumSq += (double) v * (double) v;
            }
        }

        const auto rms = std::sqrt (sumSq / (double) (totalSamples * 2));
        const auto silent = rms < 0.0005;
        const auto clipped = peak > 1.0001f;
        const auto ok = ! bad && ! silent;

        if (! ok)
            ++failures;

        std::printf ("%-18s rms=%7.4f peak=%6.3f %s%s%s\n",
                     names[preset].toRawUTF8(), rms, peak,
                     bad ? "NaN! " : "", silent ? "SILENT! " : "", clipped ? "CLIP! " : "");

        if (writeWav)
        {
            juce::WavAudioFormat wav;
            auto file = outputDir.getChildFile ("preset_" + juce::String (preset + 1) + "_"
                                                + juce::File::createLegalFileName (names[preset]) + ".wav");
            file.deleteFile();

            std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

            if (stream != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer (
                    wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0));

                if (writer != nullptr)
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer (full, 0, totalSamples);
                }
            }
        }
    }

    // ---- arpeggiator check
    {
        BotanicAudioProcessor arpProc;
        arpProc.setPlayConfigDetails (0, 2, sampleRate, blockSize);
        arpProc.prepareToPlay (sampleRate, (int) blockSize);
        arpProc.initPatch();

        if (auto* p = arpProc.apvts.getParameter (Params::arpOn))
            p->setValueNotifyingHost (1.0f);
        if (auto* p = arpProc.apvts.getParameter (Params::arpRate))
            p->setValueNotifyingHost (p->convertTo0to1 (3.0f));
        if (auto* p = arpProc.apvts.getParameter (Params::arpOctaves))
            p->setValueNotifyingHost (p->convertTo0to1 (2.0f));
        if (auto* p = arpProc.apvts.getParameter (Params::arpGate))
            p->setValueNotifyingHost (p->convertTo0to1 (0.4f));

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        std::set<int> notesHeard;
        float peak = 0.0f;

        for (int b = 0; b < 400; ++b)
        {
            block.clear();
            midi.clear();

            if (b == 0)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
                midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 0);
                midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.9f), 0);
            }

            arpProc.processBlock (block, midi);

            const auto mask = arpProc.getNoteMask();

            for (int n = 0; n < 128; ++n)
                if ((mask[(size_t) (n / 64)] & ((juce::uint64) 1 << (n % 64))) != 0)
                    notesHeard.insert (n);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                    peak = juce::jmax (peak, std::abs (block.getSample (ch, i)));
        }

        std::printf ("\narpeggiator: %d distinct notes played, peak=%.3f : ",
                     (int) notesHeard.size(), peak);

        for (auto n : notesHeard)
            std::printf ("%d ", n);

        const auto arpOk = notesHeard.size() >= 5 && peak > 0.01f;
        std::printf ("%s\n", arpOk ? "PASS" : "FAIL");

        if (! arpOk)
            ++failures;
    }

    // ---- granular freeze check
    {
        BotanicAudioProcessor gr;
        gr.setPlayConfigDetails (0, 2, sampleRate, blockSize);
        gr.prepareToPlay (sampleRate, (int) blockSize);
        gr.initPatch();

        auto set = [&gr] (const juce::String& id, float v)
        {
            if (auto* p = gr.apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
        };

        set (Params::oscId (0, "table"), 3.0f);
        set (Params::cutoff, 6000.0f);
        set (Params::envId (0, "S"), 1.0f);
        set (Params::revOn, 0.0f); set (Params::dlyOn, 0.0f); set (Params::choOn, 0.0f);
        set (Params::grOn, 1.0f);
        set (Params::grMix, 1.0f);
        set (Params::grSize, 100.0f);
        set (Params::grDensity, 24.0f);
        set (Params::grPosition, 0.15f);
        set (Params::grSpray, 0.0f);
        set (Params::grPitchRandom, 0.0f);
        set (Params::grReverse, 0.0f);

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        double energyEarly = 0.0, energyLate = 0.0;
        int         countEarly = 0, countLate = 0;
        float       tailPeak = 0.0f;

        for (int b = 0; b < 600; ++b)
        {
            block.clear();
            midi.clear();

            if (b == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);

            if (b == 90)
            {
                set (Params::grFreeze, 1.0f);
                midi.addEvent (juce::MidiMessage::noteOff (1, 57), 0);
            }

            gr.processBlock (block, midi);

            double sum = 0.0;

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                {
                    const auto v = block.getSample (ch, i);
                    sum += (double) v * v;

                    if (b > 400)
                        tailPeak = juce::jmax (tailPeak, std::abs (v));
                }

            if (b > 150 && b < 220) { energyEarly += sum; ++countEarly; }
            if (b > 400)          { energyLate += sum;  ++countLate; }
        }

        const auto rmsEarly = std::sqrt (energyEarly / juce::jmax (1, countEarly) / (blockSize * 2));
        const auto rmsLate = std::sqrt (energyLate / juce::jmax (1, countLate) / (blockSize * 2));
        const auto grainOk = rmsEarly > 0.002 && rmsLate > 0.0008;

        std::printf ("\ngranular freeze: early rms=%.4f  frozen tail rms=%.4f peak=%.4f : %s\n",
                     rmsEarly, rmsLate, tailPeak, grainOk ? "PASS" : "FAIL");

        if (! grainOk)
            ++failures;
    }

    // ---- randomiser stress test
    {
        BotanicAudioProcessor rp;
        rp.setPlayConfigDetails (0, 2, sampleRate, blockSize);
        rp.prepareToPlay (sampleRate, (int) blockSize);

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        int bad = 0;
        float worst = 0.0f;

        for (int round = 0; round < 12; ++round)
        {
            rp.randomize (round % 7);

            bool noteOnSent = false;

            for (int b = 0; b < 80; ++b)
            {
                block.clear();
                midi.clear();

                if (! noteOnSent)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, 48 + round * 2, 0.9f), 0);
                    noteOnSent = true;
                }

                rp.processBlock (block, midi);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < block.getNumSamples(); ++i)
                    {
                        const auto v = block.getSample (ch, i);
                        worst = juce::jmax (worst, std::abs (v));

                        if (! std::isfinite (v) || std::abs (v) > 1.0f)
                            ++bad;
                    }
            }

            midi.clear();
            rp.processBlock (block, midi);
        }

        std::printf ("randomiser: %d bad samples, peak=%.3f : %s\n", bad, worst, bad == 0 ? "PASS" : "FAIL");

        if (bad != 0)
            ++failures;
    }

    // ---- on screen keyboard queue + held note display
    {
        BotanicAudioProcessor kp;
        kp.setPlayConfigDetails (0, 2, sampleRate, blockSize);
        kp.prepareToPlay (sampleRate, (int) blockSize);
        kp.initPatch();

        kp.keyboardNoteOn (60, 0.9f);
        const auto heldNow = kp.getHeldNotes();
        const auto visualOn = (heldNow[0] & ((juce::uint64) 1 << 60)) != 0;

        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;
        float peak = 0.0f;

        for (int b = 0; b < 40; ++b)
        {
            block.clear();
            midi.clear();
            kp.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                    peak = juce::jmax (peak, std::abs (block.getSample (ch, i)));
        }

        kp.keyboardNoteOff (60);
        const auto heldAfter = kp.getHeldNotes();
        const auto visualOff = (heldAfter[0] & ((juce::uint64) 1 << 60)) == 0;

        const auto kbdOk = visualOn && visualOff && peak > 0.01f;
        std::printf ("screen keyboard: held=%d cleared=%d peak=%.3f : %s\n",
                     (int) visualOn, (int) visualOff, peak, kbdOk ? "PASS" : "FAIL");

        if (! kbdOk)
            ++failures;
    }

    // ---- formant / vowel engine check
    {
        auto renderVowel = [&] (float vowel)
        {
            BotanicAudioProcessor fp;
            fp.setPlayConfigDetails (0, 2, sampleRate, blockSize);
            fp.prepareToPlay (sampleRate, (int) blockSize);
            fp.initPatch();

            auto set = [&fp] (const juce::String& id, float v)
            {
                if (auto* p = fp.apvts.getParameter (id))
                    p->setValueNotifyingHost (p->convertTo0to1 (v));
            };

            set (Params::oscId (0, "table"), 3.0f);
            set (Params::cutoff, 15000.0f);
            set (Params::envId (0, "S"), 1.0f);
            set (Params::revOn, 0.0f); set (Params::dlyOn, 0.0f); set (Params::choOn, 0.0f);
            set (Params::fmtOn, 1.0f);
            set (Params::fmtMix, 1.0f);
            set (Params::fmtQ, 0.6f);
            set (Params::fmtKey, 0.0f);
            set (Params::fmtShift, 0.0f);
            set (Params::fmtVowel, vowel);

            juce::AudioBuffer<float> block (2, blockSize);
            juce::MidiBuffer midi;
            juce::AudioBuffer<float> total (2, 12000);
            total.clear();

            for (int b = 0; b < 60; ++b)
            {
                block.clear();
                midi.clear();

                if (b == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 45, 0.9f), 0);

                fp.processBlock (block, midi);

                if (b * blockSize + blockSize <= total.getNumSamples())
                    for (int ch = 0; ch < 2; ++ch)
                        total.copyFrom (ch, b * blockSize, block, ch, 0, blockSize);
            }

            return total;
        };

        const auto a = renderVowel (0.05f);
        const auto e = renderVowel (0.95f);
        double diff = 0.0, energy = 0.0;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
            {
                const auto d = a.getSample (ch, i) - e.getSample (ch, i);
                diff += (double) d * d;
                energy += (double) a.getSample (ch, i) * a.getSample (ch, i);
            }

        const auto ratio = diff / juce::jmax (1.0e-12, energy);
        const auto ok = ratio > 0.02 && energy > 1.0e-6;

        std::printf ("formant morph: spectral difference %.3f : %s\n", ratio, ok ? "PASS" : "FAIL");

        if (! ok)
            ++failures;
    }

    std::printf ("\n%d test(s) failed\n", failures);
    return failures == 0 ? 0 : 1;
}
