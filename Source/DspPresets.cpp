#include "DspPresets.h"

namespace MixCheckerDspPresets
{
    MixCheckerDspEngine::Settings neutral()
    {
        MixCheckerDspEngine::Settings settings;
        settings.outputGainDb = 0.0f;
        return settings;
    }

    MixCheckerDspEngine::Settings phone()
    {
        MixCheckerDspEngine::Settings settings;
        settings.highPassEnabled = true;
        settings.highPassFrequencyHz = 180.0f;
        settings.equalizerEnabled = true;
        settings.equalizerFrequencyHz = 1600.0f;
        settings.equalizerGainDb = 3.0f;
        settings.equalizerQ = 0.8f;
        settings.lowPassEnabled = true;
        settings.lowPassFrequencyHz = 7500.0f;
        settings.stereoWidth = 0.35f;
        settings.outputGainDb = 1.0f;
        return settings;
    }

    MixCheckerDspEngine::Settings laptop()
    {
        MixCheckerDspEngine::Settings settings;
        settings.highPassEnabled = true;
        settings.highPassFrequencyHz = 120.0f;
        settings.equalizerEnabled = true;
        settings.equalizerFrequencyHz = 1800.0f;
        settings.equalizerGainDb = 2.0f;
        settings.equalizerQ = 0.8f;
        settings.lowPassEnabled = true;
        settings.lowPassFrequencyHz = 14000.0f;
        settings.stereoWidth = 0.75f;
        settings.outputGainDb = 0.0f;
        return settings;
    }

    MixCheckerDspEngine::Settings earbuds()
    {
        MixCheckerDspEngine::Settings settings;
        settings.highPassEnabled = true;
        settings.highPassFrequencyHz = 30.0f;
        settings.equalizerEnabled = true;
        settings.equalizerFrequencyHz = 110.0f;
        settings.equalizerGainDb = 2.0f;
        settings.equalizerQ = 0.8f;
        settings.lowPassEnabled = true;
        settings.lowPassFrequencyHz = 18000.0f;
        settings.stereoWidth = 1.05f;
        settings.outputGainDb = -1.0f;
        return settings;
    }

    MixCheckerDspEngine::Settings car()
    {
        MixCheckerDspEngine::Settings settings;
        settings.highPassEnabled = true;
        settings.highPassFrequencyHz = 30.0f;
        settings.equalizerEnabled = true;
        settings.equalizerFrequencyHz = 180.0f;
        settings.equalizerGainDb = 3.0f;
        settings.equalizerQ = 0.7f;
        settings.lowPassEnabled = true;
        settings.lowPassFrequencyHz = 12000.0f;
        settings.stereoWidth = 1.15f;
        settings.outputGainDb = -1.5f;
        return settings;
    }

    MixCheckerDspEngine::Settings club()
    {
        MixCheckerDspEngine::Settings settings;
        settings.highPassEnabled = true;
        settings.highPassFrequencyHz = 25.0f;
        settings.equalizerEnabled = true;
        settings.equalizerFrequencyHz = 75.0f;
        settings.equalizerGainDb = 3.0f;
        settings.equalizerQ = 0.8f;
        settings.lowPassEnabled = true;
        settings.lowPassFrequencyHz = 18000.0f;
        settings.stereoWidth = 1.10f;
        settings.outputGainDb = -1.5f;
        return settings;
    }

    MixCheckerDspEngine::Settings mono()
    {
        MixCheckerDspEngine::Settings settings;
        settings.stereoWidth = 0.0f;
        settings.outputGainDb = 0.0f;
        return settings;
    }
}
