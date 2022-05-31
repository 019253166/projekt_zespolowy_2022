/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

int windowWidth = 1000; 
int windowHeight = 600;

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer(Projekt_zespoowy_2022AudioProcessor& p) :
    audioProcessor(p),
    leftPathProducer(audioProcessor.leftChannelFifo),
    rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    using namespace Parameters;
    const auto& paramNames = GetParameters();
    auto floatHelper = [&apvts = audioProcessor.apvts, &paramNames](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramNames.at(parameterName)));
        jassert(parameter != nullptr);

    };

    floatHelper(lowCrossoverParam, Names::Low_LowMid_Crossover_Freq);
    floatHelper(midCrossoverParam, Names::LowMid_HighMid_Crossover_Freq);
    floatHelper(highCrossoverParam, Names::HighMid_High_Crossover_Freq);

    startTimerHz(60);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}


void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    drawBackgroundGrid(g);

    auto responseArea = getAnalysisArea();

    if (shouldShowFFTAnalysis)
    {
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), 0));

        g.setColour(Colour(97u, 18u, 167u)); //purple-
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), 0));

        g.setColour(Colour(215u, 201u, 134u));
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    //Path border;

    //border.setUsingNonZeroWinding(false);

    //border.addRoundedRectangle(getRenderArea(), 4);
    //border.addRectangle(getLocalBounds());

    //g.setColour(Colours::black);

    //g.fillPath(border);

    drawCrossover(g);

    drawTextLabels(g);

    //g.setcolour(colours::orange);
    //g.drawroundedrectangle(getrenderarea().tofloat(), 4.f, 1.f);
}

void SpectrumAnalyzer::drawCrossover(juce::Graphics& g)
{
    using namespace juce;
    auto renderArea = getAnalysisArea();

    const auto top = renderArea.getY();
    const auto bottom = renderArea.getBottom();

    auto mapX = [left = renderArea.getX(), width = renderArea.getWidth()](float f)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        return left + width * normX;
    };

    auto lowX = mapX(lowCrossoverParam->get());
    g.setColour(Colours::orange);
    g.drawVerticalLine(lowX, top, bottom);
    auto midX = mapX(midCrossoverParam->get());
    g.drawVerticalLine(midX, top, bottom);
    auto highX = mapX(highCrossoverParam->get());
    g.drawVerticalLine(highX, top, bottom);
}

std::vector<float> SpectrumAnalyzer::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
            200, /*300, 400,*/ 500, 1000,
            2000, /*3000, 4000,*/ 5000, 10000,
            20000
    };
}

std::vector<float> SpectrumAnalyzer::getGains()
{
    return std::vector<float>
    {
        -48, -36, -24, -12, 0, 12
    };
}

std::vector<float> SpectrumAnalyzer::getXs(const std::vector<float>& freqs, float left, float width)
{
    std::vector<float> xs;
    for (auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back(left + width * normX);
    }

    return xs;
}

void SpectrumAnalyzer::drawBackgroundGrid(juce::Graphics& g)
{
    using namespace juce;
    auto freqs = getFrequencies();

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto xs = getXs(freqs, left, width);

    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -48.f, 12.f, float(bottom), float(top));

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
}

void SpectrumAnalyzer::drawTextLabels(juce::Graphics& g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();

    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    auto freqs = getFrequencies();
    auto xs = getXs(freqs, left, width);

    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    auto gain = getGains();

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -48.f, 12.f, float(bottom), float(top));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);

        //str.clear();
        //str << (gDb - 24.f);

        r.setX(1);
        //textWidth = g.getCurrentFont().getStringWidth(str);
        //r.setSize(textWidth, fontHeight);
        //g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void SpectrumAnalyzer::resized()
{
    using namespace juce;
    auto fftBounds = getAnalysisArea().toFloat();
    auto negInf = jmap(getLocalBounds().toFloat().getBottom(), fftBounds.getBottom(), fftBounds.getY(), -48.f, 12.f);
    leftPathProducer.updateNegativeInfinity(negInf);
    rightPathProducer.updateNegativeInfinity(negInf);
}

