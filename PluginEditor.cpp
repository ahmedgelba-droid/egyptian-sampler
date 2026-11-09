#include "PluginEditor.h"
#include <cmath>

static const char* kNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static juce::Font uiFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions ("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::String noteName (int n)
{
    return juce::String (kNames[n % 12]) + juce::String (n / 12 - 1);
}

static void drawAnubis (juce::Graphics& g, float x, float y, float h, float alpha, bool eye);

// chamfered (cut-corner) rectangle used by panels, buttons, LCDs
static juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    c = juce::jmin (c, r.getWidth() * 0.5f, r.getHeight() * 0.5f);
    juce::Path p;
    p.startNewSubPath (r.getX() + c, r.getY());
    p.lineTo (r.getRight() - c, r.getY());
    p.lineTo (r.getRight(), r.getY() + c);
    p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom());
    p.lineTo (r.getX() + c, r.getBottom());
    p.lineTo (r.getX(), r.getBottom() - c);
    p.lineTo (r.getX(), r.getY() + c);
    p.closeSubPath();
    return p;
}

static void drawDiamond (juce::Graphics& g, float cx, float cy, float s, juce::Colour c)
{
    juce::Path d;
    d.startNewSubPath (cx, cy - s);
    d.lineTo (cx + s, cy);
    d.lineTo (cx, cy + s);
    d.lineTo (cx - s, cy);
    d.closeSubPath();
    g.setColour (c);
    g.fillPath (d);
}

static void ledDot (juce::Graphics& g, juce::Rectangle<float> r, bool on)
{
    if (on)
    {
        g.setColour (neon::led.withAlpha (0.25f));
        g.fillEllipse (r.expanded (r.getWidth() * 0.6f));
        g.setColour (neon::led.withAlpha (0.45f));
        g.fillEllipse (r.expanded (r.getWidth() * 0.25f));
        g.setColour (neon::led);
    }
    else
        g.setColour (juce::Colour (0xff17323a));
    g.fillEllipse (r);
}

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0b0906));
    setColour (juce::PopupMenu::textColourId, neon::text);
    setColour (juce::PopupMenu::headerTextColourId, neon::gold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neon::gold.withAlpha (0.22f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xfffff3d0));
    setColour (juce::ComboBox::textColourId, neon::text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff070504));
    setColour (juce::TextButton::textColourOffId, neon::gold);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1a1004));   // dark text on the gold fill
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff0b0906));
    setColour (juce::AlertWindow::textColourId, neon::text);
    setColour (juce::AlertWindow::outlineColourId, neon::gold);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff070504));
    setColour (juce::TextEditor::textColourId, neon::text);
    setColour (juce::TextEditor::outlineColourId, neon::bronze);
    setColour (juce::TextEditor::focusedOutlineColourId, neon::gold);
    setColour (juce::Label::textColourId, neon::text);
    setColour (juce::ScrollBar::thumbColourId, neon::gold.withAlpha (0.6f));
    setColour (juce::ListBox::backgroundColourId, juce::Colour (0xcc0b0906));
    setColour (juce::Slider::textBoxTextColourId, neon::text);
}

juce::Font NeonLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (12.5f).withExtraKerningFactor (0.06f); }
juce::Font NeonLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return uiFont (11.5f).withExtraKerningFactor (0.16f); }

// knob: black centre, metallic gold ring, tick marks, light-gold pointer, turquoise value dot
void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    const bool enabled = s.isEnabled();
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float alpha = enabled ? 1.0f : 0.35f;

    // tick marks around the ring
    for (int i = 0; i <= 10; ++i)
    {
        const float ta = a0 + (a1 - a0) * (float) i / 10.0f;
        const float r0 = r - 1.0f, r1 = r - (i % 5 == 0 ? 6.0f : 4.0f);
        g.setColour (neon::gold.withAlpha ((i % 5 == 0 ? 0.75f : 0.4f) * alpha));
        g.drawLine (c.x + std::sin (ta) * r1, c.y - std::cos (ta) * r1, c.x + std::sin (ta) * r0, c.y - std::cos (ta) * r0, 1.2f);
    }

    // soft gold glow + metallic ring
    const float rr = r - 7.0f;
    g.setColour (neon::gold.withAlpha (0.10f * alpha));
    g.fillEllipse (c.x - rr - 3.0f, c.y - rr - 3.0f, (rr + 3.0f) * 2.0f, (rr + 3.0f) * 2.0f);
    juce::ColourGradient ring (juce::Colour (0xfffbe7a6).withAlpha (alpha), c.x - rr, c.y - rr,
                               juce::Colour (0xff6b430c).withAlpha (alpha), c.x + rr, c.y + rr, false);
    ring.addColour (0.5, juce::Colour (0xffd8a43a).withAlpha (alpha));
    g.setGradientFill (ring);
    g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);

    // black centre
    const float br = rr - 3.6f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d160d), c.x, c.y - br, juce::Colour (0xff030201), c.x, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

    // light-gold pointer
    juce::Path p;
    p.addRoundedRectangle (-1.5f, -br + 2.0f, 3.0f, br * 0.62f, 1.4f);
    p.applyTransform (juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (juce::Colour (0xfffbe7a6).withAlpha (alpha));
    g.fillPath (p);

    // turquoise value dot on the ring
    if (enabled)
        ledDot (g, juce::Rectangle<float> (3.6f, 3.6f).withCentre ({ c.x + std::sin (ang) * (rr + 2.5f), c.y - std::cos (ang) * (rr + 2.5f) }), true);
}

void NeonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical)
    {
        // horizontal: dark groove, gold fill, gold thumb
        const float cy = (float) y + (float) h * 0.5f;
        juce::Rectangle<float> groove ((float) x, cy - 2.5f, (float) w, 5.0f);
        g.setColour (juce::Colour (0xff050302));
        g.fillRoundedRectangle (groove, 2.5f);
        g.setColour (neon::bronze);
        g.drawRoundedRectangle (groove, 2.5f, 1.0f);
        const float px = juce::jlimit ((float) x, (float) (x + w), pos);
        const float from = s.getMinimum() < 0.0 && s.getMaximum() > 0.0 ? (float) s.valueToProportionOfLength (0.0) * (float) w + (float) x : (float) x;
        g.setColour (neon::gold.withAlpha (s.isEnabled() ? 0.85f : 0.3f));
        g.fillRect (juce::jmin (from, px), cy - 1.5f, std::abs (px - from), 3.0f);
        juce::Rectangle<float> th (px - 6.0f, cy - 9.0f, 12.0f, 18.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7a6), 0.0f, th.getY(), juce::Colour (0xff8a5b16), 0.0f, th.getBottom(), false));
        g.fillPath (chamfer (th, 3.0f));
        g.setColour (juce::Colour (0xff2b1807));
        g.strokePath (chamfer (th, 3.0f), juce::PathStrokeType (1.0f));
        return;
    }

    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y + 12.0f, bot = (float) y + (float) h - 12.0f;

    g.setColour (neon::gold.withAlpha (0.35f));
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = bot - (bot - top) * (float) i / 10.0f;
        const float len = (i % 5 == 0) ? 10.0f : 6.0f;
        g.drawLine (cx - 19.0f - len, ty, cx - 19.0f, ty, 1.0f);
        g.drawLine (cx + 19.0f, ty, cx + 19.0f + len, ty, 1.0f);
    }

    juce::Rectangle<float> groove (cx - 5.0f, top - 4.0f, 10.0f, bot - top + 8.0f);
    g.setColour (juce::Colour (0xff030201));
    g.fillRoundedRectangle (groove, 4.0f);
    g.setColour (neon::bronze.brighter (0.3f));
    g.drawRoundedRectangle (groove, 4.0f, 1.2f);

    const float py = juce::jlimit (top, bot, pos);
    juce::Rectangle<float> fill (cx - 2.5f, py, 5.0f, bot - py + 2.0f);
    g.setColour (neon::led.withAlpha (0.25f));
    g.fillRoundedRectangle (fill.expanded (3.0f, 0.0f), 4.0f);
    g.setColour (neon::led);
    g.fillRoundedRectangle (fill, 2.5f);

    // gold fader cap (chamfered)
    juce::Rectangle<float> cap (cx - 20.0f, py - 11.0f, 40.0f, 22.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillPath (chamfer (cap.translated (0.0f, 2.5f), 4.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7a6), cap.getX(), cap.getY(),
                                             juce::Colour (0xff6b430c), cap.getX(), cap.getBottom(), false));
    g.fillPath (chamfer (cap, 4.0f));
    g.setColour (juce::Colour (0xff2b1807));
    g.strokePath (chamfer (cap, 4.0f), juce::PathStrokeType (1.2f));
    g.fillRect (cap.getX() + 6.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 12.0f, 2.0f);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const bool on = b.getToggleState();
    const bool magic = b.getButtonText().containsIgnoreCase ("MAGIC");
    const auto edge = magic ? neon::magenta : neon::gold;
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);
    const auto shape = chamfer (r, 6.0f);

    g.setColour (juce::Colour (0xff070504).withAlpha (0.9f));
    g.fillPath (shape);
    if (magic)
    {   // carnelian glow so MAGIC LEGATO stands out
        g.setColour (neon::magenta.withAlpha (on ? 0.45f : 0.2f));
        g.strokePath (chamfer (r.expanded (2.0f), 7.0f), juce::PathStrokeType (on ? 4.0f : 2.5f));
        g.setColour (neon::magenta.withAlpha (on ? 0.25f : 0.08f));
        g.fillPath (shape);
    }
    else if (on || hl)
    {
        g.setColour (neon::gold.withAlpha (on ? 0.22f : 0.1f));
        g.strokePath (chamfer (r.expanded (1.5f), 7.0f), juce::PathStrokeType (3.0f));
    }
    g.setColour (edge.withAlpha (on || magic ? 1.0f : 0.55f));
    g.strokePath (shape, juce::PathStrokeType (1.3f));

    // square LED (turquoise)
    const float ls = 8.0f;
    juce::Rectangle<float> led (r.getX() + 10.0f, r.getCentreY() - ls * 0.5f, ls, ls);
    if (on)
    {
        g.setColour ((magic ? neon::magenta : neon::led).withAlpha (0.35f));
        g.fillRoundedRectangle (led.expanded (3.0f), 3.0f);
    }
    g.setColour (on ? (magic ? juce::Colour (0xffff8a6a) : neon::led) : juce::Colour (0xff17323a));
    g.fillRoundedRectangle (led, 1.5f);

    g.setColour (magic ? juce::Colour (0xffffb09a) : (on ? neon::text : neon::gold.withAlpha (0.7f)));
    g.setFont (uiFont (11.5f).withExtraKerningFactor (0.14f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (26.0f).toNearestInt(), juce::Justification::centredLeft, true);
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    const auto accent = b.findColour (juce::TextButton::buttonColourId);
    const bool alert = accent == neon::magenta;
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    const auto shape = chamfer (r, juce::jmin (7.0f, r.getHeight() * 0.3f));

    if (on || down)
    {   // active = gold fill
        g.setColour (neon::gold.withAlpha (0.25f));
        g.strokePath (chamfer (r.expanded (2.0f), 8.0f), juce::PathStrokeType (3.5f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe1a0), 0.0f, r.getY(), juce::Colour (0xffb7832a), 0.0f, r.getBottom(), false));
        g.fillPath (shape);
        g.setColour (juce::Colour (0xff3a2408));
        g.strokePath (shape, juce::PathStrokeType (1.2f));
        return;
    }

    g.setColour (juce::Colour (0xff070504).withAlpha (0.92f));
    g.fillPath (shape);
    if (hl)
    {
        g.setColour (neon::gold.withAlpha (0.14f));
        g.fillPath (shape);
    }
    g.setColour ((alert ? neon::magenta : neon::gold).withAlpha (b.isEnabled() ? 0.95f : 0.3f));
    g.strokePath (shape, juce::PathStrokeType (1.4f));
    g.setColour (neon::bronze.withAlpha (0.8f));
    g.strokePath (chamfer (r.reduced (3.0f), juce::jmin (5.0f, r.getHeight() * 0.25f)), juce::PathStrokeType (0.8f));
}

void NeonLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    const auto shape = chamfer (r, 6.0f);
    g.setColour (juce::Colour (0xff070504));
    g.fillPath (shape);
    g.setColour (neon::gold.withAlpha (0.9f));
    g.strokePath (shape, juce::PathStrokeType (1.3f));

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (neon::gold);
    g.fillPath (arrow);
}

//==============================================================================
void LambdaListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int hgt, bool selected)
{
    if (selected)
    {
        g.setColour (neon::amber.withAlpha (0.28f));
        g.fillRect (0, 0, w, hgt);
        g.setColour (neon::amber);
        g.fillRect (0, 0, 3, hgt);
    }
    g.setColour (selected ? neon::text : neon::dim.brighter (0.2f));
    g.setFont (uiFont (14.0f, selected));
    g.drawText (getText ? getText (row) : juce::String(), 14, 0, w - 18, hgt, juce::Justification::centredLeft, true);
}

//==============================================================================
MapView::MapView (NayAudioProcessor& p) : proc (p)
{
    resetBtn.setColour (juce::TextButton::buttonColourId, neon::amber);
    resetBtn.onClick = [this]
    {
        proc.resetZones (proc.getActiveGroup());
        selected = -1;
        refresh();
        if (onChanged)
            onChanged ("Zones reset to automatic");
    };
    addAndMakeVisible (resetBtn);

    rootSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    rootSlider.setRange (0.0, 127.0, 1.0);
    rootSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
    rootSlider.setColour (juce::Slider::thumbColourId, neon::cyan);
    rootSlider.setColour (juce::Slider::trackColourId, neon::cyan.withAlpha (0.5f));
    rootSlider.setColour (juce::Slider::textBoxTextColourId, neon::text);
    rootSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    rootSlider.textFromValueFunction = [] (double v) { return noteName ((int) v); };
    rootSlider.onValueChange = [this]
    {
        auto bank = currentBank();
        if (bank != nullptr && selected >= 0 && selected < (int) bank->size())
        {
            (*bank)[(size_t) selected]->root = (int) rootSlider.getValue();
            edited();
            repaint();
        }
    };
    addAndMakeVisible (rootSlider);

    tuneSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    tuneSlider.setRange (-100.0, 100.0, 1.0);
    tuneSlider.setDoubleClickReturnValue (true, 0.0);
    tuneSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
    tuneSlider.setColour (juce::Slider::thumbColourId, neon::amber);
    tuneSlider.setColour (juce::Slider::trackColourId, neon::amber.withAlpha (0.5f));
    tuneSlider.setColour (juce::Slider::textBoxTextColourId, neon::text);
    tuneSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    tuneSlider.textFromValueFunction = [] (double v) { return (v > 0 ? "+" : "") + juce::String ((int) v) + " ct"; };
    tuneSlider.valueFromTextFunction = [] (const juce::String& t) { return t.retainCharacters ("-0123456789").getDoubleValue(); };
    tuneSlider.onValueChange = [this]
    {
        if (auto* s = selectedSample())
        {
            s->tune = (float) tuneSlider.getValue();
            edited();
        }
    };
    addAndMakeVisible (tuneSlider);
    tuneLabel.setText ("TUNE", juce::dontSendNotification);
    tuneLabel.setFont (uiFont (11.5f));
    tuneLabel.setColour (juce::Label::textColourId, neon::dim);
    addAndMakeVisible (tuneLabel);

    detectBtn.setColour (juce::TextButton::buttonColourId, neon::cyan);
    detectBtn.onClick = [this]
    {
        if (auto* s = selectedSample())
        {
            NayAudioProcessor::detectRoot (*s);
            edited();
            selectSample (selected);
        }
    };
    loopBtn.setColour (juce::TextButton::buttonColourId, neon::green);
    loopBtn.onClick = [this]
    {
        if (auto* s = selectedSample())
        {
            const int before = s->loopStart.load();
            NayAudioProcessor::autoLoop (*s);
            edited();
            if (onChanged)
                onChanged (s->loopStart.load() >= 0 && s->loopStart.load() != before ? "AUTO LOOP: new loop found - play the key to hear it"
                                                                                      : "AUTO LOOP: kept the current loop (sample too short or no better point)");
            repaint();
        }
    };
    delBtn.setColour (juce::TextButton::buttonColourId, neon::magenta);
    delBtn.onClick = [this]
    {
        if (selectedSample() == nullptr)
            return;
        proc.removeSample (proc.getActiveGroup(), selected);
        selected = -1;
        refresh();
        if (onChanged)
            onChanged ("Sample deleted - press SAVE INSTRUMENT to keep it");
    };
    for (auto* b : { &detectBtn, &loopBtn, &delBtn })
        addAndMakeVisible (*b);

    // ADD FILES: the same as drag & drop, through a file browser (multi-select)
    addFilesBtn.setColour (juce::TextButton::buttonColourId, neon::amber);
    addFilesBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Add samples to the instrument", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        juce::Component::SafePointer<MapView> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                                | juce::FileBrowserComponent::canSelectMultipleItems,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr || fc.getResults().isEmpty())
                                      return;
                                  juce::StringArray paths;
                                  for (const auto& f : fc.getResults())
                                      paths.add (f.getFullPathName());
                                  safe->filesDropped (paths, 0, 0);
                              });
    };
    editBtn.setColour (juce::TextButton::buttonColourId, neon::violet);
    editBtn.onClick = [this] { if (onEditWave) onEditWave (juce::jmax (0, selected)); };
    addAndMakeVisible (addFilesBtn);
    addAndMakeVisible (editBtn);

    rootLabel.setText ("ROOT KEY", juce::dontSendNotification);
    rootLabel.setFont (uiFont (11.5f));
    rootLabel.setColour (juce::Label::textColourId, neon::dim);
    addAndMakeVisible (rootLabel);

    fileLabel.setFont (uiFont (13.0f));
    fileLabel.setColour (juce::Label::textColourId, neon::text);
    addAndMakeVisible (fileLabel);
}

std::shared_ptr<const NayBank> MapView::currentBank() const
{
    return proc.getBankOf (proc.getActiveGroup());
}

void MapView::refresh()
{
    auto bank = currentBank();
    if (bank == nullptr || selected >= (int) bank->size())
        selected = -1;

    selectSample (selected);
}

