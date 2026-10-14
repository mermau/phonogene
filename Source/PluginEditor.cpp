#include "PluginEditor.h"

namespace
{
    using namespace juce;

    namespace Col
    {
        const Colour enamel     { 0xff7c8576 };  // smalto grigio-verde martellato
        const Colour enamelDark { 0xff4b5249 };
        const Colour deck       { 0xff3f453e };
        const Colour engrave    { 0xffe9e4d4 };
        const Colour engraveDim { 0xffb9b5a6 };
        const Colour bakelite   { 0xff141210 };
        const Colour tape       { 0xff5b3a22 };
        const Colour tapeLight  { 0xff8a6142 };
        const Colour chrome     { 0xffd5d8d6 };
        const Colour ivory      { 0xffeee5cd };
        const Colour lamp       { 0xffe2401f };
        const Colour amber      { 0xffe8b04a };
    }

    const char* noteNames[12] = { "DO", "DO#", "RÉ", "RÉ#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI" };

    Font labelFont (float size, bool bold = true)
    {
        return Font (FontOptions (size, bold ? Font::bold : Font::plain)).withExtraKerningFactor (0.12f);
    }

    // testo inciso: ombra scura sotto, riempimento chiaro sopra
    void engraved (Graphics& g, const String& text, Rectangle<float> r, float size,
                   Justification j = Justification::centred, Colour c = Col::engrave, bool bold = true)
    {
        g.setFont (labelFont (size, bold));
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawText (text, r.translated (0.0f, 1.0f), j, false);
        g.setColour (c);
        g.drawText (text, r, j, false);
    }

    void screw (Graphics& g, Point<float> c, float r, float angle)
    {
        ColourGradient grad (Col::chrome, c.x - r, c.y - r, Colour (0xff6d716e), c.x + r, c.y + r, false);
        g.setGradientFill (grad);
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c), 0.8f);
        Path slot;
        slot.addRectangle (-r * 0.8f, -r * 0.13f, r * 1.6f, r * 0.26f);
        slot.applyTransform (AffineTransform::rotation (angle).translated (c));
        g.setColour (Colour (0xff2b2d2b));
        g.fillPath (slot);
    }

    void chromeRoller (Graphics& g, Point<float> c, float r, float angle, float hubRatio = 0.35f)
    {
        auto b = Rectangle<float> (r * 2, r * 2).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (b.translated (2.0f, 3.0f));
        ColourGradient grad (Colour (0xfff2f3f1), c.x - r * 0.6f, c.y - r * 0.6f,
                             Colour (0xff7e837f), c.x + r, c.y + r, true);
        g.setGradientFill (grad);
        g.fillEllipse (b);
        g.setColour (Colour (0xff3a3d3b));
        g.drawEllipse (b, 1.0f);
        g.setColour (Colour (0xff4a4e4b));
        g.fillEllipse (Rectangle<float> (r * 2 * hubRatio, r * 2 * hubRatio).withCentre (c));
        // segno di rotazione
        g.setColour (Colour (0xff2a2c2b).withAlpha (0.7f));
        const auto p = c.getPointOnCircumference (r * 0.75f, angle);
        g.drawLine (Line<float> (c.getPointOnCircumference (r * hubRatio, angle), p), 1.6f);
    }
}

//==============================================================================
PhonoLookAndFeel::PhonoLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Col::engrave);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, Col::engrave);
}

void PhonoLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const bool selector = s.getProperties().contains ("selector");

    // tacche della scala, incise sul pannello
    const int ticks = selector ? (int) s.getMaximum() + 1 : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (ticks - 1);
        const float len = selector ? 0.14f : (i % 5 == 0 ? 0.16f : 0.09f);
        g.setColour (Col::engrave.withAlpha (0.85f));
        g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.86f, a),
                                 c.getPointOnCircumference (r * (0.86f + len), a)), selector ? 2.0f : 1.4f);
    }

    const float kr = r * 0.72f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // ombra
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (Rectangle<float> (kr * 2.1f, kr * 2.1f).withCentre (c.translated (2.5f, 4.0f)));

    if (selector)
    {
        // manopola "a testa di pollo", tipica dei selettori d'epoca
        Path chicken;
        chicken.startNewSubPath (0.0f, -kr * 1.05f);
        chicken.quadraticTo (kr * 0.38f, -kr * 0.2f, kr * 0.55f, kr * 0.55f);
        chicken.quadraticTo (0.0f, kr * 0.9f, -kr * 0.55f, kr * 0.55f);
        chicken.quadraticTo (-kr * 0.38f, -kr * 0.2f, 0.0f, -kr * 1.05f);
        chicken.closeSubPath();
        chicken.applyTransform (AffineTransform::rotation (angle).translated (c));
        ColourGradient grad (Colour (0xff3a3532), c.x - kr, c.y - kr, Col::bakelite, c.x + kr, c.y + kr, false);
        g.setGradientFill (grad);
        g.fillPath (chicken);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.strokePath (chicken, PathStrokeType (1.0f));
        g.setColour (Col::engrave);
        g.drawLine (Line<float> (c.getPointOnCircumference (kr * 0.2f, angle),
                                 c.getPointOnCircumference (kr * 0.9f, angle)), 2.2f);
        return;
    }

    // gonna zigrinata
    g.setColour (Col::bakelite);
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));
    for (int i = 0; i < 36; ++i)
    {
        const float a = (float) i / 36.0f * MathConstants<float>::twoPi + angle;
        g.setColour (Colours::white.withAlpha (0.06f));
        g.drawLine (Line<float> (c.getPointOnCircumference (kr * 0.86f, a), c.getPointOnCircumference (kr, a)), 1.0f);
    }

    // cupola in bachelite
    const float dr = kr * 0.78f;
    ColourGradient dome (Colour (0xff47413b), c.x - dr * 0.5f, c.y - dr * 0.7f, Col::bakelite, c.x + dr * 0.4f, c.y + dr, true);
    g.setGradientFill (dome);
    g.fillEllipse (Rectangle<float> (dr * 2, dr * 2).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.09f));
    g.fillEllipse (Rectangle<float> (dr * 0.9f, dr * 0.5f).withCentre (c.translated (-dr * 0.2f, -dr * 0.45f)));

    // indice
    g.setColour (Col::engrave);
    g.drawLine (Line<float> (c.getPointOnCircumference (dr * 0.25f, angle), c.getPointOnCircumference (kr * 0.97f, angle)), 2.4f);
}

void PhonoLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float sliderPos,
                                         float, float, Slider::SliderStyle, Slider&)
{
    // leva della coulisse: fessura nel pannello e impugnatura cromata
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto slot = Rectangle<float> (10.0f, b.getHeight() + 8.0f).withCentre (b.getCentre());
    g.setColour (Colour (0xff1a1c1a));
    g.fillRoundedRectangle (slot, 5.0f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (slot.translated (0.0f, 1.0f), 5.0f, 1.0f);

    // stelo
    const float cx = b.getCentreX();
    g.setColour (Colour (0xff8e928f));
    g.fillRect (Rectangle<float> (4.0f, 14.0f).withCentre ({ cx, sliderPos + 9.0f }));

    auto handle = Rectangle<float> (w * 0.78f, 22.0f).withCentre ({ cx, sliderPos });
    g.setColour (Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (handle.translated (2.0f, 4.0f), 4.0f);
    ColourGradient grad (Colour (0xfff4f5f3), handle.getX(), handle.getY(), Colour (0xff6f7471), handle.getX(), handle.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (handle, 4.0f);
    g.setColour (Colour (0xff3a3d3b));
    g.drawRoundedRectangle (handle, 4.0f, 1.0f);
    for (int i = -2; i <= 2; ++i)
    {
        g.setColour (Colour (0xff5d625f));
        g.drawHorizontalLine ((int) (handle.getCentreY() + i * 3.0f), handle.getX() + 6.0f, handle.getRight() - 6.0f);
    }
}

void PhonoLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto area = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const bool isRec = b.getComponentID() == "rec";

    auto top = area.withTrimmedBottom (18.0f);
    const auto c = top.getCentre().translated (0.0f, isRec ? 8.0f : 4.0f);

    if (isRec)
    {
        // spia rossa a gemma sopra il pulsante
        const auto lampC = Point<float> (c.x, top.getY() + 9.0f);
        const auto lb = Rectangle<float> (14.0f, 14.0f).withCentre (lampC);
        g.setColour (Colour (0xff2a2a28));
        g.fillEllipse (lb.expanded (3.0f));
        g.setColour (on ? Col::lamp : Col::lamp.darker (2.2f));
        g.fillEllipse (lb);
        if (on)
        {
            g.setColour (Col::lamp.withAlpha (0.25f));
            g.fillEllipse (lb.expanded (6.0f));
        }
        g.setColour (Colours::white.withAlpha (on ? 0.6f : 0.2f));
        g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre (lampC.translated (-2.5f, -2.5f)));

        // pulsante quadrato nero
        auto btn = Rectangle<float> (26.0f, 22.0f).withCentre (c.translated (0.0f, 10.0f));
        g.setColour (Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (btn.translated (1.5f, on ? 1.5f : 3.0f), 3.0f);
        g.setColour (on ? Col::bakelite.brighter (0.1f) : Col::bakelite.brighter (0.25f));
        g.fillRoundedRectangle (btn.translated (0.0f, on ? 1.5f : 0.0f), 3.0f);
    }
    else
    {
        // interruttore a levetta cromato su dado
        const auto nut = Rectangle<float> (20.0f, 20.0f).withCentre (c);
        ColourGradient ng (Col::chrome, nut.getX(), nut.getY(), Colour (0xff6d716e), nut.getRight(), nut.getBottom(), false);
        g.setGradientFill (ng);
        g.fillEllipse (nut);
        g.setColour (Colour (0xff3a3d3b));
        g.drawEllipse (nut, 1.0f);

        const auto tip = c.translated (0.0f, on ? -17.0f : 17.0f);
        g.setColour (Colours::black.withAlpha (0.3f));
        g.drawLine (Line<float> (c.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f)), 5.0f);
        g.setColour (Col::chrome);
        g.drawLine (Line<float> (c, tip), 5.0f);
        g.setColour (Colours::white);
        g.fillEllipse (Rectangle<float> (8.0f, 8.0f).withCentre (tip));
        g.setColour (Colour (0xff5c605d));
        g.drawEllipse (Rectangle<float> (8.0f, 8.0f).withCentre (tip), 0.8f);
    }

    engraved (g, b.getButtonText(), area.removeFromBottom (16.0f), 9.5f);
}

void PhonoLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (1.5f, down ? 1.5f : 3.0f), 4.0f);
    ColourGradient grad (Colour (0xff35312d), r.getX(), r.getY(), Col::bakelite, r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r.translated (0.0f, down ? 1.5f : 0.0f), 4.0f);
    g.setColour (Colours::white.withAlpha (highlighted ? 0.25f : 0.12f));
    g.drawRoundedRectangle (r.translated (0.0f, down ? 1.5f : 0.0f), 4.0f, 1.0f);
}

void PhonoLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool down)
{
    engraved (g, b.getButtonText(), b.getLocalBounds().toFloat().translated (0.0f, down ? 1.5f : 0.0f), 11.0f);
}

//==============================================================================
void DeckView::advance (float dt)
{
    const float v = proc.engine.display.speed.load();
    tapePhase += v * dt * 90.0f;          // pixel al secondo a velocità nominale
    if (tapePhase > 1.0e6f) tapePhase = 0.0f;
    capstanAngle += dt * 6.0f;            // il motore gira sempre
    if (capstanAngle > MathConstants<float>::twoPi) capstanAngle -= MathConstants<float>::twoPi;
}

void DeckView::paint (Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const auto& disp = proc.engine.display;
    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    const int activeKey = disp.activeKey.load();
    const float speed = disp.speed.load();

    // piastra del deck, incassata nel pannello
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (area, 6.0f);
    auto plate = area.reduced (3.0f);
    ColourGradient pg (Col::deck.brighter (0.12f), 0.0f, 0.0f, Col::deck.darker (0.25f), 0.0f, plate.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (plate, 5.0f);

    // zona comandi a destra (smalto più chiaro)
    auto ctrl = plate.removeFromRight (226.0f).reduced (8.0f);
    g.setColour (Col::enamel.withAlpha (0.35f));
    g.fillRoundedRectangle (ctrl, 4.0f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (ctrl, 4.0f, 1.0f);

    // targhetta in alluminio
    auto tag = Rectangle<float> (ctrl.getX() + 10.0f, ctrl.getY() + 10.0f, ctrl.getWidth() - 20.0f, 66.0f);
    ColourGradient tg (Colour (0xffd9dbd6), tag.getX(), tag.getY(), Colour (0xff9fa39f), tag.getRight(), tag.getBottom(), false);
    g.setGradientFill (tg);
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (Colour (0xff2a2b29));
    g.drawRoundedRectangle (tag, 3.0f, 1.0f);
    g.setFont (labelFont (20.0f));
    g.drawText (String::fromUTF8 ("PHONOGÈNE"), tag.withTrimmedBottom (28.0f).translated (0.0f, 4.0f), Justification::centred);
    g.setFont (labelFont (8.5f, false));
    const char* typeName[3] = { "à clavier  ·  type chromatique", "à coulisse  ·  vitesse continue", "universel  ·  tête rotative" };
    g.drawText (String::fromUTF8 (typeName[jlimit (0, 2, mode)]), tag.withTrimmedTop (34.0f).withTrimmedBottom (16.0f), Justification::centred);
    g.setFont (labelFont (7.5f));
    g.drawText ("COLLATERAL EFFECTS", tag.withTrimmedTop (50.0f), Justification::centred);
    for (auto p : { tag.getTopLeft().translated (6, 6), tag.getTopRight().translated (-6, 6),
                    tag.getBottomLeft().translated (6, -6), tag.getBottomRight().translated (-6, -6) })
        screw (g, p, 2.6f, 0.6f);

    // ---- meccanica del nastro ----
    const float runY = 206.0f;
    const Point<float> leftRoller { 58.0f, 168.0f }, rightRoller { 696.0f, 168.0f }, tension { 380.0f, 92.0f };
    const float guideR = 38.0f, tensionR = 13.0f;

    // posizione dei capstan: diametro proporzionale alla velocità del tasto
    float capX[12], capR[12];
    for (int k = 0; k < 12; ++k)
    {
        capX[k] = 168.0f + (float) k * 44.0f;
        capR[k] = 8.0f * std::pow (2.0f, (float) k / 12.0f);
    }

    // percorso del nastro ad anello
    Path tapePath;
    tapePath.startNewSubPath (leftRoller.x, runY);
    tapePath.lineTo (rightRoller.x, runY);
    tapePath.addCentredArc (rightRoller.x, rightRoller.y, guideR, guideR, 0.0f, MathConstants<float>::pi, 0.25f, false);
    tapePath.lineTo (tension.x + 10.0f, tension.y - tensionR);
    tapePath.addCentredArc (tension.x, tension.y, tensionR, tensionR, 0.0f, 0.75f, -0.75f, false);
    tapePath.lineTo (leftRoller.getPointOnCircumference (guideR, -0.25f));
    tapePath.addCentredArc (leftRoller.x, leftRoller.y, guideR, guideR, 0.0f, -0.25f, -MathConstants<float>::pi, false);
    tapePath.closeSubPath();

    // braccio tenditore
    g.setColour (Colour (0xff2d312c));
    g.drawLine (Line<float> (tension, { tension.x + 70.0f, tension.y + 52.0f }), 7.0f);
    screw (g, { tension.x + 70.0f, tension.y + 52.0f }, 5.0f, 0.3f);

    // rulli guida e tenditore
    chromeRoller (g, leftRoller, guideR - 3.0f, tapePhase / guideR, 0.28f);
    chromeRoller (g, rightRoller, guideR - 3.0f, tapePhase / guideR, 0.28f);
    chromeRoller (g, tension, tensionR - 3.0f, tapePhase / tensionR);

    // ombra e nastro
    g.setColour (Colours::black.withAlpha (0.35f));
    g.strokePath (tapePath, PathStrokeType (6.0f), AffineTransform::translation (2.0f, 3.0f));
    g.setColour (Col::tape);
    g.strokePath (tapePath, PathStrokeType (5.5f));

    // giunte e segni sul nastro che scorrono con la velocità reale
    const float len = tapePath.getLength();
    const float spacing = 26.0f;
    for (float d = std::fmod (tapePhase, spacing); d < len; d += spacing)
    {
        const auto p = tapePath.getPointAlongPath (d);
        g.setColour (Col::tapeLight.withAlpha (0.55f));
        g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (p));
    }
    // giunta (pezzo di nastro adesivo bianco), un riferimento visivo dell'anello
    {
        const auto p = tapePath.getPointAlongPath (std::fmod (tapePhase * 0.999f + 40.0f, len));
        g.setColour (Colour (0xffe9e5da));
        g.fillRect (Rectangle<float> (7.0f, 7.0f).withCentre (p));
    }

    // testina di lettura fissa
    {
        auto head = Rectangle<float> (36.0f, 34.0f).withCentre ({ 116.0f, runY - 20.0f });
        g.setColour (Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (head.translated (2.0f, 3.0f), 4.0f);
        ColourGradient hg (Col::chrome, head.getX(), head.getY(), Colour (0xff6e7370), head.getRight(), head.getBottom(), false);
        g.setGradientFill (hg);
        g.fillRoundedRectangle (head, 4.0f);
        g.setColour (Colour (0xff2b2d2b));
        g.drawRoundedRectangle (head, 4.0f, 1.0f);
        g.fillRect (Rectangle<float> (1.5f, 9.0f).withCentre ({ head.getCentreX(), head.getBottom() - 4.5f }));
        engraved (g, String::fromUTF8 ("TÊTE"), { head.getX() - 10.0f, head.getY() - 18.0f, head.getWidth() + 20.0f, 14.0f }, 8.5f,
                  Justification::centred, Col::engraveDim);
    }

    // capstan e rulli pressori
    for (int k = 0; k < 12; ++k)
    {
        const bool engaged = mode != 1 && k == activeKey && speed > 0.001f;
        const Point<float> cc { capX[k], runY - capR[k] - 3.0f };
        const float rr = 10.0f;
        const Point<float> rc { capX[k], runY + rr + (engaged ? 3.0f : 11.0f) };

        chromeRoller (g, cc, capR[k], capstanAngle * 1.0f + (float) k, 0.3f);

        // rullo pressore in gomma
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (rc.translated (2.0f, 3.0f)));
        g.setColour (engaged ? Colour (0xff121212) : Colour (0xff222222));
        g.fillEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (rc));
        g.setColour (Colour (0xff8c8f8d));
        g.fillEllipse (Rectangle<float> (5.0f, 5.0f).withCentre (rc));

        engraved (g, String::fromUTF8 (noteNames[k]), { capX[k] - 22.0f, runY + 36.0f, 44.0f, 14.0f }, 8.5f, Justification::centred,
                  engaged ? Col::amber : Col::engraveDim);
    }

    // velocità reale del nastro
    engraved (g, "VITESSE  " + String (speed, 3).replaceCharacter ('.', ','),
              { 26.0f, plate.getBottom() - 30.0f, 220.0f, 16.0f }, 10.0f, Justification::centredLeft, Col::engraveDim);
    if (mode == 2)
        engraved (g, String::fromUTF8 ("TÊTE ROTATIVE : PROCHAINE VERSION"),
                  { 300.0f, plate.getBottom() - 30.0f, 400.0f, 16.0f }, 10.0f, Justification::centredRight, Col::amber);

    for (auto p : { area.getTopLeft().translated (12, 12), area.getBottomLeft().translated (12, -12) })
        screw (g, p, 4.5f, 0.9f);
}

