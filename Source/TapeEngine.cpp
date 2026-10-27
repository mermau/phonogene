#include "TapeEngine.h"
#include <cmath>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    // Velocità relativa del capstan per un tasto (temperamento equabile).
    double keyRatio (double semitones) { return std::pow (2.0, semitones / 12.0); }

    // seno veloce su fase normalizzata [0,1): parabola corretta, errore < 0,1 %
    inline double fastSin01 (double ph) noexcept
    {
        const double x = (ph < 0.5 ? ph : ph - 1.0) * 2.0;        // -1..1
        const double y = 4.0 * x * (1.0 - std::abs (x));
        return y * (0.775 + 0.225 * std::abs (y));
    }

    // tanh razionale (Padé 7/6) limitata: monotona, satura a ±1
    inline float fastTanh (float x) noexcept
    {
        x = juce::jlimit (-4.97f, 4.97f, x);
        const float x2 = x * x;
        const float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
        const float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
    }
}

double TapeEngine::ratioFor (double semis) noexcept
{
    if (semis != cachedSemis)
    {
        cachedSemis = semis;
        cachedRatio = keyRatio (semis);
    }
    return cachedRatio;
}

void TapeEngine::prepare (double sampleRate, int)
{
    sr = sampleRate;
    recBuffer.setSize (2, (int) (maxRecordSeconds * sr) + 4);
    recBuffer.clear();
    recWrite = recLength = 0;
    wasRecording = false;
    heldCount = 0;
    gate = false;
    pos = speed = targetSpeed = 0.0;
    curStart = curRegion = -1;
    lastParamStart = lastParamLength = -1.0f;
    xfade = 1.0;
    hissCut = -1.0f;
    hissLow.reset();

    for (auto& f : lp)  f.setCoefficients (juce::IIRCoefficients::makeLowPass  (sr, 12000.0, 0.6));
    for (auto& f : hp)  f.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 35.0, 0.7));
    hissFilter.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 2500.0, 0.5));
    for (auto& f : lp) f.reset();
    for (auto& f : hp) f.reset();
    hissFilter.reset();

    for (auto& pk : display.recPeaks) pk.store (0.0f);
}

void TapeEngine::copyRecording (juce::AudioBuffer<float>& dest, int len) const
{
    len = juce::jlimit (0, recBuffer.getNumSamples(), len);
    dest.setSize (2, juce::jmax (1, len));
    dest.clear();
    for (int ch = 0; ch < 2; ++ch)
        dest.copyFrom (ch, 0, recBuffer, ch, 0, len);
}

void TapeEngine::queueRecording (juce::AudioBuffer<float>* buf, int len) noexcept
{
    pendingRecLen.store (len);
    delete pendingRec.exchange (buf);   // un eventuale nastro precedente non ancora preso
}

int TapeEngine::topHeldNote() const noexcept
{
    return heldCount > 0 ? heldOrder[(size_t) heldCount - 1] : -1;
}

void TapeEngine::handleMidi (const juce::MidiMessage& m, const EngineParams& p) noexcept
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        const bool wasIdle = heldCount == 0;

        // rimuove eventuali duplicati e mette la nota in cima
        for (int i = 0; i < heldCount; ++i)
            if (heldOrder[(size_t) i] == n)
            {
                for (int j = i; j < heldCount - 1; ++j) heldOrder[(size_t) j] = heldOrder[(size_t) j + 1];
                --heldCount;
                break;
            }
        if (heldCount < (int) heldOrder.size())
            heldOrder[(size_t) heldCount++] = n;

        if (wasIdle && p.restart)
            pos = 0.0;
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();
        for (int i = 0; i < heldCount; ++i)
            if (heldOrder[(size_t) i] == n)
            {
                for (int j = i; j < heldCount - 1; ++j) heldOrder[(size_t) j] = heldOrder[(size_t) j + 1];
                --heldCount;
                break;
            }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        heldCount = 0;
    }
    else if (m.isPitchWheel())
    {
        pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    }
}

