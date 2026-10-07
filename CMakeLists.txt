cmake_minimum_required(VERSION 3.22)
project(KYOTRIPPAH VERSION 0.4.2 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(KYOTO_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DreamApi.cpp)

function(kyoto_make target product code synth categories)
    juce_add_plugin(${target}
        COMPANY_NAME "Dreamdaw"
        BUNDLE_ID "com.dreamdaw.${target}"
        PLUGIN_MANUFACTURER_CODE Dmdw
        PLUGIN_CODE ${code}
        FORMATS VST3
        PRODUCT_NAME "${product}"
        IS_SYNTH ${synth}
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES ${categories})
    juce_generate_juce_header(${target})
    target_sources(${target} PRIVATE ${KYOTO_SOURCES})
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endfunction()

kyoto_make(KYOTO "KYOTO" Kyo1 TRUE "Instrument;Synth")
target_compile_definitions(KYOTO PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_REPORT_PLUGIN_BEING_CREATED=0)

kyoto_make(KYOTRIPPAHFX "KYOTRIPPAH FX" Kyo2 FALSE "Fx")
target_compile_definitions(KYOTRIPPAHFX PRIVATE
    KYOTO_IS_FX=1
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_REPORT_PLUGIN_BEING_CREATED=0)