void MapView::selectSample (int i)
{
    selected = i;
    auto bank = currentBank();

    if (bank != nullptr && i >= 0 && i < (int) bank->size())
    {
        const auto& s = *(*bank)[(size_t) i];
        rootSlider.setValue (s.root.load(), juce::dontSendNotification);
        tuneSlider.setValue (s.tune.load(), juce::dontSendNotification);
        fileLabel.setText (s.fileName + (s.detectInfo.isNotEmpty() ? "   (" + s.detectInfo + ")" : juce::String()), juce::dontSendNotification);
        for (juce::Component* c : { (juce::Component*) &rootSlider, (juce::Component*) &tuneSlider, (juce::Component*) &detectBtn,
                                    (juce::Component*) &loopBtn, (juce::Component*) &delBtn })
            c->setEnabled (true);
    }
    else
    {
        selected = -1;
        fileLabel.setText (bank != nullptr && ! bank->empty() ? "Click a zone. Drag to move, drag its edges to resize."
                                                              : "Drop WAV files (or a folder) here to build an instrument.",
                           juce::dontSendNotification);
        for (juce::Component* c : { (juce::Component*) &rootSlider, (juce::Component*) &tuneSlider, (juce::Component*) &detectBtn,
                                    (juce::Component*) &loopBtn, (juce::Component*) &delBtn })
            c->setEnabled (false);
    }
    delBtn.setEnabled (selected >= 0 && proc.isUserInstrument (proc.getActiveGroup()));
    repaint();
}

NaySample* MapView::selectedSample() const
{
    auto bank = currentBank();
    if (bank == nullptr || selected < 0 || selected >= (int) bank->size())
        return nullptr;
    return (*bank)[(size_t) selected].get();
}

void MapView::edited()
{
    proc.markDirty (proc.getActiveGroup());
    if (auto* p = getParentComponent())
        p->repaint();
}

void MapView::mouseDoubleClick (const juce::MouseEvent& e)
{
    int edges = 0;
    const int i = hitTest (e.position, edges);
    if (i >= 0 && onEditWave)
        onEditWave (i);
}

void MapView::filesDropped (const juce::StringArray& files, int, int)
{
    dropHover = false;
    juce::MouseCursor::showWaitCursor();
    const auto msg = proc.addSamples (files);
    juce::MouseCursor::hideWaitCursor();
    selected = -1;
    refresh();
    if (onChanged)
        onChanged (msg);
}

void MapView::resized()
{
    auto a = getLocalBounds().reduced (14);
    {
        auto top = juce::Rectangle<int> (a.getX(), a.getY(), a.getWidth(), 28);
        resetBtn.setBounds (top.removeFromRight (112));   top.removeFromRight (6);
        delBtn.setBounds (top.removeFromRight (118));     top.removeFromRight (6);
        loopBtn.setBounds (top.removeFromRight (96));     top.removeFromRight (6);
        detectBtn.setBounds (top.removeFromRight (108));  top.removeFromRight (6);
        editBtn.setBounds (top.removeFromRight (100));    top.removeFromRight (6);
        addFilesBtn.setBounds (top.removeFromRight (100));
    }

    waveRect = { a.getX() + 34, a.getBottom() - 150, a.getWidth() - 34, 150 };
    const int rowY = waveRect.getY() - 6 - 26;
    keyStrip = { a.getX() + 34, rowY - 6 - 34, a.getWidth() - 34, 34 };
    grid = { keyStrip.getX(), a.getY() + 40, keyStrip.getWidth(), keyStrip.getY() - 2 - (a.getY() + 40) };

    fileLabel.setBounds (a.getX(), rowY, 330, 26);
    rootLabel.setBounds (a.getX() + 340, rowY, 70, 26);
    rootSlider.setBounds (a.getX() + 410, rowY, 230, 26);
    tuneLabel.setBounds (a.getX() + 652, rowY, 46, 26);
    tuneSlider.setBounds (a.getX() + 700, rowY, a.getWidth() - 700, 26);
    waveFor = nullptr;
}

int MapView::sampleToX (const NaySample& s, int pos) const
{
    const int len = juce::jmax (1, s.data.getNumSamples());
    return waveRect.getX() + (int) ((double) pos / len * waveRect.getWidth());
}

int MapView::xToSample (const NaySample& s, float x) const
{
    const int len = s.data.getNumSamples();
    return juce::jlimit (0, juce::jmax (0, len - 1), (int) ((double) (x - (float) waveRect.getX()) / waveRect.getWidth() * len));
}

void MapView::paintWave (juce::Graphics& g, const NaySample& s)
{
    const int len = s.data.getNumSamples();
    const int w = waveRect.getWidth();
    if (waveFor != &s || waveW != w)
    {
        waveFor = &s;
        waveW = w;
        waveMin.assign ((size_t) w, 0.0f);
        waveMax.assign ((size_t) w, 0.0f);
        const float* x = s.data.getReadPointer (0);
        for (int px = 0; px < w; ++px)
        {
            const int a = (int) ((juce::int64) px * len / w), b = juce::jmax (a + 1, (int) ((juce::int64) (px + 1) * len / w));
            float mn = 0.0f, mx = 0.0f;
            for (int i = a; i < b && i < len; ++i) { mn = juce::jmin (mn, x[i]); mx = juce::jmax (mx, x[i]); }
            waveMin[(size_t) px] = mn;
            waveMax[(size_t) px] = mx;
        }
    }

    float peak = 0.001f;
    for (int px = 0; px < w; ++px) peak = juce::jmax (peak, -waveMin[(size_t) px], waveMax[(size_t) px]);
    const float cy = (float) waveRect.getCentreY(), hh = (float) waveRect.getHeight() * 0.46f / peak;

    int ls = s.loopStart.load(), le = s.loopEnd.load();
    const bool own = ls >= 0 && le > ls + 8 && le < len;
    if (! own) { ls = len / 2; le = len - 2; }   // the engine's default loop (second half)

    const int x0 = sampleToX (s, ls), x1 = sampleToX (s, le + 1);
    g.setColour (neon::green.withAlpha (own ? 0.16f : 0.07f));
    g.fillRect (x0, waveRect.getY(), juce::jmax (1, x1 - x0), waveRect.getHeight());

    g.setColour (neon::cyan.withAlpha (0.85f));
    for (int px = 0; px < w; ++px)
        g.fillRect ((float) (waveRect.getX() + px), cy - waveMax[(size_t) px] * hh, 1.0f,
                    juce::jmax (1.0f, (waveMax[(size_t) px] - waveMin[(size_t) px]) * hh));

    for (int k = 0; k < 2; ++k)
    {
        const int mx = k == 0 ? x0 : x1;
        g.setColour (own ? neon::green : neon::dim);
        g.fillRect (mx - 1, waveRect.getY(), 2, waveRect.getHeight());
        juce::Path tri;
        tri.addTriangle ((float) mx - 6.0f, (float) waveRect.getY(), (float) mx + 6.0f, (float) waveRect.getY(), (float) mx, (float) waveRect.getY() + 9.0f);
        g.fillPath (tri);
    }

    g.setFont (uiFont (10.5f));
    g.setColour (neon::text.withAlpha (0.85f));
    const double secs = (double) (le + 1 - ls) / s.sampleRate;
    g.drawText ((own ? "LOOP  " : "DEFAULT LOOP (2nd half)  ") + juce::String (secs, 2) + " s     drag the green markers  /  AUTO LOOP",
                waveRect.reduced (8, 4), juce::Justification::topLeft, false);
}

juce::Rectangle<float> MapView::zoneRect (const NaySample& s) const
{
    const float x0 = keyToX (s.lo.load());
    const float x1 = keyToX (s.hi.load() + 1);
    const float y0 = velToY (s.vhi.load());
    const float y1 = velToY (s.vlo.load() - 1);
    return { x0, y0, x1 - x0, y1 - y0 };
}

void MapView::paint (juce::Graphics& g)
{
    auto all = getLocalBounds().toFloat();
    g.setColour (neon::panel.withAlpha (0.92f));
    g.fillRoundedRectangle (all, 10.0f);
    g.setColour (neon::cyan.withAlpha (0.4f));
    g.drawRoundedRectangle (all.reduced (0.5f), 10.0f, 1.2f);

    g.setColour (juce::Colour (0xff030201));
    g.fillRect (grid);

    for (int k = 0; k < 128; k += 12)
    {
        g.setColour (neon::cyan.withAlpha (0.16f));
        g.fillRect (keyToX (k), (float) grid.getY(), 1.0f, (float) grid.getHeight());
    }
    g.setFont (uiFont (10.0f));
    for (int v : { 1, 32, 64, 96, 127 })
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRect ((float) grid.getX(), velToY (v), (float) grid.getWidth(), 1.0f);
        g.setColour (neon::dim);
        g.drawText (juce::String (v), grid.getX() - 34, (int) velToY (v) - 7, 30, 14, juce::Justification::centredRight, false);
    }
    g.setColour (neon::cyan.withAlpha (0.5f));
    g.drawRect (grid, 1);

    g.setColour (neon::cyan);
    g.setFont (uiFont (13.0f));
    g.drawText ("MAPPING", 14, 14, 200, 28, juce::Justification::centredLeft, false);

    // waveform / loop area
    g.setColour (juce::Colour (0xff030201));
    g.fillRect (waveRect);
    g.setColour (neon::cyan.withAlpha (0.35f));
    g.drawRect (waveRect, 1);
    if (auto* sel = selectedSample())
        paintWave (g, *sel);
    else
    {
        g.setColour (neon::dim);
        g.setFont (uiFont (12.0f, false));
        g.drawText ("Select a zone to see its waveform and loop points", waveRect, juce::Justification::centred, false);
    }

    if (dropHover)
    {
        g.setColour (neon::amber.withAlpha (0.18f));
        g.fillRect (grid);
        g.setColour (neon::amber);
        g.drawRect (grid, 3);
        g.setFont (uiFont (18.0f));
        g.drawText (proc.isUserInstrument (proc.getActiveGroup()) ? "DROP TO ADD TO THIS INSTRUMENT" : "DROP TO MAKE A NEW INSTRUMENT",
                    grid, juce::Justification::centred, false);
    }

    // keyboard strip (keys altered by the chosen maqam / fine tune light up)
    for (int k = 0; k < 128; ++k)
    {
        const int pc = k % 12;
        const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (black ? juce::Colour (0xff101826) : neon::text.withAlpha (0.82f));
        g.fillRect (keyToX (k), (float) keyStrip.getY(), keyW() + 0.5f, (float) keyStrip.getHeight());
        {
            const float o = proc.apvts.getRawParameterValue ("off" + juce::String (pc))->load();
            if (std::abs (o) > 0.5f)
            {
                g.setColour ((o < 0 ? neon::magenta : neon::amber).withAlpha (0.9f));
                g.fillRect (keyToX (k), (float) keyStrip.getY(), keyW() + 0.5f, (float) keyStrip.getHeight());
            }
        }
        if (pc == 0)
        {
            g.setColour (juce::Colour (0xff101826));
            g.setFont (uiFont (9.5f));
            g.drawText ("C" + juce::String (k / 12 - 1), (int) keyToX (k), keyStrip.getBottom() - 14, 24, 12, juce::Justification::centredLeft, false);
        }
    }

    auto bank = currentBank();
    if (bank == nullptr || bank->empty())
    {
        if (! dropHover)
        {
            g.setColour (neon::dim);
            g.setFont (uiFont (15.0f));
            g.drawText ("DRAG & DROP WAV FILES (or a folder) HERE   -   notes are found from the name or the sound",
                        grid, juce::Justification::centred, false);
        }
        return;
    }

    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < (int) bank->size(); ++i)
        {
            const bool sel = i == selected;
            if ((pass == 1) != sel)
                continue;

            const auto& s = *(*bank)[(size_t) i];
            const auto r = zoneRect (s);
            const auto col = juce::Colour::fromHSV (0.085f + std::fmod ((float) i * 0.023f, 0.05f), 0.72f, 0.6f + 0.4f * std::fmod ((float) i * 0.37f, 1.0f), 1.0f);

            g.setColour (col.withAlpha (sel ? 0.5f : 0.28f));
            g.fillRect (r);
            g.setColour (col.withAlpha (sel ? 1.0f : 0.8f));
            g.drawRect (r, sel ? 2.0f : 1.0f);

            // root marker
            const float rx = keyToX (s.root.load()) + keyW() * 0.5f;
            juce::Path tri;
            tri.addTriangle (rx - 4.0f, r.getBottom(), rx + 4.0f, r.getBottom(), rx, r.getBottom() - 7.0f);
            g.setColour (juce::Colours::white);
            g.fillPath (tri);

            if (r.getWidth() > 30.0f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.9f));
                g.setFont (uiFont (10.0f));
                g.drawText (s.fileName, r.reduced (3.0f, 2.0f).toNearestInt(), juce::Justification::topLeft, true);
            }
        }
}

int MapView::hitTest (juce::Point<float> pt, int& edges) const
{
    edges = 0;
    auto bank = currentBank();
    if (bank == nullptr)
        return -1;

    std::vector<int> order;
    if (selected >= 0 && selected < (int) bank->size())
        order.push_back (selected);
    for (int i = (int) bank->size() - 1; i >= 0; --i)
        if (i != selected)
            order.push_back (i);

    const float tol = 6.0f;
    for (int i : order)
    {
        const auto r = zoneRect (*(*bank)[(size_t) i]);
        if (! r.expanded (tol * 0.5f).contains (pt))
            continue;

        const float dl = std::abs (pt.x - r.getX()), dr = std::abs (pt.x - r.getRight());
        if (dl < tol && dl <= dr) edges |= eL;
        else if (dr < tol)        edges |= eR;
        if (std::abs (pt.y - r.getY()) < tol)      edges |= eT;
        else if (std::abs (pt.y - r.getBottom()) < tol) edges |= eB;
        return i;
    }
    return -1;
}

