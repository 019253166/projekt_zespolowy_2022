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

	floatHelper(compressor.attack, Names::Attack_Low);
	floatHelper(compressor.release, Names::Release_Low);
	floatHelper(compressor.threshold, Names::Threshold_Low);
	floatHelper(compressor.ratio, Names::Ratio_Low);
	boolHelper(compressor.bypassed, Names::Bypassed_Low);

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
	invAP.setType(juce::dsp::LinkwitzRileyFilterType::allpass);




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

	compressor.prepare(spec);

	//filtry
	LP1.prepare(spec);
	LP2.prepare(spec);
	LP3.prepare(spec);
	HP1.prepare(spec);
	HP2.prepare(spec);
	HP3.prepare(spec);
	
	//allpas
	invAP.prepare(spec);
	invAPBuffer.setSize(spec.numChannels, samplesPerBlock);
		
	for (auto& buffer : filterBuffers)
	{
		buffer.setSize(spec.numChannels, samplesPerBlock);
	}
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
    //compressor.updateCompressorSettings();
    //compressor.process(buffer);

	//filtry
	//kopiowanie buforów
	for (auto& fb : filterBuffers)
	{
		fb = buffer;
	}

	invAPBuffer = buffer;

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
	invAP.setCutoffFrequency(20000);
	 //do testu

	auto fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
	auto fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);
	auto fb2Block = juce::dsp::AudioBlock<float>(filterBuffers[2]);
	auto fb3Block = juce::dsp::AudioBlock<float>(filterBuffers[3]);
	
	auto invAPBlock = juce::dsp::AudioBlock<float>(invAPBuffer);
	
	auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
	auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);
	auto fb2Ctx = juce::dsp::ProcessContextReplacing<float>(fb2Block);
	auto fb3Ctx = juce::dsp::ProcessContextReplacing<float>(fb3Block);
	
	auto invAPCtx = juce::dsp::ProcessContextReplacing<float>(invAPBlock);
	invAP.process(invAPCtx);
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

	auto numSamples = buffer.getNumSamples();
	auto numChannels = buffer.getNumChannels();

	//if (compressor.bypassed->get()) {
	//	return;
	//}

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
	addFilterBand(buffer, filterBuffers[0]);
	addFilterBand(buffer, filterBuffers[1]);
	addFilterBand(buffer, filterBuffers[2]);
	addFilterBand(buffer, filterBuffers[3]);


	//jeśli bypass włączony - odwracamy cały sygnał
	if (compressor.bypassed->get()) 
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			juce::FloatVectorOperations::multiply(invAPBuffer.getWritePointer(ch), -1.f, numSamples);
		}
		addFilterBand(buffer, invAPBuffer);
	}
	
}

//==============================================================================
bool Projekt_zespoowy_2022AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Projekt_zespoowy_2022AudioProcessor::createEditor()
{
    //return new Projekt_zespoowy_2022AudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
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

	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Threshold_Low), parameters.at(Names::Threshold_Low), NormalisableRange<float>(-40, 0, 1, 1), 0));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Attack_Low), parameters.at(Names::Attack_Low), attackReleaseRange, 50));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Release_Low), parameters.at(Names::Release_Low), attackReleaseRange, 250));
	layout.add(std::make_unique<AudioParameterFloat>(parameters.at(Names::Ratio_Low), parameters.at(Names::Ratio_Low), NormalisableRange<float>(1, 30, 0.1, 0.35f), 3));
	layout.add(std::make_unique<AudioParameterBool>(parameters.at(Names::Bypassed_Low), parameters.at(Names::Bypassed_Low), false));


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
