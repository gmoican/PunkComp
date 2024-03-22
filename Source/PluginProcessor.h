#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#if (MSVC)
#include "ipps.h"
#endif

#define DEFAULT_OUTPUT 0.0f
#define DEFAULT_COMP 5.0f
#define DEFAULT_ATTACK 30.0f
#define DEFAULT_MIX 80.0f
#define DEFAULT_VOICE 1

//==============================================================================
/**
*/

class PunkKompProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    PunkKompProcessor();
    ~PunkKompProcessor() override;

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
    
    //=============== MY STUFF =====================================================
    juce::AudioProcessorValueTreeState state;
    
    // Getters
    float getGRValue();
    
    // Updaters
    void updateOnOff();
    void updateOutput();
    void updateComp();
    void updateAttack();
    void updateMix();
    void updateVoice();
    void updateState();
    
    void process(float* samples, int numSamples);

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    using FilterBand = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    using Gain = juce::dsp::Gain<float>;
    using Compressor = juce::dsp::Compressor<float>;
    using Mix = juce::dsp::DryWetMixer<float>;
    
    juce::dsp::ProcessorChain<FilterBand, FilterBand> voiceEq;
    
    // Modifiable parameters
    float threshold;
    float attackTime;
    int voice;
    bool on;
    Mix dryWetMix;
    Gain inputLevel, outputLevel;
    
    // Hidden compressor parameters
    Compressor comp;
    const float compressionRatio = 4.0f;
    float releaseTime = 50.0f;
    
    // Other stuff
    juce::LinearSmoothedValue<float> gainReduction;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PunkKompProcessor)
};