TapeEngine::TapeView TapeEngine::currentTape (const EngineParams& p) const noexcept
{
    if (p.source == 1)
    {
        const int len = recWrite > 0 && wasRecording ? recWrite : recLength;
        return { recBuffer.getReadPointer (0), recBuffer.getReadPointer (1), len };
    }

    if (fileTape != nullptr && fileTape->length > 0)
        return { fileTape->data.getReadPointer (0), fileTape->data.getReadPointer (1), fileTape->length };

    return { nullptr, nullptr, 0 };
}

// ---- Lettura a banda limitata (anti-aliasing) ----
// Sinc finestrato (Kaiser) tabulato: per velocità > 1 il taglio scende a Nyquist/velocità,
// così i contenuti che dopo l'accelerazione supererebbero Nyquist vengono filtrati
// invece di ripiegarsi nella banda udibile.
namespace
{
    constexpr int sincZeros = 8;          // attraversamenti dello zero per lato
    constexpr int sincRes = 512;          // punti per unità
    constexpr double maxRateForFilter = 4.0;

    const std::vector<float>& sincTable()
    {
        static const std::vector<float> table = []
        {
            std::vector<float> t ((size_t) (sincZeros * sincRes + 2));
            const double beta = 7.0;
            auto bessel0 = [] (double x)
            {
                double sum = 1.0, term = 1.0;
                for (int k = 1; k < 30; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; }
                return sum;
            };
            const double norm = bessel0 (beta);
            for (size_t i = 0; i < t.size(); ++i)
            {
                const double x = (double) i / sincRes;                 // 0..Z
                const double sinc = x < 1.0e-9 ? 1.0 : std::sin (juce::MathConstants<double>::pi * x) / (juce::MathConstants<double>::pi * x);
                const double r = juce::jmin (1.0, x / sincZeros);
                const double kaiser = bessel0 (beta * std::sqrt (1.0 - r * r)) / norm;
                t[i] = (float) (sinc * kaiser);
            }
            return t;
        }();
        return table;
    }
}

void TapeEngine::readBandLimited (const TapeView& tape, int start, int region, double position, double rate,
                                  float& outL, float& outR) noexcept
{
    const auto& table = sincTable();
    const double c = 0.94 / juce::jlimit (1.0, maxRateForFilter, std::abs (rate));   // taglio relativo a Nyquist
    const double half = sincZeros / c;
    const int i0 = (int) std::floor (position);
    const int reach = (int) std::ceil (half);
    const int k0 = i0 - reach + 1, k1 = i0 + reach;
    const float cf = (float) c;
    const double scale = c * sincRes;
    float l = 0.0f, r = 0.0f;

    // caso comune: la finestra non attraversa la giunta dell'anello -> accesso diretto
    if (k0 >= 0 && k1 < region && start + k1 < tape.length)
    {
        const float* pl = tape.l + start;
        const float* pr = tape.r + start;
        double d = (position - k0) * scale;           // distanza in punti di tabella, decresce
        for (int k = k0; k <= k1; ++k, d -= scale)
        {
            const double ti = std::abs (d);
            if (ti >= sincZeros * sincRes) continue;
            const int ii = (int) ti;
            const float frac = (float) (ti - ii);
            const float w = cf * (table[(size_t) ii] + frac * (table[(size_t) ii + 1] - table[(size_t) ii]));
            l += w * pl[k];
            r += w * pr[k];
        }
        outL = l;
        outR = r;
        return;
    }

    for (int k = k0; k <= k1; ++k)
    {
        const double t = std::abs (position - k) * c;
        if (t >= sincZeros) continue;
        const double ti = t * sincRes;
        const int ii = (int) ti;
        const float frac = (float) (ti - ii);
        const float w = cf * (table[(size_t) ii] + frac * (table[(size_t) ii + 1] - table[(size_t) ii]));
        int idx = k % region;
        if (idx < 0) idx += region;
        idx += start;
        if (idx >= tape.length) continue;
        l += w * tape.l[idx];
        r += w * tape.r[idx];
    }
    outL = l;
    outR = r;
}

