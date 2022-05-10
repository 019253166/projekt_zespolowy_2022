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

struct RotarySlider : juce::Slider
{
    RotarySlider() :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                     juce::Slider::TextEntryBoxPosition::NoTextBox){}
};

template<
    typename Attachment,
    typename APVTS,
    typename Params,
    typename ParamName,
    typename SliderType>

void makeAttachment(std::unique_ptr<Attachment>& attachment, APVTS& apvts, const Params& parameters, const ParamName& name, SliderType& slider)
{
    attachment = std::make_unique<Attachment>(apvts, parameters.at(name), slider);
}

struct GlobalControls : juce::Component
{
    GlobalControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;
private:
    RotarySlider inputGainSlider, outputGainSlider, lowCrossoverSlider, midCrossoverSlider, highCrossoverSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> inputGainSliderAttachment,
                                outputGainSliderAttachment,
                                lowCrossoverSliderAttachment,
                                midCrossoverSliderAttachment,
                                highCrossoverSliderAttachment;
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

    Placeholder /*globalControls,*/ bandLowControls, bandLowMidControls, bandHighMidControls, bandHighControls;
    GlobalControls globalControls {audioProcessor.apvts};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Projekt_zespoowy_2022AudioProcessorEditor)
};
