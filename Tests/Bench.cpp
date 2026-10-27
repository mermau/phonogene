// Benchmark: costo del DSP per modalità e velocità, disegno dell'interfaccia, salvataggio dello stato.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>
#include <chrono>

using namespace juce;
using Clock = std::chrono::steady_clock;

static void setParam (PhonogeneProcessor& p, const String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File dir (argc > 1 ? argv[1] : File::getCurrentWorkingDirectory().getFullPathName());
    const double sr = 48000.0;
    const int block = 256;

    // nastro di prova: rumore rosa approssimato, 10 s
    const File wav = dir.getChildFile ("bench.wav");
    {
        WavAudioFormat fmt;
        wav.deleteFile();
        auto os = wav.createOutputStream();
        std::unique_ptr<AudioFormatWriter> w (fmt.createWriterFor (os.get(), sr, 2, 24, {}, 0));
        os.release();
        AudioBuffer<float> b (2, (int) (sr * 10));
        Random r (1); float y = 0;
        for (int i = 0; i < b.getNumSamples(); ++i) { y = 0.98f * y + 0.1f * (r.nextFloat() - 0.5f); b.setSample (0, i, y); b.setSample (1, i, y); }
        w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }

    PhonogeneProcessor p;
    p.prepareToPlay (sr, block);
    p.loadFile (wav);

    auto run = [&] (const char* label, int mode, int note, float lever, bool ext)
    {
        setParam (p, "mode", (float) mode);
        setParam (p, "coulisse", lever);
        setParam (p, "extended", ext ? 1.0f : 0.0f);
        AudioBuffer<float> buf (4, block);
        MidiBuffer m;
        m.addEvent (MidiMessage::noteOn (1, note, 1.0f), 0);
        buf.clear();
        p.processBlock (buf, m);
        const double secs = 10.0;
        const int n = (int) (secs * sr / block);
        const auto t0 = Clock::now();
        for (int i = 0; i < n; ++i)
        {
            buf.clear();
            MidiBuffer none;
            p.processBlock (buf, none);
        }
        const double el = std::chrono::duration<double> (Clock::now() - t0).count();
        MidiBuffer off; off.addEvent (MidiMessage::noteOff (1, note), 0);
        p.processBlock (buf, off);
        for (int i = 0; i < 50; ++i) { MidiBuffer none; p.processBlock (buf, none); }
        std::cout << String (label).paddedRight (' ', 40) << String (100.0 * el / secs, 2) << " % CPU (un core)\n";
    };

    std::cout << "--- DSP, 48 kHz, blocchi da " << block << " ---\n";
    run ("cromatico DO (1x)", 0, 60, 1.0f, false);
    run ("cromatico SI (1.89x)", 0, 71, 1.0f, false);
    run ("cromatico esteso +2 ott. (4x)", 0, 84, 1.0f, true);
    run ("coulisse 0.5x", 1, 60, 0.5f, false);
    run ("coulisse 2x", 1, 60, 2.0f, false);
    run ("universale SOL, nastro 1x", 2, 67, 1.0f, false);
    run ("universale SOL, nastro 0.1x", 2, 67, 0.1f, false);
    run ("universale +2 ott. esteso", 2, 84, 1.0f, true);

    // stato: con 30 s di registrazione
    {
        setParam (p, "source", 1.0f);
        setParam (p, "record", 1.0f);
        AudioBuffer<float> buf (4, block);
        Random r (2);
        for (int i = 0; i < (int) (30.0 * sr / block); ++i)
        {
            for (int ch = 0; ch < 4; ++ch) for (int s = 0; s < block; ++s) buf.setSample (ch, s, 0.2f * (r.nextFloat() - 0.5f));
            MidiBuffer none; p.processBlock (buf, none);
        }
        setParam (p, "record", 0.0f);
        MidiBuffer none; p.processBlock (buf, none);
        MemoryBlock mb;
        const auto t0 = Clock::now();
        p.getStateInformation (mb);
        const double el = std::chrono::duration<double> (Clock::now() - t0).count();
        MemoryBlock mb2;
        const auto t2 = Clock::now();
        p.getStateInformation (mb2);
        const double el2 = std::chrono::duration<double> (Clock::now() - t2).count();
        std::cout << "--- salvataggio successivo (nastro invariato): " << String (el2 * 1000.0, 1) << " ms\n";
        std::cout << "--- stato con 30 s registrati: " << mb.getSize() / 1024 << " KB, " << String (el * 1000.0, 1) << " ms\n";
        setParam (p, "source", 0.0f);
    }

    // interfaccia: tempo di un fotogramma (deck + nastro + tastiera)
    {
        std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (1000, 740);
        Image img (Image::ARGB, 1000, 740, true);
        for (int i = 0; i < 3; ++i) { Graphics g (img); ed->paintEntireComponent (g, true); }
        const int frames = 60;
        const auto t0 = Clock::now();
        for (int i = 0; i < frames; ++i) { Graphics g (img); ed->paintEntireComponent (g, true); }
        const double el = std::chrono::duration<double> (Clock::now() - t0).count() / frames;
        for (auto* c : ed->getChildren())
        {
            if (c->getWidth() < 300) continue;   // solo i componenti grandi (deck, nastro, tastiera)
            Image ci (Image::ARGB, c->getWidth(), c->getHeight(), true);
            const auto t1 = Clock::now();
            for (int i = 0; i < frames; ++i) { Graphics g (ci); c->paintEntireComponent (g, true); }
            const double e1 = std::chrono::duration<double> (Clock::now() - t1).count() / frames;
            std::cout << "    componente " << c->getWidth() << "x" << c->getHeight() << ": " << String (e1 * 1000.0, 2) << " ms\n";
        }
        std::cout << "--- interfaccia: " << String (el * 1000.0, 2) << " ms per fotogramma completo (a 30 fps = "
                  << String (el * 30.0 * 100.0, 1) << " % del thread grafico)\n";
    }
    return 0;
}
