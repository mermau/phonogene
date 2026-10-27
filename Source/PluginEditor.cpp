#include "PluginEditor.h"
#include <map>

namespace
{
    using namespace juce;

    namespace Col
    {
        const Colour enamel     { 0xffcdcbc2 };  // smalto grigio chiaro della macchina
        const Colour enamelDark { 0xffa9a79e };
        const Colour plate      { 0xffdad8cf };
        const Colour well       { 0xffc9c7be };
        const Colour engrave    { 0xff262624 };  // serigrafia nera
        const Colour engraveDim { 0xff5c5b55 };
        const Colour stripText  { 0xffb9b5a6 };
        const Colour cream      { 0xffeae6da };
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

    // serigrafia: inchiostro scuro con un leggero riflesso chiaro sotto
    void engraved (Graphics& g, const String& text, Rectangle<float> r, float size,
                   Justification j = Justification::centred, Colour c = Col::engrave, bool bold = true)
    {
        g.setFont (labelFont (size, bold));
        g.setColour (Colours::white.withAlpha (0.35f));
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

    // ombra (solo per le manopole rotonde; il selettore proietta la propria forma)
    if (! selector)
    {
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (Rectangle<float> (kr * 2.05f, kr * 2.05f).withCentre (c.translated (2.5f, 4.0f)));
    }

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
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillPath (chicken, AffineTransform::translation (2.5f, 3.5f));
        ColourGradient grad (Colour (0xff3a3532), c.x - kr, c.y - kr, Col::bakelite, c.x + kr, c.y + kr, false);
        g.setGradientFill (grad);
        g.fillPath (chicken);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.strokePath (chicken, PathStrokeType (1.0f));
        g.setColour (Col::cream);
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
    g.setColour (Col::cream);
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
        // la spia (con alone) deve stare tutta dentro il componente
        const auto lampC = Point<float> (c.x, top.getY() + 14.0f);
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
        // interruttore a levetta in acciaio: rondella, dado esagonale, boccola filettata, leva conica
        const float pi = MathConstants<float>::pi;

        // segni serigrafati I / O
        engraved (g, "I", Rectangle<float> (12.0f, 10.0f).withCentre (c.translated (0.0f, -29.0f)), 8.0f);
        engraved (g, "O", Rectangle<float> (12.0f, 10.0f).withCentre (c.translated (0.0f, 29.0f)), 8.0f);

        // rondella dentata
        {
            const float rw = 13.5f;
            ColourGradient wg (Colour (0xffe9eae6), c.x - rw, c.y - rw, Colour (0xff8d908b), c.x + rw, c.y + rw, false);
            g.setColour (Colours::black.withAlpha (0.22f));
            g.fillEllipse (Rectangle<float> (2 * rw, 2 * rw).withCentre (c.translated (1.5f, 2.0f)));
            g.setGradientFill (wg);
            g.fillEllipse (Rectangle<float> (2 * rw, 2 * rw).withCentre (c));
            g.setColour (Colour (0xff6b6e69));
            g.drawEllipse (Rectangle<float> (2 * rw, 2 * rw).withCentre (c), 0.7f);
        }

        // dado esagonale con facce illuminate dall'alto a sinistra
        {
            const float rn = 10.5f;
            Point<float> v[6];
            for (int i = 0; i < 6; ++i)
                v[i] = c.getPointOnCircumference (rn, pi / 6.0f + (float) i * pi / 3.0f);
            const Point<float> light { -0.6f, -0.8f };
            for (int i = 0; i < 6; ++i)
            {
                const auto a = v[i], b2 = v[(i + 1) % 6];
                const auto mid = (a + b2) * 0.5f - c;
                const float len = std::sqrt (mid.x * mid.x + mid.y * mid.y);
                const float lit = jlimit (0.0f, 1.0f, 0.5f + 0.5f * (mid.x * light.x + mid.y * light.y) / len);
                Path tri;
                tri.addTriangle (c, a, b2);
                g.setColour (Colour (0xff6f726d).interpolatedWith (Colour (0xfff4f5f2), lit));
                g.fillPath (tri);
            }
            Path hex;
            hex.startNewSubPath (v[0]);
            for (int i = 1; i < 6; ++i) hex.lineTo (v[i]);
            hex.closeSubPath();
            g.setColour (Colour (0xff4c4f4a));
            g.strokePath (hex, PathStrokeType (0.8f));
            // smusso interno del dado
            g.setColour (Colour (0xffc9cbc6));
            g.fillEllipse (Rectangle<float> (15.0f, 15.0f).withCentre (c));
        }

        // boccola filettata
        {
            ColourGradient bg (Colour (0xffdfe0dc), c.x - 6, c.y - 6, Colour (0xff5d605b), c.x + 6, c.y + 6, false);
            g.setGradientFill (bg);
            g.fillEllipse (Rectangle<float> (12.0f, 12.0f).withCentre (c));
            g.setColour (Colour (0xff545752));
            for (float rr : { 6.0f, 4.6f })
                g.drawEllipse (Rectangle<float> (2 * rr, 2 * rr).withCentre (c), 0.6f);
            g.setColour (Colour (0xff1e1f1d));
            g.fillEllipse (Rectangle<float> (5.0f, 5.0f).withCentre (c));
        }

        // leva conica cromata con estremità arrotondata
        {
            // la leva esce quasi perpendicolare al pannello e scatta di ~20° verso I o O:
            // vista di fronte appare corta (scorcio) con la punta vicina all'osservatore
            const float dir = on ? -1.0f : 1.0f;
            const float L = 10.0f;
            const auto tip = c.translated (0.0f, dir * L);

            auto lever = [&] (Point<float> base, Point<float> end, float wb, float wt)
            {
                Path p;
                p.startNewSubPath (base.translated (-wb, 0));
                p.lineTo (end.translated (-wt, 0));
                p.lineTo (end.translated (wt, 0));
                p.lineTo (base.translated (wb, 0));
                p.closeSubPath();
                return p;
            };

            // ombra
            g.setColour (Colours::black.withAlpha (0.28f));
            g.fillPath (lever (c.translated (2.5f, 3.0f), tip.translated (6.5f, 7.0f), 3.0f, 3.2f));
            g.fillEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (tip.translated (6.5f, 7.0f)));

            const auto body = lever (c, tip, 3.0f, 3.4f);
            ColourGradient lg (Colour (0xff6a6d68), c.x - 3.0f, 0, Colour (0xff767974), c.x + 3.0f, 0, false);
            lg.addColour (0.35, Colour (0xfff7f8f5));
            lg.addColour (0.55, Colour (0xffc4c6c1));
            g.setGradientFill (lg);
            g.fillPath (body);
            g.setColour (Colour (0xff4e514c));
            g.strokePath (body, PathStrokeType (0.5f));

            // estremità a oliva
            auto ball = Rectangle<float> (9.5f, 9.0f).withCentre (tip);
            ColourGradient tg (Colours::white, ball.getX() + 1.5f, ball.getY() + 1.5f, Colour (0xff70736e), ball.getRight(), ball.getBottom(), true);
            g.setGradientFill (tg);
            g.fillEllipse (ball);
            g.setColour (Colour (0xff4e514c));
            g.drawEllipse (ball, 0.6f);
            g.setColour (Colours::white.withAlpha (0.9f));
            g.fillEllipse (Rectangle<float> (2.6f, 2.6f).withCentre (tip.translated (-1.6f, -1.8f)));
        }
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
    g.setFont (labelFont (11.0f));
    g.setColour (Col::cream);
    g.drawText (b.getButtonText(), b.getLocalBounds().toFloat().translated (0.0f, down ? 1.5f : 0.0f), Justification::centred);
}

//==============================================================================
void DeckView::advance (float dt)
{
    const float v = proc.engine.display.speed.load();
    tapePhase += v * dt * 90.0f;          // pixel al secondo a velocità nominale
    if (tapePhase > 1.0e6f) tapePhase = 0.0f;
    // il motore gira sempre; nella coulisse la sua velocità segue la leva
    capstanAngle += dt * 6.0f * proc.engine.display.motorSpeed.load();
    if (capstanAngle > MathConstants<float>::twoPi) capstanAngle -= MathConstants<float>::twoPi;
}

namespace
{
    // disco metallico visto dall'alto (capstan, perni, rulli)
    void metalTop (Graphics& g, Point<float> c, float r, float rot = 0.0f, bool mark = true)
    {
        auto b = Rectangle<float> (2 * r, 2 * r).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.26f));
        g.fillEllipse (b.translated (r * 0.15f + 1.5f, r * 0.2f + 2.0f));
        ColourGradient gr (Colour (0xfffbfbf8), c.x - r * 0.5f, c.y - r * 0.6f, Colour (0xff868884), c.x + r, c.y + r, true);
        g.setGradientFill (gr);
        g.fillEllipse (b);
        g.setColour (Colour (0xff666864));
        g.drawEllipse (b, 0.9f);
        g.setColour (Colour (0xffa2a49f));
        g.drawEllipse (b.reduced (r * 0.33f), 0.8f);
        g.setColour (Colour (0xff595b57));
        g.fillEllipse (Rectangle<float> (r * 0.36f, r * 0.36f).withCentre (c));
        if (mark)
        {
            g.setColour (Colour (0xff4f514d).withAlpha (0.7f));
            g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.3f, rot), c.getPointOnCircumference (r * 0.62f, rot)), 1.3f);
        }
    }

    void rubberTop (Graphics& g, Point<float> c, float r, float rot)
    {
        auto b = Rectangle<float> (2 * r, 2 * r).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillEllipse (b.translated (2.0f, 2.5f));
        ColourGradient gr (Colour (0xff3d3d3b), c.x - r * 0.4f, c.y - r * 0.5f, Colour (0xff111110), c.x + r, c.y + r, true);
        g.setGradientFill (gr);
        g.fillEllipse (b);
        metalTop (g, c, r * 0.42f, rot, false);
    }

    // Geometria dell'anello: 12 stazioni (capstan + rullo pressore) nel pozzetto,
    // testina e perno tenditore nel varco in alto.
    struct Ring
    {
        Point<float> c;
        float R, capR[12], ang[12];
        Point<float> cap[12], pinch[12], head, guideL, guideR, inner, tension;
        float pinchR = 10.0f, tensionR = 12.0f;
        Path tape;

        Ring (Point<float> centre, float radius, int active) : c (centre), R (radius)
        {
            for (int k = 0; k < 12; ++k)
            {
                ang[k] = degreesToRadians (34.0f + 292.0f * (float) k / 11.0f);
                capR[k] = 7.0f * std::pow (2.0f, (float) k / 12.0f);
                cap[k] = c.getPointOnCircumference (R, ang[k]);
                pinch[k] = c.getPointOnCircumference (R + capR[k] + 3.0f + pinchR + (k == active ? 1.0f : 8.0f), ang[k]);
            }
            guideL  = c.getPointOnCircumference (R + 6.0f, degreesToRadians (341.0f));
            head    = c.getPointOnCircumference (R + 1.0f, degreesToRadians (352.0f));
            guideR  = c.getPointOnCircumference (R + 6.0f, degreesToRadians (19.0f));
            inner   = c.getPointOnCircumference (R * 0.6f, degreesToRadians (9.0f));
            tension = c.translated (0.0f, -R * 0.12f);

            // il nastro passa all'esterno dei capstan, contro i rulli pressori
            Path poly;
            poly.startNewSubPath (c.getPointOnCircumference (R + capR[0] + 1.5f, ang[0]));
            for (int k = 1; k < 12; ++k)
                poly.lineTo (c.getPointOnCircumference (R + capR[k] + 1.5f, ang[k]));
            poly.lineTo (guideL.getPointOnCircumference (5.0f, degreesToRadians (341.0f)));
            poly.lineTo (head.getPointOnCircumference (2.0f, degreesToRadians (352.0f)));
            poly.lineTo (tension.translated (-tensionR - 1.5f, -4.0f));
            poly.lineTo (tension.translated (-tensionR * 0.6f, tensionR + 1.0f));
            poly.lineTo (tension.translated (tensionR * 0.6f, tensionR + 1.0f));
            poly.lineTo (tension.translated (tensionR + 1.5f, -4.0f));
            poly.lineTo (inner.translated (5.0f, 0.0f));
            poly.lineTo (guideR.getPointOnCircumference (5.0f, degreesToRadians (19.0f)));
            poly.closeSubPath();

            // nastro morbido: curve dolci attorno a perni e capstan, niente angoli vivi
            tape = poly.createPathWithRoundedCorners (22.0f);
        }
    };
}

