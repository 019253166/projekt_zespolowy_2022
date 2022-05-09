/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

Placeholder::Placeholder()
{
    juce::Random r;
    customColour = juce::Colour(r.nextInt(255), r.nextInt(255), r.nextInt(255));
}
//==============================================================================
Projekt_zespoowy_2022AudioProcessorEditor::Projekt_zespoowy_2022AudioProcessorEditor (Projekt_zespoowy_2022AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible(globalControls);
    addAndMakeVisible(bandLowControls);
    addAndMakeVisible(bandLowMidControls);
    addAndMakeVisible(bandHighMidControls);
    addAndMakeVisible(bandHighControls);
    setSize (800, 500);
}

Projekt_zespoowy_2022AudioProcessorEditor::~Projekt_zespoowy_2022AudioProcessorEditor()
{
}

//==============================================================================
void Projekt_zespoowy_2022AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void Projekt_zespoowy_2022AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    globalControls.setBounds(bounds.removeFromTop(64));
    bandLowControls.setBounds(bounds.removeFromLeft(200));
    bandLowMidControls.setBounds(bounds.removeFromLeft(200));
    bandHighMidControls.setBounds(bounds.removeFromLeft(200));
    bandHighControls.setBounds(bounds.removeFromLeft(200));
}
