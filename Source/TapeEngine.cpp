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

        if (! gate)
            targetSpeed = 0.0;
        else if (p.mode == 1)
            targetSpeed = (double) p.coulisse * keyRatio ((double) pitchBend * p.bendRange);
        else
        {
            const int semis = p.extended ? note - 60 : ((note % 12) + 12) % 12;
            targetSpeed = keyRatio (semis);
        }

        const double c = targetSpeed > speed ? coefAttack : coefRelease;
        speed = targetSpeed + (speed - targetSpeed) * c;
        if (speed < 1.0e-5 && targetSpeed <= 0.0) speed = 0.0;

        // pleurage (wow) e scintillio (flutter): modulano la velocità reale
        wowPhase += 0.55 / sr;      if (wowPhase >= 1.0) wowPhase -= 1.0;
        flutterPhase += 7.3 / sr;   if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        driftState += 0.0005f * (rng.nextFloat() * 2.0f - 1.0f - driftState);
        const double mod = 1.0 + wowDepth * std::sin (twoPi * wowPhase)
                               + flutterDepth * std::sin (twoPi * flutterPhase)
                               + p.wow * 0.01 * driftState;
        const double v = speed * mod;

        // ---- lettura del nastro ----
        float l = 0.0f, r = 0.0f;
        const auto tape = currentTape (p);

        if (tape.length > 4 && v > 0.0)
        {
            const int start = juce::jlimit (0, tape.length - 4, (int) (p.loopStart * tape.length));
            const int region = juce::jlimit (4, tape.length - start, (int) (p.loopLength * tape.length));

            if (pos >= region) pos = std::fmod (pos, (double) region);

            l = hermite (tape.l, tape.length, start, region, pos);
            r = hermite (tape.r, tape.length, start, region, pos);

            pos += v;
            if (pos >= region) pos -= region;
        }

        // il segnale svanisce quando il nastro rallenta fino a fermarsi
        const float motion = (float) juce::jlimit (0.0, 1.0, speed / 0.12);
        l *= motion;
        r *= motion;

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
    display.position.store ((float) (pos / region));
    const int note = topHeldNote();
    display.activeKey.store (note < 0 ? -1 : (p.mode == 1 ? -1 : ((note % 12) + 12) % 12));
    display.recording.store (recNow);
    display.recSeconds.store ((float) ((recNow ? recWrite : recLength) / sr));
    display.outLevel.store (meter);
}
