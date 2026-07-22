#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PolyBLEPOscillator.h"

class JangolizerAudioProcessor  : public juce::AudioProcessor
{
public:
    JangolizerAudioProcessor();
    ~JangolizerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override { juce::ignoreUnused (this); }

    #if !ELK_HEADLESS
    juce::AudioProcessorEditor* createEditor() override;
    #endif
    bool hasEditor() const override;

    const juce::String getName() const override { return "Jangolizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    PolyBLEPOscillator lfo;
    
    using FilterState = juce::dsp::IIR::Coefficients<float>;
    using FilterType = juce::dsp::IIR::Filter<float>;
    juce::dsp::ProcessorDuplicator<FilterType, FilterState> bandPassFilter;

    juce::LinearSmoothedValue<float> smoothedSpeed;
    juce::LinearSmoothedValue<float> smoothedDepth;
    juce::LinearSmoothedValue<float> smoothedBias;
    juce::LinearSmoothedValue<float> smoothedGain;

    double currentSampleRate = 44100.0;

    static constexpr double kMaxReverseChunkSeconds = 2.0;
    juce::AudioBuffer<float> reverseBuffer;
    int reverseWritePos = 0;
    int reverseChunkLength = 0;
    int reverseChunkRemaining = 0;
    int reverseChunkStartPos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JangolizerAudioProcessor)
};
