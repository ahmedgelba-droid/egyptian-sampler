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

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d1626));
    setColour (juce::PopupMenu::textColourId, neon::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, neon::cyan.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::ComboBox::textColourId, neon::text);
    setColour (juce::TextButton::textColourOffId, neon::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff0d1626));
    setColour (juce::AlertWindow::textColourId, neon::text);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff070c16));
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

    // gold bezel + dark stone-like cap (NileVerb style)
    const float bz = r - 5.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2c866), c.x - bz * 0.6f, c.y - bz * 0.8f,
                                             juce::Colour (0xff7a4a16), c.x + bz * 0.6f, c.y + bz * 0.9f, false));
    g.fillEllipse (c.x - bz, c.y - bz, bz * 2.0f, bz * 2.0f);
    const float br = r - 8.5f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c4352), c.x - br * 0.5f, c.y - br * 0.7f,
                                             juce::Colour (0xff0b1018), c.x + br * 0.6f, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
    g.setColour (juce::Colour (0xff1a1209).withAlpha (0.8f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

    juce::Path p;
    p.addRoundedRectangle (-1.6f, -br + 2.5f, 3.2f, br * 0.5f, 1.5f);
    p.applyTransform (juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (juce::Colour (0xfff2c866));
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
    g.setColour (juce::Colour (0xff070c16));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (accent);
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
    g.setColour (neon::cyan.withAlpha (0.4f));
    g.drawRoundedRectangle (all.reduced (0.5f), 10.0f, 1.2f);

    g.setColour (juce::Colour (0xff070c16));
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
    g.drawText ("MAPPING  //  key range x velocity range", 14, 14, 420, 28, juce::Justification::centredLeft, false);

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
        g.setColour (neon::dim);
        g.setFont (uiFont (15.0f));
        g.drawText ("NO INSTRUMENT  (use ADD FOLDER)",
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
            const auto col = juce::Colour::fromHSV (std::fmod ((float) i * 0.17f + 0.5f, 1.0f), 0.75f, 1.0f, 1.0f);

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
void WheelView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float v = juce::jlimit (bipolar ? -1.0f : 0.0f, 1.0f, getter());

    g.setColour (juce::Colour (0xff070c16));
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
    g.setColour (col.withAlpha (0.3f));
    g.fillRect (r.getX() + 1.0f, pos - 4.0f, r.getWidth() - 2.0f, 8.0f);
    g.setColour (col.brighter (0.4f));
    g.fillRect (r.getX() + 1.0f, pos - 1.5f, r.getWidth() - 2.0f, 3.0f);

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
    const float eff = effective();
    const bool alt = std::abs (eff) > 0.5f;
    const auto led = alt ? juce::Colour (0xffff3b2f) : juce::Colour (0xff2fa8ff);
    const float cx = (float) getWidth() * 0.5f, cy = 31.0f;

    g.setColour (neon::text);
    g.setFont (uiFont (13.0f));
    g.drawText (kNames[pc], 0, 0, getWidth(), 13, juce::Justification::centred, false);

    if (alt)
    {
        g.setColour (led.withAlpha (0.25f));
        g.fillEllipse (cx - 21.0f, cy - 21.0f, 42.0f, 42.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a5430), cx - 8.0f, cy - 12.0f, juce::Colour (0xff0b1018), cx + 10.0f, cy + 17.0f, false));
    g.fillEllipse (cx - 17.0f, cy - 17.0f, 34.0f, 34.0f);
    g.setColour (neon::amber.withAlpha (0.55f));
    g.drawEllipse (cx - 17.0f, cy - 17.0f, 34.0f, 34.0f, 1.5f);
    g.setColour (juce::Colour (0xff070c16));
    g.fillEllipse (cx - 11.0f, cy - 11.0f, 22.0f, 22.0f);
    g.setColour (led);
    g.drawEllipse (cx - 11.0f, cy - 11.0f, 22.0f, 22.0f, 1.5f);
    g.fillEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);

    g.setColour (alt ? juce::Colour (0xffff7a6a) : neon::dim);
    g.setFont (uiFont (12.0f));
    const int v = (int) std::round (eff);
    g.drawText ((v > 0 ? "+" : "") + juce::String (v), 0, 49, getWidth(), 13, juce::Justification::centred, false);
}

//==============================================================================
static void drawAnkh (juce::Graphics& g, float x, float y, float h, juce::Colour c)
{
    g.setColour (c);
    const float w = h * 0.6f, th = h * 0.1f;
    g.drawEllipse (x + w * 0.2f, y, w * 0.6f, h * 0.42f, th);
    g.fillRect (x + w * 0.5f - th * 0.5f, y + h * 0.42f, th, h * 0.58f);
    g.fillRect (x, y + h * 0.5f, w, th);
}

NayAudioProcessorEditor::NayAudioProcessorEditor (NayAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), mapView (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (page);
    addChildComponent (settings);
    addChildComponent (browse);
    addChildComponent (mixer);
    settings.addAndMakeVisible (mapView);

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
    tSat = addToggle (page, P_satOn, "ACTIVE", gd);
    kSatDrive = addKnob (page, P_satDrive, "DRIVE",  2, "", gd);
    kSatMix   = addKnob (page, P_satMix,   "AMOUNT", 2, "", gd);


    tWah = addToggle (page, P_wahOn, "ACTIVE", pp);
    kWahPos = addKnob (page, P_wahPos, "PEDAL",  2, "", pp);
    kWahMix = addKnob (page, P_wahMix, "AMOUNT", 2, "", pp);

    tPh = addToggle (page, P_phOn, "ACTIVE", lp);
    kPhRate  = addKnob (page, P_phRate,  "RATE",   2, "Hz", lp);
    kPhDepth = addKnob (page, P_phDepth, "DEPTH",  2, "", lp);
    kPhMix   = addKnob (page, P_phMix,   "AMOUNT", 2, "", lp);

    kDlTime = addKnob (page, P_delayTime,  "TIME",     0, "ms", tq);
    kDlFb   = addKnob (page, P_delayFb,    "FEEDBACK", 2, "", tq);
    kDlLvl  = addKnob (page, P_delayLevel, "LEVEL",    1, "dB", tq);
    kRvSize = addKnob (page, P_rvSize,     "SIZE",     2, "", cr);
    kRvLvl  = addKnob (page, P_rvLevel,    "LEVEL",    1, "dB", cr);

    // ---- arpeggiator ----
    tArp = addToggle (page, P_arpOn, "ARP ON", gd);
    tArpHold = addToggle (page, P_arpHold, "HOLD", gd);
    addCombo (page, arpModeBox, P_arpMode, arpModeAtt);
    addCombo (page, arpRateBox, P_arpRate, arpRateAtt);
    arpModeBox.setColour (juce::ComboBox::outlineColourId, gd);
    arpRateBox.setColour (juce::ComboBox::outlineColourId, gd);
    kArpOct  = addKnob (page, P_arpOct,  "OCTAVES", 0, "", gd);
    kArpGate = addKnob (page, P_arpGate, "GATE",    2, "", gd);

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

    // ---- browse page ----
    searchBox.setTextToShowWhenEmpty ("search by name...", neon::dim);
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff070c16));
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
        lb->setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff070c16));
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

    mixer.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -mixer.getX(), -mixer.getY());
        drawPanel (g, pMixMain, "MAIN", neon::cyan);
        drawPanel (g, pMixDuo, "DUO", neon::amber);
        drawPanel (g, pMixMaster, "MASTER", neon::green);

        const int a = proc.getActiveGroup(), d = proc.getDuoGroup();
        g.setFont (uiFont (13.0f));
        g.setColour (neon::text);
        g.drawText (a >= 0 ? proc.getGroupName (a) : juce::String ("NO INSTRUMENT"), pMixMain.getX() + 10, pMixMain.getY() + 28, pMixMain.getWidth() - 20, 18, juce::Justification::centred, true);
        g.drawText (d >= 0 ? proc.getGroupName (d) : juce::String ("OFF  (choose one in BROWSE)"), pMixDuo.getX() + 10, pMixDuo.getY() + 28, pMixDuo.getWidth() - 20, 18, juce::Justification::centred, true);
        g.setColour (neon::dim);
        g.setFont (uiFont (10.5f, false));
        g.drawText ("Pan: center = unity gain.  Right-click any knob = MIDI learn.", 0, 330, 968, 14, juce::Justification::centredLeft, false);
    };

    info.setColour (juce::Label::textColourId, neon::dim);
    info.setFont (uiFont (11.0f, false));
    info.setJustificationType (juce::Justification::topLeft);
    settings.addAndMakeVisible (info);

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
        drawPanel (g, pMaqam, "MICROTONAL KEYS", neon::amber);
        drawPanel (g, pSat, "KRONOS", neon::amber);
        drawPanel (g, pWah, "WAH", neon::green);
        drawPanel (g, pPh, "PHASER", neon::violet);
        drawPanel (g, pDelay, "DELAY", neon::cyan);
        drawPanel (g, pReverb, "REVERB", neon::magenta);
        drawPanel (g, pArp, "ARPEGGIATOR", neon::amber);
        drawPanel (g, pWheels, "WHEELS", neon::cyan);
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
        g.drawText ("Altered notes light up red.  Click = quarter-tone on/off.  Drag = fine tune.",
                    pMaqam.getX() + 330, pMaqam.getY() + 6, 630, 14, juce::Justification::centredRight, false);
    };

    // ---------------- settings page painting ----------------
    settings.onPaint = [this] (juce::Graphics& g)
    {
        g.drawImageAt (bgImage, -settings.getX(), -settings.getY());

        const int a = proc.getActiveGroup();
        auto bank = proc.getBankOf (a);
        drawLcd (g, lcdA2, "INSTRUMENT", a >= 0 ? proc.getGroupName (a) : "NO INSTRUMENT",
                 a >= 0 ? juce::String (bank != nullptr ? (int) bank->size() : 0) + " samples" : "press ADD FOLDER", neon::cyan);
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
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff070c16));
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

