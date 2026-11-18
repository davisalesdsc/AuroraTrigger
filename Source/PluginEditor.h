#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
// Aurora palette
//==============================================================================
namespace AuroraColours
{
    const juce::Colour panelTop    { 0xff666a73 };
    const juce::Colour panelBottom { 0xff4d5159 };
    const juce::Colour stripTop    { 0xffaab0bc };
    const juce::Colour stripBottom { 0xff7b818d };
    const juce::Colour ink         { 0xff1a1d24 };
    const juce::Colour readoutBg   { 0xff0d0f13 };
    const juce::Colour aurora      { 0xff38bdf8 };   // signature blue
    const juce::Colour auroraGreen { 0xff34d399 };
    const juce::Colour auroraViolet{ 0xffa78bfa };
    const juce::Colour pink        { 0xfff0708a };   // gate / detector knobs
    const juce::Colour blueKnob    { 0xff5b8def };   // filters / volumes
    const juce::Colour purple      { 0xffa855f7 };   // pan
    const juce::Colour copper      { 0xffc99a7c };   // tune
}

//==============================================================================
class AuroraMetallicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraMetallicLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOver, bool isButtonDown) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isMouseOver, bool isButtonDown) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

//==============================================================================
// Segmented stereo LED meter, 0 .. -48 dB with printed scale
//==============================================================================
class LedMeterComponent : public juce::Component
{
public:
    explicit LedMeterComponent (std::array<std::atomic<float>, 2>& levelSource) : levels (levelSource) {}
    void paint (juce::Graphics&) override;

private:
    std::array<std::atomic<float>, 2>& levels;
    std::array<float, 2> held { 0.0f, 0.0f };
};

//==============================================================================
class WaveformScopeComponent : public juce::Component
{
public:
    explicit WaveformScopeComponent (AuroraTriggerAudioProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void updateDetailFromMouseY (int y);
    AuroraTriggerAudioProcessor& processor;
};

//==============================================================================
// Button that starts a native drag-and-drop of the captured .mid when dragged
//==============================================================================
class DragMidiButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;
    std::function<void()> onDragStart;

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging && e.getDistanceFromDragStart() > 4 && onDragStart)
        {
            dragging = true;
            onDragStart();
        }
        juce::TextButton::mouseDrag (e);
    }
    void mouseUp (const juce::MouseEvent& e) override { dragging = false; juce::TextButton::mouseUp (e); }

private:
    bool dragging = false;
};

//==============================================================================
class AuroraTriggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          private juce::Timer
{
public:
    explicit AuroraTriggerAudioProcessorEditor (AuroraTriggerAudioProcessor&);
    ~AuroraTriggerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Used by the preview/screenshot tooling
    void showTab (int tabIndex) { setActiveTab (tabIndex); }

private:
    void timerCallback() override;
    void setActiveTab (int tabIndex);
    void loadSampleViaChooser (int slotIndex);
    void dragCapturedMidiToDaw();
    void applyMixerVisibility();

    AuroraTriggerAudioProcessor& audioProcessor;
    AuroraMetallicLookAndFeel metallicLnf;

    int activeViewTab = 1; // 0 = Browser, 1 = Triggering, 2 = Settings
    bool mixerVisible = true;
    int selectedSlot = 0;
    int midiBoxY = 546;
    std::array<bool, AuroraTriggerAudioProcessor::numSlots> padWasLoaded {};

    // Left Navigation Column
    juce::TextButton browserBtn     { "Browser" };
    juce::TextButton triggeringBtn  { "Triggering" };
    juce::TextButton settingsBtn    { "Settings" };

    juce::TextButton viewMixerBtn   { "View Mixer" };
    juce::TextButton viewCurvesBtn  { "View Curves" };

    juce::ToggleButton linkTuneBtn  { "LINK TUNE" };
    juce::TextButton flipPhasesBtn  { "FLIP ALL PHASES" };
    juce::TextButton savePresetBtn  { "SAVE PRESET" };

    DragMidiButton   dragOnTrackBtn { "DRAG ON TRACK" };
    juce::TextButton clearBufferBtn { "CLEAR BUFFER" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkTuneAtt;

    // Triggering page
    juce::ToggleButton gateOnBtn { "On" };
    juce::Slider gateAttSlider, gateHoldSlider, gateRelSlider;
    juce::Slider inVolSlider;
    LedMeterComponent inputMeter;

    juce::Slider lowcutSlider, highcutSlider, suppressSlider;
    WaveformScopeComponent waveformScope;

    juce::Slider sensSlider, retrigSlider, detailSlider;
    LedMeterComponent outputMeter;
    juce::Slider outVolSlider, mixSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAttAtt, gateHoldAtt, gateRelAtt, inVolAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowcutAtt, highcutAtt, suppressAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensAtt, retrigAtt, detailAtt, outVolAtt, mixAtt;

    // Browser & Settings views
    juce::TextButton loadSampleBtn { "Load .WAV / .AIFF into Slot" };
    juce::TextButton auditionSlotBtn { "Audition Selected Slot" };
    juce::TextButton resetChilliKitBtn { "Reload Chilli Snare Kit" };
    juce::Label browserViewLabel;
    juce::Label settingsViewLabel;

    // 8-channel mixer strips with bottom trigger pads
    struct MixerStripUI
    {
        juce::Slider panSlider;
        juce::Label  panReadout;
        juce::Slider tuneSlider;
        juce::Label  tuneReadout;
        juce::Slider volSlider;
        juce::Label  volReadout;
        juce::TextButton soloBtn  { "S" };
        juce::TextButton muteBtn  { "M" };
        juce::TextButton phaseBtn { juce::CharPointer_UTF8 ("\xc3\x98") }; // Ø
        juce::Label  artLabel;
        juce::TextButton bottomPadBtn { "Empty" };

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAtt, tuneAtt, volAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAtt, muteAtt, phaseAtt;
    };
    std::array<MixerStripUI, AuroraTriggerAudioProcessor::numSlots> strips;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Image brushedTile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraTriggerAudioProcessorEditor)
};
