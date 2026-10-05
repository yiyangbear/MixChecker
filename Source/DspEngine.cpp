#include "DspEngine.h"

void MixCheckerDspEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    settings = sanitiseSettings (settings);

    for (auto& chain : chains)
    {
        chain.setSettings (settings, sampleRate);
        chain.prepare (spec);
    }

    transitionBuffer.setSize (static_cast<int> (spec.numChannels),
                              juce::jmax (1, static_cast<int> (spec.maximumBlockSize)),
                              false, false, true);
    transitionMix.reset (sampleRate, smoothingTimeSeconds);
    transitionMix.setCurrentAndTargetValue (0.0f);

    activeChain = 0;
    targetChain = 0;
    queuedChain = -1;
    transitioning = false;
    prepared = true;
}

void MixCheckerDspEngine::reset() noexcept
{
    for (auto& chain : chains)
        chain.reset();

    activeChain = 0;
    targetChain = 0;
    queuedChain = -1;
    transitioning = false;
    transitionMix.setCurrentAndTargetValue (0.0f);
}

void MixCheckerDspEngine::process (juce::AudioBuffer<float>& buffer) noexcept
{
    jassert (buffer.getNumChannels() == 2);

    if (buffer.getNumChannels() != 2)
        return;

    juce::dsp::AudioBlock<float> block (buffer);

    if (! transitioning)
    {
        chains[static_cast<size_t> (activeChain)].process (block);
        return;
    }

    const auto scratchCapacity = transitionBuffer.getNumSamples();

    for (size_t offset = 0; offset < block.getNumSamples();)
    {
        if (! transitioning)
        {
            chains[static_cast<size_t> (activeChain)].process (
                block.getSubBlock (offset, block.getNumSamples() - offset));
            break;
        }

        const auto samplesThisTime = juce::jmin (
            static_cast<size_t> (scratchCapacity), block.getNumSamples() - offset);
        auto activeBlock = block.getSubBlock (offset, samplesThisTime);
        auto scratchBlock = juce::dsp::AudioBlock<float> (transitionBuffer)
                                .getSubBlock (0, samplesThisTime);
        scratchBlock.copyFrom (activeBlock);

        chains[static_cast<size_t> (activeChain)].process (activeBlock);
        chains[static_cast<size_t> (targetChain)].process (scratchBlock);

        auto* activeLeft = activeBlock.getChannelPointer (0);
        auto* activeRight = activeBlock.getChannelPointer (1);
        const auto* targetLeft = scratchBlock.getChannelPointer (0);
        const auto* targetRight = scratchBlock.getChannelPointer (1);

        for (size_t sample = 0; sample < samplesThisTime; ++sample)
        {
            const auto targetAmount = transitionMix.getNextValue();
            const auto activeAmount = 1.0f - targetAmount;
            activeLeft[sample] = activeLeft[sample] * activeAmount
                + targetLeft[sample] * targetAmount;
            activeRight[sample] = activeRight[sample] * activeAmount
                + targetRight[sample] * targetAmount;
        }

        offset += samplesThisTime;
        finishTransitionIfNeeded();
    }
}

void MixCheckerDspEngine::setSettings (Settings newSettings)
{
    newSettings = sanitiseSettings (newSettings);
    settings = newSettings;

    if (! prepared)
    {
        for (auto& chain : chains)
            chain.setSettings (settings, sampleRate);

        return;
    }

    const auto standbyChain = findStandbyChain();
    chains[static_cast<size_t> (standbyChain)].setSettings (settings, sampleRate);
    chains[static_cast<size_t> (standbyChain)].reset();

    if (transitioning)
        queuedChain = standbyChain;
    else
        beginTransitionTo (standbyChain);
}

const MixCheckerDspEngine::Settings& MixCheckerDspEngine::getSettings() const noexcept
{
    return settings;
}

