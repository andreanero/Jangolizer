#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class PolyBLEPOscillator 
{
public:
    enum Waveform { Square, Triangle, Sawtooth, InvSawtooth, Sine };

    PolyBLEPOscillator() = default;

    void setSampleRate(double newSampleRate) { sampleRate = newSampleRate; }
    void setFrequency(float newFrequency) { frequency = newFrequency; }
    void setWaveform(Waveform newWaveform) { currentWaveform = newWaveform; }

    void advance() 
    {
        if (sampleRate > 0.0) {
            phase += static_cast<float>(frequency / sampleRate);
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    float getSample() 
    {
        float output = 0.0f;
        float dt = static_cast<float>(frequency / sampleRate);

        switch (currentWaveform) 
        {
            case Square:
                output = (phase < 0.5f) ? 1.0f : -1.0f;
                output += polyBlep(dt, phase);
                output -= polyBlep(dt, std::fmod(phase + 0.5f, 1.0f));
                break;

            case Sawtooth:
                output = (2.0f * phase) - 1.0f;
                output -= polyBlep(dt, phase);
                break;

            case InvSawtooth:
                output = 1.0f - (2.0f * phase);
                output += polyBlep(dt, phase);
                break;

            case Triangle:
                output = (phase < 0.5f) ? 1.0f : -1.0f;
                output += polyBlep(dt, phase);
                output -= polyBlep(dt, std::fmod(phase + 0.5f, 1.0f));
                integratedOutput = 0.99f * integratedOutput + (output * dt * 4.0f);
                return juce::jlimit(-1.0f, 1.0f, integratedOutput);

            case Sine:
                output = std::sin(phase * juce::MathConstants<float>::twoPi);
                break;
        }
        return output;
    }

private:
    float polyBlep(float dt, float t) 
    {
        if (t < dt) {
            t /= dt;
            return t + t - (t * t) - 1.0f;
        }
        else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return (t * t) + t + t + 1.0f;
        }
        return 0.0f;
    }

    double sampleRate = 44100.0;
    float frequency = 1.0f;
    float phase = 0.0f;
    float integratedOutput = 0.0f;
    Waveform currentWaveform = Triangle;
};
