#pragma once

#include <array>
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

    juce::AudioProcessorEditor* createEditor() override;
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
    // BusesProperties is protected on juce::AudioProcessor, so this must be a member
    // (not a free function) to be allowed to construct one.
    static BusesProperties makeBusesProperties();

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    PolyBLEPOscillator lfo;
    
    using FilterState = juce::dsp::IIR::Coefficients<float>;
    using FilterType = juce::dsp::IIR::Filter<float>;
    // Two independent filters, both swept by the same LFO modulation value but with their
    // own coefficients: signalBandPassFilter carves the entry guitar signal (auto-wah
    // style, blended via VCF_MIX); noiseLowPassFilter shapes the noise drone with a low,
    // resonant low-pass (sunn O)))-style amp-stack low end) instead of a bright sweep.
    juce::dsp::ProcessorDuplicator<FilterType, FilterState> signalBandPassFilter;
    juce::dsp::ProcessorDuplicator<FilterType, FilterState> noiseLowPassFilter;

    juce::LinearSmoothedValue<float> smoothedSpeed;
    juce::LinearSmoothedValue<float> smoothedDepth;
    juce::LinearSmoothedValue<float> smoothedBias;
    juce::LinearSmoothedValue<float> smoothedGain;
    juce::LinearSmoothedValue<float> smoothedVcaMix;
    juce::LinearSmoothedValue<float> smoothedVcfMix;
    juce::LinearSmoothedValue<float> smoothedNoiseLevel;

    // Wall-of-sound thickness: kNoiseLayers decorrelated noise voices per channel, each
    // colored to a different darkness (see kNoiseLayerLeaks in .cpp) and summed before
    // drive/filtering, so the drone reads as a dense broadband mass instead of a single
    // thin voice. On the stereo (desktop) build this also decorrelates the L/R noise
    // drone into a real stereo image instead of a duplicated-mono hiss; on the mono
    // (Elk) build only index 0 is ever touched.
    static constexpr int kNoiseLayers = 4;
    std::array<std::array<juce::Random, kNoiseLayers>, 2> noiseRandom;

    // Per-channel, per-layer one-pole leaky-integrator state: darkens each layer's raw
    // white noise toward its own brown/pink character before the layers are summed,
    // driven, and filtered (sunn O)))-style noise is thick and low, never a bright hiss).
    std::array<std::array<float, kNoiseLayers>, 2> noiseIntegratorState {};

    double currentSampleRate = 44100.0;

    // Per-channel: sized to the bus's channel count (1 on Elk, 2 on desktop) in
    // prepareToPlay. Filters are excluded — dsp::IIR::Filter already keeps independent
    // per-channel state internally once prepared with the right spec.numChannels.
    juce::AudioBuffer<float> noiseBuffer;
    juce::AudioBuffer<float> noiseDryBuffer;
    juce::AudioBuffer<float> signalDryBuffer;

    // Noise drone envelope: gated by the stage-1 LFO oscillator (machine pulses with
    // the tremolo, a shared mono control signal — not audio, so no per-channel copy
    // needed) AND by a per-channel envelope follower on that channel's own raw input
    // (drone tracks each channel's own loudness), so the wall of sound swells and
    // recedes with what's actually being played — visceral, input-reactive, never a
    // flat, input-independent hiss.
    juce::AudioBuffer<float> lfoEnvelopeBuffer;
    juce::AudioBuffer<float> inputEnvelopeBuffer;
    std::array<float, 2> inputEnvelopeState { 0.0f, 0.0f };
    float inputEnvAttackCoeff = 1.0f;
    float inputEnvReleaseCoeff = 1.0f;

    // Fixed (non-parameter) per-channel feedback delay: dark/cavernous sustain tail for
    // drone and post-punk dub-style washes, always blended in at a modest fixed amount.
    // Read/write position is shared (same delay time both channels advance together);
    // only the delay line contents themselves are per-channel.
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    int delaySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JangolizerAudioProcessor)
};
