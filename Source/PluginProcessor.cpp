#include "PluginProcessor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto valToStrBool = [](bool e, int)
    {
        return juce::String(e ? "Enabled" : "Disabled");
	};
	const auto strToValBool = [](const juce::String& s)
	{
        return s.getIntValue() != 0;
	};
    

    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "mpeEnabled", "MPE", false, "", valToStrBool, strToValBool
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "kill", "Kill", false, "", valToStrBool, strToValBool
    ));

    return { params.begin(), params.end() };
}

//==============================================================================
MIDIHoldAudioProcessor::MIDIHoldAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
	apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
	mpeEnabledParam(*apvts.getParameter("mpeEnabled")),
	killParam(*apvts.getParameter("kill")),
    mpeSplit(),
    midiHold()
#endif
{
}

const juce::String MIDIHoldAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MIDIHoldAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MIDIHoldAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MIDIHoldAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MIDIHoldAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MIDIHoldAudioProcessor::getNumPrograms()
{
    return 1;
}

int MIDIHoldAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MIDIHoldAudioProcessor::setCurrentProgram (int)
{
}

const juce::String MIDIHoldAudioProcessor::getProgramName (int)
{
    return {};
}

void MIDIHoldAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void MIDIHoldAudioProcessor::prepareToPlay (double, int)
{
}

void MIDIHoldAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MIDIHoldAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MIDIHoldAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
	const auto numSamples = buffer.getNumSamples();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);
    
    if (numSamples == 0)
        return;

    const auto nPlayHead = getPlayHead();
    bool isPlaying = false;
    if (nPlayHead != nullptr)
    {
        const auto transport = nPlayHead->getPosition();
        if (transport.hasValue())
            isPlaying = transport->getIsPlaying();
    }

    const bool mpeEnabled = mpeEnabledParam.getValue() > .5f;
	const bool kill = killParam.getValue() > .5f;

    if (mpeEnabled)
    {
        mpeSplit(midi);
        for (auto ch = 0; ch < mpesplit::NumChannels; ++ch)
        {
            auto& hold = midiHold[ch];
            
            const auto& mpeBuffer = mpeSplit[ch + 1];
            hold.synthesizeMIDI(mpeBuffer, isPlaying, kill);
            const auto& holdBuffer = hold.getMIDI();
			midi.addEvents(holdBuffer, 0, numSamples, 0);
        }
    }
    else
        midiHold[0](midi, isPlaying, kill);
}

//==============================================================================
bool MIDIHoldAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* MIDIHoldAudioProcessor::createEditor()
{
    return nullptr;
}

//==============================================================================
void MIDIHoldAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MIDIHoldAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MIDIHoldAudioProcessor();
}
