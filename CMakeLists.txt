cmake_minimum_required(VERSION 3.22)
project(AuroraTrigger VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AuroraTrigger
    COMPANY_NAME "Aurora Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Auro
    PLUGIN_CODE AuTr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Aurora Trigger"
)

target_sources(AuroraTrigger
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

option(AURORA_PREVIEW_DEMO "Fake scope/meter data for UI screenshots (never ship)" OFF)

target_compile_definitions(AuroraTrigger
    PUBLIC
        AURORA_PREVIEW_DEMO=$<BOOL:${AURORA_PREVIEW_DEMO}>
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AuroraTrigger
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
