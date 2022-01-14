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
    setSize (500, 300);
    startTimerHz(48);

    //Show our Gain Slider
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 100, 25);
    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "GAIN", gainSlider);
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    //Show our One Knob
    slapKnob.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    slapKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 100, 25);
    slapKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SLAP", slapKnob);
    slapKnob.addListener(this);
    addAndMakeVisible(slapKnob);

    //Show our Drop Down List
    addAndMakeVisible(instrType);
    instrType.addItem ("None", 1);
    instrType.addItem("Kick", 2);
    instrType.addItem("Snare", 3);
    instrType.addItem("Hi-Hat", 4);
    instrType.onChange = [this] { instrumentChanged(); };
    instrumentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "INSTRUMENT", instrType);
 

    //show our bypass button
    addAndMakeVisible(pluginBypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "BYPASS", pluginBypassButton);
    pluginBypassButton.onClick = [this] {bypassButtonToggleState(pluginBypassButton.getToggleState()); };

    //show our peak level label
    addAndMakeVisible(peakLabel);
    peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
    peakLabel.attachToComponent(&gainSlider, false);

    //image time baby
    auto logoImage = juce::ImageCache::getFromMemory(BinaryData::Logo_png, BinaryData::Logo_pngSize);
    if (!logoImage.isNull()) 
    {
        mImageComponent.setImage(logoImage, juce::RectanglePlacement::stretchToFit);
    }
    else {
        jassert (! logoImage.isNull());
    }
    addAndMakeVisible(mImageComponent);
 


}

SlapsAudioProcessorEditor::~SlapsAudioProcessorEditor()
{
}

//==============================================================================
void SlapsAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::slategrey);

    //Title lol
    g.setFont(juce::Font(48.0f));
    g.setColour(juce::Colours::black);
    g.drawText("SLAPS", getLocalBounds(), juce::Justification::centredTop, true);




}

void SlapsAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    //gain Slider
    gainSlider.setBounds(100,100, 50, 150);

    //slap Knob
    slapKnob.setBounds(250, 100, 150, 150);

    //Instrument Type Box
    instrType.setBounds(390, 10, 100, 50);

    //bypass button
    pluginBypassButton.setBounds(10, 10, 50, 50);

    //peak Label
    peakLabel.setBounds(112, 75, 25, 25);

    //image
    mImageComponent.setBoundsRelative(0.23f, .23f, 0.03f, 0.055f);

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
        audioProcessor.volumeSlap = pow(10, slapKnob.getValue() / 60);
    }
}

//this is the function that lets things change in the gui
void SlapsAudioProcessorEditor::timerCallback()
{
    if (++framesElapsed > 100)
    {
        framesElapsed = 0;

    }

    //change the peak value color on screen
    if (audioProcessor.peakLevel >= -0.5)
    {

        peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::red);
    }
    else if (audioProcessor.peakLevel > -6 )
    {
        if (peakLabel.findColour(juce::Label::backgroundColourId) == juce::Colours::red)
        {
            peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
        }
        else
        {
            //peakLabel.setText(std::to_string(audioProcessor.peakLevel), juce::sendNotification);
            peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::green);
        }
    }
    else
    {
        if (peakLabel.findColour(juce::Label::backgroundColourId) == juce::Colours::green)
        {
            peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgreen);
        }
        else if (peakLabel.findColour(juce::Label::backgroundColourId) == juce::Colours::red)
        {
            peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkred);
        }
        else 
        {
            peakLabel.setText("", juce::sendNotification);
            peakLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
        }
    }

}