//==============================================================================
void TapeStrip::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r, 5.0f);
    auto win = r.reduced (4.0f);
    ColourGradient bg (Colour (0xff1d1712), win.getX(), win.getY(), Colour (0xff0d0a08), win.getX(), win.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (win, 3.0f);

    const int source = (int) proc.apvts.getRawParameterValue ("source")->load();
    const float start = proc.apvts.getRawParameterValue ("loopStart")->load();
    const float length = proc.apvts.getRawParameterValue ("loopLength")->load();
    const auto& disp = proc.engine.display;

    // dati della panoramica
    float peaks[512] {};
    int used = 512;
    bool empty = true;
    String caption;

    if (source == 0)
    {
        for (int i = 0; i < 512; ++i) { peaks[i] = proc.filePeaks[(size_t) i].load(); empty &= peaks[i] <= 0.0f; }
        caption = proc.getFileName().isEmpty() ? String (String::fromUTF8 ("AUCUNE BANDE  ·  GLISSER UN FICHIER ICI OU « CHARGER »"))
                                               : proc.getFileName().toUpperCase() + String::fromUTF8 ("  ·  ")
                                                     + String (proc.getFileSeconds(), 1).replaceCharacter ('.', ',') + " s";
    }
    else
    {
        const float secs = disp.recSeconds.load();
        used = jlimit (1, 512, (int) std::ceil (secs / (float) TapeEngine::maxRecordSeconds * 512.0f));
        for (int i = 0; i < used; ++i) { peaks[i] = disp.recPeaks[(size_t) i].load(); empty &= peaks[i] <= 0.0f; }
        caption = disp.recording.load() ? String::fromUTF8 ("ENREGISTREMENT…  ") + String (secs, 1).replaceCharacter ('.', ',') + " s"
                                        : (secs > 0.0f ? String::fromUTF8 ("BANDE ENREGISTRÉE  ·  ") + String (secs, 1).replaceCharacter ('.', ',') + " s"
                                                       : String (String::fromUTF8 ("ENTRÉE  ·  « ENREG. » POUR CAPTURER LE SIDECHAIN")));
    }

    auto wave = win.reduced (10.0f, 6.0f).withTrimmedTop (14.0f);

    // regione dell'anello
    const float x0 = wave.getX() + wave.getWidth() * start;
    const float x1 = jmin (wave.getRight(), x0 + wave.getWidth() * length);
    g.setColour (Col::tape.withAlpha (0.55f));
    g.fillRect (Rectangle<float> (x0, wave.getY(), x1 - x0, wave.getHeight()));

    if (! empty)
    {
        const float mid = wave.getCentreY();
        for (int px = 0; px < (int) wave.getWidth(); px += 2)
        {
            const int bin = jlimit (0, used - 1, (int) ((float) px / wave.getWidth() * (float) used));
            const float a = jmin (1.0f, peaks[bin]) * wave.getHeight() * 0.5f;
            const float x = wave.getX() + (float) px;
            const bool inside = x >= x0 && x <= x1;
            g.setColour ((inside ? Col::amber : Col::engraveDim).withAlpha (inside ? 0.9f : 0.3f));
            g.drawVerticalLine ((int) x, mid - a, mid + a + 1.0f);
        }

        // testina sull'anello
        const float ph = x0 + (x1 - x0) * jlimit (0.0f, 1.0f, disp.position.load());
        g.setColour (Col::lamp);
        g.drawLine (Line<float> (ph, wave.getY() - 2.0f, ph, wave.getBottom() + 2.0f), 1.6f);
    }

    g.setFont (labelFont (9.5f));
    g.setColour (Col::engraveDim);
    g.drawText (caption, win.reduced (10.0f, 4.0f).removeFromTop (12.0f), Justification::centredLeft);
}