namespace
{
    const Point<float> wellCentre { 212.0f, 190.0f };
    constexpr float lampX0 = 432.0f, lampX1 = 700.0f;

    Point<float> lampCentre (int k) { return { lampX0 + 10.0f + (float) k * ((lampX1 - lampX0 - 20.0f) / 11.0f), 76.0f }; }

    void drawLamp (Graphics& g, Point<float> lc, bool on)
    {
        g.setColour (Colour (0xff2a2a28));
        g.fillEllipse (Rectangle<float> (15.0f, 15.0f).withCentre (lc));
        if (on) { g.setColour (Col::amber.withAlpha (0.3f)); g.fillEllipse (Rectangle<float> (22.0f, 22.0f).withCentre (lc)); }
        g.setColour (on ? Col::amber : Col::amber.darker (2.0f));
        g.fillEllipse (Rectangle<float> (10.0f, 10.0f).withCentre (lc));
        g.setColour (Colours::white.withAlpha (on ? 0.6f : 0.2f));
        g.fillEllipse (Rectangle<float> (3.5f, 3.5f).withCentre (lc.translated (-1.8f, -1.8f)));
    }

    struct Tacho
    {
        Rectangle<float> face;
        Point<float> pivot;
        float rad = 140.0f, maxV = 2.0f;
        explicit Tacho (float scaleMax) : maxV (scaleMax)
        {
            face = Rectangle<float> (lampX0, 112.0f, lampX1 - lampX0, 128.0f).reduced (6.0f);
            pivot = { face.getCentreX(), face.getBottom() + 30.0f };
        }
        float angleFor (float v) const { return -0.8f + 1.6f * jlimit (0.0f, 1.0f, v / maxV); }
    };
}

