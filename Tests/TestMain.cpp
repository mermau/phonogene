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

static void writeStereo (const File& f, double sr, double secs, std::function<float (int, int)> gen)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<OutputStream> os (f.createOutputStream().release());
    std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    os.release();
    AudioBuffer<float> b (2, (int) (sr * secs));
    for (int i = 0; i < b.getNumSamples(); ++i)
        for (int ch = 0; ch < 2; ++ch) b.setSample (ch, i, gen (ch, i));
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

    // altezza per autocorrelazione: robusta ai raccordi di fase tra le testine rotanti
    double pitchOfLast (double secs, double fMin = 60.0, double fMax = 1200.0) const
    {
        const int n = jmin ((int) (secs * sr), (int) captured.size());
        const float* x = captured.data() + captured.size() - (size_t) n;
        const int lagMin = (int) (sr / fMax), lagMax = (int) (sr / fMin);
        std::vector<double> ac ((size_t) lagMax + 2, 0.0);
        for (int lag = lagMin - 1; lag <= lagMax + 1; ++lag)
        {
            double acc = 0, e1 = 0, e2 = 0;
            for (int i = 0; i + lag < n; ++i) { acc += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
            ac[(size_t) lag] = acc / std::sqrt (e1 * e2 + 1e-12);
        }
        // primo massimo locale sopra il 90% del massimo globale (evita i sottomultipli)
        double best = 0; for (int lag = lagMin; lag <= lagMax; ++lag) best = jmax (best, ac[(size_t) lag]);
        for (int lag = lagMin; lag <= lagMax; ++lag)
            if (ac[(size_t) lag] >= 0.9 * best && ac[(size_t) lag] >= ac[(size_t) lag - 1] && ac[(size_t) lag] >= ac[(size_t) lag + 1])
            {
                const double a = ac[(size_t) lag - 1], b = ac[(size_t) lag], c = ac[(size_t) lag + 1];
                const double off = 0.5 * (a - c) / (a - 2 * b + c + 1e-12);
                return sr / (lag + off);
            }
        return 0.0;
    }

    // picco spettrale (DFT con finestra di Hann) in una banda: misura la portante,
    // non le bande laterali prodotte dai raccordi tra le testine
    double spectralPeak (double secs, double f0, double f1, double step = 0.5) const
    {
        const int n = jmin ((int) (secs * sr), (int) captured.size());
        const float* x = captured.data() + captured.size() - (size_t) n;
        double bestF = f0, bestP = -1.0;
        for (double f = f0; f <= f1; f += step)
        {
            double re = 0, im = 0;
            const double w = MathConstants<double>::twoPi * f / sr;
            for (int i = 0; i < n; ++i)
            {
                const double win = 0.5 - 0.5 * std::cos (MathConstants<double>::twoPi * i / (n - 1));
                re += x[i] * win * std::cos (w * i);
                im -= x[i] * win * std::sin (w * i);
            }
            const double pw = re * re + im * im;
            if (pw > bestP) { bestP = pw; bestF = f; }
        }
        return bestF;
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

    // anti-aliasing: una sinusoide a 15 kHz letta a 2x (30 kHz) non deve ripiegarsi a 18 kHz
    {
        const File hf = outDir.getChildFile ("test15k.wav");
        writeSine (hf, 15000.0, 48000.0, 2.0);
        p.loadFile (hf);
        r.run (0.05);
        setParam (p, "extended", 1.0f);
        r.run (0.6, {}, note (60, true));
        const float rms1 = r.rmsOfLast (0.3);
        r.run (0.3, {}, note (60, false));
        r.run (0.6, {}, note (72, true));                 // C4 con ÉTENDU: velocità 2x
        const float rms2 = r.rmsOfLast (0.3);
        r.run (0.3, {}, note (72, false));
        const double db = 20.0 * std::log10 ((rms2 + 1e-9) / rms1);
        std::cout << "15 kHz a 1x: " << rms1 << "   a 2x: " << rms2 << " (" << db << " dB)\n";
        check ("residuo di aliasing a 2x sotto -40 dB", db < -40.0 ? 1.0 : 0.0, 1.0, 0.0);
        setParam (p, "extended", 0.0f);
        p.housekeeping();
        p.loadFile (wav);
        r.run (0.05);
        p.housekeeping();
    }

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
    // coulisse: inerzia del motore (la leva non cambia la velocità di colpo)
    setParam (p, "coulisse", 1.0f);
    r.run (0.6);
    const float rmsAt1 = r.rmsOfLast (0.3);
    setParam (p, "coulisse", 0.5f);
    r.run (0.03);
    check ("motore dopo 30 ms (tra 0,5 e 1)", p.engine.display.motorSpeed.load(), 0.75, 0.2);
    r.run (0.6);
    check ("motore a regime", p.engine.display.motorSpeed.load(), 0.5, 0.01);

    // verso l'arresto: il livello cala, il suono resta
    setParam (p, "coulisse", 0.1f);
    r.run (1.0);
    const float rmsLow = r.rmsOfLast (0.3);
    std::cout << "RMS a 1x: " << rmsAt1 << "   a 0,1x: " << rmsLow << "\n";
    // -16 dB dal calo con la velocità, più la perdita dei bassi: 220 Hz diventano 22 Hz
    check ("calo di livello a 0,1x (dB)", 20.0 * std::log10 (rmsLow / rmsAt1), -22.0, 6.0);
    check ("velocita del nastro a 0,1x", p.engine.display.speed.load(), 0.1, 0.01);
    setParam (p, "coulisse", 1.0f);
    setParam (p, "mode", 0.0f);

    // CC MIDI: assegnazione predefinita e MIDI learn
    {
        MidiBuffer m;
        m.addEvent (MidiMessage::controllerEvent (1, 74, 64), 0);   // DÉBUT
        m.addEvent (MidiMessage::controllerEvent (1, 27, 127), 0);  // MODE -> universel
        r.run (0.02, {}, m);
        p.applyPendingCCs();
        check ("CC74 -> DEBUT", p.apvts.getRawParameterValue ("loopStart")->load(), 64.0 / 127.0, 0.01);
        check ("CC27 -> MODE", p.apvts.getRawParameterValue ("mode")->load(), 2.0, 0.0);

        const int hissIdx = p.ccIndexForParam ("hiss");
        p.startLearn (hissIdx);
        MidiBuffer l; l.addEvent (MidiMessage::controllerEvent (1, 90, 127), 0);
        r.run (0.02, {}, l);
        p.applyPendingCCs();
        check ("learn: SOUFFLE su CC90", p.getAssignedCC (hissIdx), 90, 0);
        check ("learn: valore applicato", p.apvts.getRawParameterValue ("hiss")->load(), 1.0, 0.001);
        p.resetDefaultCCs();
        setParam (p, "loopStart", 0.0f);
        setParam (p, "mode", 0.0f);
        setParam (p, "hiss", 0.0f);
        r.run (0.05);
    }

    // nastro registrato: esportazione e ripristino dallo stato del plugin
    {
        const File wavOut = outDir.getChildFile ("export.wav"), aifOut = outDir.getChildFile ("export.aiff");
        check ("esportazione WAV", p.exportRecording (wavOut) ? 1.0 : 0.0, 1.0, 0.0);
        check ("esportazione AIFF", p.exportRecording (aifOut) ? 1.0 : 0.0, 1.0, 0.0);
        AudioFormatManager fm; fm.registerBasicFormats();
        for (auto& f : { wavOut, aifOut })
        {
            std::unique_ptr<AudioFormatReader> rd (fm.createReaderFor (f));
            check ("  durata " + f.getFileExtension(), rd != nullptr ? (double) rd->lengthInSamples / rd->sampleRate : 0.0, 1.5, 0.02);
        }

        MemoryBlock state;
        p.getStateInformation (state);
        std::cout << "stato del plugin: " << state.getSize() / 1024 << " KB\n";

        // nuovo plugin a 44,1 kHz: il nastro va ripristinato e ricampionato
        PhonogeneProcessor q;
        q.setStateInformation (state.getData(), (int) state.getSize());
        q.prepareToPlay (44100.0, 512);
        Runner rq { q, 44100.0 };
        rq.run (0.05);
        q.housekeeping();
        check ("ripristino: secondi del nastro", q.engine.display.recSeconds.load(), 1.5, 0.02);
        setParam (q, "source", 1.0f);
        setParam (q, "hiss", 0.0f);
        setParam (q, "wow", 0.0f);
        rq.run (1.0, {}, note (60, true));
        check ("ripristino: il nastro suona a 220 Hz", rq.freqOfLast (0.5), 220.0, 3.0);

        // cambio di frequenza della sessione: il nastro non si perde
        q.prepareToPlay (48000.0, 512);
        Runner rq2 { q, 48000.0 };
        rq2.run (0.05);
        q.housekeeping();
        check ("cambio frequenza: nastro conservato", q.engine.display.recSeconds.load(), 1.5, 0.02);
        rq2.run (1.0, {}, note (60, true));
        check ("cambio frequenza: ancora 220 Hz", rq2.freqOfLast (0.5), 220.0, 3.0);
    }

    // universale: altezza dalla tastiera, durata dalla leva, indipendenti
    {
        r.run (0.3, {}, note (64, false));   // rilascia il MI tenuto dalle prove precedenti
        setParam (p, "mode", 2.0f);
        setParam (p, "hiss", 0.0f);
        setParam (p, "wow", 0.0f);
        const double sol = 220.0 * std::pow (2.0, 7.0 / 12.0);
        for (float lever : { 1.0f, 0.25f, 0.02f, 2.0f })
        {
            setParam (p, "coulisse", lever);
            r.run (1.0, {}, note (67, true));
            check ("universale SOL, nastro " + String (lever, 2) + " (Hz)", r.spectralPeak (0.5, 250.0, 420.0), sol, 8.0);
            check ("  velocita del nastro", p.engine.display.speed.load(), lever, lever * 0.02 + 0.001);
        }
        // con FENÊTRE pari a un numero intero di periodi il salto tra testine non sposta la fase:
        // la portante coincide esattamente con l'altezza voluta
        setParam (p, "window", 13.0f / 220.0f * 1000.0f);
        setParam (p, "coulisse", 0.3f);
        r.run (1.0);
        check ("universale, finestra commensurata (Hz)", r.spectralPeak (0.5, 250.0, 420.0, 0.25), sol, 0.6);
        setParam (p, "window", 60.0f);
        // comandi del tamburo girati durante la riproduzione: nessun crepitio
        {
            // i crepitii sono discontinuità: energia a larga banda sopra 6 kHz,
            // mentre il segnale (un SOL e i suoi glissandi) resta sotto 1 kHz
            auto maxStep = [&] (const std::vector<float>& x, int from)
            {
                IIRFilter h1, h2, h3;
                const auto c = IIRCoefficients::makeHighPass (sr, 6000.0, 0.7);
                h1.setCoefficients (c); h2.setCoefficients (c); h3.setCoefficients (c);
                float m = 0.0f;
                for (size_t i = 0; i < x.size(); ++i)
                {
                    const float y = h3.processSingleSampleRaw (h2.processSingleSampleRaw (h1.processSingleSampleRaw (x[i])));
                    if ((int) i >= from + 256) m = jmax (m, std::abs (y));
                }
                return m;
            };
            // riferimento: i raccordi delle testine producono già un po' di energia acuta, che dipende
            // dalle impostazioni; si prende il caso peggiore con i comandi fermi agli estremi
            setParam (p, "coulisse", 0.5f);
            float steady = 0.0f;
            for (float w : { 10.0f, 250.0f })
                for (float h : { 0.0f, 0.6f })
                    for (float n : { 0.0f, 2.0f })
                    {
                        setParam (p, "window", w); setParam (p, "hardness", h); setParam (p, "heads", n);
                        r.run (0.5);
                        steady = jmax (steady, maxStep (r.captured, (int) (0.25 * sr)));
                    }

            // FENÊTRE e DURETÉ mosse a scatti ogni blocco (~11 ms), come girando le manopole
            Random rnd (5);
            std::vector<float> all;
            for (int b = 0; b < 60; ++b)
            {
                setParam (p, "window", 10.0f + rnd.nextFloat() * 240.0f);
                setParam (p, "hardness", rnd.nextFloat() * 0.6f);
                r.run (512.0 / sr);
                all.insert (all.end(), r.captured.begin(), r.captured.end());
            }
            setParam (p, "heads", 1.0f);   // cambio del numero di testine durante la nota
            r.run (0.2);
            all.insert (all.end(), r.captured.begin(), r.captured.end());
            const float moving = maxStep (all, 0);
            std::cout << "salto massimo: fermo " << steady << "  manopole mosse " << moving << "\n";
            check ("nessun crepitio muovendo FENETRE/DURETE/TETES (picchi sopra 6 kHz < 2x)", moving / steady < 2.0f ? 1.0 : 0.0, 1.0, 0.0);
            setParam (p, "window", 60.0f);
            setParam (p, "hardness", 0.5f);
            setParam (p, "heads", 2.0f);
            r.run (0.3);
        }

        setParam (p, "heads", 0.0f);      // 2 testine
        setParam (p, "hardness", 1.0f);   // commutazione secca
        setParam (p, "coulisse", 0.5f);
        r.run (1.0);
        check ("universale 2 testine dure (Hz)", r.spectralPeak (0.5, 250.0, 420.0), sol, 8.0);
        r.run (0.4, {}, note (67, false));
        check ("universale: silenzio al rilascio", r.rmsOfLast (0.1), 0.0, 0.001);
        setParam (p, "heads", 2.0f);
        setParam (p, "hardness", 0.5f);
        setParam (p, "coulisse", 1.0f);
        setParam (p, "mode", 0.0f);
        r.run (0.1);
    }

    // ---- migliorie di fedeltà ----
    {
        r.run (0.3, {}, [] { MidiBuffer m; for (int n = 0; n < 128; ++n) m.addEvent (MidiMessage::noteOff (1, n), 0); return m; }());
        setParam (p, "source", 0.0f);
        setParam (p, "mode", 0.0f);
        setParam (p, "hiss", 0.0f);
        setParam (p, "wow", 0.0f);
        setParam (p, "extended", 0.0f);
        p.housekeeping();
        p.loadFile (wav);
        r.run (0.05);
        p.housekeeping();

        // OCTAVE: seconda ottava della tastiera
        setParam (p, "octave", 1.0f);
        r.run (1.0, {}, note (60, true));
        check ("OCTAVE cromatico: DO a 2x (Hz)", r.freqOfLast (0.5), 880.0, 8.0);
        r.run (0.4, {}, note (60, false));
        setParam (p, "mode", 2.0f);
        setParam (p, "coulisse", 0.5f);
        r.run (1.0, {}, note (67, true));
        const double sol2 = 2.0 * 440.0 * std::pow (2.0, 7.0 / 12.0);
        check ("OCTAVE universale: SOL un'ottava sopra (Hz)", r.spectralPeak (0.5, 1150.0, 1500.0), sol2, sol2 * 0.02);
        r.run (0.4, {}, note (67, false));
        setParam (p, "octave", 0.0f);
        setParam (p, "mode", 0.0f);
        setParam (p, "coulisse", 1.0f);

        // MONO: nastro con canali diversi -> uscite identiche
        const File st = outDir.getChildFile ("stereo.wav");
        writeStereo (st, 48000.0, 1.0, [] (int ch, int i) { return ch == 0 ? 0.5f * (float) std::sin (0.0576 * i) : 0.0f; });
        p.housekeeping(); p.loadFile (st); r.run (0.05); p.housekeeping();
        setParam (p, "mono", 1.0f);
        {
            AudioBuffer<float> buf (jmax (2, p.getTotalNumInputChannels()), 512);
            MidiBuffer on; on.addEvent (MidiMessage::noteOn (1, 60, 1.0f), 0);
            float maxDiff = 0.0f, maxL = 0.0f;
            for (int b = 0; b < 60; ++b)
            {
                buf.clear();
                MidiBuffer m; if (b == 0) m = on;
                p.processBlock (buf, m);
                if (b > 20)
                    for (int i = 0; i < 512; ++i)
                    {
                        maxDiff = jmax (maxDiff, std::abs (buf.getSample (0, i) - buf.getSample (1, i)));
                        maxL = jmax (maxL, std::abs (buf.getSample (0, i)));
                    }
            }
            check ("MONO: canali identici", maxDiff, 0.0, 1.0e-6);
            check ("MONO: segnale presente", maxL > 0.1f ? 1.0 : 0.0, 1.0, 0.0);
        }
        setParam (p, "mono", 0.0f);
        r.run (0.4, {}, note (60, false));

        // COLLURE: calo di livello alla giunta dell'anello
        p.housekeeping(); p.loadFile (wav); r.run (0.05); p.housekeeping();
        setParam (p, "loopLength", 0.1f);   // anello di 0,2 s: 5 giunte al secondo
        auto minEnvelope = [&] (bool spliceOn)
        {
            setParam (p, "splice", spliceOn ? 1.0f : 0.0f);
            r.run (1.2, {}, note (60, true));
            const int win = (int) (0.002 * sr);
            float mn = 1.0e9f;
            for (size_t i = r.captured.size() - (size_t) sr; i + (size_t) win < r.captured.size(); i += (size_t) win / 2)
            {
                double acc = 0; for (int k = 0; k < win; ++k) acc += r.captured[i + (size_t) k] * r.captured[i + (size_t) k];
                mn = jmin (mn, (float) std::sqrt (acc / win));
            }
            r.run (0.4, {}, note (60, false));
            return mn;
        };
        const float envOff = minEnvelope (false), envOn = minEnvelope (true);
        std::cout << "inviluppo minimo: senza giunta " << envOff << ", con giunta " << envOn << "\n";
        check ("COLLURE: calo udibile alla giunta (< 70 %)", envOn / envOff < 0.7f ? 1.0 : 0.0, 1.0, 0.0);
        setParam (p, "splice", 0.0f);
        setParam (p, "loopLength", 1.0f);

        // DÉBUT/LONGUEUR mossi mentre suona: niente clic (dissolvenza)
        {
            IIRFilter h1, h2, h3;
            const auto c = IIRCoefficients::makeHighPass (sr, 6000.0, 0.7);
            h1.setCoefficients (c); h2.setCoefficients (c); h3.setCoefficients (c);
            r.run (0.5, {}, note (60, true));
            Random rd (3);
            float hf = 0.0f;
            for (int b = 0; b < 40; ++b)
            {
                setParam (p, "loopStart", rd.nextFloat() * 0.6f);
                setParam (p, "loopLength", 0.2f + rd.nextFloat() * 0.4f);
                r.run (0.05);
                for (size_t i = 0; i < r.captured.size(); ++i)
                {
                    const float y = h3.processSingleSampleRaw (h2.processSingleSampleRaw (h1.processSingleSampleRaw (r.captured[i])));
                    if (b > 0) hf = jmax (hf, std::abs (y));
                }
            }
            std::cout << "picco sopra 6 kHz muovendo DEBUT/LONGUEUR: " << hf << "\n";
            check ("DEBUT/LONGUEUR senza clic (picco < 0,02)", hf < 0.02f ? 1.0 : 0.0, 1.0, 0.0);
            r.run (0.4, {}, note (60, false));
            setParam (p, "loopStart", 0.0f);
            setParam (p, "loopLength", 1.0f);
        }

        // soffio trasposto con il nastro: più acuto a velocità doppia
        {
            const File silent = outDir.getChildFile ("silenzio.wav");
            writeStereo (silent, 48000.0, 1.0, [] (int, int) { return 0.0f; });
            p.housekeeping(); p.loadFile (silent); r.run (0.05); p.housekeeping();
            setParam (p, "hiss", 1.0f);
            auto centroid = [&] (int noteNum)
            {
                r.run (0.8, {}, note (noteNum, true));
                const int n = 8192;
                const float* x = r.captured.data() + r.captured.size() - (size_t) n;
                double num = 0, den = 0;
                for (int k = 1; k < n / 2; k += 8)
                {
                    double re = 0, im = 0; const double w = MathConstants<double>::twoPi * k / n;
                    for (int i = 0; i < n; ++i) { re += x[i] * std::cos (w * i); im += x[i] * std::sin (w * i); }
                    const double pw = re * re + im * im;
                    num += pw * k; den += pw;
                }
                r.run (0.4, {}, note (noteNum, false));
                return num / den * sr / n;
            };
            setParam (p, "extended", 1.0f);
            const double c1 = centroid (48), c2 = centroid (60);
            std::cout << "baricentro del soffio: 0,5x " << c1 << " Hz, 1x " << c2 << " Hz\n";
            check ("soffio trasposto (baricentro a 1x > 1,4 volte quello a 0,5x)", c2 / c1 > 1.4 ? 1.0 : 0.0, 1.0, 0.0);
            setParam (p, "extended", 0.0f);
            setParam (p, "hiss", 0.0f);
            p.housekeeping(); p.loadFile (wav); r.run (0.05); p.housekeeping();
        }
    }

    // testo dei valori a due decimali
    for (auto id : { "loopStart", "loopLength", "inertia", "level", "wow", "hiss", "drive" })
    {
        auto* prm = p.apvts.getParameter (id);
        const auto txt = prm->getText (prm->getValue(), 0);
        const bool ok = txt.fromFirstOccurrenceOf (".", false, false).length() == 2;
        std::cout << (ok ? "OK   " : "FAIL ") << id << " -> \"" << txt << "\"\n";
        if (! ok) ++failures;
    }

    // tastiera del controller, ottava di C3 (nota MIDI 60 in Live e Logic): ogni tasto bianco e nero
    {
        r.run (0.3, {}, note (64, false)); // rilascia il MI tenuto dalla prova precedente
        setParam (p, "source", 0.0f);
        const char* names[12] = { "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3" };
        int keyErrors = 0;
        for (int k = 0; k < 12; ++k)
        {
            r.run (0.6, {}, note (60 + k, true));
            const int shown = p.engine.display.activeKey.load();
            const double f = r.freqOfLast (0.3);
            const double expected = 440.0 * std::pow (2.0, k / 12.0);
            const bool ok = shown == k && std::abs (f - expected) < expected * 0.012;
            std::cout << (ok ? "OK   " : "FAIL ") << names[k] << " (nota " << 60 + k << "): tasto " << shown
                      << ", " << f << " Hz (atteso " << expected << ")\n";
            if (! ok) ++keyErrors;
            r.run (0.3, {}, note (60 + k, false));
            if (p.engine.display.activeKey.load() != -1) { std::cout << "FAIL tasto non rilasciato\n"; ++keyErrors; }
        }
        failures += keyErrors;
    }

    // istantanea dell'interfaccia: file caricato, tasto FA# (nero) premuto dal controller
    r.run (0.05, {}, note (66, true));
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

    // seconda istantanea: modalità coulisse, leva a 0,6, nastro innestato
    setParam (p, "mode", 1.0f);
    setParam (p, "coulisse", 0.6f);
    r.run (0.8, {}, note (60, true));
    ed->repaint();
    auto snap2 = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    const File png2 = outDir.getChildFile ("phonogene_ui_coulisse.png");
    png2.deleteFile();
    if (auto os = png2.createOutputStream())
        PNGImageFormat().writeImageToStream (snap2, *os);

    // terza istantanea: universale, nastro a 0,4, SOL suonato, registrazione accesa (spia)
    setParam (p, "mode", 2.0f);
    setParam (p, "coulisse", 0.4f);
    r.run (0.8, {}, [] { MidiBuffer m; m.addEvent (MidiMessage::noteOff (1, 60), 0); m.addEvent (MidiMessage::noteOn (1, 67, 1.0f), 1); return m; }());
    setParam (p, "record", 1.0f);
    ed->repaint();
    auto snap3 = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    setParam (p, "record", 0.0f);
    const File png3 = outDir.getChildFile ("phonogene_ui_universel.png");
    png3.deleteFile();
    if (auto os = png3.createOutputStream())
        PNGImageFormat().writeImageToStream (snap3, *os);

    // quarta istantanea: interfaccia ridimensionata al 70%
    ed->setSize (700, 518);
    auto snap4 = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    const File png4 = outDir.getChildFile ("phonogene_ui_small.png");
    png4.deleteFile();
    if (auto os = png4.createOutputStream())
        PNGImageFormat().writeImageToStream (snap4, *os);

    ed.reset();
    std::cout << (failures == 0 ? "TUTTI I TEST SUPERATI" : "TEST FALLITI: " + String (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
