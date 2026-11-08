#pragma once

#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace neon
{
    const juce::Colour bg0     { 0xff04060d };
    const juce::Colour bg1     { 0xff0a1022 };
    const juce::Colour panel   { 0xff0b1222 };
    const juce::Colour cyan    { 0xff00e5ff };
    const juce::Colour magenta { 0xffff2bd6 };
    const juce::Colour violet  { 0xff8a63ff };
    const juce::Colour green   { 0xff3dffa8 };
    const juce::Colour amber   { 0xffffb800 };
    const juce::Colour dim     { 0xff6f7fa6 };
    const juce::Colour text    { 0xffd6e4ff };
}

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

// Small number box showing a per-note fine tune in cents.
// Drag up/down to change, double-click toggles 0 <-> -50 (quarter tone flat).
class CentsBox : public juce::Slider
{
public:
    explicit CentsBox (juce::RangedAudioParameter& p) : param (p) {}
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter& param;
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

class NayAudioProcessorEditor : public juce::AudioProcessorEditor
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

    Knob* addKnob (int paramIndex, const char* name, int decimals, const juce::String& suffix, juce::Colour accent);
    juce::ToggleButton* addToggle (int paramIndex, const juce::String& text, juce::Colour accent);
    void addCombo (juce::ComboBox&, int paramIndex, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att);
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

    Page page;
    MapView mapView;
    bool mapMode = false;
    juce::TextButton mapButton { "MAPPING" };

    std::vector<std::unique_ptr<Knob>> knobs;
    Knob *kGain {}, *kAtk {}, *kRel {}, *kGlide {}, *kVibD {}, *kVibR {}, *kVel {}, *kPb {},
         *kLbLevel {}, *kLbVel {},
         *kSatDrive {}, *kSatMix {}, *kWahPos {}, *kWahRate {}, *kWahMix {},
         *kPhRate {}, *kPhDepth {}, *kPhMix {}, *kDlTime {}, *kDlFb {}, *kDlLvl {}, *kRvSize {}, *kRvLvl {};

    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAtts;
    juce::ToggleButton *tMono {}, *tLegato {}, *tSat {}, *tWah {}, *tPh {};

    juce::ComboBox maqamBox, rootBox, modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> maqamAtt, rootAtt, modeAtt;

    std::array<std::unique_ptr<CentsBox>, 12> boxes;   // indexed by pitch class 0..11 (C..B)
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 12> boxAtt;

    juce::ShapeButton prevA { "prevA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton nextA { "nextA", neon::cyan, neon::cyan.brighter (0.4f), neon::cyan.darker (0.3f) };
    juce::ShapeButton prevB { "prevB", neon::magenta, neon::magenta.brighter (0.4f), neon::magenta.darker (0.3f) };
    juce::ShapeButton nextB { "nextB", neon::magenta, neon::magenta.brighter (0.4f), neon::magenta.darker (0.3f) };
    juce::TextButton renameButton { "RENAME" };
    juce::TextButton addButton { "ADD FOLDER" };
    juce::Label info;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image bgImage;
    juce::Rectangle<int> lcdA, lcdB, pageRect;
    juce::Rectangle<int> pVoice, pLayer, pMaqam, pSat, pWah, pPh, pDelay, pReverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NayAudioProcessorEditor)
};
