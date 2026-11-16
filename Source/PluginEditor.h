#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/MainView.h"

class BotanicAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit BotanicAudioProcessorEditor (BotanicAudioProcessor&);
    ~BotanicAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void checkComputerKeyboard();
    void buildView();

    BotanicAudioProcessor& proc;
    std::unique_ptr<ui::MainView> mainView;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    bool rebuilding = false;

    int tick = 0;
    std::array<bool, 17> keyStates {};
    bool anyKeyDown = false;
    bool snapshotMode = false;
    juce::String snapshotPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BotanicAudioProcessorEditor)
};
