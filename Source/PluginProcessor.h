/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
    Right, //effectively 0
    Left //effectively 1
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,             //channel
            bufferSize,    //num samples
            false,         //keepExistingContent
            true,          //clear extra space
            true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};


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

		Knee_Low,
		Knee_LowMid,
		Knee_HighMid,
		Knee_High,

		Input_Gain,
		Output_Gain,
	};


	inline const std::map<Names, juce::String>& GetParameters()
	{
		static std::map<Names, juce::String> parameters = 
		{
			{Low_LowMid_Crossover_Freq, "Low-LowMid Crossover (Hz)"},
			{LowMid_HighMid_Crossover_Freq, "LowMid-HighMid Crossover (Hz)"},
			{HighMid_High_Crossover_Freq, "HighMid-High Crossover (Hz)"},

			{Threshold_Low, "Threshold Low (dB)"},
			{Threshold_LowMid, "Threshold LowMid (dB)"},
			{Threshold_HighMid, "Threshold HighMid (dB)"},
			{Threshold_High, "Threshold High (dB)"},

			{Attack_Low, "Attack Low (ms)"},
			{Attack_LowMid, "Attack LowMid (ms)"},
			{Attack_HighMid, "Attack HighMid (ms)"},
			{Attack_High, "Attack High (ms)"},

			{Release_Low, "Release Low (ms)"},
			{Release_LowMid, "Release LowMid (ms)"},
			{Release_HighMid, "Release HighMid (ms)"},
			{Release_High, "Release High (ms)"},

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

			{Knee_Low, "Knee Low"},
			{Knee_LowMid, "Knee LowMid"},
			{Knee_HighMid, "Knee HighMid"},
			{Knee_High, "Knee High"},

			{Input_Gain,"Input Gain (dB)"},
			{Output_Gain,"Output Gain (dB)"},

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
    juce::AudioParameterFloat* knee{ nullptr };

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
		compressor.setKnee(knee->get());
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

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right };
private:
	/*
	juce::dsp::Compressor<float> compressor;
	juce::AudioParameterFloat* attack{ nullptr };
	juce::AudioParameterFloat* release{ nullptr };
	juce::AudioParameterFloat* threshold{ nullptr };
	juce::AudioParameterFloat* ratio{ nullptr };
	juce::AudioParameterBool* bypassed{ nullptr };
	*/
	//4 kompresory
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

	//wzmocnienie wejścia i wyjścia
	juce::dsp::Gain<float> inputGain, outputGain;
	juce::AudioParameterFloat* inputGainParameter{ nullptr };
	juce::AudioParameterFloat* outputGainParameter{ nullptr };



	//testowanie odwróconym allpass
	//Filter invAP;
	//juce::AudioBuffer<float> invAPBuffer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Projekt_zespoowy_2022AudioProcessor)
};