// Parti fisse del deck: disegnate una volta in un'immagine e riusate a ogni fotogramma.
void DeckView::paintStatic (Graphics& g, int mode, int activeKey, float scaleMax)
{
    const auto area = getLocalBounds().toFloat();

    // piano superiore del cabinet (smalto grigio chiaro)
    g.setColour (Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (area, 6.0f);
    auto plate = area.reduced (2.0f);
    ColourGradient pg (Col::plate.brighter (0.04f), 0.0f, 0.0f, Col::plate.darker (0.08f), 0.0f, plate.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (Colours::white.withAlpha (0.55f));
    g.drawLine (plate.getX() + 6, plate.getY() + 0.5f, plate.getRight() - 6, plate.getY() + 0.5f, 1.0f);

    // zona comandi a destra, con la targhetta in acciaio
    auto ctrl = plate.removeFromRight (226.0f).reduced (8.0f);
    g.setColour (Colours::black.withAlpha (0.06f));
    g.fillRoundedRectangle (ctrl, 4.0f);
    g.setColour (Colours::black.withAlpha (0.18f));
    g.drawRoundedRectangle (ctrl, 4.0f, 1.0f);

    auto tag = Rectangle<float> (ctrl.getX() + 10.0f, ctrl.getY() + 10.0f, ctrl.getWidth() - 20.0f, 66.0f);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (tag.translated (1.5f, 2.0f), 3.0f);
    ColourGradient tg (Colour (0xffe2e4df), tag.getX(), tag.getY(), Colour (0xff9da19d), tag.getRight(), tag.getBottom(), false);
    g.setGradientFill (tg);
    g.fillRoundedRectangle (tag, 3.0f);
    for (float y = tag.getY() + 2.0f; y < tag.getBottom() - 2.0f; y += 2.0f)   // spazzolatura dell'acciaio
    {
        g.setColour (Colours::white.withAlpha (0.08f));
        g.drawHorizontalLine ((int) y, tag.getX() + 2.0f, tag.getRight() - 2.0f);
    }
    g.setColour (Colour (0xff2a2b29));
    g.drawRoundedRectangle (tag, 3.0f, 1.0f);
    g.setFont (labelFont (20.0f));
    g.drawText (String::fromUTF8 ("PHONOGÈNE"), tag.withTrimmedBottom (28.0f).translated (0.0f, 4.0f), Justification::centred);
    g.setFont (labelFont (8.5f, false));
    const char* typeName[3] = { "à clavier  ·  type chromatique", "à coulisse  ·  vitesse continue", "universel  ·  tête rotative" };
    g.drawText (String::fromUTF8 (typeName[jlimit (0, 2, mode)]), tag.withTrimmedTop (34.0f).withTrimmedBottom (16.0f), Justification::centred);
    g.setFont (labelFont (7.5f));
    g.drawText (String::fromUTF8 ("COLLATERAL EFFECTS  ·  v") + JucePlugin_VersionString, tag.withTrimmedTop (50.0f), Justification::centred);
    for (auto p : { tag.getTopLeft().translated (6, 6), tag.getTopRight().translated (-6, 6),
                    tag.getBottomLeft().translated (6, -6), tag.getBottomRight().translated (-6, -6) })
        screw (g, p, 2.6f, 0.6f);

    // ---- pozzetto circolare ----
    const auto wc = wellCentre;
    const float wellR = 174.0f;
    {
        auto wb = Rectangle<float> (2 * wellR, 2 * wellR).withCentre (wc);
        g.setColour (Colour (0xff2b2b29));
        g.fillEllipse (wb.expanded (4.0f));
        ColourGradient wg (Col::well.darker (0.2f), wc.x, wc.y - wellR, Col::well.brighter (0.04f), wc.x, wc.y + wellR * 0.3f, false);
        g.setGradientFill (wg);
        g.fillEllipse (wb);
        ColourGradient inner (Colours::transparentBlack, wc.x, wc.y, Colours::black.withAlpha (0.16f), wc.x + wellR, wc.y, true);
        g.setGradientFill (inner);
        g.fillEllipse (wb);
    }

    Ring ring (wc, 130.0f, activeKey);

    // braccio del perno tenditore
    g.setColour (Colour (0xff6d6f6b));
    g.drawLine (Line<float> (wc, ring.tension), 5.0f);
    metalTop (g, wc, 9.0f, 0.0f, false);

    // nastro con ombra
    g.setColour (Colours::black.withAlpha (0.28f));
    g.strokePath (ring.tape, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (1.5f, 2.5f));
    g.setColour (Col::tape);
    g.strokePath (ring.tape, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // corpi di capstan, rulli pressori e guide (i segni di rotazione sono dinamici)
    for (int k = 0; k < 12; ++k)
    {
        metalTop (g, ring.cap[k], ring.capR[k], 0.0f, false);
        rubberTop (g, ring.pinch[k], ring.pinchR, 0.0f);
        if (k == activeKey)
        {
            g.setColour (Col::amber.withAlpha (0.85f));
            g.drawEllipse (Rectangle<float> (2 * ring.pinchR + 6, 2 * ring.pinchR + 6).withCentre (ring.pinch[k]), 1.5f);
        }
    }
    metalTop (g, ring.guideL, 5.0f, 0.0f, false);
    metalTop (g, ring.guideR, 5.0f, 0.0f, false);
    metalTop (g, ring.inner, 5.0f, 0.0f, false);
    metalTop (g, ring.tension, ring.tensionR - 2.0f, 0.0f, false);

    if (mode != 2)
    {
        // testina di lettura con schermo
        Graphics::ScopedSaveState s (g);
        g.addTransform (AffineTransform::rotation (degreesToRadians (352.0f), ring.head.x, ring.head.y));
        auto hb = Rectangle<float> (22.0f, 15.0f).withCentre (ring.head.translated (0.0f, -10.0f));
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (hb.translated (2, 3), 2.0f);
        ColourGradient hg (Colour (0xfff2f2ef), hb.getX(), hb.getY(), Colour (0xff6e706c), hb.getRight(), hb.getBottom(), false);
        g.setGradientFill (hg);
        g.fillRoundedRectangle (hb, 2.0f);
        g.setColour (Colour (0xff2b2b29));
        g.drawRoundedRectangle (hb, 2.0f, 0.8f);
        g.fillRect (Rectangle<float> (1.2f, 4.0f).withCentre ({ hb.getCentreX(), hb.getBottom() - 2.0f }));
    }

    // ---- zona centrale: spie dei capstan (spente) ----
    engraved (g, mode == 1 ? String::fromUTF8 ("CABESTANS  ·  HORS SERVICE EN MODE COULISSE")
                           : (mode == 2 ? String::fromUTF8 ("HAUTEUR  ·  TAMBOUR ROTATIF") : String ("CABESTANS")),
              { lampX0, 40.0f, lampX1 - lampX0, 12.0f }, 8.5f, Justification::centredLeft, Col::engraveDim);
    for (int k = 0; k < 12; ++k)
    {
        const auto lc = lampCentre (k);
        drawLamp (g, lc, false);
        engraved (g, String::fromUTF8 (noteNames[k]), Rectangle<float> (30.0f, 11.0f).withCentre (lc.translated (0.0f, 17.0f)), 7.0f);
    }

    // quadrante del tachimetro (scala 0–2, o 0–4 con OCTAVE)
    {
        const Tacho t (scaleMax);
        g.setColour (Colour (0xff1d1d1c));
        g.fillRoundedRectangle (t.face.expanded (6.0f), 3.0f);
        ColourGradient fg (Colour (0xfff8f3e2), t.face.getX(), t.face.getY(), Colour (0xffe2dac2), t.face.getX(), t.face.getBottom(), false);
        g.setGradientFill (fg);
        g.fillRect (t.face);
        for (int i = 0; i <= 20; ++i)
        {
            const float a = t.angleFor ((float) i * scaleMax / 20.0f);
            g.setColour (Col::engrave);
            g.drawLine (Line<float> (t.pivot.getPointOnCircumference (t.rad * 0.78f, a),
                                     t.pivot.getPointOnCircumference (t.rad * (i % 5 == 0 ? 0.88f : 0.83f), a)), i % 5 == 0 ? 1.4f : 0.9f);
        }
        if (scaleMax > 2.5f)
        {
            for (int v = 0; v <= 4; ++v)
                engraved (g, String (v), Rectangle<float> (24.0f, 12.0f).withCentre (t.pivot.getPointOnCircumference (t.rad * 0.96f, t.angleFor ((float) v))), 8.5f);
        }
        else
        {
            const std::pair<float, const char*> marks[] = { { 0.0f, "0" }, { 0.5f, "\xc2\xbd" }, { 1.0f, "1" }, { 1.5f, "1\xc2\xbd" }, { 2.0f, "2" } };
            for (auto [v, txt] : marks)
                engraved (g, String::fromUTF8 (txt), Rectangle<float> (24.0f, 12.0f).withCentre (t.pivot.getPointOnCircumference (t.rad * 0.96f, t.angleFor (v))), 8.5f);
        }
    }

    // riquadro dei comandi della testina rotante
    {
        const auto box = Rectangle<float> (lampX0, 250.0f, lampX1 - lampX0, 120.0f);
        g.setColour (Colours::black.withAlpha (mode == 2 ? 0.07f : 0.03f));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawRoundedRectangle (box, 4.0f, 1.0f);
        engraved (g, String::fromUTF8 ("TÊTE ROTATIVE  ·  UNIVERSEL"), box.reduced (8.0f, 4.0f).removeFromTop (12.0f), 8.0f,
                  Justification::centredLeft, mode == 2 ? Col::engrave : Col::engraveDim);
    }

    for (auto p : { area.getTopLeft().translated (12, 12), area.getBottomLeft().translated (12, -12) })
        screw (g, p, 4.5f, 0.9f);
}

void DeckView::paint (Graphics& g)
{
    const auto& disp = proc.engine.display;
    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    const bool octave = proc.apvts.getRawParameterValue ("octave")->load() > 0.5f;
    const float speed = disp.speed.load();
    // cromatico: innestato il capstan del tasto; coulisse e universale: il nastro è
    // trascinato dal primo capstan a velocità variabile
    const bool engaged = disp.engaged.load();
    const int activeKey = mode == 0 ? (speed > 0.001f ? disp.activeKey.load() : -1)
                                    : (engaged ? 0 : -1);
    // spie: il tasto suonato (in universale indica l'altezza del tamburo)
    const int lampKey = mode == 1 ? -1 : (engaged || speed > 0.001f ? disp.activeKey.load() : -1);
    const float scaleMax = (octave && mode != 1) ? 4.0f : 2.0f;

    // ---- strato fisso, ricostruito solo quando cambia qualcosa che vi compare ----
    const float pix = jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int key = mode * 100000 + (activeKey + 1) * 1000 + (int) scaleMax * 100 + roundToInt (pix * 10.0f);
    if (key != cacheKey || cache.isNull())
    {
        cache = Image (Image::ARGB, roundToInt ((float) getWidth() * pix), roundToInt ((float) getHeight() * pix), true);
        Graphics cg (cache);
        cg.addTransform (AffineTransform::scale (pix));
        paintStatic (cg, mode, activeKey, scaleMax);
        cacheKey = key;
    }
    g.drawImageTransformed (cache, AffineTransform::scale (1.0f / pix));

    // ---- strato mobile ----
    Ring ring (wellCentre, 130.0f, activeKey);

    // segni sul nastro che scorrono (nascosti dove il nastro passa sotto capstan e rulli)
    auto hidden = [&] (Point<float> pt)
    {
        for (int k = 0; k < 12; ++k)
            if (pt.getDistanceFrom (ring.cap[k]) < ring.capR[k] + 2.8f
                || pt.getDistanceFrom (ring.pinch[k]) < ring.pinchR + 2.0f)
                return true;
        for (auto gp : { ring.guideL, ring.guideR, ring.inner })
            if (pt.getDistanceFrom (gp) < 7.5f) return true;
        return pt.getDistanceFrom (ring.tension) < ring.tensionR + 0.5f;
    };
    const float len = ring.tape.getLength();
    g.setColour (Col::tapeLight.withAlpha (0.7f));
    for (float d = std::fmod (tapePhase, 22.0f); d < len; d += 22.0f)
    {
        const auto pt = ring.tape.getPointAlongPath (d);
        if (! hidden (pt))
            g.fillEllipse (Rectangle<float> (2.6f, 2.6f).withCentre (pt));
    }
    {
        const auto sp = ring.tape.getPointAlongPath (std::fmod (tapePhase + 60.0f, len));
        if (! hidden (sp))
        {
            g.setColour (Colour (0xffeeeae0));
            g.fillRect (Rectangle<float> (5.0f, 5.0f).withCentre (sp));
        }
    }

    // segni di rotazione: capstan (girano sempre) e guide (girano con il nastro)
    g.setColour (Colour (0xff4f514d).withAlpha (0.7f));
    auto mark = [&] (Point<float> c, float r, float rot)
    {
        g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.3f, rot), c.getPointOnCircumference (r * 0.62f, rot)), 1.3f);
    };
    for (int k = 0; k < 12; ++k)
        mark (ring.cap[k], ring.capR[k], capstanAngle + (float) k);
    mark (ring.guideL, 5.0f, tapePhase / 5.0f);
    mark (ring.guideR, 5.0f, tapePhase / 5.0f);
    mark (ring.inner, 5.0f, tapePhase / 5.0f);
    mark (ring.tension, ring.tensionR - 2.0f, tapePhase / ring.tensionR);

    if (mode == 2)
    {
        // tamburo con le testine rotanti (regolatore temporale di Springer)
        const auto dc = ring.head.translated (0.0f, -8.0f);
        const float dr = 15.0f;
        const int N = disp.heads.load();
        const float rot = disp.headPhase.load() * MathConstants<float>::twoPi / (float) N;
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillEllipse (Rectangle<float> (2 * dr, 2 * dr).withCentre (dc.translated (2.0f, 3.0f)));
        ColourGradient dg (Colour (0xfff4f4f1), dc.x - dr * 0.5f, dc.y - dr * 0.6f, Colour (0xff7a7c78), dc.x + dr, dc.y + dr, true);
        g.setGradientFill (dg);
        g.fillEllipse (Rectangle<float> (2 * dr, 2 * dr).withCentre (dc));
        g.setColour (Colour (0xff4a4c48));
        g.drawEllipse (Rectangle<float> (2 * dr, 2 * dr).withCentre (dc), 0.9f);
        g.setColour (Colour (0xff2b2b29));
        for (int i = 0; i < N; ++i)
        {
            const float a = rot + (float) i * MathConstants<float>::twoPi / (float) N;
            g.fillRect (Rectangle<float> (4.0f, 4.0f).withCentre (dc.getPointOnCircumference (dr - 3.0f, a)));
        }
        metalTop (g, dc, 4.0f, 0.0f, false);
    }

    // spia accesa
    if (lampKey >= 0 && lampKey < 12)
        drawLamp (g, lampCentre (lampKey), true);

    // tachimetro: lancetta e valori
    {
        const Tacho t (scaleMax);
        engraved (g, mode == 1 ? String::fromUTF8 ("VITESSE DU RUBAN  ·  MOTEUR ") + String (disp.motorSpeed.load(), 2).replaceCharacter ('.', ',')
                               : (mode == 2 ? String::fromUTF8 ("VITESSE DU RUBAN  ·  HAUTEUR ") + String (disp.pitch.load(), 2).replaceCharacter ('.', ',')
                                            : String ("VITESSE DU RUBAN")),
                  { t.face.getX(), t.face.getBottom() - 34.0f, t.face.getWidth(), 12.0f }, 8.5f);
        engraved (g, String (speed, 3).replaceCharacter ('.', ','), { t.face.getX(), t.face.getBottom() - 20.0f, t.face.getWidth(), 14.0f }, 10.0f);
        Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (t.face.toNearestInt());
        g.setColour (Col::engrave);
        g.drawLine (Line<float> (t.pivot.getPointOnCircumference (t.rad * 0.45f, t.angleFor (speed)),
                                 t.pivot.getPointOnCircumference (t.rad * 0.9f, t.angleFor (speed))), 1.4f);
        g.setColour (Colours::white.withAlpha (0.15f));
        g.fillRect (t.face.withHeight (t.face.getHeight() * 0.3f));
    }
}

