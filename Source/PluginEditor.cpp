/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

int windowWidth = 1000; 
int windowHeight = 600;

template<typename T>
bool truncateKiloValue(T& value)
{
    if (value > static_cast<T>(999))
    {
        value /= static_cast<T>(1000);
        return true;
    }
    return false;
}

juce::String getValString(const juce::RangedAudioParameter& param, bool getLow, juce::String suffix)
{
    juce::String str;

    auto val = getLow ? param.getNormalisableRange().start :
                        param.getNormalisableRange().end;
    bool useK = truncateKiloValue(val);
    str << val;
    if (useK) str << "k";
    str << suffix;
    return str;
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(Colours::black);
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colours::darkcyan : Colours::grey);
    g.drawEllipse(bounds, 3.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
    {
        Path powerButton;

        auto bounds = toggleButton.getLocalBounds();

        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f; //30.f;

        size -= 6;

        powerButton.addCentredArc(r.getCentreX(),
            r.getCentreY(),
            size * 0.5,
            size * 0.5,
            0.f,
            degreesToRadians(ang),
            degreesToRadians(360.f - ang),
            true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
    {
        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);

        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}
//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    auto bounds = getLocalBounds();

    g.setColour(Colours::white);
    g.drawFittedText(getName(), bounds.removeFromTop(getTextHeight() + 2).removeFromLeft(getWidth() / 2 - 15), Justification::topRight, 1);

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        static_cast<float>(jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0)),
        startAng,
        endAng,
        *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colours::white);
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(getTextHeight());

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 1.5;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    //r.setY(2);
    r.setY(bounds.getY());

    return r;

}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();

        addK = truncateKiloValue(val);

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; //this shouldn't happen!
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}

void RotarySliderWithLabels::changeParam(juce::RangedAudioParameter* p)
{
    param = p;
    repaint();
}

//==============================================================================

Placeholder::Placeholder()
{
    juce::Random r;
    customColour = juce::Colour(r.nextInt(255), r.nextInt(255), r.nextInt(255));
}

//==============================================================================

BandControls::BandControls(juce::AudioProcessorValueTreeState& apv) : apvts(apv),
attackLowSlider(nullptr, "ms", "Attack"),
releaseLowSlider(nullptr, "ms", "Release"),
threshLowSlider(nullptr, "dB", "Thresh"),
ratioLowSlider(nullptr, "", "Ratio"),
kneeLowSlider(nullptr, "", "Knee")
{
    using namespace Parameters;
    const auto& parameters = GetParameters();

    auto getParamHelper = [&parameters, &apvts = this->apvts](const auto& name) -> auto&
    {
        return getParam(apvts, parameters, name);
    };

    attackLowSlider.changeParam(&getParamHelper(Names::Attack_Low));
    releaseLowSlider.changeParam(&getParamHelper(Names::Release_Low));
    threshLowSlider.changeParam(&getParamHelper(Names::Threshold_Low));
    ratioLowSlider.changeParam(&getParamHelper(Names::Ratio_Low));
    kneeLowSlider.changeParam(&getParamHelper(Names::Knee_Low));

    addLabelPairs(attackLowSlider.labels, getParamHelper(Names::Attack_Low), "ms");
    addLabelPairs(releaseLowSlider.labels, getParamHelper(Names::Release_Low), "ms");
    addLabelPairs(threshLowSlider.labels, getParamHelper(Names::Threshold_Low), "dB");
    addLabelPairs(kneeLowSlider.labels, getParamHelper(Names::Knee_Low), "");

    ratioLowSlider.labels.add({ 0.f, "1:1" });
    ratioLowSlider.labels.add({ 1.f, "30:1" });

    auto makeAttachmentHelper = [&parameters, &apvts = this->apvts](auto& attachment, const auto& name, auto& slider)
    {
        makeAttachment(attachment, apvts, parameters, name, slider);
    };

    makeAttachmentHelper(attackLowSliderAttachment, Names::Attack_Low, attackLowSlider);
    makeAttachmentHelper(releaseLowSliderAttachment, Names::Release_Low, releaseLowSlider);
    makeAttachmentHelper(threshLowSliderAttachment, Names::Threshold_Low, threshLowSlider);
    makeAttachmentHelper(ratioLowSliderAttachment, Names::Ratio_Low, ratioLowSlider);
    makeAttachmentHelper(kneeLowSliderAttachment, Names::Knee_Low, kneeLowSlider);

    addAndMakeVisible(attackLowSlider);
    addAndMakeVisible(releaseLowSlider);
    addAndMakeVisible(threshLowSlider);
    addAndMakeVisible(ratioLowSlider);
    addAndMakeVisible(kneeLowSlider);
};

void BandControls::paint(juce::Graphics& g)
{
    using namespace juce;
    auto bounds = getLocalBounds();
    g.setColour(Colours::darkgrey);
    g.fillAll();

    bounds.reduce(1, 1);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 3);
}

