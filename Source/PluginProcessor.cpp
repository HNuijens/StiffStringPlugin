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
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
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

void StiffStringPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // MIDI Input
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage currentMessage;
    int samplePos;

    while (it.getNextEvent(currentMessage, samplePos))
    {
        if (currentMessage.isNoteOn())
        {
            if (currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber()) != f0)
            {
                f0 = currentMessage.getMidiNoteInHertz(currentMessage.getNoteNumber());
                parameters.set("f0", f0);
                stiffString.setGrid(parameters);
            }

            stiffString.exciteSystem(1, 0.3, 15, false);
        }
    }

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
    return true; // (change this to false if you choose to not supply an editor)
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