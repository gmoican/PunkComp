/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PunkCompProcessor::PunkCompProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), state(*this, nullptr, "parameters", createParams())
#endif
{
}

PunkCompProcessor::~PunkCompProcessor()
{
}

//==============================================================================
const juce::String PunkCompProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PunkCompProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PunkCompProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PunkCompProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PunkCompProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PunkCompProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PunkCompProcessor::getCurrentProgram()
{
    return 0;
}

void PunkCompProcessor::setCurrentProgram (int index)
{
}

const juce::String PunkCompProcessor::getProgramName (int index)
{
    return {};
}

void PunkCompProcessor::changeProgramName (int index, const juce::String& newName)
{
}

// =========== PARAMETER LAYOUT ====================
juce::AudioProcessorValueTreeState::ParameterLayout PunkCompProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
    params.push_back(std::make_unique<juce::AudioParameterBool>("ONOFF", "On/Off", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("COMP", "Compression", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), DEFAULT_COMP, ""));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LEVEL", "Output Level", juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), DEFAULT_OUTPUT, "dB"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f), DEFAULT_ATTACK, "ms"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MIX", "Mix", juce::NormalisableRange<float>(10.0f, 100.0f, 0.1f), DEFAULT_MIX, "%"));
    
    params.push_back(std::make_unique<juce::AudioParameterInt>("VOICE", "Voice", 0, 2, DEFAULT_VOICE));
    
    return { params.begin(), params.end() };
}

// ============ VALUE GETTERS ======================
float PunkCompProcessor::getGRValue()
{
    return gainReduction.getCurrentValue();
}

// ============ VALUE UPDATERS =====================
void PunkCompProcessor::updateOnOff()
{
    auto ONOFF = state.getRawParameterValue("ONOFF");
    on = ONOFF->load();
}

void PunkCompProcessor::updateOutput()
{
    auto OUT = state.getRawParameterValue("LEVEL");
    float val = OUT->load();
    outputLevel.setGainDecibels(val);
}

void PunkCompProcessor::updateComp()
{
    auto THRES = state.getRawParameterValue("COMP");
    // Mapping function: y = (x-A)/(B-A) * (D-C) + C
    // x = {A, B} = {0.0, 10.0}
    // y = {C, D} = {0.0, -40.0}
    threshold = THRES->load() / 10.0f * (-40.0f);
    comp.setThreshold(threshold);
}

void PunkCompProcessor::updateAttack()
{
    auto ATT = state.getRawParameterValue("ATTACK");
    attackTime = ATT->load();
    comp.setAttack(attackTime);
}

void PunkCompProcessor::updateMix()
{
    auto MIX = state.getRawParameterValue("MIX");
    dryWetMix.setWetMixProportion(MIX->load() / 100.0f);
}

void PunkCompProcessor::updateVoice()
{
    auto VOICE = state.getRawParameterValue("VOICE");
    voice = VOICE->load();
    
    float sampleRate = getSampleRate();
    
    // ISSUE: Fine tune frequencies and gainFactors...
    switch (voice) {
        case 0:
            *eq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 700.0f, 0.7071f, 1.5f);
            *eq.get<1>().state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 2000.0f, 0.7071f, 1.0f);
            break;
        case 1:
            *eq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 700.0f, 0.7071f, 1.0f);
            *eq.get<1>().state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 2000.0f, 0.7071f, 1.0f);
            break;
        case 2:
            *eq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 700.0f, 0.7071f, 1.5f);
            *eq.get<1>().state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 2000.0f, 0.7071f, 1.5f);
            break;
            
        default:
            break;
    };
}

//==============================================================================
void PunkCompProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    comp.prepare(spec);
    comp.setRatio(compressionRatio);
    comp.setThreshold(threshold);
    comp.setAttack(attackTime);
    comp.setRelease(releaseTime);
    
    dryWetMix.prepare(spec);
    
    eq.prepare(spec);
    eq.reset();
    
    outputLevel.prepare(spec);
    outputLevel.setRampDurationSeconds(0.1f);
    
    gainReduction.reset(sampleRate, 0.5);
    gainReduction.setCurrentAndTargetValue(0.0f);
}

void PunkCompProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PunkCompProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PunkCompProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
    
    updateOnOff();
    if(on)
    {
        juce::dsp::AudioBlock<float> audioBlock = juce::dsp::AudioBlock<float>(buffer);
        dryWetMix.pushDrySamples(audioBlock);
        
        // Input
        float peakInput = juce::Decibels::gainToDecibels(buffer.getMagnitude(0, buffer.getNumSamples()));
        
        // Compressor
        updateAttack();
        updateComp();
        comp.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        float peakOutput = juce::Decibels::gainToDecibels(buffer.getMagnitude(0, buffer.getNumSamples()));
        
        // Gain reduction meter
        gainReduction.skip(buffer.getNumSamples());
        {
            const auto value = peakInput - peakOutput;
            if (value < gainReduction.getCurrentValue())
                gainReduction.setTargetValue(value);
            else
                gainReduction.setCurrentAndTargetValue(value);
        }
        
        // Voice
        updateVoice();
        eq.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        
        // Mix
        updateMix();
        dryWetMix.mixWetSamples(audioBlock);
        
        // Output
        updateOutput();
        outputLevel.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    } else
        gainReduction.setCurrentAndTargetValue(0.0f);
}

//==============================================================================
bool PunkCompProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PunkCompProcessor::createEditor()
{
    return new PunkCompEditor (*this);
}

//==============================================================================
void PunkCompProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PunkCompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PunkCompProcessor();
}
