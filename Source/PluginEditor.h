/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        //normalize the fft values.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            //            fftData[i] /= (float) numBins;
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getBottom();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 12.f,
                bottom, top);
        };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 4; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<Projekt_zespoowy_2022AudioProcessor::BlockType>& scsf) :
        leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }

    void updateNegativeInfinity(float nf) {
        negativeInfinity = nf;
    }
private:
    SingleChannelSampleFifo<Projekt_zespoowy_2022AudioProcessor::BlockType>* leftChannelFifo;

    juce::AudioBuffer<float> monoBuffer;

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathProducer;

    juce::Path leftChannelFFTPath;

    float negativeInfinity{ -48.f };
};

struct SpectrumAnalyzer : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    SpectrumAnalyzer(Projekt_zespoowy_2022AudioProcessor&);
    ~SpectrumAnalyzer();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
private:
    Projekt_zespoowy_2022AudioProcessor& audioProcessor;

    bool shouldShowFFTAnalysis = true;

    juce::Atomic<bool> parametersChanged{ false };

    //MonoChain monoChain;

    //void updateResponseCurve();

    //juce::Path responseCurve;

    //void updateChain();

    void drawBackgroundGrid(juce::Graphics& g);
    void drawTextLabels(juce::Graphics& g);

    std::vector<float> getFrequencies();
    std::vector<float> getGains();
    std::vector<float> getXs(const std::vector<float>& freqs, float left, float width);

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();

    PathProducer leftPathProducer, rightPathProducer;
 
    void drawCrossover(juce::Graphics& g);

    juce::AudioParameterFloat* lowCrossoverParam{ nullptr };
    juce::AudioParameterFloat* midCrossoverParam{ nullptr };
    juce::AudioParameterFloat* highCrossoverParam{ nullptr };

    juce::AudioParameterFloat* lowThresholdParam{ nullptr };
    juce::AudioParameterFloat* lowMidThresholdParam{ nullptr };
    juce::AudioParameterFloat* highMidThresholdParam{ nullptr };
    juce::AudioParameterFloat* highThresholdParam{ nullptr };
};

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
    RotarySliderWithLabels(juce::RangedAudioParameter* rap, const juce::String& unitSuffix, const juce::String& title /*= "NO TITLE"*/) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(rap),
        suffix(unitSuffix)
    {
        setName(title);
      //  setLookAndFeel(&lnf);
    }

    //~RotarySliderWithLabels()
    //{
    //    setLookAndFeel(nullptr);
    //}

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

    void changeParam(juce::RangedAudioParameter* p);

private:
    //LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct VerticalSliderWithLabels : juce::Slider
{
    VerticalSliderWithLabels(juce::RangedAudioParameter* rap, const juce::String& unitSuffix, const juce::String& title /*= "NO TITLE"*/) :
        juce::Slider(juce::Slider::SliderStyle::LinearVertical,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(rap),
        suffix(unitSuffix)
    {
        setName(title);
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

    void changeParam(juce::RangedAudioParameter* p);

private:

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct VerticalSlider : juce::Slider
{
    VerticalSlider() : 
        juce::Slider(juce::Slider::SliderStyle::LinearVertical,
                     juce::Slider::TextEntryBoxPosition::NoTextBox){}
};

struct PowerButton : juce::ToggleButton { };

struct MuteButton : juce::ToggleButton { };
struct SoloButton : juce::ToggleButton { };

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

//==============================================================================

struct BandControls : juce::Component
{
    BandControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    RotarySliderWithLabels attackLowSlider, releaseLowSlider, ratioLowSlider, kneeLowSlider;
    VerticalSlider threshLowSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> attackLowSliderAttachment,
                                releaseLowSliderAttachment,
                                threshLowSliderAttachment,
                                ratioLowSliderAttachment,
                                kneeLowSliderAttachment;

    MuteButton muteLowButton;
    SoloButton soloLowButton;
    juce::ToggleButton bypassLowButton;

    using BtnAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttachment> muteLowAttachment,
                                   soloLowAttachment,
                                   bypassLowAttachment;
};

struct BandLMControls : juce::Component
{
    BandLMControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    RotarySliderWithLabels attackLowMidSlider, releaseLowMidSlider, ratioLowMidSlider, kneeLowMidSlider;
    VerticalSlider threshLowMidSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment>
        attackLowMidSliderAttachment,
        releaseLowMidSliderAttachment,
        threshLowMidSliderAttachment,
        ratioLowMidSliderAttachment,
        kneeLowMidSliderAttachment;

    MuteButton muteLowMidButton;
    SoloButton soloLowMidButton;
    juce::ToggleButton bypassLowMidButton;

    using BtnAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttachment> muteLowMidAttachment,
        soloLowMidAttachment,
        bypassLowMidAttachment;
};

struct BandHMControls : juce::Component
{
    BandHMControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    RotarySliderWithLabels attackHighMidSlider, releaseHighMidSlider, ratioHighMidSlider, kneeHighMidSlider;
    VerticalSlider threshHighMidSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment>
        attackHighMidSliderAttachment,
        releaseHighMidSliderAttachment,
        threshHighMidSliderAttachment,
        ratioHighMidSliderAttachment,
        kneeHighMidSliderAttachment;

    MuteButton muteHighMidButton;
    SoloButton soloHighMidButton;
    juce::ToggleButton bypassHighMidButton;

    using BtnAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttachment> muteHighMidAttachment,
        soloHighMidAttachment,
        bypassHighMidAttachment;
};

struct BandHControls : juce::Component
{
    BandHControls(juce::AudioProcessorValueTreeState& apvts);

    void paint(juce::Graphics& g) override;

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    RotarySliderWithLabels attackHighSlider, releaseHighSlider, ratioHighSlider, kneeHighSlider;
    VerticalSlider threshHighSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> 
        attackHighSliderAttachment,
        releaseHighSliderAttachment,
        threshHighSliderAttachment,
        ratioHighSliderAttachment,
        kneeHighSliderAttachment;

    MuteButton muteHighButton;
    SoloButton soloHighButton;
    juce::ToggleButton bypassHighButton;

    using BtnAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttachment> muteHighAttachment,
        soloHighAttachment,
        bypassHighAttachment;
};

//==============================================================================

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
    LookAndFeel lnf;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Projekt_zespoowy_2022AudioProcessor& audioProcessor;

   // Placeholder /*globalControls,*//* bandLowControls,*/ bandLowMidControls, bandHighMidControls, bandHighControls;
    GlobalControls globalControls {audioProcessor.apvts};
    BandControls bandLowControls{ audioProcessor.apvts };
    BandLMControls bandLowMidControls{ audioProcessor.apvts };
    BandHMControls bandHighMidControls{ audioProcessor.apvts };
    BandHControls bandHighControls{ audioProcessor.apvts };

    SpectrumAnalyzer analyzer {audioProcessor};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Projekt_zespoowy_2022AudioProcessorEditor);
};
