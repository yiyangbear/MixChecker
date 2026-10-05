#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MixCheckerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& graphics,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override;

    void drawButtonText (juce::Graphics& graphics,
                         juce::TextButton& button,
                         bool isMouseOverButton,
                         bool isButtonDown) override;
};

class MixCheckerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit MixCheckerAudioProcessorEditor (MixCheckerAudioProcessor& processor);
    ~MixCheckerAudioProcessorEditor() override;

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureButton (juce::TextButton& button,
                          const juce::String& componentId,
                          MixCheckerAudioProcessor::Mode mode);
    void updateButtonStates();

    MixCheckerAudioProcessor& audioProcessor;
    MixCheckerLookAndFeel lookAndFeel;
    MixCheckerAudioProcessor::Mode displayedMode { MixCheckerAudioProcessor::Mode::bypass };

    juce::TextButton phoneButton { "PHONE" };
    juce::TextButton laptopButton { "LAPTOP" };
    juce::TextButton earbudsButton { "EARBUDS" };
    juce::TextButton carButton { "CAR" };
    juce::TextButton clubButton { "CLUB" };
    juce::TextButton monoButton { "MONO" };
    juce::TextButton bypassButton { "BYPASS" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixCheckerAudioProcessorEditor)
};
