// Proposte grafiche (statiche) ispirate alle foto del Phonogène chromatique.
// Disegnate con JUCE, così la proposta scelta si porta direttamente nell'editor.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <iostream>

using namespace juce;

namespace Art
{
    const Colour enamel    { 0xffcdcbc2 };   // smalto grigio chiaro della macchina
    const Colour enamelDk  { 0xffaeaca3 };
    const Colour plate     { 0xffdad8cf };
    const Colour well      { 0xffc9c7be };
    const Colour ink       { 0xff262624 };   // serigrafia nera
    const Colour tape      { 0xff4a3020 };
    const Colour tapeHi    { 0xff7a573c };
    const Colour ivory     { 0xfff1ecdc };
    const Colour lamp      { 0xffd8401c };

    const char* noteNames[12] = { "DO", "DO#", "RÉ", "RÉ#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI" };

    String u (const char* s) { return String::fromUTF8 (s); }

    Font font (float size, bool bold = true)
    {
        return Font (FontOptions (size, bold ? Font::bold : Font::plain)).withExtraKerningFactor (0.1f);
    }

    void print (Graphics& g, const String& t, Rectangle<float> r, float size, Justification j = Justification::centred,
                Colour c = ink, bool bold = true)
    {
        g.setFont (font (size, bold));
        g.setColour (c);
        g.drawText (t, r, j, false);
    }