void MapView::mouseMove (const juce::MouseEvent& e)
{
    if (waveRect.contains (e.getPosition()))
    {
        bool near = false;
        if (auto* s = selectedSample())
        {
            const int len = s->data.getNumSamples();
            int ls = s->loopStart.load(), le = s->loopEnd.load();
            if (! (ls >= 0 && le > ls + 8 && le < len)) { ls = len / 2; le = len - 2; }
            near = std::abs (e.x - sampleToX (*s, ls)) < 8 || std::abs (e.x - sampleToX (*s, le + 1)) < 8;
        }
        setMouseCursor (near ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        return;
    }

    int edges = 0;
    const int i = hitTest (e.position, edges);
    if (i < 0)                                  setMouseCursor (juce::MouseCursor::NormalCursor);
    else if ((edges & (eL | eR)) && (edges & (eT | eB))) setMouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor);
    else if (edges & (eL | eR))                 setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    else if (edges & (eT | eB))                 setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    else                                        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void MapView::mouseDown (const juce::MouseEvent& e)
{
    loopDrag = 0;
    if (waveRect.contains (e.getPosition()))
    {
        if (auto* s = selectedSample())
        {
            const int len = s->data.getNumSamples();
            int ls = s->loopStart.load(), le = s->loopEnd.load();
            if (! (ls >= 0 && le > ls + 8 && le < len)) { ls = len / 2; le = len - 2; }
            const int ds = std::abs (e.x - sampleToX (*s, ls)), de = std::abs (e.x - sampleToX (*s, le + 1));
            if (ds < 8 || de < 8)
            {
                loopDrag = ds <= de ? 1 : 2;
                s->loopStart = ls;    // a default loop becomes the sample's own loop when touched
                s->loopEnd = le;
            }
        }
        return;
    }

    int edges = 0;
    const int i = hitTest (e.position, edges);
    selectSample (i);
    dragEdges = edges;
    dragStart = e.position;

    auto bank = currentBank();
    if (i >= 0 && bank != nullptr)
    {
        const auto& s = *(*bank)[(size_t) i];
        o_lo = s.lo.load(); o_hi = s.hi.load(); o_vlo = s.vlo.load(); o_vhi = s.vhi.load();
    }
}

void MapView::mouseDrag (const juce::MouseEvent& e)
{
    if (loopDrag != 0)
    {
        if (auto* s = selectedSample())
        {
            const int len = s->data.getNumSamples();
            const float* x = s->data.getReadPointer (0);
            int pos = xToSample (*s, e.position.x);
            // snap to the nearest upward zero crossing (clean loop point)
            const int range = juce::jmax (64, len / juce::jmax (1, waveRect.getWidth()) * 4);
            int best = -1;
            for (int d = 0; d < range && best < 0; ++d)
                for (int c : { pos - d, pos + d })
                    if (c > 0 && c < len - 1 && x[c - 1] <= 0.0f && x[c] > 0.0f) { best = c; break; }
            if (best >= 0) pos = best;

            if (loopDrag == 1)
                s->loopStart = juce::jlimit (0, s->loopEnd.load() - 64, pos);
            else
                s->loopEnd = juce::jlimit (s->loopStart.load() + 64, len - 2, pos - 1);
            edited();
            repaint();
        }
        return;
    }

    auto bank = currentBank();
    if (bank == nullptr || selected < 0 || selected >= (int) bank->size())
        return;

    auto& s = *(*bank)[(size_t) selected];
    const int dk = (int) std::round ((e.position.x - dragStart.x) / keyW());
    const int dv = (int) std::round (-(e.position.y - dragStart.y) / (float) grid.getHeight() * 127.0f);

    if (dragEdges == 0)
    {
        const int kw = o_hi - o_lo, vw = o_vhi - o_vlo;
        const int lo = juce::jlimit (0, 127 - kw, o_lo + dk);
        const int vlo = juce::jlimit (1, 127 - vw, o_vlo + dv);
        s.lo = lo;  s.hi = lo + kw;
        s.vlo = vlo; s.vhi = vlo + vw;
    }
    else
    {
        if (dragEdges & eL) s.lo  = juce::jlimit (0, o_hi, o_lo + dk);
        if (dragEdges & eR) s.hi  = juce::jlimit (o_lo, 127, o_hi + dk);
        if (dragEdges & eT) s.vhi = juce::jlimit (o_vlo, 127, o_vhi + dv);
        if (dragEdges & eB) s.vlo = juce::jlimit (1, o_vhi, o_vlo + dv);
    }
    edited();
    repaint();
}

//==============================================================================
void WheelView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float v = juce::jlimit (bipolar ? -1.0f : 0.0f, 1.0f, getter());

    g.setColour (juce::Colour (0xff030201));
    g.fillRoundedRectangle (r, 6.0f);

    g.saveState();
    g.reduceClipRegion (r.toNearestInt());
    const float off = std::fmod (std::abs (v) * 40.0f, 6.0f) * (v < 0 ? -1.0f : 1.0f) + 6.0f;
    for (float y = r.getY() - 12.0f + off; y < r.getBottom(); y += 6.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRect (r.getX() + 3.0f, y, r.getWidth() - 6.0f, 2.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.6f), r.getX(), 0.0f,
                                             juce::Colours::black.withAlpha (0.6f), r.getRight(), 0.0f, false));
    g.fillRect (r);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getX(), 0.0f,
                                             juce::Colours::white.withAlpha (0.10f), r.getCentreX(), 0.0f, true));
    g.fillRect (r);
    g.restoreState();

    const float pos = bipolar ? r.getCentreY() - v * (r.getHeight() * 0.5f - 6.0f)
                              : r.getBottom() - 6.0f - v * (r.getHeight() - 12.0f);
    juce::ignoreUnused (col);
    g.setColour (neon::led.withAlpha (0.3f));
    g.fillRect (r.getX() + 1.0f, pos - 5.0f, r.getWidth() - 2.0f, 10.0f);
    g.setColour (neon::led);
    g.fillRoundedRectangle (r.getX() + 3.0f, pos - 2.5f, r.getWidth() - 6.0f, 5.0f, 2.0f);

    if (bipolar)
    {
        g.setColour (neon::amber.withAlpha (0.8f));
        g.fillRect (r.getX() - 1.0f, r.getCentreY() - 0.5f, 4.0f, 1.0f);
        g.fillRect (r.getRight() - 3.0f, r.getCentreY() - 0.5f, 4.0f, 1.0f);
    }

    g.setColour (neon::amber.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

//==============================================================================
NoteKey::NoteKey (NayAudioProcessor& p, int n) : proc (p), pc (n)
{
    par = proc.apvts.getParameter ("off" + juce::String (pc));
    startTimerHz (12);
}

float NoteKey::maq() const
{
    return 0.0f;   // no preset maqams any more: the keys are tuned by hand
}

float NoteKey::user() const { return par->convertFrom0to1 (par->getValue()); }

void NoteKey::setUser (float c)
{
    par->setValueNotifyingHost (par->convertTo0to1 (juce::jlimit (-100.0f, 100.0f, std::round (c))));
}

void NoteKey::timerCallback()
{
    const float e = effective();
    if (e != lastEff)
    {
        lastEff = e;
        repaint();
    }
}

void NoteKey::mouseDown (const juce::MouseEvent&)
{
    par->beginChangeGesture();
    startUser = user();
    dragged = false;
}

void NoteKey::mouseDrag (const juce::MouseEvent& e)
{
    const int dy = e.getDistanceFromDragStartY();
    if (std::abs (dy) > 3)
        dragged = true;
    if (dragged)
        setUser (startUser - (float) dy * 0.5f);
}

void NoteKey::mouseUp (const juce::MouseEvent&)
{
    if (! dragged)
    {
        const float eff = startUser + maq();
        setUser ((std::abs (eff) > 0.5f ? 0.0f : -50.0f) - maq());   // toggle: quarter-tone flat <-> normal
    }
    par->endChangeGesture();
}

void NoteKey::paint (juce::Graphics& g)
{
    // Yamaha "SCALE TUNE" style button: dark key with an LED bar on top (red = lowered, white = raised, dark = normal)
    const float eff = effective();
    const bool lowered = eff < -0.5f, raised = eff > 0.5f;
    const float w = (float) getWidth();
    const juce::Rectangle<float> r (3.0f, 13.0f, w - 6.0f, 36.0f);

    g.setColour (neon::gold);
    g.setFont (uiFont (12.5f).withExtraKerningFactor (0.1f));
    g.drawText (kNames[pc], 0, 0, getWidth(), 13, juce::Justification::centred, false);

    // dark key, chamfered gold outline
    juce::Path key;
    {
        const float c = 6.0f;
        key.startNewSubPath (r.getX() + c, r.getY());
        key.lineTo (r.getRight() - c, r.getY());  key.lineTo (r.getRight(), r.getY() + c);
        key.lineTo (r.getRight(), r.getBottom() - c); key.lineTo (r.getRight() - c, r.getBottom());
        key.lineTo (r.getX() + c, r.getBottom()); key.lineTo (r.getX(), r.getBottom() - c);
        key.lineTo (r.getX(), r.getY() + c);      key.closeSubPath();
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff18120a), 0.0f, r.getY(), juce::Colour (0xff050302), 0.0f, r.getBottom(), false));
    g.fillPath (key);
    g.setColour ((lowered || raised) ? neon::gold : neon::gold.withAlpha (0.6f));
    g.strokePath (key, juce::PathStrokeType (1.3f));

    // LED bar: light turquoise glow when the note is altered
    const juce::Rectangle<float> led (r.getCentreX() - 17.0f, r.getY() + 6.0f, 34.0f, 6.0f);
    if (lowered || raised)
    {
        g.setColour (neon::led.withAlpha (0.3f));
        g.fillRoundedRectangle (led.expanded (4.0f, 3.0f), 4.0f);
        g.setColour (neon::led);
        g.fillRoundedRectangle (led, 2.5f);
    }
    else
    {
        g.setColour (juce::Colour (0xff141a1c));
        g.fillRoundedRectangle (led, 2.5f);
    }

    g.setColour ((lowered || raised) ? neon::led : neon::gold.withAlpha (0.65f));
    g.setFont (uiFont (12.0f));
    const int v = (int) std::round (eff);
    g.drawText ((v > 0 ? "+" : "") + juce::String (v), r.toNearestInt().withTrimmedTop (18), juce::Justification::centred, false);
}

//==============================================================================
void NayAudioProcessorEditor::applyMaqam (int idx)
{
    // tonic = C (move it with TRANSPOSE: the quarter tones travel with the scale)
    float cents[12] {};
    if (idx == 2 || idx == 5) { cents[4] = -50.0f; cents[11] = -50.0f; }       // Rast / Sika: E and B
    if (idx == 3 || idx == 4) { cents[4] = -50.0f; }                           // Bayati / Saba: E
    if (idx <= 1) return;
    for (int i = 0; i < 12; ++i)
        if (auto* q = proc.apvts.getParameter ("off" + juce::String (i)))
        {
            q->beginChangeGesture();
            q->setValueNotifyingHost (q->convertTo0to1 (cents[i]));
            q->endChangeGesture();
        }
}

void NayAudioProcessorEditor::storeScale (int slot)
{
    juce::StringArray s;
    for (int i = 0; i < 12; ++i)
        if (auto* q = proc.apvts.getParameter ("off" + juce::String (i)))
            s.add (juce::String ((int) std::round (q->convertFrom0to1 (q->getValue()))));
    proc.apvts.state.setProperty ("scaleMem" + juce::String (slot), s.joinIntoString (","), nullptr);
}

void NayAudioProcessorEditor::recallScale (int slot)
{
    const auto t = proc.apvts.state.getProperty ("scaleMem" + juce::String (slot)).toString();
    if (t.isEmpty())
        return;
    juce::StringArray s;
    s.addTokens (t, ",", "");
    for (int i = 0; i < 12 && i < s.size(); ++i)
        if (auto* q = proc.apvts.getParameter ("off" + juce::String (i)))
        {
            q->beginChangeGesture();
            q->setValueNotifyingHost (q->convertTo0to1 ((float) s[i].getIntValue()));
            q->endChangeGesture();
        }
}

static void drawAnkh (juce::Graphics& g, float x, float y, float h, juce::Colour c)
{
    g.setColour (c);
    const float w = h * 0.6f, th = h * 0.1f;
    g.drawEllipse (x + w * 0.2f, y, w * 0.6f, h * 0.42f, th);
    g.fillRect (x + w * 0.5f - th * 0.5f, y + h * 0.42f, th, h * 0.58f);
    g.fillRect (x, y + h * 0.5f, w, th);
}

