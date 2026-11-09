// Round-trip test for user instruments: WAV drop -> note detection -> auto loop -> .esi save -> reload.
// Built only with -DEGY_TESTS=ON (not part of the plugin).
#include "../PluginProcessor.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

static juce::File writeTone (const juce::File& f, double freq, double secs, double fs)
{
    const int n = (int) (secs * fs);
    juce::AudioBuffer<float> b (1, n);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / fs;
        const double env = std::min (1.0, t / 0.03) * (0.8 + 0.2 * std::exp (-t * 2.0));
        double v = 0.0;
        for (int h = 1; h <= 6; ++h)
            v += std::sin (juce::MathConstants<double>::twoPi * freq * h * t) / (h * h);
        b.setSample (0, i, (float) (0.4 * env * v));
    }
    f.deleteFile();
    f.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), fs, 1, 24, {}, 0));
    w->writeFromAudioSampleBuffer (b, 0, n);
    return f;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("egy_tests");
    tmp.deleteRecursively();
    tmp.createDirectory();

    // A3 + 30 cents, no note in the name -> must be detected.  C4 by name.
    const double a3up = 220.0 * std::pow (2.0, 0.30 / 12.0);
    auto f1 = writeTone (tmp.getChildFile ("take_01.wav"), a3up, 3.0, 48000.0);
    auto f2 = writeTone (tmp.getChildFile ("C4.wav"), 261.6256, 2.5, 44100.0);
    auto f3 = writeTone (tmp.getChildFile ("sub").getChildFile ("C4.wav"), 261.6256, 2.0, 44100.0);   // same name, other folder

    juce::String path;
    {
        NayAudioProcessor p;
        const int idx = p.createInstrument ("Test Nay");
        check (p.isUserInstrument (idx), "createInstrument makes a user instrument");

        juce::StringArray drop { f1.getFullPathName(), f2.getFullPathName(), f3.getFullPathName() };
        const auto msg = p.addSamples (drop);
        std::printf ("      %s\n", msg.toRawUTF8());

        auto bank = p.getBankOf (idx);
        check (bank != nullptr && bank->size() == 3, "3 samples added");

        int detectedRoot = -1, names = 0;
        float detectedTune = 0.0f;
        bool loopsOk = true;
        for (const auto& s : *bank)
        {
            if (s->fileName == "take_01.wav") { detectedRoot = s->root.load(); detectedTune = s->tune.load(); }
            if (s->fileName.startsWith ("C4")) ++names;
            loopsOk = loopsOk && s->loopStart.load() >= 0 && s->loopEnd.load() > s->loopStart.load();
            std::printf ("      %-16s root %3d  tune %+6.1f  loop %d..%d  (%s)\n", s->fileName.toRawUTF8(), s->root.load(),
                         s->tune.load(), s->loopStart.load(), s->loopEnd.load(), s->detectInfo.toRawUTF8());
        }
        check (detectedRoot == 57, "pitch detection: A3 (57)");
        check (std::abs (detectedTune + 30.0f) < 3.0f, "pitch detection: tune corrects +30 ct (got " + juce::String (detectedTune, 1) + ")");
        check (names == 2, "duplicate file name kept as a second sample");
        check (loopsOk, "auto loop found on every sample");

        for (const auto& s : *bank) s->tune = s->tune.load() + 0.0f;
        check (p.saveInstrument (idx), "save .esi");
        path = p.getGroupPath (idx);
        check (juce::File (path).existsAsFile(), "file exists: " + path);
    }

    {
        NayAudioProcessor p2;   // new instance scans the Instruments folder
        int found = -1;
        for (int i = 0; i < p2.getNumGroups(); ++i)
            if (p2.getGroupPath (i) == path) found = i;
        check (found >= 0, "reloaded from the Instruments folder");
        if (found >= 0)
        {
            p2.selectGroup (found);
            auto bank = p2.getBankOf (found);
            check (bank != nullptr && bank->size() == 3, "reloaded 3 samples");
            check (p2.getGroupName (found) == "Test Nay", "name kept");
            bool same = true;
            for (const auto& s : *bank)
                if (s->fileName == "take_01.wav")
                    same = s->root.load() == 57 && std::abs (s->tune.load() + 30.0f) < 3.0f && s->loopStart.load() >= 0
                           && std::abs (s->sampleRate - 48000.0) < 1.0 && s->data.getNumSamples() == 144000;
            check (same, "root / tune / loop / audio length survive the round trip");
        }
    }

    // the raw file must not be a readable WAV/FLAC (protection)
    {
        juce::MemoryBlock mb;
        juce::File (path).loadFileAsData (mb);
        const auto s = juce::String::fromUTF8 ((const char*) mb.getData(), juce::jmin (4096, (int) mb.getSize()));
        check (! s.contains ("fLaC") && ! s.contains ("take_01"), "file contents are scrambled");
    }

    // wave editor ops: delete a range, then trim; loop survives / resets correctly
    {
        auto s = std::make_shared<NaySample>();
        s->sampleRate = 48000.0;
        s->data.setSize (2, 48000);
        for (int i = 0; i < 48000; ++i) { s->data.setSample (0, i, std::sin (i * 0.05f)); s->data.setSample (1, i, std::cos (i * 0.05f)); }
        s->loopStart = 30000; s->loopEnd = 40000;
        auto d = NayAudioProcessor::applyWaveOp (*s, WaveOp::Delete, 1000, 11000);
        check (d != nullptr && d->data.getNumSamples() == 38000 && d->data.getNumChannels() == 2, "DELETE removes 10000 samples (stereo kept)");
        check (d != nullptr && d->loopStart.load() == 20000 && d->loopEnd.load() == 30000, "loop moves with DELETE");
        auto t = NayAudioProcessor::applyWaveOp (*s, WaveOp::Trim, 25000, 45000);
        check (t != nullptr && t->data.getNumSamples() == 20000 && t->loopStart.load() == 5000, "TRIM keeps the selection + loop");
        auto n = NayAudioProcessor::applyWaveOp (*s, WaveOp::Normalize, 0, 0);
        check (n != nullptr && std::abs (n->data.getMagnitude (0, 48000) - juce::Decibels::decibelsToGain (-0.3f)) < 0.01f, "NORMALIZE to -0.3 dB");
        NayAudioProcessor::autoLoop (*s);
        check (s->loopStart.load() >= 0 && s->xfade.load() > 0, "AUTO LOOP sets a crossfade (" + juce::String (s->xfade.load()) + " samples)");
    }

    // render the UI pages to PNG (visual check)
    {
        NayAudioProcessor p3;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p3.createEditor());
        auto snap = [&] (const juce::String& name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = juce::File::getCurrentWorkingDirectory().getChildFile ("ui_" + name + ".png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (img, os);
        };
        auto click = [&] (const juce::String& text)
        {
            for (auto* c : ed->getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*> (c))
                    if (b->getButtonText() == text && b->onClick) { b->onClick(); return; }
        };
        snap ("main");
        click ("MIXER");    snap ("mixer");
        click ("BACK");     click ("SETTINGS"); snap ("settings");
        click ("BACK");     click ("VOICE");    snap ("voice");
        click ("BACK");     click ("BROWSE");   snap ("browse");
    }

    juce::File (path).deleteFile();
    tmp.deleteRecursively();
    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
