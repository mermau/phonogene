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
    bool loadFile (const juce::File& file);
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
    static constexpr int numCCTargets = 14;
    static const CCTarget ccTargets[numCCTargets];

    int  ccIndexForParam (const juce::String& paramId) const;
    int  getAssignedCC (int index) const { return assignedCC[(size_t) index].load(); }
    void startLearn (int index) { learnIndex.store (index); }
    void cancelLearn() { learnIndex.store (-1); }
    int  getLearnIndex() const { return learnIndex.load(); }
    void forgetCC (int index) { assignedCC[(size_t) index].store (-1); }
    void resetDefaultCCs();
    void applyPendingCCs();   // thread dei messaggi (chiamata dal timer)

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

    std::atomic<float>* pMode {}, *pSource {}, *pRecord {}, *pMonitor {}, *pRestart {}, *pExtended {},
                      *pStart {}, *pLength {}, *pInertia {}, *pWow {}, *pHiss {}, *pDrive {},
                      *pLevel {}, *pCoulisse {}, *pBend {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhonogeneProcessor)
};
