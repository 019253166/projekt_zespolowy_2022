/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

int windowWidth = 1000;

Placeholder::Placeholder()
{
    juce::Random r;
    customColour = juce::Colour(r.nextInt(255), r.nextInt(255), r.nextInt(255));
}

//==============================================================================

GlobalControls::GlobalControls(juce::AudioProcessorValueTreeState& apvts)
{
    using namespace Parameters;
    const auto& parameters = GetParameters();

    auto makeAttachmentHelper = [&parameters, &apvts](auto& attachment, const auto& name, auto& slider)
    {
        makeAttachment(attachment, apvts, parameters, name, slider);
    };

    makeAttachmentHelper(inputGainSliderAttachment, Names::Input_Gain, inputGainSlider);
    makeAttachmentHelper(outputGainSliderAttachment, Names::Output_Gain, outputGainSlider);
    makeAttachmentHelper(lowCrossoverSliderAttachment, Names::Low_LowMid_Crossover_Freq, lowCrossoverSlider);
    makeAttachmentHelper(midCrossoverSliderAttachment, Names::LowMid_HighMid_Crossover_Freq, midCrossoverSlider);
    makeAttachmentHelper(highCrossoverSliderAttachment, Names::HighMid_High_Crossover_Freq, highCrossoverSlider);

    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(outputGainSlider);
    addAndMakeVisible(lowCrossoverSlider);
    addAndMakeVisible(midCrossoverSlider);
    addAndMakeVisible(highCrossoverSlider);
}

void GlobalControls::paint(juce::Graphics& g)
{
    using namespace juce;
    auto bounds = getLocalBounds();
    g.setColour(Colours::darkgrey);
    g.fillAll();

    bounds.reduce(3, 3);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 3);
}

void GlobalControls::resized()
{
    auto bounds = getLocalBounds();
    using namespace juce;

    FlexBox flexBox;
    flexBox.flexDirection = FlexBox::Direction::row;
    flexBox.flexWrap = FlexBox::Wrap::noWrap;

    flexBox.items.add(FlexItem(inputGainSlider).withFlex(1.f));
    flexBox.items.add(FlexItem(lowCrossoverSlider).withFlex(1.f));
    flexBox.items.add(FlexItem(midCrossoverSlider).withFlex(1.f));
    flexBox.items.add(FlexItem(highCrossoverSlider).withFlex(1.f));
    flexBox.items.add(FlexItem(outputGainSlider).withFlex(1.f));

    flexBox.performLayout(bounds);
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
    setSize (windowWidth, 600);
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
    bandLowControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandLowMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
}
