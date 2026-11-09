#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

static const char* kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char* kAudioWildcard = "*.wav;*.aif;*.aiff;*.flac";

static float dbToGainOrOff (float db) { return db <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (db); }

const char* const NayAudioProcessor::paramIds[P_count] =
{
    "gain", "attack", "release", "glide", "vibDepth", "vibRate", "mono", "legato",
    "delayTime", "delayFb", "delayLevel", "rvSize", "rvLevel",
    "velSens", "pbRange",
    "satOn", "satDrive", "satMix",
    "wahOn", "wahPos", "wahRate", "wahMix",
    "phOn", "phRate", "phDepth", "phMix",
    "velOn",
    "arpOn", "arpMode", "arpRate", "arpOct", "arpGate", "arpHold",
    "transpose", "octave",
    "mainLevel", "mainPan", "mainMute", "duoLevel", "duoPan", "duoMute", "master", "varOn", "satTone", "satBass", "yamOn", "yamMix", "yamDrive", "yamWah", "yamTone",
    "tuneBypass", "yamShine", "portaOn", "fastThr", "fastTime", "vSens", "vRef", "minTime", "velLo", "velHi", "tDepth", "tOff", "magicTime", "kCut", "kRes",
    "oscOn", "oscOnly", "oscWave", "oscLevel", "oscTune", "oscFine", "oscCut", "oscPan", "oscMute"
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NayAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto id = [] (int i) { return juce::ParameterID { paramIds[i], 1 }; };

    l.add (std::make_unique<F> (id (P_gain),     "Gain",         R (0.f, 1.f, 0.001f), 0.8f));
    l.add (std::make_unique<F> (id (P_attack),   "Attack",       R (0.002f, 1.f, 0.001f, 0.4f), 0.03f));
    l.add (std::make_unique<F> (id (P_release),  "Release",      R (0.02f, 3.f, 0.001f, 0.4f), 0.25f));
    l.add (std::make_unique<F> (id (P_glide),    "Legato Glide", R (0.f, 500.f, 1.f, 0.5f), 60.f));
    l.add (std::make_unique<F> (id (P_vibDepth), "Vib Depth",    R (0.f, 100.f, 0.1f), 12.f));
    l.add (std::make_unique<F> (id (P_vibRate),  "Vib Speed",    R (0.5f, 10.f, 0.01f), 5.f));
    l.add (std::make_unique<B> (id (P_mono),     "Mono (off = Poly)", true));
    l.add (std::make_unique<B> (id (P_legato),   "Legato (Mono)",     true));

    l.add (std::make_unique<F> (id (P_delayTime),  "Delay Time",     R (10.f, 1000.f, 1.f, 0.5f), 350.f));
    l.add (std::make_unique<F> (id (P_delayFb),    "Delay Feedback", R (0.f, 0.9f, 0.001f), 0.35f));
    l.add (std::make_unique<F> (id (P_delayLevel), "Delay Level (dB)", R (-60.f, 0.f, 0.1f), -60.f));
    l.add (std::make_unique<F> (id (P_rvSize),     "Reverb Size",    R (0.f, 1.f, 0.001f), 0.6f));
    l.add (std::make_unique<F> (id (P_rvLevel),    "Reverb Level (dB)", R (-60.f, 0.f, 0.1f), -20.f));

    l.add (std::make_unique<F> (id (P_velSens), "Velocity Sens", R (0.f, 1.f, 0.001f), 0.85f));
    l.add (std::make_unique<F> (id (P_pbRange), "Bend Range (st)", R (1.f, 24.f, 1.f), 2.f));


    l.add (std::make_unique<B> (id (P_satOn),    "KRONOS On", false));
    l.add (std::make_unique<F> (id (P_satDrive), "KRONOS Drive", R (0.f, 1.f, 0.001f), 0.3f));
    l.add (std::make_unique<F> (id (P_satMix),   "KRONOS Amount", R (0.f, 1.f, 0.001f), 0.5f));

    l.add (std::make_unique<B> (id (P_wahOn),   "Wah On", false));
    l.add (std::make_unique<F> (id (P_wahPos),  "Wah Pedal", R (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<F> (id (P_wahRate), "Wah Rate (Hz)", R (0.f, 8.f, 0.01f, 0.6f), 0.f));
    l.add (std::make_unique<F> (id (P_wahMix),  "Wah Amount", R (0.f, 1.f, 0.001f), 1.f));

    l.add (std::make_unique<B> (id (P_phOn),    "Phaser On", false));
    l.add (std::make_unique<F> (id (P_phRate),  "Phaser Rate (Hz)", R (0.05f, 8.f, 0.01f, 0.5f), 0.5f));
    l.add (std::make_unique<F> (id (P_phDepth), "Phaser Depth", R (0.f, 1.f, 0.001f), 0.6f));
    l.add (std::make_unique<F> (id (P_phMix),   "Phaser Amount", R (0.f, 1.f, 0.001f), 0.7f));

    l.add (std::make_unique<B> (id (P_arpOn),   "Arp On", false));
    l.add (std::make_unique<C> (id (P_arpMode), "Arp Pattern", juce::StringArray { "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    l.add (std::make_unique<C> (id (P_arpRate), "Arp Rate", juce::StringArray { "1/4", "1/8", "1/8T", "1/16" }, 1));
    l.add (std::make_unique<F> (id (P_arpOct),  "Arp Octaves", R (1.f, 4.f, 1.f), 1.f));
    l.add (std::make_unique<F> (id (P_arpGate), "Arp Gate", R (0.1f, 1.f, 0.01f), 0.8f));
    l.add (std::make_unique<B> (id (P_arpHold), "Arp Hold", false));
    l.add (std::make_unique<F> (id (P_transpose), "Transpose (st)", R (-12.f, 12.f, 1.f), 0.f));
    l.add (std::make_unique<F> (id (P_octave),    "Octave", R (-3.f, 3.f, 1.f), 0.f));

    l.add (std::make_unique<F> (id (P_mainLevel), "Main Level (dB)", R (-60.f, 6.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (id (P_mainPan),   "Main Pan", R (-1.f, 1.f, 0.01f), 0.f));
    l.add (std::make_unique<B> (id (P_mainMute),  "Main Mute", false));
    l.add (std::make_unique<F> (id (P_duoLevel),  "Duo Level (dB)", R (-60.f, 6.f, 0.1f), 0.f));
    l.add (std::make_unique<F> (id (P_duoPan),    "Duo Pan", R (-1.f, 1.f, 0.01f), 0.f));
    l.add (std::make_unique<B> (id (P_duoMute),   "Duo Mute", false));
    l.add (std::make_unique<F> (id (P_master),    "Master (dB)", R (-60.f, 6.f, 0.1f), 0.f));

    l.add (std::make_unique<B> (id (P_velOn),   "Velocity On", true));
    l.add (std::make_unique<B> (id (P_varOn),   "Note Variation", true));
    l.add (std::make_unique<F> (id (P_satTone), "KRONOS Mid", R (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<F> (id (P_satBass), "KRONOS Low", R (0.f, 1.f, 0.001f), 0.33f));
    l.add (std::make_unique<B> (id (P_yamOn),    "YAMAHA On", false));
    l.add (std::make_unique<F> (id (P_yamMix),   "YAMAHA Amount", R (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<F> (id (P_yamDrive), "YAMAHA Drive", R (0.f, 1.f, 0.001f), 0.3f));
    l.add (std::make_unique<F> (id (P_yamWah),   "YAMAHA Wah", R (0.f, 1.f, 0.001f), 0.4f));
    l.add (std::make_unique<F> (id (P_yamTone),  "YAMAHA Tone", R (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<B> (id (P_tuneBypass), "Scale Tune Bypass", false));
    l.add (std::make_unique<F> (id (P_yamShine),   "YAMAHA Shine", R (0.f, 1.f, 0.001f), 0.5f));
    l.add (std::make_unique<B> (id (P_portaOn),    "Magic Legato", false));
    l.add (std::make_unique<F> (id (P_fastThr),    "Fast Portamento Threshold", R (0.f, 127.f, 1.f), 10.f));
    l.add (std::make_unique<F> (id (P_fastTime),   "Fast Portamento Time", R (0.f, 127.f, 1.f), 7.f));
    l.add (std::make_unique<F> (id (P_vSens),      "Velocity to Portamento Sens", R (0.f, 127.f, 1.f), 26.f));
    l.add (std::make_unique<F> (id (P_vRef),       "Reference Velocity", R (1.f, 127.f, 1.f), 70.f));
    l.add (std::make_unique<F> (id (P_minTime),    "Min Portamento Time", R (0.f, 127.f, 1.f), 6.f));
    l.add (std::make_unique<F> (id (P_velLo),      "Velocity Limit Low", R (1.f, 127.f, 1.f), 1.f));
    l.add (std::make_unique<F> (id (P_velHi),      "Velocity Limit High", R (1.f, 127.f, 1.f), 127.f));
    l.add (std::make_unique<F> (id (P_tDepth),     "Touch Depth", R (0.f, 127.f, 1.f), 127.f));
    l.add (std::make_unique<F> (id (P_tOff),       "Touch Offset", R (0.f, 127.f, 1.f), 70.f));
    l.add (std::make_unique<F> (id (P_magicTime),  "Magic Legato Time", R (0.f, 127.f, 1.f), 0.f));
    l.add (std::make_unique<F> (id (P_kCut), "KRONOS Cutoff", R (-64.f, 63.f, 1.f), 0.f));
    l.add (std::make_unique<F> (id (P_kRes), "KRONOS Resonance", R (-64.f, 63.f, 1.f), 0.f));

    // OSCILLATOR channel: its own mixer strip (level / pan / mute / SOLO), follows glide / bend / vibrato / maqam
    l.add (std::make_unique<B> (id (P_oscOn),    "Oscillator On", false));
    l.add (std::make_unique<B> (id (P_oscOnly),  "Oscillator Solo", false));
    l.add (std::make_unique<C> (id (P_oscWave),  "Oscillator Wave", juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
    l.add (std::make_unique<F> (id (P_oscLevel), "Oscillator Level (dB)", R (-60.f, 6.f, 0.1f), -12.f));
    l.add (std::make_unique<F> (id (P_oscTune),  "Oscillator Tune (st)", R (-24.f, 24.f, 1.f), 0.f));
    l.add (std::make_unique<F> (id (P_oscFine),  "Oscillator Fine (ct)", R (-100.f, 100.f, 1.f), 0.f));
    l.add (std::make_unique<F> (id (P_oscCut),   "Oscillator Filter (Hz)", R (100.f, 18000.f, 1.f, 0.3f), 18000.f));
    l.add (std::make_unique<F> (id (P_oscPan),   "Oscillator Pan", R (-1.f, 1.f, 0.01f), 0.f));
    l.add (std::make_unique<B> (id (P_oscMute),  "Oscillator Mute", false));


    for (int i = 0; i < 12; ++i)
        l.add (std::make_unique<F> (juce::ParameterID { "off" + juce::String (i), 1 },
                                    juce::String ("Fine tune ") + kNoteNames[i] + " (cents)", R (-100.f, 100.f, 1.f), 0.f));
    return l;
}

//==============================================================================
NayAudioProcessor::NayAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NAY_STATE", createLayout())
{
    for (int i = 0; i < P_count; ++i)
        raws[i] = apvts.getRawParameterValue (paramIds[i]);

    for (int i = 0; i < P_count; ++i)
        params[i] = apvts.getParameter (paramIds[i]);
    for (auto& m : ccMap)
        m = -1;

    for (int i = 0; i < 12; ++i)
        offRaw[i] = apvts.getRawParameterValue ("off" + juce::String (i));

    // the factory library (if installed) is always available; the first instrument opens by default
    if (libraryRoot().isDirectory())
        scanFolder (libraryRoot());
    scanInstruments (instrumentsDir());   // the user's own instruments (.esi files)
    if (! groups.empty())
        selectGroup (0);
}

bool NayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void NayAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    for (auto& g : ghosts)
    {
        g.adsr.setSampleRate (sampleRate);
        g.adsr.reset();
        g.sa.reset();
        g.sb.reset();
    }
    satEng[0] = satEng[1] = yamEng[0] = yamEng[1] = 0.f;
    previewVoice.adsr.setSampleRate (sampleRate);
    previewVoice.adsr.reset();
    previewVoice.sa.reset();
    previewVoice.preview = true;
    for (auto& v : voices)
    {
        v.adsr.setSampleRate (sampleRate);
        v.adsr.reset();
        v.sa.reset();
        v.sb.reset();
        v.gate = false;
        v.note = -1;
    }
    held.clear();

    delayBuf.setSize (2, (int) (sampleRate * 1.1) + 8);
    delayBuf.clear();
    delayWrite = 0;

    satLo[0] = satLo[1] = satHi[0] = satHi[1] = 0.f;
    for (int c2 = 0; c2 < 2; ++c2)
    {
        yamLev[c2] = yamHpS[c2] = yamDcX[c2] = yamDcY[c2] = yamEnvIn[c2] = yamEnvOut[c2] = 0.f;
        yamG[c2] = 1.0f;
    }
    for (auto* b : { &yamWah, &yamLo, &yamHi, &yamDLo, &yamLpf, &yamPres, &yamVf })
        for (int c2 = 0; c2 < 2; ++c2) { b->z1[c2] = 0.f; b->z2[c2] = 0.f; }

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    std::fill (std::begin (satLp), std::end (satLp), 0.f);
    std::fill (std::begin (satXPrev), std::end (satXPrev), 0.f);
    std::fill (std::begin (satDcX), std::end (satDcX), 0.f);
    std::fill (std::begin (satDcY), std::end (satDcY), 0.f);
    std::fill (std::begin (wahIc1), std::end (wahIc1), 0.f);
    std::fill (std::begin (wahIc2), std::end (wahIc2), 0.f);
    std::fill (std::begin (phFb), std::end (phFb), 0.f);
    for (int c = 0; c < 2; ++c)
        for (int s = 0; s < 4; ++s) { phX1[c][s] = 0.f; phY1[c][s] = 0.f; }
}

//==============================================================================
// File names must start with a note: "C4.wav", "F#3.wav", "Bb2.wav" or a MIDI number "60.wav".
int NayAudioProcessor::parseRootNote (const juce::String& fileName)
{
    auto t = fileName.trim().upToFirstOccurrenceOf ("_", false, false)
                            .upToFirstOccurrenceOf (" ", false, false);
    if (t.isEmpty())
        return -1;

    if (t.containsOnly ("0123456789"))
    {
        const int n = t.getIntValue();
        return n <= 127 ? n : -1;
    }

    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    const auto c = juce::CharacterFunctions::toUpperCase (t[0]);
    if (c < 'A' || c > 'G')
        return -1;

    int pc = base[c - 'A'];
    int idx = 1;
    if (t[idx] == '#')      { ++pc; ++idx; }
    else if (t[idx] == 'b') { --pc; ++idx; }

    const auto oct = t.substring (idx);
    if (oct.isEmpty() || ! oct.containsOnly ("-0123456789"))
        return -1;

    const int note = (oct.getIntValue() + 1) * 12 + pc;
    return (note >= 0 && note <= 127) ? note : -1;
}

bool NayAudioProcessor::folderHasSamples (const juce::File& folder)
{
    for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, kAudioWildcard))
        if (parseRootNote (f.getFileNameWithoutExtension()) >= 0)
            return true;
    return false;
}

// Automatic key zones: every sample covers the keys closest to its root note.
void NayAudioProcessor::autoZones (const NayBank& b)
{
    std::vector<int> u;
    for (const auto& s : b)
        u.push_back (s->root.load());
    std::sort (u.begin(), u.end());
    u.erase (std::unique (u.begin(), u.end()), u.end());

    for (const auto& s : b)
    {
        const int r = s->root.load();
        const int idx = (int) (std::find (u.begin(), u.end(), r) - u.begin());
        s->lo = idx == 0 ? 0 : (u[(size_t) idx - 1] + r) / 2 + 1;
        s->hi = idx == (int) u.size() - 1 ? 127 : (r + u[(size_t) idx + 1]) / 2;
        s->vlo = 1;
        s->vhi = 127;
    }
}

// Reads one audio file (mono mix, max 30 s) + the loop points saved in its 'smpl' chunk. Root is NOT set here.
std::shared_ptr<NaySample> NayAudioProcessor::readAudioFile (const juce::File& f, juce::AudioFormatManager& fm)
{
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
    if (reader == nullptr || reader->lengthInSamples < 4 || reader->sampleRate <= 0.0)
        return nullptr;

    const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 60.0));
    const int chans = juce::jmax (1, (int) reader->numChannels);

    juce::AudioBuffer<float> tmp (chans, len);
    reader->read (&tmp, 0, len, 0, true, true);

    // stereo stays stereo (no mono mix: summing an organ's stereo chorus to mono changes its sound)
    auto s = std::make_shared<NaySample>();
    s->fileName = f.getFileName();
    s->sampleRate = reader->sampleRate;
    s->data.setSize (chans >= 2 ? 2 : 1, len);
    s->data.copyFrom (0, 0, tmp, 0, 0, len);
    if (chans >= 2)
        s->data.copyFrom (1, 0, tmp, 1, 0, len);

    const int nl = reader->metadataValues.getValue ("NumSampleLoops", "0").getIntValue();
    if (nl > 0)
    {
        const int ls = reader->metadataValues.getValue ("Loop0Start", "-1").getIntValue();
        const int le = reader->metadataValues.getValue ("Loop0End", "-1").getIntValue();
        if (ls >= 0 && le > ls + 8 && le < len)
        {
            s->loopStart = ls;
            s->loopEnd = le;
        }
    }
    return s;
}

// Saved per-sample edits (mapping, root, tune, loop) are matched by file name.
void NayAudioProcessor::applyOverrides (const NayBank& b, const std::vector<NayZoneOv>& ov)
{
    for (const auto& z : ov)
        for (const auto& s : b)
            if (s->fileName == z.file)
            {
                s->root = z.root; s->lo = z.lo; s->hi = z.hi; s->vlo = z.vlo; s->vhi = z.vhi;
                s->tune = z.tune;
                if (z.xf != -2) s->xfade = z.xf;
                if (z.ls != -2 && z.le != -2)
                {
                    const int len = s->data.getNumSamples();
                    if (z.ls < 0 || (z.le > z.ls + 8 && z.le < len)) { s->loopStart = z.ls; s->loopEnd = z.le; }
                }
            }
}

std::shared_ptr<const NayBank> NayAudioProcessor::loadBank (const juce::File& folder, const std::vector<NayZoneOv>& ov)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    auto newBank = std::make_shared<NayBank>();

    for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, kAudioWildcard))
    {
        const int root = parseRootNote (f.getFileNameWithoutExtension());
        if (root < 0)
            continue;

        if (auto s = readAudioFile (f, fm))
        {
            s->root = root;
            s->detectInfo = "from file name";
            newBank->push_back (s);
        }
    }

    std::sort (newBank->begin(), newBank->end(),
               [] (const auto& a, const auto& b) { return a->root.load() < b->root.load(); });

    autoZones (*newBank);
    applyOverrides (*newBank, ov);
    return newBank;
}

//==============================================================================
// .esi instrument file:  "ESI1" | u32 headerSize | header XML | per sample: u32 size | FLAC 24-bit mono
// The header and the audio are scrambled so the file only opens in Egyptian Sampler (light protection, not real
// encryption).
static void esiScramble (void* data, size_t size, juce::uint32 seed)
{
    auto* p = static_cast<juce::uint8*> (data);
    juce::uint32 x = (seed + 1u) * 2654435761u ^ 0x5EB1A7E5u;
    if (x == 0) x = 0x1234567u;
    for (size_t i = 0; i < size; ++i)
    {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        p[i] ^= (juce::uint8) (x >> 24);
    }
}

static std::unique_ptr<juce::XmlElement> esiReadHeader (juce::InputStream& in)
{
    char magic[4] {};
    if (in.read (magic, 4) != 4 || std::memcmp (magic, "ESI1", 4) != 0)
        return nullptr;
    const int hs = in.readInt();
    if (hs <= 0 || hs > 16 * 1024 * 1024)
        return nullptr;
    juce::MemoryBlock hb ((size_t) hs);
    if (in.read (hb.getData(), hs) != hs)
        return nullptr;
    esiScramble (hb.getData(), hb.getSize(), 0);
    auto xml = juce::parseXML (hb.toString());
    if (xml == nullptr || ! xml->hasTagName ("ESI"))
        return nullptr;
    return xml;
}

bool NayAudioProcessor::readEsiHeader (const juce::File& file, juce::String& name, juce::String& category)
{
    juce::FileInputStream in (file);
    if (! in.openedOk())
        return false;
    auto xml = esiReadHeader (in);
    if (xml == nullptr)
        return false;
    name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    category = xml->getStringAttribute ("category", "My Instruments");
    return true;
}

std::shared_ptr<const NayBank> NayAudioProcessor::loadEsi (const juce::File& file, const std::vector<NayZoneOv>& ov)
{
    auto bank = std::make_shared<NayBank>();
    juce::FileInputStream in (file);
    if (! in.openedOk())
        return bank;
    auto xml = esiReadHeader (in);
    if (xml == nullptr)
        return bank;

    juce::FlacAudioFormat flac;
    int idx = 0;
    for (auto* e : xml->getChildWithTagNameIterator ("S"))
    {
        ++idx;
        const int size = in.readInt();
        if (size <= 0 || size > 512 * 1024 * 1024)
            break;
        juce::MemoryBlock mb ((size_t) size);
        if (in.read (mb.getData(), size) != size)
            break;
        esiScramble (mb.getData(), mb.getSize(), (juce::uint32) idx);

        std::unique_ptr<juce::AudioFormatReader> r (flac.createReaderFor (new juce::MemoryInputStream (mb, false), true));
        if (r == nullptr || r->lengthInSamples < 4)
            continue;

        auto s = std::make_shared<NaySample>();
        const int len = (int) r->lengthInSamples;
        const bool st = r->numChannels >= 2;
        s->data.setSize (st ? 2 : 1, len);
        r->read (&s->data, 0, len, 0, true, st);
        s->xfade = e->getIntAttribute ("xf", -1);
        s->fileName = e->getStringAttribute ("file");
        s->sampleRate = r->sampleRate;
        s->root = e->getIntAttribute ("root", 60);
        s->lo = e->getIntAttribute ("lo", 0);
        s->hi = e->getIntAttribute ("hi", 127);
        s->vlo = e->getIntAttribute ("vlo", 1);
        s->vhi = e->getIntAttribute ("vhi", 127);
        s->tune = (float) e->getDoubleAttribute ("tune", 0.0);
        s->detectInfo = e->getStringAttribute ("info");
        const int ls = e->getIntAttribute ("ls", -1), le = e->getIntAttribute ("le", -1);
        if (ls >= 0 && le > ls + 8 && le < len) { s->loopStart = ls; s->loopEnd = le; }
        bank->push_back (s);
    }

    std::sort (bank->begin(), bank->end(), [] (const auto& a, const auto& b) { return a->root.load() < b->root.load(); });
    applyOverrides (*bank, ov);
    return bank;
}

bool NayAudioProcessor::writeEsi (const juce::File& file, const juce::String& name, const juce::String& category, const NayBank& bank)
{
    juce::XmlElement xml ("ESI");
    xml.setAttribute ("version", 1);
    xml.setAttribute ("name", name);
    xml.setAttribute ("category", category);

    std::vector<juce::MemoryBlock> audio;
    juce::FlacAudioFormat flac;
    for (const auto& s : bank)
    {
        juce::MemoryBlock mb;
        {
            const int sr = juce::jlimit (8000, 384000, (int) std::lround (s->sampleRate));
            auto out = std::make_unique<juce::MemoryOutputStream> (mb, false);
            std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (out.get(), (double) sr, (unsigned int) s->data.getNumChannels(), 24, {}, 5));
            if (w == nullptr)
                return false;
            out.release();   // the writer owns the stream now
            w->writeFromAudioSampleBuffer (s->data, 0, s->data.getNumSamples());
        }
        esiScramble (mb.getData(), mb.getSize(), (juce::uint32) (audio.size() + 1));
        audio.push_back (std::move (mb));

        auto* e = xml.createNewChildElement ("S");
        e->setAttribute ("file", s->fileName);
        e->setAttribute ("root", s->root.load());
        e->setAttribute ("lo", s->lo.load());
        e->setAttribute ("hi", s->hi.load());
        e->setAttribute ("vlo", s->vlo.load());
        e->setAttribute ("vhi", s->vhi.load());
        e->setAttribute ("tune", (double) s->tune.load());
        e->setAttribute ("ls", s->loopStart.load());
        e->setAttribute ("le", s->loopEnd.load());
        e->setAttribute ("xf", s->xfade.load());
        e->setAttribute ("info", s->detectInfo);
    }

    juce::MemoryBlock hb;
    {
        const auto t = xml.toString (juce::XmlElement::TextFormat().singleLine().withoutHeader());
        hb.append (t.toRawUTF8(), t.getNumBytesAsUTF8());
    }
    esiScramble (hb.getData(), hb.getSize(), 0);

    file.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp (file);
    {
        juce::FileOutputStream out (tmp.getFile());
        if (! out.openedOk())
            return false;
        out.write ("ESI1", 4);
        out.writeInt ((int) hb.getSize());
        out.write (hb.getData(), hb.getSize());
        for (const auto& mb : audio)
        {
            out.writeInt ((int) mb.getSize());
            out.write (mb.getData(), mb.getSize());
        }
        out.flush();
        if (out.getStatus().failed())
            return false;
    }
    return tmp.overwriteTargetFileWithTemporary();
}

//==============================================================================
// Pitch detection (YIN) on the steady part of the sample. Root = nearest key, tune = cents that bring it in tune.
void NayAudioProcessor::detectRoot (NaySample& s)
{
    const int len = s.data.getNumSamples();
    const double fs = s.sampleRate;
    const auto mono = monoOf (s);
    const float* x = mono.data();
    const int W = fs > 60000.0 ? 4096 : 2048;
    const int tMin = juce::jmax (2, (int) (fs / 2000.0)), tMax = (int) (fs / 40.0);

    std::vector<float> d ((size_t) tMax + 2);
    std::vector<double> found;
    for (double frac : { 0.30, 0.40, 0.50, 0.60 })
    {
        const int st = (int) (len * frac);
        if (st + W + tMax + 2 >= len)
            continue;

        double energy = 0.0;
        for (int j = 0; j < W; ++j) energy += (double) x[st + j] * x[st + j];
        if (energy / W < 1.0e-7)
            continue;   // silence

        for (int tau = 1; tau <= tMax; ++tau)
        {
            double acc = 0.0;
            for (int j = 0; j < W; ++j)
            {
                const double df = (double) x[st + j] - (double) x[st + j + tau];
                acc += df * df;
            }
            d[(size_t) tau] = (float) acc;
        }
        // cumulative mean normalised difference
        double run = 0.0;
        d[0] = 1.0f;
        for (int tau = 1; tau <= tMax; ++tau)
        {
            run += d[(size_t) tau];
            d[(size_t) tau] = run > 0.0 ? (float) (d[(size_t) tau] * tau / run) : 1.0f;
        }

        int best = -1;
        for (int tau = tMin; tau < tMax; ++tau)
            if (d[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tMax && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        if (best < 0)
        {
            float mn = 0.4f;
            for (int tau = tMin; tau < tMax; ++tau)
                if (d[(size_t) tau] < mn) { mn = d[(size_t) tau]; best = tau; }
        }
        if (best <= tMin || best >= tMax - 1)
            continue;

        const double a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
        const double den = a - 2.0 * b + c;
        const double t = (double) best + (std::abs (den) > 1.0e-12 ? 0.5 * (a - c) / den : 0.0);
        found.push_back (69.0 + 12.0 * std::log2 ((fs / t) / 440.0));
    }

    if (found.empty())
    {
        s.detectInfo = "note not detected - set ROOT by hand";
        return;
    }

    std::sort (found.begin(), found.end());
    const double midi = found[found.size() / 2];
    const int root = juce::jlimit (0, 127, (int) std::lround (midi));
    s.root = root;
    s.tune = (float) juce::jlimit (-100.0, 100.0, -(midi - (double) root) * 100.0);
    s.detectInfo = "detected " + juce::String (kNoteNames[root % 12]) + juce::String (root / 12 - 1)
                 + juce::String (s.tune.load() >= 0 ? " +" : " ") + juce::String ((int) std::lround (s.tune.load())) + " ct";
}

std::vector<float> NayAudioProcessor::monoOf (const NaySample& s)
{
    const int len = s.data.getNumSamples(), nc = s.data.getNumChannels();
    std::vector<float> m ((size_t) len, 0.0f);
    for (int c = 0; c < nc; ++c)
    {
        const float* p = s.data.getReadPointer (c);
        for (int i = 0; i < len; ++i) m[(size_t) i] += p[i] / (float) nc;
    }
    return m;
}

// AUTO LOOP (like a sampler editor):
//  1) find the SUSTAIN: after the attack and before the sound starts to fade (loudness envelope)
//  2) inside it, find two upward zero crossings far apart where the waveform shape AND loudness match best
//  3) set a real crossfade (25% of the loop, 10..150 ms) so vibrato / chorus in the recording doesn't click or jump
void NayAudioProcessor::autoLoop (NaySample& s)
{
    const int len = s.data.getNumSamples();
    const double fs = s.sampleRate;
    const auto mono = monoOf (s);
    const float* x = mono.data();
    const int W = juce::jmax (256, (int) (fs * 0.008));     // ~8 ms compared on each side

    if (len < (int) (fs * 0.3) || len < 10 * W)
        return;

    // prefix sum of x^2 -> fast RMS anywhere
    std::vector<double> ps ((size_t) len + 1, 0.0);
    for (int i = 0; i < len; ++i) ps[(size_t) i + 1] = ps[(size_t) i] + (double) x[i] * x[i];
    auto rms = [&] (int c, int n)
    {
        const int a = juce::jlimit (0, len, c - n / 2), b = juce::jlimit (0, len, c + n / 2);
        return std::sqrt ((ps[(size_t) b] - ps[(size_t) a]) / juce::jmax (1, b - a)) + 1.0e-9;
    };

    // ---- 1) sustain region from the loudness envelope (30 ms windows every 10 ms) ----
    const int hop = juce::jmax (1, (int) (fs * 0.01)), ew = (int) (fs * 0.03);
    std::vector<double> env;
    for (int c = ew / 2; c < len - ew / 2; c += hop) env.push_back (rms (c, ew));
    if (env.size() < 10)
        return;
    const double mx = *std::max_element (env.begin(), env.end());
    std::vector<double> loud;
    for (double e : env) if (e >= 0.3 * mx) loud.push_back (e);
    std::nth_element (loud.begin(), loud.begin() + (long) loud.size() / 2, loud.end());
    const double ref = loud[loud.size() / 2];                      // typical sustain level

    int f0 = 0, f1 = (int) env.size() - 1;
    while (f0 < f1 && env[(size_t) f0] < 0.7 * ref) ++f0;          // end of the attack
    while (f1 > f0 && env[(size_t) f1] < 0.7 * ref) --f1;          // start of the fade-out
    int sStart = ew / 2 + f0 * hop + (int) (fs * 0.08);
    int sEnd   = ew / 2 + f1 * hop - (int) (fs * 0.04);
    if (sEnd - sStart < (int) (fs * 0.3))
    {   // no clear sustain: use the middle of the sample
        sStart = (int) (len * 0.25);
        sEnd = (int) (len * 0.92);
    }
    sEnd = juce::jmin (sEnd, len - W - 2);
    sStart = juce::jmax (sStart, W + 1);
    const int span = sEnd - sStart;
    if (span < (int) (fs * 0.15))
        return;

    std::vector<int> zc;
    for (int i = sStart; i < sEnd; ++i)
        if (x[i - 1] <= 0.0f && x[i] > 0.0f)
            zc.push_back (i);
    if (zc.size() < 4)
        return;

    const int lvlWin = (int) (fs * 0.05);
    auto score = [&] (int a, int e)
    {
        double diff = 0.0, en = 1.0e-12;
        for (int k = -W; k < W; ++k)
        {
            const double p = x[a + k], q = x[e + k];
            diff += (p - q) * (p - q);
            en += p * p + q * q;
        }
        const double lvl = std::abs (std::log (rms (a, lvlWin) / rms (e, lvlWin)));
        return diff / en + 1.5 * lvl - 0.08 * (double) (e - a) / (double) span;   // prefer longer loops a little
    };

    // ---- 2) best pair ----
    const int minLoop = juce::jmin ((int) (span * 0.8), juce::jmax ((int) (fs * 0.3), (int) (span * 0.35)));
    double bestScore = 1.0e30;
    int bestA = -1, bestE = -1;

    for (double ef : { 1.0, 0.9, 0.8, 0.7 })
    {
        const int target = sStart + (int) (span * ef);
        auto it = std::upper_bound (zc.begin(), zc.end(), target);
        if (it == zc.begin())
            continue;
        const int e = *(--it);

        std::vector<int> cand;
        for (int z : zc)
            if (z <= e - minLoop)
                cand.push_back (z);
        if (cand.empty())
            continue;

        const int stride = juce::jmax (1, (int) cand.size() / 1200);
        int coarse = -1;
        double cs = 1.0e30;
        for (size_t i = 0; i < cand.size(); i += (size_t) stride)
        {
            const double sc = score (cand[i], e);
            if (sc < cs) { cs = sc; coarse = (int) i; }
        }
        for (int i = juce::jmax (0, coarse - stride); i <= juce::jmin ((int) cand.size() - 1, coarse + stride); ++i)
        {
            const double sc = score (cand[(size_t) i], e);
            if (sc < bestScore) { bestScore = sc; bestA = cand[(size_t) i]; bestE = e; }
        }
    }

    if (bestA >= 0 && bestE - 1 > bestA + 8)
    {
        const int loopLen = bestE - bestA;
        s.loopStart = bestA;
        s.loopEnd = bestE - 1;     // the engine wraps at loopEnd + 1 = bestE, landing exactly on bestA
        // ---- 3) crossfade ----
        const int xf = juce::jlimit ((int) (fs * 0.01), (int) (fs * 0.15), loopLen / 4);
        s.xfade = juce::jmin (xf, bestA, loopLen / 2);
    }
}

//==============================================================================
std::shared_ptr<NaySample> NayAudioProcessor::cloneSample (const NaySample& s)
{
    auto c = std::make_shared<NaySample>();
    c->fileName = s.fileName;
    c->sampleRate = s.sampleRate;
    c->data.makeCopyOf (s.data);
    c->root = s.root.load(); c->lo = s.lo.load(); c->hi = s.hi.load(); c->vlo = s.vlo.load(); c->vhi = s.vhi.load();
    c->loopStart = s.loopStart.load(); c->loopEnd = s.loopEnd.load();
    c->tune = s.tune.load(); c->xfade = s.xfade.load();
    c->detectInfo = s.detectInfo;
    return c;
}

// Wave editor: DELETE / TRIM / FADE IN / FADE OUT / NORMALIZE / SILENCE / REVERSE on the range [start, end)
std::shared_ptr<NaySample> NayAudioProcessor::applyWaveOp (const NaySample& s, WaveOp op, int start, int end)
{
    const int len = s.data.getNumSamples(), nc = s.data.getNumChannels();
    start = juce::jlimit (0, len, start);
    end = juce::jlimit (start, len, end);
    if (op != WaveOp::Normalize && end - start < 2)
        return nullptr;

    auto c = cloneSample (s);
    int ls = s.loopStart.load(), le = s.loopEnd.load();
    const bool hasLoop = ls >= 0 && le > ls;

    if (op == WaveOp::Delete || op == WaveOp::Trim)
    {
        const int keepA = op == WaveOp::Trim ? start : 0;
        const int newLen = op == WaveOp::Trim ? end - start : len - (end - start);
        if (newLen < 64)
            return nullptr;
        juce::AudioBuffer<float> nb (nc, newLen);
        for (int ch = 0; ch < nc; ++ch)
        {
            if (op == WaveOp::Trim)
                nb.copyFrom (ch, 0, s.data, ch, keepA, newLen);
            else
            {
                if (start > 0)   nb.copyFrom (ch, 0, s.data, ch, 0, start);
                if (end < len)   nb.copyFrom (ch, start, s.data, ch, end, len - end);
                // tiny crossfade at the cut so it doesn't click
                const int fx = juce::jmin (64, start, len - end);
                for (int k = 0; k < fx; ++k)
                {
                    const float t = (float) (k + 1) / (float) (fx + 1);
                    const float before = s.data.getSample (ch, start - fx + k);
                    nb.setSample (ch, start - fx + k, before * (1.0f - t) + s.data.getSample (ch, end - fx + k) * t);
                }
            }
        }
        c->data = std::move (nb);

        if (hasLoop)
        {
            if (op == WaveOp::Trim)
            {
                ls -= start; le -= start;
                if (ls < 0 || le >= newLen) ls = le = -1;
            }
            else
            {
                const int cut = end - start;
                if (le < start)              { /* loop before the cut: unchanged */ }
                else if (ls >= end)          { ls -= cut; le -= cut; }
                else                         ls = le = -1;            // the loop was cut: needs a new one
            }
            c->loopStart = ls;
            c->loopEnd = le;
            if (ls < 0) c->xfade = -1;
        }
        return c;
    }

    if (op == WaveOp::Normalize && end - start < 2) { start = 0; end = len; }
    const int n = end - start;
    for (int ch = 0; ch < nc; ++ch)
    {
        float* d = c->data.getWritePointer (ch);
        switch (op)
        {
            case WaveOp::FadeIn:  for (int k = 0; k < n; ++k) d[start + k] *= (float) k / (float) n; break;
            case WaveOp::FadeOut: for (int k = 0; k < n; ++k) d[start + k] *= 1.0f - (float) (k + 1) / (float) n; break;
            case WaveOp::Silence: for (int k = 0; k < n; ++k) d[start + k] = 0.0f; break;
            case WaveOp::Reverse: std::reverse (d + start, d + end); break;
            default: break;
        }
    }
    if (op == WaveOp::Normalize)
    {
        const float pk = c->data.getMagnitude (start, n);
        if (pk > 1.0e-6f)
            c->data.applyGain (start, n, juce::Decibels::decibelsToGain (-0.3f) / pk);
    }
    return c;
}

void NayAudioProcessor::replaceSample (int index, int sampleIndex, std::shared_ptr<NaySample> s)
{
    if (s == nullptr || index < 0 || index >= (int) groups.size() || groups[(size_t) index].bank == nullptr)
        return;
    auto nb = std::make_shared<NayBank> (*groups[(size_t) index].bank);
    if (sampleIndex < 0 || sampleIndex >= (int) nb->size())
        return;
    (*nb)[(size_t) sampleIndex] = s;
    setBank (index, nb);
    groups[(size_t) index].dirty = true;
}

void NayAudioProcessor::startPreview (std::shared_ptr<NaySample> s)
{
    {
        juce::SpinLock::ScopedLockType lock (bankLock);
        previewReq = std::move (s);
    }
    previewCmd = 1;
    previewOn = true;
}

void NayAudioProcessor::stopPreview()
{
    previewCmd = 2;
    previewOn = false;
}

//==============================================================================
void NayAudioProcessor::addGroup (const juce::String& name, const juce::String& path, const juce::String& category)
{
    for (const auto& g : groups)
        if (g.path == path)
            return; // already added

    NayGroup ng;
    ng.name = name;
    ng.path = path;
    ng.category = category;
    groups.push_back (std::move (ng));
}

// The library and the factory presets live WITH the plugin (no ProgramData / C: folders):
//  1) inside the plugin bundle:   Egyptian Sampler.vst3\Contents\Resources\Library   and   ...\Factory Presets
//  2) or next to the .vst3 file:  "Egyptian Sampler Data"\Library   and   ...\Factory Presets
//  (Standalone: "Egyptian Sampler Data" next to the .exe)
juce::File NayAudioProcessor::dataRoot()
{
    static const juce::File root = []() -> juce::File
    {
        const auto self = juce::File::getSpecialLocation (juce::File::currentApplicationFile);

        juce::File bundle;
        for (auto d = self; d != d.getParentDirectory(); d = d.getParentDirectory())
            if (d.isDirectory() && d.getFileName().endsWithIgnoreCase (".vst3"))
            {
                bundle = d;
                break;
            }

        std::vector<juce::File> cands;
        if (bundle != juce::File())
        {
            cands.push_back (bundle.getChildFile ("Contents").getChildFile ("Resources"));
            cands.push_back (bundle.getSiblingFile ("Egyptian Sampler Data"));
        }
        else
        {
            cands.push_back (self.getParentDirectory().getChildFile ("Egyptian Sampler Data"));
            cands.push_back (self.getParentDirectory());
        }

        for (const auto& c : cands)
            if (c.getChildFile ("Library").isDirectory() || c.getChildFile ("Factory Presets").isDirectory())
                return c;

        return cands.front();
    }();

    return root;
}
juce::File NayAudioProcessor::libraryRoot()       { return dataRoot().getChildFile ("Library"); }
juce::File NayAudioProcessor::factoryPresetDir()  { return dataRoot().getChildFile ("Factory Presets"); }
juce::File NayAudioProcessor::userPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Egyptian Sampler").getChildFile ("User Presets");
}

// User instruments: "Egyptian Sampler Data\Instruments" next to the .vst3 (or the Standalone .exe). If Windows does
// not allow writing there (e.g. C:\Program Files), Documents\Egyptian Sampler\Instruments is used instead.
juce::File NayAudioProcessor::instrumentsDir()
{
    static const juce::File dir = []() -> juce::File
    {
        auto canWrite = [] (const juce::File& d)
        {
            if (! d.createDirectory().wasOk())
                return false;
            const auto t = d.getChildFile (".write_test");
            const bool ok = t.replaceWithText ("ok");
            t.deleteFile();
            return ok;
        };

        const auto self = juce::File::getSpecialLocation (juce::File::currentApplicationFile);
        juce::File base = self.getParentDirectory();
        for (auto d = self; d != d.getParentDirectory(); d = d.getParentDirectory())
            if (d.isDirectory() && d.getFileName().endsWithIgnoreCase (".vst3"))
            {
                base = d.getParentDirectory();
                break;
            }

        const auto nextToPlugin = base.getChildFile ("Egyptian Sampler Data").getChildFile ("Instruments");
        if (canWrite (nextToPlugin))
            return nextToPlugin;

        auto docs = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("Egyptian Sampler").getChildFile ("Instruments");
        docs.createDirectory();
        return docs;
    }();
    return dir;
}

// Paths are stored relative ("@LIB/Stringed/Oud", "@INST/Nay.esi") so presets / projects work on any computer.
juce::String NayAudioProcessor::toStoredPath (const juce::String& path)
{
    for (const auto& [tag, root] : { std::pair<juce::String, juce::File> { "@LIB/", libraryRoot() },
                                     std::pair<juce::String, juce::File> { "@INST/", instrumentsDir() } })
    {
        const auto prefix = root.getFullPathName() + juce::File::getSeparatorString();
        if (root.getFullPathName().isNotEmpty() && path.startsWith (prefix))
            return tag + path.substring (prefix.length()).replace ("\\", "/");
    }
    return path;
}

juce::String NayAudioProcessor::fromStoredPath (const juce::String& stored)
{
    if (stored.startsWith ("@LIB/"))
        return libraryRoot().getChildFile (stored.substring (5)).getFullPathName();
    if (stored.startsWith ("@INST/"))
        return instrumentsDir().getChildFile (stored.substring (6)).getFullPathName();
    return stored;
}

int NayAudioProcessor::scanInstruments (const juce::File& dir)
{
    const int before = (int) groups.size();
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.esi");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    for (const auto& f : files)
    {
        juce::String name, cat;
        if (readEsiHeader (f, name, cat))
            addGroup (name, f.getFullPathName(), cat);
    }
    return (int) groups.size() - before;
}

void NayAudioProcessor::setBank (int index, std::shared_ptr<const NayBank> nb)
{
    groups[(size_t) index].bank = nb;
    juce::SpinLock::ScopedLockType lock (bankLock);
    if (index == activeGroup) bankA = nb;
    if (index == duoGroup)    bankB = nb;
}

int NayAudioProcessor::createInstrument (const juce::String& name)
{
    const auto clean = name.trim().isEmpty() ? juce::String ("New Instrument") : name.trim();
    const auto file = instrumentsDir().getNonexistentChildFile (juce::File::createLegalFileName (clean), ".esi", false);

    NayGroup g;
    g.name = clean;
    g.path = file.getFullPathName();
    g.category = "My Instruments";
    g.bank = std::make_shared<NayBank>();
    groups.push_back (std::move (g));

    const int idx = (int) groups.size() - 1;
    writeEsi (file, clean, "My Instruments", *groups[(size_t) idx].bank);
    selectGroup (idx);
    sampleInfo = clean + ": empty - drop WAV files on the mapping";
    return idx;
}

// Drag & drop: the audio is COPIED into the instrument file, so the original WAVs can be moved or deleted later.
// Root note: from the file name when it is a note ("C4", "F#3", "60"), otherwise detected from the sound.
juce::String NayAudioProcessor::addSamples (const juce::StringArray& paths)
{
    juce::Array<juce::File> files;
    for (const auto& p : paths)
    {
        const juce::File f (p);
        if (f.isDirectory())
            files.addArray (f.findChildFiles (juce::File::findFiles, true, kAudioWildcard));
        else if (f.hasFileExtension ("wav;aif;aiff;flac"))
            files.add (f);
    }
    if (files.isEmpty())
        return "No audio files (WAV / AIFF / FLAC) in the drop";

    int idx = activeGroup;
    if (! isUserInstrument (idx))
    {
        const auto parent = files[0].getParentDirectory().getFileName();
        idx = createInstrument (parent.isNotEmpty() ? parent : juce::String ("New Instrument"));
    }
    ensureLoaded (idx);

    auto nb = std::make_shared<NayBank> (*groups[(size_t) idx].bank);
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    int added = 0, byName = 0, detected = 0, failed = 0;
    for (const auto& f : files)
    {
        auto s = readAudioFile (f, fm);
        if (s == nullptr) { ++failed; continue; }

        // unique name inside this instrument (two "C4.wav" from different folders don't overwrite each other)
        auto nm = s->fileName;
        for (int k = 2; std::any_of (nb->begin(), nb->end(), [&] (const auto& o) { return o->fileName == nm; }); ++k)
            nm = f.getFileNameWithoutExtension() + " (" + juce::String (k) + ")" + f.getFileExtension();
        s->fileName = nm;

        const int r = parseRootNote (f.getFileNameWithoutExtension());
        if (r >= 0) { s->root = r; s->detectInfo = "from file name"; ++byName; }
        else        { detectRoot (*s); ++detected; }

        if (s->loopStart.load() < 0)
            autoLoop (*s);

        nb->push_back (s);
        ++added;
    }

    std::sort (nb->begin(), nb->end(), [] (const auto& a, const auto& b) { return a->root.load() < b->root.load(); });
    autoZones (*nb);
    setBank (idx, nb);

    auto& g = groups[(size_t) idx];
    g.dirty = ! writeEsi (juce::File (g.path), g.name, g.category, *nb);

    sampleInfo = g.name + ": " + juce::String ((int) nb->size()) + " samples";
    juce::String msg = "Added " + juce::String (added) + " sample" + (added == 1 ? "" : "s")
                     + "  (" + juce::String (byName) + " by name, " + juce::String (detected) + " detected)";
    if (failed > 0) msg << ",  " << failed << " could not be read";
    if (g.dirty)    msg << ",  NOT SAVED (cannot write " << g.path << ")";
    return msg;
}

bool NayAudioProcessor::saveInstrument (int index)
{
    if (index < 0 || index >= (int) groups.size())
        return false;
    ensureLoaded (index);
    auto& g = groups[(size_t) index];

    if (! g.isUser())
    {   // a folder instrument becomes a user instrument (one file, protected)
        g.path = instrumentsDir().getNonexistentChildFile (juce::File::createLegalFileName (g.name), ".esi", false).getFullPathName();
        g.category = "My Instruments";
    }
    g.zones.clear();
    const bool ok = writeEsi (juce::File (g.path), g.name, g.category, *g.bank);
    g.dirty = ! ok;
    sampleInfo = g.name + (ok ? ": saved" : ": SAVE FAILED");
    return ok;
}

void NayAudioProcessor::removeSample (int index, int sampleIndex)
{
    if (index < 0 || index >= (int) groups.size() || groups[(size_t) index].bank == nullptr)
        return;
    auto nb = std::make_shared<NayBank> (*groups[(size_t) index].bank);
    if (sampleIndex < 0 || sampleIndex >= (int) nb->size())
        return;
    nb->erase (nb->begin() + sampleIndex);
    setBank (index, nb);
    groups[(size_t) index].dirty = true;
    sampleInfo = groups[(size_t) index].name + ": " + juce::String ((int) nb->size()) + " samples";
}

// Finds instrument folders. Category = the folder that contains the instrument folders (e.g. "Stringed" > "Oud").
int NayAudioProcessor::scanFolder (const juce::File& folder)
{
    const int before = (int) groups.size();
    auto sortedDirs = [] (const juce::File& f)
    {
        auto d = f.findChildFiles (juce::File::findDirectories, false);
        std::sort (d.begin(), d.end(), [] (const juce::File& a, const juce::File& b)
                   { return a.getFileName().compareNatural (b.getFileName()) < 0; });
        return d;
    };

    if (folderHasSamples (folder))
    {
        addGroup (folder.getFileName(), folder.getFullPathName(), "My Instruments");
    }
    else
    {
        for (const auto& s : sortedDirs (folder))
        {
            if (folderHasSamples (s))
                addGroup (s.getFileName(), s.getFullPathName(), folder.getFileName());
            else
                for (const auto& s2 : sortedDirs (s))
                    if (folderHasSamples (s2))
                        addGroup (s2.getFileName(), s2.getFullPathName(), s.getFileName());
        }
    }

    return (int) groups.size() - before;
}

int NayAudioProcessor::addFolder (const juce::File& folder)
{
    const int firstNew = (int) groups.size();
    scanFolder (folder);
    const int added = (int) groups.size() - firstNew;

    if (added > 0)
        selectGroup (firstNew);
    else
        sampleInfo = "No new samples found (name files like C4.wav)";

    return added;
}

void NayAudioProcessor::ensureLoaded (int index)
{
    auto& g = groups[(size_t) index];
    if (g.bank == nullptr)
        g.bank = g.isUser() ? loadEsi (juce::File (g.path), g.zones) : loadBank (juce::File (g.path), g.zones);
}

void NayAudioProcessor::selectGroup (int index)
{
    if (index < 0 || index >= (int) groups.size())
        return;

    ensureLoaded (index);
    activeGroup = index;

    {
        juce::SpinLock::ScopedLockType lock (bankLock);
        bankA = groups[(size_t) index].bank;
    }

    sampleInfo = groups[(size_t) index].name + ": " + juce::String ((int) groups[(size_t) index].bank->size()) + " samples";
}

void NayAudioProcessor::selectDuo (int index)
{
    if (index < 0 || index >= (int) groups.size())
    {
        duoGroup = -1;
        juce::SpinLock::ScopedLockType lock (bankLock);
        bankB = nullptr;
        return;
    }

    ensureLoaded (index);
    duoGroup = index;

    juce::SpinLock::ScopedLockType lock (bankLock);
    bankB = groups[(size_t) index].bank;
}

void NayAudioProcessor::renameGroup (int index, const juce::String& newName)
{
    if (index < 0 || index >= (int) groups.size() || newName.trim().isEmpty())
        return;

    groups[(size_t) index].name = newName.trim();
    if (groups[(size_t) index].isUser())
        saveInstrument (index);     // the name lives inside the .esi file

    if (index == activeGroup && groups[(size_t) index].bank != nullptr)
        sampleInfo = groups[(size_t) index].name + ": " + juce::String ((int) groups[(size_t) index].bank->size()) + " samples";
}

void NayAudioProcessor::resetZones (int index)
{
    if (index < 0 || index >= (int) groups.size() || groups[(size_t) index].bank == nullptr)
        return;

    groups[(size_t) index].zones.clear();

    // re-parse roots from file names (detected roots are kept), then recompute zones
    for (const auto& s : *groups[(size_t) index].bank)
    {
        const int r = parseRootNote (juce::File (s->fileName).getFileNameWithoutExtension());
        if (r >= 0)
            s->root = r;
    }

    autoZones (*groups[(size_t) index].bank);
    if (groups[(size_t) index].isUser())
        groups[(size_t) index].dirty = true;
}

void NayAudioProcessor::syncZones()
{
    for (auto& g : groups)
        if (g.bank != nullptr)
        {
            g.zones.clear();
            for (const auto& s : *g.bank)
                g.zones.push_back ({ s->fileName, s->root.load(), s->lo.load(), s->hi.load(), s->vlo.load(), s->vhi.load(),
                                     s->tune.load(), s->loopStart.load(), s->loopEnd.load(), s->xfade.load() });
        }
}

//==============================================================================
double NayAudioProcessor::noteTarget (int note) const
{
    return (double) note + (double) off[((note - curShift) % 12 + 12) % 12] * 0.01;
}

// Zone lookup + background sample variation.
// The closest sample for the key is found with the Kontakt-style zones (key range + velocity range). Then every
// key press picks a RANDOM sample among the neighbours of that sample (one or two roots above / below) in the
// same instrument. The sample is pitch-shifted by the engine (rate = 2^((note - root)/12)), so the pressed key
// always sounds at its own pitch - the maqam / quarter-tone tuning is not affected. Never the same sample twice.
std::shared_ptr<NaySample> NayAudioProcessor::pickSample (const NayBank* b, int note, int vel, NaySample*& last)
{
    if (b == nullptr)
        return nullptr;

    int bestD = 100000, bestRoot = 60;
    for (const auto& s : *b)
        if (note >= s->lo.load() && note <= s->hi.load() && vel >= s->vlo.load() && vel <= s->vhi.load())
        {
            const int d = std::abs (s->root.load() - note);
            if (d < bestD) { bestD = d; bestRoot = s->root.load(); }
        }

    if (bestD == 100000)
        return nullptr;

    // which roots exist for this velocity
    bool present[128] = {};
    for (const auto& s : *b)
        if (vel >= s->vlo.load() && vel <= s->vhi.load())
            present[juce::jlimit (0, 127, s->root.load())] = true;

    bool allowed[128] = {};
    allowed[juce::jlimit (0, 127, bestRoot)] = true;
    for (int dir = -1; dir <= 1; dir += 2)
    {
        int found = 0;
        for (int r = bestRoot + dir; r >= 0 && r <= 127 && found < 1 && std::abs (r - bestRoot) <= 1; r += dir)   // only the closest neighbour (within 1 semitone)
            if (present[r]) { allowed[r] = true; ++found; }
    }

    int idx[64];
    int n = 0;
    for (int i = 0; i < (int) b->size() && n < 64; ++i)
    {
        const auto& s = *(*b)[(size_t) i];
        if (vel >= s.vlo.load() && vel <= s.vhi.load() && allowed[juce::jlimit (0, 127, s.root.load())])
            idx[n++] = i;
    }

    if (n == 0)
        return nullptr;

    if (getP (P_varOn) < 0.5f)
    {
        last = (*b)[(size_t) idx[0]].get();
        return (*b)[(size_t) idx[0]];
    }

    int pick = rng.nextInt (n);
    if (n > 1 && (*b)[(size_t) idx[pick]].get() == last)
        pick = (pick + 1 + rng.nextInt (n - 1)) % n;     // never the same sample twice in a row

    last = (*b)[(size_t) idx[pick]].get();
    return (*b)[(size_t) idx[pick]];
}

void NayAudioProcessor::applyVelocity (NayVoice& v, int vel)
{
    const float lin = juce::jlimit (0.f, 1.f, (float) vel / 127.f);
    v.velGain = (1.0f - pVelSens) + pVelSens * std::pow (lin, 1.3f);
    const float soft = 700.f + 15000.f * std::pow (lin, 1.4f); // soft = darker, hard = brighter
    v.velFc = 18000.f + (soft - 18000.f) * pVelSens;
}

void NayAudioProcessor::spawnGhost (const NayVoice& v, float releaseSec)
{
    if (! v.adsr.isActive() || (v.sa == nullptr && v.sb == nullptr && ! pOscOn))
        return;

    auto& g = ghosts[(size_t) (ghostIdx++ % (int) ghosts.size())];
    g = v;
    g.adsr.setSampleRate (sr);
    g.adsr.setParameters ({ 0.001f, 0.05f, 1.0f, juce::jmax (0.003f, releaseSec) });
    g.gate = false;
    g.xf = 1.0f;
    g.xfStep = 0.0f;
    g.adsr.noteOff();
}

void NayAudioProcessor::startVoice (NayVoice& v, int note, int vel,
                                    const std::shared_ptr<const NayBank>& a,
                                    const std::shared_ptr<const NayBank>& b)
{
    auto sa = pickSample (a.get(), note, vel, lastPicked);
    std::shared_ptr<NaySample> sb;
    if (b != nullptr)
        sb = pickSample (b.get(), note, vel, lastPickedB);

    if (sa == nullptr && sb == nullptr && ! pOscOn)
        return;

    spawnGhost (v, 0.012f);            // the old sound fades out quickly instead of being cut (no click)
    v.oscPh = 0.0;
    v.oscLp = 0.0f;
    v.oscTri = 0.0f;
    v.adsr.reset();
    v.xf = 1.0f;
    v.xfStep = 0.0f;
    v.sa = sa;
    v.sb = sb;
    v.posA = v.posB = 0.0;
    v.lp = v.lpR = 0.0f;
    v.lpB = v.lpBR = 0.0f;
    v.preview = false;
    v.note = note;
    v.target = noteTarget (note);
    v.pitch = v.target;
    v.detune = 0.0f;
    v.vibPhase = 0.0;
    v.glideMs = -1.0f;
    applyVelocity (v, vel);
    v.gate = true;
    v.age = ++ageCounter;
    v.adsr.noteOn();
}

void NayAudioProcessor::allNotesOff()
{
    held.clear();
    for (auto& v : voices)
    {
        v.gate = false;
        v.adsr.noteOff();
    }
}

void NayAudioProcessor::handleMidi (const juce::MidiMessage& msg,
                                    const std::shared_ptr<const NayBank>& a,
                                    const std::shared_ptr<const NayBank>& b)
{
    if (msg.isNoteOn())
    {
        const int n = msg.getNoteNumber();
        int vel = juce::jlimit (1, 127, (int) msg.getVelocity());
        if (getP (P_portaOn) > 0.5f)
        {   // MAGIC LEGATO / Voice Edit: touch sensitivity (depth / offset) then velocity limit
            const float depth = getP (P_tDepth) / 127.0f;
            vel = juce::jlimit (1, 127, (int) std::round (getP (P_tOff) + (float) (vel - 64) * depth));
            const int lo = (int) getP (P_velLo), hi = juce::jmax (lo, (int) getP (P_velHi));
            vel = juce::jlimit (lo, hi, vel);
        }
        heldVel[n] = vel;
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const double gapMs = nowMs - lastNoteOnMs;
        lastNoteOnMs = nowMs;

        if (pMono)
        {
            const bool wasHeld = ! held.empty();
            held.erase (std::remove (held.begin(), held.end(), n), held.end());
            held.push_back (n);

            auto& v = voices[0];
            const bool magicOn = getP (P_portaOn) > 0.5f;
            const bool recent = magicOn && v.adsr.isActive() && gapMs < 150.0;   // fast key after key still counts as legato
            if (pLegato && (wasHeld || recent) && v.adsr.isActive() && (v.sa != nullptr || v.sb != nullptr || pOscOn))
            {
                // legato: no retrigger, just glide
                v.note = n;
                v.target = noteTarget (n);
                if (getP (P_portaOn) > 0.5f)
                {   // MAGIC LEGATO portamento time: base -> fast playing -> velocity -> minimum
                    auto ms = [] (float t) { const float x = t / 127.0f; return 500.0f * x * x; };
                    float t = ms (getP (P_magicTime));
                    if (gapMs < (double) getP (P_fastThr) * 8.0)
                        t = ms (getP (P_fastTime));
                    t *= std::exp (-((float) vel - getP (P_vRef)) / 127.0f * (getP (P_vSens) / 127.0f) * 4.0f);
                    v.glideMs = juce::jmax (t, ms (getP (P_minTime)));

                    // crossfade between the old and the new sample like the Yamaha / Kronos "crossfade portamento":
                    // slow key press = slow blend, fast playing = fast blend, harder touch = quicker
                    double xfMs = juce::jlimit (8.0, 350.0, gapMs * 0.4);
                    if (gapMs < (double) getP (P_fastThr) * 8.0)
                        xfMs = 8.0 + (double) ms (getP (P_fastTime));
                    xfMs *= (double) std::exp (-((float) vel - getP (P_vRef)) / 127.0f * (getP (P_vSens) / 127.0f) * 4.0f);
                    xfMs = juce::jmax (xfMs, 6.0 + (double) ms (getP (P_minTime)));

                    auto sa2 = pickSample (a.get(), n, vel, lastPicked);
                    std::shared_ptr<NaySample> sb2;
                    if (b != nullptr)
                        sb2 = pickSample (b.get(), n, vel, lastPickedB);
                    if (sa2 != nullptr || sb2 != nullptr)
                    {
                        spawnGhost (v, (float) (xfMs * 0.001));
                        v.sa = sa2;
                        v.sb = sb2;
                        v.posA = v.posB = 0.0;
                        v.lp = v.lpB = v.lpR = v.lpBR = 0.0f;
                        v.pitch = v.target;
                        v.glideMs = 0.0f;
                        v.xf = 0.0f;
                        v.xfStep = (float) (1.0 / juce::jmax (1.0, xfMs * 0.001 * sr));
                    }
                }
                else
                    v.glideMs = -1.0f;
                applyVelocity (v, vel);
                v.gate = true;
            }
            else
            {
                startVoice (v, n, vel, a, b);
            }
        }
        else
        {
            NayVoice* freeVoice = nullptr;
            NayVoice* oldest = &voices[0];

            for (auto& v : voices)
            {
                if (! v.adsr.isActive()) { freeVoice = &v; break; }
                if (v.age < oldest->age) oldest = &v;
            }

            startVoice (freeVoice != nullptr ? *freeVoice : *oldest, n, vel, a, b);
        }
    }
    else if (msg.isNoteOff())
    {
        const int n = msg.getNoteNumber();

        if (pMono)
        {
            held.erase (std::remove (held.begin(), held.end(), n), held.end());
            auto& v = voices[0];

            if (held.empty())
            {
                v.gate = false;
                v.adsr.noteOff();
            }
            else if (v.note == n)
            {
                if (pLegato)
                {
                    v.note = held.back();
                    v.target = noteTarget (v.note);
                }
                else
                {
                    startVoice (v, held.back(), juce::jmax (1, heldVel[held.back()]), a, b);
                }
            }
        }
        else
        {
            for (auto& v : voices)
                if (v.gate && v.note == n)
                {
                    v.gate = false;
                    v.adsr.noteOff();
                }
        }
    }
    else if (msg.isController() && msg.getControllerNumber() == 1)
    {
        modWheel = (float) msg.getControllerValue() / 127.0f;
    }
    else if (msg.isController() && msg.getControllerNumber() == expCC.load())
    {
        expr = (float) msg.getControllerValue() / 127.0f; // expression pedal: dynamics (volume + brightness)
    }
    else if (msg.isPitchWheel())
    {
        bendNorm = ((float) msg.getPitchWheelValue() - 8192.0f) / 8192.0f;
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        allNotesOff();
    }
}

//==============================================================================
void NayAudioProcessor::renderVoice (NayVoice& v, juce::AudioBuffer<float>& buf, int start, int num)
{
    const bool oscHere = pOscOn && ! v.preview;            // the oscillator never plays in the wave-editor preview
    const bool solo = pOscOnly && ! v.preview;             // oscillator SOLO: the samples are silent
    if (num <= 0 || (v.sa == nullptr && v.sb == nullptr && ! oscHere) || ! v.adsr.isActive())
        return;

    NaySample* ss[2] = { v.sa.get(), v.sb.get() };
    double* pos[2] = { &v.posA, &v.posB };

    const float* src[2][2] = { { nullptr, nullptr }, { nullptr, nullptr } };   // [layer][L/R] (mono sample: R = L)
    double lEnd[2] = { 0, 0 }, lStart[2] = { 0, 0 }, rateBase[2] = { 0, 0 }, xfl[2] = { 0, 0 };
    bool act[2] = { false, false }, eqPow[2] = { false, false };

    for (int k = 0; k < 2; ++k)
    {
        if (solo || ss[k] == nullptr || ss[k]->data.getNumSamples() < 4)
            continue;

        const int len = ss[k]->data.getNumSamples();
        src[k][0] = ss[k]->data.getReadPointer (0);
        src[k][1] = ss[k]->data.getReadPointer (ss[k]->data.getNumChannels() > 1 ? 1 : 0);
        lEnd[k] = (double) len - 1.0;
        lStart[k] = juce::jmin (lEnd[k] - 1.0, (double) len * 0.5); // default sustain loop = second half
        const int ls = ss[k]->loopStart.load(), le = ss[k]->loopEnd.load();
        if (ls >= 0 && le > ls + 8 && le < len)                      // loop from the WAV, AUTO LOOP or set by hand
        {
            lStart[k] = (double) ls;
            lEnd[k] = juce::jmin ((double) (le + 1), (double) len - 1.0);
        }
        // loop crossfade: the sample's own length (AUTO LOOP / wave editor) or a short automatic one
        const double loopLen = lEnd[k] - lStart[k];
        const int xs = ss[k]->xfade.load();
        xfl[k] = xs >= 0 ? (double) xs : juce::jmin (1024.0, loopLen * 0.4);
        xfl[k] = juce::jmax (0.0, juce::jmin (xfl[k], loopLen * 0.5, lStart[k]));
        eqPow[k] = xfl[k] > ss[k]->sampleRate * 0.03;                 // long fades: equal power (vibrato / chorus)
        // per-sample TUNE (cents) shifts the sample's own pitch
        rateBase[k] = (ss[k]->sampleRate / sr) * std::exp2 (-((double) ss[k]->root.load() - (double) ss[k]->tune.load() * 0.01) / 12.0);
        act[k] = true;
    }

    if (! act[0] && ! act[1] && ! oscHere)
        return;

    // oscillator channel (PolyBLEP saw / square, triangle from the square, sine)
    const float oscLpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (pOscCut, 0.45f * (float) sr) / (float) sr);
    auto polyBlep = [] (double t, double dt) -> double
    {
        if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    };

    float* L = buf.getWritePointer (0, start);
    float* R = buf.getNumChannels() > 1 ? buf.getWritePointer (1, start) : nullptr;

    const float gAL = v.preview ? 1.0f : mixAL, gAR = v.preview ? 1.0f : mixAR;
    const float gt = getP (P_portaOn) > 0.5f ? juce::jmax (0.0f, v.glideMs) : pGlide;   // MAGIC LEGATO replaces the GLIDE knob
    const double glideCoef = gt < 1.0f ? 1.0 : 1.0 - std::exp (-1.0 / ((double) gt * 0.001 * sr));
    const double depthCents = v.preview ? 0.0 : juce::jmin (100.0, (double) pVibDepth * (double) modWheel);   // vibrato only through the mod wheel
    const double bendSemis = v.preview ? 0.0 : (double) bendNorm * (double) pPbRange;
    const float lpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (v.velFc * (0.2f + 0.8f * exprSm), 0.45f * (float) sr) / (float) sr);

    float es = v.preview ? 1.0f : exprSm;
    for (int i = 0; i < num; ++i)
    {
        if (! v.preview) es += (expr - es) * 0.003f;
        v.pitch += (v.target - v.pitch) * glideCoef;

        v.vibPhase += (double) pVibRate / sr;
        if (v.vibPhase >= 1.0)
            v.vibPhase -= 1.0;
        const double vib = std::sin (juce::MathConstants<double>::twoPi * v.vibPhase) * depthCents * 0.01;

        const double ratio = std::exp2 ((v.pitch + bendSemis + vib + (double) v.detune) / 12.0);

        float m[2][2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
        for (int k = 0; k < 2; ++k)
        {
            if (! act[k])
                continue;

            double& p = *pos[k];
            while (p >= lEnd[k])
                p -= (lEnd[k] - lStart[k]);

            const int i0 = (int) p;
            const float frac = (float) (p - (double) i0);
            const bool inFade = xfl[k] > 1.0 && p > lEnd[k] - xfl[k];
            float ga = 1.0f, gb = 0.0f;
            int j0 = 0;
            float jf = 0.0f;
            if (inFade)
            {
                const double q = p - (lEnd[k] - lStart[k]);          // same spot one loop earlier (just before the loop start)
                j0 = (int) q;
                jf = (float) (q - (double) j0);
                const float t = (float) ((p - (lEnd[k] - xfl[k])) / xfl[k]);
                if (eqPow[k]) { ga = std::cos (t * 1.5707963f); gb = std::sin (t * 1.5707963f); }
                else          { ga = 1.0f - t; gb = t; }
            }
            for (int c = 0; c < 2; ++c)
            {
                const float* d = src[k][c];
                float smp = d[i0] + frac * (d[i0 + 1] - d[i0]);
                if (inFade)
                    smp = smp * ga + (d[j0] + jf * (d[j0 + 1] - d[j0])) * gb;
                m[k][c] = smp;
            }

            p += ratio * rateBase[k];
        }

        float osc = 0.0f;
        if (oscHere)
        {
            const double f = 440.0 * std::exp2 ((v.pitch + bendSemis + vib + (double) v.detune + (double) pOscSemis - 69.0) / 12.0);
            const double dt = juce::jlimit (1.0e-6, 0.49, f / sr);
            const double t = v.oscPh;
            double w = 0.0;
            switch (pOscWave)
            {
                case 0:  w = std::sin (juce::MathConstants<double>::twoPi * t); break;
                case 1:  w = 2.0 * t - 1.0 - polyBlep (t, dt); break;
                default:
                {
                    double sq = (t < 0.5 ? 1.0 : -1.0) + polyBlep (t, dt) - polyBlep (std::fmod (t + 0.5, 1.0), dt);
                    if (pOscWave == 3)
                    {   // leaky integration of the square = band-limited triangle
                        v.oscTri = (float) (dt * 4.0 * sq + (1.0 - dt * 0.05) * (double) v.oscTri);
                        sq = (double) v.oscTri;
                    }
                    w = sq;
                    break;
                }
            }
            v.oscPh += dt;
            if (v.oscPh >= 1.0) v.oscPh -= 1.0;
            v.oscLp += oscLpA * ((float) w - v.oscLp);
            osc = v.oscLp * pOscGain;
        }

        v.lp   += lpA * (m[0][0] - v.lp);
        v.lpR  += lpA * (m[0][1] - v.lpR);
        v.lpB  += lpA * (m[1][0] - v.lpB);
        v.lpBR += lpA * (m[1][1] - v.lpBR);
        if (v.xf < 1.0f)
            v.xf = juce::jmin (1.0f, v.xf + v.xfStep);
        const float env = v.adsr.getNextSample() * v.xf * v.velGain * pGain * std::pow (es, 1.5f);
        const float outL = (v.lp * gAL + v.lpB * mixBL + osc * mixOL) * env;
        const float outR = (v.lpR * gAR + v.lpBR * mixBR + osc * mixOR) * env;

        if (R != nullptr) { L[i] += outL; R[i] += outR; }
        else              L[i] += 0.5f * (outL + outR);

        if (! v.adsr.isActive())
            break;
    }
}

void NayAudioProcessor::renderAll (juce::AudioBuffer<float>& buf, int start, int num)
{
    if (num <= 0)
        return;

    if (pMono)
        renderVoice (voices[0], buf, start, num);
    else
        for (auto& v : voices)
            renderVoice (v, buf, start, num);

    for (auto& g : ghosts)
        renderVoice (g, buf, start, num);

    renderVoice (previewVoice, buf, start, num);

    exprSm += (expr - exprSm) * (1.0f - std::pow (0.997f, (float) num));
}

//==============================================================================
// KRONOS model: a smooth, heavy "saturated lead" (electric-guitar lead / Kronos style) - warm low end, soft
// rounded saturation curve, extra shine in the upper mids, and level matching so the instrument stays at the same
// loudness whether the effect is on or off, at any DRIVE / AMOUNT.
void NayAudioProcessor::processSaturation (juce::AudioBuffer<float>& buffer)
{
    if (! pSatOn && satEng[0] < 0.001f && satEng[1] < 0.001f)
        return;

    // KRONOS = the Yamaha PSR-A5000 insertion effect "Auto Wah + OD Hard" used for the electric-violin lead:
    //   static auto-wah (LFO 0 Hz, resonance 2.9) -> overdrive (hard, low drive) -> Dist EQ (low +4 dB, mid +6 dB)
    //   -> LPF 3.2 kHz -> insertion EQ (low shelf 560 Hz +3 dB) -> dry/wet ~50 %, level-matched to the dry signal.
    // knobs: DRIVE = overdrive, MID = Dist EQ mid gain (0..+12 dB), BASS = Dist EQ low gain (0..+12 dB), AMOUNT = dry/wet.
    const float drive = pSatDrive, mix = pSatMix;
    const float midDb = 12.0f * getP (P_satTone), lowDb = 12.0f * getP (P_satBass);
    const float fs = (float) sr;
    const float twoPi = juce::MathConstants<float>::twoPi;

    satWahBp.bandpass (fs, 900.0f, 2.9f);
    satDLow.lowShelf (fs, 250.0f, lowDb);
    satDMid.peaking (fs, 1200.0f, 0.9f, midDb);
    satLpf.lowpass (fs, juce::jmin (3200.0f, 0.45f * fs), 0.707f);
    satInLow.lowShelf (fs, 560.0f, 3.0f);

    const float pre = 1.0f + 18.0f * drive;
    const float aHp   = 1.0f - std::exp (-twoPi * 70.0f / fs);
    const float aAtt  = 1.0f - std::exp (-1.0f / (0.004f * fs));
    const float aRel  = 1.0f - std::exp (-1.0f / (0.18f * fs));
    const float aEnv  = 1.0f - std::exp (-1.0f / (0.05f * fs));
    const float aGain = 1.0f - std::exp (-1.0f / (0.02f * fs));

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    const float aEng = 1.0f - std::exp (-1.0f / (0.03f * fs));
    const float engTarget = pSatOn ? 1.0f : 0.0f;

    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = buffer.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            satEng[ch] += aEng * (engTarget - satEng[ch]);
            const float xd = d[i];

            satHpS[ch] += aHp * (xd - satHpS[ch]);
            float x = xd - satHpS[ch];

            const float ax = std::abs (x);                   // gentle levelling so quiet notes drive the overdrive too
            satLev[ch] += (ax > satLev[ch] ? aAtt : aRel) * (ax - satLev[ch]);
            x *= juce::jlimit (1.0f, 8.0f, std::pow (0.3f / (satLev[ch] + 1.0e-4f), 0.6f));

            x = 0.5f * x + 1.3f * satWahBp.tick (ch, x);     // static auto-wah: resonant mid peak

            float y = pre * x;                               // overdrive "hard": tanh plus a little hard clipping
            y = 0.6f * std::tanh (y) + 0.4f * juce::jlimit (-0.9f, 0.9f, y);

            y = satDLow.tick (ch, y);
            y = satDMid.tick (ch, y);
            y = satLpf.tick (ch, y);
            y = satInLow.tick (ch, y);

            const float hp = y - satDcX[ch] + 0.995f * satDcY[ch];
            satDcX[ch] = y;
            satDcY[ch] = hp;

            satEnvIn[ch]  += aEnv * (xd * xd - satEnvIn[ch]);
            satEnvOut[ch] += aEnv * (hp * hp - satEnvOut[ch]);
            if (satEnvIn[ch] > 1.0e-8f)
            {
                const float target = juce::jlimit (0.05f, 8.0f, std::sqrt (satEnvIn[ch] / (satEnvOut[ch] + 1.0e-9f)));
                satG[ch] += aGain * (target - satG[ch]);
            }

            const float wet = hp * satG[ch];
            d[i] = xd + (wet - xd) * mix * satEng[ch];
        }
    }
}

//==============================================================================
// YAMAHA = PSR-A5000 "Legacy - Auto Wah + Overdrive 1": LFO 0 Hz / depth 0 (static wah), cutoff offset 40,
// resonance 2.3, EQ low 225 Hz -7 dB, EQ high 2.8 kHz -12 dB, dry/wet D<W63, drive 10, Dist EQ low +5 / mid 0,
// LPF 3.6 kHz. Dark and tight. Knobs: DRIVE, WAH (cutoff), TONE (high shelf cut), AMOUNT (dry/wet).
void NayAudioProcessor::processYamaha (juce::AudioBuffer<float>& buffer)
{
    const bool on = pSatOn;                                  // one KRONOS button drives both stages
    if (! on && yamEng[0] < 0.001f && yamEng[1] < 0.001f)
        return;

    const float fs = (float) sr;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float mix = getP (P_yamMix), drive = getP (P_yamDrive);
    const float wahHz = 350.0f * std::pow (2.0f, 3.0f * getP (P_yamWah));   // 350 Hz .. 2.8 kHz
    const float hiDb = -24.0f * getP (P_yamTone);                           // 0.5 -> -12 dB like the organ

    {   // voice filter first (cutoff / resonance like the organ's SOUND page); neutral at 0
        const float cut = getP (P_kCut), res = getP (P_kRes);
        const float fc = cut < 0.0f ? 18000.0f * std::pow (2.0f, cut / 64.0f * 4.5f) : 18000.0f;
        yamVf.lowpass (fs, juce::jmin (fc, 0.45f * fs), 0.707f * std::pow (2.0f, res / 64.0f * 1.5f));
    }
    const bool vfOn = getP (P_kCut) < 0.0f || getP (P_kRes) != 0.0f;
    const float aEng = 1.0f - std::exp (-1.0f / (0.03f * fs));
    yamWah.bandpass (fs, wahHz, 2.3f);
    yamLo.lowShelf (fs, 225.0f, -7.0f);
    yamDLo.lowShelf (fs, 250.0f, 5.0f);
    const float shine = getP (P_yamShine);                                  // 0.5 = organ setting, up = brighter
    yamHi.highShelf (fs, 2800.0f, hiDb + 24.0f * (shine - 0.5f) * 0.6f);
    yamPres.peaking (fs, 3200.0f, 0.9f, 12.0f * shine);
    yamLpf.lowpass (fs, juce::jmin (3600.0f + 9000.0f * shine * shine, 0.45f * fs), 0.707f);

    const float pre = 1.0f + 14.0f * drive;
    const float aHp   = 1.0f - std::exp (-twoPi * 60.0f / fs);
    const float aAtt  = 1.0f - std::exp (-1.0f / (0.004f * fs));
    const float aRel  = 1.0f - std::exp (-1.0f / (0.18f * fs));
    const float aEnv  = 1.0f - std::exp (-1.0f / (0.05f * fs));
    const float aGain = 1.0f - std::exp (-1.0f / (0.02f * fs));

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = buffer.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            yamEng[ch] += aEng * ((on ? 1.0f : 0.0f) - yamEng[ch]);
            const float xd = d[i];

            yamHpS[ch] += aHp * (xd - yamHpS[ch]);
            float x = xd - yamHpS[ch];
            if (vfOn)
                x = yamVf.tick (ch, x);

            const float ax = std::abs (x);
            yamLev[ch] += (ax > yamLev[ch] ? aAtt : aRel) * (ax - yamLev[ch]);
            x *= juce::jlimit (1.0f, 6.0f, std::pow (0.3f / (yamLev[ch] + 1.0e-4f), 0.5f));

            x = 0.4f * x + 1.2f * yamWah.tick (ch, x);

            float y = std::tanh (pre * x);                   // overdrive 1: soft

            y = yamLo.tick (ch, y);
            y = yamHi.tick (ch, y);
            y = yamPres.tick (ch, y);
            y = yamDLo.tick (ch, y);
            y = yamLpf.tick (ch, y);

            const float hp = y - yamDcX[ch] + 0.995f * yamDcY[ch];
            yamDcX[ch] = y;
            yamDcY[ch] = hp;

            yamEnvIn[ch]  += aEnv * (xd * xd - yamEnvIn[ch]);
            yamEnvOut[ch] += aEnv * (hp * hp - yamEnvOut[ch]);
            if (yamEnvIn[ch] > 1.0e-8f)
            {
                const float target = juce::jlimit (0.05f, 8.0f, std::sqrt (yamEnvIn[ch] / (yamEnvOut[ch] + 1.0e-9f)));
                yamG[ch] += aGain * (target - yamG[ch]);
            }

            d[i] = xd + (hp * yamG[ch] - xd) * mix * yamEng[ch];
        }
    }
}

void NayAudioProcessor::processWah (juce::AudioBuffer<float>& buffer)
{
    if (! pWahOn)
        return;

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    const float k = 1.0f / 5.0f; // Q = 5
    const float mix = pWahMix;

    int i = 0;
    while (i < n)
    {
        const int chunk = juce::jmin (16, n - i);

        double pedal = (double) pWahPos;
        if (pWahRate > 0.05f)
            pedal += 0.5 * std::sin (juce::MathConstants<double>::twoPi * wahPhase);
        pedal = juce::jlimit (0.0, 1.0, pedal);

        const float fc = (float) (350.0 * std::pow (6.3, pedal));
        const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;

        for (int ch = 0; ch < chans; ++ch)
        {
            float* x = buffer.getWritePointer (ch) + i;
            float ic1 = wahIc1[ch], ic2 = wahIc2[ch];

            for (int j = 0; j < chunk; ++j)
            {
                const float v0 = x[j];
                const float v3 = v0 - ic2;
                const float v1 = a1 * ic1 + a2 * v3;
                const float v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1;
                ic2 = 2.0f * v2 - ic2;

                const float wet = k * v1 * 2.5f; // band-pass, level compensated
                x[j] = v0 + (wet - v0) * mix;
            }

            wahIc1[ch] = ic1;
            wahIc2[ch] = ic2;
        }

        wahPhase += (double) pWahRate * (double) chunk / sr;
        if (wahPhase >= 1.0)
            wahPhase -= 1.0;
        i += chunk;
    }
}

void NayAudioProcessor::processPhaser (juce::AudioBuffer<float>& buffer)
{
    if (! pPhOn)
        return;

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), 2);
    const float mix = pPhMix * 0.5f;

    int i = 0;
    while (i < n)
    {
        const int chunk = juce::jmin (8, n - i);

        for (int ch = 0; ch < chans; ++ch)
        {
            const double lfo = std::sin (juce::MathConstants<double>::twoPi * (phPhase + 0.25 * (double) ch));
            const double f = juce::jlimit (60.0, sr * 0.45, 800.0 * std::pow (2.0, (double) pPhDepth * 3.0 * lfo));
            const float t = std::tan (juce::MathConstants<float>::pi * (float) f / (float) sr);
            const float a = (t - 1.0f) / (t + 1.0f);

            float* x = buffer.getWritePointer (ch) + i;

            for (int j = 0; j < chunk; ++j)
            {
                const float in = x[j];
                float s = in + 0.5f * phFb[ch];

                for (int st = 0; st < 4; ++st)
                {
                    const float y = -a * s + phX1[ch][st] + a * phY1[ch][st];
                    phX1[ch][st] = s;
                    phY1[ch][st] = y;
                    s = y;
                }

                phFb[ch] = s;
                x[j] = in + (s - in) * mix;
            }
        }

        phPhase += (double) pPhRate * (double) chunk / sr;
        if (phPhase >= 1.0)
            phPhase -= 1.0;
        i += chunk;
    }
}