MixCheckerDspEngine::Settings MixCheckerDspEngine::sanitiseSettings (Settings newSettings) const noexcept
{
    const auto maximumFrequency = static_cast<float> (sampleRate * 0.49);

    newSettings.highPassFrequencyHz = juce::jlimit (10.0f, maximumFrequency,
                                                    newSettings.highPassFrequencyHz);
    newSettings.equalizerFrequencyHz = juce::jlimit (10.0f, maximumFrequency,
                                                     newSettings.equalizerFrequencyHz);
    newSettings.equalizerGainDb = juce::jlimit (-24.0f, 24.0f,
                                                newSettings.equalizerGainDb);
    newSettings.equalizerQ = juce::jlimit (0.1f, 10.0f, newSettings.equalizerQ);
    newSettings.lowPassFrequencyHz = juce::jlimit (10.0f, maximumFrequency,
                                                   newSettings.lowPassFrequencyHz);
    newSettings.stereoWidth = juce::jlimit (0.0f, 2.0f, newSettings.stereoWidth);
    newSettings.outputGainDb = juce::jlimit (-24.0f, 24.0f, newSettings.outputGainDb);

    return newSettings;
}

int MixCheckerDspEngine::findStandbyChain() const noexcept
{
    for (int index = 0; index < static_cast<int> (chains.size()); ++index)
        if (index != activeChain && (! transitioning || index != targetChain))
            return index;

    jassertfalse;
    return activeChain;
}

void MixCheckerDspEngine::beginTransitionTo (int chainIndex) noexcept
{
    targetChain = chainIndex;
    transitioning = true;
    transitionMix.reset (sampleRate, smoothingTimeSeconds);
    transitionMix.setCurrentAndTargetValue (0.0f);
    transitionMix.setTargetValue (1.0f);
}

void MixCheckerDspEngine::finishTransitionIfNeeded() noexcept
{
    if (transitionMix.isSmoothing())
        return;

    activeChain = targetChain;
    transitioning = false;

    if (queuedChain >= 0 && queuedChain != activeChain)
    {
        const auto nextChain = queuedChain;
        queuedChain = -1;
        beginTransitionTo (nextChain);
    }
}

void MixCheckerDspEngine::ProcessingChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    highPass.prepare (spec);
    equalizer.prepare (spec);
    lowPass.prepare (spec);

    outputGain.setRampDurationSeconds (0.0);
    outputGain.prepare (spec);
    outputGain.setGainDecibels (settings.outputGainDb);
}

void MixCheckerDspEngine::ProcessingChain::reset() noexcept
{
    highPass.reset();
    equalizer.reset();
    lowPass.reset();
    outputGain.reset();
}

void MixCheckerDspEngine::ProcessingChain::process (juce::dsp::AudioBlock<float> block) noexcept
{
    juce::dsp::ProcessContextReplacing<float> context (block);

    if (settings.highPassEnabled)
        highPass.process (context);

    if (settings.equalizerEnabled
        && ! juce::approximatelyEqual (settings.equalizerGainDb, 0.0f))
        equalizer.process (context);

    if (settings.lowPassEnabled)
        lowPass.process (context);

    if (! juce::approximatelyEqual (settings.stereoWidth, 1.0f))
    {
        auto* left = block.getChannelPointer (0);
        auto* right = block.getChannelPointer (1);

        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            const auto mid = (left[sample] + right[sample]) * 0.5f;
            const auto side = (left[sample] - right[sample]) * 0.5f * settings.stereoWidth;

            left[sample] = mid + side;
            right[sample] = mid - side;
        }
    }

    if (! juce::approximatelyEqual (settings.outputGainDb, 0.0f))
        outputGain.process (context);
}

void MixCheckerDspEngine::ProcessingChain::setSettings (const Settings& newSettings,
                                                        double newSampleRate)
{
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    settings = newSettings;
    *highPass.state = *Coefficients::makeHighPass (newSampleRate, settings.highPassFrequencyHz);
    *equalizer.state = *Coefficients::makePeakFilter (
        newSampleRate,
        settings.equalizerFrequencyHz,
        settings.equalizerQ,
        juce::Decibels::decibelsToGain (settings.equalizerGainDb));
    *lowPass.state = *Coefficients::makeLowPass (newSampleRate, settings.lowPassFrequencyHz);

    outputGain.setGainDecibels (settings.outputGainDb);
}
