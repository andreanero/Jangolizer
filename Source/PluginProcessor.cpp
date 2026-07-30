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

    // Noise drone character constants, tuned for a sunn O)))-style wall of low, fuzzed-out
    // amp noise rather than a bright synth-y hiss: the raw white noise is darkened toward
    // brown/pink with a leaky integrator, driven hard into tanh saturation for thick fuzz
    // harmonics, then shaped by its own low, moderately resonant low-pass (not the bright,
    // near-self-oscillating sweep used on the signal filter). Drive scales with the input
    // envelope (kNoiseDriveEnvScale) so the fuzz itself gets harder as you play louder,
    // instead of a fixed amount of saturation regardless of dynamics.
    constexpr float kNoiseColorLeak = 0.95f;
    constexpr float kNoiseDrive = 35.0f;
    constexpr float kNoiseDriveEnvScale = 5.0f;
    constexpr float kNoiseFilterResonanceQ = 5.0f;
    constexpr float kNoiseFilterMinHz = 60.0f;
    constexpr float kNoiseFilterOctaveRange = 3.0f;
    constexpr float kNoiseEnvelopeContrast = 1.6f;

    constexpr float kDelayTimeSeconds = 0.35f;
    constexpr float kDelayFeedback = 0.4f;
    constexpr float kDelayWetLevel = 0.3f;
    constexpr float kMaxDelaySeconds = 1.0f;
}

// Elk Audio OS's hardware I/O is a single-channel guitar/instrument source, so that
// build stays mono in/out. Everywhere else (desktop DAWs on Windows/macOS) runs
// stereo, with each channel processed through fully independent state (see the
// per-channel members in PluginProcessor.h) — no L/R mixing/crosstalk in the DSP.
JangolizerAudioProcessor::BusesProperties JangolizerAudioProcessor::makeBusesProperties()
{
#if ELK_HEADLESS
    auto const channels = juce::AudioChannelSet::mono();
#else
    auto const channels = juce::AudioChannelSet::stereo();
#endif
    return BusesProperties().withInput  ("Input",  channels, true)
                             .withOutput ("Output", channels, true);
}

