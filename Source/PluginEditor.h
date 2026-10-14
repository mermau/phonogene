#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Aspetto di laboratorio anni '50: smalto martellato, bachelite, cromature.
class PhonoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PhonoLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
};

// Deck con l'anello di nastro, la testina e i dodici capstan.
class DeckView : public juce::Component
{
public:
    explicit DeckView (PhonogeneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void advance (float dt);

private:
    PhonogeneProcessor& proc;
    float tapePhase = 0.0f;
    float capstanAngle = 0.0f;
};

// Finestra con il contenuto del nastro, la regione dell'anello e la testina.
class TapeStrip : public juce::Component
{
public:
    explicit TapeStrip (PhonogeneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    PhonogeneProcessor& proc;
};

// Tastiera di un'ottava.
class OctaveKeyboard : public juce::Component
{
public:
    explicit OctaveKeyboard (PhonogeneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    int keyAt (juce::Point<float>) const;
    juce::Rectangle<float> keyBounds (int key) const;
    static bool isBlack (int key) { return key == 1 || key == 3 || key == 6 || key == 8 || key == 10; }

    PhonogeneProcessor& proc;
    int mouseKey = -1;
};

class PhonogeneEditor : public juce::AudioProcessorEditor,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit PhonogeneEditor (PhonogeneProcessor&);
    ~PhonogeneEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::String label;
        std::unique_ptr<SliderAtt> att;
    };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& label,
                    juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag);
    void setupToggle (juce::ToggleButton&, std::unique_ptr<ButtonAtt>&, const juce::String& paramId,
                      const juce::String& label);
    void openFileChooser();
    void buildTexture();

    PhonogeneProcessor& proc;
    PhonoLookAndFeel lnf;

    DeckView deck;
    TapeStrip strip;
    OctaveKeyboard keyboard;

    Knob kStart, kLength, kInertia, kLevel, kWow, kHiss, kDrive, kMode, kSource, kCoulisse;
    juce::ToggleButton tRecord, tMonitor, tRestart, tExtended;
    std::unique_ptr<ButtonAtt> aRecord, aMonitor, aRestart, aExtended;
    juce::TextButton loadButton { "CHARGER" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image texture;
};
