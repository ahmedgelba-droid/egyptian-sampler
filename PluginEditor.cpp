#include "PluginEditor.h"
#include <cmath>

static const char* kNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

static juce::Font uiFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions ("Consolas", size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::String noteName (int n)
{
    return juce::String (kNames[n % 12]) + juce::String (n / 12 - 1);
}

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0b1222));
    setColour (juce::PopupMenu::textColourId, neon::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neon::cyan.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::ComboBox::textColourId, neon::text);
    setColour (juce::TextButton::textColourOffId, neon::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff0b1222));
    setColour (juce::AlertWindow::textColourId, neon::text);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff050912));
    setColour (juce::TextEditor::textColourId, neon::text);
}

juce::Font NeonLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (13.0f); }
juce::Font NeonLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return uiFont (12.0f); }

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, ang, true);
    g.setColour (accent.withAlpha (0.16f));
    g.strokePath (val, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent.withAlpha (0.35f));
    g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (val, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = r - 8.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b3556), c.x - br * 0.5f, c.y - br * 0.7f,
                                             juce::Colour (0xff060912), c.x + br * 0.6f, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
    g.setColour (accent.withAlpha (0.4f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

    juce::Path p;
    p.addRoundedRectangle (-1.6f, -br + 2.5f, 3.2f, br * 0.5f, 1.5f);
    p.applyTransform (juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (accent.brighter (0.6f));
    g.fillPath (p);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);

    g.setColour (on ? accent.withAlpha (0.16f) : juce::Colours::white.withAlpha (hl ? 0.08f : 0.04f));
    g.fillRoundedRectangle (r, 6.0f);

    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.drawRoundedRectangle (r.expanded (1.5f), 7.0f, 3.0f);
    }
    g.setColour (on ? accent : neon::dim.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    const float ls = 8.0f;
    juce::Rectangle<float> led (r.getX() + 9.0f, r.getCentreY() - ls * 0.5f, ls, ls);
    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (on ? accent : neon::dim.darker (0.5f));
    g.fillEllipse (led);

    g.setColour (on ? neon::text : neon::dim);
    g.setFont (uiFont (12.5f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (24.0f).toNearestInt(), juce::Justification::centredLeft, true);
}

void NeonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float mn, float mx,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearBarVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, mn, mx, style, s);
        return;
    }

    const double v = s.getValue();
    const bool zero = std::abs (v) < 0.5;
    const auto accent = zero ? neon::dim : (v < 0 ? neon::magenta : neon::amber);
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (0.5f);

    g.setColour (juce::Colour (0xff050912));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (accent.withAlpha (zero ? 0.5f : 0.9f));
    g.drawRoundedRectangle (r, 5.0f, 1.2f);

    const float half = r.getWidth() * 0.5f - 4.0f;
    const float barW = (float) (std::abs (v) / 100.0) * half;
    const float cx = r.getCentreX();
    g.setColour (accent.withAlpha (0.8f));
    g.fillRect (v < 0 ? cx - barW : cx, r.getBottom() - 6.0f, juce::jmax (1.0f, barW), 3.0f);

    g.setColour (zero ? neon::dim : accent.brighter (0.3f));
    g.setFont (uiFont (13.0f));
    g.drawText ((v > 0.5 ? "+" : "") + juce::String ((int) std::round (v)), r.withTrimmedBottom (6.0f).toNearestInt(),
                juce::Justification::centred, false);
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    const auto accent = b.findColour (juce::TextButton::buttonColourId);
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();

    g.setColour (accent.withAlpha (on ? 0.28f : (down ? 0.3f : (hl ? 0.16f : 0.07f))));
    g.fillRoundedRectangle (r, 6.0f);
    if (on || hl)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.drawRoundedRectangle (r.expanded (1.5f), 7.0f, 3.0f);
    }
    g.setColour (accent.withAlpha (on ? 1.0f : 0.75f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

void NeonLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    const auto accent = box.findColour (juce::ComboBox::outlineColourId);
    g.setColour (juce::Colour (0xff050912));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (accent);
    g.fillPath (arrow);
}

void CentsBox::mouseDoubleClick (const juce::MouseEvent&)
{
    const float cur = param.convertFrom0to1 (param.getValue());
    const float target = std::abs (cur) > 0.5f ? 0.0f : -50.0f;

    param.beginChangeGesture();
    param.setValueNotifyingHost (param.convertTo0to1 (target));
    param.endChangeGesture();
}

//==============================================================================
MapView::MapView (NayAudioProcessor& p) : proc (p)
{
    for (auto* b : { &btnA, &btnB })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (77);
        addAndMakeVisible (*b);
    }
    btnA.setColour (juce::TextButton::buttonColourId, neon::cyan);
    btnB.setColour (juce::TextButton::buttonColourId, neon::magenta);
    resetBtn.setColour (juce::TextButton::buttonColourId, neon::amber);
    btnA.setToggleState (true, juce::dontSendNotification);

    btnA.onClick = [this] { if (btnA.getToggleState()) { editB = false; selected = -1; refresh(); } };
    btnB.onClick = [this] { if (btnB.getToggleState()) { editB = true;  selected = -1; refresh(); } };
    resetBtn.onClick = [this]
    {
        proc.resetZones (editB ? proc.getLayerBGroup() : proc.getActiveGroup());
        selected = -1;
        refresh();
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
            repaint();
        }
    };
    addAndMakeVisible (rootSlider);

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
    return proc.getBankOf (editB ? proc.getLayerBGroup() : proc.getActiveGroup());
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
        fileLabel.setText (s.fileName, juce::dontSendNotification);
        rootSlider.setEnabled (true);
    }
    else
    {
        selected = -1;
        fileLabel.setText ("Click a zone. Drag to move, drag its edges to resize.", juce::dontSendNotification);
        rootSlider.setEnabled (false);
    }
    repaint();
}

