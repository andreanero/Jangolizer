#include "PluginProcessor.h"
#include <gtest/gtest.h>

// Compiled with ELK_HEADLESS=1 (see test/CMakeLists.txt). Regression guard for the
// elk-headless build: JUCE's AudioProcessor::createEditor() is pure virtual, so it must
// stay declared/defined in every build config or JangolizerAudioProcessor becomes
// abstract and fails to compile under ELK_HEADLESS_BUILD.
TEST (PluginProcessorHeadlessTest, HasNoEditorWhenHeadless)
{
    JangolizerAudioProcessor processor;

    EXPECT_FALSE (processor.hasEditor());
    EXPECT_EQ (processor.createEditor(), nullptr);
}
