#include "PluginEditor.h"
#include "BinaryData.h"

RotarySliderLook::RotarySliderLook()
{
    setColour (juce::Slider::rotarySliderFillColourId, accentOrange);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF333333));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, darkBg);
    setColour (juce::Slider::textBoxTextColourId, accentOrange);
    setColour (juce::Slider::thumbColourId, accentOrange);
}

void RotarySliderLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider&)
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
    g.setColour (accentOrange);
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

    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);

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

    // Bypass toggle
    bypassButton.setButtonText ("BYPASS");
    bypassButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xFFFF8C00));
    bypassButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xFFFF8C00));
    bypassButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xFF444444));
    bypassButton.addListener (this);
    addAndMakeVisible (bypassButton);

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
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "BYPASS", bypassButton);

    setSize (800, 680);
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
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFFFF8C00));
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&slider, false);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xFFFF8C00));
}

void JangolizerAudioProcessorEditor::setupComboBox (juce::ComboBox& box, juce::Label& label, const juce::String& name)
{
    box.setEditableText (false);
    box.setJustificationType (juce::Justification::centredLeft);
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF2a2a2a));
    box.setColour (juce::ComboBox::textColourId, juce::Colour (0xFFFF8C00));
    box.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF444444));
    box.setColour (juce::ComboBox::buttonColourId, juce::Colour (0xFFFF8C00));
    addAndMakeVisible (box);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.attachToComponent (&box, true);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xFFFF8C00));
}

void JangolizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    else
        drawIndustrialBackground (g);

    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xFFFF8C00));
    g.drawFittedText ("JANGOLIZER", getLocalBounds().removeFromTop (36), juce::Justification::centredTop, 1);

    // Version tag
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
    g.setColour (juce::Colour (0xFF3a2a1a));
    g.drawFittedText ("v" JucePlugin_VersionString, getLocalBounds().removeFromTop (36).removeFromRight (70).removeFromBottom (14),
                       juce::Justification::centredRight, 1);

    // Bottom info
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::italic)));
    g.setColour (juce::Colour (0xFFCC7000));
    g.drawFittedText ("NO SOUND. NO WAVE.", getLocalBounds().removeFromBottom (20), juce::Justification::centredBottom, 1);
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

void JangolizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (36);
    area.removeFromTop (260);

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

    // Bottom - Bypass toggle
    auto bypassRow = area.removeFromTop (30);
    bypassButton.setBounds (bypassRow.withSizeKeepingCentre (100, 30));
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

void JangolizerAudioProcessorEditor::buttonClicked (juce::Button*)
{
}
