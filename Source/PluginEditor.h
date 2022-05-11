/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix, const juce::String& title /*= "NO TITLE"*/) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setName(title);
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct PowerButton : juce::ToggleButton { };

struct AnalyzerButton : juce::ToggleButton
{
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto insetRect = bounds.reduced(4);

        randomPath.clear();

        juce::Random r;

        randomPath.startNewSubPath(insetRect.getX(),
            insetRect.getY() + insetRect.getHeight() * r.nextFloat());

        for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2)
        {
            randomPath.lineTo(x,
                insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        }
    }

    juce::Path randomPath;
};

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

template<
    typename APVTS,
    typename Params,
    typename Name>

juce::RangedAudioParameter& getParam(APVTS& apvts, const Params& params, const Name& name)
{
    auto param = apvts.getParameter(params.at(name));
    jassert(param != nullptr);
    return *param;
}

juce::String getValString(const juce::RangedAudioParameter& param, bool getLow, juce::String suffix);

template<
    typename Labels,
    typename ParamType,
    typename SuffixType>

void addLabelPairs(Labels& labels, const ParamType& param, const SuffixType& suffix)
{
    labels.clear();
    labels.add({ 0.f, getValString(param, true, suffix) });
    labels.add({ 1.f, getValString(param, false, suffix) });
}

struct BandControls : juce::Component
{
    BandControls();
    void resized() override;

private:
    RotarySlider attackSlider, releaseSlider, threshSlider, ratioSlider, kneeSlider;
};

struct GlobalControls : juce::Component
{
    GlobalControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;
private:
    using RSWL = RotarySliderWithLabels;
    std::unique_ptr<RSWL> inputGainSlider, outputGainSlider, lowCrossoverSlider, midCrossoverSlider, highCrossoverSlider;

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
