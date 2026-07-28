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

TEST (PluginProcessorTest, ParameterLayoutHasExpectedDefaults)
{
    JangolizerAudioProcessor processor;

    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("SPEED"), 5.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("DEPTH"), 0.7f);
    // BIAS's default snaps through NormalisableRange's 0.01 interval, which
    // isn't exactly representable in float; the resulting rounding error
    // differs slightly across platforms/compilers (e.g. FMA contraction on
    // Apple Silicon), so an exact equality check is too strict here.
    EXPECT_NEAR (*processor.apvts.getRawParameterValue ("BIAS"), 0.0f, 1.0e-6f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("GAIN"), 1.0f);
    EXPECT_EQ (static_cast<int> (*processor.apvts.getRawParameterValue ("WAVE")), 1); // Triangle
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("VCA_MIX"), 1.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("VCF_MIX"), 0.0f);
    EXPECT_FLOAT_EQ (*processor.apvts.getRawParameterValue ("NOISE_LEVEL"), 0.0f);
}

TEST (PluginProcessorTest, ProcessBlockProducesFiniteBoundedOutputInVcaMode)
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

TEST (PluginProcessorTest, ProcessBlockProducesFiniteOutputInVcfMode)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "VCA_MIX", 0.0f);
    setFloatParameter (processor.apvts, "VCF_MIX", 1.0f);
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
        }
    }
}

TEST (PluginProcessorTest, ProcessBlockProducesFiniteBoundedOutputInNoiseMode)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "VCA_MIX", 0.0f);
    setFloatParameter (processor.apvts, "VCF_MIX", 0.0f);
    setFloatParameter (processor.apvts, "NOISE_LEVEL", 1.0f);

    auto buffer = makeTestBuffer (1, 512, 0.5f);
    juce::MidiBuffer midi;

    // Run several blocks so the noise stage has run for a few blocks.
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

TEST (PluginProcessorTest, ProcessBlockProducesFiniteBoundedOutputWithAllStagesBlended)
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

// Desktop build's bus is stereo (Elk stays mono; see makeBusesProperties() in
// PluginProcessor.cpp). Each channel must get its own noise/envelope/delay state, so
// feeding the same input on both channels should still decorrelate the noise drone
// into an actual stereo image rather than a duplicated-mono signal.
TEST (PluginProcessorTest, ProcessBlockDecorrelatesStereoChannelsInNoiseMode)
{
    JangolizerAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    setFloatParameter (processor.apvts, "BYPASS", 0.0f);
    setFloatParameter (processor.apvts, "VCA_MIX", 0.0f);
    setFloatParameter (processor.apvts, "VCF_MIX", 0.0f);
    setFloatParameter (processor.apvts, "NOISE_LEVEL", 1.0f);

    auto buffer = makeTestBuffer (2, 512, 0.5f);
    juce::MidiBuffer midi;

    for (int block = 0; block < 4; ++block)
        processor.processBlock (buffer, midi);

    auto const* left = buffer.getReadPointer (0);
    auto const* right = buffer.getReadPointer (1);

    bool channelsDiffer = false;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        ASSERT_FALSE (std::isnan (left[i]));
        ASSERT_FALSE (std::isnan (right[i]));
        ASSERT_FALSE (std::isinf (left[i]));
        ASSERT_FALSE (std::isinf (right[i]));
        ASSERT_LE (std::abs (left[i]), 2.0001f);
        ASSERT_LE (std::abs (right[i]), 2.0001f);

        if (std::abs (left[i] - right[i]) > 1.0e-6f)
            channelsDiffer = true;
    }

    EXPECT_TRUE (channelsDiffer);
}

TEST (PluginProcessorTest, StateRoundTripsThroughGetAndSetStateInformation)
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