// Arpeggiator: turns the held notes into a stream of note on/off. Every note still goes through the
// maqam tuning table, so the arpeggio stays in the maqam (quarter tones included).
void NayAudioProcessor::runArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int n, double bpm)
{
    static const double beats[4] = { 1.0, 0.5, 1.0 / 3.0, 0.25 };
    const double stepLen = juce::jmax (64.0, 60.0 / juce::jmax (20.0, bpm) * sr * beats[juce::jlimit (0, 3, pArpRate)]);

    auto it = in.begin();
    for (int s = 0; s < n; ++s)
    {
        while (it != in.end() && (*it).samplePosition <= s)
        {
            const auto m = (*it).getMessage();
            ++it;

            if (m.isNoteOn())
            {
                if (pArpHold && physDown == 0)
                    arpNotes.clear();
                ++physDown;

                arpNotes.erase (std::remove_if (arpNotes.begin(), arpNotes.end(),
                                                [&] (const ArpNote& a) { return a.note == m.getNoteNumber(); }), arpNotes.end());
                arpNotes.push_back ({ m.getNoteNumber(), juce::jlimit (1, 127, (int) m.getVelocity()) });

                if (arpNotes.size() == 1)
                {
                    stepCd = 0.0;
                    arpIdx = 0;
                }
            }
            else if (m.isNoteOff())
            {
                physDown = juce::jmax (0, physDown - 1);
                if (! pArpHold)
                    arpNotes.erase (std::remove_if (arpNotes.begin(), arpNotes.end(),
                                                    [&] (const ArpNote& a) { return a.note == m.getNoteNumber(); }), arpNotes.end());

                if (arpNotes.empty() && arpCur >= 0)
                {
                    out.addEvent (juce::MidiMessage::noteOff (1, arpCur), s);
                    arpCur = -1;
                }
            }
            else
            {
                out.addEvent (m, s);
            }
        }

        if (arpCur >= 0 && (gateCd -= 1.0) <= 0.0)
        {
            out.addEvent (juce::MidiMessage::noteOff (1, arpCur), s);
            arpCur = -1;
        }

        if (arpNotes.empty())
        {
            stepCd = 0.0;
            continue;
        }

        if (stepCd <= 0.0)
        {
            if (arpCur >= 0)
            {
                out.addEvent (juce::MidiMessage::noteOff (1, arpCur), s);
                arpCur = -1;
            }

            std::vector<ArpNote> base = arpNotes;
            if (pArpMode != 4)
                std::sort (base.begin(), base.end(), [] (const ArpNote& a, const ArpNote& b) { return a.note < b.note; });

            std::vector<ArpNote> seq;
            for (int o = 0; o < pArpOct; ++o)
                for (const auto& bn : base)
                    if (bn.note + 12 * o <= 127)
                        seq.push_back ({ bn.note + 12 * o, bn.vel });

            if (pArpMode == 1)
                std::reverse (seq.begin(), seq.end());
            else if (pArpMode == 2 && seq.size() > 2)
                for (int k = (int) seq.size() - 2; k >= 1; --k)
                    seq.push_back (seq[(size_t) k]);

            if (! seq.empty())
            {
                const int idx = pArpMode == 3 ? rng.nextInt ((int) seq.size()) : (arpIdx++ % (int) seq.size());
                out.addEvent (juce::MidiMessage::noteOn (1, seq[(size_t) idx].note, (juce::uint8) seq[(size_t) idx].vel), s);
                arpCur = seq[(size_t) idx].note;
                gateCd = stepLen * (double) pArpGate;
            }
            stepCd += stepLen;
        }
        stepCd -= 1.0;
    }
}