void MapView::resized()
{
    auto a = getLocalBounds().reduced (14);
    btnA.setBounds (a.getX(), a.getY(), 140, 28);
    btnB.setBounds (a.getX() + 148, a.getY(), 140, 28);
    resetBtn.setBounds (a.getRight() - 140, a.getY(), 140, 28);

    const int bottomRowH = 32;
    const int bottom = a.getBottom() - bottomRowH;
    keyStrip = { a.getX() + 34, bottom - 6 - 34, a.getWidth() - 34, 34 };
    grid = { keyStrip.getX(), a.getY() + 40, keyStrip.getWidth(), keyStrip.getY() - 2 - (a.getY() + 40) };

    fileLabel.setBounds (a.getX(), bottom + 4, 380, 26);
    rootLabel.setBounds (a.getX() + 400, bottom + 4, 80, 26);
    rootSlider.setBounds (a.getX() + 480, bottom + 4, 360, 26);
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
    g.setColour ((editB ? neon::magenta : neon::cyan).withAlpha (0.4f));
    g.drawRoundedRectangle (all.reduced (0.5f), 10.0f, 1.2f);

    g.setColour (juce::Colour (0xff060a14));
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
    g.setColour ((editB ? neon::magenta : neon::cyan).withAlpha (0.5f));
    g.drawRect (grid, 1);

    // keyboard strip
    for (int k = 0; k < 128; ++k)
    {
        const int pc = k % 12;
        const bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        g.setColour (black ? juce::Colour (0xff0b0f1c) : neon::text.withAlpha (0.82f));
        g.fillRect (keyToX (k), (float) keyStrip.getY(), keyW() + 0.5f, (float) keyStrip.getHeight());
        if (pc == 0)
        {
            g.setColour (juce::Colour (0xff0b0f1c));
            g.setFont (uiFont (9.5f));
            g.drawText ("C" + juce::String (k / 12 - 1), (int) keyToX (k), keyStrip.getBottom() - 14, 24, 12, juce::Justification::centredLeft, false);
        }
    }

    auto bank = currentBank();
    if (bank == nullptr || bank->empty())
    {
        g.setColour (neon::dim);
        g.setFont (uiFont (15.0f));
        g.drawText (editB ? "NO INSTRUMENT ON LAYER B  (pick one with the arrows)" : "NO INSTRUMENT ON LAYER A  (use ADD FOLDER)",
                    grid, juce::Justification::centred, false);
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
            const auto col = juce::Colour::fromHSV (std::fmod ((float) i * 0.17f + (editB ? 0.8f : 0.5f), 1.0f), 0.75f, 1.0f, 1.0f);

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
    repaint();
}

//==============================================================================
NayAudioProcessorEditor::NayAudioProcessorEditor (NayAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), mapView (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (page);
    addChildComponent (mapView);

    const auto cy = neon::cyan, mg = neon::magenta, am = neon::amber, gr = neon::green, vi = neon::violet;

    // ---- voice ----
    kGain  = addKnob (P_gain,    "GAIN",    2, "", cy);
    kAtk   = addKnob (P_attack,  "ATTACK",  3, "s", cy);
    kRel   = addKnob (P_release, "RELEASE", 2, "s", cy);
    kGlide = addKnob (P_glide,   "GLIDE",   0, "ms", cy);
    kVibD  = addKnob (P_vibDepth,"VIB DEPTH", 0, "ct", cy);
    kVibR  = addKnob (P_vibRate, "VIB SPEED", 1, "Hz", cy);
    kVel   = addKnob (P_velSens, "VELOCITY", 2, "", cy);
    kPb    = addKnob (P_pbRange, "BEND RNG", 0, "st", cy);
    tMono   = addToggle (P_mono,   "MONO",   cy);
    tLegato = addToggle (P_legato, "LEGATO", cy);

    // ---- layer B ----
    kLbLevel = addKnob (P_layerBLevel, "B LEVEL", 1, "dB", mg);
    kLbVel   = addKnob (P_layerBVel,   "B FROM VEL", 0, "", mg);
    addCombo (modeBox, P_layerBMode, modeAtt);
    modeBox.setColour (juce::ComboBox::outlineColourId, mg);

    // ---- maqam ----
    addCombo (maqamBox, P_maqam, maqamAtt);
    addCombo (rootBox, P_root, rootAtt);
    maqamBox.setColour (juce::ComboBox::outlineColourId, am);
    rootBox.setColour (juce::ComboBox::outlineColourId, am);

    for (int i = 0; i < 12; ++i)
    {
        const auto id = "off" + juce::String (i);
        boxes[(size_t) i] = std::make_unique<CentsBox> (*proc.apvts.getParameter (id));
        boxes[(size_t) i]->setSliderStyle (juce::Slider::LinearBarVertical);
        boxes[(size_t) i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        boxAtt[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *boxes[(size_t) i]);
        page.addAndMakeVisible (*boxes[(size_t) i]);
    }

    // ---- effects ----
    tSat = addToggle (P_satOn, "ON", am);
    kSatDrive = addKnob (P_satDrive, "DRIVE",  2, "", am);
    kSatMix   = addKnob (P_satMix,   "AMOUNT", 2, "", am);

    tWah = addToggle (P_wahOn, "ON", gr);
    kWahPos  = addKnob (P_wahPos,  "PEDAL",  2, "", gr);
    kWahRate = addKnob (P_wahRate, "RATE",   2, "Hz", gr);
    kWahMix  = addKnob (P_wahMix,  "AMOUNT", 2, "", gr);

    tPh = addToggle (P_phOn, "ON", vi);
    kPhRate  = addKnob (P_phRate,  "RATE",   2, "Hz", vi);
    kPhDepth = addKnob (P_phDepth, "DEPTH",  2, "", vi);
    kPhMix   = addKnob (P_phMix,   "AMOUNT", 2, "", vi);

    kDlTime = addKnob (P_delayTime,  "TIME",     0, "ms", cy);
    kDlFb   = addKnob (P_delayFb,    "FEEDBACK", 2, "", cy);
    kDlLvl  = addKnob (P_delayLevel, "LEVEL",    1, "dB", cy);
    kRvSize = addKnob (P_rvSize,     "SIZE",     2, "", mg);
    kRvLvl  = addKnob (P_rvLevel,    "LEVEL",    1, "dB", mg);

    // ---- instrument selectors ----
    juce::Path left, right;
    left.addTriangle (0.f, 0.5f, 1.f, 0.f, 1.f, 1.f);
    right.addTriangle (1.f, 0.5f, 0.f, 0.f, 0.f, 1.f);
    prevA.setShape (left, false, true, false);  nextA.setShape (right, false, true, false);
    prevB.setShape (left, false, true, false);  nextB.setShape (right, false, true, false);
    for (auto* b : { &prevA, &nextA, &prevB, &nextB })
        addAndMakeVisible (*b);

    prevA.onClick = [this] { stepGroup (-1); };
    nextA.onClick = [this] { stepGroup (1); };
    prevB.onClick = [this] { stepLayerB (-1); };
    nextB.onClick = [this] { stepLayerB (1); };

    addButton.setColour (juce::TextButton::buttonColourId, neon::cyan);
    renameButton.setColour (juce::TextButton::buttonColourId, neon::cyan);
    mapButton.setColour (juce::TextButton::buttonColourId, neon::magenta);
    addAndMakeVisible (addButton);
    addAndMakeVisible (renameButton);
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

    mapButton.setClickingTogglesState (true);
    mapButton.onClick = [this]
    {
        mapMode = mapButton.getToggleState();
        page.setVisible (! mapMode);
        mapView.setVisible (mapMode);
        mapButton.setButtonText (mapMode ? "BACK" : "MAPPING");
        mapView.refresh();
    };

    info.setColour (juce::Label::textColourId, neon::dim);
    info.setFont (uiFont (11.0f, false));
    info.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (info);

    // panels are painted on the page (page-local coordinates)
    page.onPaint = [this] (juce::Graphics& g)
    {
        drawPanel (g, pVoice, "VOICE", neon::cyan);
        drawPanel (g, pLayer, "LAYER B  //  VELOCITY SWITCH", neon::magenta);
        drawPanel (g, pMaqam, "MAQAM  //  MICROTONAL TUNING", neon::amber);
        drawPanel (g, pSat, "SATURATION", neon::amber);
        drawPanel (g, pWah, "WAH-WAH", neon::green);
        drawPanel (g, pPh, "PHASER", neon::violet);
        drawPanel (g, pDelay, "DELAY", neon::cyan);
        drawPanel (g, pReverb, "REVERB", neon::magenta);

        g.setFont (uiFont (10.5f));
        g.setColour (neon::dim);
        g.drawText ("MODE", pLayer.getX() + 236, pLayer.getY() + 30, 100, 14, juce::Justification::centredLeft, false);
        g.drawFittedText ("Layer B (e.g. tremolo or oud) plays from the chosen velocity.\n"
                          "SWITCH = replaces layer A.  STACK = plays on top of A.",
                          pLayer.getX() + 14, pLayer.getBottom() - 46, pLayer.getWidth() - 28, 36,
                          juce::Justification::topLeft, 3);

        g.setColour (neon::amber);
        g.drawText ("MAQAM", pMaqam.getX() + 14, pMaqam.getY() + 24, 100, 14, juce::Justification::centredLeft, false);
        g.drawText ("ROOT KEY", pMaqam.getX() + 14, pMaqam.getY() + 70, 100, 14, juce::Justification::centredLeft, false);

        for (int i = 0; i < 12; ++i)
        {
            const int cx = pMaqam.getX() + 250 + i * 59;
            g.setColour (neon::text);
            g.setFont (uiFont (12.0f));
            g.drawText (kNames[i], cx, pMaqam.getY() + 28, 56, 16, juce::Justification::centred, false);
        }
        g.setFont (uiFont (10.0f, false));
        g.setColour (neon::dim);
        g.drawText ("FINE TUNE (cents): drag, double-click = quarter tone.  The maqam offsets are added on top.",
                    pMaqam.getX() + 250, pMaqam.getBottom() - 22, 710, 14, juce::Justification::centredLeft, false);
    };

    refreshGroups();
    built = true;
    setSize (1000, 596); // last, so resized() finds every component already created
}

NayAudioProcessorEditor::~NayAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

NayAudioProcessorEditor::Knob* NayAudioProcessorEditor::addKnob (int idx, const char* name, int decimals,
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
    s.setPopupDisplayEnabled (false, false, nullptr);

    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], s);

    // set after the attachment so our text formatting wins
    s.textFromValueFunction = [decimals, suffix] (double v) -> juce::String
    {
        if (suffix == "dB" && v <= -59.9)
            return "OFF";
        return juce::String (v, decimals) + (suffix.isEmpty() ? juce::String() : " " + suffix);
    };
    s.valueFromTextFunction = [] (const juce::String& t) -> double
    {
        return t.trim().startsWithIgnoreCase ("off") ? -60.0 : t.getDoubleValue();
    };
    s.updateText();

    k->label.setText (name, juce::dontSendNotification);
    k->label.setFont (uiFont (10.5f));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, neon::dim);
    k->label.setInterceptsMouseClicks (false, false);

    page.addAndMakeVisible (s);
    page.addAndMakeVisible (k->label);

    knobs.push_back (std::move (k));
    return knobs.back().get();
}

