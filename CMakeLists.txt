cmake_minimum_required(VERSION 3.22)

# On a Mac, default to a universal (Apple Silicon + Intel) build.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures")
    endif()
    # 10.15 so the Intel slice loads on Catalina. The arm64 slice is raised to 11.0 by the linker.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version" FORCE)
endif()

project(XoxicoaSynth VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(XS_BUILD_TESTS "Build the offline audit / demo / UI snapshot tools" OFF)
set(XS_JUCE_DIR "" CACHE PATH "Optional path to a local JUCE checkout (otherwise fetched from GitHub)")

if(XS_JUCE_DIR)
    add_subdirectory(${XS_JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(XoxicoaSynth
    COMPANY_NAME              "Xoxicoa"
    PRODUCT_NAME              "Xoxicoa Synth"
    BUNDLE_ID                 com.xoxicoa.xoxicoasynth
    PLUGIN_MANUFACTURER_CODE  Xoxi
    PLUGIN_CODE               XxSy
    FORMATS                   AU VST3 Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Instrument Synth
    AU_MAIN_TYPE              kAudioUnitType_MusicDevice)

set(XS_SOURCES Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_sources(XoxicoaSynth PRIVATE ${XS_SOURCES})
target_include_directories(XoxicoaSynth PRIVATE Source)

set(XS_DEFS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_compile_definitions(XoxicoaSynth PUBLIC ${XS_DEFS})

target_link_libraries(XoxicoaSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(APPLE)
    target_link_libraries(XoxicoaSynth PUBLIC juce::juce_recommended_lto_flags)
endif()

if(XS_BUILD_TESTS)
    # Offline engine audit + demo renderer + headless UI snapshot (development only)
    foreach(tool XoxicoaSynthAudit XoxicoaSynthUISnapshot XoxicoaSynthKitGenTest)
        juce_add_console_app(${tool} PRODUCT_NAME "${tool}")
        target_include_directories(${tool} PRIVATE Source)
        target_compile_definitions(${tool} PRIVATE ${XS_DEFS} JucePlugin_Name="Xoxicoa Synth" JUCE_MODAL_LOOPS_PERMITTED=1)
        target_link_libraries(${tool} PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats
                              PUBLIC juce::juce_recommended_config_flags)
    endforeach()
    target_sources(XoxicoaSynthAudit PRIVATE tests/Audit.cpp ${XS_SOURCES})
    target_sources(XoxicoaSynthUISnapshot PRIVATE tests/UISnapshot.cpp ${XS_SOURCES})
    target_sources(XoxicoaSynthKitGenTest PRIVATE tests/KitGenTest.cpp ${XS_SOURCES})
endif()