void NayAudioProcessor::processDelay (juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const int size = delayBuf.getNumSamples();
    if (size < 16)
        return;

    const int d = juce::jlimit (1, size - 1, (int) ((double) pDelayTime * 0.001 * sr));
    const float fb = pDelayFb, mix = dbToGainOrOff (pDelayLvl);
    const int chans = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < chans; ++ch)
    {
        float* x = buffer.getWritePointer (ch);
        float* db = delayBuf.getWritePointer (ch);
        int w = delayWrite;

        for (int i = 0; i < n; ++i)
        {
            int r = w - d;
            if (r < 0)
                r += size;

            const float delayed = db[r];
            db[w] = x[i] + delayed * fb;
            x[i] += delayed * mix;

            if (++w >= size)
                w = 0;
        }
    }

    delayWrite = (delayWrite + n) % size;
}

void NayAudioProcessor::processReverb (juce::AudioBuffer<float>& buffer)
{
    juce::Reverb::Parameters p;
    p.roomSize = pRvSize;
    p.damping = 0.5f;
    const float wet = dbToGainOrOff (pRvLvl);
    p.wetLevel = wet;
    p.dryLevel = 1.0f - 0.4f * wet;
    p.width = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters (p);

    if (buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
    else
        reverb.processMono (buffer.getWritePointer (0), buffer.getNumSamples());
}

//==============================================================================
void NayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    std::shared_ptr<const NayBank> a, b;
    {
        juce::SpinLock::ScopedTryLockType lock (bankLock);
        if (lock.isLocked())
        {
            a = bankA;
            b = bankB;
        }
    }

    pGain      = getP (P_gain);
    pGlide     = getP (P_glide);
    pVibDepth  = getP (P_vibDepth);
    pVibRate   = getP (P_vibRate);
    pMono      = getP (P_mono) > 0.5f;
    pLegato    = getP (P_legato) > 0.5f;
    if (getP (P_portaOn) > 0.5f) { pMono = true; pLegato = true; }   // MAGIC LEGATO forces mono + legato
    pDelayTime = getP (P_delayTime);
    pDelayFb   = getP (P_delayFb);
    pDelayLvl  = getP (P_delayLevel);
    pRvSize    = getP (P_rvSize);
    pRvLvl     = getP (P_rvLevel);
    pVelSens   = getP (P_velOn) > 0.5f ? 0.85f : 0.0f;
    pPbRange   = getP (P_pbRange);
    pSatOn = getP (P_satOn) > 0.5f;   pSatDrive = getP (P_satDrive);  pSatMix = getP (P_satMix);
    pWahOn = getP (P_wahOn) > 0.5f;   pWahPos = getP (P_wahPos);      pWahRate = getP (P_wahRate);  pWahMix = getP (P_wahMix);
    pPhOn = getP (P_phOn) > 0.5f;     pPhRate = getP (P_phRate);      pPhDepth = getP (P_phDepth);  pPhMix = getP (P_phMix);
    pArpOn = getP (P_arpOn) > 0.5f;  pArpMode = (int) getP (P_arpMode);  pArpRate = (int) getP (P_arpRate);
    pArpOct = (int) getP (P_arpOct); pArpGate = getP (P_arpGate);        pArpHold = getP (P_arpHold) > 0.5f;
    pOscOn    = getP (P_oscOn) > 0.5f;
    pOscOnly  = pOscOn && getP (P_oscOnly) > 0.5f;
    pOscWave  = juce::jlimit (0, 3, (int) getP (P_oscWave));
    pOscGain  = dbToGainOrOff (getP (P_oscLevel)) * (pOscWave == 0 || pOscWave == 3 ? 0.5f : 0.35f);
    pOscSemis = getP (P_oscTune) + getP (P_oscFine) * 0.01f;
    pOscCut   = getP (P_oscCut);

    // fine-tune per note (the microtonal keys)
    for (int i = 0; i < 12; ++i)
        off[i] = getP (P_tuneBypass) > 0.5f ? 0.0f : offRaw[i]->load();

    // mixer: main / duo level, pan and mute (center = unity gain), master level
    {
        auto panGains = [] (float pan, float& gl, float& gr)
        {
            const float ang = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            gl = std::cos (ang) * 1.41421356f;
            gr = std::sin (ang) * 1.41421356f;
        };
        const float gA = getP (P_mainMute) > 0.5f ? 0.0f : dbToGainOrOff (getP (P_mainLevel));
        const float gB = getP (P_duoMute)  > 0.5f ? 0.0f : dbToGainOrOff (getP (P_duoLevel));
        float l, r;
        panGains (getP (P_mainPan), l, r);  mixAL = gA * l;  mixAR = gA * r;
        panGains (getP (P_duoPan),  l, r);  mixBL = gB * l;  mixBR = gB * r;
        const float gO = getP (P_oscMute) > 0.5f ? 0.0f : 1.0f;      // level is inside pOscGain
        panGains (getP (P_oscPan),  l, r);  mixOL = gO * l;  mixOR = gO * r;
        if (pOscOnly) { mixAL = mixAR = mixBL = mixBR = 0.0f; }      // SOLO: only the oscillator channel
        pMaster = dbToGainOrOff (getP (P_master));
    }

    // wave editor PLAY / STOP
    if (const int cmd = previewCmd.exchange (0))
    {
        auto& pv = previewVoice;
        if (cmd == 1)
        {
            std::shared_ptr<NaySample> req;
            {
                juce::SpinLock::ScopedLockType lock (bankLock);
                req = previewReq;
            }
            if (req != nullptr)
            {
                spawnGhost (pv, 0.01f);
                pv.sa = req; pv.sb = nullptr;
                pv.preview = true;
                pv.posA = 0.0;
                pv.lp = pv.lpR = pv.lpB = pv.lpBR = 0.0f;
                pv.note = req->root.load();
                pv.target = pv.pitch = (double) pv.note;
                pv.detune = 0.0f; pv.glideMs = 0.0f;
                pv.xf = 1.0f; pv.xfStep = 0.0f;
                pv.velGain = 1.0f; pv.velFc = 20000.0f;
                pv.adsr.setSampleRate (sr);
                pv.adsr.setParameters ({ 0.002f, 0.05f, 1.0f, 0.08f });
                pv.adsr.reset();
                pv.adsr.noteOn();
                pv.gate = true;
            }
        }
        else
        {
            pv.gate = false;
            pv.adsr.noteOff();
        }
    }

    if (pMono != lastMono)
    {
        allNotesOff(); // switching mono <-> poly releases everything
        lastMono = pMono;
    }

    const juce::ADSR::Parameters ap { getP (P_attack), 0.05f, 1.0f, getP (P_release) };
    for (auto& v : voices)
    {
        v.adsr.setParameters (ap);
        if (v.adsr.isActive())
            v.target = noteTarget (v.note); // live update when tuning / maqam changes
    }

    int pos = 0;
    const int total = buffer.getNumSamples();

    for (const auto metadata : midi)
    {
        const auto mm = metadata.getMessage();
        int k = 0, a = 0, b = 0;
        if (mm.isNoteOn())             { k = 1; a = mm.getNoteNumber(); b = mm.getVelocity(); }
        else if (mm.isNoteOff())       { k = 2; a = mm.getNoteNumber(); b = 0; }
        else if (mm.isController())
        {
            k = 3; a = mm.getControllerNumber(); b = mm.getControllerValue();

            const int lt = learnTarget.load();
            if (lt != -1)
            {
                if (lt == -2)
                {
                    expCC = a;
                    ccMap[a] = -1;
                }
                else
                {
                    if (expCC.load() == a) expCC = -1;                                  // the pedal forgets its old job
                    for (auto& m : ccMap) if (m.load() == lt) m = -1;                   // one CC per control
                    ccMap[a] = lt;
                }
                doneCC = a; doneTarget = lt; ++doneCount;
                learnTarget = -1;
            }
            else
            {
                const int pi = ccMap[a].load();
                if (pi >= 0 && pi < P_count && params[pi] != nullptr)
                    params[pi]->setValueNotifyingHost ((float) b / 127.0f);
            }
        }
        else if (mm.isPitchWheel())    { k = 4; a = mm.getPitchWheelValue(); }
        if (k != 0)
        {
            monKind = k; monA = a; monB = b;
            ++monCount;
        }
    }

    // TRANSPOSE (semitones) + OCTAVE (12 semitones): shifts the notes that get played. The note-off always
    // uses the same shifted note as its note-on, so changing the shift while holding keys never sticks a note.
    const int shiftSt = (int) std::lround (getP (P_transpose)) + 12 * (int) std::lround (getP (P_octave));
    curShift = ((shiftSt % 12) + 12) % 12;
    juce::MidiBuffer shifted;
    for (const auto metadata : midi)
    {
        auto m = metadata.getMessage();
        if (m.isNoteOn())
        {
            const int orig = m.getNoteNumber();
            const int nn = orig + shiftSt;
            if (nn < 0 || nn > 127) { sentNote[orig] = -1; continue; }
            sentNote[orig] = nn + 1;
            m.setNoteNumber (nn);
        }
        else if (m.isNoteOff())
        {
            const int orig = m.getNoteNumber();
            const int sn = sentNote[orig];
            sentNote[orig] = 0;
            if (sn < 0) continue;
            const int nn = sn > 0 ? sn - 1 : orig + shiftSt;
            if (nn < 0 || nn > 127) continue;
            m.setNoteNumber (nn);
        }
        shifted.addEvent (m, metadata.samplePosition);
    }

    juce::MidiBuffer arpOut;
    const juce::MidiBuffer* src = &shifted;

    if (pArpOn)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto ppos = ph->getPosition())
                if (auto b = ppos->getBpm())
                    bpm = *b;

        runArp (shifted, arpOut, buffer.getNumSamples(), bpm);
        src = &arpOut;
        arpWasOn = true;
    }
    else if (arpWasOn)
    {
        arpWasOn = false;
        arpNotes.clear();
        arpCur = -1;
        physDown = 0;
        allNotesOff();
    }

    for (const auto metadata : *src)
    {
        const int p = juce::jlimit (0, total, metadata.samplePosition);
        renderAll (buffer, pos, p - pos);
        pos = p;
        curPos = clockPos + p;
        handleMidi (metadata.getMessage(), a, b);
    }

    renderAll (buffer, pos, total - pos);
    clockPos += total;

    processYamaha (buffer);
    processSaturation (buffer);
    processWah (buffer);
    processPhaser (buffer);
    processDelay (buffer);
    processReverb (buffer);
    buffer.applyGain (pMaster);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float pk = ch < buffer.getNumChannels() ? buffer.getMagnitude (ch, 0, buffer.getNumSamples()) : 0.0f;
        meter[ch].store (juce::jmax (pk, meter[ch].load() * 0.92f));
    }
}

