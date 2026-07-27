#include "PluginProcessor.h"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    void setFloatParameter (juce::AudioProcessorValueTreeState& apvts, juce::String const& id, float normalisedValue)
    {
        auto* param = apvts.getParameter (id);
        ASSERT_NE (param, nullptr);
        param->setValueNotifyingHost (normalisedValue);
    }

    juce::AudioBuffer<float> makeTestBuffer (int numChannels, int numSamples, float value)
    {
        juce::AudioBuffer<float> buffer (numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (ch, i, value);
        return buffer;
    }
}

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

// The DSP path (PluginProcessor.cpp) is identical between the desktop and
// ELK_HEADLESS=1 configs -- only createEditor()/hasEditor() branch on the flag.
// These mirror the desktop-side PluginProcessorTests.cpp DSP tests so the
// noise-drone/VCA/VCF chain gets exercised under the headless compile too,
// not just the createEditor() stub above.
TEST (PluginProcessorHeadlessTest, ParameterLayoutHasExpectedDefaults)
{
    JangolizerAudioProcessor processor;

    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("SPEED"), 5.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("DEPTH"), 0.7f);
    EXPECT_NEAR (*processor.apvts.getRawParameterValue ("BIAS"), 0.0f, 1.0e-6f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("GAIN"), 1.0f);
    EXPECT_EQ (static_cast<int> (*processor.apvts.getRawParameterValue ("WAVE")), 1); // Triangle
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("VCA_MIX"), 1.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("VCF_MIX"), 0.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("NOISE_LEVEL"), 0.0f);
}

TEST (PluginProcessorHeadlessTest, ProcessBlockProducesFiniteBoundedOutputInVcaMode)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "VCA_MIX", 1.0f);
    setFloatParameter (processor.apvts, "VCF_MIX", 0.0f);
    setFloatParameter (processor.apvts, "NOISE_LEVEL", 0.0f);

    auto buffer = makeTestBuffer (1, 512, 0.5f);
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto const* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            ASSERT_FALSE (std::isnan (data[i]));
            ASSERT_FALSE (std::isinf (data[i]));
            ASSERT_LE (std::abs (data[i]), 1.0001f);
        }
    }
}

TEST (PluginProcessorHeadlessTest, ProcessBlockProducesFiniteBoundedOutputInNoiseMode)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "VCA_MIX", 0.0f);
    setFloatParameter (processor.apvts, "VCF_MIX", 0.0f);
    setFloatParameter (processor.apvts, "NOISE_LEVEL", 1.0f);

    auto buffer = makeTestBuffer (1, 512, 0.5f);
    juce::MidiBuffer midi;

    for (int block = 0; block < 4; ++block)
        processor.processBlock (buffer, midi);

    // NOISE_LEVEL is additive (drone layered on top of the entry signal, not
    // crossfaded), so the bound is entry (<=1) + raw noise (<=1) rather than 1.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto const* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            ASSERT_FALSE (std::isnan (data[i]));
            ASSERT_FALSE (std::isinf (data[i]));
            ASSERT_LE (std::abs (data[i]), 2.0001f);
        }
    }
}

TEST (PluginProcessorHeadlessTest, ProcessBlockProducesFiniteOutputWithAllStagesBlended)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "VCA_MIX", 0.6f);
    setFloatParameter (processor.apvts, "VCF_MIX", 0.5f);
    setFloatParameter (processor.apvts, "NOISE_LEVEL", 0.4f);

    auto buffer = makeTestBuffer (1, 512, 0.5f);
    juce::MidiBuffer midi;

    for (int block = 0; block < 4; ++block)
        processor.processBlock (buffer, midi);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto const* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            ASSERT_FALSE (std::isnan (data[i]));
            ASSERT_FALSE (std::isinf (data[i]));
        }
    }
}

TEST (PluginProcessorHeadlessTest, StateRoundTripsThroughGetAndSetStateInformation)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto* speedParam = processor.apvts.getParameter ("SPEED");
    speedParam->setValueNotifyingHost (0.9f);
    float const savedSpeed = *processor.apvts.getRawParameterValue ("SPEED");

    juce::MemoryBlock state;
    processor.getStateInformation (state);

    speedParam->setValueNotifyingHost (0.1f);
    ASSERT_NE (*processor.apvts.getRawParameterValue ("SPEED"), savedSpeed);

    processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("SPEED"), savedSpeed);
}
