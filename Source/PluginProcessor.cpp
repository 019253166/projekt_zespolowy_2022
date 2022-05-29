/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Projekt_zespoowy_2022AudioProcessor::Projekt_zespoowy_2022AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	using namespace Parameters;
	const auto& parameters = GetParameters();

	//przekazywanie parametrów typu float z apvts
	auto floatHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
	{
		parameter = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters.at(parameterName)));
		jassert(parameter != nullptr);

	};
	//przekazywanie parametrów typu bool z apvts
	auto boolHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
	{
		parameter = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters.at(parameterName)));
		jassert(parameter != nullptr);

	};
	//4 kompresory
	floatHelper(lowComp.attack, Names::Attack_Low);
	floatHelper(lowComp.release, Names::Release_Low);
	floatHelper(lowComp.threshold, Names::Threshold_Low);
	floatHelper(lowComp.ratio, Names::Ratio_Low);
	floatHelper(lowComp.knee, Names::Knee_Low);
	boolHelper(lowComp.bypassed, Names::Bypassed_Low);
	boolHelper(lowComp.mute, Names::Mute_Low);
	boolHelper(lowComp.solo, Names::Solo_Low);
	

	floatHelper(lowMidComp.attack, Names::Attack_LowMid);
	floatHelper(lowMidComp.release, Names::Release_LowMid);
	floatHelper(lowMidComp.threshold, Names::Threshold_LowMid);
	floatHelper(lowMidComp.ratio, Names::Ratio_LowMid);
	floatHelper(lowMidComp.knee, Names::Knee_LowMid);
	boolHelper(lowMidComp.bypassed, Names::Bypassed_LowMid);
	boolHelper(lowMidComp.mute, Names::Mute_LowMid);
	boolHelper(lowMidComp.solo, Names::Solo_LowMid);

	floatHelper(highMidComp.attack, Names::Attack_HighMid);
	floatHelper(highMidComp.release, Names::Release_HighMid);
	floatHelper(highMidComp.threshold, Names::Threshold_HighMid);
	floatHelper(highMidComp.ratio, Names::Ratio_HighMid);
	floatHelper(highMidComp.knee, Names::Knee_HighMid);
	boolHelper(highMidComp.bypassed, Names::Bypassed_HighMid);
	boolHelper(highMidComp.mute, Names::Mute_HighMid);
	boolHelper(highMidComp.solo, Names::Solo_HighMid);

	floatHelper(highComp.attack, Names::Attack_High);
	floatHelper(highComp.release, Names::Release_High);
	floatHelper(highComp.threshold, Names::Threshold_High);
	floatHelper(highComp.ratio, Names::Ratio_High);
	floatHelper(highComp.knee, Names::Knee_High);
	boolHelper(highComp.bypassed, Names::Bypassed_High);
	boolHelper(highComp.mute, Names::Mute_High);
	boolHelper(highComp.solo, Names::Solo_High);



	//filtry
	floatHelper(lowLowMidCrossover, Names::Low_LowMid_Crossover_Freq);
	floatHelper(lowMidHighMidCrossover, Names::LowMid_HighMid_Crossover_Freq);
	floatHelper(highMidHighCrossover, Names::HighMid_High_Crossover_Freq);
	
	LP1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
	LP2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
	LP3.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
	HP1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
	HP2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
	HP3.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

	//wzmocnienie
	floatHelper(inputGainParameter, Names::Input_Gain);
	floatHelper(outputGainParameter, Names::Output_Gain);


	//invAP.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

    //tutaj jest konstruktor ¿eby parametry nie by³y przekazywane w ka¿dej partii próbek tylko raz
}

Projekt_zespoowy_2022AudioProcessor::~Projekt_zespoowy_2022AudioProcessor()
{
}

//==============================================================================
const juce::String Projekt_zespoowy_2022AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Projekt_zespoowy_2022AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Projekt_zespoowy_2022AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Projekt_zespoowy_2022AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Projekt_zespoowy_2022AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Projekt_zespoowy_2022AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Projekt_zespoowy_2022AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Projekt_zespoowy_2022AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Projekt_zespoowy_2022AudioProcessor::getProgramName (int index)
{
    return {};
}

