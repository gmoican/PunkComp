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
    setSliderComponent(voiceSwitch, voiceSwitchAttachment, "VOICE", "Lin");
    
    setSliderComponent(compKnob, compKnobAttachment, "COMP", "Rot");
    setSliderComponent(levelKnob, levelKnobAttachment, "LEVEL", "Rot");
    
    setSliderComponent(attackKnob, attackKnobAttachment, "ATTACK", "Rot");
    setSliderComponent(mixKnob, mixKnobAttachment, "MIX", "Rot");

    setToggleComponent(onToggle, onToggleAttachment, "ONOFF");

    // ================= ASSETS =======================
    background = juce::ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    lightOff = juce::ImageCache::getFromMemory(BinaryData::lightOff_png, BinaryData::lightOff_pngSize);
    switchTop = juce::ImageCache::getFromMemory(BinaryData::switchTop_png, BinaryData::switchTop_pngSize);
    knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_png, BinaryData::knob_pngSize);
    
    // =========== GAIN REDUCTION METER ====================
    addAndMakeVisible(grMeter);
    startTimerHz(20);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (180, 320);
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
    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
        
    // =========== On/Off state ====================
    if (!onToggle.getToggleState()) {
        juce::AffineTransform t;
        t = t.scaled(0.485f);
        t = t.translated(75.5, 144.5);
        g.drawImageTransformed(lightOff, t);
    }
    
    // =========== Switch state ====================
    switch((int) voiceSwitch.getValue()){
        case 0:
            g.drawImageTransformed(switchTop, imageTransforms(0.5f, 72, 14));
            break;
        case 1:
            g.drawImageTransformed(switchTop, imageTransforms(0.5f, 82, 14));
            break;
        case 2:
            g.drawImageTransformed(switchTop, imageTransforms(0.5f, 92, 14));
            break;
            
        default:
            break;
    };
    
    // ========== Parameter knobs angle in radians ==================
    // Comp knob mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {0.0, 10.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float compRadians = ((compKnob.getValue() / 10.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // Output knob mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {-18.0, 18.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float levelRadians = ((levelKnob.getValue() + 18.0f) / (36.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // Attack/Mix mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {1.0, 100.0}
    // y = {C, D} = {-150, 150} * PI / 180
    float attackRadians = ((attackKnob.getValue() - 1.0f) / (99.0f) * 300.0f - 150.0f) * DEG2RADS;
    float mixRadians = ((mixKnob.getValue() - 10.0f) / (90.0f) * 300.0f - 150.0f) * DEG2RADS;
    
    // ========== Draw parameter knobs ==================
    g.drawImageTransformed(knobImage, knobRotation(compRadians, 23.5, 23));
    g.drawImageTransformed(knobImage, knobRotation(levelRadians, 112.5, 23));
    g.drawImageTransformed(knobImage, knobRotation(attackRadians, 23.5, 91));
    g.drawImageTransformed(knobImage, knobRotation(mixRadians, 112.5, 91));
}

void PunkCompEditor::resized()
{
    // Upper row
    voiceSwitch.setBounds(74, 16, 32, 14);
    compKnob.setBounds(24, 23, 46, 46);
    levelKnob.setBounds(113, 23, 46, 46);
    
    // Bottom row
    attackKnob.setBounds(24, 91, 46, 46);
    mixKnob.setBounds(113, 91, 46, 46);
    
    // Gain reduction meter
    grMeter.setBounds(3, 177, 173, 16);
    
    // OnOff
    onToggle.setBounds(65, 240, 50, 50);
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
    t = t.rotated(radians, 46.0f, 46.0f);
    t = t.scaled(0.48f);
    t = t.translated(posX, posY);
    return t;
}

juce::AffineTransform PunkCompEditor::imageTransforms(float scaleFactor, float posX, float posY) {
    juce::AffineTransform t;
    t = t.scaled(scaleFactor);
    t = t.translated(posX, posY);
    return t;
}
