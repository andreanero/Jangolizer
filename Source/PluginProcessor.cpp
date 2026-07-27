#include "PluginProcessor.h"
#if !ELK_HEADLESS
#include "PluginEditor.h"
#endif

namespace
{
    // Fixed character constants (not exposed as parameters) that push the plugin
    // toward industrial / post-punk / dark-drone territory: a narrow, near-self-
    // oscillating filter resonance, and a dark mono feedback delay tail.
    constexpr float kFilterResonanceQ = 9.0f;

    constexpr float kDelayTimeSeconds = 0.35f;
    constexpr float kDelayFeedback = 0.4f;
    constexpr float kDelayWetLevel = 0.3f;
    constexpr float kMaxDelaySeconds = 1.0f;
}

// Input is always mono (single-channel guitar/instrument source, incl. the Elk Audio
// OS hardware target), so both buses are mono — no stereo channel to ever touch.
JangolizerAudioProcessor::JangolizerAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::mono(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

JangolizerAudioProcessor::~JangolizerAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout JangolizerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "SPEED", "LFO Speed", juce::NormalisableRange<float> (0.1f, 400.0f, 0.01f, 0.5f), 5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "DEPTH", "Modulation Depth", 0.0f, 1.0f, 0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "BIAS", "DC Bias Offset", -1.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "GAIN", "Input Drive", 1.0f, 10.0f, 1.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "WAVE", "LFO Waveform", juce::StringArray { "Square", "Triangle", "Sawtooth", "InvSawtooth", "Sine" }, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "VCA_MIX", "VCA Mix", 0.0f, 1.0f, 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "VCF_MIX", "VCF Mix", 0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "NOISE_LEVEL", "Noise Drone Level", 0.0f, 1.0f, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "BYPASS", "Bypass", true));

    return layout;
}

void JangolizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    lfo.setSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32 (samplesPerBlock);
    spec.numChannels = juce::uint32 (getTotalNumInputChannels());

    signalBandPassFilter.prepare (spec);
    signalBandPassFilter.reset();
    noiseBandPassFilter.prepare (spec);
    noiseBandPassFilter.reset();

    smoothedSpeed.reset (sampleRate, 0.02);
    smoothedDepth.reset (sampleRate, 0.02);
    smoothedBias.reset  (sampleRate, 0.02);
    smoothedGain.reset  (sampleRate, 0.02);
    smoothedVcaMix.reset (sampleRate, 0.02);
    smoothedVcfMix.reset (sampleRate, 0.02);
    smoothedNoiseLevel.reset (sampleRate, 0.02);

    noiseBuffer.setSize (1, samplesPerBlock);
    noiseDryBuffer.setSize (1, samplesPerBlock);
    signalDryBuffer.setSize (1, samplesPerBlock);
    lfoEnvelopeBuffer.setSize (1, samplesPerBlock);
    inputEnvelopeBuffer.setSize (1, samplesPerBlock);

    inputEnvelopeState = 0.0f;
    inputEnvAttackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.005));
    inputEnvReleaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.080));

    delaySamples = juce::jmax (1, (int) (sampleRate * kDelayTimeSeconds));
    delayBuffer.setSize (1, juce::jmax (1, (int) (sampleRate * kMaxDelaySeconds)));
    delayBuffer.clear();
    delayWritePos = 0;
}

void JangolizerAudioProcessor::releaseResources() {}

void JangolizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (*apvts.getRawParameterValue ("BYPASS") > 0.5f)
        return;

    smoothedSpeed.setTargetValue  (*apvts.getRawParameterValue ("SPEED"));
    smoothedDepth.setTargetValue  (*apvts.getRawParameterValue ("DEPTH"));
    smoothedBias.setTargetValue   (*apvts.getRawParameterValue ("BIAS"));
    smoothedGain.setTargetValue   (*apvts.getRawParameterValue ("GAIN"));
    smoothedVcaMix.setTargetValue (*apvts.getRawParameterValue ("VCA_MIX"));
    smoothedVcfMix.setTargetValue (*apvts.getRawParameterValue ("VCF_MIX"));
    smoothedNoiseLevel.setTargetValue (*apvts.getRawParameterValue ("NOISE_LEVEL"));

    int const wave = static_cast<int>(*apvts.getRawParameterValue ("WAVE"));

    lfo.setWaveform (static_cast<PolyBLEPOscillator::Waveform>(wave));

    int const numSamples = buffer.getNumSamples();
    auto* channelData = buffer.getWritePointer (0);

    // Stage 1: saturate, apply VCA (tremolo) blend, track modulation for the filter cutoff
    // and for the noise stage's two envelopes (LFO oscillator + input follower).
    float lastUnipolarMod = 0.0f;
    auto* lfoEnvelopeWrite = lfoEnvelopeBuffer.getWritePointer (0);
    auto* inputEnvelopeWrite = inputEnvelopeBuffer.getWritePointer (0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float const currentSpeed  = smoothedSpeed.getNextValue();
        float const currentDepth  = smoothedDepth.getNextValue();
        float const currentBias   = smoothedBias.getNextValue();
        float const currentGain   = smoothedGain.getNextValue();
        float const currentVcaMix = smoothedVcaMix.getNextValue();

        lfo.setFrequency (currentSpeed);
        lfo.advance();
        float const lfoSample = lfo.getSample();

        float modulation = (lfoSample * currentDepth) + currentBias;
        modulation = juce::jlimit (-1.0f, 1.0f, modulation);
        float const unipolarMod = (modulation + 1.0f) * 0.5f;
        lastUnipolarMod = unipolarMod;
        lfoEnvelopeWrite[sample] = unipolarMod;

        float const inputAbs = std::abs (channelData[sample]);
        float const followerCoeff = inputAbs > inputEnvelopeState ? inputEnvAttackCoeff : inputEnvReleaseCoeff;
        inputEnvelopeState += followerCoeff * (inputAbs - inputEnvelopeState);
        inputEnvelopeWrite[sample] = inputEnvelopeState;

        float const saturated = std::tanh (channelData[sample] * currentGain);

        channelData[sample] = juce::jmap (currentVcaMix, saturated, saturated * unipolarMod);
    }

    // Coefficients only take effect once, at each filter's process() call below, so
    // compute them once per block (from the block's final modulation value) instead of
    // per sample. Both filters share the same LFO-swept, narrow/near-self-oscillating
    // (high Q) coefficients but hold independent internal state.
    float const targetCutoff = 80.0f * std::pow (2.0f, lastUnipolarMod * 6.5f);
    auto const filterCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, targetCutoff, kFilterResonanceQ);
    *signalBandPassFilter.state = *filterCoeffs;
    *noiseBandPassFilter.state  = *filterCoeffs;

    // Stage 2: VCF applied directly to the entry guitar signal (auto-wah style sweep),
    // blended dry/wet via VCF_MIX so the resonant filter is only heard as VCF_MIX opens up.
    signalDryBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);

    {
        juce::dsp::AudioBlock<float> signalBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> signalContext (signalBlock);
        signalBandPassFilter.process (signalContext);
    }

    // Stage 3: noise-fed VCF (bandpass filter) drone. VCF_MIX shapes the drone itself
    // (raw noise vs. filtered/resonant noise); NOISE_LEVEL sets how loud the drone is
    // layered on top of the entry signal (additive, entry signal stays untouched).
    // The drone's amplitude is gated by the LFO oscillator envelope and by an envelope
    // follower on the input, so it pulses with the oscillator and tracks input loudness
    // instead of playing as a flat, input-independent hiss.
    {
        auto* noiseData = noiseBuffer.getWritePointer (0);
        for (int sample = 0; sample < numSamples; ++sample)
            noiseData[sample] = noiseRandom.nextFloat() * 2.0f - 1.0f;
    }

    noiseDryBuffer.copyFrom (0, 0, noiseBuffer, 0, 0, numSamples);

    {
        juce::dsp::AudioBlock<float> noiseBlock (noiseBuffer);
        juce::dsp::ProcessContextReplacing<float> noiseContext (noiseBlock);
        noiseBandPassFilter.process (noiseContext);
    }

    // Single mixing pass: both filters have already run over the whole block above, so
    // VCF_MIX (signal dry/wet + noise raw/filtered) and NOISE_LEVEL are each consumed
    // from their smoothers exactly once per sample here.
    auto const* drySignal = signalDryBuffer.getReadPointer (0);
    auto const* dryNoise = noiseDryBuffer.getReadPointer (0);
    auto const* filteredNoise = noiseBuffer.getReadPointer (0);
    auto const* lfoEnvelopeRead = lfoEnvelopeBuffer.getReadPointer (0);
    auto const* inputEnvelopeRead = inputEnvelopeBuffer.getReadPointer (0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float const currentVcfMix     = smoothedVcfMix.getNextValue();
        float const currentNoiseLevel = smoothedNoiseLevel.getNextValue();

        channelData[sample] = juce::jmap (currentVcfMix, drySignal[sample], channelData[sample]);

        float const drone = juce::jmap (currentVcfMix, dryNoise[sample], filteredNoise[sample]);
        float const noiseEnvelope = lfoEnvelopeRead[sample] * inputEnvelopeRead[sample];

        channelData[sample] += drone * currentNoiseLevel * noiseEnvelope;
    }

    // Stage 4: fixed mono feedback delay (no parameter) for a dark, cavernous sustain
    // tail suited to drone/post-punk dub washes. Output is added on top (wet, not
    // crossfaded away from dry).
    int const delayBufferSize = delayBuffer.getNumSamples();
    auto* delayData = delayBuffer.getWritePointer (0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        int const readPos = (delayWritePos - delaySamples + delayBufferSize) % delayBufferSize;
        float const delayed = delayData[readPos];

        delayData[delayWritePos] = channelData[sample] + delayed * kDelayFeedback;
        channelData[sample] += delayed * kDelayWetLevel;

        delayWritePos = (delayWritePos + 1) % delayBufferSize;
    }

    // Final safety soft-clip: resonant filters, the additive drone, and the feedback
    // delay can stack above unity, so tame the combined output before it leaves the plugin.
    for (int sample = 0; sample < numSamples; ++sample)
        channelData[sample] = std::tanh (channelData[sample]);
}

void JangolizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void JangolizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

    apvts.getParameter ("BYPASS")->setValueNotifyingHost (1.0f);
}

bool JangolizerAudioProcessor::hasEditor() const
{
    #if ELK_HEADLESS
    return false;
    #else
    return true;
    #endif
}

juce::AudioProcessorEditor* JangolizerAudioProcessor::createEditor()
{
    #if ELK_HEADLESS
    return nullptr;
    #else
    return new JangolizerAudioProcessorEditor (*this);
    #endif
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JangolizerAudioProcessor();
}
