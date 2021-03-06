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
                       ), apvts (*this, nullptr, "Parameters", createParameters())
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

    //low cut filter info
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(20.f, sampleRate, 2);

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    *leftLowCut.get<0>().coefficients = *cutCoefficients[0];

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];

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

    auto chainSettings = getChainSettings(apvts);
    rawVolume = chainSettings.gainKnob;
    slapLevel = chainSettings.slapLevel;
    volumeSlap = chainSettings.volumeSlap;

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
    
    //set the peak level to be used for the indicator
    peakLevel = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));


    //Now compress the signal

    if (pluginBypassed == false)
    {
        compressor.setRatio(10.0);
        compressor.setAttack(40);
        compressor.setRelease(200);
        compressor.setThreshold(slapLevel * -0.5);
        compressor.process(context);
    }
    else {}

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
        cutFreq = 20.f;
    }
    
    else if (instrument == 3)
    {
        peakOneFreq = 137.f;
        peakOneQ = 0.979796f;
        peakTwoFreq = 600.f;
        peakTwoQ = 1.2f;
        peakThreeFreq = 7746.f;
        peakThreeQ = 0.704179f;
        cutFreq = 75.f;
    }

    else if (instrument == 4)
    {
        peakOneFreq = 387.f;
        peakOneQ = 1.9365f;
        peakTwoFreq = 200.f;
        peakTwoQ = 0.866f;
        peakThreeFreq = 10000.f;
        peakThreeQ = 0.6666667f;
        cutFreq = 275.f;
    }

    else if (instrument == 1)
    {
        peakOneFreq = 387.f;
        peakOneQ = 1.9365f;
        peakTwoFreq = 200.f;
        peakTwoQ = 0.866f;
        peakThreeFreq = 10000.f;
        peakThreeQ = 0.6666667f;
        cutFreq = 20.f;
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
        auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakOneFreq, peakOneQ, juce::Decibels::decibelsToGain(slapLevel * 0.3));

        leftChain.setBypassed <ChainPositions::PeakOne>(pluginBypassed);
        rightChain.setBypassed <ChainPositions::PeakOne>(pluginBypassed);
        leftChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;
        rightChain.get<ChainPositions::PeakOne>().coefficients = *peakCoefficients;

        auto peakTwoCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakTwoFreq, peakTwoQ, juce::Decibels::decibelsToGain(slapLevel * -0.2));

        leftChain.setBypassed <ChainPositions::PeakTwo>(pluginBypassed);
        rightChain.setBypassed <ChainPositions::PeakTwo>(pluginBypassed);
        leftChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;
        rightChain.get<ChainPositions::PeakTwo>().coefficients = *peakTwoCoefficients;

        auto peakThreeCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), peakThreeFreq, peakThreeQ, juce::Decibels::decibelsToGain(slapLevel * 0.25));

        leftChain.setBypassed <ChainPositions::PeakThree>(pluginBypassed);
        rightChain.setBypassed <ChainPositions::PeakThree>(pluginBypassed);
        leftChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
        rightChain.get<ChainPositions::PeakThree>().coefficients = *peakThreeCoefficients;
    }

    //low cut filter info
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(cutFreq, getSampleRate(), 2);

    leftChain.setBypassed <ChainPositions::LowCut>(pluginBypassed);
    rightChain.setBypassed <ChainPositions::LowCut>(pluginBypassed);

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    *leftLowCut.get<0>().coefficients = *cutCoefficients[0];

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];


    //juce::dsp::AudioBlock<float> block(buffer);
    //and these lines basically get the eq into the signal, replacing the old version
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    double chainVolume;
    if (pluginBypassed == false)
    {
        chainVolume = rawVolume / volumeSlap;
    }
    else
    {
        chainVolume = rawVolume;
    }

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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}



void SlapsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> theParams(getXmlFromBinary(data, sizeInBytes));

    if (theParams != nullptr)
    {
        if (theParams->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml(*theParams));
        }
    }
}





//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlapsAudioProcessor();
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.gainKnob = pow(10, apvts.getRawParameterValue("GAIN")->load() / 20);
    settings.slapLevel = apvts.getRawParameterValue("SLAP")->load();
    settings.bypass = apvts.getRawParameterValue("BYPASS")->load();
    settings.volumeSlap = pow(10, settings.slapLevel / 60);
    settings.instrument = apvts.getRawParameterValue("INSTRUMENT")->load();
    

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout SlapsAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", -36.0f, 36.0f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SLAP", "Slap", -6.0f, 36.0f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("BYPASS", "Bypass", false));

    juce::StringArray stringArray;
    juce::String str;
    stringArray.add("None");
    stringArray.add("Kick");
    stringArray.add("Snare");
    stringArray.add("Hi Hat");

    params.push_back(std::make_unique<juce::AudioParameterChoice>("INSTRUMENT", "Instrument", stringArray, 0));


    return { params.begin(), params.end() };
}
