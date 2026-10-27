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

// Manopola che, se è un selettore, scatta alla posizione successiva con un clic
// e cambia posizione con un breve trascinamento.
class PhonoSlider : public juce::Slider
{
public:
    // ctrl+clic (riassegnazione del CC) e tasto destro (menu MIDI) sono gestiti dall'editor
    void mouseDown (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::Slider::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::Slider::mouseDrag (e); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || e.mods.isCtrlDown()) return;
        const bool wasClick = ! e.mouseWasDraggedSinceMouseDown();
        juce::Slider::mouseUp (e);

        if (wasClick && getProperties().contains ("selector") && isEnabled())
        {
            double v = std::round (getValue()) + 1.0;
            if (v > getMaximum()) v = getMinimum();
            setValue (v, juce::sendNotificationSync);
        }
    }
};

class PhonoToggle : public juce::ToggleButton
{
public:
    void mouseDown (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::ToggleButton::mouseDown (e); }
    void mouseUp (const juce::MouseEvent& e) override   { if (! e.mods.isPopupMenu() && ! e.mods.isCtrlDown()) juce::ToggleButton::mouseUp (e); }
};

// Deck con l'anello di nastro, la testina e i dodici capstan.
class DeckView : public juce::Component
{
public:
    explicit DeckView (PhonogeneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void advance (float dt);

private:
    void paintStatic (juce::Graphics&, int mode, int activeKey, float scaleMax);

    PhonogeneProcessor& proc;
    float tapePhase = 0.0f;
    float capstanAngle = 0.0f;
    juce::Image cache;      // parti fisse del deck
    int cacheKey = -1;
};

// Finestra con il contenuto del nastro, la regione dell'anello e la testina.
class TapeStrip : public juce::Component
{
public:
    explicit TapeStrip (PhonogeneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void refreshIfChanged();   // ridisegna solo se qualcosa di visibile è cambiato

private:
    PhonogeneProcessor& proc;
    double lastSignature = -1.0;
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
    void refreshIfChanged();

private:
    int keyAt (juce::Point<float>) const;
    int lastShown = -2;
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
    void mouseDown (const juce::MouseEvent&) override;
    void showHoverFor (const juce::String& paramId);   // per i test e le istantanee

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        PhonoSlider slider;
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
    Knob kHeads, kWindow, kHardness;   // testina rotante
    PhonoToggle tRecord, tMonitor, tRestart, tExtended, tOctave, tMono, tSplice;
    std::unique_ptr<ButtonAtt> aRecord, aMonitor, aRestart, aExtended, aOctave, aMono, aSplice;
    juce::TextButton loadButton { "CHARGER" };
    juce::TextButton exportButton { "EXPORTER" };
    void exportRecording();

    // l'interfaccia è disegnata in coordinate logiche 1000×740 e scalata
    static constexpr float logicalW = 1000.0f, logicalH = 740.0f;
    float uiScale() const { return (float) getWidth() / logicalW; }
    void layoutLogical();
    void paintPanelLabels (juce::Graphics&);
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image texture;

    // comandi mappabili via CC: componente -> parametro
    std::vector<std::pair<juce::Component*, juce::String>> ccControls;
    void registerCC (juce::Component& c, const juce::String& paramId);
    void showCCMenu (juce::Component& c, const juce::String& paramId);
    juce::Component* componentForCCIndex (int index) const;
    int lastLearn = -1;

    // riquadro informativo al passaggio del mouse (italiano, valore a 2 decimali)
    juce::Component* hovered = nullptr;
    juce::String hoverKey, forcedHover;
    juce::Rectangle<float> hoverRect;           // coordinate logiche
    std::vector<std::pair<juce::Component*, juce::String>> hoverOnly;   // pulsanti senza CC
    void updateHover();
    juce::String paramIdFor (juce::Component*) const;
    juce::String valueText (const juce::String& paramId) const;
    void paintHover (juce::Graphics&);
    juce::Rectangle<int> toScreenArea (juce::Rectangle<float> logical) const;
};