void SpectrumAnalyzer::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size = tempIncomingBuffer.getNumSamples();

            jassert(size <= monoBuffer.getNumSamples());
            size = juce::jmin(size, monoBuffer.getNumSamples());

            auto writePointer = monoBuffer.getWritePointer(0, 0);
            auto readPointer = monoBuffer.getReadPointer(0, size);

            std::copy(readPointer, readPointer + (monoBuffer.getNumSamples() - size), writePointer);

            //juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
            //    monoBuffer.getReadPointer(0, size),
            //    monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, negativeInfinity);
        }
    }

    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, negativeInfinity);
        }
    }

    while (pathProducer.getNumPathsAvailable() > 0)
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void SpectrumAnalyzer::timerCallback()
{
    if (shouldShowFFTAnalysis)
    {
        auto fftBounds = getAnalysisArea().toFloat();
        fftBounds.setBottom(getLocalBounds().getBottom());
        auto sampleRate = audioProcessor.getSampleRate();

        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if (parametersChanged.compareAndSetBool(false, true))
    {
    }

    repaint();
}

juce::Rectangle<int> SpectrumAnalyzer::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}


juce::Rectangle<int> SpectrumAnalyzer::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
//==============================================================================

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
//threshLowSlider(nullptr, "dB", "Thresh"),
ratioLowSlider(nullptr, ":1", "Ratio"),
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
    //threshLowSlider.changeParam(&getParamHelper(Names::Threshold_Low));
    ratioLowSlider.changeParam(&getParamHelper(Names::Ratio_Low));
    kneeLowSlider.changeParam(&getParamHelper(Names::Knee_Low));

    addLabelPairs(attackLowSlider.labels, getParamHelper(Names::Attack_Low), "ms");
    addLabelPairs(releaseLowSlider.labels, getParamHelper(Names::Release_Low), "ms");
    //addLabelPairs(threshLowSlider.labels, getParamHelper(Names::Threshold_Low), "dB");
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

    flexRow1.items.add(endCap);
    flexRow1.items.add(FlexItem(attackLowSlider).withFlex(1.f));
    flexRow1.items.add(spacer);
    flexRow1.items.add(FlexItem(releaseLowSlider).withFlex(1.f));

    flexRow1.performLayout(bounds.removeFromTop(windowHeight * 5 / 30).reduced(5));

    FlexBox flexRow2;
    flexRow2.flexDirection = FlexBox::Direction::row;
    flexRow2.flexWrap = FlexBox::Wrap::noWrap;
    flexRow2.items.add(endCap);
    flexRow2.items.add(FlexItem(threshLowSlider).withFlex(1.f));
    flexRow2.items.add(spacer);
    flexRow2.items.add(FlexItem(ratioLowSlider).withFlex(1.f));

    flexRow2.performLayout(bounds.removeFromTop(windowHeight * 5 / 30).reduced(5));

    FlexBox flexRow3;
    flexRow3.flexDirection = FlexBox::Direction::row;
    flexRow3.flexWrap = FlexBox::Wrap::noWrap;
    flexRow3.items.add(endCap);
    flexRow3.items.add(FlexItem(kneeLowSlider).withFlex(1.f));

    flexRow3.performLayout(bounds.removeFromTop(windowHeight * 5 / 30).reduced(5));

    //FlexBox flexColumn1;
    //flexColumn1.flexDirection = FlexBox::Direction::row;
    //flexColumn1.flexWrap = FlexBox::Wrap::noWrap;


    //bounds.removeFromTop(windowHeight * 5 / 24);

    ////flexColumn1.items.add(endCap);
    //flexColumn1.items.add(FlexItem(attackLowSlider).withFlex(1.f));
    ////flexColumn1.items.add(spacer);
    //flexColumn1.items.add(FlexItem(ratioLowSlider).withFlex(1.f));

    //FlexBox flexColumn2;
    //flexColumn2.flexDirection = FlexBox::Direction::column;
    //flexColumn2.flexWrap = FlexBox::Wrap::noWrap;
    ////flexColumn2.items.add(endCap);
    //flexColumn2.items.add(FlexItem(releaseLowSlider).withFlex(1.f));
    ////flexColumn2.items.add(spacer);
    //flexColumn2.items.add(FlexItem(kneeLowSlider).withFlex(1.f));

    ////flexColumn2.performLayout(bounds.removeFromTop(windowHeight * 5 / 24).reduced(5));
    //FlexBox flexRow;

    //flexRow.items.add(FlexItem(threshLowSlider).withWidth(getWidth() / 4.0f));
    //flexRow.items.add(flexColumn1);
    //flexRow.items.add(flexColumn2);
    //flexRow.performLayout(bounds);
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
    addAndMakeVisible(analyzer);

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
    analyzer.setBounds(bounds.removeFromTop(windowHeight / 3));
    bandLowControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandLowMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighMidControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
    bandHighControls.setBounds(bounds.removeFromLeft(windowWidth / 4));
}
