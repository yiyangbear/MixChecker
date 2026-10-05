#include "PluginEditor.h"

namespace
{
    constexpr int editorWidth = 560;
    constexpr int editorHeight = 260;
    constexpr int modeButtonGroup = 1001;

    const auto background = juce::Colour { 0xff292620 };
    const auto panelSurface = juce::Colour { 0xff211f1b };
    const auto panelBorder = juce::Colour { 0xff4b453b };
    const auto inactiveSurface = juce::Colour { 0xff312e29 };
    const auto activeSurface = juce::Colour { 0xff40362b };
    const auto inactiveBorder = juce::Colour { 0xff565046 };
    const auto activeBorder = juce::Colour { 0xffa8743f };
    const auto textColour = juce::Colour { 0xffe5dece };
    const auto mutedTextColour = juce::Colour { 0xff948b7d };
    const auto accent = juce::Colour { 0xffd0934b };
    const auto inactiveIndicator = juce::Colour { 0xff171613 };
}

void MixCheckerLookAndFeel::drawButtonBackground (juce::Graphics& graphics,
                                                   juce::Button& button,
                                                   const juce::Colour&,
                                                   bool isMouseOverButton,
                                                   bool isButtonDown)
{
    const auto isActive = button.getToggleState();
    auto fill = isActive ? activeSurface : inactiveSurface;

    if (isButtonDown)
        fill = fill.darker (0.08f);
    else if (isMouseOverButton)
        fill = fill.brighter (0.045f);

    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto shadowBounds = bounds.translated (0.0f, 1.0f);

    graphics.setColour (juce::Colours::black.withAlpha (0.26f));
    graphics.fillRoundedRectangle (shadowBounds, 7.0f);

    graphics.setColour (fill);
    graphics.fillRoundedRectangle (bounds, 7.0f);

    graphics.setColour (isActive ? activeBorder : inactiveBorder);
    graphics.drawRoundedRectangle (bounds, 7.0f, 1.0f);

    graphics.setColour (juce::Colours::white.withAlpha (isActive ? 0.075f : 0.05f));
    graphics.drawLine (bounds.getX() + 7.0f,
                       bounds.getY() + 1.5f,
                       bounds.getRight() - 7.0f,
                       bounds.getY() + 1.5f,
                       0.75f);

    graphics.setColour (juce::Colours::black.withAlpha (0.24f));
    graphics.drawLine (bounds.getX() + 7.0f,
                       bounds.getBottom() - 1.5f,
                       bounds.getRight() - 7.0f,
                       bounds.getBottom() - 1.5f,
                       0.8f);

    const juce::Point<float> indicatorCentre { bounds.getX() + 14.0f, bounds.getCentreY() };
    graphics.setColour (juce::Colours::black.withAlpha (0.55f));
    graphics.fillEllipse (indicatorCentre.x - 4.0f,
                          indicatorCentre.y - 4.0f,
                          8.0f,
                          8.0f);
    graphics.setColour (isActive ? accent : inactiveIndicator);
    graphics.fillEllipse (indicatorCentre.x - 2.65f,
                          indicatorCentre.y - 2.65f,
                          5.3f,
                          5.3f);

    if (isActive)
    {
        graphics.setColour (juce::Colours::white.withAlpha (0.26f));
        graphics.fillEllipse (indicatorCentre.x - 1.5f,
                              indicatorCentre.y - 1.65f,
                              1.4f,
                              1.4f);
    }
}

void MixCheckerLookAndFeel::drawButtonText (juce::Graphics& graphics,
                                             juce::TextButton& button,
                                             bool,
                                             bool)
{
    graphics.setColour (button.getToggleState() ? textColour.brighter (0.08f) : textColour);
    graphics.setFont (juce::Font (juce::FontOptions { 11.5f, juce::Font::bold }));
    graphics.drawFittedText (button.getButtonText(),
                             button.getLocalBounds().reduced (24, 2),
                             juce::Justification::centred,
                             1);
}

MixCheckerAudioProcessorEditor::MixCheckerAudioProcessorEditor (MixCheckerAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse), audioProcessor (processorToUse)
{
    setLookAndFeel (&lookAndFeel);

    configureButton (phoneButton, "phone", MixCheckerAudioProcessor::Mode::phone);
    configureButton (laptopButton, "laptop", MixCheckerAudioProcessor::Mode::laptop);
    configureButton (earbudsButton, "earbuds", MixCheckerAudioProcessor::Mode::earbuds);
    configureButton (carButton, "car", MixCheckerAudioProcessor::Mode::car);
    configureButton (clubButton, "club", MixCheckerAudioProcessor::Mode::club);
    configureButton (monoButton, "mono", MixCheckerAudioProcessor::Mode::mono);
    configureButton (bypassButton, "bypass", MixCheckerAudioProcessor::Mode::bypass);

    setResizable (false, false);
    setSize (editorWidth, editorHeight);
    updateButtonStates();
    startTimerHz (30);
}