JangolizerAudioProcessor::JangolizerAudioProcessor()
    : AudioProcessor (makeBusesProperties()),
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
    noiseLowPassFilter.prepare (spec);
    noiseLowPassFilter.reset();

    smoothedSpeed.reset (sampleRate, 0.02);
    smoothedDepth.reset (sampleRate, 0.02);
    smoothedBias.reset  (sampleRate, 0.02);
    smoothedGain.reset  (sampleRate, 0.02);
    smoothedVcaMix.reset (sampleRate, 0.02);
    smoothedVcfMix.reset (sampleRate, 0.02);
    smoothedNoiseLevel.reset (sampleRate, 0.02);

    int const numChannels = getTotalNumInputChannels();

    noiseBuffer.setSize (numChannels, samplesPerBlock);
    noiseDryBuffer.setSize (numChannels, samplesPerBlock);
    signalDryBuffer.setSize (numChannels, samplesPerBlock);
    lfoEnvelopeBuffer.setSize (1, samplesPerBlock);
    inputEnvelopeBuffer.setSize (numChannels, samplesPerBlock);

    inputEnvelopeState.fill (0.0f);
    noiseIntegratorState.fill (0.0f);
    // Slow attack/release so the drone swells and fades like a bowed-in amp wall rather
    // than snapping to each pick transient (sunn O)))-style volume swells, not tremolo).
    inputEnvAttackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.15));
    inputEnvReleaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * 1.5));

    delaySamples = juce::jmax (1, (int) (sampleRate * kDelayTimeSeconds));
    delayBuffer.setSize (numChannels, juce::jmax (1, (int) (sampleRate * kMaxDelaySeconds)));
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
    int const numChannels = buffer.getNumChannels();

    // Stage 1: saturate, apply VCA (tremolo) blend, track modulation for the filter cutoff
    // and for the noise stage's two envelopes (LFO oscillator + input follower). The dry
    // signal snapshot and the raw noise (both needed pre-filter, further down) are written
    // inline here too: same values as a post-loop copy/generate pass would produce, just
    // without the extra full-buffer traversals. The LFO/modulation is a single shared
    // control signal (computed once, not per channel); everything channel-specific
    // (input envelope, noise RNG, dry snapshots) runs through per-channel state instead,
    // so stereo channels never mix or influence each other.
    float lastUnipolarMod = 0.0f;
    auto* lfoEnvelopeWrite = lfoEnvelopeBuffer.getWritePointer (0);

    std::array<float*, 2> channelWritePtrs {};
    std::array<float*, 2> inputEnvelopeWritePtrs {};
    std::array<float*, 2> signalDryWritePtrs {};
    std::array<float*, 2> noiseWritePtrs {};
    std::array<float*, 2> noiseDryWritePtrs {};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        channelWritePtrs[channel]       = buffer.getWritePointer (channel);
        inputEnvelopeWritePtrs[channel] = inputEnvelopeBuffer.getWritePointer (channel);
        signalDryWritePtrs[channel]     = signalDryBuffer.getWritePointer (channel);
        noiseWritePtrs[channel]         = noiseBuffer.getWritePointer (channel);
        noiseDryWritePtrs[channel]      = noiseDryBuffer.getWritePointer (channel);
    }

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

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = channelWritePtrs[channel];

            float const inputAbs = std::abs (channelData[sample]);
            float const followerCoeff = inputAbs > inputEnvelopeState[channel] ? inputEnvAttackCoeff : inputEnvReleaseCoeff;
            inputEnvelopeState[channel] += followerCoeff * (inputAbs - inputEnvelopeState[channel]);
            inputEnvelopeWritePtrs[channel][sample] = inputEnvelopeState[channel];

            float const saturated = std::tanh (channelData[sample] * currentGain);

            channelData[sample] = juce::jmap (currentVcaMix, saturated, saturated * unipolarMod);
            signalDryWritePtrs[channel][sample] = channelData[sample];

            float const whiteNoise = noiseRandom[channel].nextFloat() * 2.0f - 1.0f;
            noiseIntegratorState[channel] = noiseIntegratorState[channel] * kNoiseColorLeak
                                             + whiteNoise * (1.0f - kNoiseColorLeak);
            float const noiseDrive = kNoiseDrive * (1.0f + inputEnvelopeState[channel] * kNoiseDriveEnvScale);
            float const noiseSample = std::tanh (noiseIntegratorState[channel] * noiseDrive);
            noiseWritePtrs[channel][sample] = noiseSample;
            noiseDryWritePtrs[channel][sample] = noiseSample;
        }
    }

    // Coefficients only take effect once, at each filter's process() call below, so
    // compute them once per block (from the block's final modulation value) instead of
    // per sample. Both filters are swept by the same LFO modulation value but with
    // independent coefficients and internal state: the signal filter stays a bright,
    // near-self-oscillating bandpass sweep (auto-wah); the noise filter is a low,
    // moderately resonant low-pass so the drone stays a thick low-end wall, not a shriek.
    float const targetCutoff = 80.0f * std::pow (2.0f, lastUnipolarMod * 6.5f);
    auto const signalFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, targetCutoff, kFilterResonanceQ);
    *signalBandPassFilter.state = *signalFilterCoeffs;

    float const noiseCutoff = kNoiseFilterMinHz * std::pow (2.0f, lastUnipolarMod * kNoiseFilterOctaveRange);
    auto const noiseFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, noiseCutoff, kNoiseFilterResonanceQ);
    *noiseLowPassFilter.state = *noiseFilterCoeffs;

    // Stage 2: VCF applied directly to the entry guitar signal (auto-wah style sweep),
    // blended dry/wet via VCF_MIX so the resonant filter is only heard as VCF_MIX opens up.
    {
        juce::dsp::AudioBlock<float> signalBlock (buffer);
        juce::dsp::ProcessContextReplacing<float> signalContext (signalBlock);
        signalBandPassFilter.process (signalContext);
    }

    // Stage 3: noise-fed low-pass drone. VCF_MIX shapes the drone itself (raw fuzzed
    // noise vs. filtered/resonant noise); NOISE_LEVEL sets how loud the drone is
    // layered on top of the entry signal (additive, entry signal stays untouched).
    // The drone's amplitude is gated by the LFO oscillator envelope and by a slow
    // envelope follower on the input, so it swells in and out with input loudness
    // instead of playing as a flat, input-independent hiss.
    {
        // noiseBuffer/delayBuffer are sized to the bus's max channel count (prepareToPlay),
        // which can exceed this block's actual channel count (e.g. a mono buffer fed
        // through the stereo desktop build in a test) — subset to the channels in play.
        auto noiseBlock = juce::dsp::AudioBlock<float> (noiseBuffer).getSubsetChannelBlock (0, (size_t) numChannels);
        juce::dsp::ProcessContextReplacing<float> noiseContext (noiseBlock);
        noiseLowPassFilter.process (noiseContext);
    }

    // Single fused pass over the block: mixing (VCF_MIX signal dry/wet + noise raw/filtered,
    // NOISE_LEVEL), the fixed per-channel feedback delay (stage 4 — dark/cavernous sustain
    // tail, wet added on top, not crossfaded away from dry), and the final safety soft-clip
    // (resonant filters + additive drone + delay can stack above unity). Each stage only
    // touches the current sample's already-finalized value, so folding them into one loop
    // is the same per-sample operation order as three separate passes, just one traversal.
    // VCF_MIX/NOISE_LEVEL and the delay read/write position are shared across channels
    // (same knob, same delay time); only the buffer contents themselves are per-channel.
    auto const* lfoEnvelopeRead = lfoEnvelopeBuffer.getReadPointer (0);
    int const delayBufferSize = delayBuffer.getNumSamples();

    std::array<float const*, 2> drySignalPtrs {};
    std::array<float const*, 2> dryNoisePtrs {};
    std::array<float const*, 2> filteredNoisePtrs {};
    std::array<float const*, 2> inputEnvelopeReadPtrs {};
    std::array<float*, 2> delayWritePtrs {};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        drySignalPtrs[channel]         = signalDryBuffer.getReadPointer (channel);
        dryNoisePtrs[channel]          = noiseDryBuffer.getReadPointer (channel);
        filteredNoisePtrs[channel]     = noiseBuffer.getReadPointer (channel);
        inputEnvelopeReadPtrs[channel] = inputEnvelopeBuffer.getReadPointer (channel);
        delayWritePtrs[channel]        = delayBuffer.getWritePointer (channel);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float const currentVcfMix     = smoothedVcfMix.getNextValue();
        float const currentNoiseLevel = smoothedNoiseLevel.getNextValue();
        float const lfoEnvelopeValue  = lfoEnvelopeRead[sample];

        int const readPos = (delayWritePos - delaySamples + delayBufferSize) % delayBufferSize;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = channelWritePtrs[channel];
            auto* delayData = delayWritePtrs[channel];

            channelData[sample] = juce::jmap (currentVcfMix, drySignalPtrs[channel][sample], channelData[sample]);

            float const drone = juce::jmap (currentVcfMix, dryNoisePtrs[channel][sample], filteredNoisePtrs[channel][sample]);
            float const rawNoiseEnvelope = lfoEnvelopeValue * inputEnvelopeReadPtrs[channel][sample];
            float const noiseEnvelope = std::pow (rawNoiseEnvelope, kNoiseEnvelopeContrast);

            channelData[sample] += drone * currentNoiseLevel * noiseEnvelope;

            float const delayed = delayData[readPos];

            delayData[delayWritePos] = channelData[sample] + delayed * kDelayFeedback;
            channelData[sample] += delayed * kDelayWetLevel;

            channelData[sample] = std::tanh (channelData[sample]);
        }

        delayWritePos = (delayWritePos + 1) % delayBufferSize;
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
