cmake_minimum_required(VERSION 3.15)
project(LooperPlugin VERSION 1.0.0)

# Install CPM package manager
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
)

juce_add_plugin(LooperPlugin
    COMPANY_NAME "NathanMyles"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Namy
    PLUGIN_CODE Lopr
    FORMATS VST3 AU Standalone
)

target_sources(LooperPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_definitions(LooperPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LooperPlugin
    PRIVATE
        PUBLIC juce::juce_audio_utils
        PUBLIC juce::juce_dsp
        PRIVATE juce::juce_recommended_config_flags
        PRIVATE juce::juce_recommended_lto_flags
        PRIVATE juce::juce_recommended_warning_flags
)