juce::ToggleButton* NayAudioProcessorEditor::addToggle (int idx, const juce::String& text, juce::Colour accent)
{
    auto t = std::make_unique<juce::ToggleButton> (text);
    t->setColour (juce::ToggleButton::tickColourId, accent);
    toggleAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], *t));
    page.addAndMakeVisible (*t);
    toggles.push_back (std::move (t));
    return toggles.back().get();
}

void NayAudioProcessorEditor::addCombo (juce::ComboBox& box, int idx,
                                        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
{
    if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (NayAudioProcessor::paramIds[idx])))
        box.addItemList (ch->choices, 1);

    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, NayAudioProcessor::paramIds[idx], box);
    box.setColour (juce::ComboBox::outlineColourId, neon::cyan);
    page.addAndMakeVisible (box);
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
void NayAudioProcessorEditor::stepGroup (int delta)
{
    const int n = proc.getNumGroups();
    if (n <= 0)
        return;

    proc.selectGroup ((proc.getActiveGroup() + delta + n) % n);
    refreshGroups();
}

void NayAudioProcessorEditor::stepLayerB (int delta)
{
    const int n = proc.getNumGroups();
    if (n <= 0)
        return;

    int next = proc.getLayerBGroup() + delta;   // -1 = off
    if (next >= n)  next = -1;
    if (next < -1)  next = n - 1;

    proc.selectLayerB (next);
    refreshGroups();
}

