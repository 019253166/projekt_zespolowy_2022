/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace Parameters
{
	enum Names //uporządkowanie parametrów kompresorów po filtracji
	{
		//częstotliwości graniczne
		Low_LowMid_Crossover_Freq, 
		LowMid_HighMid_Crossover_Freq,
		HighMid_High_Crossover_Freq,

		Threshold_Low,
		Threshold_LowMid,
		Threshold_HighMid,
		Threshold_High,

		Attack_Low,
		Attack_LowMid,
		Attack_HighMid,
		Attack_High,

		Release_Low,
		Release_LowMid,
		Release_HighMid,
		Release_High,

		Ratio_Low,
		Ratio_LowMid,
		Ratio_HighMid,
		Ratio_High,

		Bypassed_Low,
		Bypassed_LowMid,
		Bypassed_HighMid,
		Bypassed_High,

		Mute_Low,
		Mute_LowMid,
		Mute_HighMid,
		Mute_High,

		Solo_Low,
		Solo_LowMid,
		Solo_HighMid,
		Solo_High,
	};


	inline const std::map<Names, juce::String>& GetParameters()
	{
		static std::map<Names, juce::String> parameters = 
		{
			{Low_LowMid_Crossover_Freq, "Low-LowMid Crossover Freq"},
			{LowMid_HighMid_Crossover_Freq, "LowMid-HighMid Crossover Freq"},
			{HighMid_High_Crossover_Freq, "HighMid-High Crossover Freq"},

			{Threshold_Low, "Threshold Low"},
			{Threshold_LowMid, "Threshold LowMid"},
			{Threshold_HighMid, "Threshold HighMid"},
			{Threshold_High, "Threshold High"},

			{Attack_Low, "Attack Low"},
			{Attack_LowMid, "Attack LowMid"},
			{Attack_HighMid, "Attack HighMid"},
			{Attack_High, "Attack High"},

			{Release_Low, "Release Low"},
			{Release_LowMid, "Release LowMid"},
			{Release_HighMid, "Release HighMid"},
			{Release_High, "Release High"},

			{Ratio_Low, "Ratio Low"},
			{Ratio_LowMid, "Ratio LowMid"},
			{Ratio_HighMid, "Ratio HighMid"},
			{Ratio_High, "Ratio High"},

			{Bypassed_Low, "Bypassed Low"},
			{Bypassed_LowMid, "Bypassed LowMid"},
			{Bypassed_HighMid, "Bypassed HighMid"},
			{Bypassed_High, "Bypassed High"},

			{Mute_Low, "Mute Low"},
			{Mute_LowMid, "Mute LowMid"},
			{Mute_HighMid, "Mute HighMid"},
			{Mute_High, "Mute High"},

			{Solo_Low, "Solo Low"},
			{Solo_LowMid, "Solo LowMid"},
			{Solo_HighMid, "Solo HighMid"},
			{Solo_High, "Solo High"},

		};
		return parameters;
	}
}

struct CompressorBand 
{
    juce::AudioParameterFloat* attack{ nullptr };
    juce::AudioParameterFloat* release{ nullptr };
    juce::AudioParameterFloat* threshold{ nullptr };
    juce::AudioParameterFloat* ratio{ nullptr };
    juce::AudioParameterBool* bypassed{ nullptr };
    juce::AudioParameterBool* mute{ nullptr };
    juce::AudioParameterBool* solo{ nullptr };

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
	/*
	juce::dsp::Compressor<float> compressor;
	juce::AudioParameterFloat* attack{ nullptr };
	juce::AudioParameterFloat* release{ nullptr };
	juce::AudioParameterFloat* threshold{ nullptr };
	juce::AudioParameterFloat* ratio{ nullptr };
	juce::AudioParameterBool* bypassed{ nullptr };
	*/
    std::array<CompressorBand, 4> compressors;
	CompressorBand& lowComp = compressors[0];
	CompressorBand& lowMidComp = compressors[1];
	CompressorBand& highMidComp = compressors[2];
	CompressorBand& highComp = compressors[3];

	//filtry Linkwitza-Rileya
	using Filter = juce::dsp::LinkwitzRileyFilter<float>;
	
	Filter LP1, LP2, LP3;
	Filter HP1, HP2, HP3;

	
	juce::AudioParameterFloat* lowLowMidCrossover{ nullptr };
	juce::AudioParameterFloat* lowMidHighMidCrossover{ nullptr };
	juce::AudioParameterFloat* highMidHighCrossover{ nullptr };
	std::array<juce::AudioBuffer<float>, 4> filterBuffers;

	//testowanie odwróconym allpass
	//Filter invAP;
	//juce::AudioBuffer<float> invAPBuffer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Projekt_zespoowy_2022AudioProcessor)
};