NayAudioProcessorEditor::NayAudioProcessorEditor (NayAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), mapView (p), waveEd (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (page);
    addChildComponent (settings);
    addChildComponent (browse);
    addChildComponent (mixer);
    addChildComponent (voiceEd);
    settings.addAndMakeVisible (mapView);
    settings.addChildComponent (waveEd);
    mapView.onEditWave = [this] (int i)
    {
        if (proc.getActiveGroup() < 0)
            return;
        waveEd.open (i);
        waveEd.setVisible (true);
        waveEd.toFront (false);
    };
    waveEd.onClose = [this]
    {
        waveEd.setVisible (false);
        refreshGroups();
    };
    waveEd.onChanged = [this] (const juce::String& msg)
    {
        info.setText (msg, juce::dontSendNotification);
        settings.repaint (lcdA2);
    };

    const auto tq = neon::cyan, cr = neon::magenta, gd = neon::amber, pp = neon::green, lp = neon::violet;

    // ---- voice ----
    kGain  = addKnob (page, P_gain,    "GAIN",      2, "", tq);
    kAtk   = addKnob (page, P_attack,  "ATTACK",    3, "s", tq);
    kRel   = addKnob (page, P_release, "RELEASE",   2, "s", tq);
    kGlide = addKnob (page, P_glide,   "GLIDE",     0, "ms", tq);
    kVibD  = addKnob (page, P_vibDepth,"VIB DEPTH", 0, "ct", tq);
    kVibR  = addKnob (page, P_vibRate, "VIB SPEED", 1, "Hz", tq);
    tVel   = addToggle (page, P_velOn, "VELOCITY", gd);
    tMono   = addToggle (page, P_mono,   "MONO",   tq);
    tLegato = addToggle (page, P_legato, "LEGATO", tq);
    tVar = addToggle (settings, P_varOn, "VARIATION", pp);

    // ---- scale tune: maqam presets, memory 1-5, bypass ----
    tBypass = addToggle (page, P_tuneBypass, "BYPASS (12-TET)", gd);
    maqamBox.addItem ("MAQAM: none", 1);
    maqamBox.addItem ("Rast  (E, B quarter-flat)", 2);
    maqamBox.addItem ("Bayati  (E quarter-flat)", 3);
    maqamBox.addItem ("Saba  (E quarter-flat)", 4);
    maqamBox.addItem ("Sika / Segah  (E, B)", 5);
    maqamBox.addItem ("Hijaz / Nahawand  (no quarter)", 6);
    maqamBox.setSelectedId (1, juce::dontSendNotification);
    maqamBox.onChange = [this] { applyMaqam (maqamBox.getSelectedId()); };
    page.addAndMakeVisible (maqamBox);
    memStoreBtn.setColour (juce::TextButton::buttonColourId, neon::magenta);
    page.addAndMakeVisible (memStoreBtn);
    memStoreBtn.onClick = [this]
    {
        memArmed = ! memArmed;
        memStoreBtn.setToggleState (memArmed, juce::dontSendNotification);
    };
    for (int i = 0; i < 5; ++i)
    {
        memBtn[i].setButtonText (juce::String (i + 1));
        memBtn[i].setColour (juce::TextButton::buttonColourId, neon::cyan);
        page.addAndMakeVisible (memBtn[i]);
        memBtn[i].onClick = [this, i]
        {
            if (memArmed) { storeScale (i); memArmed = false; memStoreBtn.setToggleState (false, juce::dontSendNotification); }
            else          recallScale (i);
        };
    }

    // ---- microtonal keys: reset + transpose / octave beside them ----
    resetKeysBtn.setColour (juce::TextButton::buttonColourId, neon::magenta);
    page.addAndMakeVisible (resetKeysBtn);
    resetKeysBtn.onClick = [this]
    {
        for (int i = 0; i < 12; ++i)
            if (auto* q = proc.apvts.getParameter ("off" + juce::String (i)))
            {
                q->beginChangeGesture();
                q->setValueNotifyingHost (q->convertTo0to1 (0.0f));
                q->endChangeGesture();
            }
    };
    for (int i = 0; i < 12; ++i)
    {
        keys[(size_t) i] = std::make_unique<NoteKey> (proc, i);
        page.addAndMakeVisible (*keys[(size_t) i]);
    }

    // ---- effects ----
    tSat = addToggle (mixer, P_satOn, "ACTIVE", gd);
    kSatDrive = addKnob (mixer, P_satDrive, "DRIVE",  2, "", gd);
    kSatMix   = addKnob (mixer, P_satMix,   "AMOUNT", 2, "", gd);
    kSatTone  = addKnob (mixer, P_satTone,  "MID", 2, "", gd);
    kSatBass  = addKnob (mixer, P_satBass,  "LOW", 2, "", gd);



    tWah = addToggle (mixer, P_wahOn, "ACTIVE", pp);
    kWahPos = addKnob (mixer, P_wahPos, "PEDAL",  2, "", pp);
    kWahMix = addKnob (mixer, P_wahMix, "AMOUNT", 2, "", pp);

    tPh = addToggle (mixer, P_phOn, "ACTIVE", lp);
    kPhRate  = addKnob (mixer, P_phRate,  "RATE",   2, "Hz", lp);
    kPhDepth = addKnob (mixer, P_phDepth, "DEPTH",  2, "", lp);
    kPhMix   = addKnob (mixer, P_phMix,   "AMOUNT", 2, "", lp);

    kDlTime = addKnob (mixer, P_delayTime,  "TIME",     0, "ms", tq);
    kDlFb   = addKnob (mixer, P_delayFb,    "FEEDBACK", 2, "", tq);
    kDlLvl  = addKnob (mixer, P_delayLevel, "LEVEL",    1, "dB", tq);
    kRvSize = addKnob (mixer, P_rvSize,     "SIZE",     2, "", cr);
    kRvLvl  = addKnob (mixer, P_rvLevel,    "LEVEL",    1, "dB", cr);

    // ---- arpeggiator ----
    tArp = addToggle (page, P_arpOn, "ARP ON", gd);
    tArpHold = addToggle (page, P_arpHold, "HOLD", gd);
    addCombo (page, arpModeBox, P_arpMode, arpModeAtt);
    addCombo (page, arpRateBox, P_arpRate, arpRateAtt);
    arpModeBox.setColour (juce::ComboBox::outlineColourId, gd);
    arpRateBox.setColour (juce::ComboBox::outlineColourId, gd);
    kArpOct  = addKnob (page, P_arpOct,  "OCTAVES", 0, "", gd);
    kArpGate = addKnob (page, P_arpGate, "GATE",    2, "", gd);

    // ---- oscillator layer ----
    tOscOn   = addToggle (page, P_oscOn,   "OSCILLATOR ON", pp);
    tOscOnly = addToggle (page, P_oscOnly, "OSC ONLY", pp);
    addCombo (page, oscWaveBox, P_oscWave, oscWaveAtt);
    oscWaveBox.setColour (juce::ComboBox::outlineColourId, pp);
    kOscLvl  = addKnob (page, P_oscLevel, "LEVEL",  1, "dB", pp);
    kOscTune = addKnob (page, P_oscTune,  "TUNE",   0, "semi", pp);
    kOscFine = addKnob (page, P_oscFine,  "FINE",   0, "ct", pp);
    kOscCut  = addKnob (page, P_oscCut,   "FILTER", 0, "Hz", pp);

    // ---- wheels + bend range ----
    page.addAndMakeVisible (pitchWheel);
    page.addAndMakeVisible (modWheelView);
    page.addAndMakeVisible (expWheel);
    learnTargets.push_back ({ &expWheel, -2 });
    expWheel.addMouseListener (this, false);
    setupStepBox (pbBox, pbAtt, "pbRange", false);
    setupStepBox (trBox, trAtt, "transpose", true);
    setupStepBox (ocBox, ocAtt, "octave", true);

    // ---- settings page: folders, instrument, mapping ----
    juce::Path left, right;
    left.addTriangle (0.f, 0.5f, 1.f, 0.f, 1.f, 1.f);
    right.addTriangle (1.f, 0.5f, 0.f, 0.f, 0.f, 1.f);
    for (auto* b : { &prevA, &prevA2, &prevD })  b->setShape (left, false, true, false);
    for (auto* b : { &nextA, &nextA2, &nextD })  b->setShape (right, false, true, false);
    page.addAndMakeVisible (prevA);   page.addAndMakeVisible (nextA);
    page.addAndMakeVisible (prevD);   page.addAndMakeVisible (nextD);
    prevD.onClick = [this] { stepDuo (-1); };
    nextD.onClick = [this] { stepDuo (1); };
    settings.addAndMakeVisible (prevA2); settings.addAndMakeVisible (nextA2);

    prevA.onClick = prevA2.onClick = [this] { stepGroup (-1); };
    nextA.onClick = nextA2.onClick = [this] { stepGroup (1); };

    addButton.setColour (juce::TextButton::buttonColourId, tq);
    renameButton.setColour (juce::TextButton::buttonColourId, tq);
    mapButton.setColour (juce::TextButton::buttonColourId, gd);
    settings.addAndMakeVisible (addButton);
    settings.addAndMakeVisible (renameButton);
    addAndMakeVisible (mapButton);

    addButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Pick a sample folder (or a folder of instrument folders)", juce::File(), "");
        juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe == nullptr)
                                      return;

                                  const auto dir = fc.getResult();
                                  if (dir.isDirectory())
                                      safe->proc.addFolder (dir);

                                  safe->refreshGroups();
                              });
    };

    renameButton.onClick = [this]
    {
        const int idx = proc.getActiveGroup();
        if (idx < 0)
            return;

        auto* w = new juce::AlertWindow ("Rename instrument", "Enter a name:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", proc.getGroupName (idx));
        w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);
        w->enterModalState (true,
                            juce::ModalCallbackFunction::create ([safe, w, idx] (int result)
                            {
                                if (result == 1 && safe != nullptr)
                                {
                                    safe->proc.renameGroup (idx, w->getTextEditorContents ("name"));
                                    safe->refreshGroups();
                                }
                            }),
                            true);
    };

    // ---- user instruments: NEW (name it, then drop WAVs on the mapping) / SAVE (one .esi file) ----
    newInstBtn.setColour (juce::TextButton::buttonColourId, gd);
    saveInstBtn.setColour (juce::TextButton::buttonColourId, pp);
    settings.addAndMakeVisible (newInstBtn);
    settings.addAndMakeVisible (saveInstBtn);
    newInstBtn.onClick = [this]
    {
        promptName ("New instrument", "My Instrument", [this] (juce::String nm)
        {
            proc.createInstrument (nm);
            refreshGroups();
            info.setText ("New instrument \"" + nm.trim() + "\" - now drag & drop WAV files on the mapping", juce::dontSendNotification);
        });
    };
    saveInstBtn.onClick = [this]
    {
        const int idx = proc.getActiveGroup();
        if (idx < 0)
            return;
        const bool wasUser = proc.isUserInstrument (idx);
        const bool ok = proc.saveInstrument (idx);
        refreshGroups();
        info.setText (ok ? (wasUser ? "Saved:  " : "Saved as a new user instrument:  ") + proc.getGroupPath (idx)
                         : "SAVE FAILED - cannot write " + proc.getGroupPath (idx),
                      juce::dontSendNotification);
    };
    mapView.onChanged = [this] (const juce::String& msg)
    {
        refreshGroups();
        info.setText (msg, juce::dontSendNotification);
    };

    // ---- top buttons: BROWSE / MIXER / SETTINGS (click the active one again to go back) ----
    browseButton.setColour (juce::TextButton::buttonColourId, tq);
    mixerButton.setColour (juce::TextButton::buttonColourId, lp);
    addAndMakeVisible (browseButton);
    addAndMakeVisible (mixerButton);
    mapButton.onClick    = [this] { showView (view == 3 ? 0 : 3); };
    browseButton.onClick = [this] { showView (view == 1 ? 0 : 1); };
    mixerButton.onClick  = [this] { showView (view == 2 ? 0 : 2); };

    // ---- mixer page ----
    kMainLvl = addKnob (mixer, P_mainLevel, "LEVEL", 1, "dB", tq);
    kMainPan = addKnob (mixer, P_mainPan,   "PAN",   2, "",   tq);
    tMainMute = addToggle (mixer, P_mainMute, "MUTE", cr);
    kDuoLvl  = addKnob (mixer, P_duoLevel, "LEVEL", 1, "dB", gd);
    kDuoPan  = addKnob (mixer, P_duoPan,   "PAN",   2, "",   gd);
    tDuoMute = addToggle (mixer, P_duoMute, "MUTE", cr);
    kMaster  = addKnob (mixer, P_master, "MASTER LEVEL", 1, "dB", pp);

    auto mkFader = [] (Knob* k, juce::Colour col)
    {
        k->slider.setSliderStyle (juce::Slider::LinearVertical);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
        k->slider.setColour (juce::Slider::trackColourId, col);
        k->label.setVisible (false);
    };
    mkFader (kMainLvl, tq);  mkFader (kDuoLvl, gd);  mkFader (kSatMix, gd);  mkFader (kWahMix, pp);
    mkFader (kPhMix, lp);    mkFader (kDlLvl, tq);   mkFader (kRvLvl, cr);    mkFader (kMaster, pp);

    // ---- OSC channel strip: its own fader / pan / mute / SOLO ----
    tOscSoloMix = addToggle (mixer, P_oscOnly, "SOLO", gd);
    kOscFader   = addKnob (mixer, P_oscLevel, "LEVEL", 1, "dB", pp);
    kOscPan     = addKnob (mixer, P_oscPan, "PAN", 2, "", pp);
    tOscMuteMix = addToggle (mixer, P_oscMute, "MUTE", cr);
    tOscOnMix   = addToggle (mixer, P_oscOn, "ON", pp);
    mkFader (kOscFader, pp);

    // ---- voice edit page (Yamaha-style portamento / velocity / touch) ----
    tPorta = addToggle (voiceEd, P_portaOn, "MAGIC LEGATO", cr);
    tMagic = addToggle (page, P_portaOn, "MAGIC LEGATO", cr);
    vk[0] = addKnob (voiceEd, P_magicTime, "TIME",       0, "",   tq);
    vk[1] = addKnob (voiceEd, P_fastThr,  "THRESHOLD",   0, "",   tq);
    vk[2] = addKnob (voiceEd, P_fastTime, "FAST TIME",   0, "",   tq);
    vk[3] = addKnob (voiceEd, P_vSens,    "VEL SENS",    0, "",   gd);
    vk[4] = addKnob (voiceEd, P_vRef,     "REF VELOCITY", 0, "",  gd);
    vk[5] = addKnob (voiceEd, P_minTime,  "MIN TIME",    0, "",   gd);
    vk[6] = addKnob (voiceEd, P_velLo,    "LOW",         0, "",   pp);
    vk[7] = addKnob (voiceEd, P_velHi,    "HIGH",        0, "",   pp);
    vk[8] = addKnob (voiceEd, P_tDepth,   "DEPTH",       0, "",   cr);
    vk[9] = addKnob (voiceEd, P_tOff,     "OFFSET",      0, "",   cr);
    vk[10] = addKnob (voiceEd, P_kCut,     "CUTOFF",    0, "", cr);
    vk[11] = addKnob (voiceEd, P_kRes,     "RESONANCE", 0, "", cr);
    vk[12] = addKnob (voiceEd, P_yamWah,   "WAH",       2, "", cr);
    vk[13] = addKnob (voiceEd, P_yamDrive, "DRIVE 2",   2, "", cr);
    vk[14] = addKnob (voiceEd, P_yamTone,  "TONE",      2, "", cr);
    vk[15] = addKnob (voiceEd, P_yamShine, "SHINE",     2, "", cr);
    vk[16] = addKnob (voiceEd, P_yamMix,   "FX2 MIX",   2, "", cr);
    voiceButton.setColour (juce::TextButton::buttonColourId, pp);
    addAndMakeVisible (voiceButton);
    voiceButton.onClick = [this] { showView (view == 4 ? 0 : 4); };

    // ---- browse page ----
    searchBox.setTextToShowWhenEmpty ("search by name...", neon::dim);
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff030201));
    searchBox.setColour (juce::TextEditor::textColourId, neon::text);
    searchBox.setColour (juce::TextEditor::outlineColourId, neon::amber.withAlpha (0.6f));
    searchBox.setFont (uiFont (14.0f, false));
    searchBox.onTextChange = [this] { rebuildShown(); };
    browse.addAndMakeVisible (searchBox);

    catModel.getCount = [this] { return (int) cats.size(); };
    catModel.getText  = [this] (int r) { return r >= 0 && r < (int) cats.size() ? cats[(size_t) r].second : juce::String(); };
    catModel.onSelect = [this] (int r) { selCat = r; rebuildShown(); };
    instModel.getCount = [this] { return (int) shown.size(); };
    instModel.getText = [this] (int r)
    {
        if (r < 0 || r >= (int) shown.size()) return juce::String();
        const auto& e = shown[(size_t) r];
        juce::String t = e.name;
        if (e.group >= 0 && e.group == proc.getActiveGroup()) t = "* " + t;
        else if (e.group >= 0 && e.group == proc.getDuoGroup()) t = "+ " + t;
        return t;
    };
    instModel.onSelect = [this] (int) { updateBrowseButtons(); };

    for (auto* lb : { &catList, &instList })
    {
        lb->setRowHeight (30);
        lb->setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff030201));
        lb->setColour (juce::ListBox::outlineColourId, neon::amber.withAlpha (0.6f));
        lb->setOutlineThickness (1);
        browse.addAndMakeVisible (*lb);
    }
    catList.setModel (&catModel);
    instList.setModel (&instModel);

    auto loadSelected = [this]
    {
        const int r = instList.getSelectedRow();
        if (r < 0 || r >= (int) shown.size())
            return;
        const auto e = shown[(size_t) r];
        if (e.group >= 0)
            proc.selectGroup (e.group);
        else if (e.preset.existsAsFile())
            proc.loadPreset (e.preset);
        refreshGroups();
        rebuildBrowse();
    };
    loadBtn.onClick = loadSelected;
    instModel.onDouble = [loadSelected] (int) { loadSelected(); };

    duoBtn.onClick = [this]
    {
        const int r = instList.getSelectedRow();
        if (r >= 0 && r < (int) shown.size() && shown[(size_t) r].group >= 0)
        {
            proc.selectDuo (shown[(size_t) r].group);
            refreshGroups();
            rebuildShown();
        }
    };
    noDuoBtn.onClick = [this] { proc.selectDuo (-1); refreshGroups(); rebuildShown(); };

    saveBtn.onClick = [this]
    {
        promptName ("Save preset", proc.getPresetName().isEmpty() ? juce::String ("My Preset") : proc.getPresetName(),
                    [this] (juce::String n)
                    {
                        if (proc.savePreset (n))
                            rebuildBrowse ((int) cats.size() - 1);
                    });
    };
    renPresetBtn.onClick = [this]
    {
        const int r = instList.getSelectedRow();
        if (r < 0 || r >= (int) shown.size() || ! shown[(size_t) r].user)
            return;
        const auto f = shown[(size_t) r].preset;
        promptName ("Rename preset", f.getFileNameWithoutExtension(), [this, f] (juce::String n)
                    {
                        NayAudioProcessor::renamePreset (f, n);
                        rebuildBrowse ((int) cats.size() - 1);
                    });
    };
    delPresetBtn.onClick = [this]
    {
        const int r = instList.getSelectedRow();
        if (r < 0 || r >= (int) shown.size() || ! shown[(size_t) r].user)
            return;
        const auto f = shown[(size_t) r].preset;

        auto* w = new juce::AlertWindow ("Delete preset", "Delete  \"" + f.getFileNameWithoutExtension() + "\" ?", juce::MessageBoxIconType::NoIcon);
        w->addButton ("Delete", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);
        w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, f] (int result)
                            {
                                if (result == 1 && safe != nullptr)
                                {
                                    NayAudioProcessor::deletePreset (f);
                                    safe->rebuildBrowse ((int) safe->cats.size() - 1);
                                }
                            }), true);
    };

    loadBtn.setColour (juce::TextButton::buttonColourId, tq);
    duoBtn.setColour (juce::TextButton::buttonColourId, gd);
    noDuoBtn.setColour (juce::TextButton::buttonColourId, gd.darker (0.3f));
    saveBtn.setColour (juce::TextButton::buttonColourId, pp);
    renPresetBtn.setColour (juce::TextButton::buttonColourId, lp);
    delPresetBtn.setColour (juce::TextButton::buttonColourId, cr);
    for (auto* b : { &loadBtn, &duoBtn, &noDuoBtn, &saveBtn, &renPresetBtn, &delPresetBtn })
        browse.addAndMakeVisible (*b);

    browseInfo.setColour (juce::Label::textColourId, neon::dim);
    browseInfo.setFont (uiFont (11.5f, false));
    browseInfo.setJustificationType (juce::Justification::topLeft);
    browse.addAndMakeVisible (browseInfo);

    browse.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -browse.getX(), -browse.getY());
        g.setColour (neon::amber);
        g.setFont (uiFont (11.0f));
        g.drawText ("CATEGORIES", 0, 38 - 16, 300, 14, juce::Justification::centredLeft, false);
        g.drawText ("INSTRUMENTS  /  PRESETS", 312, 38 - 16, 430, 14, juce::Justification::centredLeft, false);
        g.drawText ("PRESETS", 754, 176, 214, 14, juce::Justification::centredLeft, false);
        g.setColour (neon::dim);
        g.setFont (uiFont (10.5f, false));
        g.drawText ("* = main    + = duo    (double-click = load)", 312, 566, 430, 14, juce::Justification::centredLeft, false);
    };

    voiceEd.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -voiceEd.getX(), -voiceEd.getY());
        drawPanel (g, vePorta, "MAGIC LEGATO  (portamento)", neon::cyan);
        drawPanel (g, veVel,   "VELOCITY TO PORTAMENTO TIME", neon::amber);
        drawPanel (g, veLimit, "VELOCITY LIMIT", neon::green);
        drawPanel (g, veTouch, "TOUCH SENSITIVITY", neon::magenta);
        drawPanel (g, veSound, "KRONOS SOUND  (the KRONOS button switches all of this on)", neon::magenta);
        g.setFont (uiFont (10.5f, false));
        g.setColour (neon::dim);
        g.drawText ("CUTOFF below 0 darkens the sound first (like the organ's filter).  Then wah + overdrive, KRONOS saturation, SHINE adds the sparkle.",
                    veSound.getX() + 14, veSound.getBottom() - 20, 940, 14, juce::Justification::centredLeft, false);
        g.setFont (uiFont (10.5f, false));
        g.setColour (neon::dim);
        g.drawText ("These settings work only while MAGIC LEGATO is ON (it replaces GLIDE).  TIME = glide between notes.  THRESHOLD / FAST TIME = quicker glide when notes are played fast.",
                    vePorta.getX() + 14, vePorta.getBottom() - 20, 940, 14, juce::Justification::centredLeft, false);
        g.drawText ("Higher velocity = shorter glide (SENS = amount, REF = neutral velocity). MIN TIME = shortest glide.",
                    veVel.getX() + 14, veVel.getBottom() - 20, 940, 14, juce::Justification::centredLeft, false);
        g.drawText ("Incoming velocity is kept between LOW and HIGH.", veLimit.getX() + 14, veLimit.getBottom() - 20, 440, 14, juce::Justification::centredLeft, false);
        g.drawText ("DEPTH = response curve, OFFSET = overall touch level (64 = neutral).", veTouch.getX() + 14, veTouch.getBottom() - 20, 440, 14, juce::Justification::centredLeft, false);
    };

    mixer.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -mixer.getX(), -mixer.getY());

        const char* titles[9] = { "MAIN", "DUO", "OSC", "KRONOS", "WAH", "PHASER", "DELAY", "REVERB", "MASTER" };
        const juce::Colour cols[9] = { neon::cyan, neon::amber, neon::green, neon::amber, neon::green, neon::violet, neon::cyan, neon::magenta, neon::green };
        for (int i = 0; i < 9; ++i)
            drawPanel (g, mixStrip[i], titles[i], cols[i]);

        const int a = proc.getActiveGroup(), d = proc.getDuoGroup();
        g.setFont (uiFont (10.5f, false));
        g.setColour (neon::text);
        g.drawText (a >= 0 ? proc.getGroupName (a) : juce::String ("NO INSTRUMENT"), mixStrip[0].getX() + 6, 24, mixStrip[0].getWidth() - 12, 14, juce::Justification::centred, true);
        g.drawText (d >= 0 ? proc.getGroupName (d) : juce::String ("OFF"), mixStrip[1].getX() + 6, 24, mixStrip[1].getWidth() - 12, 14, juce::Justification::centred, true);

        // master level meter
        {
            const auto& s = mixStrip[8];
            for (int ch = 0; ch < 2; ++ch)
            {
                const float lv = proc.getMeter (ch);
                const float db = 20.0f * std::log10 (juce::jmax (1.0e-4f, lv));
                const float fr = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
                const float bx = (float) s.getRight() - 24.0f + (float) ch * 10.0f, by = 80.0f, bh = 268.0f;
                g.setColour (juce::Colour (0xff030201));
                g.fillRoundedRectangle (bx, by, 8.0f, bh, 3.0f);
                const float fh2 = bh * fr;
                juce::ColourGradient cg (juce::Colour (0xffe8fbff), 0.0f, by, neon::led.darker (0.3f), 0.0f, by + bh, false);
                cg.addColour (0.2, neon::led);
                g.setGradientFill (cg);
                g.fillRoundedRectangle (bx, by + bh - fh2, 8.0f, fh2, 3.0f);
                g.setColour (neon::led.withAlpha (0.18f));
                g.fillRoundedRectangle (bx - 2.0f, by + bh - fh2, 12.0f, fh2, 4.0f);
                g.setColour (neon::bronze.brighter (0.3f));
                g.drawRoundedRectangle (bx, by, 8.0f, bh, 3.0f, 1.0f);
            }
        }

        g.setColour (neon::dim);
        g.setFont (uiFont (10.5f, false));
        g.drawText ("Pan: center = unity gain.  Right-click any knob / fader = MIDI learn.", 0, 578, 968, 14, juce::Justification::centredLeft, false);
    };

    info.setColour (juce::Label::textColourId, neon::dim);
    info.setFont (uiFont (11.0f, false));
    info.setJustificationType (juce::Justification::topLeft);
    settings.addAndMakeVisible (info);

    page.onMouse = [this] (const juce::MouseEvent& e)
    {
        if (lcdA.contains (e.getPosition())) showVoiceMenu (false);
        else if (lcdD.contains (e.getPosition())) showVoiceMenu (true);
    };

    // ---------------- main page painting ----------------
    page.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -page.getX(), -page.getY());

        const int a = proc.getActiveGroup();
        auto bank = proc.getBankOf (a);
        drawLcd (g, lcdA, "INSTRUMENT", a >= 0 ? proc.getGroupName (a) : "NO INSTRUMENT",
                 a >= 0 ? juce::String (bank != nullptr ? (int) bank->size() : 0) + " samples"
                        : "open BROWSE or SETTINGS > ADD FOLDER", neon::cyan);
        {
            const int d = proc.getDuoGroup();
            auto dbank = proc.getBankOf (d);
            drawLcd (g, lcdD, "DUO  //  2nd INSTRUMENT", d >= 0 ? proc.getGroupName (d) : "OFF",
                     d >= 0 ? juce::String (dbank != nullptr ? (int) dbank->size() : 0) + " samples"
                            : "BROWSE > ADD AS DUO", neon::amber);
        }

        drawPanel (g, pVoice, "VOICE", neon::cyan);
        drawPanel (g, pMaqam, "SCALE TUNE SETTING", neon::amber);
        drawPanel (g, pPerf, "OSCILLATOR", neon::green);
        drawAnubis (g, (float) pPerf.getX() + 360.0f, (float) pPerf.getY() + 10.0f, 118.0f, 0.10f, false);   // faint watermark
        drawPanel (g, pArp, "ARPEGGIATOR", neon::amber);
        drawPanel (g, pWheels, "WHEELS", neon::cyan);
        g.setFont (uiFont (10.5f));
        g.setColour (neon::dim);
        g.drawText ("WAVE", pPerf.getX() + 176, pPerf.getY() + 30, 140, 12, juce::Justification::centredLeft, false);
        g.setFont (uiFont (10.5f, false));
        g.drawText ("Plays with the samples and follows glide, bend, vibrato and the maqam.  OSC ONLY = the oscillator alone.  Goes through all the effects.",
                    pPerf.getX() + 176, pPerf.getY() + 100, 330, 28, juce::Justification::topLeft, true);
        g.setFont (uiFont (9.0f));
        g.setColour (neon::dim);
        g.drawText ("PITCH", pWheels.getX() + 4, pWheels.getY() + 24, 36, 10, juce::Justification::centred, false);
        g.drawText ("MOD", pWheels.getX() + 36, pWheels.getY() + 24, 36, 10, juce::Justification::centred, false);
        g.drawText ("EXP", pWheels.getX() + 68, pWheels.getY() + 24, 36, 10, juce::Justification::centred, false);
        g.drawText ("BEND RANGE", pWheels.getX() + 106, pWheels.getY() + 24, 92, 10, juce::Justification::centred, false);
        g.setFont (uiFont (10.5f));
        g.setColour (neon::dim);
        g.drawText ("PATTERN", pArp.getX() + 128, pArp.getY() + 24, 100, 12, juce::Justification::centredLeft, false);
        g.drawText ("RATE (BPM)", pArp.getX() + 276, pArp.getY() + 24, 100, 12, juce::Justification::centredLeft, false);

        g.setFont (uiFont (9.5f));
        g.setColour (neon::dim);
        g.drawText ("TRANSPOSE (semitones)", pMaqam.getX() + 14, pMaqam.getY() + 28, 112, 12, juce::Justification::centred, false);
        g.drawText ("OCTAVE", pMaqam.getX() + 130, pMaqam.getY() + 28, 108, 12, juce::Justification::centred, false);
        g.setColour (neon::dim);
        g.setFont (uiFont (10.5f, false));
        g.drawText ("Altered notes light up.  Click = quarter-tone on/off.  Drag = fine tune.",
                    pMaqam.getX() + 330, pMaqam.getY() + 6, 630, 14, juce::Justification::centredRight, false);
        g.setFont (uiFont (9.5f));
        g.drawText ("SCALE TUNE MEMORY: MEMORY then 1-5 = store, 1-5 = recall", pMaqam.getX() + 776, pMaqam.getY() + 130, 190, 12, juce::Justification::centred, false);
    };

    // ---------------- settings page painting ----------------
    settings.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -settings.getX(), -settings.getY());

        const int a = proc.getActiveGroup();
        auto bank = proc.getBankOf (a);
        juce::String sub = "press NEW INSTRUMENT or ADD FOLDER";
        if (a >= 0)
        {
            sub = juce::String (bank != nullptr ? (int) bank->size() : 0) + " samples";
            if (proc.isUserInstrument (a))
                sub << (proc.isDirty (a) ? "   *  UNSAVED CHANGES - press SAVE INSTRUMENT" : "   -  my instrument (saved)");
            else
                sub << "   -  folder  (SAVE INSTRUMENT = make it one file)";
        }
        drawLcd (g, lcdA2, "INSTRUMENT", a >= 0 ? proc.getGroupName (a) : "NO INSTRUMENT", sub,
                 a >= 0 && proc.isDirty (a) ? neon::magenta : neon::cyan);
    };

    refreshGroups();
    built = true;
    setSize (1064, 742); // last, so resized() finds every component already created
    startTimerHz (20);
}