    void screw (Graphics& g, Point<float> c, float r, float a = 0.5f)
    {
        ColourGradient gr (Colour (0xffeeeeea), c.x - r, c.y - r, Colour (0xff7d7f7b), c.x + r, c.y + r, false);
        g.setGradientFill (gr);
        g.fillEllipse (Rectangle<float> (2 * r, 2 * r).withCentre (c));
        g.setColour (Colour (0xff55574f));
        g.drawEllipse (Rectangle<float> (2 * r, 2 * r).withCentre (c), 0.7f);
        g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.8f, a), c.getPointOnCircumference (r * 0.8f, a + MathConstants<float>::pi)), 1.2f);
    }

    // disco metallico visto dall'alto (capstan, rullo, perno)
    void metalTop (Graphics& g, Point<float> c, float r, bool bright = true)
    {
        auto b = Rectangle<float> (2 * r, 2 * r).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.28f));
        g.fillEllipse (b.translated (r * 0.18f + 1.5f, r * 0.22f + 2.0f));
        ColourGradient gr (Colour (bright ? 0xfffbfbf8 : 0xffe4e4e0), c.x - r * 0.5f, c.y - r * 0.6f,
                           Colour (0xff8a8c88), c.x + r, c.y + r, true);
        g.setGradientFill (gr);
        g.fillEllipse (b);
        g.setColour (Colour (0xff6c6e6a));
        g.drawEllipse (b, 0.9f);
        g.setColour (Colour (0xff9fa19c));
        g.drawEllipse (b.reduced (r * 0.35f), 0.8f);
        g.setColour (Colour (0xff5c5e5a));
        g.fillEllipse (Rectangle<float> (r * 0.36f, r * 0.36f).withCentre (c));
    }

    void rubberTop (Graphics& g, Point<float> c, float r)
    {
        auto b = Rectangle<float> (2 * r, 2 * r).withCentre (c);
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillEllipse (b.translated (2.0f, 2.5f));
        ColourGradient gr (Colour (0xff3d3d3b), c.x - r * 0.4f, c.y - r * 0.5f, Colour (0xff111110), c.x + r, c.y + r, true);
        g.setGradientFill (gr);
        g.fillEllipse (b);
        metalTop (g, c, r * 0.42f, false);
    }

    // cilindro in prospettiva: base sul pavimento, altezza verso l'alto
    void cylinder (Graphics& g, Point<float> base, float r, float k, float h, bool rubber = false, bool cap = true)
    {
        const float ry = r * k;
        // ombra
        g.setColour (Colours::black.withAlpha (0.22f));
        g.fillEllipse (Rectangle<float> (2 * r * 1.15f, 2 * ry * 1.15f).withCentre (base.translated (r * 0.35f, ry * 0.5f)));
        // fianco
        Rectangle<float> side (base.x - r, base.y - h, 2 * r, h);
        ColourGradient sg (rubber ? Colour (0xff1c1c1b) : Colour (0xff8e908c), side.getX(), 0,
                           rubber ? Colour (0xff050505) : Colour (0xff6a6c68), side.getRight(), 0, false);
        sg.addColour (0.3, rubber ? Colour (0xff3a3a38) : Colour (0xfff6f6f3));
        g.setGradientFill (sg);
        g.fillRect (side);
        g.fillEllipse (Rectangle<float> (2 * r, 2 * ry).withCentre (base));
        // coperchio
        if (cap)
        {
            const auto top = base.translated (0, -h);
            ColourGradient tg (rubber ? Colour (0xff3a3a38) : Colour (0xfffdfdfb), top.x - r * 0.4f, top.y - ry,
                               rubber ? Colour (0xff141413) : Colour (0xffa9aba6), top.x + r, top.y + ry, false);
            g.setGradientFill (tg);
            g.fillEllipse (Rectangle<float> (2 * r, 2 * ry).withCentre (top));
            g.setColour (Colour (rubber ? 0xff000000 : 0xff6c6e6a));
            g.drawEllipse (Rectangle<float> (2 * r, 2 * ry).withCentre (top), 0.8f);
            if (! rubber)
            {
                g.setColour (Colour (0xff7f817d));
                g.drawEllipse (Rectangle<float> (r, ry).withCentre (top), 0.7f);
            }
        }
    }

    void blackKnob (Graphics& g, Point<float> c, float r, float angle, bool ticks = true)
    {
        if (ticks)
            for (int i = 0; i < 11; ++i)
            {
                const float a = -2.35f + 4.7f * (float) i / 10.0f;
                g.setColour (ink.withAlpha (0.8f));
                g.drawLine (Line<float> (c.getPointOnCircumference (r * 1.18f, a), c.getPointOnCircumference (r * (i % 5 == 0 ? 1.42f : 1.32f), a)), 1.1f);
            }
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillEllipse (Rectangle<float> (2 * r, 2 * r).withCentre (c.translated (2.0f, 3.0f)));
        ColourGradient gr (Colour (0xff4a4440), c.x - r * 0.5f, c.y - r * 0.6f, Colour (0xff0d0c0b), c.x + r, c.y + r, true);
        g.setGradientFill (gr);
        g.fillEllipse (Rectangle<float> (2 * r, 2 * r).withCentre (c));
        for (int i = 0; i < 24; ++i)
        {
            const float a = (float) i / 24.0f * MathConstants<float>::twoPi;
            g.setColour (Colours::white.withAlpha (0.07f));
            g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.82f, a), c.getPointOnCircumference (r, a)), 1.0f);
        }
        g.setColour (Colour (0xffeae6da));
        g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.25f, angle), c.getPointOnCircumference (r * 0.92f, angle)), 2.0f);
    }

    void toggle (Graphics& g, Point<float> c, bool on)
    {
        metalTop (g, c, 8.0f);
        const auto tip = c.translated (0.0f, on ? -14.0f : 14.0f);
        g.setColour (Colours::black.withAlpha (0.25f));
        g.drawLine (Line<float> (c.translated (2, 3), tip.translated (2, 3)), 4.0f);
        g.setColour (Colour (0xffd9dbd7));
        g.drawLine (Line<float> (c, tip), 4.0f);
        g.setColour (Colours::white);
        g.fillEllipse (Rectangle<float> (7, 7).withCentre (tip));
    }

    void lampJewel (Graphics& g, Point<float> c, float r, bool on, Colour col = lamp)
    {
        g.setColour (Colour (0xff2a2a28));
        g.fillEllipse (Rectangle<float> (2 * r + 6, 2 * r + 6).withCentre (c));
        if (on) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (4 * r, 4 * r).withCentre (c)); }
        g.setColour (on ? col : col.darker (2.0f));
        g.fillEllipse (Rectangle<float> (2 * r, 2 * r).withCentre (c));
        g.setColour (Colours::white.withAlpha (on ? 0.6f : 0.2f));
        g.fillEllipse (Rectangle<float> (r * 0.7f, r * 0.7f).withCentre (c.translated (-r * 0.35f, -r * 0.35f)));
    }

    void pushButton (Graphics& g, Rectangle<float> r, const String& text)
    {
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (r.translated (1.5f, 2.5f), 3.0f);
        ColourGradient gr (Colour (0xff3b3734), r.getX(), r.getY(), Colour (0xff121110), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, 3.0f);
        print (g, text, r, 9.5f, Justification::centred, Colour (0xffe8e4d8));
    }

    void vuMeter (Graphics& g, Rectangle<float> r, float level, const String& caption)
    {
        g.setColour (Colour (0xff1d1d1c));
        g.fillRoundedRectangle (r, 3.0f);
        auto face = r.reduced (6.0f);
        ColourGradient fg (Colour (0xfff8f3e2), face.getX(), face.getY(), Colour (0xffe2dac2), face.getX(), face.getBottom(), false);
        g.setGradientFill (fg);
        g.fillRect (face);
        const Point<float> pivot { face.getCentreX(), face.getBottom() + face.getHeight() * 0.3f };
        const float rad = face.getHeight() * 1.1f;
        for (int i = 0; i <= 10; ++i)
        {
            const float a = -0.75f + 1.5f * (float) i / 10.0f;
            g.setColour (i >= 8 ? Colour (0xffb8241a) : ink);
            g.drawLine (Line<float> (pivot.getPointOnCircumference (rad * 0.8f, a), pivot.getPointOnCircumference (rad * (i % 5 == 0 ? 0.92f : 0.87f), a)), 1.1f);
        }
        Path arc;
        arc.addCentredArc (pivot.x, pivot.y, rad * 0.8f, rad * 0.8f, 0.0f, -0.75f, 0.45f, true);
        g.setColour (ink);
        g.strokePath (arc, PathStrokeType (1.0f));
        Path red;
        red.addCentredArc (pivot.x, pivot.y, rad * 0.8f, rad * 0.8f, 0.0f, 0.45f, 0.75f, true);
        g.setColour (Colour (0xffb8241a));
        g.strokePath (red, PathStrokeType (2.2f));
        print (g, caption, face.withTrimmedTop (face.getHeight() * 0.62f).withTrimmedBottom (2.0f), 8.5f);
        const float a = -0.75f + 1.5f * level;
        g.saveState();
        g.reduceClipRegion (face.toNearestInt());
        g.setColour (ink);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (rad * 0.42f, a), pivot.getPointOnCircumference (rad * 0.9f, a)), 1.2f);
        g.restoreState();
        g.setColour (Colours::white.withAlpha (0.18f));
        g.fillRect (face.withHeight (face.getHeight() * 0.35f));
    }

    void keyboard (Graphics& g, Rectangle<float> r, int active, bool labels = true)
    {
        auto keys = r;
        const float ww = keys.getWidth() / 7.0f;
        static const int whiteIndex[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
        static const float blackPos[12] = { 0, 1, 0, 2, 0, 0, 4, 0, 5, 0, 6, 0 };
        auto isBlack = [] (int k) { return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; };

        g.setColour (Colour (0xff1a1a19));
        g.fillRect (keys.expanded (2.0f, 2.0f));
        for (int k = 0; k < 12; ++k)
            if (! isBlack (k))
            {
                auto kb = Rectangle<float> (keys.getX() + ww * (float) whiteIndex[k], keys.getY(), ww, keys.getHeight()).reduced (1.0f, 0.0f);
                const bool down = k == active;
                ColourGradient kg (down ? ivory.darker (0.18f) : ivory, kb.getX(), kb.getY(), ivory.darker (down ? 0.3f : 0.1f), kb.getX(), kb.getBottom(), false);
                g.setGradientFill (kg);
                g.fillRoundedRectangle (kb.withTrimmedBottom (down ? 0.0f : 3.0f), 2.5f);
                if (labels)
                    print (g, u (noteNames[k]), kb.removeFromBottom (20.0f), 8.0f, Justification::centred,
                           down ? lamp.darker (0.2f) : Colour (0xff8a8270));
            }
        for (int k = 0; k < 12; ++k)
            if (isBlack (k))
            {
                const float bw = ww * 0.56f;
                auto kb = Rectangle<float> (keys.getX() + ww * blackPos[k] - bw * 0.5f, keys.getY(), bw, keys.getHeight() * 0.6f);
                g.setColour (Colours::black.withAlpha (0.35f));
                g.fillRoundedRectangle (kb.translated (2, 2), 2.0f);
                ColourGradient kg (Colour (0xff3d3935), kb.getX(), kb.getY(), Colour (0xff0b0a09), kb.getX(), kb.getBottom(), false);
                g.setGradientFill (kg);
                g.fillRoundedRectangle (kb, 2.0f);
            }
    }

    void tapeStrip (Graphics& g, Rectangle<float> r, bool lightBezel)
    {
        g.setColour (lightBezel ? Colour (0xff8f8d85) : Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r, 4.0f);
        auto win = r.reduced (4.0f);
        g.setColour (Colour (0xff17120e));
        g.fillRoundedRectangle (win, 2.0f);
        auto wave = win.reduced (10.0f, 6.0f).withTrimmedTop (13.0f);
        const float x0 = wave.getX() + wave.getWidth() * 0.12f, x1 = x0 + wave.getWidth() * 0.55f;
        g.setColour (tape.withAlpha (0.6f));
        g.fillRect (Rectangle<float> (x0, wave.getY(), x1 - x0, wave.getHeight()));
        Random rnd (3);
        float env = 0.3f;
        for (int px = 0; px < (int) wave.getWidth(); px += 2)
        {
            env = jlimit (0.08f, 1.0f, env + (rnd.nextFloat() - 0.48f) * 0.12f);
            const float a = env * wave.getHeight() * 0.48f;
            const float x = wave.getX() + (float) px;
            const bool in = x >= x0 && x <= x1;
            g.setColour (Colour (0xffe8b04a).withAlpha (in ? 0.9f : 0.28f));
            g.drawVerticalLine ((int) x, wave.getCentreY() - a, wave.getCentreY() + a);
        }
        g.setColour (lamp);
        g.drawLine (x0 + (x1 - x0) * 0.4f, wave.getY() - 2, x0 + (x1 - x0) * 0.4f, wave.getBottom() + 2, 1.5f);
        print (g, u ("VOIX_TAKE3.WAV  ·  4,2 s"), win.reduced (10.0f, 3.0f).removeFromTop (12.0f), 9.0f,
               Justification::centredLeft, Colour (0xffb9b5a6));
    }

    //==========================================================================
    // Meccanica: anello di 12 stazioni (capstan + rullo pressore) nel pozzetto,
    // testina e perno tenditore nello spazio libero in alto.
    struct Ring
    {
        Point<float> c;
        float R;
        float capR[12], ang[12];
        Point<float> cap[12], pinch[12];
        Point<float> head, guide1, guide2, guide3, tension;
        float pinchR = 11.0f, tensionR = 13.0f;
        Path tapePath;

        Ring (Point<float> centre, float radius, int active, float scale = 1.0f) : c (centre), R (radius)
        {
            pinchR *= scale; tensionR *= scale;
            for (int k = 0; k < 12; ++k)
            {
                ang[k] = degreesToRadians (34.0f + 292.0f * (float) k / 11.0f);
                capR[k] = 7.5f * scale * std::pow (2.0f, (float) k / 12.0f);
                cap[k] = c.getPointOnCircumference (R, ang[k]);
                const float gap = (k == active) ? 1.5f * scale : 9.0f * scale;
                pinch[k] = c.getPointOnCircumference (R + capR[k] + 3.0f * scale + pinchR + gap, ang[k]);
            }
            guide1  = c.getPointOnCircumference (R + 10.0f * scale, degreesToRadians (342.0f));
            head    = c.getPointOnCircumference (R + 2.0f * scale, degreesToRadians (353.0f));
            guide2  = c.getPointOnCircumference (R + 10.0f * scale, degreesToRadians (18.0f));
            guide3  = c.getPointOnCircumference (R * 0.62f, degreesToRadians (8.0f));
            tension = c.translated (0.0f, -R * 0.12f);

            // percorso del nastro: tangente all'esterno dei capstan (tra capstan e rullo)
            tapePath.startNewSubPath (c.getPointOnCircumference (R + capR[0] + 1.5f * scale, ang[0]));
            for (int k = 1; k < 12; ++k)
                tapePath.lineTo (c.getPointOnCircumference (R + capR[k] + 1.5f * scale, ang[k]));
            tapePath.lineTo (guide1.getPointOnCircumference (5.0f * scale, degreesToRadians (342.0f)));
            tapePath.lineTo (head.getPointOnCircumference (6.0f * scale, degreesToRadians (353.0f)));
            // ansa verso il perno tenditore al centro
            tapePath.lineTo (tension.translated (-tensionR, 0.0f));
            tapePath.addCentredArc (tension.x, tension.y, tensionR, tensionR, 0.0f,
                                    -MathConstants<float>::halfPi, -MathConstants<float>::pi * 1.5f, false);
            tapePath.lineTo (guide3.translated (5.0f * scale, 0.0f));
            tapePath.lineTo (guide2.getPointOnCircumference (5.0f * scale, degreesToRadians (18.0f)));
            tapePath.closeSubPath();
        }
    };

    void tapeMarks (Graphics& g, const Path& p, float phase, float dotR, Colour col, const AffineTransform& t = {})
    {
        const float len = p.getLength();
        for (float d = std::fmod (phase, 22.0f); d < len; d += 22.0f)
        {
            const auto pt = p.getPointAlongPath (d).transformedBy (t);
            g.setColour (col);
            g.fillEllipse (Rectangle<float> (dotR * 2, dotR * 2).withCentre (pt));
        }
        const auto sp = p.getPointAlongPath (std::fmod (phase + 60.0f, len)).transformedBy (t);
        g.setColour (Colour (0xffeeeae0));
        g.fillRect (Rectangle<float> (dotR * 4, dotR * 4).withCentre (sp));
    }

    // vista dall'alto del pozzetto con l'anello
    void ringTopView (Graphics& g, Point<float> c, float wellR, float R, int active, float scale = 1.0f)
    {
        // pozzetto incassato
        auto wb = Rectangle<float> (2 * wellR, 2 * wellR).withCentre (c);
        g.setColour (Colour (0xff2b2b29));
        g.fillEllipse (wb.expanded (4.0f));
        ColourGradient wg (well.darker (0.18f), c.x, c.y - wellR, well.brighter (0.04f), c.x, c.y + wellR * 0.3f, false);
        g.setGradientFill (wg);
        g.fillEllipse (wb);
        ColourGradient inner (Colours::transparentBlack, c.x, c.y, Colours::black.withAlpha (0.16f), c.x + wellR, c.y, true);
        g.setGradientFill (inner);
        g.fillEllipse (wb);

        Ring ring (c, R, active, scale);

        // perni guida e tenditore
        g.setColour (Colour (0xff6d6f6b));
        g.drawLine (Line<float> (c, ring.tension), 5.0f * scale);
        metalTop (g, c, 9.0f * scale);

        // nastro
        g.setColour (Colours::black.withAlpha (0.3f));
        g.strokePath (ring.tapePath, PathStrokeType (4.5f * scale), AffineTransform::translation (1.5f, 2.5f));
        g.setColour (tape);
        g.strokePath (ring.tapePath, PathStrokeType (4.0f * scale));
        tapeMarks (g, ring.tapePath, 7.0f, 1.3f * scale, tapeHi);

        for (int k = 0; k < 12; ++k)
        {
            metalTop (g, ring.cap[k], ring.capR[k]);
            rubberTop (g, ring.pinch[k], ring.pinchR);
            if (k == active)
            {
                g.setColour (Colour (0xffe8b04a).withAlpha (0.8f));
                g.drawEllipse (Rectangle<float> (2 * ring.pinchR + 6, 2 * ring.pinchR + 6).withCentre (ring.pinch[k]), 1.5f);
            }
        }
        metalTop (g, ring.guide1, 5.0f * scale);
        metalTop (g, ring.guide2, 5.0f * scale);
        metalTop (g, ring.guide3, 5.0f * scale);
        metalTop (g, ring.tension, ring.tensionR - 2.0f);

        // testina: blocco con schermo in mu-metal
        {
            Graphics::ScopedSaveState s (g);
            g.addTransform (AffineTransform::rotation (degreesToRadians (353.0f), ring.head.x, ring.head.y));
            auto hb = Rectangle<float> (20.0f * scale, 14.0f * scale).withCentre (ring.head.translated (0.0f, -9.0f * scale));
            g.setColour (Colours::black.withAlpha (0.3f));
            g.fillRoundedRectangle (hb.translated (2, 3), 2.0f);
            ColourGradient hg (Colour (0xfff2f2ef), hb.getX(), hb.getY(), Colour (0xff6e706c), hb.getRight(), hb.getBottom(), false);
            g.setGradientFill (hg);
            g.fillRoundedRectangle (hb, 2.0f);
            g.setColour (Colour (0xff2b2b29));
            g.drawRoundedRectangle (hb, 2.0f, 0.8f);
            g.fillRect (Rectangle<float> (1.2f, 4.0f * scale).withCentre ({ hb.getCentreX(), hb.getBottom() - 2.0f * scale }));
        }
    }

    void bodyBackground (Graphics& g, Rectangle<float> r, Colour base)
    {
        ColourGradient bg (base.brighter (0.06f), 0, r.getY(), base.darker (0.12f), 0, r.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRect (r);
        Random rnd (21);
        for (int i = 0; i < 1400; ++i)
        {
            g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.018f));
            const float x = rnd.nextFloat() * r.getWidth(), y = rnd.nextFloat() * r.getHeight(), s = 1.0f + rnd.nextFloat() * 3.0f;
            g.fillEllipse (x, y, s, s);
        }
    }

    void panel (Graphics& g, Rectangle<float> r, Colour c, float corner = 4.0f)
    {
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (2, 3), corner);
        ColourGradient pg (c.brighter (0.05f), r.getX(), r.getY(), c.darker (0.06f), r.getX(), r.getBottom(), false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (r, corner);
        g.setColour (Colours::white.withAlpha (0.45f));
        g.drawLine (r.getX() + corner, r.getY() + 0.5f, r.getRight() - corner, r.getY() + 0.5f, 1.0f);
        g.setColour (Colours::black.withAlpha (0.3f));
        g.drawRoundedRectangle (r, corner, 1.0f);
    }

    void knobWithLabel (Graphics& g, Point<float> c, float r, float a, const String& label, bool ticks = true)
    {
        blackKnob (g, c, r, a, ticks);
        print (g, label, Rectangle<float> (100, 12).withCentre (c.translated (0, r * 1.45f + 9)), 8.5f);
    }

    void selector (Graphics& g, Point<float> c, float r, int pos, const StringArray& names, const String& label)
    {
        const int n = names.size();
        for (int i = 0; i < n; ++i)
        {
            const float a = -0.75f + 1.5f * (float) i / (float) (n - 1);
            print (g, names[i], Rectangle<float> (70, 11).withCentre (c.getPointOnCircumference (r * 1.75f, a)), 7.5f);
            g.setColour (ink);
            g.drawLine (Line<float> (c.getPointOnCircumference (r * 1.15f, a), c.getPointOnCircumference (r * 1.35f, a)), 1.5f);
        }
        blackKnob (g, c, r, -0.75f + 1.5f * (float) pos / (float) (n - 1), false);
        print (g, label, Rectangle<float> (100, 12).withCentre (c.translated (0, r * 1.4f + 8)), 8.5f);
    }
}

