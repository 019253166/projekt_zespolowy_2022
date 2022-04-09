/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct CompressorBand 
{
    juce::AudioParameterFloat* attack{ nullptr };
    juce::AudioParameterFloat* release{ nullptr };
    juce::AudioParameterFloat* threshold{ nullptr };
    juce::AudioParameterFloat* ratio{ nullptr };
    juce::AudioParameterBool* bypassed{ nullptr };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        compressor.prepare(spec);
    }

    void updateCompressorSettings()
    {
        compressor.setAttack(attack->get());
        compressor.setRelease(release->get());
        compressor.setThreshold(threshold->get());
        compressor.setRatio(ratio->get());
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        //przejmuje bufor i dodaje do niego próbki sygna³u
        auto context = juce::dsp::ProcessContextReplacing<float>(block);
        //odpowiada za wymianê próbek w buforze na te przetworzone przez kompresor

        context.isBypassed = bypassed->get();
        //jeœli bypass jest w³¹czony, nie rób nic

        compressor.process(context);
    }
private:
    juce::dsp::Compressor<float> compressor;
    
};
//==============================================================================
/**
*/
class Projekt_zespoowy_2022AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Projekt_zespoowy_2022AudioProcessor();
    ~Projekt_zespoowy_2022AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:

  //  juce::dsp::Compressor<float> compressor;

//    juce::AudioParameterFloat* attack{ nullptr };
 //   juce::AudioParameterFloat* release{ nullptr };
  //juce::AudioParameterFloat* threshold{ nullptr };
  //juce::AudioParameterFloat* ratio{ nullptr };
  //juce::AudioParameterBool* bypassed{ nullptr };

    CompressorBand compressor;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Projekt_zespoowy_2022AudioProcessor)
};