void NayAudioProcessorEditor::refreshGroups()
{
    info.setText (proc.getSampleInfo(), juce::dontSendNotification);
    mapView.refresh();
    repaint();
}

//==============================================================================
void NayAudioProcessorEditor::rebuildBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    bgImage = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (bgImage);

    g.setGradientFill (juce::ColourGradient (neon::bg1, 0.0f, 0.0f, neon::bg0, 0.0f, (float) h, false));
    g.fillAll();

    g.setColour (neon::cyan.withAlpha (0.035f));
    for (int x = 0; x < w; x += 40) g.fillRect (x, 0, 1, h);
    for (int y = 0; y < h; y += 40) g.fillRect (0, y, w, 1);

    g.setGradientFill (juce::ColourGradient (neon::cyan.withAlpha (0.14f), 0.0f, 0.0f, neon::cyan.withAlpha (0.0f), 520.0f, 360.0f, true));
    g.fillRect (0, 0, w, h);
    g.setGradientFill (juce::ColourGradient (neon::magenta.withAlpha (0.12f), (float) w, (float) h, neon::magenta.withAlpha (0.0f), (float) w - 520.0f, (float) h - 330.0f, true));
    g.fillRect (0, 0, w, h);

    g.setGradientFill (juce::ColourGradient (neon::cyan.withAlpha (0.8f), 16.0f, 0.0f, neon::magenta.withAlpha (0.8f), (float) w - 16.0f, 0.0f, false));
    g.fillRect (16, 64, w - 32, 1);
}

void NayAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& title, juce::Colour accent)
{
    auto r = ri.toFloat();
    g.setColour (neon::panel.withAlpha (0.82f));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (accent.withAlpha (0.28f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    // corner brackets
    g.setColour (accent);
    const float L = 11.0f;
    g.fillRect (r.getX(), r.getY(), L, 2.0f);                 g.fillRect (r.getX(), r.getY(), 2.0f, L);
    g.fillRect (r.getRight() - L, r.getY(), L, 2.0f);         g.fillRect (r.getRight() - 2.0f, r.getY(), 2.0f, L);
    g.fillRect (r.getX(), r.getBottom() - 2.0f, L, 2.0f);     g.fillRect (r.getX(), r.getBottom() - L, 2.0f, L);
    g.fillRect (r.getRight() - L, r.getBottom() - 2.0f, L, 2.0f); g.fillRect (r.getRight() - 2.0f, r.getBottom() - L, 2.0f, L);

    g.setFont (uiFont (11.5f).withExtraKerningFactor (0.15f));
    g.setColour (accent);
    g.drawText (title, ri.getX() + 14, ri.getY() + 6, ri.getWidth() - 20, 16, juce::Justification::centredLeft, false);
}

void NayAudioProcessorEditor::drawLcd (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& tag,
                                       const juce::String& name, const juce::String& sub, juce::Colour accent)
{
    auto r = ri.toFloat();
    g.setColour (juce::Colour (0xff050912));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (accent.withAlpha (0.18f));
    g.drawRoundedRectangle (r.expanded (1.5f), 9.0f, 3.0f);
    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.2f);

    auto inner = ri.reduced (38, 0);
    g.setColour (accent.withAlpha (0.7f));
    g.setFont (uiFont (10.0f).withExtraKerningFactor (0.2f));
    g.drawText (tag, inner.getX(), ri.getY() + 5, inner.getWidth(), 12, juce::Justification::centred, false);

    g.setColour (accent.withAlpha (0.2f));
    g.setFont (uiFont (21.0f));
    g.drawText (name, inner.translated (1, 1).withTrimmedTop (14).withTrimmedBottom (14), juce::Justification::centred, true);
    g.setColour (accent.brighter (0.5f));
    g.drawText (name, inner.withTrimmedTop (14).withTrimmedBottom (14), juce::Justification::centred, true);

    g.setColour (neon::dim);
    g.setFont (uiFont (10.5f, false));
    g.drawText (sub, inner.getX(), ri.getBottom() - 17, inner.getWidth(), 12, juce::Justification::centred, true);
}

void NayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (bgImage, 0, 0);

    // title with glow
    auto f = uiFont (30.0f).withExtraKerningFactor (0.12f);
    g.setFont (f);
    for (int d = 3; d >= 1; --d)
    {
        g.setColour (neon::cyan.withAlpha (0.06f));
        g.drawText ("EGYPTIAN SAMPLER", 24 - d, 10 - d, 520 + 2 * d, 36 + 2 * d, juce::Justification::centredLeft, false);
    }
    g.setColour (neon::text);
    g.drawText ("EGYPTIAN SAMPLER", 24, 10, 520, 36, juce::Justification::centredLeft, false);

    g.setFont (uiFont (10.5f).withExtraKerningFactor (0.25f));
    g.setColour (neon::magenta);
    g.drawText ("MAQAM ENGINE  //  MICROTONAL SAMPLER", 26, 44, 520, 14, juce::Justification::centredLeft, false);

    const int a = proc.getActiveGroup(), b = proc.getLayerBGroup();
    auto subOf = [this] (int i)
    {
        auto bank = proc.getBankOf (i);
        return bank != nullptr ? juce::String ((int) bank->size()) + " samples" : juce::String();
    };

    drawLcd (g, lcdA, "LAYER A", a >= 0 ? proc.getGroupName (a) : "NO INSTRUMENT",
             a >= 0 ? subOf (a) : "press ADD FOLDER", neon::cyan);
    drawLcd (g, lcdB, "LAYER B", b >= 0 ? proc.getGroupName (b) : "OFF",
             b >= 0 ? subOf (b) : "use the arrows to pick", neon::magenta);
}