//==============================================================================
// A — VUE DE DESSUS: la macchina vista dall'alto, piastra quadrata con il pozzetto,
// pannello frontale con VU-meter e tastiera come nella foto.
static void drawA (Graphics& g, Rectangle<float> r)
{
    using namespace Art;
    bodyBackground (g, r, enamel);

    // piano superiore del cabinet
    auto top = Rectangle<float> (18, 18, 964, 432);
    panel (g, top, plate, 6.0f);
    for (auto p : { top.getTopLeft().translated (14, 14), top.getTopRight().translated (-14, 14),
                    top.getBottomLeft().translated (14, -14), top.getBottomRight().translated (-14, -14) })
        screw (g, p, 4.0f);

    ringTopView (g, { 250.0f, 234.0f }, 196.0f, 150.0f, 4);

    // puleggia del motore nell'angolo (come nella foto)
    {
        const Point<float> pc { 930.0f, 64.0f };
        metalTop (g, pc, 26.0f);
        for (int i = 0; i < 5; ++i)
        {
            g.setColour (Colour (0xff6f716d));
            g.fillEllipse (Rectangle<float> (8, 8).withCentre (pc.getPointOnCircumference (15.0f, (float) i * 1.2566f)));
        }
    }

    // serigrafia e comandi sul piano
    print (g, u ("PHONOGÈNE"), { 478, 40, 360, 30 }, 24.0f, Justification::centredLeft);
    print (g, u ("À CLAVIER  ·  TYPE CHROMATIQUE  ·  COLLATERAL EFFECTS"), { 478, 70, 400, 14 }, 8.5f,
           Justification::centredLeft, ink.withAlpha (0.7f), false);
    tapeStrip (g, { 478, 100, 488, 66 }, true);

    selector (g, { 540, 250 }, 22, 0, { "CHROM.", "COULISSE", "UNIVERSEL" }, "MODE");
    selector (g, { 660, 250 }, 22, 0, { "FICHIER", u ("ENTRÉE") }, "SOURCE");
    knobWithLabel (g, { 770, 250 }, 20, -0.6f, u ("DÉBUT"));
    knobWithLabel (g, { 870, 250 }, 20, 1.1f, "LONGUEUR");
    pushButton (g, { 478, 346, 110, 42 }, "CHARGER");
    knobWithLabel (g, { 680, 368 }, 20, -0.3f, "INERTIE");
    // la grande manopola nera nell'angolo, come sulla macchina
    blackKnob (g, { 905, 372 }, 30, 0.4f);
    print (g, "NIVEAU", { 855, 412, 100, 12 }, 8.5f);

    // pannello frontale inclinato
    auto front = Rectangle<float> (18, 466, 964, 176);
    panel (g, front, enamel.brighter (0.08f), 5.0f);
    vuMeter (g, { 40, 484, 150, 110 }, 0.62f, "NIV. SORTIE");
    lampJewel (g, { 228, 500 }, 6, true);
    pushButton (g, { 210, 520, 36, 26 }, "");
    print (g, "ENREG.", { 196, 552, 64, 12 }, 8.5f);
    toggle (g, { 290, 520 }, false);  print (g, u ("ÉCOUTE"), { 260, 552, 60, 12 }, 8.5f);
    toggle (g, { 350, 520 }, true);   print (g, "RETOUR", { 320, 552, 60, 12 }, 8.5f);
    toggle (g, { 410, 520 }, false);  print (g, u ("ÉTENDU"), { 380, 552, 60, 12 }, 8.5f);
    knobWithLabel (g, { 250, 600 }, 15, -0.9f, "PLEURAGE");
    knobWithLabel (g, { 340, 600 }, 15, -1.2f, "SOUFFLE");
    knobWithLabel (g, { 430, 600 }, 15, -0.8f, "SATURATION");

    // leva della coulisse orizzontale
    {
        auto slot = Rectangle<float> (480, 534, 140, 8);
        g.setColour (Colour (0xff1c1c1b)); g.fillRoundedRectangle (slot, 4.0f);
        metalTop (g, { 560, 538 }, 10);
        print (g, "COULISSE", { 480, 552, 140, 12 }, 8.5f);
        const std::pair<float, const char*> marks[] = { { 482.f, "0" }, { 560.f, "1" }, { 618.f, "2" } };
        for (auto [x, t] : marks)
            print (g, t, { x - 8, 516, 16, 10 }, 8.0f);
    }

    keyboard (g, { 650, 484, 310, 140 }, 4);
    print (g, "VITESSE 1,260", { 482, 600, 140, 14 }, 9.0f, Justification::centredLeft, ink.withAlpha (0.7f));
}

