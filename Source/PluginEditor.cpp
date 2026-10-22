#include "PluginEditor.h"
#include "BinaryData.h"

#include <array>
#include <utility>

namespace
{
    constexpr int editorWidth = 474;
    constexpr int editorHeight = 208;
    constexpr int modeButtonLeft = 23;
    constexpr int modeButtonTop = 85;
    constexpr int modeButtonWidth = 62;
    constexpr int modeButtonHeight = 60;
    constexpr int modeButtonGap = 11;
    constexpr int bypassButtonTop = 160;

    constexpr std::array screwCentres {
        juce::Point<float> { 21.0f, 29.0f },
        juce::Point<float> { 453.0f, 29.0f },
        juce::Point<float> { 21.0f, 181.0f },
        juce::Point<float> { 453.0f, 181.0f }
    };

    const auto warmWhite = juce::Colour { 0xffffead0 };
    const auto engravedText = juce::Colour { 0xff252728 };

    juce::Image loadEmbeddedImage (const void* data, int dataSize)
    {
        auto image = juce::ImageCache::getFromMemory (data, dataSize);
        jassert (image.isValid());
        return image;
    }

    juce::Rectangle<float> modeButtonBounds (int index)
    {
        return { static_cast<float> (modeButtonLeft + index * (modeButtonWidth + modeButtonGap)),
                 static_cast<float> (modeButtonTop),
                 static_cast<float> (modeButtonWidth),
                 static_cast<float> (modeButtonHeight) };
    }
}

MixCheckerLookAndFeel::MixCheckerLookAndFeel (juce::Image modeUpImage,
                                               juce::Image modeDownImage,
                                               juce::Image bypassUpImage,
                                               juce::Image bypassDownImage)
    : modeUp (std::move (modeUpImage)),
      modeDown (std::move (modeDownImage)),
      bypassUp (std::move (bypassUpImage)),
      bypassDown (std::move (bypassDownImage))
{
}

void MixCheckerLookAndFeel::drawButtonBackground (juce::Graphics& graphics,
                                                   juce::Button& button,
                                                   const juce::Colour&,
                                                   bool isMouseOverButton,
                                                   bool isButtonDown)
{
    const auto selected = button.getToggleState();
    const auto compact = button.getComponentID() == "bypass";
    const auto pressed = selected || isButtonDown;
    const auto& image = compact ? (pressed ? bypassDown : bypassUp)
                                : (pressed ? modeDown : modeUp);

    graphics.setOpacity (1.0f);
    graphics.drawImage (image, button.getLocalBounds().toFloat());

    if (isMouseOverButton && ! pressed)
    {
        graphics.setColour (juce::Colours::white.withAlpha (0.035f));
        graphics.fillRoundedRectangle (button.getLocalBounds().toFloat().reduced (3.0f),
                                       compact ? 3.0f : 5.0f);
    }
}

void MixCheckerLookAndFeel::drawButtonText (juce::Graphics& graphics,
                                             juce::TextButton& button,
                                             bool,
                                             bool isButtonDown)
{
    const auto compact = button.getComponentID() == "bypass";
    const auto down = button.getToggleState() || isButtonDown;
    auto textBounds = button.getLocalBounds().translated (0, down ? 2 : 0);

    graphics.setColour (juce::Colours::black.withAlpha (0.65f));
    graphics.setFont (juce::Font (juce::FontOptions { 10.0f }.withStyle ("Medium"))
                          .withExtraKerningFactor (compact ? 0.07f : 0.025f));
    graphics.drawFittedText (button.getButtonText(), textBounds.translated (0, 1),
                             juce::Justification::centred, 1);
    graphics.setColour (button.getToggleState() ? warmWhite.brighter (0.11f) : warmWhite);
    graphics.drawFittedText (button.getButtonText(), textBounds,
                             juce::Justification::centred, 1);
}

