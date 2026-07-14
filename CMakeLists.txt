cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)

project(JangolizerPlugin VERSION 1.0.0)

# Opzione per decidere se compilare la versione Headless per Elk OS
option(ELK_HEADLESS_BUILD "Build the plugin without GUI for Elk Audio OS" OFF)

if(PROJECT_IS_TOP_LEVEL)
  if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
  endif()

  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>" CACHE STRING "" FORCE)
endif()

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include(cmake/cpm.cmake)

cpmaddpackage(
  NAME
    JUCE
  GIT_TAG
    8.0.12
  GITHUB_REPOSITORY
    juce-framework/JUCE
  SOURCE_DIR
    ${LIB_DIR}/juce
)

include(cmake/CompilerWarnings.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_directory_properties(PROPERTIES JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_directory_properties(PROPERTIES JUCE_VST3_COPY_DIR "$ENV{HOME}/.vst3")
endif()

set(SharedSources
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PolyBLEPOscillator.h
)

if (NOT ELK_HEADLESS_BUILD)
    list(APPEND SharedSources
        Source/PluginEditor.cpp
        Source/PluginEditor.h
    )
endif()

juce_add_plugin(JangolizerPlugin
    COMPANY_NAME "JangolizerLabs"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    FORMATS VST3 LV2 Standalone
    PRODUCT_NAME "Jangolizer"
    LV2URI "https://jangolizer.audio/jangolizer"
    VST3_AUTO_MANIFEST FALSE
)

target_sources(JangolizerPlugin PRIVATE ${SharedSources})

if (ELK_HEADLESS_BUILD)
    target_compile_definitions(JangolizerPlugin PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        ELK_HEADLESS=1
        JUCE_PLUGINHOST_VST=0
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
    )
else()
    target_compile_definitions(JangolizerPlugin PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        ELK_HEADLESS=0
        JUCE_PLUGINHOST_VST=0
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
    )
endif()

target_link_libraries(JangolizerPlugin PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_include_directories(JangolizerPlugin PRIVATE Source)
