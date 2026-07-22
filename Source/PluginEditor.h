#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"

class RotarySliderLook : public juce::LookAndFeel_V4
{
public:
    RotarySliderLook();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

private:
    juce::Colour accentGreen {0xFF00FF00};
    juce::Colour darkBg {0xFF1a1a1a};
};

class JangolizerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Slider::Listener,
                                         public juce::ComboBox::Listener,
                                         public juce::Button::Listener
{
public:
    explicit JangolizerAudioProcessorEditor (JangolizerAudioProcessor&);
    ~JangolizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void sliderValueChanged (juce::Slider* slider) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;
    void buttonClicked (juce::Button* button) override;

private:
    JangolizerAudioProcessor& audioProcessor;

    RotarySliderLook rotaryLook;

    juce::Slider speedSlider, depthSlider, biasSlider, gainSlider;
    juce::Label speedLabel, depthLabel, biasLabel, gainLabel;

    juce::ComboBox waveformSelector, modeSelector;
    juce::Label waveformLabel, modeLabel;

    juce::ToggleButton bypassButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> biasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    void drawOwlEyes (juce::Graphics& g, int centerX, int centerY, float depth);
    void drawIndustrialBackground (juce::Graphics& g);
    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& name);
    void setupComboBox (juce::ComboBox& box, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JangolizerAudioProcessorEditor)
};
