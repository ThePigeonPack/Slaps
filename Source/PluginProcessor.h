/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct ChainSettings
{
    float gainKnob{ 0 }, slapLevel{ 0 }, volumeSlap{ 0 }; bool bypass{ false }; int instrument{ 0 };

};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SlapsAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SlapsAudioProcessor();
    ~SlapsAudioProcessor() override;

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

    double rawVolume = 0;
    float slapLevel = 0;
    int instrument = 1;
    double volumeSlap = 1;

    float peakOneFreq;
    float peakTwoFreq;
    float peakThreeFreq;
    float peakOneQ;
    float peakTwoQ;
    float peakThreeQ;
    float cutFreq;

    bool pluginBypassed{ false };

    float peakLevel;

    //what you gotta do for the slider parameters to save
    juce::AudioProcessorValueTreeState apvts;

 

private:

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::dsp::Compressor<float> compressor;

    using Filter = juce::dsp::IIR::Filter<float>;

    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>;

    MonoChain leftChain, rightChain;

    enum ChainPositions
    {
        LowCut,
        PeakOne,
        PeakTwo,
        PeakThree,
        HighCut
    };



    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapsAudioProcessor)
};
