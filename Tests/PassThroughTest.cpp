#include "PluginProcessor.h"
#include "DspPresets.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

namespace
{
    struct LevelMeasurement
    {
        float rmsRatio = 0.0f;
        float peak = 0.0f;
    };

    float measureGainAtFrequency (const MixCheckerDspEngine::Settings& settings,
                                  float frequencyHz)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        constexpr int blockCount = 128;

        MixCheckerDspEngine engine;
        engine.setSettings (settings);
        engine.prepare ({ sampleRate, blockSize, 2 });

        juce::AudioBuffer<float> testBuffer (2, blockSize);
        double inputEnergy = 0.0;
        double outputEnergy = 0.0;
        int sampleIndex = 0;

        for (int block = 0; block < blockCount; ++block)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                const auto phase = juce::MathConstants<double>::twoPi
                    * static_cast<double> (frequencyHz)
                    * static_cast<double> (sampleIndex++) / sampleRate;
                const auto value = static_cast<float> (0.25 * std::sin (phase));
                testBuffer.setSample (0, sample, value);
                testBuffer.setSample (1, sample, value);

                if (block >= blockCount / 2)
                    inputEnergy += static_cast<double> (value) * value;
            }

            engine.process (testBuffer);

            if (block >= blockCount / 2)
            {
                const auto* output = testBuffer.getReadPointer (0);

                for (int sample = 0; sample < blockSize; ++sample)
                    outputEnergy += static_cast<double> (output[sample]) * output[sample];
            }
        }

        return static_cast<float> (std::sqrt (outputEnergy / inputEnergy));
    }

    LevelMeasurement measureRepresentativeLevel (const MixCheckerDspEngine::Settings& settings)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        constexpr int blockCount = 96;
        constexpr std::array frequencies { 60.0, 120.0, 300.0, 1000.0, 3500.0, 9000.0, 15000.0 };
        constexpr std::array amplitudes { 0.035, 0.045, 0.050, 0.055, 0.040, 0.025, 0.015 };
        constexpr std::array rightPhaseOffsets { 0.0, 0.2, 0.4, 0.15, 0.55, 0.8, 1.1 };

        MixCheckerDspEngine engine;
        engine.setSettings (settings);
        engine.prepare ({ sampleRate, blockSize, 2 });

        juce::AudioBuffer<float> testBuffer (2, blockSize);
        double inputEnergy = 0.0;
        double outputEnergy = 0.0;
        float outputPeak = 0.0f;
        int sampleIndex = 0;

        for (int block = 0; block < blockCount; ++block)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                double left = 0.0;
                double right = 0.0;

                for (size_t tone = 0; tone < frequencies.size(); ++tone)
                {
                    const auto phase = juce::MathConstants<double>::twoPi
                        * frequencies[tone] * static_cast<double> (sampleIndex) / sampleRate;
                    left += amplitudes[tone] * std::sin (phase);
                    right += amplitudes[tone] * std::sin (phase + rightPhaseOffsets[tone]);
                }

                ++sampleIndex;
                testBuffer.setSample (0, sample, static_cast<float> (left));
                testBuffer.setSample (1, sample, static_cast<float> (right));

                if (block >= blockCount / 2)
                    inputEnergy += left * left + right * right;
            }

            engine.process (testBuffer);

            if (block >= blockCount / 2)
            {
                for (int channel = 0; channel < testBuffer.getNumChannels(); ++channel)
                {
                    const auto* output = testBuffer.getReadPointer (channel);

                    for (int sample = 0; sample < blockSize; ++sample)
                    {
                        outputEnergy += static_cast<double> (output[sample]) * output[sample];
                        outputPeak = juce::jmax (outputPeak, std::abs (output[sample]));
                    }
                }
            }
        }

        return { static_cast<float> (std::sqrt (outputEnergy / inputEnergy)), outputPeak };
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    MixCheckerAudioProcessor processor;

    using Mode = MixCheckerAudioProcessor::Mode;

    if (processor.getMode() != Mode::bypass)
    {
        std::cerr << "Expected the default mode to be Bypass.\n";
        return 1;
    }

    if (processor.getTotalNumInputChannels() != 2
        || processor.getTotalNumOutputChannels() != 2
        || processor.getLatencySamples() != 0)
    {
        std::cerr << "Expected stereo input, stereo output, and zero latency.\n";
        return 1;
    }

    constexpr int numSamples = 4096;
    juce::AudioBuffer<float> buffer (2, numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        buffer.setSample (0, sample, static_cast<float> ((sample % 257) - 128) / 128.0f);
        buffer.setSample (1, sample, static_cast<float> (((sample * 37) % 263) - 131) / 131.0f);
    }

    juce::AudioBuffer<float> expected;
    expected.makeCopyOf (buffer);

    constexpr std::array modes {
        Mode::bypass,
        Mode::phone,
        Mode::laptop,
        Mode::earbuds,
        Mode::car,
        Mode::club,
        Mode::mono
    };

    processor.prepareToPlay (48000.0, 512);
    juce::MidiBuffer midi;

    for (const auto mode : modes)
    {
        processor.setMode (mode);

        if (processor.getMode() != mode)
        {
            std::cerr << "The active mode did not update correctly.\n";
            return 1;
        }

        buffer.makeCopyOf (expected);
        processor.processBlock (buffer, midi);

        buffer.makeCopyOf (expected);
        processor.processBlock (buffer, midi);

        const auto modeBufferBytes = sizeof (float) * static_cast<size_t> (numSamples);
        const auto outputIsUnchanged = std::memcmp (buffer.getReadPointer (0),
                                                     expected.getReadPointer (0),
                                                     modeBufferBytes) == 0
            && std::memcmp (buffer.getReadPointer (1),
                            expected.getReadPointer (1),
                            modeBufferBytes) == 0;

        if ((mode == Mode::bypass) != outputIsUnchanged)
        {
            std::cerr << "A mode did not produce its expected processed or transparent output.\n";
            return 1;
        }

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                if (! std::isfinite (buffer.getSample (channel, sample)))
                {
                    std::cerr << "A playback mode produced an invalid sample.\n";
                    return 1;
                }
            }
        }

        if (mode == Mode::mono
            && std::memcmp (buffer.getReadPointer (0),
                            buffer.getReadPointer (1),
                            modeBufferBytes) != 0)
        {
            std::cerr << "Mono mode did not produce identical output channels.\n";
            return 1;
        }
    }

    buffer.makeCopyOf (expected);
    processor.setMode (Mode::phone);
    processor.processBlock (buffer, midi);

    const auto bufferBytes = sizeof (float) * static_cast<size_t> (numSamples);

    if (std::memcmp (buffer.getReadPointer (0), expected.getReadPointer (0), bufferBytes) == 0
        && std::memcmp (buffer.getReadPointer (1), expected.getReadPointer (1), bufferBytes) == 0)
    {
        std::cerr << "Phone mode did not process the audio.\n";
        return 1;
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (! std::isfinite (buffer.getSample (channel, sample)))
            {
                std::cerr << "Phone mode produced an invalid sample.\n";
                return 1;
            }
        }
    }

    processor.releaseResources();

    const auto phoneSettings = MixCheckerDspPresets::phone();

    if (! phoneSettings.highPassEnabled
        || ! phoneSettings.equalizerEnabled
        || ! phoneSettings.lowPassEnabled
        || ! juce::approximatelyEqual (phoneSettings.highPassFrequencyHz, 180.0f)
        || ! juce::approximatelyEqual (phoneSettings.equalizerFrequencyHz, 1600.0f)
        || ! juce::approximatelyEqual (phoneSettings.equalizerGainDb, 3.0f)
        || ! juce::approximatelyEqual (phoneSettings.equalizerQ, 0.8f)
        || ! juce::approximatelyEqual (phoneSettings.lowPassFrequencyHz, 7500.0f)
        || ! juce::approximatelyEqual (phoneSettings.stereoWidth, 0.35f)
        || ! juce::approximatelyEqual (phoneSettings.outputGainDb, 1.0f))
    {
        std::cerr << "Phone preset values are incorrect.\n";
        return 1;
    }

    const auto lowGain = measureGainAtFrequency (phoneSettings, 80.0f);
    const auto midGain = measureGainAtFrequency (phoneSettings, 1600.0f);
    const auto highGain = measureGainAtFrequency (phoneSettings, 12000.0f);

    if (lowGain >= midGain * 0.45f || highGain >= midGain * 0.6f)
    {
        std::cerr << "Phone mode does not sufficiently focus the midrange.\n";
        return 1;
    }

    const auto laptopSettings = MixCheckerDspPresets::laptop();
    const auto earbudsSettings = MixCheckerDspPresets::earbuds();
    const auto carSettings = MixCheckerDspPresets::car();
    const auto clubSettings = MixCheckerDspPresets::club();
    const auto monoSettings = MixCheckerDspPresets::mono();

    const auto settingsMatch = [] (const MixCheckerDspEngine::Settings& settings,
                                   bool highPassEnabled,
                                   float highPassFrequencyHz,
                                   bool equalizerEnabled,
                                   float equalizerFrequencyHz,
                                   float equalizerGainDb,
                                   float equalizerQ,
                                   bool lowPassEnabled,
                                   float lowPassFrequencyHz,
                                   float stereoWidth,
                                   float outputGainDb)
    {
        return settings.highPassEnabled == highPassEnabled
            && juce::approximatelyEqual (settings.highPassFrequencyHz, highPassFrequencyHz)
            && settings.equalizerEnabled == equalizerEnabled
            && juce::approximatelyEqual (settings.equalizerFrequencyHz, equalizerFrequencyHz)
            && juce::approximatelyEqual (settings.equalizerGainDb, equalizerGainDb)
            && juce::approximatelyEqual (settings.equalizerQ, equalizerQ)
            && settings.lowPassEnabled == lowPassEnabled
            && juce::approximatelyEqual (settings.lowPassFrequencyHz, lowPassFrequencyHz)
            && juce::approximatelyEqual (settings.stereoWidth, stereoWidth)
            && juce::approximatelyEqual (settings.outputGainDb, outputGainDb);
    };

    if (! settingsMatch (laptopSettings, true, 120.0f, true, 1800.0f, 2.0f, 0.8f,
                         true, 14000.0f, 0.75f, 0.0f)
        || ! settingsMatch (earbudsSettings, true, 30.0f, true, 110.0f, 2.0f, 0.8f,
                            true, 18000.0f, 1.05f, -1.0f)
        || ! settingsMatch (carSettings, true, 30.0f, true, 180.0f, 3.0f, 0.7f,
                            true, 12000.0f, 1.15f, -1.5f)
        || ! settingsMatch (clubSettings, true, 25.0f, true, 75.0f, 3.0f, 0.8f,
                            true, 18000.0f, 1.10f, -1.5f)
        || ! settingsMatch (monoSettings, false, 20.0f, false, 1000.0f, 0.0f, 0.7071f,
                            false, 20000.0f, 0.0f, 0.0f))
    {
        std::cerr << "One or more Task 8 preset values are incorrect.\n";
        return 1;
    }

    const auto laptopLowGain = measureGainAtFrequency (laptopSettings, 80.0f);
    const auto laptopMidGain = measureGainAtFrequency (laptopSettings, 1800.0f);
    const auto earbudsLowGain = measureGainAtFrequency (earbudsSettings, 80.0f);
    const auto carLowMidGain = measureGainAtFrequency (carSettings, 180.0f);
    const auto carMidGain = measureGainAtFrequency (carSettings, 1500.0f);
    const auto carHighGain = measureGainAtFrequency (carSettings, 15000.0f);
    const auto clubBassGain = measureGainAtFrequency (clubSettings, 75.0f);
    const auto clubMidGain = measureGainAtFrequency (clubSettings, 1500.0f);
    const auto clubHighGain = measureGainAtFrequency (clubSettings, 15000.0f);

    if (laptopLowGain >= laptopMidGain * 0.7f
        || earbudsLowGain <= laptopLowGain * 1.35f
        || carLowMidGain <= carMidGain * 1.2f
        || clubBassGain <= clubMidGain * 1.2f
        || clubHighGain <= carHighGain * 1.15f)
    {
        std::cerr << "Task 8 presets do not have the intended relative frequency responses.\n";
        return 1;
    }

    const std::array compensatedSettings {
        MixCheckerDspPresets::neutral(),
        MixCheckerDspPresets::phone(),
        MixCheckerDspPresets::laptop(),
        MixCheckerDspPresets::earbuds(),
        MixCheckerDspPresets::car(),
        MixCheckerDspPresets::club(),
        MixCheckerDspPresets::mono()
    };

    for (const auto& compensated : compensatedSettings)
    {
        auto uncompensated = compensated;
        uncompensated.outputGainDb = 0.0f;

        const auto compensatedLevel = measureRepresentativeLevel (compensated);
        const auto uncompensatedLevel = measureRepresentativeLevel (uncompensated);
        const auto compensatedDifferenceDb = std::abs (
            juce::Decibels::gainToDecibels (compensatedLevel.rmsRatio));
        const auto uncompensatedDifferenceDb = std::abs (
            juce::Decibels::gainToDecibels (uncompensatedLevel.rmsRatio));

        if (compensatedDifferenceDb > uncompensatedDifferenceDb + 0.05f
            || compensatedDifferenceDb > 3.0f
            || compensatedLevel.peak >= 1.0f)
        {
            std::cerr << "A fixed mode gain did not improve conservative level matching: "
                      << compensatedDifferenceDb << " dB compensated versus "
                      << uncompensatedDifferenceDb << " dB at unity, peak "
                      << compensatedLevel.peak << ".\n";
            return 1;
        }
    }

    constexpr int smoothingBlockSize = 128;
    constexpr int smoothingBlockCount = 12;
    MixCheckerDspEngine smoothingEngine;
    smoothingEngine.setSettings (MixCheckerDspPresets::neutral());
    smoothingEngine.prepare ({ 48000.0, smoothingBlockSize, 2 });

    auto smoothedMonoSettings = MixCheckerDspPresets::mono();
    smoothedMonoSettings.outputGainDb = -12.0f;
    smoothingEngine.setSettings (smoothedMonoSettings);

    juce::AudioBuffer<float> smoothingBuffer (2, smoothingBlockSize);
    float previousLeft = 0.5f;
    float maximumMonoStep = 0.0f;
    float firstMonoSample = 0.0f;

    for (int block = 0; block < smoothingBlockCount; ++block)
    {
        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            smoothingBuffer.setSample (0, sample, 0.5f);
            smoothingBuffer.setSample (1, sample, -0.5f);
        }

        smoothingEngine.process (smoothingBuffer);

        if (block == 0)
            firstMonoSample = smoothingBuffer.getSample (0, 0);

        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            const auto currentLeft = smoothingBuffer.getSample (0, sample);
            maximumMonoStep = juce::jmax (maximumMonoStep,
                                         std::abs (currentLeft - previousLeft));
            previousLeft = currentLeft;
        }
    }

    if (firstMonoSample < 0.49f
        || maximumMonoStep > 0.001f
        || std::abs (smoothingBuffer.getSample (0, smoothingBlockSize - 1)) > 0.0001f
        || std::abs (smoothingBuffer.getSample (1, smoothingBlockSize - 1)) > 0.0001f)
    {
        std::cerr << "The transition into Mono was not short and smooth.\n";
        return 1;
    }

    smoothingEngine.setSettings (MixCheckerDspPresets::neutral());
    previousLeft = 0.0f;
    float maximumBypassStep = 0.0f;
    float firstBypassSample = 0.0f;

    for (int block = 0; block < smoothingBlockCount; ++block)
    {
        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            smoothingBuffer.setSample (0, sample, 0.5f);
            smoothingBuffer.setSample (1, sample, -0.5f);
        }

        smoothingEngine.process (smoothingBuffer);

        if (block == 0)
            firstBypassSample = smoothingBuffer.getSample (0, 0);

        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            const auto currentLeft = smoothingBuffer.getSample (0, sample);
            maximumBypassStep = juce::jmax (maximumBypassStep,
                                           std::abs (currentLeft - previousLeft));
            previousLeft = currentLeft;
        }
    }

    if (firstBypassSample > 0.01f
        || maximumBypassStep > 0.001f
        || ! juce::approximatelyEqual (
            smoothingBuffer.getSample (0, smoothingBlockSize - 1), 0.5f)
        || ! juce::approximatelyEqual (
            smoothingBuffer.getSample (1, smoothingBlockSize - 1), -0.5f))
    {
        std::cerr << "The transition from Mono to transparent Bypass was not smooth.\n";
        return 1;
    }

    MixCheckerDspEngine gainSmoothingEngine;
    gainSmoothingEngine.setSettings (MixCheckerDspPresets::neutral());
    gainSmoothingEngine.prepare ({ 48000.0, smoothingBlockSize, 2 });
    auto reducedGainSettings = MixCheckerDspPresets::neutral();
    reducedGainSettings.outputGainDb = -12.0f;
    gainSmoothingEngine.setSettings (reducedGainSettings);

    previousLeft = 0.25f;
    float maximumGainStep = 0.0f;
    float firstGainSample = 0.0f;

    for (int block = 0; block < smoothingBlockCount; ++block)
    {
        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            smoothingBuffer.setSample (0, sample, 0.25f);
            smoothingBuffer.setSample (1, sample, 0.25f);
        }

        gainSmoothingEngine.process (smoothingBuffer);

        if (block == 0)
            firstGainSample = smoothingBuffer.getSample (0, 0);

        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            const auto currentLeft = smoothingBuffer.getSample (0, sample);
            maximumGainStep = juce::jmax (maximumGainStep,
                                         std::abs (currentLeft - previousLeft));
            previousLeft = currentLeft;
        }
    }

    const auto expectedReducedSample = 0.25f * juce::Decibels::decibelsToGain (-12.0f);

    if (firstGainSample < 0.249f
        || maximumGainStep > 0.001f
        || ! juce::approximatelyEqual (
            smoothingBuffer.getSample (0, smoothingBlockSize - 1), expectedReducedSample))
    {
        std::cerr << "Output gain did not transition smoothly.\n";
        return 1;
    }

    for (int block = 0; block < 300; ++block)
    {
        gainSmoothingEngine.setSettings (
            compensatedSettings[static_cast<size_t> (block) % compensatedSettings.size()]);

        for (int sample = 0; sample < smoothingBlockSize; ++sample)
        {
            const auto phase = juce::MathConstants<float>::twoPi
                * 440.0f * static_cast<float> (block * smoothingBlockSize + sample) / 48000.0f;
            smoothingBuffer.setSample (0, sample, 0.2f * std::sin (phase));
            smoothingBuffer.setSample (1, sample, 0.17f * std::sin (phase + 0.4f));
        }

        gainSmoothingEngine.process (smoothingBuffer);

        for (int channel = 0; channel < smoothingBuffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < smoothingBuffer.getNumSamples(); ++sample)
                if (! std::isfinite (smoothingBuffer.getSample (channel, sample)))
                {
                    std::cerr << "Rapid smoothed mode changes produced an invalid sample.\n";
                    return 1;
                }
    }

    MixCheckerDspEngine monoEngine;
    monoEngine.setSettings (monoSettings);
    monoEngine.prepare ({ 48000.0, 512, 2 });

    juce::AudioBuffer<float> monoBuffer (2, 512);

    for (int sample = 0; sample < monoBuffer.getNumSamples(); ++sample)
    {
        monoBuffer.setSample (0, sample, static_cast<float> (sample - 256) / 256.0f);
        monoBuffer.setSample (1, sample, static_cast<float> (128 - sample) / 384.0f);
    }

    monoBuffer.setSample (0, 0, 1.0f);
    monoBuffer.setSample (1, 0, -1.0f);

    juce::AudioBuffer<float> monoInput;
    monoInput.makeCopyOf (monoBuffer);
    monoEngine.process (monoBuffer);

    for (int sample = 0; sample < monoBuffer.getNumSamples(); ++sample)
    {
        const auto expectedMono = (monoInput.getSample (0, sample)
                                   + monoInput.getSample (1, sample)) * 0.5f
            * juce::Decibels::decibelsToGain (monoSettings.outputGainDb);

        if (! juce::approximatelyEqual (monoBuffer.getSample (0, sample), expectedMono))
        {
            std::cerr << "Mono mode did not use standard averaging on both channels.\n";
            return 1;
        }
    }

    if (std::memcmp (monoBuffer.getReadPointer (0),
                     monoBuffer.getReadPointer (1),
                     static_cast<size_t> (monoBuffer.getNumSamples()) * sizeof (float)) != 0)
    {
        std::cerr << "Mono mode did not produce byte-identical left and right channels.\n";
        return 1;
    }

    MixCheckerDspEngine widthEngine;
    MixCheckerDspEngine::Settings widthSettings;
    widthSettings.stereoWidth = phoneSettings.stereoWidth;
    widthEngine.setSettings (widthSettings);
    widthEngine.prepare ({ 48000.0, 512, 2 });

    juce::AudioBuffer<float> widthBuffer (2, 512);
    widthBuffer.clear();
    widthBuffer.getWritePointer (0)[0] = 0.25f;
    widthBuffer.getWritePointer (1)[0] = -0.25f;
    widthEngine.process (widthBuffer);

    if (! juce::approximatelyEqual (widthBuffer.getSample (0, 0), 0.0875f)
        || ! juce::approximatelyEqual (widthBuffer.getSample (1, 0), -0.0875f))
    {
        std::cerr << "Phone stereo width was not applied correctly.\n";
        return 1;
    }

    MixCheckerDspEngine configurableEngine;
    MixCheckerDspEngine::Settings configuredSettings;
    configuredSettings.highPassEnabled = true;
    configuredSettings.highPassFrequencyHz = 80.0f;
    configuredSettings.equalizerEnabled = true;
    configuredSettings.equalizerFrequencyHz = 1200.0f;
    configuredSettings.equalizerGainDb = 3.0f;
    configuredSettings.equalizerQ = 0.8f;
    configuredSettings.lowPassEnabled = true;
    configuredSettings.lowPassFrequencyHz = 12000.0f;
    configuredSettings.stereoWidth = 0.75f;
    configuredSettings.outputGainDb = -1.0f;
    configurableEngine.setSettings (configuredSettings);

    const auto& appliedSettings = configurableEngine.getSettings();

    if (! appliedSettings.highPassEnabled
        || ! appliedSettings.equalizerEnabled
        || ! appliedSettings.lowPassEnabled
        || ! juce::approximatelyEqual (appliedSettings.highPassFrequencyHz, 80.0f)
        || ! juce::approximatelyEqual (appliedSettings.equalizerFrequencyHz, 1200.0f)
        || ! juce::approximatelyEqual (appliedSettings.equalizerGainDb, 3.0f)
        || ! juce::approximatelyEqual (appliedSettings.equalizerQ, 0.8f)
        || ! juce::approximatelyEqual (appliedSettings.lowPassFrequencyHz, 12000.0f)
        || ! juce::approximatelyEqual (appliedSettings.stereoWidth, 0.75f)
        || ! juce::approximatelyEqual (appliedSettings.outputGainDb, -1.0f))
    {
        std::cerr << "DSP settings were not applied correctly.\n";
        return 1;
    }

    configurableEngine.prepare ({ 48000.0, 512, 2 });
    juce::AudioBuffer<float> configuredBuffer (2, 512);
    configuredBuffer.clear();
    configuredBuffer.setSample (0, 0, 1.0f);
    configuredBuffer.setSample (1, 0, -0.5f);

    juce::AudioBuffer<float> configuredInput;
    configuredInput.makeCopyOf (configuredBuffer);
    configurableEngine.process (configuredBuffer);

    const auto configuredBufferBytes = sizeof (float)
        * static_cast<size_t> (configuredBuffer.getNumSamples());

    if (std::memcmp (configuredBuffer.getReadPointer (0),
                     configuredInput.getReadPointer (0),
                     configuredBufferBytes) == 0
        && std::memcmp (configuredBuffer.getReadPointer (1),
                        configuredInput.getReadPointer (1),
                        configuredBufferBytes) == 0)
    {
        std::cerr << "Configured DSP did not process the test signal.\n";
        return 1;
    }

    for (int channel = 0; channel < configuredBuffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < configuredBuffer.getNumSamples(); ++sample)
        {
            if (! std::isfinite (configuredBuffer.getSample (channel, sample)))
            {
                std::cerr << "Configured DSP produced an invalid sample.\n";
                return 1;
            }
        }
    }

    configurableEngine.reset();

    MixCheckerAudioProcessor uiProcessor;
    uiProcessor.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (uiProcessor.createEditor());

    if (editor == nullptr
        || editor->getWidth() != 474
        || editor->getHeight() != 208
        || editor->isResizable())
    {
        std::cerr << "Expected a fixed 474 x 208 custom editor.\n";
        return 1;
    }

    struct ButtonExpectation
    {
        const char* componentId;
        Mode mode;
    };

    constexpr std::array buttonExpectations {
        ButtonExpectation { "phone", Mode::phone },
        ButtonExpectation { "laptop", Mode::laptop },
        ButtonExpectation { "earbuds", Mode::earbuds },
        ButtonExpectation { "car", Mode::car },
        ButtonExpectation { "club", Mode::club },
        ButtonExpectation { "mono", Mode::mono },
        ButtonExpectation { "bypass", Mode::bypass }
    };

    const auto countSelectedButtons = [&editor, &buttonExpectations]
    {
        int selectedCount = 0;

        for (const auto& expectation : buttonExpectations)
        {
            const auto* button = dynamic_cast<juce::TextButton*> (
                editor->findChildWithID (expectation.componentId));

            if (button != nullptr && button->getToggleState())
                ++selectedCount;
        }

        return selectedCount;
    };

    for (const auto& expectation : buttonExpectations)
    {
        auto* button = dynamic_cast<juce::TextButton*> (
            editor->findChildWithID (expectation.componentId));

        if (button == nullptr
            || ! button->isVisible()
            || ! editor->getLocalBounds().contains (button->getBounds()))
        {
            std::cerr << "A required mode button is missing or outside the editor.\n";
            return 1;
        }
    }

    auto* bypassButton = dynamic_cast<juce::TextButton*> (editor->findChildWithID ("bypass"));

    if (bypassButton == nullptr || ! bypassButton->getToggleState() || countSelectedButtons() != 1)
    {
        std::cerr << "Expected Bypass to be the editor's only initially selected mode.\n";
        return 1;
    }

    for (const auto& expectation : buttonExpectations)
    {
        if (expectation.mode == Mode::bypass)
            continue;

        auto* button = dynamic_cast<juce::TextButton*> (
            editor->findChildWithID (expectation.componentId));

        uiProcessor.setMode (Mode::bypass);
        button->onClick();

        if (uiProcessor.getMode() != expectation.mode
            || ! button->getToggleState()
            || countSelectedButtons() != 1)
        {
            std::cerr << "A mode button did not enter its mode from Bypass.\n";
            return 1;
        }

        button->onClick();

        if (uiProcessor.getMode() != Mode::bypass
            || ! bypassButton->getToggleState()
            || countSelectedButtons() != 1)
        {
            std::cerr << "Clicking the active mode did not return to Bypass.\n";
            return 1;
        }
    }

    auto* carButton = dynamic_cast<juce::TextButton*> (editor->findChildWithID ("car"));
    auto* phoneButton = dynamic_cast<juce::TextButton*> (editor->findChildWithID ("phone"));
    carButton->onClick();
    phoneButton->onClick();

    if (uiProcessor.getMode() != Mode::phone
        || ! phoneButton->getToggleState()
        || countSelectedButtons() != 1)
    {
        std::cerr << "Switching directly between two modes did not select the target mode.\n";
        return 1;
    }

    bypassButton->onClick();
    bypassButton->onClick();

    if (uiProcessor.getMode() != Mode::bypass
        || ! bypassButton->getToggleState()
        || countSelectedButtons() != 1)
    {
        std::cerr << "Repeated Bypass clicks did not remain in Bypass.\n";
        return 1;
    }

    juce::AudioBuffer<float> uiInput (2, 512);

    for (int sample = 0; sample < uiInput.getNumSamples(); ++sample)
    {
        uiInput.setSample (0, sample, static_cast<float> ((sample % 61) - 30) / 30.0f);
        uiInput.setSample (1, sample, static_cast<float> (((sample * 17) % 67) - 33) / 33.0f);
    }

    const auto uiBufferBytes = sizeof (float) * static_cast<size_t> (uiInput.getNumSamples());

    for (const auto& expectation : buttonExpectations)
    {
        auto* button = dynamic_cast<juce::TextButton*> (
            editor->findChildWithID (expectation.componentId));
        button->onClick();

        if (uiProcessor.getMode() != expectation.mode
            || ! button->getToggleState()
            || countSelectedButtons() != 1)
        {
            std::cerr << "A mode button did not update the single active mode.\n";
            return 1;
        }

        juce::AudioBuffer<float> uiOutput;

        for (int transitionBlock = 0; transitionBlock < 3; ++transitionBlock)
        {
            uiOutput.makeCopyOf (uiInput);
            uiProcessor.processBlock (uiOutput, midi);
        }

        uiOutput.makeCopyOf (uiInput);
        uiProcessor.processBlock (uiOutput, midi);

        const auto outputIsUnchanged = std::memcmp (uiOutput.getReadPointer (0),
                                                     uiInput.getReadPointer (0),
                                                     uiBufferBytes) == 0
            && std::memcmp (uiOutput.getReadPointer (1),
                            uiInput.getReadPointer (1),
                            uiBufferBytes) == 0;

        if ((expectation.mode == Mode::bypass) != outputIsUnchanged)
        {
            std::cerr << "A UI-selected mode did not produce its expected audio behavior.\n";
            return 1;
        }
    }

    uiProcessor.setMode (Mode::phone);
    std::this_thread::sleep_for (std::chrono::milliseconds (40));
    juce::Timer::callPendingTimersSynchronously();

    if (phoneButton == nullptr || ! phoneButton->getToggleState() || countSelectedButtons() != 1)
    {
        std::cerr << "The editor did not follow an external processor mode change.\n";
        return 1;
    }

    uiProcessor.setMode (Mode::bypass);
    std::this_thread::sleep_for (std::chrono::milliseconds (40));
    juce::Timer::callPendingTimersSynchronously();

    if (! bypassButton->getToggleState() || countSelectedButtons() != 1)
    {
        std::cerr << "The editor did not resynchronise to Bypass.\n";
        return 1;
    }

    const auto bypassSnapshotPath1x = juce::SystemStats::getEnvironmentVariable (
        "MIXCHECKER_UI_BYPASS_SNAPSHOT_1X", {});
    const auto bypassSnapshotPath2x = juce::SystemStats::getEnvironmentVariable (
        "MIXCHECKER_UI_BYPASS_SNAPSHOT_2X", {});
    const auto carSnapshotPath1x = juce::SystemStats::getEnvironmentVariable (
        "MIXCHECKER_UI_CAR_SNAPSHOT_1X", {});
    const auto carSnapshotPath2x = juce::SystemStats::getEnvironmentVariable (
        "MIXCHECKER_UI_CAR_SNAPSHOT_2X", {});

    if (bypassSnapshotPath1x.isNotEmpty() || bypassSnapshotPath2x.isNotEmpty()
        || carSnapshotPath1x.isNotEmpty() || carSnapshotPath2x.isNotEmpty())
    {
        const auto writeSnapshot = [&editor] (const juce::String& path, float scale)
        {
            if (path.isEmpty())
                return true;

            const auto snapshot = editor->createComponentSnapshot (
                editor->getLocalBounds(), true, scale);
            auto output = juce::File (path).createOutputStream();
            return ! snapshot.isNull()
                && output != nullptr
                && juce::PNGImageFormat().writeImageToStream (snapshot, *output);
        };

        if (! writeSnapshot (bypassSnapshotPath1x, 1.0f)
            || ! writeSnapshot (bypassSnapshotPath2x, 2.0f))
        {
            std::cerr << "Could not render the requested Bypass UI snapshot.\n";
            return 1;
        }

        uiProcessor.setMode (Mode::car);
        std::this_thread::sleep_for (std::chrono::milliseconds (40));
        juce::Timer::callPendingTimersSynchronously();

        if (! writeSnapshot (carSnapshotPath1x, 1.0f)
            || ! writeSnapshot (carSnapshotPath2x, 2.0f))
        {
            std::cerr << "Could not render the requested Car UI snapshot.\n";
            return 1;
        }
    }

    uiProcessor.releaseResources();

    struct StateExpectation
    {
        Mode mode;
        const char* componentId;
        const char* serialisedName;
    };

    constexpr std::array stateExpectations {
        StateExpectation { Mode::bypass, "bypass", "bypass" },
        StateExpectation { Mode::phone, "phone", "phone" },
        StateExpectation { Mode::laptop, "laptop", "laptop" },
        StateExpectation { Mode::earbuds, "earbuds", "earbuds" },
        StateExpectation { Mode::car, "car", "car" },
        StateExpectation { Mode::club, "club", "club" },
        StateExpectation { Mode::mono, "mono", "mono" }
    };

    for (const auto& expectation : stateExpectations)
    {
        MixCheckerAudioProcessor sourceProcessor;
        sourceProcessor.setMode (expectation.mode);
        juce::MemoryBlock savedState;
        sourceProcessor.getStateInformation (savedState);

        const auto savedXml = juce::AudioProcessor::getXmlFromBinary (
            savedState.getData(), static_cast<int> (savedState.getSize()));

        if (savedXml == nullptr
            || ! savedXml->hasTagName ("MixCheckerState")
            || savedXml->getStringAttribute ("mode") != expectation.serialisedName)
        {
            std::cerr << "A mode was not serialised in the expected minimal state format.\n";
            return 1;
        }

        MixCheckerAudioProcessor restoredProcessor;
        restoredProcessor.setStateInformation (savedState.getData(),
                                               static_cast<int> (savedState.getSize()));

        if (restoredProcessor.getMode() != expectation.mode)
        {
            std::cerr << "A saved mode was not restored correctly.\n";
            return 1;
        }

        restoredProcessor.prepareToPlay (48000.0, 512);
        sourceProcessor.prepareToPlay (48000.0, 512);

        juce::AudioBuffer<float> restoredOutput (2, 4096);

        for (int sample = 0; sample < restoredOutput.getNumSamples(); ++sample)
        {
            restoredOutput.setSample (0, sample,
                                      static_cast<float> ((sample % 181) - 90) / 180.0f);
            restoredOutput.setSample (1, sample,
                                      static_cast<float> (((sample * 29) % 191) - 95) / 190.0f);
        }

        juce::AudioBuffer<float> expectedOutput;
        expectedOutput.makeCopyOf (restoredOutput);
        restoredProcessor.processBlock (restoredOutput, midi);
        sourceProcessor.processBlock (expectedOutput, midi);

        const auto restoredBytes = static_cast<size_t> (restoredOutput.getNumSamples())
            * sizeof (float);

        if (std::memcmp (restoredOutput.getReadPointer (0),
                         expectedOutput.getReadPointer (0), restoredBytes) != 0
            || std::memcmp (restoredOutput.getReadPointer (1),
                            expectedOutput.getReadPointer (1), restoredBytes) != 0)
        {
            std::cerr << "Restored DSP did not match the saved mode.\n";
            return 1;
        }

        std::unique_ptr<juce::AudioProcessorEditor> restoredEditor (
            restoredProcessor.createEditor());
        int restoredSelectedCount = 0;

        for (const auto& buttonExpectation : buttonExpectations)
        {
            const auto* button = dynamic_cast<juce::TextButton*> (
                restoredEditor->findChildWithID (buttonExpectation.componentId));

            if (button != nullptr && button->getToggleState())
                ++restoredSelectedCount;
        }

        const auto* restoredButton = dynamic_cast<juce::TextButton*> (
            restoredEditor->findChildWithID (expectation.componentId));

        if (restoredButton == nullptr
            || ! restoredButton->getToggleState()
            || restoredSelectedCount != 1)
        {
            std::cerr << "The editor did not display the restored mode.\n";
            return 1;
        }

        restoredProcessor.releaseResources();
        sourceProcessor.releaseResources();
    }

    const auto expectInvalidStateToBypass = [&midi] (const void* stateData, int stateSize)
    {
        MixCheckerAudioProcessor invalidStateProcessor;
        invalidStateProcessor.setMode (Mode::car);
        invalidStateProcessor.setStateInformation (stateData, stateSize);

        if (invalidStateProcessor.getMode() != Mode::bypass)
            return false;

        invalidStateProcessor.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> invalidStateOutput (2, 512);

        for (int sample = 0; sample < invalidStateOutput.getNumSamples(); ++sample)
        {
            invalidStateOutput.setSample (0, sample, static_cast<float> (sample - 256) / 512.0f);
            invalidStateOutput.setSample (1, sample, static_cast<float> (127 - sample) / 384.0f);
        }

        juce::AudioBuffer<float> invalidStateInput;
        invalidStateInput.makeCopyOf (invalidStateOutput);
        invalidStateProcessor.processBlock (invalidStateOutput, midi);

        const auto stateBufferBytes = static_cast<size_t> (invalidStateOutput.getNumSamples())
            * sizeof (float);
        const auto isTransparent = std::memcmp (invalidStateOutput.getReadPointer (0),
                                                invalidStateInput.getReadPointer (0),
                                                stateBufferBytes) == 0
            && std::memcmp (invalidStateOutput.getReadPointer (1),
                            invalidStateInput.getReadPointer (1),
                            stateBufferBytes) == 0;
        invalidStateProcessor.releaseResources();
        return isTransparent;
    };

    juce::MemoryBlock missingModeState;
    juce::ValueTree missingModeTree ("MixCheckerState");

    if (const auto xml = missingModeTree.createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, missingModeState);

    juce::MemoryBlock unknownModeState;
    juce::ValueTree unknownModeTree ("MixCheckerState");
    unknownModeTree.setProperty ("mode", "spaceship", nullptr);

    if (const auto xml = unknownModeTree.createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, unknownModeState);

    juce::MemoryBlock wrongTypeState;
    juce::ValueTree wrongTypeTree ("SomeOtherPlugin");
    wrongTypeTree.setProperty ("mode", "car", nullptr);

    if (const auto xml = wrongTypeTree.createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, wrongTypeState);

    constexpr std::array corruptState { 'n', 'o', 't', '-', 'x', 'm', 'l' };

    if (! expectInvalidStateToBypass (nullptr, 0)
        || ! expectInvalidStateToBypass (missingModeState.getData(),
                                         static_cast<int> (missingModeState.getSize()))
        || ! expectInvalidStateToBypass (unknownModeState.getData(),
                                         static_cast<int> (unknownModeState.getSize()))
        || ! expectInvalidStateToBypass (wrongTypeState.getData(),
                                         static_cast<int> (wrongTypeState.getSize()))
        || ! expectInvalidStateToBypass (corruptState.data(),
                                         static_cast<int> (corruptState.size())))
    {
        std::cerr << "Invalid or missing state did not fall back to transparent Bypass.\n";
        return 1;
    }

    return 0;
}
