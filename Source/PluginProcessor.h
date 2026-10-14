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

    std::atomic<float>* pMode {}, *pSource {}, *pRecord {}, *pMonitor {}, *pRestart {}, *pExtended {},
                      *pStart {}, *pLength {}, *pInertia {}, *pWow {}, *pHiss {}, *pDrive {},
                      *pLevel {}, *pCoulisse {}, *pBend {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhonogeneProcessor)
};