void NayAudioProcessorEditor::resized()
{
    if (! built)
        return;

    rebuildBackground();

    lcdA = { 16, 72, 330, 60 };
    lcdB = { 354, 72, 330, 60 };
    prevA.setBounds (lcdA.getX() + 8, lcdA.getCentreY() - 14, 20, 28);
    nextA.setBounds (lcdA.getRight() - 28, lcdA.getCentreY() - 14, 20, 28);
    prevB.setBounds (lcdB.getX() + 8, lcdB.getCentreY() - 14, 20, 28);
    nextB.setBounds (lcdB.getRight() - 28, lcdB.getCentreY() - 14, 20, 28);

    addButton.setBounds (692, 72, 140, 28);
    renameButton.setBounds (692, 104, 140, 28);
    info.setBounds (842, 72, 142, 60);
    mapButton.setBounds (842, 18, 142, 32);

    pageRect = { 16, 142, 968, 438 };
    page.setBounds (pageRect);
    mapView.setBounds (pageRect);

    // ---- page-local layout ----
    pVoice = { 0, 0, 520, 166 };
    pLayer = { 528, 0, 440, 166 };
    pMaqam = { 0, 174, 968, 124 };

    layoutRow (pVoice.withTrimmedTop (24).reduced (10, 0).withHeight (92).withY (24),
               { kGain, kAtk, kRel, kGlide, kVibD, kVibR, kVel, kPb });
    tMono->setBounds (pVoice.getX() + 12, 24 + 92 + 8, 110, 28);
    tLegato->setBounds (pVoice.getX() + 130, 24 + 92 + 8, 110, 28);

    layoutRow ({ pLayer.getX() + 10, 24, 200, 92 }, { kLbLevel, kLbVel });
    modeBox.setBounds (pLayer.getX() + 230, 48, 190, 28);

    maqamBox.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 40, 210, 28);
    rootBox.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 86, 210, 28);
    for (int i = 0; i < 12; ++i)
        boxes[(size_t) i]->setBounds (pMaqam.getX() + 250 + i * 59 + 3, pMaqam.getY() + 46, 51, 38);

    // FX row: widths follow the knob count
    const int counts[5] = { 2, 3, 3, 3, 2 };
    int sum = 0;
    for (int c : counts) sum += c * 64 + 16;
    const int gap = (968 - sum) / 4;
    const int fy = 306, fh = 132;

    juce::Rectangle<int>* fx[5] = { &pSat, &pWah, &pPh, &pDelay, &pReverb };
    int x = 0;
    for (int i = 0; i < 5; ++i)
    {
        const int w = counts[i] * 64 + 16;
        *fx[i] = { x, fy, w, fh };
        x += w + gap;
    }

    auto knobArea = [&] (const juce::Rectangle<int>& p, int n) { return juce::Rectangle<int> (p.getX() + 8, p.getY() + 34, n * 64, 92); };
    layoutRow (knobArea (pSat, 2),   { kSatDrive, kSatMix });
    layoutRow (knobArea (pWah, 3),   { kWahPos, kWahRate, kWahMix });
    layoutRow (knobArea (pPh, 3),    { kPhRate, kPhDepth, kPhMix });
    layoutRow (knobArea (pDelay, 3), { kDlTime, kDlFb, kDlLvl });
    layoutRow (knobArea (pReverb, 2),{ kRvSize, kRvLvl });

    tSat->setBounds (pSat.getRight() - 52, pSat.getY() + 5, 44, 22);
    tWah->setBounds (pWah.getRight() - 52, pWah.getY() + 5, 44, 22);
    tPh->setBounds (pPh.getRight() - 52, pPh.getY() + 5, 44, 22);
}