NayAudioProcessorEditor::~NayAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

NayAudioProcessorEditor::Knob* NayAudioProcessorEditor::addKnob (juce::Component& parent, int idx, const char* name, int decimals,
                                                                 const juce::String& suffix, juce::Colour accent)
{
    auto k = std::make_unique<Knob>();
    auto& s = k->slider;

    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 14);
    s.setColour (juce::Slider::rotarySliderFillColourId, accent);
    s.setColour (juce::Slider::textBoxTextColourId, neon::text);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], s);

    // set after the attachment so our text formatting wins
    s.textFromValueFunction = [decimals, suffix] (double v) -> juce::String
    {
        if (suffix == "dB" && v <= -59.9)
            return "OFF";
        if (suffix == "st")
            return juce::String (juce::CharPointer_UTF8 ("\xc2\xb1")) + juce::String ((int) v) + " st";
        return juce::String (v, decimals) + (suffix.isEmpty() ? juce::String() : " " + suffix);
    };
    s.valueFromTextFunction = [] (const juce::String& t) -> double
    {
        return t.trim().startsWithIgnoreCase ("off") ? -60.0 : t.retainCharacters ("-0123456789.").getDoubleValue();
    };
    s.updateText();

    k->label.setText (name, juce::dontSendNotification);
    k->label.setFont (uiFont (10.0f));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, neon::dim);
    k->label.setInterceptsMouseClicks (false, false);

    learnTargets.push_back ({ &s, idx });
    s.addMouseListener (this, false);
    parent.addAndMakeVisible (s);
    parent.addAndMakeVisible (k->label);

    knobs.push_back (std::move (k));
    return knobs.back().get();
}

juce::ToggleButton* NayAudioProcessorEditor::addToggle (juce::Component& parent, int idx, const juce::String& text, juce::Colour accent)
{
    auto t = std::make_unique<juce::ToggleButton> (text);
    t->setColour (juce::ToggleButton::tickColourId, accent);
    toggleAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], *t));
    learnTargets.push_back ({ t.get(), idx });
    t->addMouseListener (this, false);
    parent.addAndMakeVisible (*t);
    toggles.push_back (std::move (t));
    return toggles.back().get();
}

void NayAudioProcessorEditor::addCombo (juce::Component& parent, juce::ComboBox& box, int idx,
                                        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
{
    if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (NayAudioProcessor::paramIds[idx])))
        box.addItemList (ch->choices, 1);

    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], box);
    box.setColour (juce::ComboBox::outlineColourId, neon::cyan);
    learnTargets.push_back ({ &box, idx });
    box.addMouseListener (this, false);
    parent.addAndMakeVisible (box);
}

void NayAudioProcessorEditor::layoutRow (juce::Rectangle<int> area, std::initializer_list<Knob*> ks)
{
    const int n = (int) ks.size();
    const int cw = area.getWidth() / juce::jmax (1, n);
    int i = 0;
    for (auto* k : ks)
    {
        auto cell = juce::Rectangle<int> (area.getX() + i * cw, area.getY(), cw, area.getHeight());
        k->label.setBounds (cell.removeFromTop (14));
        k->slider.setBounds (cell);
        ++i;
    }
}

//==============================================================================
void NayAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    int target = -99;
    for (auto& p : learnTargets)
        if (p.first == e.eventComponent) { target = p.second; break; }
    if (target == -99)
        return;

    const int cc = proc.getAssignedCC (target);
    juce::PopupMenu m;
    m.addItem (1, proc.getLearnTarget() == target ? "Cancel MIDI Learn"
                                                  : "MIDI Learn  (then move a knob / pedal on your device)");
    m.addItem (2, target == -2 ? "Reset pedal to CC 11" : (cc >= 0 ? "Clear assignment (CC " + juce::String (cc) + ")" : "Clear assignment"),
               target == -2 ? cc != 11 : cc >= 0);

    juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, target] (int r)
    {
        if (safe == nullptr) return;
        if (r == 1)      safe->proc.setLearnTarget (target);
        else if (r == 2) safe->proc.clearAssign (target);
        safe->repaint (safe->midiRect);
    });
}

void NayAudioProcessorEditor::timerCallback()
{
    {   // MAGIC LEGATO takes over: the plain GLIDE knob, MONO and LEGATO switches are locked while it is on
        const bool magic = proc.apvts.getRawParameterValue ("portaOn")->load() > 0.5f;
        if (kGlide != nullptr && tMono != nullptr && tLegato != nullptr && kGlide->slider.isEnabled() == magic)
        {
            kGlide->slider.setEnabled (! magic);
            tMono->setEnabled (! magic);
            tLegato->setEnabled (! magic);
            const float a = magic ? 0.35f : 1.0f;
            kGlide->slider.setAlpha (a);
            kGlide->label.setAlpha (a);
            tMono->setAlpha (a);
            tLegato->setAlpha (a);
        }
    }
    {
        int cc, tgt, cnt;
        proc.getLearnDone (cc, tgt, cnt);
        if (cnt != lastLearnCount) { lastLearnCount = cnt; lastLearnMs = juce::Time::getMillisecondCounter(); repaint (midiRect); }
        if (proc.getLearnTarget() != lastLearnT) { lastLearnT = proc.getLearnTarget(); repaint (midiRect); }
        if (juce::Time::getMillisecondCounter() - lastLearnMs < 3100) repaint (midiRect);
    }

    {
        const auto& plist = proc.getParameters();
        if (! paramsInit || lastParamVals.size() != (size_t) plist.size())
        {
            lastParamVals.clear();
            for (auto* q : plist) lastParamVals.push_back (q->getValue());
            paramsInit = true;
        }
        else
        {
            for (int i = 0; i < plist.size(); ++i)
            {
                const float v = plist[i]->getValue();
                if (std::abs (v - lastParamVals[(size_t) i]) > 1.0e-6f)
                {
                    lastParamVals[(size_t) i] = v;
                    touchText = plist[i]->getName (28) + ": " + plist[i]->getCurrentValueAsText();
                    touchMs = juce::Time::getMillisecondCounter();
                    lastMonMs = touchMs;
                    repaint (midiRect);
                }
            }
        }
    }

    int k, a, b, cnt;
    proc.getMidiMonitor (k, a, b, cnt);
    const bool flash = juce::Time::getMillisecondCounter() - lastMonMs < 400;
    if (cnt != lastMonCount)
    {
        lastMonCount = cnt;
        lastMonMs = juce::Time::getMillisecondCounter();
        lastMidiMs = lastMonMs;
        repaint (midiRect);
    }
    else if (flash)
        repaint (midiRect);

    pitchWheel.repaint();
    modWheelView.repaint();
    expWheel.repaint();

    if ((++tick & 1) == 0)
    {
        if (page.isVisible()) page.repaint();
        if (mixer.isVisible()) mixer.repaint();
        if (settings.isVisible()) mapView.repaint();
    }
}

void NayAudioProcessorEditor::stepGroup (int delta)
{
    const int n = proc.getNumGroups();
    if (n <= 0)
        return;

    proc.selectGroup ((proc.getActiveGroup() + delta + n) % n);
    refreshGroups();
}

void NayAudioProcessorEditor::setupStepBox (juce::Slider& s, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att,
                                            const char* paramId, bool showSign)
{
    s.setSliderStyle (juce::Slider::IncDecButtons);
    s.setIncDecButtonsMode (juce::Slider::incDecButtonsNotDraggable);
    s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 32);
    s.setColour (juce::Slider::textBoxTextColourId, neon::amber);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff030201));
    s.setColour (juce::Slider::textBoxOutlineColourId, neon::amber.withAlpha (0.6f));
    s.setColour (juce::TextButton::buttonColourId, neon::amber);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, s);
    s.textFromValueFunction = [showSign] (double v)
    {
        const int i = (int) std::lround (v);
        return (showSign && i > 0 ? juce::String ("+") : juce::String()) + juce::String (i);
    };
    s.updateText();
    page.addAndMakeVisible (s);
}

void NayAudioProcessorEditor::stepDuo (int delta)
{
    const int n = proc.getNumGroups();
    if (n <= 0)
        return;

    int next = proc.getDuoGroup() + delta;   // -1 = off
    if (next >= n)  next = -1;
    if (next < -1)  next = n - 1;

    proc.selectDuo (next);
    refreshGroups();
}

void NayAudioProcessorEditor::showVoiceMenu (bool duo)
{
    const int n = proc.getNumGroups();
    juce::PopupMenu m;
    m.addSectionHeader (duo ? "DUO  -  2nd instrument" : "MAIN  -  instrument");
    if (duo)
        m.addItem (1000, "OFF  (no duo)", true, proc.getDuoGroup() < 0);

    juce::StringArray cats;
    for (int i = 0; i < n; ++i)
        if (! cats.contains (proc.getGroupCategory (i)))
            cats.add (proc.getGroupCategory (i));

    const int cur = duo ? proc.getDuoGroup() : proc.getActiveGroup();
    for (const auto& cat : cats)
    {
        m.addSeparator();
        m.addSectionHeader (cat.isEmpty() ? juce::String ("OTHER") : cat);
        for (int i = 0; i < n; ++i)
            if (proc.getGroupCategory (i) == cat)
                m.addItem (i + 1, proc.getGroupName (i), true, i == cur);
    }

    if (n == 0)
        m.addItem (2000, "no instruments yet - open BROWSE or SETTINGS", false);

    auto target = juce::Rectangle<int> ((duo ? lcdD : lcdA).getX(), (duo ? lcdD : lcdA).getY(), (duo ? lcdD : lcdA).getWidth(), (duo ? lcdD : lcdA).getHeight());
    target = page.localAreaToGlobal (target);

    juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (target).withMinimumWidth (target.getWidth()).withMaximumNumColumns (3),
                     [safe, duo] (int r)
                     {
                         if (safe == nullptr || r == 0)
                             return;
                         if (duo) safe->proc.selectDuo (r == 1000 ? -1 : r - 1);
                         else     safe->proc.selectGroup (r - 1);
                         safe->refreshGroups();
                     });
}

void NayAudioProcessorEditor::showView (int v)
{
    view = v;
    page.setVisible (v == 0);
    browse.setVisible (v == 1);
    mixer.setVisible (v == 2);
    voiceEd.setVisible (v == 4);
    voiceButton.setToggleState (v == 4, juce::dontSendNotification);
    voiceButton.setButtonText (v == 4 ? "BACK" : "VOICE");
    settings.setVisible (v == 3);
    if (v != 3 && waveEd.isVisible())
    {
        proc.stopPreview();
        waveEd.setVisible (false);
    }
    browseButton.setToggleState (v == 1, juce::dontSendNotification);
    mixerButton.setToggleState (v == 2, juce::dontSendNotification);
    mapButton.setToggleState (v == 3, juce::dontSendNotification);
    mapButton.setButtonText (v == 3 ? "BACK" : "SETTINGS");
    browseButton.setButtonText (v == 1 ? "BACK" : "BROWSE");
    mixerButton.setButtonText (v == 2 ? "BACK" : "MIXER");

    if (v == 1) rebuildBrowse();
    if (v == 3) mapView.refresh();
}

void NayAudioProcessorEditor::promptName (const juce::String& title, const juce::String& initial, std::function<void (juce::String)> cb)
{
    auto* w = new juce::AlertWindow (title, "Enter a name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", initial);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<NayAudioProcessorEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, cb] (int result)
                        {
                            if (result == 1 && safe != nullptr)
                                cb (w->getTextEditorContents ("name"));
                        }), true);
}

void NayAudioProcessorEditor::rebuildBrowse (int forceCat)
{
    cats.clear();
    cats.push_back ({ 0, "ALL INSTRUMENTS" });
    for (int i = 0; i < proc.getNumGroups(); ++i)
    {
        auto cat = proc.getGroupCategory (i);
        if (cat.isEmpty())
            cat = "Instruments";
        bool have = false;
        for (const auto& c0 : cats)
            if (c0.first == 1 && c0.second == cat)
                have = true;
        if (! have)
            cats.push_back ({ 1, cat });
    }
    cats.push_back ({ 2, "FACTORY PRESETS" });
    cats.push_back ({ 3, "USER PRESETS" });

    if (forceCat >= 0)
        selCat = forceCat;
    selCat = juce::jlimit (0, (int) cats.size() - 1, selCat);

    catList.updateContent();
    catList.selectRow (selCat, true, true);
    rebuildShown();
}

void NayAudioProcessorEditor::rebuildShown()
{
    shown.clear();
    const auto q = searchBox.getText().trim();

    if (selCat >= 0 && selCat < (int) cats.size())
    {
        const int kind = cats[(size_t) selCat].first;
        const auto cname = cats[(size_t) selCat].second;

        if (kind <= 1)
        {
            for (int i = 0; i < proc.getNumGroups(); ++i)
            {
                auto cat = proc.getGroupCategory (i);
                if (cat.isEmpty())
                    cat = "Instruments";
                const auto nm = proc.getGroupName (i);
                const bool inCat = kind == 0 || q.isNotEmpty() || cat == cname;
                if (inCat && (q.isEmpty() || nm.containsIgnoreCase (q)))
                {
                    BrowseEntry e;
                    e.name = nm;
                    e.group = i;
                    shown.push_back (e);
                }
            }
        }
        else
        {
            for (const auto& f : NayAudioProcessor::listPresets (kind == 2))
            {
                const auto nm = f.getFileNameWithoutExtension();
                if (q.isEmpty() || nm.containsIgnoreCase (q))
                {
                    BrowseEntry e;
                    e.name = nm;
                    e.preset = f;
                    e.user = kind == 3;
                    shown.push_back (e);
                }
            }
        }
    }

    instList.updateContent();
    instList.deselectAllRows();
    instList.repaint();
    updateBrowseButtons();
}

void NayAudioProcessorEditor::updateBrowseButtons()
{
    const int r = instList.getSelectedRow();
    const bool ok = r >= 0 && r < (int) shown.size();
    loadBtn.setEnabled (ok);
    duoBtn.setEnabled (ok && shown[(size_t) r].group >= 0);
    renPresetBtn.setEnabled (ok && shown[(size_t) r].user);
    delPresetBtn.setEnabled (ok && shown[(size_t) r].user);

    const int a = proc.getActiveGroup(), d = proc.getDuoGroup();
    browseInfo.setText ("Main: " + (a >= 0 ? proc.getGroupName (a) : juce::String ("none"))
                        + "     Duo: " + (d >= 0 ? proc.getGroupName (d) : juce::String ("off"))
                        + "     Preset: " + (proc.getPresetName().isEmpty() ? juce::String ("-") : proc.getPresetName())
                        + "\nLibrary folder:  " + NayAudioProcessor::libraryRoot().getFullPathName()
                        + "\nUser presets:  " + NayAudioProcessor::userPresetDir().getFullPathName(),
                        juce::dontSendNotification);
}

