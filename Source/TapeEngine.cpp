#include "TapeEngine.h"
#include <cmath>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    // Velocità relativa del capstan per un tasto (temperamento equabile).
    double keyRatio (double semitones) { return std::pow (2.0, semitones / 12.0); }
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
    const int k0 = i0 - (int) std::ceil (half) + 1, k1 = i0 + (int) std::ceil (half);
    float l = 0.0f, r = 0.0f;
    for (int k = k0; k <= k1; ++k)
    {
        const double t = std::abs (position - k) * c;
        if (t >= sincZeros) continue;
        const double ti = t * sincRes;
        const int ii = (int) ti;
        const float frac = (float) (ti - ii);
        const float w = (float) c * (table[(size_t) ii] + frac * (table[(size_t) ii + 1] - table[(size_t) ii]));
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
    wasRecording = recNow;

    // l'ingresso può condividere la memoria con l'uscita: si legge ogni campione
    // prima di scrivere quello corrispondente in uscita
    const int inCh = input != nullptr ? input->getNumChannels() : 0;

    const int recMax = recBuffer.getNumSamples() - 4;
    auto* recL = recBuffer.getWritePointer (0);
    auto* recR = recBuffer.getWritePointer (1);

    std::array<float, 2> inSample {};

    // ---- parametri del blocco ----
    const double inertiaSec = juce::jmax (0.001, (double) p.inertiaMs * 0.001);
    const double coefAttack = std::exp (-1.0 / (inertiaSec * sr));
    const double coefRelease = std::exp (-1.0 / (inertiaSec * 0.7 * sr));
    // il motore con volano è più lento dell'innesto (al massimo 3 s)
    const double coefMotor = std::exp (-1.0 / (juce::jmin (3.0, inertiaSec * 3.0) * sr));
    const double coefEngageOn = std::exp (-1.0 / (0.012 * sr));
    const double coefEngageOff = std::exp (-1.0 / (0.030 * sr));
    const float rumbleCoef = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 45.0 / sr));
    const float levelGain = juce::Decibels::decibelsToGain (p.levelDb);
    const float driveAmt = 1.0f + p.drive * 6.0f;
    const float driveNorm = 1.0f / std::sqrt (driveAmt);
    const float hissGain = p.hiss * p.hiss * 0.02f;
    const float wowDepth = p.wow * 0.004f;
    const float flutterDepth = p.wow * 0.0015f;

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
        if (inCh > 0)
        {
            inSample[0] = input->getSample (0, s);
            inSample[1] = inCh > 1 ? input->getSample (1, s) : inSample[0];
        }
        else
        {
            inSample = { 0.0f, 0.0f };
        }

        if (recNow && recWrite < recMax)
        {
            recL[recWrite] = inSample[0];
            recR[recWrite] = inSample[1];
            const int bin = (int) ((double) recWrite / recMax * 512.0);
            auto& pk = display.recPeaks[(size_t) juce::jlimit (0, 511, bin)];
            const float a = juce::jmax (std::abs (inSample[0]), std::abs (inSample[1]));
            if (a > pk.load (std::memory_order_relaxed)) pk.store (a, std::memory_order_relaxed);
            ++recWrite;
        }

        // ---- meccanica: quale capstan è innestato ----
        const int note = topHeldNote();
        gate = note >= 0;

        double lowSpeed = 0.0;   // 0..1: quanto il nastro è vicino all'arresto (solo coulisse)

        if (p.mode == 1)
        {
            // Phonogène à coulisse: la leva regola il motore, che gira sempre e ha una
            // propria inerzia (manopola INERTIE); il tasto innesta/rilascia il nastro.
            const double lever = (double) p.coulisse * keyRatio ((double) pitchBend * p.bendRange);
            motorSpeed = lever + (motorSpeed - lever) * coefMotor;
            const double eTarget = gate ? 1.0 : 0.0;
            engage = eTarget + (engage - eTarget) * (gate ? coefEngageOn : coefEngageOff);
            targetSpeed = motorSpeed * eTarget;
            speed = motorSpeed * engage;
            if (speed < 1.0e-6 && ! gate) speed = 0.0;
            lowSpeed = juce::jlimit (0.0, 1.0, 1.0 - motorSpeed / 0.5);
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
                pitchTarget = keyRatio ((double) semis + (double) pitchBend * p.bendRange);
            }
            pitchNow = pitchTarget + (pitchNow - pitchTarget) * coefAttack;   // glissando con INERTIE

            targetSpeed = gate ? (double) p.coulisse : 0.0;
            const double c = targetSpeed > speed ? coefAttack : coefRelease;
            speed = targetSpeed + (speed - targetSpeed) * c;
            if (speed < 1.0e-6 && ! gate) speed = 0.0;
            motorSpeed = (double) p.coulisse;
        }
        else
        {
            if (! gate)
                targetSpeed = 0.0;
            else
            {
                const int semis = p.extended ? note - 60 : ((note % 12) + 12) % 12;
                targetSpeed = keyRatio (semis);
            }

            const double c = targetSpeed > speed ? coefAttack : coefRelease;
            speed = targetSpeed + (speed - targetSpeed) * c;
            if (speed < 1.0e-5 && targetSpeed <= 0.0) speed = 0.0;

            // il motore della coulisse segue comunque la leva, pronto al cambio di modalità
            motorSpeed = (double) p.coulisse;
            engage = 0.0;
        }

        // pleurage (wow) e scintillio (flutter): modulano la velocità reale
        wowPhase += 0.55 / sr;      if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += 7.3 / sr;   if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        driftState += 0.0005f * (rng.nextFloat() * 2.0f - 1.0f - driftState);
        // a bassa velocità il motore è meno stabile: più scintillio e deriva
        const double flutterNow = flutterDepth * (1.0 + 3.0 * lowSpeed) + 0.0012 * lowSpeed;
        const double mod = 1.0 + wowDepth * std::sin (twoPi * wowPhase)
                               + flutterNow * std::sin (twoPi * flutterPhase)
                               + (p.wow + 0.5 * lowSpeed) * 0.01 * driftState;
        const double v = speed * mod;

        // ---- lettura del nastro ----
        float l = 0.0f, r = 0.0f;
        const auto tape = currentTape (p);

        if (p.mode == 2 && tape.length > 4 && engage > 1.0e-4)
        {
            const int start = juce::jlimit (0, tape.length - 4, (int) (p.loopStart * tape.length));
            const int region = juce::jlimit (4, tape.length - start, (int) (p.loopLength * tape.length));
            if (pos >= region || pos < 0.0) pos = std::fmod (std::fmod (pos, (double) region) + region, (double) region);

            // Modello del tamburo: N testine equidistanti; il nastro avvolge 1/N della
            // circonferenza più una sovrapposizione (morbida = due testine insieme,
            // dura = commutazione quasi istantanea). FENÊTRE è la distanza tra due
            // testine misurata sul nastro: ad ogni cambio di testina la lettura salta
            // indietro di questo tratto, che resta fisso in lunghezza di nastro.
            const int N = juce::jlimit (2, 4, p.heads);
            const double spacing = juce::jlimit (16.0, juce::jmax (16.0, region * 0.45), (double) p.windowMs * 0.001 * sr);
            const double ov = 0.02 + 0.98 * (1.0 - juce::jlimit (0.0, 1.0, (double) p.hardness));
            const double a = 1.0 / N;
            const double contact = a * (1.0 + ov);               // arco di contatto di una testina
            const double W = spacing * (1.0 + ov);               // nastro letto durante il contatto
            const double vp = pitchNow * mod;
            headPhase += (vp - v) * contact / W;                 // velocità relativa testina/nastro
            headPhase -= std::floor (headPhase);

            float accL = 0.0f, accR = 0.0f, sumW = 0.0f;
            for (int i = 0; i < N; ++i)
            {
                double x = headPhase + (double) i / N;
                x -= std::floor (x);
                if (x >= contact) continue;                      // testina fuori dal nastro
                const float w = (float) juce::jmin (1.0, x / (ov * a), (contact - x) / (ov * a));
                double rp = pos + (x / contact - 0.5) * W;
                rp = std::fmod (rp, (double) region);
                if (rp < 0.0) rp += region;
                float hl, hr;
                readBandLimited (tape, start, region, rp, vp, hl, hr);
                accL += w * hl;
                accR += w * hr;
                sumW += w;
            }
            const float norm = 1.0f / juce::jmax (0.5f, sumW);
            l = accL * norm;
            r = accR * norm;

            pos += v;
            if (pos >= region) pos -= region;
        }
        else if (p.mode != 2 && tape.length > 4 && v > 0.0)
        {
            const int start = juce::jlimit (0, tape.length - 4, (int) (p.loopStart * tape.length));
            const int region = juce::jlimit (4, tape.length - start, (int) (p.loopLength * tape.length));

            if (pos >= region) pos = std::fmod (pos, (double) region);

            readBandLimited (tape, start, region, pos, v, l, r);

            pos += v;
            if (pos >= region) pos -= region;
        }

        // il segnale svanisce quando il nastro rallenta fino a fermarsi
        float motion = (float) juce::jlimit (0.0, 1.0, speed / 0.12);
        if (p.mode == 2)
            motion = (float) engage;   // l'altezza non dipende dalla velocità del nastro
        if (p.mode == 1)
        {
            // con la correzione della testina tarata per la velocità nominale,
            // il livello cala con la velocità (circa 6 dB per ottava sotto 1×)
            motion = (float) (engage * std::pow (juce::jlimit (0.0, 1.0, motorSpeed), 0.8));
        }
        l *= motion;
        r *= motion;

        // rombo del motore e del trascinamento, udibile solo vicino all'arresto
        rumbleState += rumbleCoef * ((rng.nextFloat() * 2.0f - 1.0f) - rumbleState);
        const float rumble = rumbleState * (float) (lowSpeed * engage) * (0.15f + p.hiss) * 0.35f;
        l += rumble;
        r += rumble;

        // soffio del nastro, presente solo quando il nastro scorre
        const float h = hissFilter.processSingleSampleRaw (rng.nextFloat() * 2.0f - 1.0f) * hissGain * motion;
        l += h;
        r += h * 0.9f + 0.1f * hissGain * motion * (rng.nextFloat() * 2.0f - 1.0f);

        // risposta della testina e saturazione
        l = hp[0].processSingleSampleRaw (lp[0].processSingleSampleRaw (l));
        r = hp[1].processSingleSampleRaw (lp[1].processSingleSampleRaw (r));
        l = std::tanh (driveAmt * l) * driveNorm;
        r = std::tanh (driveAmt * r) * driveNorm;

        levelSmoothed += 0.002f * (levelGain - levelSmoothed);
        l *= levelSmoothed;
        r *= levelSmoothed;

        if (p.monitor)
        {
            l += inSample[0];
            r += inSample[1];
        }

        if (numOut > 0) out.setSample (0, s, l);
        if (numOut > 1) out.setSample (1, s, r);
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
    }

    for (int ch = 2; ch < numOut; ++ch)
        out.clear (ch, 0, numSamples);

    // ---- stato per l'interfaccia ----
    const auto tape = currentTape (p);
    const int region = tape.length > 0 ? juce::jmax (1, (int) (p.loopLength * tape.length)) : 1;
    meter = juce::jmax (peak, meter * 0.85f);
    display.speed.store ((float) speed);
    display.motorSpeed.store ((float) (p.mode == 1 ? motorSpeed : 1.0));
    display.pitch.store ((float) (p.mode == 2 ? pitchNow : speed));
    display.headPhase.store ((float) headPhase);
    display.heads.store (juce::jlimit (2, 4, p.heads));
    display.engaged.store (gate);
    display.position.store ((float) (pos / region));
    const int note = topHeldNote();
    display.activeKey.store (note < 0 ? -1 : (p.mode == 1 ? -1 : ((note % 12) + 12) % 12));
    display.recording.store (recNow);
    display.recSeconds.store ((float) ((recNow ? recWrite : recLength) / sr));
    recordedShared.store (recNow ? recWrite : recLength);
    display.outLevel.store (meter);
}
