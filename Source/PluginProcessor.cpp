#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// Assegnazioni predefinite: CC 20-31 liberi nello standard MIDI, mod wheel sulla leva.
const PhonogeneProcessor::CCTarget PhonogeneProcessor::ccTargets[numCCTargets] = {
    { "loopStart", 20 }, { "loopLength", 21 }, { "inertia", 22 }, { "level", 23 },
    { "wow", 24 }, { "hiss", 25 }, { "drive", 26 }, { "coulisse", 1 },
    { "mode", 27 }, { "source", 28 }, { "record", 29 }, { "monitor", 30 },
    { "restart", 31 }, { "extended", 14 },
    { "heads", 15 }, { "window", 16 }, { "hardness", 17 }
};

namespace
{
    juce::String twoDecimals (float v, int) { return juce::String (v, 2); }
}

PhonogeneProcessor::PhonogeneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), false)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PHONOGENE", createLayout())
{
    formatManager.registerBasicFormats();

    pMode     = apvts.getRawParameterValue ("mode");
    pSource   = apvts.getRawParameterValue ("source");
    pRecord   = apvts.getRawParameterValue ("record");
    pMonitor  = apvts.getRawParameterValue ("monitor");
    pRestart  = apvts.getRawParameterValue ("restart");
    pExtended = apvts.getRawParameterValue ("extended");
    pStart    = apvts.getRawParameterValue ("loopStart");
    pLength   = apvts.getRawParameterValue ("loopLength");
    pInertia  = apvts.getRawParameterValue ("inertia");
    pWow      = apvts.getRawParameterValue ("wow");
    pHiss     = apvts.getRawParameterValue ("hiss");
    pDrive    = apvts.getRawParameterValue ("drive");
    pLevel    = apvts.getRawParameterValue ("level");
    pCoulisse = apvts.getRawParameterValue ("coulisse");
    pBend     = apvts.getRawParameterValue ("bendRange");
    pHeads    = apvts.getRawParameterValue ("heads");
    pWindow   = apvts.getRawParameterValue ("window");
    pHardness = apvts.getRawParameterValue ("hardness");

    resetDefaultCCs();
    for (auto& v : pendingCC) v.store (-1.0f);

    startTimerHz (30);
}

void PhonogeneProcessor::resetDefaultCCs()
{
    for (int i = 0; i < numCCTargets; ++i)
        assignedCC[(size_t) i].store (ccTargets[i].defaultCC);
}

int PhonogeneProcessor::ccIndexForParam (const juce::String& paramId) const
{
    for (int i = 0; i < numCCTargets; ++i)
        if (paramId == ccTargets[i].paramId) return i;
    return -1;
}

// Thread audio: legge i CC, gestisce il learn e mette i valori in attesa per il thread dei messaggi.
void PhonogeneProcessor::handleControllers (const juce::MidiBuffer& midi) noexcept
{
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isController())
            continue;

        const int cc = m.getControllerNumber();
        if (cc >= 120) continue;   // messaggi di modo canale (all notes off...)
        const float v = (float) m.getControllerValue() / 127.0f;

        if (const int learn = learnIndex.exchange (-1); learn >= 0)
        {
            // un CC controlla un solo comando: lo si toglie agli altri
            for (auto& a : assignedCC)
                if (a.load() == cc) a.store (-1);
            assignedCC[(size_t) learn].store (cc);
        }

        for (int i = 0; i < numCCTargets; ++i)
            if (assignedCC[(size_t) i].load() == cc)
                pendingCC[(size_t) i].store (v);
    }
}

// Thread dei messaggi: applica i valori dei CC ai parametri, notificando l'host.
void PhonogeneProcessor::applyPendingCCs()
{
    for (int i = 0; i < numCCTargets; ++i)
    {
        const float v = pendingCC[(size_t) i].exchange (-1.0f);
        if (v < 0.0f) continue;

        auto* param = apvts.getParameter (ccTargets[i].paramId);
        if (param == nullptr) continue;

        float norm = v;
        if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
            norm = v >= 0.5f ? 1.0f : 0.0f;
        else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            const int n = choice->choices.size();
            norm = choice->convertTo0to1 ((float) juce::roundToInt (v * (float) (n - 1)));
        }

        param->beginChangeGesture();
        param->setValueNotifyingHost (norm);
        param->endChangeGesture();
    }
}