MixCheckerAudioProcessorEditor::~MixCheckerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MixCheckerAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (background);

    const auto frame = getLocalBounds().toFloat().reduced (0.5f);
    graphics.setColour (panelBorder);
    graphics.drawRoundedRectangle (frame, 8.0f, 1.0f);

    graphics.setColour (textColour);
    graphics.setFont (juce::Font (juce::FontOptions { 18.0f, juce::Font::bold })
                          .withExtraKerningFactor (0.055f));
    graphics.drawText ("MIX CHECKER", 0, 10, getWidth(), 22, juce::Justification::centred);

    graphics.setColour (mutedTextColour);
    graphics.setFont (juce::Font (juce::FontOptions { 9.0f, juce::Font::plain })
                          .withExtraKerningFactor (0.08f));
    graphics.drawText ("TRANSLATION MONITOR", 0, 31, getWidth(), 14, juce::Justification::centred);

    graphics.setColour (panelBorder.withAlpha (0.68f));
    graphics.drawHorizontalLine (52, 24.0f, static_cast<float> (getWidth() - 24));

    const auto controlPanel = juce::Rectangle<float> { 24.0f, 60.0f, 512.0f, 132.0f };
    graphics.setColour (juce::Colours::black.withAlpha (0.18f));
    graphics.fillRoundedRectangle (controlPanel.translated (0.0f, 1.0f), 8.0f);
    graphics.setColour (panelSurface);
    graphics.fillRoundedRectangle (controlPanel, 8.0f);
    graphics.setColour (panelBorder.withAlpha (0.72f));
    graphics.drawRoundedRectangle (controlPanel, 8.0f, 1.0f);
    graphics.setColour (juce::Colours::white.withAlpha (0.025f));
    graphics.drawLine (32.0f, 61.5f, 528.0f, 61.5f, 0.75f);

    graphics.setColour (mutedTextColour);
    graphics.setFont (juce::Font (juce::FontOptions { 8.5f, juce::Font::bold })
                          .withExtraKerningFactor (0.09f));
    graphics.drawText ("PLAYBACK ENVIRONMENT", 44, 67, 472, 12, juce::Justification::centredLeft);
    graphics.drawText ("SOURCE", 0, 199, getWidth(), 12, juce::Justification::centred);
}

void MixCheckerAudioProcessorEditor::resized()
{
    constexpr int left = 44;
    constexpr int contentWidth = editorWidth - (left * 2);
    constexpr int gap = 14;
    constexpr int buttonWidth = (contentWidth - (gap * 2)) / 3;
    constexpr int buttonHeight = 38;

    phoneButton.setBounds (left, 83, buttonWidth, buttonHeight);
    laptopButton.setBounds (left + buttonWidth + gap, 83, buttonWidth, buttonHeight);
    earbudsButton.setBounds (left + (buttonWidth + gap) * 2, 83, buttonWidth, buttonHeight);

    carButton.setBounds (left, 132, buttonWidth, buttonHeight);
    clubButton.setBounds (left + buttonWidth + gap, 132, buttonWidth, buttonHeight);
    monoButton.setBounds (left + (buttonWidth + gap) * 2, 132, buttonWidth, buttonHeight);

    bypassButton.setBounds ((editorWidth - buttonWidth) / 2, 215, buttonWidth, 32);
}

void MixCheckerAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.getMode() != displayedMode)
        updateButtonStates();
}

void MixCheckerAudioProcessorEditor::configureButton (juce::TextButton& button,
                                                        const juce::String& componentId,
                                                        MixCheckerAudioProcessor::Mode mode)
{
    button.setComponentID (componentId);
    button.setClickingTogglesState (true);
    button.setRadioGroupId (modeButtonGroup, juce::dontSendNotification);
    button.onClick = [this, mode]
    {
        audioProcessor.setMode (mode);
        updateButtonStates();
    };
    addAndMakeVisible (button);
}

void MixCheckerAudioProcessorEditor::updateButtonStates()
{
    const auto activeMode = audioProcessor.getMode();
    const auto setActive = [activeMode] (juce::TextButton& button,
                                         MixCheckerAudioProcessor::Mode mode)
    {
        button.setToggleState (activeMode == mode, juce::dontSendNotification);
    };

    setActive (phoneButton, MixCheckerAudioProcessor::Mode::phone);
    setActive (laptopButton, MixCheckerAudioProcessor::Mode::laptop);
    setActive (earbudsButton, MixCheckerAudioProcessor::Mode::earbuds);
    setActive (carButton, MixCheckerAudioProcessor::Mode::car);
    setActive (clubButton, MixCheckerAudioProcessor::Mode::club);
    setActive (monoButton, MixCheckerAudioProcessor::Mode::mono);
    setActive (bypassButton, MixCheckerAudioProcessor::Mode::bypass);
    displayedMode = activeMode;
}
