#include "PluginProcessor.h"
#include "DspPresets.h"
#include "PluginEditor.h"

namespace
{
    const juce::Identifier stateType { "MixCheckerState" };
    const juce::Identifier modeProperty { "mode" };

    juce::String modeToString (MixCheckerAudioProcessor::Mode mode)
    {
        using Mode = MixCheckerAudioProcessor::Mode;

        switch (mode)
        {
            case Mode::bypass:  return "bypass";
            case Mode::phone:   return "phone";
            case Mode::laptop:  return "laptop";
            case Mode::earbuds: return "earbuds";
            case Mode::car:     return "car";
            case Mode::club:    return "club";
            case Mode::mono:    return "mono";
        }

        return "bypass";
    }

    MixCheckerAudioProcessor::Mode modeFromString (const juce::String& storedMode)
    {
        using Mode = MixCheckerAudioProcessor::Mode;

        if (storedMode == "phone")   return Mode::phone;
        if (storedMode == "laptop")  return Mode::laptop;
        if (storedMode == "earbuds") return Mode::earbuds;
        if (storedMode == "car")     return Mode::car;
        if (storedMode == "club")    return Mode::club;
        if (storedMode == "mono")    return Mode::mono;

        return Mode::bypass;
    }
}

MixCheckerAudioProcessor::MixCheckerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

MixCheckerAudioProcessor::Mode MixCheckerAudioProcessor::getMode() const noexcept
{
    return currentMode.load (std::memory_order_relaxed);
}

void MixCheckerAudioProcessor::setMode (Mode newMode)
{
    if (getMode() == newMode)
        return;

    const juce::ScopedLock callbackLock (getCallbackLock());
    applySettingsForMode (newMode);
    currentMode.store (newMode, std::memory_order_relaxed);
}

void MixCheckerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    applySettingsForMode (getMode());
    dspEngine.prepare ({ sampleRate,
                         static_cast<juce::uint32> (samplesPerBlock),
                         static_cast<juce::uint32> (getTotalNumOutputChannels()) });
}

void MixCheckerAudioProcessor::releaseResources()
{
    dspEngine.reset();
}

bool MixCheckerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MixCheckerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    dspEngine.process (buffer);
}

void MixCheckerAudioProcessor::applySettingsForMode (Mode mode)
{
    switch (mode)
    {
        case Mode::bypass:  dspEngine.setSettings (MixCheckerDspPresets::neutral()); break;
        case Mode::phone:   dspEngine.setSettings (MixCheckerDspPresets::phone());   break;
        case Mode::laptop:  dspEngine.setSettings (MixCheckerDspPresets::laptop());  break;
        case Mode::earbuds: dspEngine.setSettings (MixCheckerDspPresets::earbuds()); break;
        case Mode::car:     dspEngine.setSettings (MixCheckerDspPresets::car());     break;
        case Mode::club:    dspEngine.setSettings (MixCheckerDspPresets::club());    break;
        case Mode::mono:    dspEngine.setSettings (MixCheckerDspPresets::mono());    break;
    }
}

juce::AudioProcessorEditor* MixCheckerAudioProcessor::createEditor()
{
    return new MixCheckerAudioProcessorEditor (*this);
}

bool MixCheckerAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MixCheckerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MixCheckerAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MixCheckerAudioProcessor::producesMidi() const
{
    return false;
}

bool MixCheckerAudioProcessor::isMidiEffect() const
{
    return false;
}

double MixCheckerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MixCheckerAudioProcessor::getNumPrograms()
{
    return 1;
}

int MixCheckerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MixCheckerAudioProcessor::setCurrentProgram (int)
{
}

const juce::String MixCheckerAudioProcessor::getProgramName (int)
{
    return {};
}

void MixCheckerAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void MixCheckerAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    juce::ValueTree state (stateType);
    state.setProperty (modeProperty, modeToString (getMode()), nullptr);

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destinationData);
}

void MixCheckerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto restoredMode = Mode::bypass;

    if (data != nullptr && sizeInBytes > 0)
    {
        if (const auto xml = getXmlFromBinary (data, sizeInBytes))
        {
            const auto state = juce::ValueTree::fromXml (*xml);

            if (state.hasType (stateType) && state.hasProperty (modeProperty))
                restoredMode = modeFromString (state.getProperty (modeProperty).toString());
        }
    }

    setMode (restoredMode);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixCheckerAudioProcessor();
}
