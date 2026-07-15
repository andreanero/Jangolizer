#include "PluginEditor.h"

RotarySliderLook::RotarySliderLook()
{
    setColour (juce::Slider::rotarySliderFillColourId, accentGreen);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF333333));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, darkBg);
    setColour (juce::Slider::textBoxTextColourId, accentGreen);
    setColour (juce::Slider::thumbColourId, accentGreen);
}

void RotarySliderLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float) juce::jmin (width / 2, height / 2) - 2.0f;
    auto centreX = (float) x + (float) width * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour (juce::Colour (0xFF2a2a2a));
    g.fillEllipse (rx, ry, rw, rw);

    // Outline
    g.setColour (juce::Colour (0xFF444444));
    g.drawEllipse (rx, ry, rw, rw, 2.0f);

    // Fill arc
    juce::Path filledArc;
    filledArc.addCentredArc (centreX, centreY, radius, radius,
                            0.0f, rotaryStartAngle, angle, true);
    g.setColour (accentGreen);
    g.strokePath (filledArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointer;
    auto pointerLength = radius * 0.33f;
    auto pointerThickness = 2.5f;
    pointer.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    pointer.applyTransform (juce::AffineTransform::rotation (angle, centreX, centreY));
    g.fillPath (pointer);
}

//==============================================================================

JangolizerAudioProcessorEditor::JangolizerAudioProcessorEditor (JangolizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&rotaryLook);

    // Setup sliders
    setupSlider (speedSlider, speedLabel, "SPEED");
    setupSlider (depthSlider, depthLabel, "DEPTH");
    setupSlider (biasSlider, biasLabel, "BIAS");
    setupSlider (gainSlider, gainLabel, "GAIN");

    speedSlider.setRange (0.1f, 400.0f, 0.01f);
    speedSlider.setSkewFactorFromMidPoint (5.0f);
    depthSlider.setRange (0.0f, 1.0f, 0.01f);
    biasSlider.setRange (-1.0f, 1.0f, 0.01f);
    gainSlider.setRange (1.0f, 10.0f, 0.01f);

    // Setup combo boxes
    setupComboBox (waveformSelector, waveformLabel, "WAVEFORM");
    setupComboBox (modeSelector, modeLabel, "MODE");

    waveformSelector.addItemList ({"Square", "Triangle", "Sawtooth", "Inv-Sawtooth", "Sine"}, 1);
    modeSelector.addItemList ({"VCA (Tremolo)", "VCF (Filter)"}, 1);

    waveformSelector.addListener (this);
    modeSelector.addListener (this);

    // Create attachments
    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "SPEED", speedSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "DEPTH", depthSlider);
    biasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "BIAS", biasSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "GAIN", gainSlider);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "WAVE", waveformSelector);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "MODE", modeSelector);

    setSize (800, 600);
}

JangolizerAudioProcessorEditor::~JangolizerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void JangolizerAudioProcessorEditor::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xFF1a1a1a));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF00FF00));
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&slider, false);
    label.setFont (juce::Font (11.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xFF00FF00));
}

void JangolizerAudioProcessorEditor::setupComboBox (juce::ComboBox& box, juce::Label& label, const juce::String& name)
{
    box.setEditableText (false);
    box.setJustificationType (juce::Justification::centredLeft);
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF2a2a2a));
    box.setColour (juce::ComboBox::textColourId, juce::Colour (0xFF00FF00));
    box.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF444444));
    box.setColour (juce::ComboBox::buttonColourId, juce::Colour (0xFF00FF00));
    addAndMakeVisible (box);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.attachToComponent (&box, true);
    label.setFont (juce::Font (11.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xFF00FF00));
}

void JangolizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawIndustrialBackground (g);

    // Title
    g.setFont (juce::Font (28.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xFF00FF00));
    g.drawFittedText ("JANGOLIZER", getLocalBounds().removeFromTop (60), juce::Justification::centredTop, 1);

    // Draw stylized cat eyes as visual feedback (depth indicator)
    auto depthNorm = (float) depthSlider.getValue() / depthSlider.getMaximum();
    drawCatEyes (g, getWidth() / 2, 35, depthNorm);

    // Bottom info
    g.setFont (juce::Font (10.0f, juce::Font::italic));
    g.setColour (juce::Colour (0xFF00AA00));
    g.drawFittedText ("NO SUN. NO SYNTH.", getLocalBounds().removeFromBottom (20), juce::Justification::centredBottom, 1);
}

