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

    // Bypass toggle
    bypassButton.setButtonText ("BYPASS");
    bypassButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xFF00FF00));
    bypassButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xFF00FF00));
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
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF00FF00));
    addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.attachToComponent (&slider, false);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
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
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xFF00FF00));
}

void JangolizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawIndustrialBackground (g);

    // Title, kept small so the owl eyes stay the star of the show
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xFF00FF00));
    g.drawFittedText ("JANGOLIZER", getLocalBounds().removeFromTop (36), juce::Justification::centredTop, 1);

    // Version tag
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::plain)));
    g.setColour (juce::Colour (0xFF666666));
    g.drawFittedText ("v" JucePlugin_VersionString, getLocalBounds().removeFromTop (36).removeFromRight (70).removeFromBottom (14),
                       juce::Justification::centredRight, 1);

    // Big, majestic owl eyes: the plugin's centerpiece and depth indicator
    auto depthNorm = (float) (depthSlider.getValue() / depthSlider.getMaximum());
    drawOwlEyes (g, getWidth() / 2, 186, depthNorm);

    // Bottom info
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::italic)));
    g.setColour (juce::Colour (0xFF00AA00));
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

void JangolizerAudioProcessorEditor::drawOwlEyes (juce::Graphics& g, int centerX, int centerY, float depth)
{
    // Big, majestic owl eyes as the plugin's centerpiece.
    auto eyeRadius   = 68.0f + (depth * 28.0f);
    auto eyeSpacing  = 98.0f;
    auto speedNorm   = (float) ((speedSlider.getValue() - 0.1f) / (400.0f - 0.1f));

    auto maskWidth  = eyeSpacing * 2.0f + eyeRadius * 2.3f;
    auto maskHeight = eyeRadius * 2.6f;
    auto maskLeft   = (float) centerX - maskWidth * 0.5f;
    auto maskTop    = (float) centerY - maskHeight * 0.5f;

    // Facial disc (unified feathered mask joining both eyes)
    juce::Path faceMask;
    faceMask.addRoundedRectangle (maskLeft, maskTop, maskWidth, maskHeight, maskHeight * 0.42f);
    juce::ColourGradient maskGradient (juce::Colour (0xFF3d3128), (float) centerX, maskTop,
                                       juce::Colour (0xFF17130f), (float) centerX, maskTop + maskHeight, false);
    g.setGradientFill (maskGradient);
    g.fillPath (faceMask);

    g.setColour (juce::Colour (0xFF544433));
    g.strokePath (faceMask, juce::PathStrokeType (2.0f));

    // Feather texture: subtle concentric radiating lines across the mask
    g.setColour (juce::Colour (0xFF2a2019).withAlpha (0.5f));
    for (int i = 1; i <= 5; ++i)
    {
        auto inset = (float) i * (maskHeight * 0.08f);
        juce::Path ring;
        ring.addRoundedRectangle (maskLeft + inset, maskTop + inset,
                                   maskWidth - inset * 2.0f, maskHeight - inset * 2.0f,
                                   (maskHeight - inset * 2.0f) * 0.42f);
        g.strokePath (ring, juce::PathStrokeType (0.6f));
    }

    // Ear tufts (great-horned-owl crown) for a fierce, majestic silhouette
    auto drawEarTuft = [&] (float sign)
    {
        juce::Path tuft;
        auto baseX = (float) centerX + sign * (maskWidth * 0.32f);
        auto baseY = maskTop + maskHeight * 0.12f;
        tuft.startNewSubPath (baseX - 16.0f, baseY);
        tuft.lineTo (baseX + sign * 10.0f, baseY - eyeRadius * 0.95f);
        tuft.lineTo (baseX + 20.0f, baseY);
        tuft.closeSubPath();
        juce::ColourGradient tuftGradient (juce::Colour (0xFF4a3a2a), baseX, baseY - eyeRadius,
                                            juce::Colour (0xFF1c1712), baseX, baseY, false);
        g.setGradientFill (tuftGradient);
        g.fillPath (tuft);
    };
    drawEarTuft (-1.0f);
    drawEarTuft (1.0f);

    // Beak, nestled beneath the eyes
    juce::Path beak;
    auto beakY = (float) centerY + eyeRadius * 0.55f;
    beak.addTriangle ((float) centerX - 10.0f, beakY,
                       (float) centerX + 10.0f, beakY,
                       (float) centerX, beakY + 20.0f);
    g.setColour (juce::Colour (0xFFCC7A1E));
    g.fillPath (beak);

    // One glowing, expressive eye
    auto drawEye = [&] (float sign)
    {
        auto eyeCentreX = (float) centerX + sign * eyeSpacing;
        auto eyeCentreY = (float) centerY;

        // Outer glow halo
        for (int i = 4; i >= 1; --i)
        {
            auto glowR = eyeRadius + (float) i * 6.0f;
            g.setColour (juce::Colour (0xFFFF8C00).withAlpha (0.05f));
            g.fillEllipse (eyeCentreX - glowR, eyeCentreY - glowR, glowR * 2.0f, glowR * 2.0f);
        }

        // Eye socket
        g.setColour (juce::Colour (0xFF0d0d0d));
        g.fillEllipse (eyeCentreX - eyeRadius, eyeCentreY - eyeRadius, eyeRadius * 2.0f, eyeRadius * 2.0f);

        // Iris, radial gradient for depth and glow
        auto irisRadius = eyeRadius * 0.86f;
        juce::ColourGradient irisGradient (juce::Colour (0xFFFFC94D), eyeCentreX, eyeCentreY,
                                            juce::Colour (0xFFB84E00), eyeCentreX, eyeCentreY - irisRadius, true);
        irisGradient.addColour (0.55, juce::Colour (0xFFFF8C00));
        g.setGradientFill (irisGradient);
        g.fillEllipse (eyeCentreX - irisRadius, eyeCentreY - irisRadius, irisRadius * 2.0f, irisRadius * 2.0f);

        // Fine iris striations
        g.setColour (juce::Colour (0xFF8A3D00).withAlpha (0.4f));
        for (int i = 0; i < 16; ++i)
        {
            auto a = (float) i / 16.0f * juce::MathConstants<float>::twoPi;
            juce::Line<float> spoke (eyeCentreX + std::cos (a) * irisRadius * 0.3f,
                                      eyeCentreY + std::sin (a) * irisRadius * 0.3f,
                                      eyeCentreX + std::cos (a) * irisRadius * 0.95f,
                                      eyeCentreY + std::sin (a) * irisRadius * 0.95f);
            g.drawLine (spoke, 1.0f);
        }

        // Pupil, reacts to speed and dilates with depth
        auto pupilRadius = irisRadius * (0.30f + depth * 0.12f);
        auto pupilOffsetX = (-1.0f + speedNorm * 2.0f) * (irisRadius - pupilRadius) * 0.6f;
        g.setColour (juce::Colours::black);
        g.fillEllipse (eyeCentreX + pupilOffsetX - pupilRadius, eyeCentreY - pupilRadius,
                       pupilRadius * 2.0f, pupilRadius * 2.0f);

        // Glint highlight
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        auto glintR = eyeRadius * 0.12f;
        g.fillEllipse (eyeCentreX + pupilOffsetX - glintR * 2.2f, eyeCentreY - irisRadius * 0.45f,
                       glintR, glintR);

        // Fierce upper eyelid, droops slightly deeper the more DEPTH is dialled in
        juce::Path eyelid;
        auto lidDrop = eyeRadius * (0.15f + depth * 0.25f);
        eyelid.startNewSubPath (eyeCentreX - eyeRadius * 1.05f, eyeCentreY - eyeRadius * 0.55f);
        eyelid.quadraticTo (eyeCentreX, eyeCentreY - eyeRadius - lidDrop,
                            eyeCentreX + eyeRadius * 1.05f, eyeCentreY - eyeRadius * 0.55f);
        eyelid.lineTo (eyeCentreX + eyeRadius * 1.05f, eyeCentreY - eyeRadius * 1.1f);
        eyelid.quadraticTo (eyeCentreX, eyeCentreY - eyeRadius * 1.75f - lidDrop,
                            eyeCentreX - eyeRadius * 1.05f, eyeCentreY - eyeRadius * 1.1f);
        eyelid.closeSubPath();
        g.setColour (juce::Colour (0xFF1c1712));
        g.fillPath (eyelid);
    };

    drawEye (-1.0f);
    drawEye (1.0f);
}

void JangolizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (36);
    area.removeFromTop (260); // reserved for the big owl eyes centerpiece

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