void Projekt_zespoowy_2022AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Projekt_zespoowy_2022AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	

	juce::dsp::ProcessSpec spec;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumOutputChannels();
	spec.sampleRate = sampleRate;

	//LATENCJA :(
	AudioProcessor::setLatencySamples(0);
	

	for(auto &comp:compressors)
		comp.prepare(spec);

	//filtry
	LP1.prepare(spec);
	LP2.prepare(spec);
	LP3.prepare(spec);
	HP1.prepare(spec);
	HP2.prepare(spec);
	HP3.prepare(spec);
	
	//allpass
	//invAP.prepare(spec);
	//invAPBuffer.setSize(spec.numChannels, samplesPerBlock);
		
	//wzmocnienie
	inputGain.prepare(spec);
	outputGain.prepare(spec);

	inputGain.setRampDurationSeconds(0.05);
	inputGain.setRampDurationSeconds(0.05);

	for (auto& buffer : filterBuffers)
	{
		buffer.setSize(spec.numChannels, samplesPerBlock);
	}

	leftChannelFifo.prepare(samplesPerBlock);
	rightChannelFifo.prepare(samplesPerBlock);

}
void Projekt_zespoowy_2022AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Projekt_zespoowy_2022AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Projekt_zespoowy_2022AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	leftChannelFifo.update(buffer);
	rightChannelFifo.update(buffer);

	for (auto& compressor : compressors)
		compressor.updateCompressorSettings();
	/*
  //compressor.setAttack(attack->get());
  //compressor.setRelease(release->get());
  //compressor.setThreshold(threshold->get());
  //compressor.setRatio(ratio->get());
    //wczytanie parametrów kompresora

  //auto block = juce::dsp::AudioBlock<float>(buffer);
    //przejmuje bufor i dodaje do niego próbki sygna³u
 // auto context = juce::dsp::ProcessContextReplacing<float>(block); 
    //odpowiada za wymianê próbek w buforze na te przetworzone przez kompresor

 // context.isBypassed = bypassed->get();
    //jeœli bypass jest w³¹czony, nie rób nic

 // compressor.process(context);
    //w³aœciwe przetwarzanie sygna³u na podstawie podanych parametrów
	*/

	//wzmocnienie
	inputGain.setGainDecibels(inputGainParameter->get());
	outputGain.setGainDecibels(outputGainParameter->get());

	auto inputGainBlock = juce::dsp::AudioBlock<float>(buffer);
	auto inputGainCtx = juce::dsp::ProcessContextReplacing<float>(inputGainBlock);
	
	auto outputGainBlock = juce::dsp::AudioBlock<float>(buffer);
	auto outputGainCtx = juce::dsp::ProcessContextReplacing<float>(outputGainBlock);
	inputGain.process(inputGainCtx);
   	
	//filtry
	//kopiowanie buforów

	for (auto& fb : filterBuffers)
	{
		fb = buffer;
	}

	//ustawienie częstotliwości filtrów
	auto lowLowMidCutoff = lowLowMidCrossover -> get();
	LP1.setCutoffFrequency(lowLowMidCutoff);
	HP1.setCutoffFrequency(lowLowMidCutoff);

	auto lowMidHighMidCutoff = lowMidHighMidCrossover -> get();
	LP2.setCutoffFrequency(lowMidHighMidCutoff);
	HP2.setCutoffFrequency(lowMidHighMidCutoff);

	auto highMidHighCutoff = highMidHighCrossover -> get();
	LP3.setCutoffFrequency(highMidHighCutoff);
	HP3.setCutoffFrequency(highMidHighCutoff);

	auto fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
	auto fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);
	auto fb2Block = juce::dsp::AudioBlock<float>(filterBuffers[2]);
	auto fb3Block = juce::dsp::AudioBlock<float>(filterBuffers[3]);
	
	auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
	auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);
	auto fb2Ctx = juce::dsp::ProcessContextReplacing<float>(fb2Block);
	auto fb3Ctx = juce::dsp::ProcessContextReplacing<float>(fb3Block);
	
	//filtry
	/*
	//filtry do buforów
	LP1.process(fb0Ctx);
	AP2.process(fb0Ctx);
	AP3.process(fb0Ctx);

	HP1.process(fb1Ctx);
	filterBuffers[2] = filterBuffers[1];
	
	LP2.process(fb1Ctx);
	AP3.process(fb1Ctx);

	HP2.process(fb2Ctx);
	filterBuffers[3] = filterBuffers[2];
	LP3.process(fb2Ctx);
	
	HP3.process(fb3Ctx);
	*/
	
	//LOW = LP2 + LP1
	LP2.process(fb1Ctx);
	filterBuffers[0] = filterBuffers[1];
	LP1.process(fb0Ctx);

	//LOWMID = LP2 + HP1
	HP1.process(fb1Ctx);

	//HIGHMID = HP2 + LP3
	HP2.process(fb3Ctx);
	filterBuffers[2] = filterBuffers[3];
	LP3.process(fb2Ctx);

	//HIGH = HP2 + HP3
	HP3.process(fb3Ctx);

	//kompresowanie pasm
	for (size_t i = 0; i < filterBuffers.size(); i++)
		compressors[i].process(filterBuffers[i]);

	auto numSamples = buffer.getNumSamples();
	auto numChannels = buffer.getNumChannels();

	buffer.clear();

	//lambda przechwytywanie pasm
	auto addFilterBand = [nc = numChannels, ns = numSamples](auto& inputBuffer, const auto& source)
	{
		for(auto i = 0; i < nc; ++i)
		{
			//(docelowy kanał, docelowa próbka startowa, bufor źródłowy, kanał źródłowy, źródłowa próbka startowa, liczba próbek)
			inputBuffer.addFrom(i, 0, source, i, 0, ns);
		}
	};

	//przyciski solo i mute
	auto bandSoloed = false;
	for (auto& comp : compressors)
	{
		if (comp.solo->get())
		{
			//jeśli pasmo wysolowane - true
			bandSoloed = true;
			break;
		}
	}

	if (bandSoloed)
	{
		for (size_t i = 0; i < compressors.size(); i++)
		{
			auto& comp = compressors[i];
			if (comp.solo->get())
			{
				//jeśli wysolowane - dodaj do bufora
				addFilterBand(buffer, filterBuffers[i]);
			}
		}
	}
	else
	{
		for (size_t i = 0; i < compressors.size(); i++)
		{
			auto& comp = compressors[i];
			if (!comp.mute->get())
			{
				//jeśli nie zmutowane, dodaj
				addFilterBand(buffer, filterBuffers[i]);
			}
		}
	}

	//wzmocnienie output
	outputGain.process(outputGainCtx);

	/*
	addFilterBand(buffer, filterBuffers[0]);
	addFilterBand(buffer, filterBuffers[1]);
	addFilterBand(buffer, filterBuffers[2]);
	addFilterBand(buffer, filterBuffers[3]);
	*/

	//allpass do testu
	/*
	//invAPBuffer = buffer;
	invAP.setCutoffFrequency(20000);
	auto invAPBlock = juce::dsp::AudioBlock<float>(invAPBuffer);
	auto invAPCtx = juce::dsp::ProcessContextReplacing<float>(invAPBlock);
	invAP.process(invAPCtx);
	//jeśli bypass włączony - odwracamy cały sygnał
	if (compressor.bypassed->get()) 
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			juce::FloatVectorOperations::multiply(invAPBuffer.getWritePointer(ch), -1.f, numSamples);
		}
		addFilterBand(buffer, invAPBuffer);
	}
	*/	
}

