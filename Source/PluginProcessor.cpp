/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SlapsAudioProcessor::SlapsAudioProcessor()
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
}

SlapsAudioProcessor::~SlapsAudioProcessor()
{
}

//==============================================================================
const juce::String SlapsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SlapsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SlapsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SlapsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SlapsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SlapsAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SlapsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SlapsAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SlapsAudioProcessor::getProgramName (int index)
{
    return {};
}

void SlapsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SlapsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = getTotalNumOutputChannels();

    spec.sampleRate = sampleRate;


    //prep compressor
    compressor.prepare(spec);

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto instrument = "bass";

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 63.f, 1.0541f, juce::Decibels::decibelsToGain(0));

    leftChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;
    rightChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;

    auto peakTwoCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 433.f, 0.866f, juce::Decibels::decibelsToGain(0));

    leftChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;
    rightChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;

    auto peakThreeCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 5477.f, 0.782464f, juce::Decibels::decibelsToGain(0));

    leftChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
    rightChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;

    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SlapsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SlapsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SlapsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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



    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    //Deal with the gain first 
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {
            channelData[sample] = buffer.getSample(channel, sample) * rawVolume;

        }
    }
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    //Now compress the signal
    compressor.setRatio(10.0);
    compressor.setAttack(40);
    compressor.setRelease(200);
    compressor.setThreshold(slapLevel * -2.5);



    compressor.process(context);

    //now we get into eq stuff
    //setting the values for each instrument. I'm dumb, so 1 = none, 2 = kick, 3 = snare, 4 = hihat
    if (instrument == 2)
    {
        peakOneFreq = 63.f;
        peakOneQ = 1.0541f;
        peakTwoFreq = 433.f;
        peakTwoQ = 0.866f;
        peakThreeFreq = 5477.f;
        peakThreeQ = 0.782464f;
    }
    
    else if (instrument == 3)
    {
        peakOneFreq = 137.f;
        peakOneQ = 0.979796f;
        peakTwoFreq = 600.f;
        peakTwoQ = 1.2f;
        peakThreeFreq = 7746.f;
        peakThreeQ = 0.704179f;
    }

    else if (instrument == 4)
    {
        peakOneFreq = 387.f;
        peakOneQ = 1.9365f;
        peakTwoFreq = 200.f;
        peakTwoQ = 0.866f;
        peakThreeFreq = 10000.f;
        peakThreeQ = 0.6666667f;
    }

    else if (instrument == 1)
    {
        peakOneFreq = 387.f;
        peakOneQ = 1.9365f;
        peakTwoFreq = 200.f;
        peakTwoQ = 0.866f;
        peakThreeFreq = 10000.f;
        peakThreeQ = 0.6666667f;
    }
    //stop the eq if 
    if (instrument == 1)

    {
        auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakOneFreq, peakOneQ, juce::Decibels::decibelsToGain(0));

        leftChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;
        rightChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;

        auto peakTwoCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakTwoFreq, peakTwoQ, juce::Decibels::decibelsToGain(0));

        leftChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;
        rightChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;

        auto peakThreeCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakThreeFreq, peakThreeQ, juce::Decibels::decibelsToGain(0));

        leftChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
        rightChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
    }
    //use the eq otherwise
    else {
        //these three sets set each of the peak filters to do what they gotta do
        auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakOneFreq, peakOneQ, juce::Decibels::decibelsToGain(slapLevel * 0.75));

        leftChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;
        rightChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;

        auto peakTwoCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakTwoFreq, peakTwoQ, juce::Decibels::decibelsToGain(slapLevel * -0.4));

        leftChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;
        rightChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;

        auto peakThreeCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakThreeFreq, peakThreeQ, juce::Decibels::decibelsToGain(slapLevel * 0.5));

        leftChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
        rightChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
    }


    //juce::dsp::AudioBlock<float> block(buffer);
    //and these lines basically get the eq into the signal, replacing the old version
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
    

    auto chainVolume = rawVolume / volumeSlap;

    //this part sets the volume back to normal from the initial gain slider, and in the future will also do the auto makeup gain from the compressor
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        // ..do something to the data...
        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {
            channelData[sample] = buffer.getSample(channel, sample) / chainVolume;

        }
    }

}

//==============================================================================
bool SlapsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SlapsAudioProcessor::createEditor()
{
    return new SlapsAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SlapsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}



void SlapsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

}





//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlapsAudioProcessor();
}
