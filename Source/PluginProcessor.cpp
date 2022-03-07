/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
StiffStringPluginAudioProcessor::StiffStringPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
#ifdef NOEDITOR
    addParameter(fundFreq = new AudioParameterFloat("fundamentalFreq", // parameter ID
        "Fundamental Frequency", // parameter name
        20.0f,          // minimum value
        10000.0f,       // maximum value
        220.0f));          // default value

    addParameter(sigma0 = new AudioParameterFloat("sigma0", // parameter ID
        "sigma 0", // parameter name
        0.000f,          // minimum value
        5.000f,       // maximum value
        1.00f));          // default value

    addParameter(sigma1 = new AudioParameterFloat("sigma1", // parameter ID
        "sigma 1", // parameter name
        0.00000f,          // minimum value
        0.10000f,       // maximum value
        0.00050f));          // default value

    addParameter(radius = new AudioParameterFloat("radius", // parameter ID
        "radius in mm", // parameter name
        0.001000f,          // minimum value
        10.00000f,       // maximum value
        0.500000f));          // default value

    addParameter(density = new AudioParameterFloat("density", // parameter ID
        "density", // parameter name
        100.00000f,          // minimum value
        100000.00f,       // maximum value
        8860.0000f));          // default value

    addParameter(excitationType = new AudioParameterFloat("excitationType", // parameter ID
        "excitation Type", // parameter name
        0.0f,          // minimum value
        1.0f,       // maximum value
        1.0f));          // default value

    addParameter(amplitude = new AudioParameterFloat("amplitude", // parameter ID
        "amplitude", // parameter name
        0.0f,          // minimum value
        1.0f,       // maximum value
        1.0f));          // default value

    addParameter(position = new AudioParameterFloat("position", // parameter ID
        "position", // parameter name
        0.0f,          // minimum value
        1.0f,       // maximum value
        0.3f));          // default value

    addParameter(width = new AudioParameterInt("width", // parameter ID
        "width", // parameter name
        0.0f,          // minimum value
        30.0f,       // maximum value
        15));          // default value

    addParameter(excited = new AudioParameterBool("excited", // parameter ID
        "excited", // parameter name
        false   // default value
    )); // default value

    addParameter(paramChanged = new AudioParameterBool("paramChanged", // parameter ID
        "parameter changed", // parameter name
        false   // default value
    )); // default value


#endif
}

StiffStringPluginAudioProcessor::~StiffStringPluginAudioProcessor()
{
}

//==============================================================================
const juce::String StiffStringPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool StiffStringPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StiffStringPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StiffStringPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double StiffStringPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int StiffStringPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int StiffStringPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void StiffStringPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String StiffStringPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void StiffStringPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void StiffStringPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    stiffString.setFs(sampleRate);

#ifdef NOEDITOR
    f0 = *fundFreq;
#endif // NOEDITOR

    
    parameters.set("L", 1.0);
    parameters.set("E", 2e11);
    parameters.set("f0", f0);

    updateParameters();

    stiffString.setGrid(parameters);
}

void StiffStringPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StiffStringPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void StiffStringPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // MIDI Input
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage currentMessage;
    int samplePos;

#ifdef MIDIINPUT
    while (it.getNextEvent(currentMessage, samplePos))
    {
        if (currentMessage.isNoteOn())
        {
            f0 = currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber());
            parameters.set("f0", currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber()));
            stiffString.setGrid(parameters);

            stiffString.exciteSystem(eAmp, ePos, eWidth, isStriked);
        }
    }
#endif
#ifdef NOEDITOR
    if (*excitationType <= 0.33f)
    {
        eType = "plucked";
        stiffString.bowed = false;
        isStriked = false;
    }
    if (*excitationType > 0.33f && *excitationType <= 0.66f)
    {
        eType = "bowed";
        isStriked = false; 
    }
    if (*excitationType > 0.66f)
    {
        eType = "stricked";
        stiffString.bowed = false;
        isStriked = true; 
    }

    if (*excited)
    {
        if (f0 != *fundFreq)
        {
            f0 = *fundFreq;
            parameters.set("f0", f0);
            stiffString.setGrid(parameters);
        }
        
        if (eType == "bowed")
        {
            stiffString.bowed = true;
            stiffString.ePos = *position;
        }

        if (eType == "plucked" || eType == "striked")
        {
            stiffString.exciteSystem(eAmp, ePos, eWidth, isStriked);
            *excited = false;
        }
    }

    if (*paramChanged)
    {
        updateParameters();
        stiffString.setGrid(parameters);
        *paramChanged = false;
    }

    if (ePos != *position) ePos = *position; 

#endif // NOEDITOR

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        auto outL = buffer.getWritePointer(0);
        auto outR = buffer.getWritePointer(1);

        float out = 0.f;

        out = stiffString.getNextSample(0.2);

        out = limit(out);
        outL[n] = out;
        outR[n] = out;
    }
}

//==============================================================================
bool StiffStringPluginAudioProcessor::hasEditor() const
{
#ifdef NOEDITOR
    return false;
#else
    return true;
#endif
}

juce::AudioProcessorEditor* StiffStringPluginAudioProcessor::createEditor()
{
    return new StiffStringPluginAudioProcessorEditor (*this);
}

//==============================================================================
void StiffStringPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void StiffStringPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StiffStringPluginAudioProcessor();
}

double StiffStringPluginAudioProcessor::limit(double in)
{
    if (in < -1.0) return -1.0;
    else if (in > 1.0) return 1.0;
    else return in;
}

void StiffStringPluginAudioProcessor::updateParameters()
{
#ifdef NOEDITOR
    double sig0 = *sigma0;
    double sig1 = *sigma1;
    double rho = *density;
    double rad = *radius * 0.001; // transforms radius from mm to m 

    parameters.set("rho", rho);
    parameters.set("r", rad);
    parameters.set("sig0", sig0);
    parameters.set("sig1", sig1);

    eAmp = *amplitude;
    ePos = *position;
    eWidth = *width;
#endif // 
}