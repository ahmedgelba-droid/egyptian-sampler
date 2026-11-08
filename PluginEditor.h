#pragma once

#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

// Egyptian palette (names kept from the first version): turquoise, carnelian, lapis, papyrus green, gold.
namespace neon
{
    const juce::Colour bg0     { 0xff0a0805 };
    const juce::Colour bg1     { 0xff1f1609 };
    const juce::Colour panel   { 0xff17110a };
    const juce::Colour cyan    { 0xff2fe0c4 };   // turquoise
    const juce::Colour magenta { 0xffe2573f };   // carnelian
    const juce::Colour violet  { 0xff4a86ff };   // lapis
    const juce::Colour green   { 0xffa6d84f };   // papyrus
    const juce::Colour amber   { 0xfff0b83c };   // gold
    const juce::Colour dim     { 0xff9a8a6a };
    const juce::Colour text    { 0xfff3e6c8 };
}

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
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
class MapView : public juce::Component
{
public:
    explicit MapView (NayAudioProcessor&);

    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    enum Edge { eL = 1, eR = 2, eT = 4, eB = 8 };

    std::shared_ptr<const NayBank> currentBank() const;
    juce::Rectangle<float> zoneRect (const NaySample&) const;
    int hitTest (juce::Point<float>, int& edges) const;
    float keyW() const { return (float) grid.getWidth() / 128.0f; }
    float keyToX (int k) const { return (float) grid.getX() + (float) k * keyW(); }
    float velToY (int v) const { return (float) grid.getBottom() - (float) v / 127.0f * (float) grid.getHeight(); }
    void selectSample (int i);

    NayAudioProcessor& proc;
    bool editB = false;
    int selected = -1, dragEdges = 0;
    juce::Point<float> dragStart;
    int o_lo = 0, o_hi = 0, o_vlo = 0, o_vhi = 0;

    juce::Rectangle<int> grid, keyStrip;
    juce::TextButton btnA { "EDIT LAYER A" }, btnB { "EDIT LAYER B" }, resetBtn { "RESET ZONES" };
    juce::Slider rootSlider;
    juce::Label rootLabel, fileLabel;
};

class NayAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NayAudioProcessorEditor (NayAudioProcessor&);
    ~NayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

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
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
    };

    void timerCallback() override { if (page.isVisible()) page.repaint(); }
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
    void stepLayerB (int delta);

    NayAudioProcessor& proc;
    NeonLookAndFeel laf;
    bool built = false;

    Page page, settings;       // main page / settings page (layers, folders, mapping)
    MapView mapView;
    juce::TextButton mapButton { "SETTINGS" };

    std::vector<std::unique_ptr<Knob>> knobs;
    Knob *kGain {}, *kAtk {}, *kRel {}, *kGlide {}, *kVibD {}, *kVibR {}, *kVel {}, *kPb {},
         *kLbLevel {}, *kLbVel {},
         *kSatDrive {}, *kSatMix {}, *kChRate {}, *kChDepth {}, *kChMix {}, *kWahPos {}, *kWahMix {},
         *kPhRate {}, *kPhDepth {}, *kPhMix {}, *kDlTime {}, *kDlFb {}, *kDlLvl {}, *kRvSize {}, *kRvLvl {};

    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAtts;
    juce::ToggleButton *tMono {}, *tLegato {}, *tSat {}, *tCh {}, *tWah {}, *tPh {};

    juce::ComboBox maqamBox, rootBox, modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> maqamAtt, rootAtt, modeAtt;

    std::array<std::unique_ptr<NoteKey>, 12> keys;     // indexed by pitch class 0..11 (C..B)

    juce::ShapeButton prevA { "prevA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton nextA { "nextA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton prevA2 { "prevA2", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton nextA2 { "nextA2", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton prevB { "prevB", neon::magenta, neon::magenta.brighter (0.4f), neon::magenta.darker (0.3f) };
    juce::ShapeButton nextB { "nextB", neon::magenta, neon::magenta.brighter (0.4f), neon::magenta.darker (0.3f) };
    juce::TextButton renameButton { "RENAME" };
    juce::TextButton addButton { "ADD FOLDER" };
    juce::Label info;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image bgImage;
    juce::Rectangle<int> pageRect, lcdA, lcdM, lcdA2, lcdB2;
    juce::Rectangle<int> pVoice, pMaqam, pSat, pCh, pWah, pPh, pDelay, pReverb, pLayer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NayAudioProcessorEditor)
};