//==============================================================================
Rectangle<float> OctaveKeyboard::keyBounds (int key) const
{
    auto r = getLocalBounds().toFloat().reduced (22.0f, 10.0f);
    const float ww = r.getWidth() / 7.0f;
    static const int whiteIndex[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    static const float blackPos[12] = { 0, 1, 0, 2, 0, 0, 4, 0, 5, 0, 6, 0 };

    if (! isBlack (key))
        return { r.getX() + ww * (float) whiteIndex[key], r.getY(), ww, r.getHeight() };

    const float bw = ww * 0.58f;
    return { r.getX() + ww * blackPos[key] - bw * 0.5f, r.getY(), bw, r.getHeight() * 0.6f };
}

int OctaveKeyboard::keyAt (Point<float> p) const
{
    for (int k = 0; k < 12; ++k)
        if (isBlack (k) && keyBounds (k).contains (p)) return k;
    for (int k = 0; k < 12; ++k)
        if (! isBlack (k) && keyBounds (k).contains (p)) return k;
    return -1;
}

void OctaveKeyboard::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // cassa in legno
    ColourGradient wood (Colour (0xff5a3a24), r.getX(), r.getY(), Colour (0xff2e1d12), r.getRight(), r.getBottom(), false);
    g.setGradientFill (wood);
    g.fillRoundedRectangle (r, 5.0f);
    Random rnd (7);
    for (int i = 0; i < 40; ++i)
    {
        g.setColour (Colours::black.withAlpha (0.06f + rnd.nextFloat() * 0.06f));
        const float y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.drawLine (r.getX(), y, r.getRight(), y + rnd.nextFloat() * 6.0f - 3.0f, 0.8f);
    }
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRect (r.reduced (20.0f, 8.0f));

    const int active = proc.engine.display.activeKey.load();
    auto pressed = [&] (int k) { return k == mouseKey || k == active; };

    for (int pass = 0; pass < 2; ++pass)
        for (int k = 0; k < 12; ++k)
        {
            if (isBlack (k) != (pass == 1)) continue;
            auto kb = keyBounds (k);
            const bool down = pressed (k);

            if (! isBlack (k))
            {
                kb = kb.reduced (1.0f, 0.0f);
                ColourGradient kg (down ? Col::ivory.darker (0.15f) : Col::ivory, kb.getX(), kb.getY(),
                                   down ? Col::ivory.darker (0.3f) : Col::ivory.darker (0.12f), kb.getX(), kb.getBottom(), false);
                g.setGradientFill (kg);
                g.fillRoundedRectangle (kb.withTrimmedBottom (down ? 0.0f : 3.0f), 3.0f);
                g.setColour (Colours::black.withAlpha (0.35f));
                g.drawRoundedRectangle (kb, 3.0f, 0.8f);
                g.setFont (labelFont (8.0f));
                g.setColour (down ? Col::lamp.darker (0.3f) : Colour (0xff7a725f));
                g.drawText (String::fromUTF8 (noteNames[k]), kb.removeFromBottom (22.0f), Justification::centred);
            }
            else
            {
                g.setColour (Colours::black.withAlpha (0.4f));
                g.fillRoundedRectangle (kb.translated (2.0f, 2.0f), 2.5f);
                ColourGradient kg (Colour (0xff3a3632), kb.getX(), kb.getY(), Colour (0xff0b0a09), kb.getX(), kb.getBottom(), false);
                g.setGradientFill (kg);
                g.fillRoundedRectangle (kb.withTrimmedBottom (down ? 0.0f : 3.0f), 2.5f);
                if (down)
                {
                    g.setColour (Col::lamp.withAlpha (0.5f));
                    g.fillRect (kb.removeFromBottom (4.0f).reduced (3.0f, 0.0f));
                }
            }
        }
}

