#pragma once

#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

// Monumental Anubis palette: near-black + gold everywhere, light turquoise only for LEDs / meters, carnelian for alerts.
// (the old accent names are kept so every control keeps working; they all map to golds now)
namespace neon
{
    const juce::Colour bg0     { 0xff050403 };   // charcoal black
    const juce::Colour bg1     { 0xff15100a };
    const juce::Colour panel   { 0xff0b0906 };
    const juce::Colour gold    { 0xffe9b947 };
    const juce::Colour bronze  { 0xff6b430c };
    const juce::Colour led     { 0xff5fe8ff };   // LEDs, meters, indicator dots
    const juce::Colour cyan    { 0xffe9b947 };   // gold
    const juce::Colour magenta { 0xffe2573f };   // carnelian (alerts, MAGIC LEGATO, reset / mute)
    const juce::Colour violet  { 0xffc49a45 };   // antique gold
    const juce::Colour green   { 0xfff2d27a };   // pale gold
    const juce::Colour amber   { 0xffe9b947 };   // gold
    const juce::Colour dim     { 0xff8c7d66 };   // warm stone grey
    const juce::Colour text    { 0xfff3e2b8 };
}

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

// Vertical wheel (pitch / mod) that shows the incoming MIDI value, like a keyboard's wheels.
class WheelView : public juce::Component
{
public:
    WheelView (std::function<float()> getValue, bool bipolarMode, juce::Colour c)
        : getter (std::move (getValue)), bipolar (bipolarMode), col (c) {}
    void paint (juce::Graphics&) override;

private:
    std::function<float()> getter;
    bool bipolar;
    juce::Colour col;
};

// One note button (C..B). Lit when the note is altered (by the maqam or by hand); the value in cents is shown below.
// Click = quarter-tone flat on/off. Drag up/down = fine tune.
class NoteKey : public juce::Component, private juce::Timer
{
public:
    NoteKey (NayAudioProcessor&, int pitchClass);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    float maq() const;
    float user() const;
    float effective() const { return user() + maq(); }
    void setUser (float cents);

    NayAudioProcessor& proc;
    juce::RangedAudioParameter* par = nullptr;
    int pc;
    float lastEff = 9999.f, startUser = 0.f;
    bool dragged = false;
};

// Kontakt-style mapping editor: key range (x) x velocity range (y) for each sample of a layer.
// Drop WAV files (or folders) on it to build an instrument. Bottom: waveform with the loop points.
class MapView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit MapView (NayAudioProcessor&);

    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

    std::function<void (const juce::String&)> onChanged;   // instrument changed (message for the info box)
    std::function<void (int)> onEditWave;                  // open the wave editor on this sample
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::TextButton addFilesBtn { "ADD FILES" }, editBtn { "EDIT WAVE" };
    std::unique_ptr<juce::FileChooser> chooser;
    void edited();                                          // marks the instrument as changed
    void paintWave (juce::Graphics&, const NaySample&);
    int sampleToX (const NaySample&, int pos) const;
    int xToSample (const NaySample&, float x) const;
    NaySample* selectedSample() const;
    juce::Rectangle<int> waveRect;
    juce::TextButton detectBtn { "DETECT NOTE" }, loopBtn { "AUTO LOOP" }, delBtn { "DELETE SAMPLE" };
    juce::Slider tuneSlider;
    juce::Label tuneLabel;
    bool dropHover = false;
    int loopDrag = 0;                                       // 1 = loop start marker, 2 = loop end marker
    std::vector<float> waveMin, waveMax;
    const NaySample* waveFor = nullptr;
    int waveW = 0;

    enum Edge { eL = 1, eR = 2, eT = 4, eB = 8 };

    std::shared_ptr<const NayBank> currentBank() const;
    juce::Rectangle<float> zoneRect (const NaySample&) const;
    int hitTest (juce::Point<float>, int& edges) const;
    float keyW() const { return (float) grid.getWidth() / 128.0f; }
    float keyToX (int k) const { return (float) grid.getX() + (float) k * keyW(); }
    float velToY (int v) const { return (float) grid.getBottom() - (float) v / 127.0f * (float) grid.getHeight(); }
    void selectSample (int i);

    NayAudioProcessor& proc;
    int selected = -1, dragEdges = 0;
    juce::Point<float> dragStart;
    int o_lo = 0, o_hi = 0, o_vlo = 0, o_vhi = 0;

    juce::Rectangle<int> grid, keyStrip;
    juce::TextButton resetBtn { "RESET ZONES" };
    juce::Slider rootSlider;
    juce::Label rootLabel, fileLabel;
};

// Small list model driven by lambdas (used by the Browse page).
class LambdaListModel : public juce::ListBoxModel
{
public:
    std::function<int()> getCount;
    std::function<juce::String (int)> getText;
    std::function<void (int)> onSelect, onDouble;