//==============================================================================
juce::AudioProcessorEditor* NayAudioProcessor::createEditor()
{
    return new NayAudioProcessorEditor (*this);
}

juce::ValueTree NayAudioProcessor::buildState()
{
    syncZones();
    auto state = apvts.copyState();

    auto old = state.getChildWithName ("GROUPS");
    if (old.isValid())
        state.removeChild (old, nullptr);

    state.setProperty ("presetName", currentPreset, nullptr);

    juce::ValueTree gs ("GROUPS");
    gs.setProperty ("active", activeGroup, nullptr);
    gs.setProperty ("duo", duoGroup, nullptr);
    for (const auto& g : groups)
    {
        juce::ValueTree c ("GROUP");
        c.setProperty ("name", g.name, nullptr);
        c.setProperty ("path", toStoredPath (g.path), nullptr);
        c.setProperty ("category", g.category, nullptr);
        for (const auto& z : g.zones)
        {
            juce::ValueTree zt ("ZONE");
            zt.setProperty ("file", z.file, nullptr);
            zt.setProperty ("root", z.root, nullptr);
            zt.setProperty ("lo", z.lo, nullptr);
            zt.setProperty ("hi", z.hi, nullptr);
            zt.setProperty ("vlo", z.vlo, nullptr);
            zt.setProperty ("vhi", z.vhi, nullptr);
            zt.setProperty ("tune", z.tune, nullptr);
            zt.setProperty ("ls", z.ls, nullptr);
            zt.setProperty ("le", z.le, nullptr);
            zt.setProperty ("xf", z.xf, nullptr);
            c.appendChild (zt, nullptr);
        }
        gs.appendChild (c, nullptr);
    }
    state.appendChild (gs, nullptr);

    auto oldMap = state.getChildWithName ("MIDIMAP");
    if (oldMap.isValid())
        state.removeChild (oldMap, nullptr);
    juce::ValueTree mm ("MIDIMAP");
    mm.setProperty ("expCC", expCC.load(), nullptr);
    for (int i = 0; i < 128; ++i)
        if (ccMap[i].load() >= 0)
        {
            juce::ValueTree t ("CC");
            t.setProperty ("cc", i, nullptr);
            t.setProperty ("param", juce::String (paramIds[ccMap[i].load()]), nullptr);
            mm.appendChild (t, nullptr);
        }
    state.appendChild (mm, nullptr);
    return state;
}

