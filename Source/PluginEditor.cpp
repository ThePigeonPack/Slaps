/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SlapsAudioProcessorEditor::SlapsAudioProcessorEditor (SlapsAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 200);

    //Show our Gain Slider
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 100, 25);
    gainSlider.setRange(-18.0, 18.0);
    gainSlider.setValue(-1.0);
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    //Show our One Knob
    slapKnob.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slapKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 100, 25);
    slapKnob.setRange(-24.0, 24.0);
    slapKnob.setValue(0.0);
    slapKnob.addListener(this);
    addAndMakeVisible(slapKnob);

    //Show our Drop Down List
    addAndMakeVisible(instrType);
    instrType.addItem ("None", 1);
    instrType.addItem("Kick", 2);
    instrType.addItem("Snare", 3);
    instrType.addItem("Hi-Hat", 4);
    instrType.setSelectedId(1);


}

SlapsAudioProcessorEditor::~SlapsAudioProcessorEditor()
{
}

//==============================================================================
void SlapsAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::burlywood);

    //Title lol
    g.setFont(juce::Font(48.0f));
    g.setColour(juce::Colours::red);
    g.drawText("Slaps", getLocalBounds(), juce::Justification::centredTop, true);




}

void SlapsAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    //gain Slider
    gainSlider.setBounds(100,100, 50, 100);

    //slap Knob
    slapKnob.setBounds(200, 100, 100, 100);

    //Instrument Type Box
    instrType.setBounds(50, 50, 100, 50);

}

void SlapsAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &gainSlider)
    {
        audioProcessor.rawVolume = pow(10, gainSlider.getValue() / 20);
    }
    if (slider == &slapKnob)
    {
        audioProcessor.slapLevel = slapKnob.getValue();
    }
}