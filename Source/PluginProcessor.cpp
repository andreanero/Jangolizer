#include "PluginProcessor.h"
#if !ELK_HEADLESS
#include "PluginEditor.h"
#endif

JangolizerAudioProcessor::JangolizerAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
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

    bandPassFilter.prepare (spec);
    bandPassFilter.reset();

    smoothedSpeed.reset (sampleRate, 0.02);
    smoothedDepth.reset (sampleRate, 0.02);
    smoothedBias.reset  (sampleRate, 0.02);
    smoothedGain.reset  (sampleRate, 0.02);
    smoothedVcaMix.reset (sampleRate, 0.02);
    smoothedVcfMix.reset (sampleRate, 0.02);
    smoothedNoiseLevel.reset (sampleRate, 0.02);

    noiseBuffer.setSize (2, samplesPerBlock);
    noiseDryBuffer.setSize (2, samplesPerBlock);
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
    auto* leftChannel = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);

    // Stage 1: saturate, apply VCA (tremolo) blend, track modulation for the filter cutoff.
    float lastUnipolarMod = 0.0f;

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

        float const saturatedL = std::tanh (leftChannel[sample] * currentGain);
        float const saturatedR = std::tanh (rightChannel[sample] * currentGain);

        leftChannel[sample]  = juce::jmap (currentVcaMix, saturatedL, saturatedL * unipolarMod);
        rightChannel[sample] = juce::jmap (currentVcaMix, saturatedR, saturatedR * unipolarMod);
    }

    // Coefficients only take effect once, at bandPassFilter.process() below, so compute
    // them once per block (from the block's final modulation value) instead of per sample.
    float const targetCutoff = 80.0f * std::pow (2.0f, lastUnipolarMod * 6.5f);
    *bandPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, targetCutoff, 2.5f);

    // Stage 2: noise-fed VCF (bandpass filter) drone. VCF_MIX shapes the drone itself
    // (raw noise vs. filtered/resonant noise); NOISE_LEVEL sets how loud the drone is
    // layered on top of the entry signal (additive, entry signal stays untouched).
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* noiseData = noiseBuffer.getWritePointer (ch);
        for (int sample = 0; sample < numSamples; ++sample)
            noiseData[sample] = noiseRandom.nextFloat() * 2.0f - 1.0f;
    }

    for (int ch = 0; ch < 2; ++ch)
        noiseDryBuffer.copyFrom (ch, 0, noiseBuffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> block (noiseBuffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    bandPassFilter.process (context);

    auto const* dryNoiseLeft  = noiseDryBuffer.getReadPointer (0);
    auto const* dryNoiseRight = noiseDryBuffer.getReadPointer (1);
    auto const* filteredNoiseLeft  = noiseBuffer.getReadPointer (0);
    auto const* filteredNoiseRight = noiseBuffer.getReadPointer (1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float const currentVcfMix     = smoothedVcfMix.getNextValue();
        float const currentNoiseLevel = smoothedNoiseLevel.getNextValue();

        float const droneL = juce::jmap (currentVcfMix, dryNoiseLeft[sample],  filteredNoiseLeft[sample]);
        float const droneR = juce::jmap (currentVcfMix, dryNoiseRight[sample], filteredNoiseRight[sample]);

        leftChannel[sample]  += droneL * currentNoiseLevel;
        rightChannel[sample] += droneR * currentNoiseLevel;
    }
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
