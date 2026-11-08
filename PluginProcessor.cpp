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
    "mainLevel", "mainPan", "mainMute", "duoLevel", "duoPan", "duoMute", "master"
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
    l.add (std::make_unique<F> (id (P_satDrive), "KRONOS Drive", R (0.f, 1.f, 0.001f), 0.6f));
    l.add (std::make_unique<F> (id (P_satMix),   "KRONOS Amount", R (0.f, 1.f, 0.001f), 0.6f));

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
    if (libraryRoot().isDirectory() && scanFolder (libraryRoot()) > 0)
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

        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
        if (reader == nullptr || reader->lengthInSamples < 4)
            continue;

        const int len = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 30.0));
        const int chans = (int) reader->numChannels;

        juce::AudioBuffer<float> tmp (chans, len);
        reader->read (&tmp, 0, len, 0, true, true);

        auto s = std::make_shared<NaySample>();
        s->fileName = f.getFileName();
        s->root = root;
        s->sampleRate = reader->sampleRate;
        s->data.setSize (1, len);
        s->data.clear();
        for (int ch = 0; ch < chans; ++ch)
            s->data.addFrom (0, 0, tmp, ch, 0, len, 1.0f / (float) chans);

        newBank->push_back (s);
    }

    std::sort (newBank->begin(), newBank->end(),
               [] (const auto& a, const auto& b) { return a->root.load() < b->root.load(); });

    autoZones (*newBank);

    for (const auto& z : ov)
        for (const auto& s : *newBank)
            if (s->fileName == z.file)
            {
                s->root = z.root; s->lo = z.lo; s->hi = z.hi; s->vlo = z.vlo; s->vhi = z.vhi;
            }

    return newBank;
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

// Sample folders inside the library are stored relative ("@LIB/Stringed/Oud") so presets / projects work on any computer.
juce::String NayAudioProcessor::toStoredPath (const juce::String& path)
{
    const auto lib = libraryRoot().getFullPathName();
    const auto prefix = lib + juce::File::getSeparatorString();
    if (lib.isNotEmpty() && path.startsWith (prefix))
        return "@LIB/" + path.substring (prefix.length()).replace ("\\", "/");
    return path;
}

juce::String NayAudioProcessor::fromStoredPath (const juce::String& stored)
{
    if (stored.startsWith ("@LIB/"))
        return libraryRoot().getChildFile (stored.substring (5)).getFullPathName();
    return stored;
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
        g.bank = loadBank (juce::File (g.path), g.zones);
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

    if (index == activeGroup && groups[(size_t) index].bank != nullptr)
        sampleInfo = groups[(size_t) index].name + ": " + juce::String ((int) groups[(size_t) index].bank->size()) + " samples";
}

void NayAudioProcessor::resetZones (int index)
{
    if (index < 0 || index >= (int) groups.size() || groups[(size_t) index].bank == nullptr)
        return;

    groups[(size_t) index].zones.clear();
    for (const auto& s : *groups[(size_t) index].bank)
        s->root = 60; // placeholder, re-read from file name below

    // re-parse roots from file names, then recompute zones
    for (const auto& s : *groups[(size_t) index].bank)
        s->root = parseRootNote (juce::File (s->fileName).getFileNameWithoutExtension());

    autoZones (*groups[(size_t) index].bank);
}

void NayAudioProcessor::syncZones()
{
    for (auto& g : groups)
        if (g.bank != nullptr)
        {
            g.zones.clear();
            for (const auto& s : *g.bank)
                g.zones.push_back ({ s->fileName, s->root.load(), s->lo.load(), s->hi.load(), s->vlo.load(), s->vhi.load() });
        }
}