//==============================================================================
void TapeStrip::refreshIfChanged()
{
    // firma di ciò che la finestra mostra: posizione, regione, sorgente, panoramica, stato
    const auto& d = proc.engine.display;
    double sig = d.position.load() * 7.0 + d.recSeconds.load() * 13.0 + (d.recording.load() ? 17.0 : 0.0)
               + proc.apvts.getRawParameterValue ("source")->load() * 19.0
               + proc.apvts.getRawParameterValue ("loopStart")->load() * 23.0
               + proc.apvts.getRawParameterValue ("loopLength")->load() * 29.0
               + (proc.isLoading() ? 31.0 : 0.0) + proc.getFileSeconds() * 37.0;
    for (size_t i = 0; i < 512; i += 8)
        sig += (proc.filePeaks[i].load() + d.recPeaks[i].load()) * (double) (i + 1);
    if (sig != lastSignature)
    {
        lastSignature = sig;
        repaint();
    }
}

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

    if (source == 0 && proc.isLoading())
    {
        caption = String::fromUTF8 ("CHARGEMENT DE LA BANDE…");
        for (int i = 0; i < 512; ++i) { peaks[i] = proc.filePeaks[(size_t) i].load(); empty &= peaks[i] <= 0.0f; }
    }
    else if (source == 0)
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
            g.setColour ((inside ? Col::amber : Col::stripText).withAlpha (inside ? 0.9f : 0.3f));
            g.drawVerticalLine ((int) x, mid - a, mid + a + 1.0f);
        }

        // testina sull'anello
        const float ph = x0 + (x1 - x0) * jlimit (0.0f, 1.0f, disp.position.load());
        g.setColour (Col::lamp);
        g.drawLine (Line<float> (ph, wave.getY() - 2.0f, ph, wave.getBottom() + 2.0f), 1.6f);
    }

    g.setFont (labelFont (9.5f));
    g.setColour (Col::stripText);
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