MixCheckerAudioProcessorEditor::MixCheckerAudioProcessorEditor (MixCheckerAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      audioProcessor (processorToUse),
      faceplate (loadEmbeddedImage (BinaryData::faceplate_3x_png, BinaryData::faceplate_3x_pngSize)),
      modeUp (loadEmbeddedImage (BinaryData::mode_up_3x_png, BinaryData::mode_up_3x_pngSize)),
      modeDown (loadEmbeddedImage (BinaryData::mode_down_3x_png, BinaryData::mode_down_3x_pngSize)),
      bypassUp (loadEmbeddedImage (BinaryData::bypass_up_3x_png, BinaryData::bypass_up_3x_pngSize)),
      bypassDown (loadEmbeddedImage (BinaryData::bypass_down_3x_png, BinaryData::bypass_down_3x_pngSize)),
      lampOff (loadEmbeddedImage (BinaryData::lamp_off_3x_png, BinaryData::lamp_off_3x_pngSize)),
      lampOn (loadEmbeddedImage (BinaryData::lamp_on_3x_png, BinaryData::lamp_on_3x_pngSize)),
      screw (loadEmbeddedImage (BinaryData::screw_3x_png, BinaryData::screw_3x_pngSize)),
      lookAndFeel (modeUp, modeDown, bypassUp, bypassDown)
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
    graphics.fillAll (juce::Colour { 0xff111314 });
    graphics.setOpacity (1.0f);
    graphics.drawImage (faceplate, getLocalBounds().toFloat());

    for (const auto centre : screwCentres)
        drawScrew (graphics, centre);

    graphics.setColour (juce::Colours::white.withAlpha (0.42f));
    graphics.setFont (juce::Font (juce::FontOptions { 17.0f }.withStyle ("Regular"))
                          .withExtraKerningFactor (0.10f));
    graphics.drawText ("MIX CHECKER", 34, 30, 300, 20,
                       juce::Justification::centredLeft);
    graphics.setColour (engravedText);
    graphics.drawText ("MIX CHECKER", 34, 29, 300, 20,
                       juce::Justification::centredLeft);

    graphics.setFont (juce::Font (juce::FontOptions { 7.5f }.withStyle ("Regular"))
                          .withExtraKerningFactor (0.07f));
    graphics.setColour (engravedText.withAlpha (0.78f));
    graphics.drawText ("MIX TRANSLATION MONITOR", 34, 50, 210, 10,
                       juce::Justification::centredLeft);

    const auto bypassed = displayedMode == MixCheckerAudioProcessor::Mode::bypass;
    drawIndicator (graphics, { 420.0f, 35.0f }, ! bypassed, 9.0f);
    graphics.setColour (engravedText.withAlpha (0.82f));
    graphics.setFont (juce::Font (juce::FontOptions { 7.5f }.withStyle ("Medium"))
                          .withExtraKerningFactor (0.07f));
    graphics.drawText ("ACTIVE", 401, 40, 38, 10, juce::Justification::centred);

    const std::array modes {
        MixCheckerAudioProcessor::Mode::phone,
        MixCheckerAudioProcessor::Mode::laptop,
        MixCheckerAudioProcessor::Mode::earbuds,
        MixCheckerAudioProcessor::Mode::car,
        MixCheckerAudioProcessor::Mode::club,
        MixCheckerAudioProcessor::Mode::mono
    };

    for (int index = 0; index < static_cast<int> (modes.size()); ++index)
    {
        const auto bounds = modeButtonBounds (index);
        drawIndicator (graphics, { bounds.getCentreX(), 72.0f },
                       ! bypassed && displayedMode == modes[static_cast<size_t> (index)], 11.0f);
    }

    graphics.setColour (engravedText.withAlpha (0.76f));
    graphics.setFont (juce::Font (juce::FontOptions { 7.5f }.withStyle ("Regular"))
                          .withExtraKerningFactor (0.06f));
    graphics.drawText ("STEREO MONITORING", 304, 174, 134, 10,
                       juce::Justification::centredRight);

}

void MixCheckerAudioProcessorEditor::resized()
{
    std::array<juce::TextButton*, 6> buttons {
        &phoneButton, &laptopButton, &earbudsButton, &carButton, &clubButton, &monoButton
    };

    for (int index = 0; index < static_cast<int> (buttons.size()); ++index)
        buttons[static_cast<size_t> (index)]->setBounds (modeButtonBounds (index).toNearestInt());

    bypassButton.setBounds (30, bypassButtonTop, 76, 24);
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
    button.setClickingTogglesState (false);
    button.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    button.onClick = [this, mode]
    {
        const auto currentMode = audioProcessor.getMode();
        const auto targetMode = mode != MixCheckerAudioProcessor::Mode::bypass
                                    && currentMode == mode
            ? MixCheckerAudioProcessor::Mode::bypass
            : mode;

        audioProcessor.setMode (targetMode);
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
    repaint();
}

void MixCheckerAudioProcessorEditor::drawIndicator (juce::Graphics& graphics,
                                                     juce::Point<float> centre,
                                                     bool isLit,
                                                     float diameter) const
{
    const auto bounds = juce::Rectangle<float> (diameter, diameter).withCentre (centre);
    graphics.setOpacity (1.0f);
    graphics.drawImage (isLit ? lampOn : lampOff, bounds);
}

void MixCheckerAudioProcessorEditor::drawScrew (juce::Graphics& graphics,
                                                 juce::Point<float> centre) const
{
    const auto bounds = juce::Rectangle<float> (7.0f, 7.0f).withCentre (centre);
    graphics.setOpacity (1.0f);
    graphics.drawImage (screw, bounds);
}
