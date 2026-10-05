#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>

class MixCheckerDspEngine final
{
public:
    struct Settings
    {
        bool highPassEnabled = false;
        float highPassFrequencyHz = 20.0f;

        bool equalizerEnabled = false;
        float equalizerFrequencyHz = 1000.0f;
        float equalizerGainDb = 0.0f;
        float equalizerQ = 0.7071f;

        bool lowPassEnabled = false;
        float lowPassFrequencyHz = 20000.0f;

        float stereoWidth = 1.0f;
        float outputGainDb = 0.0f;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer) noexcept;

    // Configuration updates may create new filter coefficients, so call this
    // outside the real-time audio callback.
    void setSettings (Settings newSettings);
    const Settings& getSettings() const noexcept;

private:
    using StereoFilter = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    struct ProcessingChain
    {
        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset() noexcept;
        void process (juce::dsp::AudioBlock<float> block) noexcept;
        void setSettings (const Settings& newSettings, double sampleRate);

        Settings settings;
        StereoFilter highPass;
        StereoFilter equalizer;
        StereoFilter lowPass;
        juce::dsp::Gain<float> outputGain;
    };

    static constexpr double smoothingTimeSeconds = 0.03;

    Settings sanitiseSettings (Settings newSettings) const noexcept;
    int findStandbyChain() const noexcept;
    void beginTransitionTo (int chainIndex) noexcept;
    void finishTransitionIfNeeded() noexcept;

    Settings settings;
    double sampleRate = 44100.0;
    bool prepared = false;

    std::array<ProcessingChain, 3> chains;
    int activeChain = 0;
    int targetChain = 0;
    int queuedChain = -1;
    bool transitioning = false;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> transitionMix;
    juce::AudioBuffer<float> transitionBuffer;
};
