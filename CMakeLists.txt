cmake_minimum_required(VERSION 3.22)
project(Botanic VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MINGW)
    add_compile_definitions(JUCE_DIRECT2D=0)
    add_link_options(-static -static-libgcc -static-libstdc++)
    # JUCE's graphics module needs these when building with MinGW
    set(BOTANIC_MINGW_SYSTEM_LIBS dcomp d3d11 dwrite d2d1 windowscodecs)
endif()

# JUCE 7.0.12: use the vendored copy when present, otherwise fetch it once.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

#===============================================================================
juce_add_binary_data(BotanicAssets
    SOURCES
        Assets/stage.png
        Assets/mountains.png
        Assets/texture.png
        Assets/fonts/Inter-Regular.ttf
        Assets/fonts/Inter-Medium.ttf
        Assets/fonts/Inter-SemiBold.ttf
        Assets/fonts/Inter-Bold.ttf
        Assets/fonts/Poppins-Bold.ttf)

set(BOTANIC_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp
    Source/dsp/WavetableBank.cpp
    Source/dsp/Engine.cpp
    Source/dsp/Effects.cpp
    Source/dsp/Texture.cpp
    Source/ui/Widgets.cpp
    Source/ui/Displays.cpp
    Source/ui/MainView.cpp)

if(APPLE)
    set(BOTANIC_FORMATS VST3 AU Standalone)
else()
    set(BOTANIC_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Botanic
    COMPANY_NAME "Botanic Audio"
    BUNDLE_ID "com.botanicaudio.botanic"
    PLUGIN_MANUFACTURER_CODE Btan
    PLUGIN_CODE Btnc
    FORMATS ${BOTANIC_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED FALSE
    PRODUCT_NAME "Botanic"
    VST3_CATEGORIES "Instrument Synth"
    VST3_AUTO_MANIFEST FALSE
    DESCRIPTION "Botanic - hybrid wavetable synthesizer")

juce_generate_juce_header(Botanic)

target_sources(Botanic PRIVATE ${BOTANIC_SOURCES})

target_compile_definitions(Botanic PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_USE_WINRT_MIDI=0
    BOTANIC_VERSION="1.0.0")

target_link_libraries(Botanic
    PRIVATE
        BotanicAssets
        ${BOTANIC_MINGW_SYSTEM_LIBS}
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags)

# static linking handled globally above

#===============================================================================
# Offline render harness used for testing the DSP without a host.
juce_add_console_app(BotanicRender PRODUCT_NAME "BotanicRender")

juce_generate_juce_header(BotanicRender)

target_sources(BotanicRender PRIVATE Tools/RenderTest.cpp ${BOTANIC_SOURCES})

target_compile_definitions(BotanicRender PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    BOTANIC_VERSION="1.0.0")

target_link_libraries(BotanicRender
    PRIVATE
        BotanicAssets
        ${BOTANIC_MINGW_SYSTEM_LIBS}
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags)



#===============================================================================
# VST3 host smoke test (loads the built plugin through JUCE's hosting layer).
juce_add_console_app(BotanicHostTest PRODUCT_NAME "BotanicHostTest")

juce_generate_juce_header(BotanicHostTest)

target_sources(BotanicHostTest PRIVATE Tools/HostTest.cpp)

target_compile_definitions(BotanicHostTest PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_AU=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1)

target_link_libraries(BotanicHostTest
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags)