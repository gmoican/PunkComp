/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Component/GainReductionMeter.h"

#define DEG2RADS 0.0174533f

//==============================================================================
/**
*/
class PunkCompEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PunkCompEditor (PunkCompProcessor&);
    ~PunkCompEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //=================== PARAMETER MANIPULATION ===================================
    void setSliderComponent(juce::Slider& slider, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& sliderAttachment, juce::String paramName, juce::String style);
    void setToggleComponent(juce::ToggleButton& button, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& buttonAttachment, juce::String paramName);
    juce::AffineTransform knobRotation(float radians, float posX, float posY);
    juce::AffineTransform imageTransforms(float scaleFactor, float posX, float posY);
    
    //=================== GAIN REDUCTION UPDATER ===================================
    void timerCallback() override;

private:
    // Parameters
    juce::Slider compKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compKnobAttachment;
    
    juce::Slider attackKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackKnobAttachment;
    
    juce::Slider voiceSwitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> voiceSwitchAttachment;
    
    juce::Slider mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixKnobAttachment;
    
    juce::Slider levelKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelKnobAttachment;
    
    juce::ToggleButton onToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onToggleAttachment;
    
    // Assets - Background, knobs and switch
    juce::Image background;
    juce::Image lightOff;
    juce::Image switchTop;
    juce::Image knobImage;
    
    // Extra
    juce::Gui::GainReductionMeter grMeter;
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PunkCompProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PunkCompEditor)
};