    int getNumRows() override { return getCount ? getCount() : 0; }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int hgt, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override { if (onSelect && lastRowSelected >= 0) onSelect (lastRowSelected); }
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { if (onDouble) onDouble (row); }
};

// Wave editor (Wave Extreme style): sample list, zoomable waveform, selection, DELETE / TRIM / FADE / NORMALIZE,
// loop markers + crossfade, AUTO LOOP, UNDO and PLAY (audition) - all on the active instrument.
class WaveEditor : public juce::Component, private juce::ScrollBar::Listener
{
public:
    explicit WaveEditor (NayAudioProcessor&);
    ~WaveEditor() override;

    void open (int sampleIndex);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void()> onClose;
    std::function<void (const juce::String&)> onChanged;

private:
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    std::shared_ptr<NaySample> current() const;
    void selectRow (int i);
    void doOp (WaveOp op);
    void pushUndo();
    void edited (const juce::String& msg);
    void setView (double start, double len);
    void updateControls();
    int xToPos (float x) const;
    float posToX (double p) const;
    void loopBounds (const NaySample&, int& ls, int& le) const;
    int snapZero (const NaySample&, int pos) const;

    NayAudioProcessor& proc;
    int group = -1, idx = -1;
    int selA = -1, selB = -1, dragMode = 0;
    double viewStart = 0.0, viewLen = 1.0;
    std::vector<std::pair<int, std::shared_ptr<NaySample>>> undo;    // (sample index, previous sample)

    LambdaListModel listModel;
    juce::ListBox list;
    juce::TextButton backBtn { "BACK" }, playBtn { "PLAY" }, undoBtn { "UNDO" }, saveBtn { "SAVE" },
                     zoomSelBtn { "ZOOM SEL" }, zoomAllBtn { "ZOOM ALL" },
                     delBtn { "DELETE" }, trimBtn { "TRIM" }, silBtn { "SILENCE" }, finBtn { "FADE IN" }, foutBtn { "FADE OUT" },
                     normBtn { "NORMALIZE" }, revBtn { "REVERSE" },
                     loopSelBtn { "LOOP = SELECTION" }, autoLoopBtn { "AUTO LOOP" }, clearLoopBtn { "NO LOOP (DEFAULT)" };
    juce::Slider xfSlider;
    juce::Label xfLabel;
    juce::ScrollBar hbar { false };
    juce::Rectangle<int> waveArea, statusArea;

    std::vector<float> cMin, cMax;               // drawing cache
    const NaySample* cFor = nullptr;
    double cStart = -1.0, cLen = -1.0;
    int cW = 0;
};

class NayAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NayAudioProcessorEditor (NayAudioProcessor&);
    ~NayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // right-click on a control = MIDI learn menu

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Page : public juce::Component
    {
        std::function<void (juce::Graphics&)> onPaint;
        std::function<void (const juce::MouseEvent&)> onMouse;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
        void mouseDown (const juce::MouseEvent& e) override { if (onMouse) onMouse (e); }
    };

    void timerCallback() override;
    int lastMonCount = -1, tick = 0;
    juce::uint32 lastMonMs = 0;
    juce::Rectangle<int> midiRect;
    std::vector<std::pair<juce::Component*, int>> learnTargets;
    int lastLearnT = -1, lastLearnCount = 0;
    juce::uint32 lastLearnMs = 0;
    Knob* addKnob (juce::Component& parent, int paramIndex, const char* name, int decimals, const juce::String& suffix, juce::Colour accent);
    juce::ToggleButton* addToggle (juce::Component& parent, int paramIndex, const juce::String& text, juce::Colour accent);
    void addCombo (juce::Component& parent, juce::ComboBox&, int paramIndex, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att);
    void layoutRow (juce::Rectangle<int> area, std::initializer_list<Knob*> ks);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, juce::Colour accent);
    void drawLcd (juce::Graphics&, juce::Rectangle<int>, const juce::String& tag, const juce::String& name,
                  const juce::String& sub, juce::Colour accent);
    void rebuildBackground();
    void refreshGroups();
    void stepGroup (int delta);
    void stepDuo (int delta);
    void showVoiceMenu (bool duo);
    void showView (int v);                       // 0 main, 1 browse, 2 mixer, 3 settings
    void rebuildBrowse (int forceCat = -1);
    void rebuildShown();
    void updateBrowseButtons();
    void promptName (const juce::String& title, const juce::String& initial, std::function<void (juce::String)> cb);

    NayAudioProcessor& proc;
    NeonLookAndFeel laf;
    bool built = false;

    Page page, settings, browse, mixer, voiceEd;       // main / settings (folders, mapping) / browse / mixer
    MapView mapView;
    WaveEditor waveEd;
    juce::TextButton mapButton { "SETTINGS" }, browseButton { "BROWSE" }, mixerButton { "MIXER" }, voiceButton { "VOICE" };
    int view = 0;

    // ---- browse page ----
    struct BrowseEntry { juce::String name; int group = -1; juce::File preset; bool user = false; };
    std::vector<std::pair<int, juce::String>> cats;     // kind: 0 all, 1 category, 2 factory presets, 3 user presets
    std::vector<BrowseEntry> shown;
    int selCat = 0;
    LambdaListModel catModel, instModel;
    juce::ListBox catList, instList;
    juce::TextEditor searchBox;
    juce::TextButton loadBtn { "LOAD" }, duoBtn { "ADD AS DUO" }, noDuoBtn { "REMOVE DUO" },
                     saveBtn { "SAVE PRESET" }, renPresetBtn { "RENAME PRESET" }, delPresetBtn { "DELETE PRESET" };
    juce::Label browseInfo;
    juce::ToggleButton *tMainMute {}, *tDuoMute {};
    Knob *kMainLvl {}, *kMainPan {}, *kDuoLvl {}, *kDuoPan {}, *kMaster {};
    juce::Rectangle<int> pMixMain, pMixDuo, pMixMaster;
    juce::Rectangle<int> mixStrip[9];
    Knob* vk[17] {};
    juce::Rectangle<int> vePorta, veSound, veVel, veLimit, veTouch;

    std::vector<std::unique_ptr<Knob>> knobs;
    Knob *kGain {}, *kAtk {}, *kRel {}, *kGlide {}, *kVibD {}, *kVibR {}, *kPb {},
         *kSatDrive {}, *kSatMix {}, *kSatTone {}, *kSatBass {},  *kWahPos {}, *kWahMix {},
         *kArpOct {}, *kArpGate {}, *kPhRate {}, *kPhDepth {}, *kPhMix {}, *kDlTime {}, *kDlFb {}, *kDlLvl {}, *kRvSize {}, *kRvLvl {};

    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAtts;
    juce::ToggleButton *tMono {}, *tLegato {}, *tVel {}, *tArp {}, *tArpHold {}, *tSat {},  *tBypass {}, *tPorta {}, *tMagic {}, *tWah {}, *tPh {}, *tVar {};

    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAtt, arpRateAtt;

    std::array<std::unique_ptr<NoteKey>, 12> keys;     // indexed by pitch class 0..11 (C..B)

    juce::ShapeButton prevA { "prevA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton nextA { "nextA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton prevA2 { "prevA2", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton nextA2 { "nextA2", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::TextButton renameButton { "RENAME" };
    juce::TextButton resetKeysBtn { "RESET KEYS" };
    juce::ComboBox maqamBox;
    juce::TextButton memStoreBtn { "MEMORY" }, memBtn[5];
    bool memArmed = false;
    void applyMaqam (int idx);
    void storeScale (int slot);
    void recallScale (int slot);
    juce::ShapeButton prevD { "prevD", neon::amber, neon::amber.brighter (0.4f), neon::amber.darker (0.3f) };
    juce::ShapeButton nextD { "nextD", neon::amber, neon::amber.brighter (0.4f), neon::amber.darker (0.3f) };
    juce::TextButton addButton { "ADD FOLDER" }, newInstBtn { "NEW INSTRUMENT" }, saveInstBtn { "SAVE INSTRUMENT" };
    // oscillator panel (main page)
    juce::ToggleButton *tOscOn {}, *tOscOnly {}, *tOscSoloMix {}, *tOscMuteMix {}, *tOscOnMix {};
    Knob *kOscLvl {}, *kOscTune {}, *kOscFine {}, *kOscCut {}, *kOscFader {}, *kOscPan {};
    juce::ComboBox oscWaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscWaveAtt;
    juce::Label info;
    std::unique_ptr<juce::FileChooser> chooser;

    WheelView pitchWheel { [this] { return proc.getBend(); }, true, neon::cyan };
    WheelView modWheelView { [this] { return proc.getModWheel(); }, false, neon::amber };
    WheelView expWheel { [this] { return proc.getExpr(); }, false, neon::green };
    juce::Slider pbBox, trBox, ocBox;     // bend range / transpose (semitones) / octave
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pbAtt, trAtt, ocAtt;
    void setupStepBox (juce::Slider&, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>&, const char* paramId, bool showSign);
    // anything touched by hand is also written to the info strip
    std::vector<float> lastParamVals;
    bool paramsInit = false;
    juce::String touchText;
    juce::uint32 touchMs = 0, lastMidiMs = 0;
    juce::Rectangle<int> pWheels;

    juce::Image bgImage;
    juce::Rectangle<int> pageRect, lcdA, lcdD, lcdA2;
    juce::Rectangle<int> pVoice, pMaqam, pSat, pWah, pPh, pDelay, pReverb, pArp, pPerf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NayAudioProcessorEditor)
};