void BandControls::resized()
{
    {
    auto bounds = getLocalBounds().reduced(4);
    using namespace juce;

    FlexBox flexRow1;
    flexRow1.flexDirection = FlexBox::Direction::row;
    flexRow1.flexWrap = FlexBox::Wrap::noWrap;

    auto spacer = FlexItem().withWidth(4);
    auto endCap = FlexItem().withWidth(6);

    bounds.removeFromTop(windowHeight * 5 / 24);

    flexRow1.items.add(endCap);
    flexRow1.items.add(FlexItem(attackLowSlider).withFlex(1.f));
    flexRow1.items.add(spacer);
    flexRow1.items.add(FlexItem(releaseLowSlider).withFlex(1.f));

    flexRow1.performLayout(bounds.removeFromTop(windowHeight * 5 / 24).reduced(5));

    FlexBox flexRow2;
    flexRow2.flexDirection = FlexBox::Direction::row;
    flexRow2.flexWrap = FlexBox::Wrap::noWrap;
    flexRow2.items.add(endCap);
    flexRow2.items.add(FlexItem(threshLowSlider).withFlex(1.f));
    flexRow2.items.add(spacer);
    flexRow2.items.add(FlexItem(ratioLowSlider).withFlex(1.f));

    flexRow2.performLayout(bounds.removeFromTop(windowHeight * 5 / 24).reduced(5));

    FlexBox flexRow3;
    flexRow3.flexDirection = FlexBox::Direction::row;
    flexRow3.flexWrap = FlexBox::Wrap::noWrap;
    flexRow3.items.add(endCap);
    flexRow3.items.add(FlexItem(kneeLowSlider).withFlex(1.f));

    flexRow3.performLayout(bounds.removeFromTop(windowHeight * 5 / 24).reduced(5));
    }
}

//==============================================================================

GlobalControls::GlobalControls(juce::AudioProcessorValueTreeState& apvts)
{
    using namespace Parameters;
    const auto& parameters = GetParameters();

    auto getParamHelper = [&parameters, &apvts](const auto& name) -> auto&
    {
        return getParam(apvts, parameters, name);
    };

    auto& inputGainParam = getParamHelper(Names::Input_Gain);
    auto& outputGainParam = getParamHelper(Names::Output_Gain);
    auto& lowCrossoverParam = getParamHelper(Names::Low_LowMid_Crossover_Freq);
    auto& midCrossoverParam = getParamHelper(Names::LowMid_HighMid_Crossover_Freq);
    auto& highCrossoverParam = getParamHelper(Names::HighMid_High_Crossover_Freq);

    inputGainSlider = std::make_unique<RSWL>(&inputGainParam, "dB", "Input Gain");
    outputGainSlider = std::make_unique<RSWL>(&outputGainParam, "dB", "Output Gain");
    lowCrossoverSlider = std::make_unique<RSWL>(&lowCrossoverParam, "Hz", "Low Crossover");
    midCrossoverSlider = std::make_unique<RSWL>(&midCrossoverParam, "Hz", "Mid Crossover");
    highCrossoverSlider = std::make_unique<RSWL>(&highCrossoverParam, "Hz", "High Crossover");

    auto makeAttachmentHelper = [&parameters, &apvts](auto& attachment, const auto& name, auto& slider)
    {
        makeAttachment(attachment, apvts, parameters, name, slider);
    };

    makeAttachmentHelper(inputGainSliderAttachment, Names::Input_Gain, *inputGainSlider);
    makeAttachmentHelper(outputGainSliderAttachment, Names::Output_Gain, *outputGainSlider);
    makeAttachmentHelper(lowCrossoverSliderAttachment, Names::Low_LowMid_Crossover_Freq, *lowCrossoverSlider);
    makeAttachmentHelper(midCrossoverSliderAttachment, Names::LowMid_HighMid_Crossover_Freq, *midCrossoverSlider);
    makeAttachmentHelper(highCrossoverSliderAttachment, Names::HighMid_High_Crossover_Freq, *highCrossoverSlider);

    addLabelPairs(inputGainSlider->labels, inputGainParam, "dB");
    addLabelPairs(outputGainSlider->labels, outputGainParam, "dB");
    addLabelPairs(lowCrossoverSlider->labels, lowCrossoverParam, "Hz");
    addLabelPairs(midCrossoverSlider->labels, midCrossoverParam, "Hz");
    addLabelPairs(highCrossoverSlider->labels, highCrossoverParam, "Hz");

    addAndMakeVisible(*inputGainSlider);
    addAndMakeVisible(*outputGainSlider);
    addAndMakeVisible(*lowCrossoverSlider);
    addAndMakeVisible(*midCrossoverSlider);
    addAndMakeVisible(*highCrossoverSlider);
}

void GlobalControls::paint(juce::Graphics& g)
{
    using namespace juce;
    auto bounds = getLocalBounds();
    g.setColour(Colours::darkgrey);
    g.fillAll();

    bounds.reduce(1, 3);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(bounds.toFloat(), 3);
}

void GlobalControls::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    using namespace juce;

    FlexBox flexBox;
    flexBox.flexDirection = FlexBox::Direction::row;
    flexBox.flexWrap = FlexBox::Wrap::noWrap;

    auto spacer = FlexItem().withWidth(4);
    auto endCap = FlexItem().withWidth(6);

    flexBox.items.add(endCap);
    flexBox.items.add(FlexItem(*inputGainSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(*lowCrossoverSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(*midCrossoverSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(*highCrossoverSlider).withFlex(1.f));
    flexBox.items.add(spacer);
    flexBox.items.add(FlexItem(*outputGainSlider).withFlex(1.f));

    flexBox.performLayout(bounds);
}

//==============================================================================
Projekt_zespoowy_2022AudioProcessorEditor::Projekt_zespoowy_2022AudioProcessorEditor (Projekt_zespoowy_2022AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setLookAndFeel(&lnf);

    addAndMakeVisible(globalControls);
    addAndMakeVisible(bandLowControls);
    addAndMakeVisible(bandLowMidControls);
    addAndMakeVisible(bandHighMidControls);
    addAndMakeVisible(bandHighControls);

    setSize (windowWidth, windowHeight);
}

Projekt_zespoowy_2022AudioProcessorEditor::~Projekt_zespoowy_2022AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}
 
//==============================================================================
void Projekt_zespoowy_2022AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
}

void Projekt_zespoowy_2022AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    globalControls.setBounds(bounds.removeFromTop(windowHeight / 6));
    bandLowControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandLowMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
}
