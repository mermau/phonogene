#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "TapeEngine.h"

class PhonogeneProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    PhonogeneProcessor();
    ~PhonogeneProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Phonogene"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // carica un file audio come nastro (thread dei messaggi)
    bool loadFile (const juce::File& file);          // sincrono (usato dai test)
    void loadFileAsync (const juce::File& file);     // decodifica in background, poi aggiorna il nastro
    bool isLoading() const { return loadingCount.load() > 0; }
    juce::String getFileName() const { return currentFile.getFileName(); }
    float getFileSeconds() const { return fileSeconds; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    TapeEngine engine;
    std::array<std::atomic<float>, 512> filePeaks {};
    juce::AudioFormatManager formatManager;

    static constexpr double maxFileSeconds = 120.0;

    // ---- MIDI CC: assegnazioni predefinite modificabili con MIDI learn ----
    struct CCTarget { const char* paramId; int defaultCC; };
    static constexpr int numCCTargets = 20;
    static const CCTarget ccTargets[numCCTargets];

    int  ccIndexForParam (const juce::String& paramId) const;
    int  getAssignedCC (int index) const { return assignedCC[(size_t) index].load(); }
    void startLearn (int index) { learnIndex.store (index); }
    void cancelLearn() { learnIndex.store (-1); }
    int  getLearnIndex() const { return learnIndex.load(); }
    void forgetCC (int index) { assignedCC[(size_t) index].store (-1); }
    // assegna un CC a un comando, togliendolo a quello che lo usava
    void assignCC (int index, int cc)
    {
        for (auto& a : assignedCC) if (a.load() == cc) a.store (-1);
        assignedCC[(size_t) index].store (cc);
    }
    int  commandForCC (int cc) const
    {
        for (int i = 0; i < numCCTargets; ++i) if (assignedCC[(size_t) i].load() == cc) return i;
        return -1;
    }
    void resetDefaultCCs();
    void applyPendingCCs();   // thread dei messaggi (chiamata dal timer)
    void housekeeping();      // thread dei messaggi: CC in attesa e nastri da liberare

    // nastro registrato: esportazione in WAV/AIFF (in base all'estensione)
    bool hasRecording() const;
    bool exportRecording (const juce::File& file);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    EngineParams readParams() const;

    juce::File currentFile;
    float fileSeconds = 0.0f;
    double currentRate = 0.0;

    // passaggio senza blocchi del nastro tra i thread
    std::atomic<Tape*> pendingTape { nullptr };
    std::atomic<Tape*> retiredTape { nullptr };
    Tape* activeTape = nullptr; // solo thread audio
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

    std::array<std::atomic<int>, numCCTargets>   assignedCC {};
    std::array<std::atomic<float>, numCCTargets> pendingCC {};   // -1 = nessun valore in attesa
    std::atomic<int> learnIndex { -1 };
    void handleControllers (const juce::MidiBuffer& midi) noexcept;

    // copia di appoggio del nastro registrato (ripristino dallo stato, cambio di frequenza)
    juce::AudioBuffer<float> stagedRec;
    int    stagedRecLen = 0;
    double stagedRecRate = 0.0;
    void stageCurrentRecording();
    void restoreRecordingFromFlac (const void* data, size_t size);
    juce::int64 stagedGen = 0;
    juce::MemoryBlock cachedFlac;            // nastro registrato già compresso per il salvataggio
    juce::int64 cachedFlacKey = -1;
    void pushStagedRecording();
    // il nastro da salvare/esportare: quello nel motore o, se non ancora caricato, la copia di appoggio
    bool getRecordingForSave (juce::AudioBuffer<float>& dest, int& len, double& rate) const;

    std::atomic<float>* pMode {}, *pSource {}, *pRecord {}, *pMonitor {}, *pRestart {}, *pExtended {},
                      *pStart {}, *pLength {}, *pInertia {}, *pWow {}, *pHiss {}, *pDrive {},
                      *pLevel {}, *pCoulisse {}, *pBend {}, *pHeads {}, *pWindow {}, *pHardness {},
                      *pOctave {}, *pMono {}, *pSplice {};

    // decodifica + ricampionamento di un file (thread qualsiasi)
    static std::unique_ptr<Tape> decodeFile (const juce::File& file, double rate, float& seconds);
    void commitTape (std::unique_ptr<Tape> tape, const juce::File& file, float seconds);
    std::atomic<int> loadingCount { 0 };
    juce::ThreadPool loader { 1 };   // per ultimo: viene distrutto (e atteso) per primo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhonogeneProcessor)
};