void NayAudioProcessorEditor::refreshGroups()
{
    info.setText (proc.getSampleInfo(), juce::dontSendNotification);
    mapView.refresh();
    page.repaint();
    settings.repaint();
    mixer.repaint();
}

//==============================================================================
// hieroglyph frieze symbols: ankh, Eye of Horus, djed, scarab, feather, water (drawn with paths, outlined)
static void drawGlyph (juce::Graphics& g, int kind, float cx, float cy, float s, juce::Colour col)
{
    juce::Path p;
    switch (kind % 6)
    {
        case 0: // ankh
            p.addEllipse (cx - s * 0.18f, cy - s * 0.5f, s * 0.36f, s * 0.42f);
            p.startNewSubPath (cx, cy - s * 0.08f); p.lineTo (cx, cy + s * 0.5f);
            p.startNewSubPath (cx - s * 0.3f, cy + s * 0.02f); p.lineTo (cx + s * 0.3f, cy + s * 0.02f);
            break;
        case 1: // Eye of Horus
            p.startNewSubPath (cx - s * 0.45f, cy - s * 0.05f);
            p.quadraticTo (cx, cy - s * 0.38f, cx + s * 0.45f, cy - s * 0.05f);
            p.quadraticTo (cx, cy + s * 0.2f, cx - s * 0.45f, cy - s * 0.05f);
            p.addEllipse (cx - s * 0.1f, cy - s * 0.17f, s * 0.2f, s * 0.2f);
            p.startNewSubPath (cx - s * 0.05f, cy + s * 0.08f); p.quadraticTo (cx - s * 0.1f, cy + s * 0.45f, cx - s * 0.3f, cy + s * 0.4f);
            p.startNewSubPath (cx + s * 0.1f, cy + s * 0.08f); p.lineTo (cx + s * 0.2f, cy + s * 0.45f);
            p.startNewSubPath (cx - s * 0.45f, cy - s * 0.3f); p.quadraticTo (cx, cy - s * 0.55f, cx + s * 0.45f, cy - s * 0.3f);
            break;
        case 2: // djed pillar
            p.startNewSubPath (cx - s * 0.12f, cy + s * 0.5f); p.lineTo (cx - s * 0.12f, cy - s * 0.5f);
            p.startNewSubPath (cx + s * 0.12f, cy + s * 0.5f); p.lineTo (cx + s * 0.12f, cy - s * 0.5f);
            for (int k = 0; k < 4; ++k)
            {
                const float yy = cy - s * 0.42f + (float) k * s * 0.12f;
                p.startNewSubPath (cx - s * 0.3f, yy); p.lineTo (cx + s * 0.3f, yy);
            }
            break;
        case 3: // scarab
            p.addEllipse (cx - s * 0.2f, cy - s * 0.18f, s * 0.4f, s * 0.55f);
            p.addEllipse (cx - s * 0.11f, cy - s * 0.38f, s * 0.22f, s * 0.2f);
            p.startNewSubPath (cx, cy - s * 0.18f); p.lineTo (cx, cy + s * 0.37f);
            for (int sd = -1; sd <= 1; sd += 2)
            {
                p.startNewSubPath (cx + (float) sd * s * 0.2f, cy - s * 0.05f); p.lineTo (cx + (float) sd * s * 0.42f, cy - s * 0.2f);
                p.startNewSubPath (cx + (float) sd * s * 0.2f, cy + s * 0.2f);  p.lineTo (cx + (float) sd * s * 0.42f, cy + s * 0.38f);
            }
            break;
        case 4: // feather of Maat
            p.startNewSubPath (cx, cy + s * 0.5f);
            p.quadraticTo (cx - s * 0.22f, cy - s * 0.1f, cx + s * 0.05f, cy - s * 0.5f);
            p.quadraticTo (cx + s * 0.22f, cy - s * 0.05f, cx, cy + s * 0.5f);
            p.startNewSubPath (cx, cy + s * 0.5f); p.lineTo (cx + s * 0.03f, cy - s * 0.35f);
            break;
        default: // water
            for (int k = 0; k < 2; ++k)
            {
                const float yy = cy - s * 0.08f + (float) k * s * 0.22f;
                p.startNewSubPath (cx - s * 0.4f, yy);
                for (int z = 1; z <= 4; ++z)
                    p.lineTo (cx - s * 0.4f + (float) z * s * 0.2f, yy + (z % 2 ? -s * 0.12f : 0.0f));
            }
            break;
    }
    g.setColour (col);
    g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, s * 0.07f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// Anubis jackal head in profile (facing right), gold gradient, turquoise eye
static void drawAnubis (juce::Graphics& g, float x, float y, float h, float alpha, bool eye)
{
    const float w = h * 0.95f;
    auto P = [&] (float u, float v) { return juce::Point<float> (x + u * w, y + v * h); };
    juce::Path p;
    p.startNewSubPath (P (0.30f, 0.00f));           // ear tip
    p.lineTo (P (0.44f, 0.36f));                    // ear front
    p.quadraticTo (P (0.52f, 0.36f), P (0.60f, 0.42f));   // forehead
    p.lineTo (P (0.95f, 0.56f));                    // snout top
    p.quadraticTo (P (1.00f, 0.58f), P (0.98f, 0.63f));   // nose
    p.lineTo (P (0.70f, 0.66f));                    // jaw
    p.quadraticTo (P (0.58f, 0.70f), P (0.54f, 0.78f));   // throat
    p.lineTo (P (0.62f, 1.00f));                    // neck front
    p.lineTo (P (0.12f, 1.00f));                    // neck back
    p.quadraticTo (P (0.06f, 0.70f), P (0.16f, 0.50f));   // back of the head
    p.lineTo (P (0.22f, 0.34f));                    // ear back
    p.closeSubPath();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7a6).withAlpha (alpha), x, y,
                                             juce::Colour (0xff7a4c10).withAlpha (alpha), x + w, y + h, false));
    g.fillPath (p);
    g.setColour (juce::Colour (0xff2b1807).withAlpha (alpha * 0.8f));
    g.strokePath (p, juce::PathStrokeType (juce::jmax (0.8f, h * 0.012f)));

    // inner ear + collar lines
    juce::Path d;
    d.startNewSubPath (P (0.31f, 0.08f)); d.lineTo (P (0.38f, 0.33f));
    d.startNewSubPath (P (0.20f, 0.80f)); d.lineTo (P (0.24f, 0.98f));
    d.startNewSubPath (P (0.26f, 0.78f)); d.lineTo (P (0.30f, 0.98f));
    d.startNewSubPath (P (0.32f, 0.76f)); d.lineTo (P (0.36f, 0.98f));
    g.setColour (juce::Colour (0xff2b1807).withAlpha (alpha * 0.7f));
    g.strokePath (d, juce::PathStrokeType (juce::jmax (0.8f, h * 0.012f)));

    // kohl-lined eye
    juce::Path e;
    e.startNewSubPath (P (0.50f, 0.50f));
    e.quadraticTo (P (0.58f, 0.45f), P (0.68f, 0.50f));
    e.quadraticTo (P (0.58f, 0.53f), P (0.50f, 0.50f));
    g.setColour (juce::Colours::black.withAlpha (alpha));
    g.strokePath (e, juce::PathStrokeType (juce::jmax (1.0f, h * 0.02f)));
    if (eye)
    {
        const auto c = P (0.585f, 0.495f);
        g.setColour (neon::led.withAlpha (alpha));
        g.fillEllipse (c.x - h * 0.02f, c.y - h * 0.02f, h * 0.04f, h * 0.04f);
    }
}

// winged sun disk
static void drawWingedSun (juce::Graphics& g, float cx, float cy, float s)
{
    for (int sd = -1; sd <= 1; sd += 2)
    {
        juce::Path wing;
        wing.startNewSubPath (cx + (float) sd * s * 0.18f, cy - s * 0.05f);
        wing.quadraticTo (cx + (float) sd * s * 0.7f, cy - s * 0.35f, cx + (float) sd * s * 1.15f, cy - s * 0.12f);
        wing.quadraticTo (cx + (float) sd * s * 0.7f, cy - s * 0.05f, cx + (float) sd * s * 0.95f, cy + s * 0.08f);
        wing.quadraticTo (cx + (float) sd * s * 0.55f, cy + s * 0.06f, cx + (float) sd * s * 0.18f, cy + s * 0.12f);
        wing.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7a6), cx, cy - s * 0.3f, juce::Colour (0xff8a5b16), cx, cy + s * 0.2f, false));
        g.fillPath (wing);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff0c0), cx - s * 0.2f, cy - s * 0.25f, juce::Colour (0xffb7832a), cx + s * 0.2f, cy + s * 0.25f, false));
    g.fillEllipse (cx - s * 0.24f, cy - s * 0.24f, s * 0.48f, s * 0.48f);
    g.setColour (neon::gold.withAlpha (0.25f));
    g.drawEllipse (cx - s * 0.32f, cy - s * 0.32f, s * 0.64f, s * 0.64f, 2.0f);
}

static void drawLotus (juce::Graphics& g, float cx, float cy, float s)
{
    for (int k = -2; k <= 2; ++k)
    {
        juce::Path p;
        p.addEllipse (-s * 0.12f, -s * 0.5f, s * 0.24f, s * 0.5f);
        p.applyTransform (juce::AffineTransform::rotation ((float) k * 0.45f).translated (cx, cy));
        g.setColour (neon::gold.withAlpha (0.85f));
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
}

void NayAudioProcessorEditor::rebuildBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    bgImage = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (bgImage);

    // near-black charcoal + a faint warm vignette at the top
    g.fillAll (neon::bg0);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a2a10).withAlpha (0.55f), (float) w * 0.3f, -40.0f,
                                             juce::Colours::transparentBlack, (float) w * 0.3f, (float) h * 0.55f, true));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, (float) w * 0.5f, (float) h * 0.45f,
                                             juce::Colours::black.withAlpha (0.55f), 0.0f, (float) h, true));
    g.fillAll();

    // subtle stone grain
    juce::Random rnd (7);
    for (int i = 0; i < 2500; ++i)
    {
        g.setColour ((i & 1 ? neon::gold : juce::Colours::black).withAlpha (0.025f));
        g.fillRect (rnd.nextInt (w), rnd.nextInt (h), 1 + rnd.nextInt (2), 1);
    }

    // hieroglyph friezes along the top and bottom edges (subtle gold)
    const int fx = 30, hy = 70, fh = 36;
    for (int i = 0, x = 16; x < w - 8; x += 30, ++i)
        drawGlyph (g, i, (float) x, 6.0f, 9.0f, neon::gold.withAlpha (0.16f));
    g.setColour (neon::bronze.withAlpha (0.6f));
    g.fillRect (0, 13, w, 1);

    // sides: dark stone pillars with a gold edge and faint glyph columns
    for (int side = 0; side < 2; ++side)
    {
        juce::Rectangle<int> col (side == 0 ? 0 : w - fx, 14, fx, h - 14 - fh);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff120d07), (float) col.getX(), 0.0f,
                                                 juce::Colour (0xff070504), (float) col.getRight(), 0.0f, false));
        g.fillRect (col);
        g.setColour (neon::bronze);
        g.fillRect (side == 0 ? col.getRight() - 1 : col.getX(), col.getY(), 1, col.getHeight());
        const float cx = (float) col.getCentreX();
        for (int i = 0, y = 40; y < col.getBottom() - 12; y += 34, ++i)
            drawGlyph (g, i + side * 3, cx, (float) y, 16.0f, neon::gold.withAlpha (0.32f));
    }

    // footer frieze + "Developed in Egypt" plate
    {
        juce::Rectangle<int> foot (fx, h - fh, w - 2 * fx, fh);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff100b06), 0.0f, (float) foot.getY(), juce::Colour (0xff050302), 0.0f, (float) h, false));
        g.fillRect (foot);
        g.setColour (neon::bronze);
        g.fillRect (foot.getX(), foot.getY(), foot.getWidth(), 1);
        for (int i = 0, x = fx + 16; x < w - fx - 14; x += 32, ++i)
            if (x < w / 2 - 125 || x > w / 2 + 125)
                drawGlyph (g, i, (float) x, (float) h - (float) fh * 0.5f, 15.0f, neon::gold.withAlpha (0.3f));
        auto plate = juce::Rectangle<float> ((float) w * 0.5f - 112.0f, (float) h - (float) fh + 5.0f, 224.0f, (float) fh - 10.0f);
        g.setColour (juce::Colour (0xff070504));
        g.fillPath (chamfer (plate, 6.0f));
        g.setColour (neon::gold);
        g.strokePath (chamfer (plate, 6.0f), juce::PathStrokeType (1.2f));
        drawLotus (g, plate.getX() + 20.0f, plate.getBottom() - 3.0f, 18.0f);
        drawLotus (g, plate.getRight() - 20.0f, plate.getBottom() - 3.0f, 18.0f);
        g.setColour (neon::gold);
        g.setFont (uiFont (11.5f).withExtraKerningFactor (0.18f));
        g.drawText ("DEVELOPED IN EGYPT", plate.toNearestInt(), juce::Justification::centred, false);
    }

    // header separator: double gold line with diamonds
    g.setColour (neon::gold.withAlpha (0.8f));
    g.fillRect (fx, hy - 3, w - 2 * fx, 1);
    g.setColour (neon::bronze);
    g.fillRect (fx, hy - 1, w - 2 * fx, 1);
    drawDiamond (g, (float) fx + 4.0f, (float) hy - 2.0f, 4.0f, neon::gold);
    drawDiamond (g, (float) (w - fx) - 4.0f, (float) hy - 2.0f, 4.0f, neon::gold);
}

// panel: dark translucent glass, chamfered corners, double gold frame, corner diamonds, letter-spaced gold title
void NayAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& title, juce::Colour)
{
    auto r = ri.toFloat().reduced (1.0f);
    const float ch = 12.0f;

    g.setColour (neon::gold.withAlpha (0.07f));                       // soft glow
    g.strokePath (chamfer (r.expanded (2.5f), ch + 2.0f), juce::PathStrokeType (5.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17110a).withAlpha (0.82f), 0.0f, r.getY(),
                                             juce::Colour (0xff050403).withAlpha (0.82f), 0.0f, r.getBottom(), false));
    g.fillPath (chamfer (r, ch));

    g.setColour (neon::gold);                                         // outer gold frame
    g.strokePath (chamfer (r, ch), juce::PathStrokeType (1.6f));
    g.setColour (neon::bronze.brighter (0.25f));                      // inner bronze frame
    g.strokePath (chamfer (r.reduced (4.0f), ch - 3.0f), juce::PathStrokeType (1.0f));

    for (auto p : { r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight() })
        drawDiamond (g, p.x, p.y, 4.0f, neon::gold);

    // indicator dots, top right (turquoise); narrow mixer strips get one dot and tighter title spacing
    const bool narrow = ri.getWidth() < 160;
    if (! narrow)
        ledDot (g, juce::Rectangle<float> (r.getRight() - 30.0f, r.getY() + 11.0f, 5.0f, 5.0f), false);
    ledDot (g, juce::Rectangle<float> (r.getRight() - 20.0f, r.getY() + 11.0f, 5.0f, 5.0f), true);

    g.setFont (uiFont (narrow ? 10.5f : 11.5f).withExtraKerningFactor (narrow ? 0.12f : 0.42f));
    g.setColour (neon::gold);
    g.drawText (title, ri.getX() + (narrow ? 10 : 16), ri.getY() + 5, juce::jmax (40, ri.getWidth() - (narrow ? 34 : 60)), 16,
                juce::Justification::centredLeft, true);
}

// LCD: black glass, chamfered gold frame, glowing gold text, scanlines, turquoise status dot
void NayAudioProcessorEditor::drawLcd (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& tag,
                                       const juce::String& name, const juce::String& sub, juce::Colour accent)
{
    auto r = ri.toFloat().reduced (1.0f);
    const bool alert = accent == neon::magenta;
    g.setColour (juce::Colour (0xff020101));
    g.fillPath (chamfer (r, 8.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), 0.0f, r.getY(),
                                             juce::Colours::transparentWhite, 0.0f, r.getCentreY(), false));
    g.fillPath (chamfer (r.reduced (2.0f), 7.0f));
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (float yy = r.getY() + 3.0f; yy < r.getBottom() - 3.0f; yy += 3.0f)
        g.fillRect (r.getX() + 4.0f, yy, r.getWidth() - 8.0f, 1.0f);
    g.setColour (neon::gold.withAlpha (0.18f));
    g.strokePath (chamfer (r.expanded (1.5f), 9.0f), juce::PathStrokeType (3.0f));
    g.setColour (alert ? neon::magenta : neon::gold);
    g.strokePath (chamfer (r, 8.0f), juce::PathStrokeType (1.6f));
    g.setColour (neon::bronze);
    g.strokePath (chamfer (r.reduced (3.5f), 6.0f), juce::PathStrokeType (0.8f));

    ledDot (g, juce::Rectangle<float> (r.getX() + 32.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f), name != "OFF");

    auto inner = ri.reduced (40, 0);
    g.setColour (neon::gold.withAlpha (0.6f));
    g.setFont (uiFont (9.5f).withExtraKerningFactor (0.35f));
    g.drawText (tag, inner.getX(), ri.getY() + 5, inner.getWidth(), 12, juce::Justification::centred, false);

    const auto nf = uiFont (20.0f).withExtraKerningFactor (0.14f);
    g.setFont (nf);
    const auto txtCol = name == "OFF" ? neon::dim : juce::Colour (0xfff7d77e);
    for (int d = 2; d >= 1; --d)
    {   // glow
        g.setColour (txtCol.withAlpha (0.08f));
        g.drawText (name.toUpperCase(), inner.withTrimmedTop (14).withTrimmedBottom (14).expanded (d, d), juce::Justification::centred, true);
    }
    g.setColour (txtCol);
    g.drawText (name.toUpperCase(), inner.withTrimmedTop (14).withTrimmedBottom (14), juce::Justification::centred, true);

    g.setColour (alert ? juce::Colour (0xffff9a80) : neon::dim);
    g.setFont (uiFont (10.0f, false));
    g.drawText (sub, inner.getX(), ri.getBottom() - 16, inner.getWidth(), 12, juce::Justification::centred, true);
}

void NayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (bgImage, 0, 0);


    // ---- MIDI monitor strip ----
    {
        int k, a, b, cnt;
        proc.getMidiMonitor (k, a, b, cnt);

        juce::String t = "waiting for MIDI...";
        if (touchMs != 0 && touchMs >= lastMidiMs)
            t = touchText;
        else if (k == 1 || k == 2)
        {
            const juce::String nn = juce::String (kNames[a % 12]) + juce::String (a / 12);   // C5 = middle C (60)
            t = k == 1 ? "Note on: " + nn + " / velocity " + juce::String (b) : "Note off: " + nn;
        }
        else if (k == 3)
        {
            juce::String nm = a == 1 ? "Mod Wheel" : a == 7 ? "Volume" : a == 11 ? "Expression" : a == 64 ? "Sustain" : "CC " + juce::String (a);
            t = nm + " (CC " + juce::String (a) + "): " + juce::String (b);
        }
        else if (k == 4)
            t = "Pitch bend: " + juce::String (((float) a - 8192.0f) / 8192.0f, 2);

        {
            auto nameOf = [this] (int t) { return t == -2 ? juce::String ("Expression pedal") : proc.getParamLabel (t); };
            int dcc, dtgt, dcnt;
            proc.getLearnDone (dcc, dtgt, dcnt);
            if (proc.getLearnTarget() != -1)
                t = "MIDI LEARN:  move a knob / pedal on your MIDI device for  [ " + nameOf (proc.getLearnTarget()) + " ]   (right-click it again to cancel)";
            else if (juce::Time::getMillisecondCounter() - lastLearnMs < 3000 && lastLearnCount > 0)
                t = "Assigned  CC " + juce::String (dcc) + "  ->  " + nameOf (dtgt);
        }

        auto r = midiRect.toFloat();
        g.setColour (juce::Colour (0xff030201));
        g.fillPath (chamfer (r, 6.0f));
        g.setColour (neon::gold.withAlpha (0.6f));
        g.strokePath (chamfer (r.reduced (0.5f), 6.0f), juce::PathStrokeType (1.0f));

        const float age = (float) (juce::Time::getMillisecondCounter() - lastMonMs);
        const float act = juce::jlimit (0.0f, 1.0f, 1.0f - age / 400.0f);
        g.setColour (neon::led.withAlpha (0.2f * act));
        g.fillEllipse (r.getX() + 7.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f);
        g.setColour (neon::led.withAlpha (0.25f + 0.75f * act));
        g.fillEllipse (r.getX() + 10.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);

        g.setColour (k == 0 ? neon::dim : neon::text);
        g.setFont (uiFont (12.5f, false));
        g.drawText (t, midiRect.withTrimmedLeft (28), juce::Justification::centredLeft, false);
    }

    // monumental title: Anubis head + winged sun + letter-spaced gold
    drawAnubis (g, 38.0f, 14.0f, 52.0f, 1.0f, true);
    drawWingedSun (g, 122.0f, 30.0f, 26.0f);
    auto f = juce::Font (juce::FontOptions ("Segoe UI", 27.0f, juce::Font::bold)).withExtraKerningFactor (0.42f);
    g.setFont (f);
    for (int d = 3; d >= 1; --d)
    {
        g.setColour (neon::gold.withAlpha (0.06f));
        g.drawText ("EGYPTIAN", 158 - d, 12 - d, 330 + 2 * d, 32 + 2 * d, juce::Justification::centredLeft, false);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbe7a6), 0.0f, 14.0f, juce::Colour (0xffb7832a), 0.0f, 42.0f, false));
    g.drawText ("EGYPTIAN", 158, 12, 330, 32, juce::Justification::centredLeft, false);

    g.setFont (uiFont (11.0f).withExtraKerningFactor (1.1f));
    g.setColour (neon::gold.withAlpha (0.85f));
    g.drawText ("SAMPLER", 160, 44, 330, 14, juce::Justification::centredLeft, false);
}

void NayAudioProcessorEditor::resized()
{
    if (! built)
        return;

    rebuildBackground();

    mapButton.setBounds (872, 16, 142, 32);
    browseButton.setBounds (640, 16, 110, 32);
    mixerButton.setBounds (756, 16, 110, 32);
    voiceButton.setBounds (524, 16, 110, 32);
    midiRect = { 46, 674, 968, 24 };
    pageRect = { 46, 76, 968, 592 };
    page.setBounds (pageRect);
    settings.setBounds (pageRect);
    browse.setBounds (pageRect);
    voiceEd.setBounds (pageRect);
    vePorta = { 0, 0, 968, 132 };
    veVel   = { 0, 140, 968, 132 };
    veLimit = { 0, 280, 476, 132 };
    veTouch = { 492, 280, 476, 132 };
    veSound = { 0, 420, 968, 160 };
    tPorta->setBounds (vePorta.getX() + 16, vePorta.getY() + 50, 160, 26);
    layoutRow ({ 14, veSound.getY() + 26, 940, 110 }, { vk[10], vk[11], vk[12], vk[13], vk[14], vk[15], vk[16] });
    layoutRow ({ 200, vePorta.getY() + 26, 600, 84 }, { vk[0], vk[1], vk[2] });
    layoutRow ({ 200, veVel.getY() + 26, 600, 84 }, { vk[3], vk[4], vk[5] });
    layoutRow ({ veLimit.getX() + 60, veLimit.getY() + 26, 360, 84 }, { vk[6], vk[7] });
    layoutRow ({ veTouch.getX() + 60, veTouch.getY() + 26, 360, 84 }, { vk[8], vk[9] });
    mixer.setBounds (pageRect);

    // ================= main page (local coords) =================
    lcdA = { 0, 0, 478, 60 };      // instrument name (left) + DUO screen (right), top of the page
    lcdD = { 490, 0, 478, 60 };
    prevA.setBounds (lcdA.getX() + 8, lcdA.getCentreY() - 14, 20, 28);
    nextA.setBounds (lcdA.getRight() - 28, lcdA.getCentreY() - 14, 20, 28);
    prevD.setBounds (lcdD.getX() + 8, lcdD.getCentreY() - 14, 20, 28);
    nextD.setBounds (lcdD.getRight() - 28, lcdD.getCentreY() - 14, 20, 28);

    pVoice = { 0, 68, 968, 128 };
    layoutRow ({ 10, pVoice.getY() + 22, 600, 100 }, { kGain, kAtk, kRel, kVibD, kVibR, kGlide });
    tMono->setBounds (748, pVoice.getY() + 20, 100, 24);
    tLegato->setBounds (856, pVoice.getY() + 20, 104, 24);
    tVel->setBounds (748, pVoice.getY() + 46, 212, 24);
    tMagic->setBounds (748, pVoice.getY() + 76, 212, 34);

    pMaqam = { 0, 204, 968, 150 };
    trBox.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 46, 108, 34);
    ocBox.setBounds (pMaqam.getX() + 130, pMaqam.getY() + 46, 108, 34);
    resetKeysBtn.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 100, 224, 36);
    static const float px[12] = { 0, 0.5f, 1, 1.5f, 2, 3, 3.5f, 4, 4.5f, 5, 5.5f, 6 };
    maqamBox.setBounds (pMaqam.getX() + 776, pMaqam.getY() + 28, 184, 28);
    memStoreBtn.setBounds (pMaqam.getX() + 776, pMaqam.getY() + 64, 60, 28);
    for (int i = 0; i < 5; ++i)
        memBtn[i].setBounds (pMaqam.getX() + 840 + i * 24, pMaqam.getY() + 64, 22, 28);
    tBypass->setBounds (pMaqam.getX() + 776, pMaqam.getY() + 100, 184, 26);
    for (int i = 0; i < 12; ++i)
    {
        const bool sharp = i == 1 || i == 3 || i == 6 || i == 8 || i == 10;
        keys[(size_t) i]->setBounds (250 + (int) (px[i] * 72.0f), pMaqam.getY() + (sharp ? 20 : 84), 62, 62);
    }

    // ================= settings page (local coords) =================
    lcdA2 = { 0, 0, 420, 60 };
    prevA2.setBounds (lcdA2.getX() + 8, 16, 20, 28);
    nextA2.setBounds (lcdA2.getRight() - 28, 16, 20, 28);
    newInstBtn.setBounds (430, 0, 150, 28);
    saveInstBtn.setBounds (430, 32, 150, 28);
    addButton.setBounds (588, 0, 124, 28);
    renameButton.setBounds (588, 32, 124, 28);
    tVar->setBounds (720, 0, 120, 26);
    info.setBounds (720, 28, 248, 34);

    mapView.setBounds (0, 68, 968, 524);
    waveEd.setBounds (0, 68, 968, 524);

    pPerf = { 0, 362, 968, 132 };
    tOscOn->setBounds (14, pPerf.getY() + 30, 150, 28);
    tOscOnly->setBounds (14, pPerf.getY() + 66, 150, 26);
    oscWaveBox.setBounds (176, pPerf.getY() + 46, 150, 28);
    layoutRow ({ 520, pPerf.getY() + 22, 440, 104 }, { kOscLvl, kOscTune, kOscFine, kOscCut });
    pArp = { 0, 502, 560, 90 };           // smaller
    pWheels = { 568, 502, 400, 90 };      // bigger: wheels + bend range + transpose + octave
    pitchWheel.setBounds (pWheels.getX() + 8, pWheels.getY() + 36, 28, 50);
    modWheelView.setBounds (pWheels.getX() + 40, pWheels.getY() + 36, 28, 50);
    expWheel.setBounds (pWheels.getX() + 72, pWheels.getY() + 36, 28, 50);
    pbBox.setBounds (pWheels.getX() + 108, pWheels.getY() + 40, 88, 34);
    tArp->setBounds (12, pArp.getY() + 28, 110, 26);
    tArpHold->setBounds (12, pArp.getY() + 58, 110, 26);
    arpModeBox.setBounds (128, pArp.getY() + 38, 140, 28);
    arpRateBox.setBounds (276, pArp.getY() + 38, 100, 28);
    layoutRow ({ 384, pArp.getY() + 20, 168, 66 }, { kArpOct, kArpGate });

    // ================= browse page =================
    searchBox.setBounds (0, 0, 300, 24);
    catList.setBounds (0, 38, 300, 520);
    instList.setBounds (312, 38, 430, 520);
    loadBtn.setBounds (754, 38, 214, 34);
    duoBtn.setBounds (754, 78, 214, 34);
    noDuoBtn.setBounds (754, 118, 214, 30);
    saveBtn.setBounds (754, 196, 214, 34);
    renPresetBtn.setBounds (754, 236, 214, 30);
    delPresetBtn.setBounds (754, 272, 214, 30);
    browseInfo.setBounds (754, 330, 214, 228);

    // ================= mixer page: 9 channel strips =================
    const int sw = 104, sg = 4;
    for (int i = 0; i < 9; ++i)
        mixStrip[i] = { i * (sw + sg), 0, sw, 572 };

    auto placeFader = [&] (Knob* k, int i)
    {
        const int fxp = i == 8 ? mixStrip[i].getX() + 4 : mixStrip[i].getX() + (sw - 76) / 2;
        k->slider.setBounds (fxp, 68, i == 8 ? 72 : 76, 292);
    };
    auto placeKnob = [&] (Knob* k, int i, int slot)
    {
        layoutRow ({ mixStrip[i].getX() + 14, 366 + slot * 92, sw - 28, 90 }, { k });
    };
    auto placeTog = [&] (juce::ToggleButton* t, int i) { t->setBounds (mixStrip[i].getX() + 8, 40, sw - 16, 22); };

    placeTog (tMainMute, 0); placeFader (kMainLvl, 0); placeKnob (kMainPan, 0, 0);
    placeTog (tDuoMute, 1);  placeFader (kDuoLvl, 1);  placeKnob (kDuoPan, 1, 0);
    placeTog (tOscSoloMix, 2); placeFader (kOscFader, 2); placeKnob (kOscPan, 2, 0);
    tOscMuteMix->setBounds (mixStrip[2].getX() + 8, 466, sw - 16, 24);
    tOscOnMix->setBounds (mixStrip[2].getX() + 8, 498, sw - 16, 24);
    placeTog (tSat, 3);      placeFader (kSatMix, 3);
    layoutRow ({ mixStrip[3].getX() + 14, 358, sw - 28, 68 }, { kSatDrive });
    layoutRow ({ mixStrip[3].getX() + 14, 428, sw - 28, 68 }, { kSatTone });
    layoutRow ({ mixStrip[3].getX() + 14, 498, sw - 28, 68 }, { kSatBass });
    placeTog (tWah, 4);      placeFader (kWahMix, 4);  placeKnob (kWahPos, 4, 0);
    placeTog (tPh, 5);       placeFader (kPhMix, 5);   placeKnob (kPhRate, 5, 0); placeKnob (kPhDepth, 5, 1);
    placeFader (kDlLvl, 6);  placeKnob (kDlTime, 6, 0); placeKnob (kDlFb, 6, 1);
    placeFader (kRvLvl, 7);  placeKnob (kRvSize, 7, 0);
    placeFader (kMaster, 8);
}

//==============================================================================
// WAVE EDITOR
//==============================================================================
WaveEditor::WaveEditor (NayAudioProcessor& p) : proc (p)
{
    listModel.getCount = [this]
    {
        auto b = proc.getBankOf (proc.getActiveGroup());
        return b != nullptr ? (int) b->size() : 0;
    };
    listModel.getText = [this] (int r) -> juce::String
    {
        auto b = proc.getBankOf (proc.getActiveGroup());
        if (b == nullptr || r < 0 || r >= (int) b->size())
            return {};
        const auto& s = *(*b)[(size_t) r];
        return noteName (s.root.load()) + "   " + s.fileName;
    };
    listModel.onSelect = [this] (int r) { if (r != idx) selectRow (r); };
    list.setModel (&listModel);
    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, neon::panel.withAlpha (0.9f));
    list.setColour (juce::ListBox::outlineColourId, neon::cyan.withAlpha (0.4f));
    list.setOutlineThickness (1);
    addAndMakeVisible (list);

    auto setup = [this] (juce::TextButton& b, juce::Colour c, std::function<void()> fn)
    {
        b.setColour (juce::TextButton::buttonColourId, c);
        b.onClick = std::move (fn);
        addAndMakeVisible (b);
    };
    setup (backBtn, neon::amber, [this] { proc.stopPreview(); playBtn.setButtonText ("PLAY"); if (onClose) onClose(); });
    setup (playBtn, neon::green, [this]
    {
        if (proc.isPreviewing()) { proc.stopPreview(); playBtn.setButtonText ("PLAY"); }
        else if (auto s = current()) { proc.startPreview (s); playBtn.setButtonText ("STOP"); }
    });
    setup (undoBtn, neon::violet, [this]
    {
        if (undo.empty()) return;
        auto u = undo.back();
        undo.pop_back();
        proc.replaceSample (group, u.first, u.second);
        idx = u.first;
        selA = selB = -1;
        if (auto s = current()) setView (viewStart, juce::jmin (viewLen, (double) s->data.getNumSamples()));
        edited ("Undo");
    });
    setup (saveBtn, neon::green, [this]
    {
        const bool ok = proc.saveInstrument (group);
        if (onChanged) onChanged (ok ? "Saved:  " + proc.getGroupPath (group) : "SAVE FAILED");
        repaint();
    });
    setup (zoomSelBtn, neon::cyan, [this]
    {
        if (selA >= 0 && selB != selA)
            setView (juce::jmin (selA, selB), std::abs (selB - selA));
    });
    setup (zoomAllBtn, neon::cyan, [this] { if (auto s = current()) setView (0, s->data.getNumSamples()); });

    setup (delBtn,  neon::magenta, [this] { doOp (WaveOp::Delete); });
    setup (trimBtn, neon::magenta, [this] { doOp (WaveOp::Trim); });
    setup (silBtn,  neon::magenta, [this] { doOp (WaveOp::Silence); });
    setup (finBtn,  neon::amber,   [this] { doOp (WaveOp::FadeIn); });
    setup (foutBtn, neon::amber,   [this] { doOp (WaveOp::FadeOut); });
    setup (normBtn, neon::amber,   [this] { doOp (WaveOp::Normalize); });
    setup (revBtn,  neon::amber,   [this] { doOp (WaveOp::Reverse); });

    setup (loopSelBtn, neon::green, [this]
    {
        auto s = current();
        if (s == nullptr || selA < 0 || std::abs (selB - selA) < 64)
        {
            if (onChanged) onChanged ("Select a part of the wave first (drag on it), then LOOP = SELECTION");
            return;
        }
        pushUndo();
        const int a = snapZero (*s, juce::jmin (selA, selB)), b = snapZero (*s, juce::jmax (selA, selB));
        if (b - a < 64) return;
        s->loopStart = a;
        s->loopEnd = b - 1;
        const int xf = juce::jmin ((int) (s->sampleRate * 0.1), (b - a) / 4, a);
        s->xfade = juce::jmax (0, xf);
        edited ("Loop set from the selection (crossfade " + juce::String ((int) (s->xfade.load() * 1000.0 / s->sampleRate)) + " ms)");
    });
    setup (autoLoopBtn, neon::green, [this]
    {
        auto s = current();
        if (s == nullptr) return;
        pushUndo();
        const int before = s->loopStart.load(), beforeE = s->loopEnd.load();
        NayAudioProcessor::autoLoop (*s);
        const bool changed = s->loopStart.load() != before || s->loopEnd.load() != beforeE;
        edited (changed ? "AUTO LOOP: loop + crossfade set in the sustain - press PLAY to hear it"
                        : "AUTO LOOP: no better loop found (sample too short?)");
    });
    setup (clearLoopBtn, neon::dim, [this]
    {
        auto s = current();
        if (s == nullptr) return;
        pushUndo();
        s->loopStart = -1; s->loopEnd = -1; s->xfade = -1;
        edited ("Loop cleared (the engine loops the 2nd half)");
    });

    xfSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    xfSlider.setRange (0.0, 300.0, 1.0);
    xfSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    xfSlider.setTextValueSuffix (" ms");
    xfSlider.setColour (juce::Slider::thumbColourId, neon::green);
    xfSlider.setColour (juce::Slider::trackColourId, neon::green.withAlpha (0.5f));
    xfSlider.setColour (juce::Slider::textBoxTextColourId, neon::text);
    xfSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    xfSlider.onDragStart = [this] { pushUndo(); };
    xfSlider.onValueChange = [this]
    {
        if (auto s = current())
        {
            s->xfade = (int) (xfSlider.getValue() * 0.001 * s->sampleRate);
            proc.markDirty (group);
            repaint();
        }
    };
    addAndMakeVisible (xfSlider);
    xfLabel.setText ("LOOP CROSSFADE", juce::dontSendNotification);
    xfLabel.setFont (uiFont (11.0f));
    xfLabel.setColour (juce::Label::textColourId, neon::dim);
    addAndMakeVisible (xfLabel);

    hbar.setAutoHide (false);
    hbar.addListener (this);
    hbar.setColour (juce::ScrollBar::thumbColourId, neon::cyan.withAlpha (0.6f));
    addAndMakeVisible (hbar);
}

