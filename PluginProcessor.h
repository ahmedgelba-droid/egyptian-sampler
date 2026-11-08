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
};

struct NayVoice
{
    std::shared_ptr<NaySample> sa, sb;    // layer A / layer B sample
    juce::ADSR adsr;
    double posA = 0.0, posB = 0.0, pitch = 60.0, target = 60.0, vibPhase = 0.0;
    float velGain = 1.0f, velFc = 18000.f, lp = 0.f;
    int note = -1;
    bool gate = false;
    juce::uint64 age = 0;
};

enum NayParam
{
    P_gain, P_attack, P_release, P_glide, P_vibDepth, P_vibRate, P_mono, P_legato,
    P_delayTime, P_delayFb, P_delayLevel, P_rvSize, P_rvLevel,
    P_velSens, P_pbRange, P_maqam, P_root,
    P_layerBLevel, P_layerBVel, P_layerBMode,
    P_satOn, P_satDrive, P_satMix,
    P_wahOn, P_wahPos, P_wahRate, P_wahMix,
    P_phOn, P_phRate, P_phDepth, P_phMix,
    P_chOn, P_chRate, P_chDepth, P_chMix,
    P_count
};

class NayAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumVoices = 8;
    static const char* const paramIds[P_count];
    static juce::StringArray maqamNames();
    static float maqamOffset (int maqam, int pitchClass, int root);

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
    void selectGroup (int index);                 // layer A
    void selectLayerB (int index);                // layer B (-1 = none)
    void renameGroup (int index, const juce::String& newName);
    void resetZones (int index);                  // back to automatic mapping
    int getNumGroups() const { return (int) groups.size(); }
    juce::String getGroupName (int i) const { return groups[(size_t) i].name; }
    int getActiveGroup() const { return activeGroup; }
    int getLayerBGroup() const { return layerBGroup; }
    std::shared_ptr<const NayBank> getBankOf (int groupIndex) const
    {
        return (groupIndex >= 0 && groupIndex < (int) groups.size()) ? groups[(size_t) groupIndex].bank : nullptr;
    }
    juce::String getSampleInfo() const { return sampleInfo; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static int parseRootNote (const juce::String& fileName);
    static bool folderHasSamples (const juce::File& folder);
    static std::shared_ptr<const NayBank> loadBank (const juce::File& folder, const std::vector<NayZoneOv>& ov);
    static void autoZones (const NayBank& b);
    static std::shared_ptr<NaySample> pickSample (const NayBank* b, int note, int vel);

    void addGroup (const juce::String& name, const juce::String& path);
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
    void processChorus (juce::AudioBuffer<float>&);
    void applyVelocity (NayVoice&, int vel);
    void processDelay (juce::AudioBuffer<float>&);
    void processReverb (juce::AudioBuffer<float>&);
    double noteTarget (int note) const;
    float getP (int i) const { return raws[i]->load(); }

    // instruments
    std::vector<NayGroup> groups;
    int activeGroup = -1, layerBGroup = -1;
    juce::String sampleInfo { "No instruments yet - click Add Folder" };

    // banks used by the audio thread (swapped from the message thread)
    std::shared_ptr<const NayBank> bankA, bankB;
    juce::SpinLock bankLock;

    std::atomic<float>* raws[P_count] {};
    std::atomic<float>* offRaw[12] {};

    // per-block parameter values
    float pGain = 0.8f, pGlide = 60.f, pVibDepth = 0.f, pVibRate = 5.f;
    float pDelayTime = 350.f, pDelayFb = 0.3f, pDelayLvl = -10.f, pRvSize = 0.6f, pRvLvl = -20.f;
    float pVelSens = 0.7f, pPbRange = 2.f, pLayerBGain = 1.f;
    int pLayerBVel = 100, pLayerBMode = 0;
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
    float modWheel = 0.f, bendNorm = 0.f;
    double sr = 44100.0;

    // effect state
    float satLp[2] {}, satXPrev[2] {}, satDcX[2] {}, satDcY[2] {};
    float wahIc1[2] {}, wahIc2[2] {};
    double wahPhase = 0.0;
    float phX1[2][4] {}, phY1[2][4] {}, phFb[2] {};
    double phPhase = 0.0;
    juce::AudioBuffer<float> chBuf;
    int chWrite = 0;
    double chPhase = 0.0;
    float chLp[2] {};
    bool pChOn = false;
    float pChRate = 0.8f, pChDepth = 0.6f, pChMix = 0.7f;

    juce::AudioBuffer<float> delayBuf;
    int delayWrite = 0;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NayAudioProcessor)
};