//==============================================================================
double NayAudioProcessor::noteTarget (int note) const
{
    return (double) note + (double) off[note % 12] * 0.01;
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
        for (int r = bestRoot + dir; r >= 0 && r <= 127 && found < 2 && std::abs (r - bestRoot) <= 2; r += dir)   // neighbours within 2 semitones
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

void NayAudioProcessor::startVoice (NayVoice& v, int note, int vel,
                                    const std::shared_ptr<const NayBank>& a,
                                    const std::shared_ptr<const NayBank>& b)
{
    auto sa = pickSample (a.get(), note, vel, lastPicked);
    std::shared_ptr<NaySample> sb;
    if (b != nullptr)
        sb = pickSample (b.get(), note, vel, lastPickedB);

    if (sa == nullptr && sb == nullptr)
        return;

    v.sa = sa;
    v.sb = sb;
    v.posA = v.posB = 0.0;
    v.lp = 0.0f;
    v.lpB = 0.0f;
    v.note = note;
    v.target = noteTarget (note);
    v.pitch = v.target;
    v.detune = 0.0f;
    v.vibPhase = 0.0;
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
        const int vel = juce::jlimit (1, 127, (int) msg.getVelocity());
        heldVel[n] = vel;

        if (pMono)
        {
            const bool wasHeld = ! held.empty();
            held.erase (std::remove (held.begin(), held.end(), n), held.end());
            held.push_back (n);

            auto& v = voices[0];
            if (pLegato && wasHeld && v.adsr.isActive() && (v.sa != nullptr || v.sb != nullptr))
            {
                // legato: no retrigger, just glide
                v.note = n;
                v.target = noteTarget (n);
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
    if (num <= 0 || (v.sa == nullptr && v.sb == nullptr) || ! v.adsr.isActive())
        return;

    NaySample* ss[2] = { v.sa.get(), v.sb.get() };
    double* pos[2] = { &v.posA, &v.posB };
    const float lg[2] = { 1.0f, 1.0f };

    const float* src[2] = { nullptr, nullptr };
    double lEnd[2] = { 0, 0 }, lStart[2] = { 0, 0 }, rateBase[2] = { 0, 0 };
    bool act[2] = { false, false };

    for (int k = 0; k < 2; ++k)
    {
        if (ss[k] == nullptr || ss[k]->data.getNumSamples() < 4)
            continue;

        const int len = ss[k]->data.getNumSamples();
        src[k] = ss[k]->data.getReadPointer (0);
        lEnd[k] = (double) len - 1.0;
        lStart[k] = juce::jmin (lEnd[k] - 1.0, (double) len * 0.5); // sustain loop = second half
        rateBase[k] = (ss[k]->sampleRate / sr) * std::exp2 (-(double) ss[k]->root.load() / 12.0);
        act[k] = true;
    }

    if (! act[0] && ! act[1])
        return;

    float* L = buf.getWritePointer (0, start);
    float* R = buf.getNumChannels() > 1 ? buf.getWritePointer (1, start) : nullptr;

    const float gt = pGlide;
    const double glideCoef = gt < 1.0f ? 1.0 : 1.0 - std::exp (-1.0 / ((double) gt * 0.001 * sr));
    const double depthCents = juce::jmin (100.0, (double) pVibDepth + (double) modWheel * 50.0);
    const double bendSemis = (double) bendNorm * (double) pPbRange;
    const float lpA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (v.velFc * (0.2f + 0.8f * exprSm), 0.45f * (float) sr) / (float) sr);

    float es = exprSm;
    for (int i = 0; i < num; ++i)
    {
        es += (expr - es) * 0.003f;
        v.pitch += (v.target - v.pitch) * glideCoef;

        v.vibPhase += (double) pVibRate / sr;
        if (v.vibPhase >= 1.0)
            v.vibPhase -= 1.0;
        const double vib = std::sin (juce::MathConstants<double>::twoPi * v.vibPhase) * depthCents * 0.01;

        const double ratio = std::exp2 ((v.pitch + bendSemis + vib + (double) v.detune) / 12.0);

        float mA = 0.0f, mB = 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            if (! act[k])
                continue;

            double& p = *pos[k];
            while (p >= lEnd[k])
                p -= (lEnd[k] - lStart[k]);

            const int i0 = (int) p;
            const float frac = (float) (p - (double) i0);
            const float smp = (src[k][i0] + frac * (src[k][i0 + 1] - src[k][i0])) * lg[k];
            if (k == 0) mA = smp; else mB = smp;

            p += ratio * rateBase[k];
        }

        v.lp  += lpA * (mA - v.lp);
        v.lpB += lpA * (mB - v.lpB);
        const float env = v.adsr.getNextSample() * v.velGain * pGain * std::pow (es, 1.5f);
        const float oa = v.lp * env, ob = v.lpB * env;

        if (R != nullptr)
        {
            L[i] += oa * mixAL + ob * mixBL;
            R[i] += oa * mixAR + ob * mixBR;
        }
        else
            L[i] += oa * 0.5f * (mixAL + mixAR) + ob * 0.5f * (mixBL + mixBR);

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

    exprSm += (expr - exprSm) * (1.0f - std::pow (0.997f, (float) num));
}

//==============================================================================
// KRONOS model: a smooth, heavy "saturated lead" (electric-guitar lead / Kronos style) - warm low end, soft
// rounded saturation curve, extra shine in the upper mids, and level matching so the instrument stays at the same
// loudness whether the effect is on or off, at any DRIVE / AMOUNT.
void NayAudioProcessor::processSaturation (juce::AudioBuffer<float>& buffer)
{
    if (! pSatOn)
        return;

    const float drive = pSatDrive, mix = pSatMix;
    const float fs = (float) sr;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float k = 1.0f + drive * 9.0f;           // soft curve (no hard clipping)
    const float bias = 0.08f * drive;               // tiny asymmetry = even harmonics, smooth and warm
    const float tb = std::tanh (bias);
    const float aMid  = 1.0f - std::exp (-twoPi * 1500.0f / fs);
    const float aLow  = 1.0f - std::exp (-twoPi * 220.0f / fs);
    const float aTop  = 1.0f - std::exp (-twoPi * juce::jmin (11000.0f, 0.45f * fs) / fs);
    const float aSpk  = 1.0f - std::exp (-twoPi * 3500.0f / fs);
    const float aEnv  = 1.0f - std::exp (-1.0f / (0.05f * fs));
    const float aGain = 1.0f - std::exp (-1.0f / (0.02f * fs));

    auto shape = [=] (float x) { return std::tanh (k * x + bias) - tb; };

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = buffer.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            const float xd = d[i];

            satHi[ch] += aMid * (xd - satHi[ch]);   // low-passed at 1.5 kHz
            satLo[ch] += aLow * (xd - satLo[ch]);   // low-passed at 220 Hz
            // pre-shaper: a little warmth from the lows, and lift the mids / highs (shine) instead of rolling them off
            const float x = xd + satLo[ch] * (0.35f * drive) + (xd - satHi[ch]) * (0.5f * drive);

            const float xm = 0.5f * (satXPrev[ch] + x);
            satXPrev[ch] = x;

            float y = 0.5f * (shape (xm) + shape (x));

            satLp[ch] += aTop * (y - satLp[ch]);    // only removes fizz above 11 kHz
            y = satLp[ch];

            satLp2[ch] += aSpk * (y - satLp2[ch]);
            y += (y - satLp2[ch]) * (0.25f * drive); // sparkle

            const float hp = y - satDcX[ch] + 0.995f * satDcY[ch]; // DC blocker
            satDcX[ch] = y;
            satDcY[ch] = hp;

            // level match: keep the loudness of the wet signal equal to the loudness of the input
            satEnvIn[ch]  += aEnv * (xd * xd - satEnvIn[ch]);
            satEnvOut[ch] += aEnv * (hp * hp - satEnvOut[ch]);
            if (satEnvIn[ch] > 1.0e-8f)
            {
                const float target = juce::jlimit (0.05f, 8.0f, std::sqrt (satEnvIn[ch] / (satEnvOut[ch] + 1.0e-9f)));
                satG[ch] += aGain * (target - satG[ch]);
            }

            const float wet = hp * satG[ch];
            d[i] = xd + (wet - xd) * mix;
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

    // fine-tune per note (the microtonal keys)
    for (int i = 0; i < 12; ++i)
        off[i] = offRaw[i]->load();

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
        pMaster = dbToGainOrOff (getP (P_master));
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

    processSaturation (buffer);
    processWah (buffer);
    processPhaser (buffer);
    processDelay (buffer);
    processReverb (buffer);
    buffer.applyGain (pMaster);
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
                                     (int) zt.getProperty ("vlo", 1), (int) zt.getProperty ("vhi", 127) });
            }
            groups.push_back (std::move (g));
        }

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
