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
        220));          // default value

    addParameter(sigma0 = new AudioParameterFloat("sigma0", // parameter ID
        "sigma 0", // parameter name
        0.0f,          // minimum value
        5.0f,       // maximum value
        1.0));          // default value

    addParameter(sigma1 = new AudioParameterFloat("sigma1", // parameter ID
        "sigma 1", // parameter name
        0.0f,          // minimum value
        1.0f,       // maximum value
        0.0005));          // default value

    addParameter(radius = new AudioParameterFloat("radius", // parameter ID
        "radius", // parameter name
        0.00001f,          // minimum value
        0.001f,       // maximum value
        0.00005));          // default value

    addParameter(density = new AudioParameterFloat("density", // parameter ID
        "density", // parameter name
        1000.0f,          // minimum value
        10000.00f,       // maximum value
        8860.0));          // default value

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

    addParameter(strike = new AudioParameterBool("strike", // parameter ID
        "strike", // parameter name
        false   // default value
    )); // default value

    addParameter(excited = new AudioParameterBool("excited", // parameter ID
        "excited", // parameter name
        false   // default value
    )); // default value

    addParameter(paramChanged = new AudioParameterBool("paramChanged", // parameter ID
        "parameter changed", // parameter name
        true   // default value
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
    parameters.set("L", 1);
    parameters.set("rho", 8860);
    parameters.set("r", 0.00005);
    parameters.set("f0", 110);
    parameters.set("E" , 2e11);
    parameters.set("sig0", 0.9);
    parameters.set("sig1" , 0.005);

    stiffString.setFs(sampleRate);
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

    while (it.getNextEvent(currentMessage, samplePos))
    {
        if (currentMessage.isNoteOn())
        {

            //f0 = currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber());
            parameters.set("f0", currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber()));
            stiffString.setGrid(parameters);


            stiffString.exciteSystem(eAmp, ePos, eWidth, isStriked);
        }
    }
#ifdef NOEDITOR

    if (*excited)
    {
        stiffString.exciteSystem(eAmp, ePos, eWidth, isStriked);
        *excited = false;
    }

    if (f0 != *fundFreq)
    {
        f0 = *fundFreq;
        stiffString.setGrid(parameters);
    }

    if (*paramChanged)
    {
        double sig0 = *sigma0;
        double sig1 = *sigma1;
        double rho = *density;
        double rad = *radius;

        parameters.set("rho", rho);
        parameters.set("r", rad);
        parameters.set("sig0", sig0);
        parameters.set("sig1", sig1);

        eAmp = *amplitude;
        ePos = *position;
        eWidth = *width;
        isStriked = *strike;

        *paramChanged = false;
    }



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