void OctaveKeyboard::refreshIfChanged()
{
    const int shown = mouseKey * 16 + proc.engine.display.activeKey.load();
    if (shown != lastShown)
    {
        lastShown = shown;
        repaint();
    }
}

void OctaveKeyboard::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // cassa in legno
    ColourGradient frame (Col::enamelDark.brighter (0.1f), r.getX(), r.getY(), Col::enamelDark.darker (0.2f), r.getX(), r.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Colours::black.withAlpha (0.3f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
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
                if (down)
                {
                    g.setColour (Col::lamp.withAlpha (0.55f));
                    g.fillRoundedRectangle (kb.removeFromBottom (7.0f).reduced (6.0f, 1.5f), 2.0f);
                }
            }
            else
            {
                if (! down)
                {
                    g.setColour (Colours::black.withAlpha (0.4f));
                    g.fillRoundedRectangle (kb.translated (2.0f, 2.0f), 2.5f);
                    ColourGradient kg (Colour (0xff3a3632), kb.getX(), kb.getY(), Colour (0xff0b0a09), kb.getX(), kb.getBottom(), false);
                    g.setGradientFill (kg);
                    g.fillRoundedRectangle (kb.withTrimmedBottom (3.0f), 2.5f);
                    // riflesso sul bordo anteriore del tasto a riposo
                    g.setColour (Colours::white.withAlpha (0.12f));
                    g.fillRect (kb.withTrimmedBottom (3.0f).removeFromBottom (5.0f).reduced (2.0f, 0.0f));
                }
                else
                {
                    // tasto abbassato: più corto, senza ombra, illuminato in rosso come i tasti bianchi
                    auto pk = kb.withTrimmedBottom (8.0f);
                    ColourGradient kg (Colour (0xff4a3a34), pk.getX(), pk.getY(), Colour (0xff2a1612), pk.getX(), pk.getBottom(), false);
                    g.setGradientFill (kg);
                    g.fillRoundedRectangle (pk, 2.5f);
                    g.setColour (Col::lamp.withAlpha (0.85f));
                    g.fillRoundedRectangle (pk.removeFromBottom (10.0f).reduced (3.0f, 1.0f), 2.0f);
                    g.setColour (Col::lamp.withAlpha (0.35f));
                    g.drawRoundedRectangle (kb.withTrimmedBottom (8.0f).expanded (1.0f), 3.0f, 1.5f);
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
    setupKnob (kHeads,    "heads",     String::fromUTF8 ("TÊTES"));
    setupKnob (kWindow,   "window",    String::fromUTF8 ("FENÊTRE"));
    setupKnob (kHardness, "hardness",  String::fromUTF8 ("DURETÉ"));

    for (auto* s : { &kMode.slider, &kSource.slider, &kHeads.slider })
    {
        s->getProperties().set ("selector", true);
        s->setMouseDragSensitivity (40);        // basta un breve trascinamento per cambiare posizione
        s->setPopupDisplayEnabled (false, false, nullptr);
        s->setRotaryParameters (MathConstants<float>::pi * 1.72f, MathConstants<float>::pi * 2.28f, true);
    }
    kSource.slider.setRotaryParameters (MathConstants<float>::pi * 1.8f, MathConstants<float>::pi * 2.2f, true);

    setupToggle (tRecord,   aRecord,   "record",   "ENREG.");
    setupToggle (tMonitor,  aMonitor,  "monitor",  String::fromUTF8 ("ÉCOUTE"));
    setupToggle (tRestart,  aRestart,  "restart",  "RETOUR");
    setupToggle (tExtended, aExtended, "extended", String::fromUTF8 ("ÉTENDU"));
    setupToggle (tOctave,   aOctave,   "octave",   "OCTAVE");
    setupToggle (tMono,     aMono,     "mono",     "MONO");
    setupToggle (tSplice,   aSplice,   "splice",   "COLLURE");
    tRecord.setComponentID ("rec");

    registerCC (kStart.slider, "loopStart");   registerCC (kLength.slider, "loopLength");
    registerCC (kInertia.slider, "inertia");   registerCC (kLevel.slider, "level");
    registerCC (kWow.slider, "wow");           registerCC (kHiss.slider, "hiss");
    registerCC (kDrive.slider, "drive");       registerCC (kCoulisse.slider, "coulisse");
    registerCC (kMode.slider, "mode");         registerCC (kSource.slider, "source");
    registerCC (tRecord, "record");            registerCC (tMonitor, "monitor");
    registerCC (tRestart, "restart");          registerCC (tExtended, "extended");
    registerCC (tOctave, "octave");            registerCC (tMono, "mono");
    registerCC (tSplice, "splice");
    registerCC (kHeads.slider, "heads");       registerCC (kWindow.slider, "window");
    registerCC (kHardness.slider, "hardness");

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { openFileChooser(); };
    addAndMakeVisible (exportButton);
    exportButton.onClick = [this] { exportRecording(); };
    hoverOnly.push_back ({ &loadButton, "load" });
    hoverOnly.push_back ({ &exportButton, "export" });
    exportButton.setTooltip (String::fromUTF8 ("Exporter la bande enregistrée en WAV ou AIFF"));

    // finestra ridimensionabile con proporzioni fisse; la dimensione si salva nel progetto
    setResizable (true, true);
    setResizeLimits (600, (int) std::round (600 * logicalH / logicalW), 1500, (int) std::round (1500 * logicalH / logicalW));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio (logicalW / logicalH);
    setSize ((int) logicalW, (int) logicalH);   // apre sempre alla dimensione d'origine
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
    k.slider.setPopupDisplayEnabled (false, false, nullptr);   // il valore è nel riquadro informativo
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
    texture = Image (Image::RGB, jmax (1, getWidth()), jmax (1, getHeight()), true);
    Graphics g (texture);
    g.fillAll (Col::enamel);
    g.addTransform (AffineTransform::scale (uiScale()));

    // smalto martellato: tante piccole ammaccature chiare e scure
    Random rnd (1953);
    for (int i = 0; i < 2600; ++i)
    {
        const float x = rnd.nextFloat() * logicalW;
        const float y = rnd.nextFloat() * logicalH;
        const float r = 3.0f + rnd.nextFloat() * 7.0f;
        const bool light = rnd.nextBool();
        g.setColour ((light ? Colours::white : Colours::black).withAlpha (0.01f + rnd.nextFloat() * 0.015f));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
    }
    ColourGradient vignette (Colours::transparentBlack, logicalW * 0.5f, logicalH * 0.4f,
                             Colours::black.withAlpha (0.16f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();
}

void PhonogeneEditor::paint (Graphics& g)
{
    if (texture.getWidth() != getWidth() || texture.getHeight() != getHeight())
        buildTexture();

    g.drawImageAt (texture, 0, 0);
    g.addTransform (AffineTransform::scale (uiScale()));

    auto r = Rectangle<float> (logicalW, logicalH);
    g.setColour (Colours::white.withAlpha (0.15f));
    g.drawRect (r.reduced (1.0f), 1.0f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawRect (r, 1.0f);

    for (auto p : { Point<float> (12, 12), Point<float> (r.getRight() - 12, 12),
                    Point<float> (12, r.getBottom() - 12), Point<float> (r.getRight() - 12, r.getBottom() - 12) })
        screw (g, p, 5.0f, 0.4f);

    // incasso della sezione inferiore
    g.setColour (Colours::black.withAlpha (0.06f));
    g.fillRoundedRectangle (Rectangle<float> (440.0f, 500.0f, 536.0f, 218.0f), 5.0f);
    g.setColour (Colours::black.withAlpha (0.15f));
    g.drawRoundedRectangle (Rectangle<float> (440.0f, 500.0f, 536.0f, 218.0f), 5.0f, 1.0f);

    paintPanelLabels (g);
}

void PhonogeneEditor::paintOverChildren (Graphics& g)
{
    g.addTransform (AffineTransform::scale (uiScale()));
    // MIDI learn in corso: cornice sul comando e istruzione
    if (const int learn = proc.getLearnIndex(); learn >= 0)
        if (auto* c = componentForCCIndex (learn))
        {
            const float pulse = 0.55f + 0.45f * std::sin ((float) Time::getMillisecondCounter() * 0.008f);
            const auto b = c->getBounds().toFloat().expanded (4.0f);
            g.setColour (Col::amber.withAlpha (pulse));
            g.drawRoundedRectangle (b, 6.0f, 2.5f);
            auto banner = Rectangle<float> (b.getCentreX() - 150.0f, b.getY() - 26.0f, 300.0f, 20.0f)
                              .constrainedWithin (Rectangle<float> (logicalW, logicalH).reduced (4.0f));
            g.setColour (Colour (0xff1d1d1c).withAlpha (0.9f));
            g.fillRoundedRectangle (banner, 4.0f);
            g.setColour (Col::amber);
            g.setFont (labelFont (9.5f));
            g.drawText ("MIDI LEARN  ·  MUOVI UN CONTROLLO  (CTRL+CLIC ANNULLA)", banner, Justification::centred);
        }

    paintHover (g);

    auto labelUnder = [&] (const Knob& k, float size = 9.5f)
    {
        const auto b = k.slider.getBounds().toFloat();
        engraved (g, k.label, { b.getX() - 10.0f, b.getBottom() - 2.0f, b.getWidth() + 20.0f, 14.0f }, size);
    };
    // etichette dentro il deck (il deck è un componente opaco: vanno disegnate sopra)
    for (auto* k : { &kWindow, &kHardness })
        labelUnder (*k, 8.5f);

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
    positions (kHeads, { "2", "3", "4" });
}

// Etichette e scale sul pannello inferiore: disegnate sotto i comandi, così il
// valore a comparsa di una manopola e l'impugnatura della leva le coprono.
void PhonogeneEditor::paintPanelLabels (Graphics& g)
{
    auto labelUnder = [&] (const Knob& k, float size = 9.5f)
    {
        const auto b = k.slider.getBounds().toFloat();
        engraved (g, k.label, { b.getX() - 10.0f, b.getBottom() - 2.0f, b.getWidth() + 20.0f, 14.0f }, size);
    };
    for (auto* k : { &kStart, &kLength, &kInertia, &kLevel, &kWow, &kHiss, &kDrive })
        labelUnder (*k);

    // scala della leva della coulisse
    {
        const auto b = kCoulisse.slider.getBounds().toFloat();
        const auto& range = kCoulisse.slider;
        const std::pair<double, const char*> marks[] = { { 2.0, "2" }, { 1.0, "1" }, { 0.5, "\xc2\xbd" }, { 0.02, "0" } };
        for (auto [v, txt] : marks)
        {
            const float y = (float) range.getPositionOfValue (v) + b.getY();
            g.setColour (Col::engrave.withAlpha (0.8f));
            g.drawHorizontalLine ((int) y, b.getX() - 4.0f, b.getX() + 10.0f);
            engraved (g, String::fromUTF8 (txt), { b.getX() - 26.0f, y - 7.0f, 20.0f, 14.0f }, 9.0f, Justification::centredRight);
        }
        engraved (g, kCoulisse.label, { b.getX() - 14.0f, b.getBottom() + 4.0f, b.getWidth() + 28.0f, 14.0f }, 9.5f);
    }
}

void PhonogeneEditor::resized()
{
    layoutLogical();

    // ogni comando è posizionato in coordinate logiche e poi scalato
    const auto t = AffineTransform::scale (uiScale());
    for (Component* c : std::initializer_list<Component*> {
             &deck, &strip, &keyboard, &loadButton, &exportButton,
             &kStart.slider, &kLength.slider, &kInertia.slider, &kLevel.slider, &kWow.slider, &kHiss.slider,
             &kDrive.slider, &kMode.slider, &kSource.slider, &kCoulisse.slider,
             &kHeads.slider, &kWindow.slider, &kHardness.slider,
             &tRecord, &tMonitor, &tRestart, &tExtended, &tOctave, &tMono, &tSplice })
        c->setTransform (t);

    texture = {};
}

void PhonogeneEditor::layoutLogical()
{
    deck.setBounds (24, 24, 952, 380);
    strip.setBounds (24, 416, 826, 70);
    loadButton.setBounds (862, 416, 114, 34);
    exportButton.setBounds (862, 454, 114, 32);

    keyboard.setBounds (24, 500, 344, 218);
    // leve della tastiera: seconda ottava ed estensione
    tOctave.setBounds   (378, 508, 54, 94);
    tExtended.setBounds (378, 614, 54, 94);

    // leva
    kCoulisse.slider.setBounds (480, 518, 44, 170);

    // manopole: due file da quattro
    const int x0 = 548, cw = 106, kh = 86;
    kStart.slider.setBounds  (x0 + cw * 0, 508, cw, kh);
    kLength.slider.setBounds (x0 + cw * 1, 508, cw, kh);
    kInertia.slider.setBounds(x0 + cw * 2, 508, cw, kh);
    kLevel.slider.setBounds  (x0 + cw * 3, 508, cw, kh);
    kWow.slider.setBounds    (x0 + cw * 0, 612, cw, kh);
    kHiss.slider.setBounds   (x0 + cw * 1, 612, cw, kh);
    kDrive.slider.setBounds  (x0 + cw * 2, 612, cw, kh);
    tRestart.setBounds       (x0 + cw * 3 - 4,  606, 56, 94);
    tSplice.setBounds        (x0 + cw * 3 + 52, 606, 56, 94);

    // zona comandi del deck
    const int dx = 24 + 952 - 226;
    kMode.slider.setBounds   (dx + 22,  150, 92, 80);
    kSource.slider.setBounds (dx + 122, 150, 92, 80);
    tRecord.setBounds        (dx + 20,  264, 60, 92);

    // testina rotante, nel riquadro sotto il tachimetro
    kHeads.slider.setBounds    (470, 312, 72, 54);
    kWindow.slider.setBounds   (550, 304, 84, 62);
    kHardness.slider.setBounds (638, 304, 84, 62);
    tMonitor.setBounds       (dx + 92,  262, 56, 94);
    tMono.setBounds          (dx + 156, 262, 56, 94);
}

//==============================================================================
// Descrizioni in italiano dei comandi (riquadro al passaggio del mouse).
namespace
{
    struct ControlInfo { const char* name; const char* text; const char* unit; };

    ControlInfo infoFor (const String& id)
    {
        static const std::map<String, ControlInfo> table = {
            { "mode",       { "Modalità", "Sceglie la macchina: cromatico (12 tasti = 12 capstan), a coulisse (velocità continua con la leva), universale (testina rotante: altezza e durata indipendenti).", "" } },
            { "source",     { "Sorgente", "Nastro da file (FICHIER) o registrato dall'ingresso / sidechain (ENTRÉE).", "" } },
            { "record",     { "Registrazione", "Registra l'ingresso sull'anello di nastro (fino a 30 s). Attiva con SOURCE su ENTRÉE.", "" } },
            { "monitor",    { "Ascolto", "Manda in uscita anche il segnale in ingresso, per sentire cosa si registra.", "" } },
            { "mono",       { "Mono", "Nastro a una traccia come l'originale: stesso segnale sulle due uscite.", "" } },
            { "restart",    { "Ritorno", "I: ogni nuovo attacco riparte dall'inizio dell'anello. O: il nastro riprende da dove si era fermato.", "" } },
            { "splice",     { "Giunta", "Fa sentire la giunta adesiva che chiude l'anello: un breve calo a ogni giro.", "" } },
            { "extended",   { "Tastiera estesa", "O: tutte le ottave ricadono nell'unica ottava della macchina. I: ogni tasto ha la sua velocità (C3 = nominale).", "" } },
            { "octave",     { "Ottava", "Porta la tastiera un'ottava sopra (velocità del nastro ×2), come la seconda ottava del cromatico.", "" } },
            { "loopStart",  { "Inizio", "Punto del nastro da cui comincia l'anello (frazione della lunghezza).", "" } },
            { "loopLength", { "Lunghezza", "Durata dell'anello, come frazione del nastro: anelli corti diventano ritmi e ronzii.", "" } },
            { "inertia",    { "Inerzia", "Tempo con cui il nastro raggiunge la nuova velocità: attacchi secchi o glissandi. In coulisse è la massa del motore.", "ms" } },
            { "level",      { "Livello", "Livello d'uscita.", "dB" } },
            { "wow",        { "Fluttuazione", "Pleurage e scintillio: le piccole oscillazioni di velocità del trasporto meccanico.", "" } },
            { "hiss",       { "Soffio", "Fruscio del nastro, che si traspone con la velocità di lettura.", "" } },
            { "drive",      { "Saturazione", "Saturazione morbida della testina: arrotonda i picchi e aggiunge densità.", "" } },
            { "coulisse",   { "Leva", "In coulisse: velocità del motore (0-2, mod wheel). In universale: velocità del nastro, cioè la durata, senza cambiare altezza.", "×" } },
            { "heads",      { "Testine", "Numero di testine sul tamburo rotante (modalità universale).", "" } },
            { "window",     { "Finestra", "Distanza tra due testine misurata sul nastro: corta = suono granuloso, lunga = più liscio con ripetizioni.", "ms" } },
            { "hardness",   { "Durezza", "Passaggio tra una testina e l'altra: 0 dissolvenza morbida, 1 commutazione secca con i clic tipici.", "" } },
            { "load",       { "Carica", "Carica un file audio (WAV, AIFF, FLAC...) come nastro. Si può anche trascinare il file sul plugin.", "" } },
            { "export",     { "Esporta", "Salva la registrazione dell'ingresso come file WAV o AIFF a 24 bit.", "" } },
        };
        const auto it = table.find (id);
        return it != table.end() ? it->second : ControlInfo { "", "", "" };
    }
}

String PhonogeneEditor::paramIdFor (Component* c) const
{
    for (const auto& [comp, id] : ccControls) if (comp == c) return id;
    for (const auto& [comp, id] : hoverOnly)  if (comp == c) return id;
    return {};
}

String PhonogeneEditor::valueText (const String& id) const
{
    auto* prm = proc.apvts.getParameter (id);
    if (prm == nullptr) return {};
    if (auto* b = dynamic_cast<AudioParameterBool*> (prm))
        return b->get() ? "I (attivo)" : "O (spento)";
    if (auto* c = dynamic_cast<AudioParameterChoice*> (prm))
    {
        if (id == "mode")   { const char* n[] = { "cromatico", "a coulisse", "universale" }; return n[jlimit (0, 2, c->getIndex())]; }
        if (id == "source") return c->getIndex() == 0 ? "file" : "ingresso";
        return c->getCurrentChoiceName();
    }
    const float v = prm->convertFrom0to1 (prm->getValue());
    String t = String (v, 2).replaceCharacter ('.', ',');
    const String unit = String::fromUTF8 (infoFor (id).unit);
    if (unit.isNotEmpty()) t << " " << unit;
    return t;
}

Rectangle<int> PhonogeneEditor::toScreenArea (Rectangle<float> r) const
{
    return r.transformedBy (AffineTransform::scale (uiScale())).getSmallestIntegerContainer().expanded (2);
}

void PhonogeneEditor::showHoverFor (const String& paramId)
{
    forcedHover = paramId;
    updateHover();
}

void PhonogeneEditor::updateHover()
{
    Component* now = nullptr;
    if (forcedHover.isNotEmpty())
        for (auto* list : { &ccControls, &hoverOnly })
            for (const auto& [comp, id] : *list)
                if (id == forcedHover) now = comp;
    for (auto* list : { &ccControls, &hoverOnly })
        for (const auto& [comp, id] : *list)
            if (comp->isMouseOverOrDragging (true)) { now = comp; break; }

    String key;
    if (now != nullptr)
    {
        const auto id = paramIdFor (now);
        const int idx = proc.ccIndexForParam (id);
        key << id << "|" << valueText (id) << "|" << (idx >= 0 ? proc.getAssignedCC (idx) : -2);
    }
    if (now == hovered && key == hoverKey) return;

    if (! hoverRect.isEmpty()) repaint (toScreenArea (hoverRect));
    hovered = now;
    hoverKey = key;
    hoverRect = {};
    if (hovered != nullptr)
    {
        // posizione: sotto il comando se c'è spazio, altrimenti sopra
        const float w = 270.0f, h = 92.0f;
        const auto b = hovered->getBounds().toFloat();
        Rectangle<float> r (b.getCentreX() - w * 0.5f, b.getBottom() + 4.0f, w, h);
        if (r.getBottom() > logicalH - 6.0f) r.setY (b.getY() - h - 4.0f);
        hoverRect = r.constrainedWithin (Rectangle<float> (logicalW, logicalH).reduced (6.0f));
        repaint (toScreenArea (hoverRect));
    }
}

void PhonogeneEditor::paintHover (Graphics& g)
{
    if (hovered == nullptr || hoverRect.isEmpty()) return;
    const auto id = paramIdFor (hovered);
    const auto info = infoFor (id);
    const int idx = proc.ccIndexForParam (id);

    auto r = hoverRect;
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (2.0f, 3.0f), 5.0f);
    g.setColour (Colour (0xff1d1d1c));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Col::amber.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    auto in = r.reduced (10.0f, 7.0f);
    const auto value = valueText (id);
    auto top = in.removeFromTop (16.0f);
    g.setFont (Font (FontOptions (12.0f, Font::bold)));
    g.setColour (Col::amber);
    g.drawText (String::fromUTF8 (info.name), top, Justification::centredLeft);
    if (value.isNotEmpty())
    {
        g.setColour (Colours::white);
        g.drawText (value, top, Justification::centredRight);
    }

    auto bottom = in.removeFromBottom (13.0f);
    g.setFont (Font (FontOptions (9.5f)));
    g.setColour (Colour (0xffb9b5a6));
    if (idx >= 0)
    {
        const int cc = proc.getAssignedCC (idx);
        g.drawText ((cc >= 0 ? "CC " + String (cc) : String ("nessun CC")) + String::fromUTF8 ("  ·  ctrl+clic: riassegna  ·  tasto destro: menu"),
                    bottom, Justification::centredLeft);
    }

    g.setColour (Colour (0xffe9e4d4));
    g.setFont (Font (FontOptions (10.5f)));
    g.drawFittedText (String::fromUTF8 (info.text), in.reduced (0.0f, 2.0f).toNearestInt(), Justification::topLeft, 4, 0.95f);
}

void PhonogeneEditor::registerCC (Component& c, const String& paramId)
{
    ccControls.push_back ({ &c, paramId });
    c.addMouseListener (this, false);
}

Component* PhonogeneEditor::componentForCCIndex (int index) const
{
    if (index < 0 || index >= PhonogeneProcessor::numCCTargets) return nullptr;
    for (const auto& [comp, id] : ccControls)
        if (id == PhonogeneProcessor::ccTargets[index].paramId) return comp;
    return nullptr;
}

void PhonogeneEditor::mouseDown (const MouseEvent& e)
{
    if (! e.mods.isPopupMenu()) return;   // comprende ctrl+clic e tasto destro
    for (const auto& [comp, id] : ccControls)
        if (e.eventComponent == comp || comp->isParentOf (e.eventComponent))
        {
            const int idx = proc.ccIndexForParam (id);
            if (e.mods.isCtrlDown() && ! e.mods.isRightButtonDown() && idx >= 0)
            {
                // ctrl+clic: riassegnazione immediata (muovi il controllo MIDI desiderato)
                if (proc.getLearnIndex() == idx) proc.cancelLearn();
                else proc.startLearn (idx);
                repaint();
            }
            else
                showCCMenu (*comp, id);
            return;
        }
}

void PhonogeneEditor::showCCMenu (Component& c, const String& paramId)
{
    const int idx = proc.ccIndexForParam (paramId);
    if (idx < 0) return;
    const int cc = proc.getAssignedCC (idx);

    PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addSectionHeader (cc >= 0 ? "CC " + String (cc) : String ("Nessun CC assegnato"));
    m.addItem (1, "MIDI learn (muovi un controllo)");

    // scelta diretta del numero di CC, a gruppi di 16
    PopupMenu choose;
    for (int group = 0; group < 8; ++group)
    {
        PopupMenu sub;
        for (int n = group * 16; n < jmin (120, group * 16 + 16); ++n)
        {
            const int other = proc.commandForCC (n);
            String text = "CC " + String (n);
            if (other >= 0 && other != idx)
                text << "  (ora: " << infoFor (PhonogeneProcessor::ccTargets[other].paramId).name << ")";
            sub.addItem (1000 + n, text, true, n == cc);
        }
        if (group * 16 < 120)
            choose.addSubMenu ("CC " + String (group * 16) + " - " + String (jmin (119, group * 16 + 15)), sub);
    }
    m.addSubMenu ("Assegna CC", choose);
    m.addItem (2, "Rimuovi il CC", cc >= 0);
    m.addSeparator();
    m.addItem (3, "Ripristina i CC predefiniti");
    if (proc.getLearnIndex() >= 0)
        m.addItem (4, "Annulla il MIDI learn");

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&c),
                     [this, idx] (int r)
                     {
                         if (r == 1) proc.startLearn (idx);
                         if (r == 2) proc.forgetCC (idx);
                         if (r == 3) proc.resetDefaultCCs();
                         if (r == 4) proc.cancelLearn();
                         if (r >= 1000) proc.assignCC (idx, r - 1000);
                         hoverKey = {};
                         repaint();
                     });
}

void PhonogeneEditor::timerCallback()
{
    deck.advance (1.0f / 30.0f);
    updateHover();
    const bool coulisseMode = (int) proc.apvts.getRawParameterValue ("mode")->load() == 1;
    kCoulisse.slider.setAlpha (coulisseMode ? 1.0f : 0.5f);
    tExtended.setAlpha (coulisseMode ? 0.5f : 1.0f);    // in coulisse l'altezza del tasto non conta
    tOctave.setAlpha (coulisseMode ? 0.5f : 1.0f);
    const bool universal = (int) proc.apvts.getRawParameterValue ("mode")->load() == 2;
    kCoulisse.slider.setAlpha (coulisseMode || universal ? 1.0f : 0.5f);   // in universale la leva regola la durata
    for (auto* k : { &kHeads, &kWindow, &kHardness })
        k->slider.setAlpha (universal ? 1.0f : 0.5f);

    const int learn = proc.getLearnIndex();
    if (learn != lastLearn || learn >= 0)
        repaint();
    lastLearn = learn;
    deck.repaint();
    strip.refreshIfChanged();
    keyboard.refreshIfChanged();
}

void PhonogeneEditor::openFileChooser()
{
    chooser = std::make_unique<FileChooser> ("Choisir une bande", File(), proc.formatManager.getWildcardForAllFormats());
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (! f.existsAsFile()) return;
                              proc.loadFileAsync (f);
                              if (auto* src = proc.apvts.getParameter ("source"))
                                  src->setValueNotifyingHost (0.0f);
                          });
}

void PhonogeneEditor::exportRecording()
{
    if (! proc.hasRecording())
    {
        AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "Phonogene",
                                          String::fromUTF8 ("Aucune bande enregistrée : choisissez SOURCE ENTRÉE et ENREG."));
        return;
    }

    chooser = std::make_unique<FileChooser> ("Exporter la bande",
                                             File::getSpecialLocation (File::userMusicDirectory).getChildFile ("Phonogene bande.wav"),
                                             "*.wav;*.aif;*.aiff");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                              | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == File()) return;
                              if (! f.hasFileExtension ("wav;aif;aiff"))
                                  f = f.withFileExtension ("wav");
                              if (! proc.exportRecording (f))
                                  AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "Phonogene",
                                                                    String::fromUTF8 ("Exportation impossible : ") + f.getFullPathName());
                          });
}

void PhonogeneEditor::filesDropped (const StringArray& files, int, int)
{
    for (const auto& path : files)
        if (proc.formatManager.findFormatForFileExtension (File (path).getFileExtension()) != nullptr)
        {
            proc.loadFileAsync (File (path));
            if (auto* src = proc.apvts.getParameter ("source"))
                src->setValueNotifyingHost (0.0f);
            break;
        }
}