PhonogeneProcessor::~PhonogeneProcessor()
{
    alive->store (false);
    stopTimer();
    delete pendingTape.exchange (nullptr);
    delete retiredTape.exchange (nullptr);
    delete activeTape;
}

APVTS::ParameterLayout PhonogeneProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout l;
    auto fmt = [] { return AudioParameterFloatAttributes().withStringFromValueFunction (twoDecimals); };

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                                                   StringArray { "Chromatique", "A coulisse", "Universel" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "source", 1 }, "Source",
                                                   StringArray { "Fichier", "Entree" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "record", 1 }, "Enregistrement", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "monitor", 1 }, "Ecoute entree", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "restart", 1 }, "Retour au debut", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "extended", 1 }, "Clavier etendu", false));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "loopStart", 1 }, "Debut boucle",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.0f, fmt()));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "loopLength", 1 }, "Longueur boucle",
                                                  NormalisableRange<float> (0.005f, 1.0f, 0.0f, 0.5f), 1.0f, fmt()));
    // fino a 2 s: plausibile per il motore con volano della coulisse; nel cromatico
    // l'innesto reale stava sotto i ~300 ms, oltre è un uso espressivo
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "inertia", 1 }, "Inertie",
                                                  NormalisableRange<float> (2.0f, 2000.0f, 0.0f, 0.3f), 35.0f,
                                                  fmt().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "wow", 1 }, "Pleurage",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.25f, fmt()));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "hiss", 1 }, "Souffle",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.2f, fmt()));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Saturation",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.2f, fmt()));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Niveau",
                                                  NormalisableRange<float> (-36.0f, 12.0f), 0.0f,
                                                  fmt().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "coulisse", 1 }, "Coulisse",
                                                  NormalisableRange<float> (0.02f, 2.0f, 0.0f, 0.6f), 1.0f, fmt()));
    // testina rotante (universale)
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "heads", 1 }, "Tetes",
                                                   StringArray { "2", "3", "4" }, 2));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "window", 1 }, "Fenetre",
                                                  NormalisableRange<float> (10.0f, 250.0f, 0.0f, 0.5f), 60.0f,
                                                  fmt().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "hardness", 1 }, "Durete",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.5f, fmt()));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bendRange", 1 }, "Plage pitch bend",
                                                  NormalisableRange<float> (0.0f, 24.0f, 1.0f), 12.0f,
                                                  AudioParameterFloatAttributes().withLabel ("st")));
    return l;
}

bool PhonogeneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;

    for (int i = 0; i < layouts.inputBuses.size(); ++i)
    {
        const auto in = layouts.getChannelSet (true, i);
        if (! in.isDisabled() && in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono())
            return false;
    }
    return true;
}

void PhonogeneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();

    // prepare() rialloca il nastro di registrazione: lo si mette da parte e lo si ricarica
    if (engine.getRecordedLength() > 0 && stagedRecLen == 0 && currentRate > 0.0)
        stageCurrentRecording();

    engine.prepare (sampleRate, samplesPerBlock);

    // il nastro del file va ricampionato alla nuova frequenza
    if (! juce::approximatelyEqual (sampleRate, currentRate))
    {
        currentRate = sampleRate;
        if (currentFile.existsAsFile())
            juce::MessageManager::callAsync ([this, f = currentFile, a = alive] { if (a->load()) loadFileAsync (f); });
    }
    pushStagedRecording();
}

void PhonogeneProcessor::stageCurrentRecording()
{
    const int len = engine.getRecordedLength();
    engine.copyRecording (stagedRec, len);
    stagedRecLen = len;
    stagedRecRate = currentRate;
}

