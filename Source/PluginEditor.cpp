#include "PluginEditor.h"
//#include "BinaryData.h"

JangolizerAudioProcessorEditor::JangolizerAudioProcessorEditor (JangolizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Use generic editor as fallback with semi-transparent background
    genericEditor = std::make_unique<juce::GenericAudioProcessorEditor> (audioProcessor);
    addAndMakeVisible (genericEditor.get());
    
    setSize (800, 600);
}

JangolizerAudioProcessorEditor::~JangolizerAudioProcessorEditor() {}

void JangolizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (!backgroundImage.isNull())
    {
        // Draw background image scaled to fill the window
        g.drawImage (backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    }
    else
    {
        // Fallback: dark background
        g.fillAll (juce::Colour(0xFF1a1a1a));
    }
}

void JangolizerAudioProcessorEditor::resized()
{
    if (genericEditor)
        genericEditor->setBounds (getLocalBounds());
}