// Giunta del nastro (COLLURE): il nastro adesivo che chiude l'anello passa sulla testina.
// Tratto fisso di nastro (~6 ms alla velocità nominale): a velocità bassa dura di più.
float TapeEngine::spliceGain (double rp, int region) const noexcept
{
    const double w = 0.006 * sr;
    double d = std::fmod (rp, (double) region);
    if (d < 0.0) d += region;
    d = juce::jmin (d, (double) region - d);
    if (d >= w || region < 4.0 * w) return 1.0f;
    return (float) (1.0 - 0.55 * 0.5 * (1.0 + std::cos (juce::MathConstants<double>::pi * d / w)));
}

// Interpolazione di Hermite a 4 punti con avvolgimento nella regione dell'anello.
float TapeEngine::hermite (const float* d, int len, int start, int region, double position) noexcept
{
    const int i1 = (int) position;
    const float t = (float) (position - i1);

    auto at = [&] (int i)
    {
        i %= region;
        if (i < 0) i += region;
        const int idx = start + i;
        return idx < len ? d[idx] : 0.0f;
    };

    const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

void TapeEngine::process (juce::AudioBuffer<float>& out,
                          const juce::AudioBuffer<float>* input,
                          const juce::MidiBuffer& midi,
                          const EngineParams& p) noexcept
{
    const int numSamples = out.getNumSamples();
    const int numOut = out.getNumChannels();
    float* outL = numOut > 0 ? out.getWritePointer (0) : nullptr;
    float* outR = numOut > 1 ? out.getWritePointer (1) : nullptr;

    // ---- nastro registrato ripristinato dallo stato del plugin ----
    if (consumedRec.load() == nullptr)
        if (auto* restored = pendingRec.exchange (nullptr))
        {
            const int len = juce::jmin (pendingRecLen.load(), restored->getNumSamples(), recBuffer.getNumSamples() - 4);
            for (int ch = 0; ch < 2; ++ch)
                recBuffer.copyFrom (ch, 0, *restored, juce::jmin (ch, restored->getNumChannels() - 1), 0, len);
            recLength = recWrite = len;
            wasRecording = false;
            consumedRec.store (restored);   // liberato dal thread dei messaggi
            display.recRevision.fetch_add (1);
        }

    // ---- registrazione dall'ingresso (sidechain o ingresso principale) ----
    const bool recNow = p.record && p.source == 1;
    if (recNow && ! wasRecording)
    {
        recWrite = 0;
        for (auto& pk : display.recPeaks) pk.store (0.0f);
    }
    if (! recNow && wasRecording)
        recLength = recWrite;
    if (recNow != wasRecording)
        display.recRevision.fetch_add (1);
    wasRecording = recNow;

    // l'ingresso può condividere la memoria con l'uscita: si legge ogni campione
    // prima di scrivere quello corrispondente in uscita
    const int inCh = input != nullptr ? input->getNumChannels() : 0;
    const float* inL = inCh > 0 ? input->getReadPointer (0) : nullptr;
    const float* inR = inCh > 1 ? input->getReadPointer (1) : inL;

    const int recMax = recBuffer.getNumSamples() - 4;
    auto* recL = recBuffer.getWritePointer (0);
    auto* recR = recBuffer.getWritePointer (1);

    // ---- parametri del blocco ----
    const double inertiaSec = juce::jmax (0.001, (double) p.inertiaMs * 0.001);
    const double coefAttack = std::exp (-1.0 / (inertiaSec * sr));
    const double coefRelease = std::exp (-1.0 / (inertiaSec * 0.7 * sr));
    // il motore con volano è più lento dell'innesto (al massimo 3 s)
    const double coefMotor = std::exp (-1.0 / (juce::jmin (3.0, inertiaSec * 3.0) * sr));
    const double coefEngageOn = std::exp (-1.0 / (0.012 * sr));
    const double coefEngageOff = std::exp (-1.0 / (0.030 * sr));
    const double paramSmooth = 1.0 - std::exp (-1.0 / (0.06 * sr));   // ~60 ms
    const double fadeStep = 1.0 / (0.008 * sr);                         // 8 ms
    const double xfadeStep = 1.0 / (0.010 * sr);                        // 10 ms per DÉBUT/LONGUEUR
    const float rumbleCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 45.0 / sr));
    const float levelGain = juce::Decibels::decibelsToGain (p.levelDb);
    const float driveAmt = 1.0f + p.drive * 6.0f;
    const float driveNorm = 1.0f / std::sqrt (driveAmt);
    const float hissGain = p.hiss * p.hiss * 0.02f;
    const float wowDepth = p.wow * 0.004f;
    const float flutterDepth = p.wow * 0.0015f;
    const double octaveMul = p.octave ? 2.0 : 1.0;
    const double invSr = 1.0 / sr;

    // il soffio è inciso sul nastro: si traspone con la lettura (velocità del nastro,
    // o altezza del tamburo in universale). Filtri aggiornati una volta per blocco.
    {
        const double readRate = p.mode == 2 ? pitchNow : (p.mode == 1 ? motorSpeed : juce::jmax (speed, targetSpeed));
        const float f = (float) juce::jlimit (0.05, 4.0, readRate);
        if (hissCut < 0.0f || std::abs (f - hissCut) > 0.03f * hissCut)
        {
            hissCut = f;
            const double nyq = 0.45 * sr;
            hissFilter.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, juce::jmin (nyq, 2500.0 * f), 0.5));
            hissLow.setCoefficients (juce::IIRCoefficients::makeLowPass (sr, juce::jmin (nyq, 12000.0 * f), 0.6));
        }
    }

    auto midiIt = midi.begin();
    float peak = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        // eventi MIDI con precisione al campione
        while (midiIt != midi.end() && (*midiIt).samplePosition <= s)
        {
            handleMidi ((*midiIt).getMessage(), p);
            ++midiIt;
        }

        // ingresso
        const float in0 = inL != nullptr ? inL[s] : 0.0f;
        const float in1 = inR != nullptr ? inR[s] : 0.0f;

        if (recNow && recWrite < recMax)
        {
            recL[recWrite] = in0;
            recR[recWrite] = in1;
            const int bin = (int) ((double) recWrite / recMax * 512.0);
            auto& pk = display.recPeaks[(size_t) juce::jlimit (0, 511, bin)];
            const float a = juce::jmax (std::abs (in0), std::abs (in1));
            if (a > pk.load (std::memory_order_relaxed)) pk.store (a, std::memory_order_relaxed);
            ++recWrite;
        }

        // ---- meccanica: quale capstan è innestato ----
        const int note = topHeldNote();
        gate = note >= 0;

        double lowSpeed = 0.0;   // 0..1: quanto il nastro è vicino all'arresto (solo coulisse)
        double motorRate = 1.0;  // velocità del motore che produce pleurage e scintillio

        if (p.mode == 1)
        {
            // Phonogène à coulisse: la leva regola il motore, che gira sempre e ha una
            // propria inerzia (manopola INERTIE); il tasto innesta/rilascia il nastro.
            const double lever = (double) p.coulisse * ratioFor ((double) pitchBend * p.bendRange);
            motorSpeed = lever + (motorSpeed - lever) * coefMotor;
            const double eTarget = gate ? 1.0 : 0.0;
            engage = eTarget + (engage - eTarget) * (gate ? coefEngageOn : coefEngageOff);
            targetSpeed = motorSpeed * eTarget;
            speed = motorSpeed * engage;
            if (speed < 1.0e-6 && ! gate) speed = 0.0;
            lowSpeed = juce::jlimit (0.0, 1.0, 1.0 - motorSpeed / 0.5);
            motorRate = motorSpeed;
        }
        else if (p.mode == 2)
        {
            // Phonogène universel: il nastro scorre alla velocità della leva (durata),
            // il tamburo con le testine rotanti dà l'altezza (tastiera + pitch bend).
            const double eTarget = gate ? 1.0 : 0.0;
            engage = eTarget + (engage - eTarget) * (gate ? coefEngageOn : coefEngageOff);
            if (gate)
            {
                const int semis = p.extended ? note - 60 : ((note % 12) + 12) % 12;
                pitchTarget = octaveMul * ratioFor ((double) semis + (double) pitchBend * p.bendRange);
            }
            pitchNow = pitchTarget + (pitchNow - pitchTarget) * coefAttack;   // glissando con INERTIE

            targetSpeed = gate ? (double) p.coulisse : 0.0;
            const double c = targetSpeed > speed ? coefAttack : coefRelease;
            speed = targetSpeed + (speed - targetSpeed) * c;
            if (speed < 1.0e-6 && ! gate) speed = 0.0;
            motorSpeed = (double) p.coulisse;
            motorRate = (double) p.coulisse;   // motore del trasporto del nastro
        }
        else
        {
            if (! gate)
                targetSpeed = 0.0;
            else
            {
                const int semis = p.extended ? note - 60 : ((note % 12) + 12) % 12;
                targetSpeed = octaveMul * ratioFor ((double) semis);
            }

            const double c = targetSpeed > speed ? coefAttack : coefRelease;
            speed = targetSpeed + (speed - targetSpeed) * c;
            if (speed < 1.0e-5 && targetSpeed <= 0.0) speed = 0.0;

            // il motore della coulisse segue comunque la leva, pronto al cambio di modalità
            motorSpeed = (double) p.coulisse;
            engage = 0.0;
            // i dodici capstan girano sullo stesso motore a velocità costante
        }

        // pleurage (wow) e scintillio (flutter): nascono dalla rotazione delle parti meccaniche,
        // quindi la loro frequenza segue la velocità del motore che le produce
        const double rate = juce::jmax (0.02, motorRate);
        wowPhase += 0.55 * rate * invSr;      if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += 7.3 * rate * invSr;   if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        driftState += 0.0005f * (rng.nextFloat() * 2.0f - 1.0f - driftState);
        // a bassa velocità il motore è meno stabile: più scintillio e deriva
        const double flutterNow = flutterDepth * (1.0 + 3.0 * lowSpeed) + 0.0012 * lowSpeed;
        const double mod = 1.0 + wowDepth * fastSin01 (wowPhase)
                               + flutterNow * fastSin01 (flutterPhase)
                               + (p.wow + 0.5 * lowSpeed) * 0.01 * driftState;
        const double v = speed * mod;

        // ---- regione dell'anello (DÉBUT/LONGUEUR) con dissolvenza sui cambi ----
        float l = 0.0f, r = 0.0f;
        const auto tape = currentTape (p);

        if (tape.length > 4)
        {
            const int tStart = juce::jlimit (0, tape.length - 4, (int) (p.loopStart * tape.length));
            const int tRegion = juce::jlimit (4, tape.length - tStart, (int) (p.loopLength * tape.length));
            const bool paramMoved = p.loopStart != lastParamStart || p.loopLength != lastParamLength;

            if (curStart < 0 || curStart + curRegion > tape.length)
            {
                curStart = tStart; curRegion = tRegion;
                lastParamStart = p.loopStart; lastParamLength = p.loopLength;
                xfade = 1.0;
            }
            else if (paramMoved)
            {
                if (xfade >= 1.0)
                {
                    if (tStart != curStart || tRegion != curRegion)
                    {
                        oldStart = curStart; oldRegion = curRegion;
                        curStart = tStart; curRegion = tRegion;
                        xfade = 0.0;
                    }
                    lastParamStart = p.loopStart; lastParamLength = p.loopLength;
                }
            }
            else
            {
                curStart = tStart; curRegion = tRegion;   // il nastro cresce durante la registrazione
            }
            if (oldStart + oldRegion > tape.length) xfade = 1.0;   // nastro cambiato: niente dissolvenza

            const bool fading = xfade < 1.0;

            if (p.mode == 2 && engage > 1.0e-4)
            {
                // Modello del tamburo: N testine equidistanti; il nastro avvolge 1/N della
                // circonferenza più una sovrapposizione (morbida = due testine insieme,
                // dura = commutazione quasi istantanea). FENÊTRE è la distanza tra due
                // testine misurata sul nastro: ad ogni cambio di testina la lettura salta
                // indietro di questo tratto, che resta fisso in lunghezza di nastro.
                // I comandi cambiano gradualmente per evitare crepitii.
                windowSm  += paramSmooth * 0.5 * ((double) p.windowMs - windowSm);   // ~120 ms
                hardSm    += paramSmooth * ((double) p.hardness - hardSm);

                const int wantHeads = juce::jlimit (2, 4, p.heads);
                if (wantHeads != headsActive)
                {
                    headsFade = juce::jmax (0.0, headsFade - fadeStep);
                    if (headsFade <= 0.0) headsActive = wantHeads;
                }
                else
                    headsFade = juce::jmin (1.0, headsFade + fadeStep);

                const int N = headsActive;
                const double spacing = juce::jlimit (16.0, juce::jmax (16.0, curRegion * 0.45), windowSm * 0.001 * sr);
                const double ov = 0.02 + 0.98 * (1.0 - juce::jlimit (0.0, 1.0, hardSm));
                const double a = 1.0 / N;
                const double contact = a * (1.0 + ov);           // arco di contatto di una testina
                const double W = spacing * (1.0 + ov);           // nastro letto durante il contatto
                const double vp = pitchNow * mod;
                headPhase += (vp - v) * contact / W;             // velocità relativa testina/nastro
                headPhase -= std::floor (headPhase);
                const double invOvA = 1.0 / (ov * a);

                auto drum = [&] (int st, int reg, float& ol, float& orr)
                {
                    float accL = 0.0f, accR = 0.0f, sumW = 0.0f;
                    for (int i = 0; i < N; ++i)
                    {
                        double x = headPhase + (double) i / N;
                        x -= std::floor (x);
                        if (x >= contact) continue;              // testina fuori dal nastro
                        float w = (float) juce::jmin (1.0, x * invOvA, (contact - x) * invOvA);
                        double rp = std::fmod (pos + (x / contact - 0.5) * W, (double) reg);
                        if (rp < 0.0) rp += reg;
                        float hl, hr;
                        readBandLimited (tape, st, reg, rp, vp, hl, hr);
                        const float sg = p.splice ? spliceGain (rp, reg) : 1.0f;
                        accL += w * sg * hl;
                        accR += w * sg * hr;
                        sumW += w;
                    }
                    const float norm = (float) headsFade / juce::jmax (0.5f, sumW);
                    ol = accL * norm;
                    orr = accR * norm;
                };

                drum (curStart, curRegion, l, r);
                if (fading)
                {
                    float ol, orr;
                    drum (oldStart, oldRegion, ol, orr);
                    const float gNew = (float) fastSin01 (xfade * 0.25), gOld = (float) fastSin01 ((1.0 - xfade) * 0.25);
                    l = gNew * l + gOld * ol;
                    r = gNew * r + gOld * orr;
                }

                pos += v;
            }
            else if (p.mode != 2 && v > 0.0)
            {
                const double pn = std::fmod (pos, (double) curRegion);
                readBandLimited (tape, curStart, curRegion, pn, v, l, r);
                if (p.splice)
                {
                    const float sg = spliceGain (pn, curRegion);
                    l *= sg; r *= sg;
                }
                if (fading)
                {
                    const double po = std::fmod (pos, (double) oldRegion);
                    float ol, orr;
                    readBandLimited (tape, oldStart, oldRegion, po, v, ol, orr);
                    if (p.splice)
                    {
                        const float sg = spliceGain (po, oldRegion);
                        ol *= sg; orr *= sg;
                    }
                    const float gNew = (float) fastSin01 (xfade * 0.25), gOld = (float) fastSin01 ((1.0 - xfade) * 0.25);
                    l = gNew * l + gOld * ol;
                    r = gNew * r + gOld * orr;
                }

                pos += v;
            }

            if (fading)
                xfade = juce::jmin (1.0, xfade + xfadeStep);
            if (! fading || xfade >= 1.0)
            {
                if (pos >= curRegion || pos < 0.0)
                    pos = std::fmod (std::fmod (pos, (double) curRegion) + curRegion, (double) curRegion);
            }
        }

        // il segnale svanisce quando il nastro rallenta fino a fermarsi
        float motion = (float) juce::jlimit (0.0, 1.0, speed / 0.12);
        if (p.mode == 2)
            motion = (float) engage;   // l'altezza non dipende dalla velocità del nastro
        if (p.mode == 1)
        {
            // con la correzione della testina tarata per la velocità nominale,
            // il livello cala con la velocità (circa 6 dB per ottava sotto 1×)
            const double ms = juce::jlimit (1.0e-4, 1.0, motorSpeed);
            motion = (float) (engage * std::exp (0.8 * std::log (ms)));
        }
        l *= motion;
        r *= motion;

        // nastro a una traccia: una sola testina, un solo segnale
        if (p.mono)
        {
            const float m = 0.5f * (l + r);
            l = r = m;
        }

        // rombo del motore e del trascinamento, udibile solo vicino all'arresto
        rumbleState += rumbleCoef * ((rng.nextFloat() * 2.0f - 1.0f) - rumbleState);
        const float rumble = rumbleState * (float) (lowSpeed * engage) * (0.15f + p.hiss) * 0.35f;
        l += rumble;
        r += rumble;

        // soffio del nastro, presente solo quando il nastro scorre
        if (hissGain > 0.0f)
        {
            const float n = hissLow.processSingleSampleRaw (hissFilter.processSingleSampleRaw (rng.nextFloat() * 2.0f - 1.0f));
            const float h = n * hissGain * motion * 1.3f;
            l += h;
            r += p.mono ? h : h * 0.9f + 0.1f * hissGain * motion * (rng.nextFloat() * 2.0f - 1.0f);
        }

        // risposta della testina e saturazione
        l = hp[0].processSingleSampleRaw (lp[0].processSingleSampleRaw (l));
        r = hp[1].processSingleSampleRaw (lp[1].processSingleSampleRaw (r));
        l = fastTanh (driveAmt * l) * driveNorm;
        r = fastTanh (driveAmt * r) * driveNorm;

        levelSmoothed += 0.002f * (levelGain - levelSmoothed);
        l *= levelSmoothed;
        r *= levelSmoothed;

        if (p.monitor)
        {
            l += in0;
            r += in1;
        }

        if (outL != nullptr) outL[s] = l;
        if (outR != nullptr) outR[s] = r;
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
    }

    for (int ch = 2; ch < numOut; ++ch)
        out.clear (ch, 0, numSamples);

    // ---- stato per l'interfaccia ----
    const int region = curRegion > 0 ? curRegion : 1;
    meter = juce::jmax (peak, meter * 0.85f);
    display.speed.store ((float) speed);
    display.motorSpeed.store ((float) (p.mode == 1 ? motorSpeed : 1.0));
    display.pitch.store ((float) (p.mode == 2 ? pitchNow : speed));
    display.headPhase.store ((float) headPhase);
    display.heads.store (juce::jlimit (2, 4, p.heads));
    display.engaged.store (gate);
    display.position.store ((float) (std::fmod (pos, (double) region) / region));
    const int note = topHeldNote();
    display.activeKey.store (note < 0 ? -1 : (p.mode == 1 ? -1 : ((note % 12) + 12) % 12));
    display.recording.store (recNow);
    display.recSeconds.store ((float) ((recNow ? recWrite : recLength) / sr));
    recordedShared.store (recNow ? recWrite : recLength);
    display.outLevel.store (meter);
}