void NayAudioProcessorEditor::showView (int v)
{
    view = v;
    page.setVisible (v == 0);
    browse.setVisible (v == 1);
    mixer.setVisible (v == 2);
    settings.setVisible (v == 3);
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
// simple hand-drawn hieroglyph-like symbols for the carved side borders
static void drawGlyph (juce::Graphics& g, int kind, float cx, float cy, float s, juce::Colour col)
{
    juce::Path p;
    switch (kind % 6)
    {
        case 0: // ankh
            p.addEllipse (cx - s * 0.22f, cy - s * 0.5f, s * 0.44f, s * 0.5f);
            p.addRectangle (cx - s * 0.05f, cy, s * 0.1f, s * 0.5f);
            p.addRectangle (cx - s * 0.3f, cy + s * 0.08f, s * 0.6f, s * 0.09f);
            break;
        case 1: // eye
            p.startNewSubPath (cx - s * 0.4f, cy);
            p.quadraticTo (cx, cy - s * 0.4f, cx + s * 0.4f, cy);
            p.quadraticTo (cx, cy + s * 0.3f, cx - s * 0.4f, cy);
            p.addEllipse (cx - s * 0.12f, cy - s * 0.12f, s * 0.24f, s * 0.24f);
            p.addRectangle (cx - s * 0.04f, cy + s * 0.22f, s * 0.08f, s * 0.26f);
            break;
        case 2: // water
            for (int k = 0; k < 3; ++k)
            {
                const float yy = cy - s * 0.25f + (float) k * s * 0.25f;
                p.startNewSubPath (cx - s * 0.4f, yy);
                for (int z = 1; z <= 4; ++z)
                    p.lineTo (cx - s * 0.4f + (float) z * s * 0.2f, yy + (z % 2 ? -s * 0.1f : 0.0f));
            }
            break;
        case 3: // bird
            p.addEllipse (cx - s * 0.12f, cy - s * 0.45f, s * 0.24f, s * 0.24f);
            p.addRoundedRectangle (cx - s * 0.22f, cy - s * 0.2f, s * 0.44f, s * 0.5f, s * 0.15f);
            p.addRectangle (cx - s * 0.16f, cy + s * 0.3f, s * 0.05f, s * 0.2f);
            p.addRectangle (cx + s * 0.11f, cy + s * 0.3f, s * 0.05f, s * 0.2f);
            break;
        case 4: // was scepter
            p.addRectangle (cx - s * 0.04f, cy - s * 0.4f, s * 0.08f, s * 0.9f);
            p.addTriangle (cx - s * 0.2f, cy - s * 0.5f, cx + s * 0.2f, cy - s * 0.5f, cx, cy - s * 0.25f);
            break;
        default: // sun disk
            p.addEllipse (cx - s * 0.3f, cy - s * 0.3f, s * 0.6f, s * 0.6f);
            p.addEllipse (cx - s * 0.08f, cy - s * 0.08f, s * 0.16f, s * 0.16f);
            break;
    }
    const auto st = juce::PathStrokeType (s * 0.09f);
    g.setColour (juce::Colour (0xfff2c866).withAlpha (0.35f));
    g.strokePath (p, st, juce::AffineTransform::translation (1.0f, 1.0f));
    g.setColour (col);
    g.strokePath (p, st);
}

static void drawLotus (juce::Graphics& g, float cx, float cy, float s)
{
    for (int k = -2; k <= 2; ++k)
    {
        juce::Path p;
        p.addEllipse (-s * 0.12f, -s * 0.5f, s * 0.24f, s * 0.5f);
        p.applyTransform (juce::AffineTransform::rotation ((float) k * 0.45f).translated (cx, cy));
        g.setColour (juce::Colour (0xffe3a93f).withAlpha (0.85f));
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

    // night-blue stone body
    g.setGradientFill (juce::ColourGradient (neon::bg1, 0.0f, 0.0f, neon::bg0, 0.0f, (float) h, false));
    g.fillAll();

    // faint eight-point star lattice
    g.setColour (neon::amber.withAlpha (0.05f));
    for (int y = 0; y < h + 48; y += 48)
        for (int x = 0; x < w + 48; x += 48)
        {
            juce::Path s;
            s.addStar ({ (float) x, (float) y }, 8, 9.0f, 21.0f);
            g.strokePath (s, juce::PathStrokeType (1.0f));
        }

    // stone grain
    juce::Random rnd (7);
    for (int i = 0; i < 5000; ++i)
    {
        g.setColour ((i & 1 ? juce::Colours::white : juce::Colours::black).withAlpha (0.035f));
        g.fillRect (rnd.nextInt (w), rnd.nextInt (h), 1 + rnd.nextInt (2), 1);
    }

    // ---- header: dusk sky over the Nile + pyramids ----
    const int fx = 30, hy = 70, fh = 36;
    juce::Rectangle<float> sky ((float) fx, 0.0f, (float) (w - 2 * fx), (float) hy);
    juce::ColourGradient sg (juce::Colour (0xff0d1830), 0.0f, 0.0f, juce::Colour (0xffd0782c), 0.0f, (float) hy, false);
    sg.addColour (0.55, juce::Colour (0xff3a3a63));
    g.setGradientFill (sg);
    g.fillRect (sky);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb35c).withAlpha (0.45f), (float) w * 0.66f, (float) hy,
                                             juce::Colours::transparentBlack, (float) w * 0.66f, 4.0f, true));
    g.fillRect (sky);

    g.setColour (juce::Colour (0xff0a1020));
    for (int k = 0; k < 2; ++k)
    {
        const float px = (float) w * (0.70f + 0.07f * (float) k), sz = k == 0 ? 54.0f : 36.0f;
        juce::Path pyr;
        pyr.addTriangle (px - sz, (float) hy, px + sz, (float) hy, px, (float) hy - sz * 0.8f);
        g.fillPath (pyr);
    }
    for (int x = fx + 330; x < w - fx - 20; x += 11)             // skyline
        if (x < (int) ((float) w * 0.60f) || x > (int) ((float) w * 0.82f))
            g.fillRect (x, hy - 6 - (x * 7919 % 13), 7, 20);
    for (int k = 0; k < 3; ++k)                                  // palms
    {
        const float px = (float) fx + 380.0f + (float) k * 90.0f;
        g.drawLine (px, (float) hy, px + 2.0f, (float) hy - 26.0f, 2.0f);
        for (int f = -2; f <= 2; ++f)
            g.drawLine (px + 2.0f, (float) hy - 26.0f, px + 2.0f + (float) f * 7.0f, (float) hy - 26.0f + std::abs ((float) f) * 3.0f, 1.5f);
    }

    // ---- bronze carved frame: sides + footer ----
    auto bronze = [&] (juce::Rectangle<int> r, bool vertical)
    {
        juce::ColourGradient bg (juce::Colour (0xff6a3d10), (float) r.getX(), (float) r.getY(),
                                 juce::Colour (0xff6a3d10), vertical ? (float) r.getRight() : (float) r.getX(),
                                 vertical ? (float) r.getY() : (float) r.getBottom(), false);
        bg.addColour (0.5, juce::Colour (0xffc58a38));
        g.setGradientFill (bg);
        g.fillRect (r);
        for (int i = 0; i < r.getWidth() * r.getHeight() / 40; ++i)
        {
            g.setColour ((i & 1 ? juce::Colours::black : juce::Colours::white).withAlpha (0.06f));
            g.fillRect (r.getX() + rnd.nextInt (r.getWidth()), r.getY() + rnd.nextInt (r.getHeight()), 2, 1);
        }
    };

    bronze ({ 0, 0, fx, h }, true);
    bronze ({ w - fx, 0, fx, h }, true);
    bronze ({ fx, h - fh, w - 2 * fx, fh }, false);

    for (int side = 0; side < 2; ++side)
    {
        const float cx = side == 0 ? (float) fx * 0.5f : (float) w - (float) fx * 0.5f;
        for (int i = 0, y = 14; y < h - fh - 8; y += 36, ++i)
            drawGlyph (g, i + side * 3, cx, (float) y + 14.0f, 20.0f, juce::Colour (0xff2b1807).withAlpha (0.9f));
    }

    // footer: carved symbols + "Developed in Egypt" with lotus flowers
    for (int i = 0, x = fx + 14; x < w - fx - 14; x += 34, ++i)
        if (x < w / 2 - 120 || x > w / 2 + 120)
            drawGlyph (g, i, (float) x, (float) h - (float) fh * 0.5f, 18.0f, juce::Colour (0xff2b1807).withAlpha (0.9f));

    g.setColour (juce::Colour (0xff0b1018));
    g.fillRoundedRectangle ((float) w * 0.5f - 112.0f, (float) h - (float) fh + 4.0f, 224.0f, (float) fh - 8.0f, 8.0f);
    g.setColour (juce::Colour (0xffb8863a));
    g.drawRoundedRectangle ((float) w * 0.5f - 112.0f, (float) h - (float) fh + 4.0f, 224.0f, (float) fh - 8.0f, 8.0f, 1.2f);
    drawLotus (g, (float) w * 0.5f - 92.0f, (float) h - 8.0f, 20.0f);
    drawLotus (g, (float) w * 0.5f + 92.0f, (float) h - 8.0f, 20.0f);
    g.setColour (juce::Colour (0xfff2d88a));
    g.setFont (uiFont (12.0f, false));
    g.drawText ("Developed in Egypt", (int) ((float) w * 0.5f - 80.0f), h - fh + 4, 160, fh - 8, juce::Justification::centred, false);

    // inner bronze edge around the content area
    g.setColour (juce::Colour (0xffb8863a).withAlpha (0.9f));
    g.drawRect (fx, hy, w - 2 * fx, h - hy - fh, 2);
    g.setColour (juce::Colour (0xfff2c866).withAlpha (0.35f));
    g.drawRect (fx + 2, hy + 2, w - 2 * fx - 4, h - hy - fh - 4, 1);
}

void NayAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& title, juce::Colour accent)
{
    auto r = ri.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b2c4d).withAlpha (0.92f), 0.0f, r.getY(),
                                             juce::Colour (0xff0a1222).withAlpha (0.95f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (2.5f), 6.0f, 1.0f);
    g.setColour (juce::Colour (0xffb8863a).withAlpha (0.75f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.5f);

    g.setColour (accent);
    const float L = 11.0f;
    g.fillRect (r.getX(), r.getY(), L, 2.0f);                 g.fillRect (r.getX(), r.getY(), 2.0f, L);
    g.fillRect (r.getRight() - L, r.getY(), L, 2.0f);         g.fillRect (r.getRight() - 2.0f, r.getY(), 2.0f, L);
    g.fillRect (r.getX(), r.getBottom() - 2.0f, L, 2.0f);     g.fillRect (r.getX(), r.getBottom() - L, 2.0f, L);
    g.fillRect (r.getRight() - L, r.getBottom() - 2.0f, L, 2.0f); g.fillRect (r.getRight() - 2.0f, r.getBottom() - L, 2.0f, L);

    const auto tf = uiFont (11.5f).withExtraKerningFactor (0.08f);
    juce::GlyphArrangement ga;
    ga.addLineOfText (tf, title, 0.0f, 0.0f);
    const float tw = juce::jmin ((float) ri.getWidth() - 28.0f, ga.getBoundingBox (0, -1, true).getWidth() + 18.0f);
    juce::Rectangle<float> plate ((float) ri.getX() + 10.0f, (float) ri.getY() + 3.0f, tw, 19.0f);
    g.setColour (juce::Colour (0xff070c16));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (juce::Colour (0xffb8863a));
    g.drawRoundedRectangle (plate, 5.0f, 1.2f);
    g.setFont (tf);
    g.setColour (juce::Colour (0xfff2d88a));
    g.drawText (title, plate.toNearestInt(), juce::Justification::centred, true);
}

