#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PunkKompProcessor::PunkKompProcessor()
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

PunkKompProcessor::~PunkKompProcessor()
{
}

//==============================================================================
const juce::String PunkKompProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PunkKompProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PunkKompProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PunkKompProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PunkKompProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PunkKompProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PunkKompProcessor::getCurrentProgram()
{
    return 0;
}

void PunkKompProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String PunkKompProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PunkKompProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

// =========== PARAMETER LAYOUT ====================
juce::AudioProcessorValueTreeState::ParameterLayout PunkKompProcessor::createParams()
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
float PunkKompProcessor::getGRValue()
{
    return gainReduction.getCurrentValue();
}

// ============ VALUE UPDATERS =====================
void PunkKompProcessor::updateOnOff()
{
    auto ONOFF = state.getRawParameterValue("ONOFF");
    on = ONOFF->load();
}

void PunkKompProcessor::updateOutput()
{
    auto OUT = state.getRawParameterValue("LEVEL");
    float val = OUT->load();
    outputLevel.setGainDecibels(val);
}

void PunkKompProcessor::updateComp()
{
    auto THRES = state.getRawParameterValue("COMP");
    
    threshold = juce::jmap(THRES->load(), 0.f, 10.f, -5.f, -25.f);
    float inputGain = juce::jmap(THRES->load(), 0.f, 10.f, -5.f, 20.f);
    
    inputLevel.setGainDecibels(inputGain);
    comp.setThreshold(threshold);
}

void PunkKompProcessor::updateAttack()
{
    auto ATT = state.getRawParameterValue("ATTACK");
    attackTime = ATT->load();
    comp.setAttack(attackTime);
}

void PunkKompProcessor::updateMix()
{
    auto MIX = state.getRawParameterValue("MIX");
    dryWetMix.setWetMixProportion(MIX->load() / 100.0f);
}

void PunkKompProcessor::updateVoice()
{
    auto VOICE = state.getRawParameterValue("VOICE");
    voice = VOICE->load();
    
    float sampleRate = getSampleRate();
    
    switch (voice) {
        case 0:
            *voiceEq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 2430.f, 0.5f, 2.0f);
            break;
        case 1:
            *voiceEq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 2430.f, 0.5f, 1.f);
            break;
        case 2:
            *voiceEq.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 2000.f, 0.35f, 2.5f);
            break;
            
        default:
            break;
    };
}

void PunkKompProcessor::updateState()
{
    updateOnOff();
    updateComp();
    updateAttack();
    updateMix();
    updateVoice();
    updateOutput();
}

//==============================================================================
void PunkKompProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    inputLevel.prepare(spec);
    inputLevel.setRampDurationSeconds(0.1f);
    
    comp.prepare(spec);
    comp.setRatio(compressionRatio);
    comp.setThreshold(threshold);
    comp.setAttack(attackTime);
    comp.setRelease(releaseTime);
    
    dryWetMix.prepare(spec);
    
    voiceEq.prepare(spec);
    voiceEq.reset();
    *voiceEq.get<1>().state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 10.f);
    
    outputLevel.prepare(spec);
    outputLevel.setRampDurationSeconds(0.1f);
    
    gainReduction.reset(sampleRate, 0.5);
    gainReduction.setCurrentAndTargetValue(0.0f);
}

void PunkKompProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PunkKompProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PunkKompProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
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
    
    updateState();
    if(on)
    {
        juce::dsp::AudioBlock<float> audioBlock = juce::dsp::AudioBlock<float>(buffer);
        dryWetMix.pushDrySamples(audioBlock);
        
        // Input
        inputLevel.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        float peakInput = juce::Decibels::gainToDecibels(buffer.getMagnitude(0, buffer.getNumSamples()));
        
        // Compressor
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
        
        // Mix
        dryWetMix.mixWetSamples(audioBlock);
        
        // Voice
        voiceEq.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        
        // Output
        outputLevel.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        
    } else
        gainReduction.setCurrentAndTargetValue(0.0f);
}

//==============================================================================
bool PunkKompProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PunkKompProcessor::createEditor()
{
    return new PunkKompEditor (*this);
}

//==============================================================================
void PunkKompProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::ignoreUnused(destData);
}

void PunkKompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PunkKompProcessor();
}