//==============================================================================
bool Projekt_zespoowy_2022AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Projekt_zespoowy_2022AudioProcessor::createEditor()
{
	//wygląd wtyczki
    return new Projekt_zespoowy_2022AudioProcessorEditor (*this);
	//domyślny wygląd wtyczki
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Projekt_zespoowy_2022AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void Projekt_zespoowy_2022AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Projekt_zespoowy_2022AudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    using namespace juce;
	using namespace Parameters;
	const auto& parameters = GetParameters();

	auto attackReleaseRange = NormalisableRange<float>(1, 500, 1, 1);
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Input_Gain), parameters.at(Names::Input_Gain), NormalisableRange<float>(-20.0f, 20.0f, 0.1f, 1.0f), 0));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Output_Gain), parameters.at(Names::Output_Gain), NormalisableRange<float>(-20.0f, 20.0f, 0.1f, 1.0f), 0));


	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Threshold_Low), parameters.at(Names::Threshold_Low), NormalisableRange<float>(-50, 0, 1, 1), 0));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Threshold_LowMid), parameters.at(Names::Threshold_LowMid), NormalisableRange<float>(-50, 0, 1, 1), 0));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Threshold_HighMid), parameters.at(Names::Threshold_HighMid), NormalisableRange<float>(-50, 0, 1, 1), 0));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Threshold_High), parameters.at(Names::Threshold_High), NormalisableRange<float>(-50, 0, 1, 1), 0));

	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Attack_Low), parameters.at(Names::Attack_Low), attackReleaseRange, 50));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Attack_LowMid), parameters.at(Names::Attack_LowMid), attackReleaseRange, 50));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Attack_HighMid), parameters.at(Names::Attack_HighMid), attackReleaseRange, 50));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Attack_High), parameters.at(Names::Attack_High), attackReleaseRange, 50));
	
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Release_Low), parameters.at(Names::Release_Low), attackReleaseRange, 250));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Release_LowMid), parameters.at(Names::Release_LowMid), attackReleaseRange, 250));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Release_HighMid), parameters.at(Names::Release_HighMid), attackReleaseRange, 250));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Release_High), parameters.at(Names::Release_High), attackReleaseRange, 250));
	
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Ratio_Low), parameters.at(Names::Ratio_Low), NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Ratio_LowMid), parameters.at(Names::Ratio_LowMid), NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Ratio_HighMid), parameters.at(Names::Ratio_HighMid), NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Ratio_High), parameters.at(Names::Ratio_High), NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));

	layout.add(std::make_unique <AudioParameterFloat>(parameters.at(Names::Knee_Low), parameters.at(Names::Knee_Low), NormalisableRange<float>(0, 1, 0.01, 1), 0));
	layout.add(std::make_unique <AudioParameterFloat>(parameters.at(Names::Knee_LowMid), parameters.at(Names::Knee_LowMid), NormalisableRange<float>(0, 1, 0.01, 1), 0));
	layout.add(std::make_unique <AudioParameterFloat>(parameters.at(Names::Knee_HighMid), parameters.at(Names::Knee_HighMid), NormalisableRange<float>(0, 1, 0.01, 1), 0));
	layout.add(std::make_unique <AudioParameterFloat>(parameters.at(Names::Knee_High), parameters.at(Names::Knee_High), NormalisableRange<float>(0, 1, 0.01, 1), 0));
	
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Bypassed_Low), parameters.at(Names::Bypassed_Low), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Bypassed_LowMid), parameters.at(Names::Bypassed_LowMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Bypassed_HighMid), parameters.at(Names::Bypassed_HighMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Bypassed_High), parameters.at(Names::Bypassed_High), false));
	
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Solo_Low), parameters.at(Names::Solo_Low), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Solo_LowMid), parameters.at(Names::Solo_LowMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Solo_HighMid), parameters.at(Names::Solo_HighMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Solo_High), parameters.at(Names::Solo_High), false));

	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Mute_Low), parameters.at(Names::Mute_Low), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Mute_LowMid), parameters.at(Names::Mute_LowMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Mute_HighMid), parameters.at(Names::Mute_HighMid), false));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Mute_High), parameters.at(Names::Mute_High), false));

	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Low_LowMid_Crossover_Freq), parameters.at(Names::Low_LowMid_Crossover_Freq), NormalisableRange<float>(20, 250, 1, 1), 200));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::LowMid_HighMid_Crossover_Freq), parameters.at(Names::LowMid_HighMid_Crossover_Freq), NormalisableRange<float>(500, 2000, 1, 1), 1500));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::HighMid_High_Crossover_Freq), parameters.at(Names::HighMid_High_Crossover_Freq), NormalisableRange<float>(5000, 20000, 1, 1), 6300));
	
   
	/*
	layout.add(std::make_unique<AudioParameterFloat>("Threshold", "Threshold",NormalisableRange<float>(-40, 0, 1, 1),0));
    layout.add(std::make_unique<AudioParameterFloat>("Attack", "Attack", attackReleaseRange, 50));
    layout.add(std::make_unique<AudioParameterFloat>("Release", "Release", attackReleaseRange, 250));
    layout.add(std::make_unique<AudioParameterFloat>("Ratio", "Ratio", NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));
    layout.add(std::make_unique <AudioParameterBool>("Bypassed", "Bypassed", false));
	*/
    return layout;

}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Projekt_zespoowy_2022AudioProcessor();
}