void NayAudioProcessor::applyState (const juce::ValueTree& st)
{
    apvts.replaceState (st);
    currentPreset = st.getProperty ("presetName").toString();

    auto mmap = st.getChildWithName ("MIDIMAP");
    if (mmap.isValid())
    {
        for (auto& m : ccMap) m = -1;
        expCC = (int) mmap.getProperty ("expCC", 11);
        for (int i = 0; i < mmap.getNumChildren(); ++i)
        {
            auto t = mmap.getChild (i);
            const auto id = t.getProperty ("param").toString();
            const int cc = (int) t.getProperty ("cc", -1);
            for (int k = 0; k < P_count; ++k)
                if (id == paramIds[k] && cc >= 0 && cc < 128)
                    ccMap[cc] = k;
        }
    }

    auto gs = st.getChildWithName ("GROUPS");
    if (gs.isValid())
    {
        groups.clear();
        activeGroup = -1;
        duoGroup = -1;
        {
            juce::SpinLock::ScopedLockType lock (bankLock);
            bankA = nullptr;
            bankB = nullptr;
        }

        for (int i = 0; i < gs.getNumChildren(); ++i)
        {
            auto c = gs.getChild (i);
            NayGroup g;
            g.name = c.getProperty ("name").toString();
            g.path = fromStoredPath (c.getProperty ("path").toString());
            g.category = c.getProperty ("category").toString();

            for (int k = 0; k < c.getNumChildren(); ++k)
            {
                auto zt = c.getChild (k);
                g.zones.push_back ({ zt.getProperty ("file").toString(), (int) zt.getProperty ("root", 60),
                                     (int) zt.getProperty ("lo", 0), (int) zt.getProperty ("hi", 127),
                                     (int) zt.getProperty ("vlo", 1), (int) zt.getProperty ("vhi", 127),
                                     (float) zt.getProperty ("tune", 0.0f),
                                     (int) zt.getProperty ("ls", -2), (int) zt.getProperty ("le", -2),
                                     (int) zt.getProperty ("xf", -2) });
            }
            groups.push_back (std::move (g));
        }
        scanInstruments (instrumentsDir());   // user instruments made after this project / preset was saved

        const int a = (int) gs.getProperty ("active", -1);
        if (a >= 0 && a < (int) groups.size())
            selectGroup (a);

        const int d = (int) gs.getProperty ("duo", -1);
        if (d >= 0 && d < (int) groups.size())
            selectDuo (d);
    }
}

void NayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = buildState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NayAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            applyState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// Presets: one file (.esp = xml) holds EVERYTHING: instruments + sample folders, mapping, all knobs, keys tuning,
// effects, arp, mixer, MIDI map. "Factory Presets" ship with the plugin; "User Presets" are the user's own.
bool NayAudioProcessor::savePreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;

    userPresetDir().createDirectory();
    currentPreset = clean;
    auto state = buildState();
    auto xml = state.createXml();
    return xml != nullptr && userPresetDir().getChildFile (clean + ".esp").replaceWithText (xml->toString());
}

bool NayAudioProcessor::loadPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    applyState (juce::ValueTree::fromXml (*xml));
    currentPreset = file.getFileNameWithoutExtension();
    return true;
}

juce::Array<juce::File> NayAudioProcessor::listPresets (bool factory)
{
    auto files = (factory ? factoryPresetDir() : userPresetDir()).findChildFiles (juce::File::findFiles, true, "*.esp");
    juce::Array<juce::File> out;
    for (const auto& f : files)
        out.add (f);
    std::sort (out.begin(), out.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    return out;
}

bool NayAudioProcessor::deletePreset (const juce::File& file)
{
    return file.existsAsFile() && file.moveToTrash();
}

bool NayAudioProcessor::renamePreset (const juce::File& file, const juce::String& newName)
{
    const auto clean = juce::File::createLegalFileName (newName.trim());
    if (clean.isEmpty() || ! file.existsAsFile())
        return false;
    return file.moveFileTo (file.getSiblingFile (clean + ".esp"));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NayAudioProcessor();
}
