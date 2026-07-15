#include "PolyBLEPOscillator.h"
#include <cmath>
#include <gtest/gtest.h>

namespace
{
    int countZeroCrossings (PolyBLEPOscillator::Waveform waveform, float frequency, double sampleRate, int numSamples)
    {
        PolyBLEPOscillator osc;
        osc.setSampleRate (sampleRate);
        osc.setFrequency (frequency);
        osc.setWaveform (waveform);

        int crossings = 0;
        float previous = 0.0f;
        bool first = true;

        for (int i = 0; i < numSamples; ++i)
        {
            osc.advance();
            float const sample = osc.getSample();

            if (! first && (previous < 0.0f) != (sample < 0.0f))
                ++crossings;

            previous = sample;
            first = false;
        }

        return crossings;
    }
}

class PolyBLEPOscillatorWaveformTest : public ::testing::TestWithParam<PolyBLEPOscillator::Waveform>
{
};

TEST_P (PolyBLEPOscillatorWaveformTest, OutputStaysFiniteAndInUnitRange)
{
    PolyBLEPOscillator osc;
    osc.setSampleRate (48000.0);
    osc.setFrequency (220.0f);
    osc.setWaveform (GetParam());

    for (int i = 0; i < 48000; ++i)
    {
        osc.advance();
        float const sample = osc.getSample();

        ASSERT_FALSE (std::isnan (sample));
        ASSERT_FALSE (std::isinf (sample));
        ASSERT_GE (sample, -1.0001f);
        ASSERT_LE (sample, 1.0001f);
    }
}

INSTANTIATE_TEST_SUITE_P (
    AllWaveforms,
    PolyBLEPOscillatorWaveformTest,
    ::testing::Values (PolyBLEPOscillator::Square,
                        PolyBLEPOscillator::Triangle,
                        PolyBLEPOscillator::Sawtooth,
                        PolyBLEPOscillator::InvSawtooth));

TEST (PolyBLEPOscillatorTest, HigherFrequencyProducesMoreZeroCrossingsInFixedWindow)
{
    double const sampleRate = 48000.0;
    int const window = 4800; // 0.1 seconds

    int const lowFreqCrossings = countZeroCrossings (PolyBLEPOscillator::Sawtooth, 20.0f, sampleRate, window);
    int const highFreqCrossings = countZeroCrossings (PolyBLEPOscillator::Sawtooth, 200.0f, sampleRate, window);

    EXPECT_GT (highFreqCrossings, lowFreqCrossings);
}

TEST (PolyBLEPOscillatorTest, ZeroFrequencyProducesNoZeroCrossings)
{
    int const crossings = countZeroCrossings (PolyBLEPOscillator::Square, 0.0f, 48000.0, 4800);
    EXPECT_EQ (crossings, 0);
}
