/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"



//==============================================================================
/**
*/
class SlapsAudioProcessorEditor  : public juce::AudioProcessorEditor,
    public juce::Slider::Listener, public juce::Timer
{
public:
    SlapsAudioProcessorEditor (SlapsAudioProcessor&);
    ~SlapsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;
    


private:
 

    //where we create our sliders and such
    juce::Slider gainSlider;
    juce::Slider slapKnob;
    juce::ComboBox instrType;
    juce::ToggleButton pluginBypassButton;
    juce::Label peakLabel;
    juce::ImageComponent mImageComponent;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slapKnobAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentAttachment;

    int framesElapsed = 0;

    //quick function to set the instrument variable to equal the instrument combobox value
    void instrumentChanged()
    {
        switch (instrType.getSelectedId())
        {
        case 1: audioProcessor.instrument = 1; break;
        case 2: audioProcessor.instrument = 2; break;
        case 3: audioProcessor.instrument = 3; break;
        case 4: audioProcessor.instrument = 4; break;
        default: break;
        }
    }

    void bypassButtonToggleState(bool bypassValue)
    {
        audioProcessor.pluginBypassed = bypassValue;
    }

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SlapsAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapsAudioProcessorEditor)
};