void JangolizerAudioProcessorEditor::drawIndustrialBackground (juce::Graphics& g)
{
    // Main dark background
    g.fillAll (juce::Colour (0xFF0f0f0f));

    // Grungy texture effect (noise lines)
    juce::Random random (12345);
    for (int i = 0; i < 300; ++i)
    {
        int x = random.nextInt (getWidth());
        int y = random.nextInt (getHeight());
        int w = random.nextInt (50) + 10;
        int h = random.nextInt (2) + 1;
        float alpha = random.nextFloat() * 0.08f;
        g.setColour (juce::Colour (0x00000000).withAlpha (alpha));
        g.drawLine ((float) x, (float) y, (float) (x + w), (float) y, (float) h);
    }

    // Border highlight
    g.setColour (juce::Colour (0xFF222222));
    g.drawRect (getLocalBounds(), 2);
}

void JangolizerAudioProcessorEditor::drawCatEyes (juce::Graphics& g, int centerX, int centerY, float depth)
{
    auto eyeRadius = 6.0f + (depth * 4.0f);
    auto eyeSpacing = 15.0f;

    // Left eye
    g.setColour (juce::Colour (0xFF1a1a1a));
    g.fillEllipse ((float) (centerX - eyeSpacing - eyeRadius), (float) (centerY - eyeRadius),
                   eyeRadius * 2.0f, eyeRadius * 2.0f);

    g.setColour (juce::Colour (0xFF00FF00));
    g.fillEllipse ((float) (centerX - eyeSpacing - eyeRadius * 0.6f), (float) (centerY - eyeRadius * 0.5f),
                   eyeRadius * 1.2f, eyeRadius * 1.5f);

    // Right eye
    g.setColour (juce::Colour (0xFF1a1a1a));
    g.fillEllipse ((float) (centerX + eyeSpacing - eyeRadius), (float) (centerY - eyeRadius),
                   eyeRadius * 2.0f, eyeRadius * 2.0f);

    g.setColour (juce::Colour (0xFF00FF00));
    g.fillEllipse ((float) (centerX + eyeSpacing - eyeRadius * 0.6f), (float) (centerY - eyeRadius * 0.5f),
                   eyeRadius * 1.2f, eyeRadius * 1.5f);

    // Pupils (respond to speed parameter)
    auto speedNorm = (float) ((speedSlider.getValue() - 0.1f) / (400.0f - 0.1f));
    auto pupilX = -2.0f + (speedNorm * 4.0f);
    auto pupilY = -1.0f;

    g.setColour (juce::Colours::black);
    g.fillEllipse ((float) (centerX - eyeSpacing + pupilX - 1.5f), (float) (centerY + pupilY - 1.5f), 3.0f, 3.0f);
    g.fillEllipse ((float) (centerX + eyeSpacing + pupilX - 1.5f), (float) (centerY + pupilY - 1.5f), 3.0f, 3.0f);
}

void JangolizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (70);

    // Top row - Sliders
    auto sliderRow = area.removeFromTop (160);
    auto sliderWidth = sliderRow.getWidth() / 4;

    speedSlider.setBounds (sliderRow.removeFromLeft (sliderWidth).reduced (5));
    depthSlider.setBounds (sliderRow.removeFromLeft (sliderWidth).reduced (5));
    biasSlider.setBounds (sliderRow.removeFromLeft (sliderWidth).reduced (5));
    gainSlider.setBounds (sliderRow.removeFromLeft (sliderWidth).reduced (5));

    // Middle section - Selectors
    auto selectorRow = area.removeFromTop (80);
    auto leftCol = selectorRow.removeFromLeft (selectorRow.getWidth() / 2).reduced (10);
    auto rightCol = selectorRow.reduced (10);

    waveformSelector.setBounds (leftCol.removeFromBottom (30));
    modeSelector.setBounds (rightCol.removeFromBottom (30));
}

void JangolizerAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &speedSlider || slider == &depthSlider)
        repaint();
}

void JangolizerAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == &waveformSelector || comboBox == &modeSelector)
        repaint();
}
