/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
struct Placeholder : juce::Component
{
    Placeholder();
    void paint(juce::Graphics& g) override
    {
        g.fillAll(customColour);
    }
    juce::Colour customColour;
};
/**
*/
class Projekt_zespoowy_2022AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Projekt_zespoowy_2022AudioProcessorEditor (Projekt_zespoowy_2022AudioProcessor&);
    ~Projekt_zespoowy_2022AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Projekt_zespoowy_2022AudioProcessor& audioProcessor;

    Placeholder globalControls, bandLowControls, bandLowMidControls, bandHighMidControls, bandHighControls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Projekt_zespoowy_2022AudioProcessorEditor)
};
