#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

// Un tratto di nastro già alla frequenza di campionamento della sessione.
struct Tape
{
    juce::AudioBuffer<float> data;   // 2 canali
    int length = 0;
    std::array<float, 512> peaks {};  // panoramica per l'interfaccia
};

// Parametri letti dal processore a ogni blocco.
struct EngineParams
{
    int    mode = 0;          // 0 cromatico, 1 a coulisse, 2 universale (non ancora)
    int    source = 0;        // 0 file, 1 ingresso
    bool   record = false;
    bool   monitor = false;
    bool   restart = true;    // ogni nuovo tasto riparte dall'inizio dell'anello
    bool   extended = false;  // tastiera estesa oltre l'ottava
    float  loopStart = 0.0f;  // 0..1
    float  loopLength = 1.0f; // 0..1
    float  inertiaMs = 35.0f;
    float  wow = 0.25f;       // 0..1
    float  hiss = 0.2f;       // 0..1
    float  drive = 0.2f;      // 0..1
    float  levelDb = 0.0f;
    float  coulisse = 1.0f;   // velocità della leva (0..2)
    float  bendRange = 12.0f; // semitoni del pitch bend in modalità coulisse
    int    heads = 4;         // universale: testine sul tamburo
    float  windowMs = 40.0f;  // universale: nastro a contatto con una testina (ms a velocità nominale)
    float  hardness = 0.5f;   // universale: 0 dissolvenze morbide, 1 commutazione secca
    bool   octave = false;    // cromatico/universale: tastiera un'ottava sopra (velocità ×2)
    bool   mono = false;      // nastro a una traccia, come l'originale
    bool   splice = false;    // giunta dell'anello udibile (COLLURE)
};

// Stato condiviso con l'interfaccia (sola lettura lato GUI).
struct EngineDisplay
{
    std::atomic<float> speed { 0.0f };
    std::atomic<float> motorSpeed { 1.0f };  // velocità del motore (coulisse)
    std::atomic<bool>  engaged { false };
    std::atomic<float> pitch { 1.0f };       // altezza (universale: tamburo)
    std::atomic<float> headPhase { 0.0f };   // rotazione del tamburo 0..1
    std::atomic<int>   heads { 4 };
    std::atomic<int>   recRevision { 0 };    // cambia quando il nastro registrato cambia
    std::atomic<float> position { 0.0f };   // 0..1 nell'anello
    std::atomic<int>   activeKey { -1 };    // 0..11
    std::atomic<bool>  recording { false };
    std::atomic<float> recSeconds { 0.0f };
    std::atomic<float> outLevel { 0.0f };
    std::array<std::atomic<float>, 512> recPeaks {};
};

class TapeEngine
{
public:
    ~TapeEngine() { delete pendingRec.exchange (nullptr); delete consumedRec.exchange (nullptr); }
    static constexpr double maxRecordSeconds = 30.0;

    void prepare (double sampleRate, int maxBlock);

    // Thread audio: il nastro del file è fornito dal processore.
    void setFileTape (const Tape* t) noexcept { fileTape = t; }

    // ---- nastro registrato: lettura, ripristino (stato del plugin) ----
    int  getRecordedLength() const noexcept { return recordedShared.load(); }
    int  getMaxRecordLength() const noexcept { return juce::jmax (0, recBuffer.getNumSamples() - 4); }
    void copyRecording (juce::AudioBuffer<float>& dest, int len) const;      // thread dei messaggi
    // consegna al thread audio un nastro da caricare; il buffer torna indietro per essere liberato
    void queueRecording (juce::AudioBuffer<float>* buf, int len) noexcept;
    bool hasQueuedRecording() const noexcept { return pendingRec.load() != nullptr; }
    juce::AudioBuffer<float>* takeConsumedRecording() noexcept { return consumedRec.exchange (nullptr); }

    void process (juce::AudioBuffer<float>& out,
                  const juce::AudioBuffer<float>* input,
                  const juce::MidiBuffer& midi,
                  const EngineParams& p) noexcept;

    EngineDisplay display;

private:
    struct TapeView { const float* l; const float* r; int length; };

    void handleMidi (const juce::MidiMessage& m, const EngineParams& p) noexcept;
    int  topHeldNote() const noexcept;
    TapeView currentTape (const EngineParams& p) const noexcept;
    static float hermite (const float* d, int len, int start, int region, double pos) noexcept;
    static void readBandLimited (const TapeView& tape, int start, int region, double position, double rate,
                                 float& outL, float& outR) noexcept;
    float spliceGain (double rp, int region) const noexcept;
    double ratioFor (double semis) noexcept;

    double sr = 44100.0;

    // nastro registrato dall'ingresso
    juce::AudioBuffer<float> recBuffer;
    int  recWrite = 0;
    int  recLength = 0;
    std::atomic<int> recordedShared { 0 };
    std::atomic<juce::AudioBuffer<float>*> pendingRec { nullptr };
    std::atomic<int> pendingRecLen { 0 };
    std::atomic<juce::AudioBuffer<float>*> consumedRec { nullptr };
    bool wasRecording = false;

    const Tape* fileTape = nullptr;

    // tastiera: ordine di pressione per la priorità all'ultima nota
    std::array<int, 128> heldOrder {};
    int  heldCount = 0;
    bool gate = false;
    float pitchBend = 0.0f; // -1..1

    // trasporto
    double pos = 0.0;       // campioni dall'inizio della regione
    double speed = 0.0;
    double targetSpeed = 0.0;
    double motorSpeed = 1.0;   // coulisse: velocità del motore, segue la leva
    double engage = 0.0;       // coulisse: innesto del nastro 0..1
    float  rumbleState = 0.0f;
    double pitchTarget = 1.0, pitchNow = 1.0;   // universale
    double headPhase = 0.0;
    double windowSm = 40.0, hardSm = 0.5, headsFade = 1.0;   // comandi del tamburo smussati
    int    headsActive = 4;

    // carattere del nastro
    double wowPhase = 0.0, flutterPhase = 0.0;
    float  driftState = 0.0f;
    float  hissCut = -1.0f;            // frequenza attuale del filtro del soffio (segue la velocità)
    juce::IIRFilter hissLow;

    // regione dell'anello attuale e dissolvenza quando DÉBUT/LONGUEUR cambiano
    int    curStart = -1, curRegion = -1, oldStart = 0, oldRegion = 1;
    float  lastParamStart = -1.0f, lastParamLength = -1.0f;
    double xfade = 1.0;

    // cache del rapporto di velocità (evita pow a ogni campione)
    double cachedSemis = 1.0e9, cachedRatio = 1.0;
    juce::Random rng;
    juce::IIRFilter lp[2], hp[2], hissFilter;
    float levelSmoothed = 0.0f;
    float meter = 0.0f;
};