void PhonogeneProcessor::pushStagedRecording()
{
    if (stagedRecLen <= 0 || currentRate <= 0.0)
        return;

    // ricampiona alla frequenza della sessione, se diversa
    const double ratio = stagedRecRate / currentRate;
    const int maxLen = engine.getMaxRecordLength();
    int outLen = juce::jmin (maxLen, (int) std::floor (stagedRecLen / ratio));
    if (outLen <= 0) return;

    auto* buf = new juce::AudioBuffer<float> (2, outLen);
    for (int ch = 0; ch < 2; ++ch)
    {
        const int src = juce::jmin (ch, stagedRec.getNumChannels() - 1);
        if (juce::approximatelyEqual (ratio, 1.0))
            buf->copyFrom (ch, 0, stagedRec, src, 0, outLen);
        else
        {
            juce::WindowedSincInterpolator interp;
            interp.process (ratio, stagedRec.getReadPointer (src), buf->getWritePointer (ch), outLen,
                            stagedRec.getNumSamples(), 0);
        }
    }

    // panoramica per l'interfaccia (scala sulla durata massima, come durante la registrazione)
    for (int b = 0; b < 512; ++b)
    {
        const int a = (int) ((juce::int64) b * maxLen / 512), e = juce::jmin (outLen, (int) ((juce::int64) (b + 1) * maxLen / 512));
        float pk = 0.0f;
        for (int i = a; i < e; ++i)
            pk = juce::jmax (pk, std::abs (buf->getSample (0, i)), std::abs (buf->getSample (1, i)));
        engine.display.recPeaks[(size_t) b].store (pk);
    }
    engine.display.recSeconds.store ((float) (outLen / currentRate));
    engine.queueRecording (buf, outLen);
}

bool PhonogeneProcessor::getRecordingForSave (juce::AudioBuffer<float>& dest, int& len, double& rate) const
{
    const bool useStaged = stagedRecLen > 0 && (engine.hasQueuedRecording() || engine.getRecordedLength() == 0);
    if (useStaged)
    {
        dest.makeCopyOf (stagedRec);
        len = stagedRecLen;
        rate = stagedRecRate;
    }
    else
    {
        len = engine.getRecordedLength();
        engine.copyRecording (dest, len);
        rate = currentRate > 0.0 ? currentRate : 48000.0;
    }
    return len > 0;
}

bool PhonogeneProcessor::hasRecording() const
{
    return stagedRecLen > 0 || engine.getRecordedLength() > 0;
}

bool PhonogeneProcessor::exportRecording (const juce::File& file)
{
    juce::AudioBuffer<float> data;
    int len = 0;
    double rate = 0.0;
    if (! getRecordingForSave (data, len, rate))
        return false;

    std::unique_ptr<juce::AudioFormat> format;
    if (file.hasFileExtension ("aif;aiff")) format = std::make_unique<juce::AiffAudioFormat>();
    else                                     format = std::make_unique<juce::WavAudioFormat>();

    file.deleteFile();
    auto os = file.createOutputStream();
    if (os == nullptr) return false;
    std::unique_ptr<juce::AudioFormatWriter> writer (format->createWriterFor (os.get(), rate, 2, 24, {}, 0));
    if (writer == nullptr) return false;
    os.release();   // ora appartiene al writer
    return writer->writeFromAudioSampleBuffer (data, 0, len);
}

EngineParams PhonogeneProcessor::readParams() const
{
    EngineParams p;
    p.mode       = (int) pMode->load();
    p.source     = (int) pSource->load();
    p.record     = pRecord->load() > 0.5f;
    p.monitor    = pMonitor->load() > 0.5f;
    p.restart    = pRestart->load() > 0.5f;
    p.extended   = pExtended->load() > 0.5f;
    p.loopStart  = pStart->load();
    p.loopLength = pLength->load();
    p.inertiaMs  = pInertia->load();
    p.wow        = pWow->load();
    p.hiss       = pHiss->load();
    p.drive      = pDrive->load();
    p.levelDb    = pLevel->load();
    p.coulisse   = pCoulisse->load();
    p.bendRange  = pBend->load();
    p.heads      = 2 + (int) pHeads->load();
    p.windowMs   = pWindow->load();
    p.hardness   = pHardness->load();
    return p;
}

void PhonogeneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // prende in carico un nuovo nastro, se pronto
    if (auto* fresh = pendingTape.load(); fresh != nullptr && retiredTape.load() == nullptr)
    {
        if (pendingTape.compare_exchange_strong (fresh, nullptr))
        {
            retiredTape.store (activeTape);
            activeTape = fresh;
        }
    }
    engine.setFileTape (activeTape);

    handleControllers (midi);
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // sorgente: sidechain se attivo, altrimenti ingresso principale
    juce::AudioBuffer<float> input;
    const juce::AudioBuffer<float>* inPtr = nullptr;

    if (auto* sc = getBus (true, 1); sc != nullptr && sc->isEnabled() && sc->getNumberOfChannels() > 0)
    {
        input = getBusBuffer (buffer, true, 1);
        inPtr = &input;
    }
    else if (auto* main = getBus (true, 0); main != nullptr && main->isEnabled() && main->getNumberOfChannels() > 0)
    {
        input = getBusBuffer (buffer, true, 0);
        inPtr = &input;
    }

    auto out = getBusBuffer (buffer, false, 0);
    engine.process (out, inPtr, midi, readParams());

    // eventuali canali in più (non usati) a zero
    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

void PhonogeneProcessor::timerCallback()
{
    housekeeping();
}

void PhonogeneProcessor::housekeeping()
{
    applyPendingCCs();
    if (auto* used = engine.takeConsumedRecording())
    {
        delete used;
        stagedRecLen = 0;          // ora il nastro vive nel motore
        stagedRec.setSize (2, 1);
    }
    delete retiredTape.exchange (nullptr);
}

std::unique_ptr<Tape> PhonogeneProcessor::decodeFile (const juce::File& file, double rate, float& seconds)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr)
        return {};

    const auto srcLen = (int) juce::jmin<juce::int64> (reader->lengthInSamples,
                                                        (juce::int64) (maxFileSeconds * reader->sampleRate));
    if (srcLen < 16)
        return {};

    juce::AudioBuffer<float> src (2, srcLen);
    reader->read (&src, 0, srcLen, 0, true, reader->numChannels > 1);
    if (reader->numChannels == 1)
        src.copyFrom (1, 0, src, 0, 0, srcLen);

    const double ratio = reader->sampleRate / rate;   // campioni sorgente per campione di uscita
    const int outLen = (int) std::floor (srcLen / ratio) - 4;

    auto tape = std::make_unique<Tape>();
    tape->data.setSize (2, juce::jmax (8, outLen));
    tape->data.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        juce::WindowedSincInterpolator interp;
        interp.process (ratio, src.getReadPointer (ch), tape->data.getWritePointer (ch), outLen);
    }
    tape->length = outLen;

    for (size_t b = 0; b < tape->peaks.size(); ++b)
    {
        const int a = (int) ((juce::int64) b * outLen / (int) tape->peaks.size());
        const int e = (int) ((juce::int64) (b + 1) * outLen / (int) tape->peaks.size());
        float pk = 0.0f;
        for (int i = a; i < e; ++i)
            pk = juce::jmax (pk, std::abs (tape->data.getSample (0, i)), std::abs (tape->data.getSample (1, i)));
        tape->peaks[b] = pk;
    }
    seconds = (float) (outLen / rate);
    return tape;
}

void PhonogeneProcessor::commitTape (std::unique_ptr<Tape> tape, const juce::File& file, float seconds)
{
    for (size_t b = 0; b < tape->peaks.size(); ++b)
        filePeaks[b].store (tape->peaks[b]);
    currentFile = file;
    fileSeconds = seconds;
    delete pendingTape.exchange (tape.release());
}

