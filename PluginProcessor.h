#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

// One loaded sample (mono) + its mapping zone (Kontakt-style).
// The zone fields are atomic so the mapping editor can change them while audio runs.
struct NaySample
{
    juce::String fileName;
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> data;
    std::atomic<int> root { 60 }, lo { 0 }, hi { 127 }, vlo { 1 }, vhi { 127 };
};

using NayBank = std::vector<std::shared_ptr<NaySample>>;

struct NayZoneOv { juce::String file; int root = 60, lo = 0, hi = 127, vlo = 1, vhi = 127; };

// An "instrument": a named folder of samples.
struct NayGroup
{
    juce::String name;
    juce::String path;
    std::shared_ptr<const NayBank> bank;  // loaded lazily
    std::vector<NayZoneOv> zones;         // saved zone overrides
    juce::String category;                // e.g. "Stringed", "Flutes" (folder that contains the instrument folder)
};

struct NayVoice
{
    std::shared_ptr<NaySample> sa, sb;    // layer A / layer B sample
    juce::ADSR adsr;
    double posA = 0.0, posB = 0.0, pitch = 60.0, target = 60.0, vibPhase = 0.0;
    float detune = 0.f;
    float velGain = 1.0f, velFc = 18000.f, lp = 0.f, lpB = 0.f, glideMs = 100.f;
    int note = -1;
    bool gate = false;
    juce::uint64 age = 0;
};

enum NayParam
{
    P_gain, P_attack, P_release, P_glide, P_vibDepth, P_vibRate, P_mono, P_legato,
    P_delayTime, P_delayFb, P_delayLevel, P_rvSize, P_rvLevel,
    P_velSens, P_pbRange,
    P_satOn, P_satDrive, P_satMix,
    P_wahOn, P_wahPos, P_wahRate, P_wahMix,
    P_phOn, P_phRate, P_phDepth, P_phMix,
    P_velOn,
    P_arpOn, P_arpMode, P_arpRate, P_arpOct, P_arpGate, P_arpHold,
    P_transpose, P_octave,
    P_mainLevel, P_mainPan, P_mainMute, P_duoLevel, P_duoPan, P_duoMute, P_master,
    P_count
};

class NayAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumVoices = 8;
    static const char* const paramIds[P_count];
    // data folders (library / presets)
    static juce::File dataRoot();
    static juce::File libraryRoot();
    static juce::File factoryPresetDir();
    static juce::File userPresetDir();

    NayAudioProcessor();
    ~NayAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Egyptian Sampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- instruments (message thread only) ----
    int addFolder (const juce::File& folder);
    void selectGroup (int index);                 // main instrument
    void selectDuo (int index);                   // second instrument of the Duo (-1 = off)
    int getDuoGroup() const { return duoGroup; }
    juce::String getGroupCategory (int i) const { return groups[(size_t) i].category; }
    void renameGroup (int index, const juce::String& newName);
    void resetZones (int index);                  // back to automatic mapping
    int getNumGroups() const { return (int) groups.size(); }
    juce::String getGroupName (int i) const { return groups[(size_t) i].name; }
    int getActiveGroup() const { return activeGroup; }
    std::shared_ptr<const NayBank> getBankOf (int groupIndex) const
    {
        return (groupIndex >= 0 && groupIndex < (int) groups.size()) ? groups[(size_t) groupIndex].bank : nullptr;
    }
    juce::String getSampleInfo() const { return sampleInfo; }

    // presets (message thread)
    bool savePreset (const juce::String& name);                 // into the User folder
    bool loadPreset (const juce::File& file);
    static juce::Array<juce::File> listPresets (bool factory);
    static bool deletePreset (const juce::File& file);
    static bool renamePreset (const juce::File& file, const juce::String& newName);
    juce::String getPresetName() const { return currentPreset; }

    // last incoming MIDI event (for the monitor strip). kind: 0 none, 1 note on, 2 note off, 3 CC, 4 pitch bend
    void getMidiMonitor (int& kind, int& a, int& b, int& count) const { kind = monKind; a = monA; b = monB; count = monCount; }

    float getBend() const { return bendNorm; }
    float getModWheel() const { return modWheel; }
    float getExpr() const { return expr; }

    // ---- MIDI learn: right-click a control, then move a knob / pedal on your MIDI device ----
    // target: a NayParam index, or -2 = the expression pedal CC itself, -1 = nothing
    void setLearnTarget (int t) { learnTarget = (learnTarget.load() == t) ? -1 : t; }
    int getLearnTarget() const { return learnTarget; }
    void clearAssign (int t)
    {
        if (t == -2) { expCC = 11; return; }
        for (auto& c : ccMap) if (c.load() == t) c = -1;
    }
    int getAssignedCC (int t) const
    {
        if (t == -2) return expCC;
        for (int i = 0; i < 128; ++i) if (ccMap[i].load() == t) return i;
        return -1;
    }
    void getLearnDone (int& cc, int& target, int& count) const { cc = doneCC; target = doneTarget; count = doneCount; }
    juce::String getParamLabel (int i) const { return params[i] != nullptr ? params[i]->getName (32) : juce::String(); }   // -1..1, for the pitch-bend ribbon

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static int parseRootNote (const juce::String& fileName);
    static bool folderHasSamples (const juce::File& folder);
    static std::shared_ptr<const NayBank> loadBank (const juce::File& folder, const std::vector<NayZoneOv>& ov);
    static void autoZones (const NayBank& b);
    std::shared_ptr<NaySample> pickSample (const NayBank* b, int note, int vel, NaySample*& last);
    static juce::String toStoredPath (const juce::String&);
    static juce::String fromStoredPath (const juce::String&);
    int scanFolder (const juce::File& folder);
    juce::ValueTree buildState();
    void applyState (const juce::ValueTree&);

    void addGroup (const juce::String& name, const juce::String& path, const juce::String& category);
    void ensureLoaded (int index);
    void syncZones();
    void handleMidi (const juce::MidiMessage&, const std::shared_ptr<const NayBank>&, const std::shared_ptr<const NayBank>&);
    void startVoice (NayVoice&, int note, int vel, const std::shared_ptr<const NayBank>&, const std::shared_ptr<const NayBank>&);
    void allNotesOff();
    void renderVoice (NayVoice&, juce::AudioBuffer<float>&, int start, int num);
    void renderAll (juce::AudioBuffer<float>&, int start, int num);
    void processSaturation (juce::AudioBuffer<float>&);
    void processWah (juce::AudioBuffer<float>&);
    void processPhaser (juce::AudioBuffer<float>&);
    void runArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, double bpm);
    void applyVelocity (NayVoice&, int vel);
    void processDelay (juce::AudioBuffer<float>&);
    void processReverb (juce::AudioBuffer<float>&);
    double noteTarget (int note) const;
    float getP (int i) const { return raws[i]->load(); }

    // instruments
    std::vector<NayGroup> groups;
    int activeGroup = -1, duoGroup = -1;
    juce::String currentPreset;
    juce::String sampleInfo { "No instruments yet - click Add Folder" };

    // banks used by the audio thread (swapped from the message thread)
    std::shared_ptr<const NayBank> bankA, bankB;
    juce::SpinLock bankLock;

    std::atomic<float>* raws[P_count] {};
    std::atomic<float>* offRaw[12] {};

    // per-block parameter values
    float pGain = 0.8f, pGlide = 60.f, pVibDepth = 0.f, pVibRate = 5.f;
    float pDelayTime = 350.f, pDelayFb = 0.3f, pDelayLvl = -10.f, pRvSize = 0.6f, pRvLvl = -20.f;
    float pVelSens = 0.7f, pPbRange = 2.f;
    bool pMono = true, pLegato = true, lastMono = true;
    bool pSatOn = false, pWahOn = false, pPhOn = false;
    float pSatDrive = 0.4f, pSatMix = 0.6f;
    float pWahPos = 0.5f, pWahRate = 0.f, pWahMix = 1.f;
    float pPhRate = 0.5f, pPhDepth = 0.6f, pPhMix = 0.7f;
    float off[12] {}; // effective per-note fine tune in cents (user + maqam)

    // voices (mono mode uses voices[0] only)
    std::array<NayVoice, kNumVoices> voices;
    std::vector<int> held;
    int heldVel[128] {};
    juce::uint64 ageCounter = 0;
    float modWheel = 0.f, bendNorm = 0.f, expr = 1.f, exprSm = 1.f;
    double sr = 44100.0;

    // effect state
    float satLp[2] {}, satXPrev[2] {}, satDcX[2] {}, satDcY[2] {};
    float wahIc1[2] {}, wahIc2[2] {};
    double wahPhase = 0.0;
    float phX1[2][4] {}, phY1[2][4] {}, phFb[2] {};
    double phPhase = 0.0;
    struct ArpNote { int note, vel; };
    std::vector<ArpNote> arpNotes;
    int arpIdx = 0, arpCur = -1, physDown = 0;
    double stepCd = 0.0, gateCd = 0.0;
    bool arpWasOn = false, pArpOn = false, pArpHold = false;
    int pArpMode = 0, pArpRate = 1, pArpOct = 1;
    float pArpGate = 0.8f;
    juce::Random rng;
    NaySample* lastPicked = nullptr;
    NaySample* lastPickedB = nullptr;
    float mixAL = 1.f, mixAR = 1.f, mixBL = 1.f, mixBR = 1.f, pMaster = 1.f;      // last sample used (compared only, never dereferenced)
    int sentNote[128] {};                 // note actually played for each input key (0 = none, -1 = dropped, else note+1)
    std::atomic<int> learnTarget { -1 }, expCC { 11 }, doneCC { 0 }, doneTarget { -1 }, doneCount { 0 };
    std::atomic<int> ccMap[128];
    juce::RangedAudioParameter* params[P_count] {};
    std::atomic<int> monKind { 0 }, monA { 0 }, monB { 0 }, monCount { 0 };
    juce::int64 clockPos = 0, curPos = 0;
    float satLo[2] {}, satHi[2] {}, satLp2[2] {}, satEnvIn[2] {}, satEnvOut[2] {}, satG[2] { 1.0f, 1.0f };

    juce::AudioBuffer<float> delayBuf;
    int delayWrite = 0;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NayAudioProcessor)
};
