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

# gtest 1.14.0 triggers -Wcharacter-conversion (char8_t -> char32_t) on newer
# Apple clang; upstream fixed post-1.14.0. Don't let our -Werror settings
# fail the build over a warning in vendored code we don't control.
foreach(gtest_target gtest gtest_main gmock gmock_main)
  if(TARGET ${gtest_target})
    set_target_properties(${gtest_target} PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
  endif()
endforeach()

add_executable(JangolizerTests
  PolyBLEPOscillatorTests.cpp
  PluginProcessorTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginEditor.cpp
)

target_include_directories(JangolizerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Source)

# ELK_HEADLESS=0: PluginProcessor::createEditor() constructs the real
# JangolizerAudioProcessorEditor, so PluginEditor.cpp must be compiled and linked
# in here too, even though no test exercises the editor directly.
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
  juce::juce_gui_basics
  juce::juce_recommended_config_flags
)

include(GoogleTest)
gtest_discover_tests(JangolizerTests)
