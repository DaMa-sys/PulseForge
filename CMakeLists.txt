cmake_minimum_required(VERSION 3.22)

project(PulseForge VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Discover JUCE: checks C:/JUCE/JUCE, environment variable JUCE_PATH, local subfolder, or CMake package
if (NOT TARGET juce::juce_audio_utils)
    if (EXISTS "C:/JUCE/JUCE/CMakeLists.txt")
        add_subdirectory("C:/JUCE/JUCE" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    elseif (DEFINED ENV{JUCE_PATH} AND EXISTS "$ENV{JUCE_PATH}/CMakeLists.txt")
        add_subdirectory("$ENV{JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/JUCE" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    else()
        find_package(JUCE CONFIG REQUIRED)
    endif()
endif()

juce_add_plugin(PulseForge
    COMPANY_NAME "Orage Audio"
    BUNDLE_ID "com.orage.pulseforge"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Orag
    PLUGIN_CODE PlFg
    FORMATS VST3 Standalone
    PRODUCT_NAME "PulseForge"
)

target_sources(PulseForge
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PatternData.h
        Source/StyleGenerator.h
        Source/PreviewSynth.h
)

target_compile_definitions(PulseForge
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PulseForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Regression & Metric Test Executable
add_executable(PulseForgeRegression Tests/test_harness.cpp)
target_link_libraries(PulseForgeRegression PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeVerifyRegression Tests/verify_regression.cpp)
target_link_libraries(PulseForgeVerifyRegression PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeTestGenres Tests/test_genres.cpp)
target_link_libraries(PulseForgeTestGenres PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeFisherPilot Tests/test_fisher_pilot.cpp)
target_link_libraries(PulseForgeFisherPilot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeDump128 Tests/dump_128steps.cpp)
target_link_libraries(PulseForgeDump128 PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeStressAudio Tests/test_audio_stress.cpp)
target_link_libraries(PulseForgeStressAudio PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeTestComplexity Tests/test_complexity_layers.cpp)
target_link_libraries(PulseForgeTestComplexity PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeCorpusSampler Tests/corpus_sampler.cpp)
target_link_libraries(PulseForgeCorpusSampler PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeCorpusAnalyzer Tests/corpus_analyzer.cpp)
target_link_libraries(PulseForgeCorpusAnalyzer PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeTestKalkbrenner Tests/test_kalkbrenner_suite.cpp)
target_link_libraries(PulseForgeTestKalkbrenner PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeTestAllArtists Tests/test_all_artists_suite.cpp)
target_link_libraries(PulseForgeTestAllArtists PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)

add_executable(PulseForgeGuiPreview 
    Tests/gui_preview.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_compile_definitions(PulseForgeGuiPreview PRIVATE
    JucePlugin_Name="PulseForge"
    JucePlugin_Manufacturer="DaMA"
    JucePlugin_Version="1.0.0"
    JucePlugin_VersionCode=0x10000
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)
target_link_libraries(PulseForgeGuiPreview PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra)











