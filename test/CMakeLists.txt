cpmaddpackage(
  NAME
    googletest
  GITHUB_REPOSITORY
    google/googletest
  GIT_TAG
    v1.14.0
  OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt ON"
)

add_executable(JangolizerTests
  PolyBLEPOscillatorTests.cpp
  PluginProcessorTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.cpp
)

target_include_directories(JangolizerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Source)

# ELK_HEADLESS=0: PluginProcessor::createEditor() is pure virtual in JUCE and must stay
# declared/defined, so tests build the same desktop (non-headless) configuration.
# PluginEditor.cpp itself is NOT compiled here: createEditor() only returns a
# juce::GenericAudioProcessorEditor and never touches JangolizerAudioProcessorEditor,
# so its implementation isn't needed to satisfy the header declarations at compile time.
target_compile_definitions(JangolizerTests PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  ELK_HEADLESS=0
  JUCE_PLUGINHOST_VST=0
  JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

target_link_libraries(JangolizerTests PRIVATE
  GTest::gtest_main
  juce::juce_audio_processors
  juce::juce_audio_basics
  juce::juce_dsp
  juce::juce_recommended_config_flags
)

include(GoogleTest)
gtest_discover_tests(JangolizerTests)
