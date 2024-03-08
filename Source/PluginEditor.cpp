/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PunkCompEditor::PunkCompEditor (PunkCompProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // ================= PARAMETERS ====================
    setSliderComponent(inputKnob, inputKnobAttachment, "INPUT", "Rot");
    setSliderComponent(voiceSwitch, voiceSwitchAttachment, "VOICE", "Lin");
    setSliderComponent(outputKnob, outputKnobAttachment, "LEVEL", "Rot");

    setSliderComponent(thresKnob, thresKnobAttachment, "COMP", "Rot");
    setSliderComponent(attackKnob, attackKnobAttachment, "ATTACK", "Rot");
    setSliderComponent(mixKnob, mixKnobAttachment, "MIX", "Rot");

    setToggleComponent(onToggle, onToggleAttachment, "ONOFF");

    // ================= ASSETS =======================
    backgroundOn = juce::ImageCache::getFromMemory(BinaryData::backgroundOn_png, BinaryData::backgroundOn_pngSize);
    backgroundOff = juce::ImageCache::getFromMemory(BinaryData::backgroundOff_png, BinaryData::backgroundOff_pngSize);
    //
    switch0 = juce::ImageCache::getFromMemory(BinaryData::switch0_png, BinaryData::switch0_pngSize);
    switch1 = juce::ImageCache::getFromMemory(BinaryData::switch1_png, BinaryData::switch1_pngSize);
    switch2 = juce::ImageCache::getFromMemory(BinaryData::switch2_png, BinaryData::switch2_pngSize);
    //
    knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_png, BinaryData::knob_pngSize);
    
    // =========== GAIN REDUCTION METER ====================
    addAndMakeVisible(grMeter);
    startTimerHz(20);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (360, 675);
}

PunkCompEditor::~PunkCompEditor()
{
}

void PunkCompEditor::timerCallback()
{
    grMeter.setLevel(audioProcessor.getGRValue());
    grMeter.repaint();
}

//==============================================================================
void PunkCompEditor::paint (juce::Graphics& g)
{
    // Assets loading was here before
        
    // =========== On/Off state ====================
    if (onToggle.getToggleState())
    {
        g.drawImageWithin(backgroundOn, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
    } else
    {
        g.drawImageWithin(backgroundOff, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
    }
    
    // =========== Switch state ====================
    switch((int) voiceSwitch.getValue()){
        case 0:
            g.drawImageAt(switch0, 140, 30);
            break;
        case 1:
            g.drawImageAt(switch1, 140, 30);
            break;
        case 2:
            g.drawImageAt(switch2, 140, 30);
            break;
            
        default:
            break;
    };
    
    // ========== Parameter knobs angle in radians ==================
    // Input/Output knob mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {-18.0, 18.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float inputRadians = ((inputKnob.getValue() + 18.0f) / (36.0f) * 300.0f - 150.0f) * DEG2RADS;
    float outputRadians = ((outputKnob.getValue() + 18.0f) / (36.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // Comp knob mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {0.0, 10.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float thresRadians = ((thresKnob.getValue() / 10.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // Attack/Mix mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {1.0, 100.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float attackRadians = ((attackKnob.getValue() - 1.0f) / (99.0f) * 300.0f - 150.0f) * DEG2RADS;
    float mixRadians = ((mixKnob.getValue() - 10.0f) / (90.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // ========== Draw parameter knobs ==================
    g.drawImageTransformed(knobImage, knobRotation(inputRadians, 25, 30));
    g.drawImageTransformed(knobImage, knobRotation(outputRadians, 258, 30));
    g.drawImageTransformed(knobImage, knobRotation(thresRadians, 25, 168));
    g.drawImageTransformed(knobImage, knobRotation(attackRadians, 140, 168));
    g.drawImageTransformed(knobImage, knobRotation(mixRadians, 258, 168));
}

void PunkCompEditor::resized()
{
    // Upper row
    inputKnob.setBounds(25, 30, 80, 100);
    voiceSwitch.setBounds(140, 30, 80, 80);
    outputKnob.setBounds(260, 30, 80, 80);
    
    // Bottom row
    thresKnob.setBounds(25, 170, 80, 80);
    attackKnob.setBounds(140, 170, 80, 80);
    mixKnob.setBounds(260, 170, 80, 80);
    
    // Gain reduction meter
    grMeter.setBounds(49, 334, 258, 24);
    
    // OnOff
    onToggle.setBounds(165, 560, 80, 80);
}

void PunkCompEditor::setSliderComponent(juce::Slider &slider, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> &sliderAttachment, juce::String paramName, juce::String style){
    sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.state, paramName, slider);
    if (style == "Lin")
    {
        slider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    } else
    {
        slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    }
    slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    addAndMakeVisible(slider);
    slider.setAlpha(0);
}

void PunkCompEditor::setToggleComponent(juce::ToggleButton& button, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& buttonAttachment, juce::String paramName){
    buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.state, paramName, button);
    addAndMakeVisible(button);
    button.setAlpha(0);
}

juce::AffineTransform PunkCompEditor::knobRotation(float radians, float posX, float posY){
    juce::AffineTransform t;
    t = t.rotated(radians, 34.5f, 34.0f);
    t = t.scaled(1.2f);
    t = t.translated(posX, posY);
    return t;
}