void OctaveKeyboard::mouseDown (const MouseEvent& e)
{
    mouseKey = keyAt (e.position);
    if (mouseKey >= 0)
        proc.keyboardState.noteOn (1, 60 + mouseKey, 1.0f);
    repaint();
}

void OctaveKeyboard::mouseDrag (const MouseEvent& e)
{
    const int k = keyAt (e.position);
    if (k == mouseKey) return;
    if (mouseKey >= 0) proc.keyboardState.noteOff (1, 60 + mouseKey, 0.0f);
    mouseKey = k;
    if (mouseKey >= 0) proc.keyboardState.noteOn (1, 60 + mouseKey, 1.0f);
    repaint();
}

void OctaveKeyboard::mouseUp (const MouseEvent&)
{
    if (mouseKey >= 0) proc.keyboardState.noteOff (1, 60 + mouseKey, 0.0f);
    mouseKey = -1;
    repaint();
}

//==============================================================================
PhonogeneEditor::PhonogeneEditor (PhonogeneProcessor& p)
    : AudioProcessorEditor (&p), proc (p), deck (p), strip (p), keyboard (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (deck);
    addAndMakeVisible (strip);
    addAndMakeVisible (keyboard);

    setupKnob (kStart,   "loopStart",  String::fromUTF8 ("DÉBUT"));
    setupKnob (kLength,  "loopLength", "LONGUEUR");
    setupKnob (kInertia, "inertia",    "INERTIE");
    setupKnob (kLevel,   "level",      "NIVEAU");
    setupKnob (kWow,     "wow",        "PLEURAGE");
    setupKnob (kHiss,    "hiss",       "SOUFFLE");
    setupKnob (kDrive,   "drive",      "SATURATION");
    setupKnob (kMode,    "mode",       "MODE");
    setupKnob (kSource,  "source",     "SOURCE");
    setupKnob (kCoulisse, "coulisse",  "COULISSE", Slider::LinearVertical);

    for (auto* s : { &kMode.slider, &kSource.slider })
    {
        s->getProperties().set ("selector", true);
        s->setRotaryParameters (MathConstants<float>::pi * 1.72f, MathConstants<float>::pi * 2.28f, true);
    }
    kSource.slider.setRotaryParameters (MathConstants<float>::pi * 1.8f, MathConstants<float>::pi * 2.2f, true);

    setupToggle (tRecord,   aRecord,   "record",   "ENREG.");
    setupToggle (tMonitor,  aMonitor,  "monitor",  String::fromUTF8 ("ÉCOUTE"));
    setupToggle (tRestart,  aRestart,  "restart",  "RETOUR");
    setupToggle (tExtended, aExtended, "extended", String::fromUTF8 ("ÉTENDU"));
    tRecord.setComponentID ("rec");

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { openFileChooser(); };

    setSize (1000, 660);
    startTimerHz (30);
}

