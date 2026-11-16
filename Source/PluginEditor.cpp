#include "PluginEditor.h"

namespace
{
    constexpr int mappedKeys[] = { 'a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k', 'o', 'l', 'p', ';' };
}

BotanicAudioProcessorEditor::BotanicAudioProcessorEditor (BotanicAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    if (const char* lang = std::getenv ("BOTANIC_LANG"))
        proc.setLanguage (juce::String (lang).getIntValue());

    ui::Locale::setLanguage (proc.getLanguage());

    setOpaque (true);
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (true);

    setResizable (true, true);
    setResizeLimits (1200, 675, 2304, 1296);
    getConstrainer()->setFixedAspectRatio ((double) ui::MainView::designWidth / (double) ui::MainView::designHeight);

    setSize (1500, 844);
    buildView();
    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 650);

    startTimerHz (60);

    if (const char* toggles = std::getenv ("BOTANIC_TOGGLE_LANG"))
    {
        const auto count = juce::String (toggles).getIntValue();
        juce::Component::SafePointer<BotanicAudioProcessorEditor> safe (this);

        for (int i = 1; i <= juce::jmax (1, count); ++i)
            juce::Timer::callAfterDelay (600 + i * 150, [safe]
            {
                if (safe != nullptr && safe->mainView != nullptr)
                    safe->mainView->toggleLanguage();
            });
    }

    if (std::getenv ("BOTANIC_SAVE_DIALOG") != nullptr)
    {
        juce::Component::SafePointer<ui::MainView> safeView (mainView.get());
        juce::Timer::callAfterDelay (700, [safeView]
        {
            if (safeView != nullptr)
                safeView->openSaveDialogPublic();
        });
    }

    if (std::getenv ("BOTANIC_MENU") != nullptr)
    {
        juce::Component::SafePointer<ui::MainView> safeView (mainView.get());
        juce::Timer::callAfterDelay (700, [safeView]
        {
            if (safeView != nullptr)
                safeView->openRandomMenu();
        });
    }

    if (const char* snap = std::getenv ("BOTANIC_SNAPSHOT"))
    {
        snapshotMode = true;
        snapshotPath = juce::String (snap);
    }
}

BotanicAudioProcessorEditor::~BotanicAudioProcessorEditor()
{
    stopTimer();
}

void BotanicAudioProcessorEditor::buildView()
{
    if (rebuilding)
        return;

    const juce::ScopedValueSetter<bool> guard (rebuilding, true);
    const auto page = mainView != nullptr ? mainView->getStagePage() : 0;

    auto view = std::make_unique<ui::MainView> (proc);
    view->onToggleLanguage = [this]
    {
        proc.setLanguage (proc.getLanguage() == 0 ? 1 : 0);
        ui::Locale::setLanguage (proc.getLanguage());

        // never rebuild inside the button's own mouse callback: the button is a
        // child of the view we are about to replace
        juce::Component::SafePointer<BotanicAudioProcessorEditor> safe (this);
        juce::MessageManager::callAsync ([safe]
        {
            if (safe != nullptr)
                safe->buildView();
        });
    };

    if (mainView != nullptr)
        removeChildComponent (mainView.get());

    mainView = std::move (view);
    addAndMakeVisible (*mainView);

    if (page != 0)
        mainView->setStagePage (page);

    resized();

    if (const char* preset = std::getenv ("BOTANIC_PRESET"))
        proc.setPresetIndex (juce::String (preset).getIntValue());

    if (const char* stage = std::getenv ("BOTANIC_STAGE"))
        mainView->setStagePage (juce::String (stage).getIntValue());

    if (const char* tab = std::getenv ("BOTANIC_TEXTURE_TAB"))
        mainView->setTextureTab (juce::String (tab).getIntValue());
}

void BotanicAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff10160c));
}

void BotanicAudioProcessorEditor::resized()
{
    if (mainView == nullptr)
        return;

    const auto scale = (float) getWidth() / (float) ui::MainView::designWidth;

    mainView->setBounds (0, 0, ui::MainView::designWidth, ui::MainView::designHeight);
    mainView->setTransform (juce::AffineTransform::scale (scale));
}

void BotanicAudioProcessorEditor::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

void BotanicAudioProcessorEditor::checkComputerKeyboard()
{
    if (mainView == nullptr)
        return;

    if (! mainView->isComputerKeyboardEnabled())
    {
        if (anyKeyDown)
        {
            for (size_t i = 0; i < numElementsInArray (mappedKeys); ++i)
            {
                if (keyStates[i])
                {
                    keyStates[i] = false;
                    proc.keyboardNoteOff (60 + (int) i);
                }
            }

            anyKeyDown = false;
        }

        return;
    }

    for (size_t i = 0; i < numElementsInArray (mappedKeys); ++i)
    {
        const auto down = juce::KeyPress::isKeyCurrentlyDown (mappedKeys[i]);

        if (down != keyStates[i])
        {
            keyStates[i] = down;
            const auto note = 60 + (int) i;
            anyKeyDown = anyKeyDown || down;

            if (down)
                proc.keyboardNoteOn (note, 0.85f);
            else
                proc.keyboardNoteOff (note);
        }
    }
}

void BotanicAudioProcessorEditor::timerCallback()
{
    ++tick;

    if (mainView == nullptr)
        return;

    mainView->animate();
    mainView->refreshAll();
    checkComputerKeyboard();

    if (tick % 30 == 0)
        proc.takeUndoSnapshot();

    if (snapshotMode && tick == 20)
    {
        auto image = mainView->createComponentSnapshot (mainView->getLocalBounds(), false);

        juce::File file (snapshotPath);
        file.deleteFile();

        if (auto stream = file.createOutputStream())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream (image, *stream);
        }

        snapshotMode = false;

        if (juce::JUCEApplicationBase::isStandaloneApp())
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
}