void NayAudioProcessorEditor::drawLcd (juce::Graphics& g, juce::Rectangle<int> ri, const juce::String& tag,
                                       const juce::String& name, const juce::String& sub, juce::Colour accent)
{
    auto r = ri.toFloat();
    g.setColour (juce::Colour (0xff070c16));
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

    drawAnkh (g, 52.0f, 12.0f, 40.0f, neon::amber);

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
        g.setColour (juce::Colour (0xff070c16));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (neon::amber.withAlpha (0.45f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

        const float age = (float) (juce::Time::getMillisecondCounter() - lastMonMs);
        const float act = juce::jlimit (0.0f, 1.0f, 1.0f - age / 400.0f);
        g.setColour (neon::cyan.withAlpha (0.25f + 0.75f * act));
        g.fillEllipse (r.getX() + 10.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);

        g.setColour (k == 0 ? neon::dim : neon::text);
        g.setFont (uiFont (12.5f, false));
        g.drawText (t, midiRect.withTrimmedLeft (28), juce::Justification::centredLeft, false);
    }

    auto f = juce::Font (juce::FontOptions ("Georgia", 30.0f, juce::Font::bold)).withExtraKerningFactor (0.1f);
    g.setFont (f);
    for (int d = 3; d >= 1; --d)
    {
        g.setColour (neon::amber.withAlpha (0.07f));
        g.drawText ("EGYPTIAN SAMPLER", 92 - d, 8 - d, 520 + 2 * d, 36 + 2 * d, juce::Justification::centredLeft, false);
    }
    g.setColour (neon::amber.brighter (0.3f));
    g.drawText ("EGYPTIAN SAMPLER", 92, 8, 520, 36, juce::Justification::centredLeft, false);

    g.setFont (uiFont (10.5f).withExtraKerningFactor (0.25f));
    g.setColour (neon::cyan);
    g.drawText ("MAQAM ENGINE  //  MICROTONAL SAMPLER", 94, 42, 520, 14, juce::Justification::centredLeft, false);

    g.setFont (juce::Font (juce::FontOptions ("Segoe UI", 22.0f, juce::Font::bold)));
    g.setColour (neon::amber);
    g.drawText (juce::String (L"سامبلر مصري"), 370, 12, 260, 36, juce::Justification::centredRight, false);
}

void NayAudioProcessorEditor::resized()
{
    if (! built)
        return;

    rebuildBackground();

    mapButton.setBounds (872, 16, 142, 32);
    browseButton.setBounds (640, 16, 110, 32);
    mixerButton.setBounds (756, 16, 110, 32);
    midiRect = { 46, 674, 968, 24 };
    pageRect = { 46, 76, 968, 592 };
    page.setBounds (pageRect);
    settings.setBounds (pageRect);
    browse.setBounds (pageRect);
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
    tMono->setBounds (748, pVoice.getY() + 22, 150, 24);
    tLegato->setBounds (748, pVoice.getY() + 48, 150, 24);
    tVel->setBounds (748, pVoice.getY() + 74, 150, 24);

    pMaqam = { 0, 204, 968, 150 };
    trBox.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 46, 108, 34);
    ocBox.setBounds (pMaqam.getX() + 130, pMaqam.getY() + 46, 108, 34);
    resetKeysBtn.setBounds (pMaqam.getX() + 14, pMaqam.getY() + 100, 224, 36);
    static const float px[12] = { 0, 0.5f, 1, 1.5f, 2, 3, 3.5f, 4, 4.5f, 5, 5.5f, 6 };
    for (int i = 0; i < 12; ++i)
    {
        const bool sharp = i == 1 || i == 3 || i == 6 || i == 8 || i == 10;
        keys[(size_t) i]->setBounds (250 + (int) (px[i] * 100.0f) + 15, pMaqam.getY() + (sharp ? 20 : 84), 70, 62);
    }

    const int counts[5] = { 2, 2, 3, 3, 2 };
    int sum = 0;
    for (int c : counts) sum += c * 72 + 14;
    const int gap = (968 - sum) / 4;
    const int fy = 362, fh = 132;

    juce::Rectangle<int>* fx[5] = { &pSat, &pWah, &pPh, &pDelay, &pReverb };
    int x = 0;
    for (int i = 0; i < 5; ++i)
    {
        const int w = counts[i] * 72 + 14;
        *fx[i] = { x, fy, w, fh };
        x += w + gap;
    }

    auto knobArea = [&] (const juce::Rectangle<int>& p, int n) { return juce::Rectangle<int> (p.getX() + 7, p.getY() + 22, n * 72, 80); };
    layoutRow (knobArea (pSat, 2),    { kSatDrive, kSatMix });
    layoutRow (knobArea (pWah, 2),    { kWahPos, kWahMix });
    layoutRow (knobArea (pPh, 3),     { kPhRate, kPhDepth, kPhMix });
    layoutRow (knobArea (pDelay, 3),  { kDlTime, kDlFb, kDlLvl });
    layoutRow (knobArea (pReverb, 2), { kRvSize, kRvLvl });

    auto togArea = [] (const juce::Rectangle<int>& p) { return juce::Rectangle<int> (p.getX() + 8, p.getBottom() - 28, p.getWidth() - 16, 22); };
    tSat->setBounds (togArea (pSat));
    tWah->setBounds (togArea (pWah));
    tPh->setBounds (togArea (pPh));

    // ================= settings page (local coords) =================
    lcdA2 = { 0, 0, 600, 60 };
    prevA2.setBounds (lcdA2.getX() + 8, 16, 20, 28);
    nextA2.setBounds (lcdA2.getRight() - 28, 16, 20, 28);
    addButton.setBounds (616, 0, 150, 28);
    renameButton.setBounds (616, 32, 150, 28);
    info.setBounds (776, 0, 190, 60);

    mapView.setBounds (0, 68, 968, 524);

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

    // ================= mixer page =================
    pMixMain   = { 0, 0, 300, 316 };
    pMixDuo    = { 310, 0, 300, 316 };
    pMixMaster = { 620, 0, 348, 316 };
    layoutRow ({ pMixMain.getX() + 8, 56, 284, 190 }, { kMainLvl, kMainPan });
    layoutRow ({ pMixDuo.getX() + 8, 56, 284, 190 }, { kDuoLvl, kDuoPan });
    layoutRow ({ pMixMaster.getX() + 8, 56, 332, 190 }, { kMaster });
    tMainMute->setBounds (pMixMain.getX() + 20, 266, 120, 26);
    tDuoMute->setBounds (pMixDuo.getX() + 20, 266, 120, 26);
}
