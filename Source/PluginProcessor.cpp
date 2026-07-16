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

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "MODE", "Effect Mode", juce::StringArray { "VCA (Tremolo)", "VCF (Filter)" }, 0));

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
}

void JangolizerAudioProcessor::releaseResources() {}

void JangolizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (*apvts.getRawParameterValue ("BYPASS") > 0.5f)
        return;

    smoothedSpeed.setTargetValue (*apvts.getRawParameterValue ("SPEED"));
    smoothedDepth.setTargetValue (*apvts.getRawParameterValue ("DEPTH"));
    smoothedBias.setTargetValue  (*apvts.getRawParameterValue ("BIAS"));
    smoothedGain.setTargetValue  (*apvts.getRawParameterValue ("GAIN"));

    int const mode = static_cast<int>(*apvts.getRawParameterValue ("MODE"));
    int const wave = static_cast<int>(*apvts.getRawParameterValue ("WAVE"));

    lfo.setWaveform (static_cast<PolyBLEPOscillator::Waveform>(wave));

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    int const numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float const currentSpeed = smoothedSpeed.getNextValue();
        float const currentDepth = smoothedDepth.getNextValue();
        float const currentBias  = smoothedBias.getNextValue();
        float const currentGain  = smoothedGain.getNextValue();

        lfo.setFrequency (currentSpeed);
        lfo.advance();
        float const lfoSample = lfo.getSample();

        float modulation = (lfoSample * currentDepth) + currentBias;
        modulation = juce::jlimit (-1.0f, 1.0f, modulation);
        float const unipolarMod = (modulation + 1.0f) * 0.5f;

        float const saturatedL = std::tanh (leftChannel[sample] * currentGain);
        float const saturatedR = std::tanh (rightChannel[sample] * currentGain);

        if (mode == 0)
        {
            leftChannel[sample]  = saturatedL * unipolarMod;
            rightChannel[sample] = saturatedR * unipolarMod;
        }
        else
        {
            leftChannel[sample]  = saturatedL;
            rightChannel[sample] = saturatedR;

            float const targetCutoff = 80.0f * std::pow (2.0f, unipolarMod * 6.5f);
            
            *bandPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, targetCutoff, 2.5f);
        }
    }

    if (mode == 1)
    {
        bandPassFilter.process (context);
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

#if !ELK_HEADLESS
juce::AudioProcessorEditor* JangolizerAudioProcessor::createEditor()
{
    return new JangolizerAudioProcessorEditor (*this);
}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JangolizerAudioProcessor();
}
