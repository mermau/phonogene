#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

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

    startTimerHz (10);
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

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                                                   StringArray { "Chromatique", "A coulisse", "Universel" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "source", 1 }, "Source",
                                                   StringArray { "Fichier", "Entree" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "record", 1 }, "Enregistrement", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "monitor", 1 }, "Ecoute entree", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "restart", 1 }, "Retour au debut", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "extended", 1 }, "Clavier etendu", false));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "loopStart", 1 }, "Debut boucle",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "loopLength", 1 }, "Longueur boucle",
                                                  NormalisableRange<float> (0.005f, 1.0f, 0.0f, 0.5f), 1.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "inertia", 1 }, "Inertie",
                                                  NormalisableRange<float> (2.0f, 400.0f, 0.0f, 0.4f), 35.0f,
                                                  AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "wow", 1 }, "Pleurage",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.25f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "hiss", 1 }, "Souffle",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.2f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Saturation",
                                                  NormalisableRange<float> (0.0f, 1.0f), 0.2f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Niveau",
                                                  NormalisableRange<float> (-36.0f, 12.0f), 0.0f,
                                                  AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "coulisse", 1 }, "Coulisse",
                                                  NormalisableRange<float> (0.02f, 2.0f, 0.0f, 0.6f), 1.0f));
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
    engine.prepare (sampleRate, samplesPerBlock);

    // il nastro del file va ricampionato alla nuova frequenza
    if (! juce::approximatelyEqual (sampleRate, currentRate))
    {
        currentRate = sampleRate;
        if (currentFile.existsAsFile())
            juce::MessageManager::callAsync ([this, f = currentFile, a = alive] { if (a->load()) loadFile (f); });
    }
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
    delete retiredTape.exchange (nullptr);
}

bool PhonogeneProcessor::loadFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return false;

    const double rate = currentRate > 0.0 ? currentRate : 48000.0;
    const auto srcLen = (int) juce::jmin<juce::int64> (reader->lengthInSamples,
                                                        (juce::int64) (maxFileSeconds * reader->sampleRate));
    if (srcLen < 16)
        return false;

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

    // panoramica per l'interfaccia
    for (size_t b = 0; b < tape->peaks.size(); ++b)
    {
        const int a = (int) ((juce::int64) b * outLen / (int) tape->peaks.size());
        const int e = (int) ((juce::int64) (b + 1) * outLen / (int) tape->peaks.size());
        float pk = 0.0f;
        for (int i = a; i < e; ++i)
            pk = juce::jmax (pk, std::abs (tape->data.getSample (0, i)), std::abs (tape->data.getSample (1, i)));
        tape->peaks[b] = pk;
        filePeaks[b].store (pk);
    }

    currentFile = file;
    fileSeconds = (float) (outLen / rate);
    delete pendingTape.exchange (tape.release());
    return true;
}

void PhonogeneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("filePath", currentFile.getFullPathName(), nullptr);
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
        apvts.replaceState (state);
        if (auto* rec = apvts.getParameter ("record"))
            rec->setValueNotifyingHost (0.0f);

        if (path.isNotEmpty())
        {
            const juce::File f (path);
            if (f.existsAsFile())
                juce::MessageManager::callAsync ([this, f, a = alive] { if (a->load()) loadFile (f); });
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
