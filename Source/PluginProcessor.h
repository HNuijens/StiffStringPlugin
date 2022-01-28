/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StiffString.h"

#define NOEDITOR

//==============================================================================
/**
*/
class StiffStringPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    StiffStringPluginAudioProcessor();
    ~StiffStringPluginAudioProcessor() override;

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

    NamedValueSet parameters;

private:
    double f0; 


#ifdef NOEDITOR
    // AudioParameters:
    AudioParameterFloat* fundFreq;
    AudioParameterFloat* sigma0;
    AudioParameterFloat* sigma1;
    AudioParameterFloat* radius;
    AudioParameterFloat* density;
    // excitation
    AudioParameterFloat* amplitude;
    AudioParameterFloat* position;
    AudioParameterInt* width;
    AudioParameterBool* strike;

    AudioParameterBool* excited;
    AudioParameterBool* paramChanged;
#endif // NOEDITOR

    // String
    StiffString stiffString;

    double ePos, eAmp;
    int eWidth;
    bool isStriked; 

    double limit(double in);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StiffStringPluginAudioProcessor)
};
