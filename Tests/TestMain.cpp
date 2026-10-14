// Verifica offline del prototipo: altezze, inviluppo, registrazione dal sidechain, istantanea dell'interfaccia.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

using namespace juce;

static void writeSine (const File& f, double freq, double sr, double secs)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<OutputStream> os (f.createOutputStream().release());
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    os.release();
    AudioBuffer<float> b (2, (int) (sr * secs));
    for (int i = 0; i < b.getNumSamples(); ++i)
        b.setSample (0, i, 0.5f * (float) std::sin (MathConstants<double>::twoPi * freq * i / sr)),
        b.setSample (1, i, b.getSample (0, i));
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

struct Runner
{
    PhonogeneProcessor& p;
    double sr;
    int block = 512;
    std::vector<float> captured;

    void run (double secs, std::function<void (AudioBuffer<float>&, int)> fillInput = {}, MidiBuffer first = {})
    {
        const int total = (int) (secs * sr);
        captured.clear();
        AudioBuffer<float> buf (jmax (2, p.getTotalNumInputChannels()), block);
        int done = 0;
        bool firstBlock = true;
        while (done < total)
        {
            buf.clear();
            if (fillInput) fillInput (buf, done);
            MidiBuffer m;
            if (firstBlock) m = first;
            firstBlock = false;
            p.processBlock (buf, m);
            for (int i = 0; i < block; ++i) captured.push_back (buf.getSample (0, i));
            done += block;
        }
    }

    double freqOfLast (double secs) const
    {
        const int n = (int) (secs * sr);
        const int s0 = (int) captured.size() - n;
        int crossings = 0;
        for (int i = s0 + 1; i < (int) captured.size(); ++i)
            if (captured[(size_t) i - 1] <= 0.0f && captured[(size_t) i] > 0.0f) ++crossings;
        return crossings / secs;
    }

    float rmsOfLast (double secs) const
    {
        const int n = (int) (secs * sr);
        double acc = 0;
        for (int i = (int) captured.size() - n; i < (int) captured.size(); ++i) acc += captured[(size_t) i] * captured[(size_t) i];
        return (float) std::sqrt (acc / n);
    }
};

static MidiBuffer note (int n, bool on)
{
    MidiBuffer m;
    m.addEvent (on ? MidiMessage::noteOn (1, n, 1.0f) : MidiMessage::noteOff (1, n), 0);
    return m;
}

static void setParam (PhonogeneProcessor& p, const String& id, float plainValue)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File outDir (argc > 1 ? argv[1] : File::getCurrentWorkingDirectory().getFullPathName());
    const double sr = 48000.0;
    int failures = 0;

    auto check = [&] (const String& what, double got, double expected, double tol)
    {
        const bool ok = std::abs (got - expected) <= tol;
        std::cout << (ok ? "OK   " : "FAIL ") << what << ": " << got << " (atteso " << expected << ")\n";
        if (! ok) ++failures;
    };

    PhonogeneProcessor p;
    p.prepareToPlay (sr, 512);
    setParam (p, "hiss", 0.0f);
    setParam (p, "wow", 0.0f);
    setParam (p, "drive", 0.0f);

    const File wav = outDir.getChildFile ("test440.wav");
    writeSine (wav, 440.0, 44100.0, 2.0); // frequenza diversa: verifica anche il ricampionamento
    std::cout << "loadFile: " << p.loadFile (wav) << "\n";

    Runner r { p, sr };
    r.run (0.05);

    // tasto DO = velocità nominale
    r.run (1.0, {}, note (60, true));
    check ("DO  (Hz)", r.freqOfLast (0.5), 440.0, 4.0);

    // SOL = +7 semitoni (legato: il trasporto cambia capstan)
    r.run (1.0, {}, note (67, true));
    check ("SOL (Hz)", r.freqOfLast (0.5), 440.0 * std::pow (2.0, 7.0 / 12.0), 5.0);

    // rilascio: il nastro si ferma e il suono svanisce
    r.run (0.4, {}, [] { MidiBuffer m; m.addEvent (MidiMessage::noteOff (1, 60), 0); m.addEvent (MidiMessage::noteOff (1, 67), 0); return m; }());
    check ("RMS dopo il rilascio", r.rmsOfLast (0.1), 0.0, 0.001);

    // ottava superiore ripiegata nell'ottava (tastiera di un'ottava)
    r.run (1.0, {}, note (72, true));
    check ("DO+1 ripiegato (Hz)", r.freqOfLast (0.5), 440.0, 4.0);
    r.run (0.4, {}, note (72, false));

    // registrazione dal sidechain
    setParam (p, "source", 1.0f);
    setParam (p, "record", 1.0f);
    auto sine220 = [&] (AudioBuffer<float>& b, int offset)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = 0.5f * (float) std::sin (MathConstants<double>::twoPi * 220.0 * (offset + i) / sr);
            for (int ch = 0; ch < b.getNumChannels(); ++ch) b.setSample (ch, i, v);
        }
    };
    r.run (1.5, sine220);
    setParam (p, "record", 0.0f);
    r.run (0.05);
    check ("secondi registrati", p.engine.display.recSeconds.load(), 1.5, 0.02);

    r.run (1.0, {}, note (64, true)); // MI = +4
    check ("MI dal nastro registrato (Hz)", r.freqOfLast (0.5), 220.0 * std::pow (2.0, 4.0 / 12.0), 4.0);

    // modalità a coulisse: la nota apre il trasporto, la velocità è la leva
    setParam (p, "mode", 1.0f);
    setParam (p, "coulisse", 0.5f);
    r.run (1.0);
    check ("coulisse 0,5 (Hz)", r.freqOfLast (0.5), 110.0, 3.0);
    setParam (p, "mode", 0.0f);

    // istantanea dell'interfaccia: file caricato, tasto MI premuto
    setParam (p, "source", 0.0f);
    setParam (p, "loopStart", 0.12f);
    setParam (p, "loopLength", 0.55f);
    setParam (p, "hiss", 0.2f);
    setParam (p, "wow", 0.25f);
    r.run (0.6);

    std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (1000, 740);
    for (int i = 0; i < 20; ++i)
        if (auto* deck = dynamic_cast<PhonogeneEditor*> (ed.get())) { (void) deck; }

    auto snap = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    const File png = outDir.getChildFile ("phonogene_ui.png");
    png.deleteFile();
    if (auto os = png.createOutputStream())
        PNGImageFormat().writeImageToStream (snap, *os);
    std::cout << "snapshot: " << png.getFullPathName() << "\n";

    ed.reset();
    std::cout << (failures == 0 ? "TUTTI I TEST SUPERATI" : "TEST FALLITI: " + String (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
