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

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Libraries)

CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    SOURCE_DIR ${LIB_DIR}/juce
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
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "This app needs microphone access for audio looping"
)

if(APPLE)
    add_custom_command(TARGET LooperPlugin_Standalone POST_BUILD
        COMMAND codesign --force --deep --sign - $<TARGET_BUNDLE_DIR:LooperPlugin_Standalone>
        COMMENT "Code signing Standalone app"
    )
endif()

target_sources(LooperPlugin
    PRIVATE
        # Main plugin files
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        # Models
        Source/Models/Looper.cpp
        Source/Models/Looper.h
        Source/Models/TrackManager.cpp
        Source/Models/TrackManager.h
        Source/Models/Track.cpp
        Source/Models/Track.h
        # Views
        Source/Views/TrackView.cpp
        Source/Views/TrackView.h
        Source/Views/TrackContainer.cpp
        Source/Views/TrackContainer.h
        Source/Views/GlobalControlBar.cpp
        Source/Views/GlobalControlBar.h
)

target_compile_definitions(LooperPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LooperPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Testing
enable_testing()

CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.15.2
    VERSION 1.15.2
    OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
)

add_executable(LooperPluginTests
    Tests/test_main.cpp
    Tests/test_looper.cpp
)

target_include_directories(LooperPluginTests
    PRIVATE
        ${LIB_DIR}/juce/modules
)

target_compile_features(LooperPluginTests PRIVATE cxx_std_17)

target_link_libraries(LooperPluginTests
    PRIVATE
        gtest
        gtest_main
        LooperPlugin
)

include(GoogleTest)
gtest_discover_tests(LooperPluginTests)