//==============================================================================
// B — PERSPECTIVE: tre quarti come nella seconda foto; il nastro è un nastro
// "in piedi" che corre tra capstan e rulli, con altezza visibile.
static void drawB (Graphics& g, Rectangle<float> r)
{
    using namespace Art;
    // fondo: parete scura dello studio
    ColourGradient wall (Colour (0xff3a3833), 0, 0, Colour (0xff1c1b19), 0, r.getBottom(), false);
    g.setGradientFill (wall);
    g.fillRect (r);

    const float k = 0.5f;                 // schiacciamento prospettico
    const Point<float> c { 470.0f, 205.0f };
    const float plateW = 900.0f, plateD = 330.0f * 2.0f;

    // piano superiore in prospettiva (trapezio)
    Path topPlate;
    const float yBack = c.y - plateD * 0.5f * k, yFront = c.y + plateD * 0.5f * k;
    topPlate.startNewSubPath (c.x - plateW * 0.46f, yBack);
    topPlate.lineTo (c.x + plateW * 0.46f + 40, yBack);
    topPlate.lineTo (c.x + plateW * 0.5f + 50, yFront);
    topPlate.lineTo (c.x - plateW * 0.5f, yFront);
    topPlate.closeSubPath();
    ColourGradient tg (plate.darker (0.08f), 0, yBack, plate.brighter (0.05f), 0, yFront, false);
    g.setGradientFill (tg);
    g.fillPath (topPlate);
    g.setColour (Colours::white.withAlpha (0.5f));
    g.drawLine (c.x - plateW * 0.5f, yFront, c.x + plateW * 0.5f + 50, yFront, 1.2f);

    // fronte del cabinet con le feritoie di aerazione (prima foto)
    auto face = Rectangle<float> (c.x - plateW * 0.5f, yFront, plateW + 50, 70);
    ColourGradient fg (enamel, 0, face.getY(), enamel.darker (0.25f), 0, face.getBottom(), false);
    g.setGradientFill (fg);
    g.fillRect (face);
    for (int i = 0; i < 46; ++i)
    {
        const float x = face.getX() + 30 + (float) i * 19.0f;
        g.setColour (Colour (0xff3a3936));
        g.fillRoundedRectangle (x, face.getY() + 16, 7, 38, 2.0f);
    }

    // pozzetto (ellisse)
    const float wellR = 230.0f, R = 178.0f;
    auto proj = [&] (Point<float> p) { return Point<float> (p.x, c.y + (p.y - c.y) * k); };
    const auto T = AffineTransform::translation (0, -c.y).scaled (1.0f, k).translated (0, c.y);
    {
        auto wb = Rectangle<float> (2 * wellR, 2 * wellR * k).withCentre (c);
        g.setColour (Colour (0xff222220));
        g.fillEllipse (wb.expanded (5, 4));
        ColourGradient wg (well.darker (0.45f), 0, wb.getY(), well, 0, wb.getBottom(), false);
        g.setGradientFill (wg);
        g.fillEllipse (wb);
        // bordo interno visibile in fondo (spessore del pozzetto)
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillEllipse (wb.withTrimmedBottom (wb.getHeight() * 0.55f).reduced (20, 0));
    }

    Ring ring (c, R, 4, 1.25f);
    const float tapeH = 9.0f, tapeLift = 9.0f;  // altezza del nastro e quota da terra

    struct Obj { float y; std::function<void()> draw; };
    std::vector<Obj> objs;
    for (int i = 0; i < 12; ++i)
    {
        const auto pc = proj (ring.cap[i]);
        const float cr = ring.capR[i];
        objs.push_back ({ pc.y, [&, pc, cr] { cylinder (g, pc, cr, k, 34.0f); } });
        const auto pp = proj (ring.pinch[i]);
        const float prr = ring.pinchR;
        const bool act = i == 4;
        objs.push_back ({ pp.y, [&, pp, prr, act] {
            cylinder (g, pp, prr * 0.85f, k, 18.0f, true);
            cylinder (g, pp.translated (0, -18.0f), prr * 0.32f, k, 4.0f);
            if (act) { g.setColour (Colour (0xffe8b04a).withAlpha (0.85f)); g.drawEllipse (Rectangle<float> (2 * prr + 6, (2 * prr + 6) * k).withCentre (pp.translated (0, -18)), 1.5f); }
        } });
    }
    for (auto gp : { ring.guide1, ring.guide2, ring.guide3 })
    {
        const auto p = proj (gp);
        objs.push_back ({ p.y, [&, p] { cylinder (g, p, 6.0f, k, 28.0f); } });
    }
    { const auto p = proj (ring.tension); objs.push_back ({ p.y, [&, p] { cylinder (g, p, ring.tensionR, k, 30.0f); cylinder (g, p.translated (0, -30), 4, k, 16); } }); }
    {
        const auto p = proj (ring.head);
        objs.push_back ({ p.y, [&, p] {
            auto hb = Rectangle<float> (30, 26).withCentre (p.translated (0, -tapeLift - 10));
            g.setColour (Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (hb.translated (3, 4), 2);
            ColourGradient hg (Colour (0xfff4f4f1), hb.getX(), hb.getY(), Colour (0xff666864), hb.getRight(), hb.getBottom(), false);
            g.setGradientFill (hg); g.fillRoundedRectangle (hb, 2);
            g.setColour (Colour (0xff2a2a28)); g.drawRoundedRectangle (hb, 2, 0.8f);
            g.fillRect (Rectangle<float> (1.5f, 10).withCentre ({ hb.getCentreX(), hb.getBottom() - 5 }));
        } });
    }
    std::sort (objs.begin(), objs.end(), [] (const Obj& a, const Obj& b) { return a.y < b.y; });

    // nastro "in piedi": striscia verticale lungo il percorso proiettato
    auto drawTape = [&] (bool backHalf)
    {
        Graphics::ScopedSaveState s (g);
        if (backHalf) g.reduceClipRegion (Rectangle<int> (0, 0, 1000, (int) c.y));
        else          g.reduceClipRegion (Rectangle<int> (0, (int) c.y, 1000, 700));
        for (float dy = 0; dy <= tapeH; dy += 1.0f)
        {
            const float shade = dy / tapeH;
            g.setColour (tape.interpolatedWith (tapeHi, 0.25f + 0.35f * (1.0f - shade)));
            g.strokePath (ring.tapePath, PathStrokeType (1.4f), T.translated (0, -tapeLift - dy));
        }
        g.setColour (tapeHi.brighter (0.2f));
        g.strokePath (ring.tapePath, PathStrokeType (0.8f), T.translated (0, -tapeLift - tapeH));
    };

    size_t i = 0;
    for (; i < objs.size() && objs[i].y < c.y; ++i) objs[i].draw();
    drawTape (true);
    drawTape (false);
    tapeMarks (g, ring.tapePath, 7.0f, 1.4f, tapeHi.brighter (0.5f), T.translated (0, -tapeLift - tapeH * 0.5f));
    for (; i < objs.size(); ++i) objs[i].draw();

    // puleggia e manopola grande sul piano, come nella foto
    cylinder (g, { 860, 120 }, 30, k, 10);
    blackKnob (g, { 878, 262 }, 26, 0.4f, false);
    print (g, "NIVEAU", { 830, 292, 100, 12 }, 8.5f, Justification::centred, ink);
    print (g, u ("PHONOGÈNE"), { 730, 150, 200, 24 }, 18.0f, Justification::centred);
    print (g, u ("COLLATERAL EFFECTS"), { 730, 174, 200, 12 }, 7.5f, Justification::centred, ink.withAlpha (0.7f));

    // pannello comandi frontale (sotto, come nella foto)
    auto front = Rectangle<float> (40, 430, 920, 212);
    panel (g, front, enamel.brighter (0.06f), 4.0f);
    for (auto p : { front.getTopLeft().translated (10, 10), front.getTopRight().translated (-10, 10),
                    front.getBottomLeft().translated (10, -10), front.getBottomRight().translated (-10, -10) })
        screw (g, p, 3.5f);
    vuMeter (g, { 62, 450, 140, 100 }, 0.6f, "NIV. SORTIE");
    tapeStrip (g, { 62, 566, 380, 58 }, true);
    selector (g, { 262, 486 }, 18, 0, { "CHROM.", "COUL.", "UNIV." }, "MODE");
    selector (g, { 362, 486 }, 18, 0, { "FICH.", u ("ENTR.") }, "SOURCE");
    lampJewel (g, { 458, 466 }, 5, true);
    pushButton (g, { 442, 482, 32, 24 }, "");
    print (g, "ENREG.", { 428, 512, 60, 12 }, 8.0f);
    toggle (g, { 500, 490 }, false); print (g, u ("ÉCOUTE"), { 472, 512, 56, 12 }, 8.0f);
    pushButton (g, { 456, 568, 90, 36 }, "CHARGER");
    knobWithLabel (g, { 590, 476 }, 14, -0.6f, u ("DÉBUT"));
    knobWithLabel (g, { 590, 560 }, 14, 1.1f, "LONG.");
    knobWithLabel (g, { 650, 476 }, 14, -0.3f, "INERTIE");
    knobWithLabel (g, { 650, 560 }, 14, -0.9f, "PLEURAGE");
    knobWithLabel (g, { 710, 476 }, 14, -1.2f, "SOUFFLE");
    knobWithLabel (g, { 710, 560 }, 14, -0.8f, "SATUR.");
    toggle (g, { 545, 490 }, true); print (g, "RETOUR", { 517, 512, 56, 12 }, 8.0f);
    keyboard (g, { 752, 456, 190, 150 }, 4, false);
    print (g, u ("COULISSE  ·  pitch bend"), { 752, 612, 190, 12 }, 8.0f, Justification::centred, ink.withAlpha (0.7f), false);
}

//==============================================================================
// C — CONSOLE DE STUDIO: come nella prima foto, il rack con VU-meter e file di
// interruttori sopra, la macchina con il pozzetto e la pedana con la tastiera.
static void drawC (Graphics& g, Rectangle<float> r)
{
    using namespace Art;
    const Colour cream { 0xffe6e3d9 };
    bodyBackground (g, r, Colour (0xff9c9a92));

    // rack superiore (sinistra in alto)
    auto rack = Rectangle<float> (18, 18, 476, 300);
    panel (g, rack, Colour (0xffd9d6cc), 3.0f);
    auto inner = rack.reduced (12.0f);
    panel (g, inner, cream, 2.0f);
    vuMeter (g, { 186, 40, 150, 104 }, 0.62f, "NIV. SORTIE");
    // file di piccoli interruttori (come la riga di tasti nella foto)
    print (g, u ("PHONOGÈNE"), { 46, 44, 130, 22 }, 15.0f, Justification::centredLeft);
    print (g, u ("COLLATERAL EFFECTS"), { 46, 66, 130, 12 }, 7.0f, Justification::centredLeft, ink.withAlpha (0.7f));
    lampJewel (g, { 60, 112 }, 5, true, Colour (0xff3fb34a));
    print (g, "SECTEUR", { 34, 124, 52, 10 }, 7.0f);
    selector (g, { 400, 82 }, 18, 0, { "CHROM.", "COUL.", "UNIV." }, "MODE");
    // spie dei dodici capstan: si accende quella innestata
    for (int i = 0; i < 12; ++i)
    {
        const float x = 60.0f + (float) i * 34.0f;
        lampJewel (g, { x, 170.0f }, 5.0f, i == 4, Colour (0xffe8b04a));
        print (g, u (noteNames[i]), { x - 17.0f, 182.0f, 34.0f, 10.0f }, 7.0f);
    }
    print (g, "CABESTANS", { 46, 200, 420, 10 }, 7.0f, Justification::centredLeft, ink.withAlpha (0.6f));
    tapeStrip (g, { 40, 220, 300, 58 }, false);
    pushButton (g, { 352, 224, 120, 50 }, "CHARGER");
    for (auto p : { inner.getTopLeft().translated (8, 8), inner.getTopRight().translated (-8, 8),
                    inner.getBottomLeft().translated (8, -8), inner.getBottomRight().translated (-8, -8) })
        screw (g, p, 3.0f);

    // macchina vista dall'alto (destra)
    auto deck = Rectangle<float> (512, 18, 470, 470);
    panel (g, deck, plate, 5.0f);
    for (auto p : { deck.getTopLeft().translated (14, 14), deck.getTopRight().translated (-14, 14),
                    deck.getBottomLeft().translated (14, -14), deck.getBottomRight().translated (-14, -14) })
        screw (g, p, 4.0f);
    ringTopView (g, deck.getCentre(), 212.0f, 166.0f, 4, 1.05f);
    blackKnob (g, deck.getBottomRight().translated (-38, -40), 22, 0.4f, false);

    // fronte del cabinet con feritoie, sotto la macchina
    auto face = Rectangle<float> (512, 488, 470, 154);
    ColourGradient fcg (enamel, 0, face.getY(), enamel.darker (0.22f), 0, face.getBottom(), false);
    g.setGradientFill (fcg);
    g.fillRect (face);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawRect (face, 1.0f);
    for (int row = 0; row < 2; ++row)
        for (int i = 0; i < 22; ++i)
        {
            g.setColour (Colour (0xff3a3936));
            g.fillRoundedRectangle (face.getX() + 22 + (float) i * 20.0f, face.getY() + 18 + (float) row * 62.0f, 7, 46, 2.5f);
        }
    print (g, "VITESSE 1,260", { 530, 620, 200, 14 }, 9.0f, Justification::centredLeft, ink.withAlpha (0.7f));

    // pedana dei comandi (sinistra in basso)
    auto desk = Rectangle<float> (18, 334, 476, 308);
    panel (g, desk, cream, 3.0f);
    selector (g, { 70, 380 }, 18, 0, { "FICH.", u ("ENTR.") }, "SOURCE");
    lampJewel (g, { 156, 362 }, 5, true);
    pushButton (g, { 140, 376, 32, 24 }, "");
    print (g, "ENREG.", { 126, 404, 60, 12 }, 8.0f);
    toggle (g, { 214, 386 }, false); print (g, u ("ÉCOUTE"), { 186, 404, 56, 12 }, 8.0f);
    toggle (g, { 268, 386 }, true);  print (g, "RETOUR", { 240, 404, 56, 12 }, 8.0f);
    toggle (g, { 322, 386 }, false); print (g, u ("ÉTENDU"), { 294, 404, 56, 12 }, 8.0f);
    knobWithLabel (g, { 410, 384 }, 22, 0.4f, "NIVEAU");
    knobWithLabel (g, { 64, 470 }, 15, -0.6f, u ("DÉBUT"));
    knobWithLabel (g, { 134, 470 }, 15, 1.1f, "LONGUEUR");
    knobWithLabel (g, { 204, 470 }, 15, -0.3f, "INERTIE");
    knobWithLabel (g, { 274, 470 }, 15, -0.9f, "PLEURAGE");
    knobWithLabel (g, { 344, 470 }, 15, -1.2f, "SOUFFLE");
    knobWithLabel (g, { 414, 470 }, 15, -0.8f, "SATUR.");
    // coulisse verticale e tastiera, come la pedana della foto
    {
        auto slot = Rectangle<float> (60, 528, 8, 96);
        g.setColour (Colour (0xff1c1c1b)); g.fillRoundedRectangle (slot, 4.0f);
        metalTop (g, { 64, 566 }, 10);
        print (g, "COULISSE", { 84, 600, 80, 12 }, 8.0f, Justification::centredLeft);
    }
    keyboard (g, { 176, 520, 300, 110 }, 4);
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File outDir (argc > 1 ? argv[1] : File::getCurrentWorkingDirectory().getFullPathName());

    auto render = [&] (const char* name, std::function<void (Graphics&, Rectangle<float>)> fn)
    {
        const float s = 1.5f;
        Image img (Image::RGB, (int) (1000 * s), (int) (660 * s), true);
        Graphics g (img);
        g.addTransform (AffineTransform::scale (s));
        fn (g, { 0, 0, 1000, 660 });
        const auto f = outDir.getChildFile (name);
        f.deleteFile();
        if (auto os = f.createOutputStream())
            PNGImageFormat().writeImageToStream (img, *os);
        std::cout << f.getFullPathName() << "\n";
    };

    render ("proposta_A_vista_dallalto.png", drawA);
    render ("proposta_B_prospettiva.png", drawB);
    render ("proposta_C_console.png", drawC);
    return 0;
}