bool PhonogeneProcessor::loadFile (const juce::File& file)
{
    float secs = 0.0f;
    auto tape = decodeFile (file, currentRate > 0.0 ? currentRate : 48000.0, secs);
    if (tape == nullptr)
        return false;
    commitTape (std::move (tape), file, secs);
    return true;
}

void PhonogeneProcessor::loadFileAsync (const juce::File& file)
{
    const double rate = currentRate > 0.0 ? currentRate : 48000.0;
    ++loadingCount;
    loader.addJob ([this, file, rate, a = alive]
    {
        float secs = 0.0f;
        auto tape = decodeFile (file, rate, secs);
        auto* raw = tape.release();
        juce::MessageManager::callAsync ([this, file, secs, raw, a]
        {
            std::unique_ptr<Tape> t (raw);
            if (! a->load()) return;
            if (t != nullptr)
                commitTape (std::move (t), file, secs);
            --loadingCount;
        });
    });
}

void PhonogeneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("filePath", currentFile.getFullPathName(), nullptr);
    state.setProperty ("uiWidth", editorWidth, nullptr);
    juce::StringArray map;
    for (int i = 0; i < numCCTargets; ++i)
        map.add (juce::String (ccTargets[i].paramId) + ":" + juce::String (assignedCC[(size_t) i].load()));
    state.setProperty ("ccMap", map.joinIntoString (","), nullptr);

    // nastro registrato, compresso in FLAC 24 bit
    juce::AudioBuffer<float> rec;
    int len = 0;
    double rate = 0.0;
    if (getRecordingForSave (rec, len, rate))
    {
        juce::MemoryBlock block;
        {
            auto* mos = new juce::MemoryOutputStream (block, false);
            juce::FlacAudioFormat flac;
            std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (mos, rate, 2, 24, {}, 0));
            if (w != nullptr) w->writeFromAudioSampleBuffer (rec, 0, len);
            else delete mos;
        }
        state.setProperty ("recData", block.toBase64Encoding(), nullptr);
        state.setProperty ("recRate", rate, nullptr);
    }
    else
    {
        state.removeProperty ("recData", nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PhonogeneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid())
            return;

        const juce::String path = state.getProperty ("filePath").toString();
        editorWidth = juce::jlimit (600, 1500, (int) state.getProperty ("uiWidth", 1000));

        // nastro registrato
        if (state.hasProperty ("recData"))
        {
            juce::MemoryBlock block;
            if (block.fromBase64Encoding (state.getProperty ("recData").toString()))
            {
                juce::FlacAudioFormat flac;
                std::unique_ptr<juce::AudioFormatReader> reader (
                    flac.createReaderFor (new juce::MemoryInputStream (block, false), true));
                if (reader != nullptr && reader->lengthInSamples > 0)
                {
                    const int n = (int) reader->lengthInSamples;
                    stagedRec.setSize (2, n);
                    reader->read (&stagedRec, 0, n, 0, true, true);
                    stagedRecLen = n;
                    stagedRecRate = reader->sampleRate;
                    pushStagedRecording();
                }
            }
        }
        state.removeProperty ("recData", nullptr);   // non resta nell'albero dei parametri
        if (state.hasProperty ("ccMap"))
        {
            for (const auto& item : juce::StringArray::fromTokens (state.getProperty ("ccMap").toString(), ",", ""))
            {
                const int idx = ccIndexForParam (item.upToFirstOccurrenceOf (":", false, false));
                if (idx >= 0)
                    assignedCC[(size_t) idx].store (item.fromFirstOccurrenceOf (":", false, false).getIntValue());
            }
        }
        apvts.replaceState (state);
        if (auto* rec = apvts.getParameter ("record"))
            rec->setValueNotifyingHost (0.0f);

        if (path.isNotEmpty())
        {
            const juce::File f (path);
            if (f.existsAsFile())
                juce::MessageManager::callAsync ([this, f, a = alive] { if (a->load()) loadFileAsync (f); });
        }
    }
}

juce::AudioProcessorEditor* PhonogeneProcessor::createEditor()
{
    return new PhonogeneEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhonogeneProcessor();
}