WaveEditor::~WaveEditor()
{
    hbar.removeListener (this);
    list.setModel (nullptr);
}

std::shared_ptr<NaySample> WaveEditor::current() const
{
    auto b = proc.getBankOf (group);
    if (b == nullptr || idx < 0 || idx >= (int) b->size())
        return nullptr;
    return (*b)[(size_t) idx];
}

void WaveEditor::open (int sampleIndex)
{
    if (group != proc.getActiveGroup())
        undo.clear();
    group = proc.getActiveGroup();
    list.updateContent();
    idx = -1;
    selectRow (juce::jmax (0, sampleIndex));
}

void WaveEditor::selectRow (int i)
{
    idx = i;
    selA = selB = -1;
    list.selectRow (i, false, true);
    if (auto s = current())
        setView (0, s->data.getNumSamples());
    if (proc.isPreviewing())
    {
        if (auto s = current()) proc.startPreview (s);
    }
    updateControls();
    repaint();
}

void WaveEditor::updateControls()
{
    auto s = current();
    const int xs = s != nullptr ? s->xfade.load() : -1;
    xfSlider.setValue (s != nullptr && xs >= 0 ? xs * 1000.0 / s->sampleRate : 0.0, juce::dontSendNotification);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &playBtn, &delBtn, &trimBtn, &silBtn, &finBtn, &foutBtn, &normBtn, &revBtn,
                                                                         &loopSelBtn, &autoLoopBtn, &clearLoopBtn, &xfSlider, &zoomSelBtn, &zoomAllBtn })
        c->setEnabled (s != nullptr);
    undoBtn.setEnabled (! undo.empty());
    saveBtn.setEnabled (group >= 0);
}

void WaveEditor::pushUndo()
{
    if (auto s = current())
    {
        undo.push_back ({ idx, NayAudioProcessor::cloneSample (*s) });
        if (undo.size() > 40)
            undo.erase (undo.begin());
    }
}

void WaveEditor::edited (const juce::String& msg)
{
    proc.markDirty (group);
    list.updateContent();
    list.repaint();
    updateControls();
    cFor = nullptr;
    repaint();
    if (onChanged) onChanged (msg);
}

void WaveEditor::doOp (WaveOp op)
{
    auto s = current();
    if (s == nullptr)
        return;
    int a = juce::jmin (selA, selB), b = juce::jmax (selA, selB);
    const bool hasSel = selA >= 0 && b - a >= 2;
    if (! hasSel && op != WaveOp::Normalize && op != WaveOp::Reverse)
    {
        if (onChanged) onChanged ("Drag on the wave to select a part first");
        return;
    }
    if (! hasSel) { a = 0; b = s->data.getNumSamples(); }

    auto ns = NayAudioProcessor::applyWaveOp (*s, op, a, b);
    if (ns == nullptr)
    {
        if (onChanged) onChanged ("Not possible on this selection");
        return;
    }
    undo.push_back ({ idx, s });
    if (undo.size() > 40) undo.erase (undo.begin());
    proc.replaceSample (group, idx, ns);
    if (proc.isPreviewing()) proc.startPreview (ns);

    const int len = ns->data.getNumSamples();
    if (op == WaveOp::Delete)    { selA = selB = juce::jmin (a, len - 1); }
    else if (op == WaveOp::Trim) { selA = selB = -1; setView (0, len); }
    setView (viewStart, viewLen);

    static const char* names[] = { "Deleted the selection", "Trimmed to the selection", "Fade in", "Fade out", "Normalized", "Silenced", "Reversed" };
    edited (juce::String (names[(int) op]) + "  -  UNDO to go back, SAVE to keep");
}

void WaveEditor::setView (double start, double len)
{
    auto s = current();
    const double total = s != nullptr ? (double) s->data.getNumSamples() : 1.0;
    viewLen = juce::jlimit (juce::jmin (64.0, total), total, len);
    viewStart = juce::jlimit (0.0, juce::jmax (0.0, total - viewLen), start);
    hbar.setRangeLimits (0.0, total, juce::dontSendNotification);
    hbar.setCurrentRange (viewStart, viewLen, juce::dontSendNotification);
    repaint();
}

void WaveEditor::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    viewStart = newRangeStart;
    repaint();
}

int WaveEditor::xToPos (float x) const
{
    auto s = current();
    const int len = s != nullptr ? s->data.getNumSamples() : 1;
    const double p = viewStart + (double) (x - (float) waveArea.getX()) / waveArea.getWidth() * viewLen;
    return juce::jlimit (0, len, (int) std::lround (p));
}

float WaveEditor::posToX (double p) const
{
    return (float) waveArea.getX() + (float) ((p - viewStart) / viewLen * waveArea.getWidth());
}

void WaveEditor::loopBounds (const NaySample& s, int& ls, int& le) const
{
    const int len = s.data.getNumSamples();
    ls = s.loopStart.load(); le = s.loopEnd.load();
    if (! (ls >= 0 && le > ls + 8 && le < len)) { ls = len / 2; le = len - 2; }
}

int WaveEditor::snapZero (const NaySample& s, int pos) const
{
    const int len = s.data.getNumSamples();
    const float* x = s.data.getReadPointer (0);
    const int range = juce::jmax (32, (int) (viewLen / juce::jmax (1, waveArea.getWidth()) * 6.0));
    for (int d = 0; d < range; ++d)
        for (int c : { pos - d, pos + d })
            if (c > 0 && c < len - 1 && x[c - 1] <= 0.0f && x[c] > 0.0f)
                return c;
    return juce::jlimit (0, juce::jmax (0, len - 1), pos);
}

void WaveEditor::resized()
{
    auto r = getLocalBounds();
    list.setBounds (r.removeFromLeft (210).withTrimmedBottom (0));
    r.removeFromLeft (10);

    auto row = [&r] (int h) { auto a = r.removeFromTop (h); r.removeFromTop (6); return a; };
    auto place = [] (juce::Rectangle<int> a, std::initializer_list<juce::Component*> cs, int gap = 6)
    {
        const int n = (int) cs.size();
        const int w = (a.getWidth() - gap * (n - 1)) / n;
        int i = 0;
        for (auto* c : cs) c->setBounds (a.getX() + i++ * (w + gap), a.getY(), w, a.getHeight());
    };
    place (row (28), { &backBtn, &playBtn, &undoBtn, &saveBtn, &zoomSelBtn, &zoomAllBtn });
    place (row (28), { &delBtn, &trimBtn, &silBtn, &finBtn, &foutBtn, &normBtn, &revBtn });
    {
        auto a = row (28);
        loopSelBtn.setBounds (a.removeFromLeft (150));   a.removeFromLeft (6);
        autoLoopBtn.setBounds (a.removeFromLeft (110));  a.removeFromLeft (6);
        clearLoopBtn.setBounds (a.removeFromLeft (150)); a.removeFromLeft (12);
        xfLabel.setBounds (a.removeFromLeft (110));
        xfSlider.setBounds (a);
    }
    statusArea = r.removeFromBottom (18);
    hbar.setBounds (r.removeFromBottom (14));
    r.removeFromBottom (4);
    waveArea = r;
    cFor = nullptr;
}

void WaveEditor::paint (juce::Graphics& g)
{
    g.setColour (neon::bg0.withAlpha (0.96f));
    g.fillRect (getLocalBounds());

    g.setColour (juce::Colour (0xff030201));
    g.fillRect (waveArea);
    g.setColour (neon::cyan.withAlpha (0.45f));
    g.drawRect (waveArea, 1);

    auto s = current();
    if (s == nullptr)
    {
        g.setColour (neon::dim);
        g.setFont (uiFont (15.0f));
        g.drawText ("No sample - drop WAV files on the mapping first", waveArea, juce::Justification::centred, false);
        return;
    }

    const int len = s->data.getNumSamples(), nc = s->data.getNumChannels();
    const int w = waveArea.getWidth();

    // selection + loop shading
    int ls, le;
    loopBounds (*s, ls, le);
    const bool ownLoop = s->loopStart.load() >= 0;
    {
        const float x0 = posToX (ls), x1 = posToX (le + 1);
        g.setColour (neon::green.withAlpha (ownLoop ? 0.13f : 0.06f));
        g.fillRect (juce::Rectangle<float> (x0, (float) waveArea.getY(), x1 - x0, (float) waveArea.getHeight()).getIntersection (waveArea.toFloat()));
        const int xs = s->xfade.load();
        if (ownLoop && xs > 0)
        {   // crossfade zone (before the loop end)
            const float xa = posToX (le + 1 - xs);
            g.setColour (neon::amber.withAlpha (0.16f));
            g.fillRect (juce::Rectangle<float> (xa, (float) waveArea.getY(), x1 - xa, (float) waveArea.getHeight()).getIntersection (waveArea.toFloat()));
        }
    }
    if (selA >= 0 && selB != selA)
    {
        const float x0 = posToX (juce::jmin (selA, selB)), x1 = posToX (juce::jmax (selA, selB));
        g.setColour (neon::violet.withAlpha (0.35f));
        g.fillRect (juce::Rectangle<float> (x0, (float) waveArea.getY(), x1 - x0, (float) waveArea.getHeight()).getIntersection (waveArea.toFloat()));
    }

    // waveform (one lane per channel), cached min / max per pixel
    if (cFor != s.get() || cStart != viewStart || cLen != viewLen || cW != w)
    {
        cFor = s.get(); cStart = viewStart; cLen = viewLen; cW = w;
        cMin.assign ((size_t) (w * nc), 0.0f);
        cMax.assign ((size_t) (w * nc), 0.0f);
        for (int ch = 0; ch < nc; ++ch)
        {
            const float* x = s->data.getReadPointer (ch);
            for (int px = 0; px < w; ++px)
            {
                const int a = juce::jlimit (0, len, (int) (viewStart + (double) px / w * viewLen));
                const int b = juce::jlimit (a + 1, juce::jmax (a + 1, len), (int) (viewStart + (double) (px + 1) / w * viewLen));
                float mn = 1.0f, mx = -1.0f;
                for (int i = a; i < b && i < len; ++i) { mn = juce::jmin (mn, x[i]); mx = juce::jmax (mx, x[i]); }
                if (mx < mn) mn = mx = 0.0f;
                cMin[(size_t) (ch * w + px)] = mn;
                cMax[(size_t) (ch * w + px)] = mx;
            }
        }
    }
    const float laneH = (float) waveArea.getHeight() / (float) nc;
    for (int ch = 0; ch < nc; ++ch)
    {
        const float cy = (float) waveArea.getY() + laneH * ((float) ch + 0.5f), hh = laneH * 0.46f;
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRect ((float) waveArea.getX(), cy, (float) w, 1.0f);
        g.setColour (neon::cyan.withAlpha (0.9f));
        const bool dots = viewLen < w * 0.5;          // zoomed in a lot: draw a line
        if (dots)
        {
            juce::Path p;
            const float* x = s->data.getReadPointer (ch);
            for (int i = (int) viewStart; i <= juce::jmin (len - 1, (int) (viewStart + viewLen) + 1); ++i)
            {
                const float px = posToX (i), py = cy - x[i] * hh;
                if (i == (int) viewStart) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            g.strokePath (p, juce::PathStrokeType (1.4f));
        }
        else
            for (int px = 0; px < w; ++px)
            {
                const float mn = cMin[(size_t) (ch * w + px)], mx = cMax[(size_t) (ch * w + px)];
                g.fillRect ((float) (waveArea.getX() + px), cy - mx * hh, 1.0f, juce::jmax (1.0f, (mx - mn) * hh));
            }
    }

    // loop markers
    for (int k = 0; k < 2; ++k)
    {
        const float mx = posToX (k == 0 ? ls : le + 1);
        if (mx < (float) waveArea.getX() - 1.0f || mx > (float) waveArea.getRight() + 1.0f)
            continue;
        g.setColour (ownLoop ? neon::green : neon::dim);
        g.fillRect (mx - 1.0f, (float) waveArea.getY(), 2.0f, (float) waveArea.getHeight());
        juce::Path tri;
        tri.addTriangle (mx - 7.0f, (float) waveArea.getY(), mx + 7.0f, (float) waveArea.getY(), mx, (float) waveArea.getY() + 11.0f);
        g.fillPath (tri);
        g.setFont (uiFont (10.0f));
        g.drawText (k == 0 ? "LOOP START" : "LOOP END", (int) mx + (k == 0 ? 4 : -74), waveArea.getY() + 2, 70, 12,
                    k == 0 ? juce::Justification::centredLeft : juce::Justification::centredRight, false);
    }

    // status line
    auto secs = [&] (double n) { return juce::String (n / s->sampleRate, 3) + " s"; };
    juce::String st = s->fileName + "   |   " + noteName (s->root.load()) + "   |   " + secs (len)
                    + (nc > 1 ? "  stereo" : "  mono") + "   |   loop " + (ownLoop ? secs (ls) + " - " + secs (le + 1) : juce::String ("default"));
    if (selA >= 0 && selB != selA)
        st << "   |   selection " << secs (juce::jmin (selA, selB)) << " - " << secs (juce::jmax (selA, selB))
           << "  (" << secs (std::abs (selB - selA)) << ")";
    if (proc.isDirty (group))
        st << "   |   * UNSAVED";
    g.setColour (neon::text.withAlpha (0.85f));
    g.setFont (uiFont (11.0f, false));
    g.drawText (st, statusArea, juce::Justification::centredLeft, true);

    g.setColour (neon::dim);
    g.setFont (uiFont (10.0f, false));
    g.drawText ("drag = select   |   wheel = zoom   |   shift+wheel = scroll   |   drag the green markers = loop   |   double-click = select all",
                waveArea.reduced (8, 4), juce::Justification::bottomLeft, false);
}

void WaveEditor::mouseMove (const juce::MouseEvent& e)
{
    auto s = current();
    if (s == nullptr || ! waveArea.contains (e.getPosition())) { setMouseCursor (juce::MouseCursor::NormalCursor); return; }
    int ls, le;
    loopBounds (*s, ls, le);
    const bool near = std::abs (e.position.x - posToX (ls)) < 7.0f || std::abs (e.position.x - posToX (le + 1)) < 7.0f;
    setMouseCursor (near ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::IBeamCursor);
}

void WaveEditor::mouseDown (const juce::MouseEvent& e)
{
    dragMode = 0;
    auto s = current();
    if (s == nullptr || ! waveArea.contains (e.getPosition()))
        return;

    int ls, le;
    loopBounds (*s, ls, le);
    const float ds = std::abs (e.position.x - posToX (ls)), de = std::abs (e.position.x - posToX (le + 1));
    if (ds < 7.0f || de < 7.0f)
    {
        pushUndo();
        dragMode = ds <= de ? 2 : 3;
        s->loopStart = ls;
        s->loopEnd = le;
        if (s->xfade.load() < 0) s->xfade = juce::jmin ((int) (s->sampleRate * 0.05), ls, (le - ls) / 4);
        return;
    }
    dragMode = 1;
    selA = selB = xToPos (e.position.x);
    repaint();
}

void WaveEditor::mouseDrag (const juce::MouseEvent& e)
{
    auto s = current();
    if (s == nullptr)
        return;
    const int len = s->data.getNumSamples();

    if (dragMode == 1)
    {
        selB = xToPos (juce::jlimit ((float) waveArea.getX(), (float) waveArea.getRight(), e.position.x));
        repaint();
    }
    else if (dragMode == 2 || dragMode == 3)
    {
        const int pos = snapZero (*s, xToPos (e.position.x));
        if (dragMode == 2) s->loopStart = juce::jlimit (0, s->loopEnd.load() - 64, pos);
        else               s->loopEnd = juce::jlimit (s->loopStart.load() + 64, len - 2, pos - 1);
        s->xfade = juce::jmin (s->xfade.load(), s->loopStart.load(), (s->loopEnd.load() - s->loopStart.load()) / 2);
        proc.markDirty (group);
        repaint();
    }
}

void WaveEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragMode == 2 || dragMode == 3)
        edited ("Loop moved - press PLAY to hear it");
    dragMode = 0;
}

void WaveEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (auto s = current())
        if (waveArea.contains (e.getPosition()))
        {
            selA = 0;
            selB = s->data.getNumSamples();
            repaint();
        }
}

void WaveEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (current() == nullptr || ! waveArea.contains (e.getPosition()))
        return;
    if (e.mods.isShiftDown() || std::abs (w.deltaX) > std::abs (w.deltaY))
    {
        const float d = std::abs (w.deltaX) > std::abs (w.deltaY) ? w.deltaX : w.deltaY;
        setView (viewStart - (double) d * viewLen * 0.5, viewLen);
        return;
    }
    const double anchor = viewStart + (double) (e.position.x - (float) waveArea.getX()) / waveArea.getWidth() * viewLen;
    const double f = w.deltaY > 0 ? 0.7 : 1.0 / 0.7;
    const double nl = viewLen * f;
    setView (anchor - (anchor - viewStart) * f, nl);
}