PhonogeneEditor::~PhonogeneEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PhonogeneEditor::setupKnob (Knob& k, const String& paramId, const String& label, Slider::SliderStyle style)
{
    k.label = label;
    k.slider.setSliderStyle (style);
    k.slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    k.slider.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<SliderAtt> (proc.apvts, paramId, k.slider);
}

void PhonogeneEditor::setupToggle (ToggleButton& b, std::unique_ptr<ButtonAtt>& att, const String& paramId, const String& label)
{
    b.setButtonText (label);
    addAndMakeVisible (b);
    att = std::make_unique<ButtonAtt> (proc.apvts, paramId, b);
}

void PhonogeneEditor::buildTexture()
{
    texture = Image (Image::RGB, getWidth(), getHeight(), true);
    Graphics g (texture);
    g.fillAll (Col::enamel);

    // smalto martellato: tante piccole ammaccature chiare e scure
    Random rnd (1953);
    for (int i = 0; i < 2600; ++i)
    {
        const float x = rnd.nextFloat() * (float) getWidth();
        const float y = rnd.nextFloat() * (float) getHeight();
        const float r = 3.0f + rnd.nextFloat() * 7.0f;
        const bool light = rnd.nextBool();
        g.setColour ((light ? Colours::white : Colours::black).withAlpha (0.025f + rnd.nextFloat() * 0.03f));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
    }
    ColourGradient vignette (Colours::transparentBlack, getWidth() * 0.5f, getHeight() * 0.4f,
                             Colours::black.withAlpha (0.28f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();
}

void PhonogeneEditor::paint (Graphics& g)
{
    if (texture.getWidth() != getWidth() || texture.getHeight() != getHeight())
        buildTexture();

    g.drawImageAt (texture, 0, 0);

    auto r = getLocalBounds().toFloat();
    g.setColour (Colours::white.withAlpha (0.15f));
    g.drawRect (r.reduced (1.0f), 1.0f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawRect (r, 1.0f);

    for (auto p : { Point<float> (12, 12), Point<float> (r.getRight() - 12, 12),
                    Point<float> (12, r.getBottom() - 12), Point<float> (r.getRight() - 12, r.getBottom() - 12) })
        screw (g, p, 5.0f, 0.4f);

    // incasso della sezione inferiore
    g.setColour (Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (Rectangle<float> (440.0f, 420.0f, 536.0f, 218.0f), 5.0f);
}

void PhonogeneEditor::paintOverChildren (Graphics& g)
{
    auto labelUnder = [&] (const Knob& k, float size = 9.5f)
    {
        const auto b = k.slider.getBounds().toFloat();
        engraved (g, k.label, { b.getX() - 10.0f, b.getBottom() - 2.0f, b.getWidth() + 20.0f, 14.0f }, size);
    };

    for (auto* k : { &kStart, &kLength, &kInertia, &kLevel, &kWow, &kHiss, &kDrive })
        labelUnder (*k);

    // selettori: etichette delle posizioni
    auto positions = [&] (const Knob& k, const StringArray& names)
    {
        const auto b = k.slider.getBounds().toFloat();
        const auto c = b.getCentre();
        const auto rp = k.slider.getRotaryParameters();
        const float rad = jmin (b.getWidth(), b.getHeight()) * 0.5f + 10.0f;
        for (int i = 0; i < names.size(); ++i)
        {
            const float a = rp.startAngleRadians + (rp.endAngleRadians - rp.startAngleRadians) * (float) i / (float) (names.size() - 1);
            const auto pt = c.getPointOnCircumference (rad, a);
            engraved (g, names[i], Rectangle<float> (70.0f, 12.0f).withCentre (pt.translated (0.0f, -4.0f)), 7.5f);
        }
        engraved (g, k.label, { b.getX() - 10.0f, b.getBottom() - 4.0f, b.getWidth() + 20.0f, 14.0f }, 9.5f);
    };
    positions (kMode, { "CHROM.", "COULISSE", "UNIVERSEL" });
    positions (kSource, { "FICHIER", String::fromUTF8 ("ENTRÉE") });

    // scala della leva della coulisse
    {
        const auto b = kCoulisse.slider.getBounds().toFloat();
        const auto& range = kCoulisse.slider;
        const std::pair<double, const char*> marks[] = { { 2.0, "2" }, { 1.0, "1" }, { 0.5, "\xc2\xbd" }, { 0.02, "0" } };
        for (auto [v, txt] : marks)
        {
            const float y = (float) range.getPositionOfValue (v) + b.getY();
            g.setColour (Col::engrave.withAlpha (0.8f));
            g.drawHorizontalLine ((int) y, b.getX() - 4.0f, b.getX() + 8.0f);
            engraved (g, String::fromUTF8 (txt), { b.getX() - 26.0f, y - 7.0f, 20.0f, 14.0f }, 9.0f, Justification::centredRight);
        }
        engraved (g, kCoulisse.label, { b.getX() - 14.0f, b.getBottom() + 4.0f, b.getWidth() + 28.0f, 14.0f }, 9.5f);
    }
}

void PhonogeneEditor::resized()
{
    deck.setBounds (24, 24, 952, 300);
    strip.setBounds (24, 336, 826, 70);
    loadButton.setBounds (862, 342, 114, 56);

    keyboard.setBounds (24, 420, 404, 218);

    // leva
    kCoulisse.slider.setBounds (480, 438, 44, 170);

    // manopole: due file da quattro
    const int x0 = 548, cw = 106, kh = 86;
    kStart.slider.setBounds  (x0 + cw * 0, 428, cw, kh);
    kLength.slider.setBounds (x0 + cw * 1, 428, cw, kh);
    kInertia.slider.setBounds(x0 + cw * 2, 428, cw, kh);
    kLevel.slider.setBounds  (x0 + cw * 3, 428, cw, kh);
    kWow.slider.setBounds    (x0 + cw * 0, 532, cw, kh);
    kHiss.slider.setBounds   (x0 + cw * 1, 532, cw, kh);
    kDrive.slider.setBounds  (x0 + cw * 2, 532, cw, kh);
    tRestart.setBounds       (x0 + cw * 3 + 2,  540, 50, 76);
    tExtended.setBounds      (x0 + cw * 3 + 54, 540, 50, 76);

    // zona comandi del deck
    const int dx = 24 + 952 - 226;
    kMode.slider.setBounds   (dx + 22,  132, 92, 80);
    kSource.slider.setBounds (dx + 122, 132, 92, 80);
    tRecord.setBounds        (dx + 40,  228, 60, 84);
    tMonitor.setBounds       (dx + 128, 232, 56, 76);
}

void PhonogeneEditor::timerCallback()
{
    deck.advance (1.0f / 30.0f);
    deck.repaint();
    strip.repaint();
    keyboard.repaint();
}

void PhonogeneEditor::openFileChooser()
{
    chooser = std::make_unique<FileChooser> ("Choisir une bande", File(), proc.formatManager.getWildcardForAllFormats());
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile() && proc.loadFile (f))
                                  if (auto* src = proc.apvts.getParameter ("source"))
                                      src->setValueNotifyingHost (0.0f);
                          });
}

void PhonogeneEditor::filesDropped (const StringArray& files, int, int)
{
    for (const auto& path : files)
        if (proc.loadFile (File (path)))
        {
            if (auto* src = proc.apvts.getParameter ("source"))
                src->setValueNotifyingHost (0.0f);
            break;
        }
}
