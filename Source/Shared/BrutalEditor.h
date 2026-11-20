#pragma once
// NF Brutal editor. Shared (identical file) by the NF Brutal plug-in and the native insert inside Mixa Studio. Everything is laid out in "panel units" (810 x 270 = default window) and scaled to the window.
// Layers (strict, see NF UI rule): 1) chassis = static panel painted in BrutalEditor::paint (no controls baked in);
// 2) knob bodies = static caps drawn by drawKnobCap(); 3) dynamic parts (value arc, pointer, LEDs, meter, value texts) are separate
// and recomputed from the parameters on every repaint.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "BrutalCore.h"

namespace nfbrutalui
{
using juce::Colour;
using juce::Graphics;
inline constexpr float kW = 810.0f, kH = 270.0f;
inline constexpr int kDefaultWidth = 810;
inline const Colour kOrange(0xffff5a1f), kOrangeHot(0xffff9a3c), kRed(0xffe02a1a), kInk(0xff0b0c0f), kText(0xffe9e6df), kDim(0xff8b8780);

inline juce::Font uiFont(float h, bool bold = false, float kern = 0.0f)
{
    return juce::Font(juce::FontOptions(h).withStyle(bold ? "Bold" : "Regular").withKerningFactor(kern));
}
inline void centredText(Graphics& g, const juce::String& t, float cx, float cy, const juce::Font& f)
{
    g.setFont(f); g.drawText(t, juce::Rectangle<float>(cx - 70.0f, cy - 9.0f, 140.0f, 18.0f), juce::Justification::centred, false);
}

// ---- layer 2: static knob body (disc + rim only; no pointer, no arc) ----
inline void drawKnobCap(Graphics& g, float cx, float cy, float r)
{
    juce::Path sp; sp.addEllipse(cx - r, cy - r, 2 * r, 2 * r);
    juce::DropShadow(Colour(0xd0000000), (int) (r * 0.7f), { (int) (r * 0.10f), (int) (r * 0.26f) }).drawForPath(g, sp);
    g.setGradientFill(juce::ColourGradient(Colour(0xff6a6d74), cx - r * 0.7f, cy - r * 0.8f, Colour(0xff17181c), cx + r * 0.7f, cy + r * 0.9f, false));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);                                         // rim
    for (int i = 0; i < 48; ++i)                                                       // knurled edge
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 48.0f, s = std::sin(a), c = -std::cos(a);
        g.setColour(Colour(0x55000000)); g.drawLine(cx + s * r * 0.93f, cy + c * r * 0.93f, cx + s * r * 0.99f, cy + c * r * 0.99f, juce::jmax(0.8f, r * 0.03f));
    }
    const float ri = r * 0.80f;
    g.setGradientFill(juce::ColourGradient(Colour(0xff3b3d44), cx - ri * 0.6f, cy - ri * 0.8f, Colour(0xff101114), cx + ri * 0.5f, cy + ri * 0.9f, false));
    g.fillEllipse(cx - ri, cy - ri, 2 * ri, 2 * ri);                                     // cap
    g.setColour(Colour(0x30ffffff)); g.drawEllipse(cx - ri, cy - ri, 2 * ri, 2 * ri, juce::jmax(0.8f, r * 0.025f));
    g.setGradientFill(juce::ColourGradient(Colour(0x22ffffff), cx, cy - ri, Colour(0x00ffffff), cx, cy, false));   // top sheen
    g.fillEllipse(cx - ri * 0.86f, cy - ri * 0.92f, ri * 1.72f, ri * 1.0f);
}

// ---- layer 3: dynamic parts of a knob, from the value (0..1) ----
inline void drawKnobDynamic(Graphics& g, float cx, float cy, float r, float prop, bool hero)
{
    constexpr float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
    const float ang = a0 + prop * (a1 - a0), ra = r + juce::jmax(5.0f, r * 0.22f), th = hero ? 4.2f : 3.2f;
    juce::Path track; track.addCentredArc(cx, cy, ra, ra, 0.0f, a0, a1, true);
    g.setColour(Colour(0xff2b2220)); g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (prop > 0.002f)
    {
        juce::Path val; val.addCentredArc(cx, cy, ra, ra, 0.0f, a0, ang, true);
        g.setColour(Colour(0x40ff5a1f)); g.strokePath(val, juce::PathStrokeType(th + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));   // glow
        g.setColour(kOrange); g.strokePath(val, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float s = std::sin(ang), c = -std::cos(ang), r0 = r * 0.30f, r1 = r * 0.74f;
    g.setColour(Colour(0xff0a0a0c)); g.drawLine(cx + s * r0, cy + c * r0, cx + s * r1, cy + c * r1, hero ? 5.2f : 4.2f);
    g.setColour(kOrangeHot); g.drawLine(cx + s * r0, cy + c * r0, cx + s * r1, cy + c * r1, hero ? 2.8f : 2.2f);
}

// Rotary knob bound to a parameter. The slider works on the normalised 0..1 position, so the Gain taper is respected.
class ParamKnob final : public juce::Slider
{
public:
    ParamKnob(juce::RangedAudioParameter& p, float radiusUnits, bool heroKnob)
        : param(p), r(radiusUnits), hero(heroKnob),
          attach(p, [this](float v) { setValue(param.convertTo0to1(v), juce::dontSendNotification); repaint(); if (onChanged) onChanged(); })
    {
        setSliderStyle(juce::Slider::RotaryVerticalDrag); setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        setRange(0.0, 1.0, 0.0); setMouseDragSensitivity(240); setPopupMenuEnabled(false);
        setDoubleClickReturnValue(true, param.getDefaultValue());
        onDragStart = [this] { attach.beginGesture(); };
        onDragEnd = [this] { attach.endGesture(); };
        onValueChange = [this] { attach.setValueAsPartOfGesture(param.convertFrom0to1((float) getValue())); if (onChanged) onChanged(); };
        attach.sendInitialUpdate();
    }
    juce::String getTextFromValue(double v) override { return param.getText((float) v, 32); }
    void setScale(float s) { scale = s; }
    void paint(Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        drawKnobCap(g, c.x, c.y, r * scale);                                          // static layer
        drawKnobDynamic(g, c.x, c.y, r * scale, (float) getValue(), hero);             // dynamic layer
    }
    std::function<void()> onChanged;
    juce::RangedAudioParameter& param;
    float r;
private:
    bool hero; float scale = 1.0f;
    juce::ParameterAttachment attach;
};

// Square LED push button (Lofi) and round power button.
class LedButton final : public juce::ToggleButton
{
public:
    explicit LedButton(bool roundButton) : round(roundButton) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paintButton(Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        const bool on = getToggleState();
        if (round)
        {
            const auto c = b.getCentre(); const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
            if (on) { g.setColour(Colour(0x40ff5a1f)); g.fillEllipse(c.x - r - 2, c.y - r - 2, 2 * r + 4, 2 * r + 4); }
            g.setGradientFill(juce::ColourGradient(Colour(0xff4a4c52), c.x, c.y - r, Colour(0xff121317), c.x, c.y + r, false)); g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
            g.setColour(on ? kOrange : Colour(0xff55575e)); g.drawEllipse(c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f, juce::jmax(1.6f, r * 0.14f));
            g.setColour(Colour(0xff121317)); g.fillRect(c.x - r * 0.12f, c.y - r * 0.80f, r * 0.24f, r * 0.62f);
            g.setColour(on ? kOrangeHot : Colour(0xff55575e)); g.fillRoundedRectangle(c.x - r * 0.07f, c.y - r * 0.78f, r * 0.14f, r * 0.58f, r * 0.05f);
        }
        else
        {
            g.setGradientFill(juce::ColourGradient(on ? Colour(0xff3a2a22) : Colour(0xff2c2d33), b.getX(), b.getY(), Colour(0xff121317), b.getX(), b.getBottom(), false));
            g.fillRoundedRectangle(b, 5.0f);
            g.setColour(on ? kOrange : (over ? Colour(0xff77797f) : Colour(0xff4a4c52))); g.drawRoundedRectangle(b, 5.0f, 1.3f);
            const float led = juce::jmin(b.getHeight() * 0.34f, 9.0f), lx = b.getX() + b.getWidth() * 0.5f, ly = b.getY() + b.getHeight() * 0.32f;
            if (on) { g.setColour(Colour(0x55ff5a1f)); g.fillEllipse(lx - led, ly - led, 2 * led, 2 * led); }
            g.setColour(on ? kOrangeHot : Colour(0xff3b3d44)); g.fillEllipse(lx - led * 0.5f, ly - led * 0.5f, led, led);
        }
    }
private:
    bool round;
};

// Gain-reduction LED ladder: 0 .. 30 dB, 30 segments.
class GrMeter final : public juce::Component, private juce::Timer
{
public:
    explicit GrMeter(std::atomic<float>& src) : source(src) { setInterceptsMouseClicks(false, false); shown = source.load(); startTimerHz(40); }
    void paint(Graphics& g) override
    {
        const int n = 30; const float gap = 2.0f, w = ((float) getWidth() - gap * (float) (n - 1)) / (float) n, h = (float) getHeight();
        for (int i = 0; i < n; ++i)
        {
            const float x = (float) i * (w + gap); const bool lit = shown > (float) i + 0.15f, hot = i >= 24;
            const Colour on = hot ? kRed : (i >= 16 ? kOrangeHot : kOrange);
            if (lit) { g.setColour(on.withAlpha(0.30f)); g.fillRoundedRectangle(x - 1.5f, -1.5f, w + 3.0f, h + 3.0f, 2.5f); }
            g.setColour(lit ? on : Colour(0xff2a2220)); g.fillRoundedRectangle(x, 0.0f, w, h, 1.8f);
            if (lit) { g.setColour(Colour(0x55ffffff)); g.fillRect(x + 0.8f, 1.2f, w - 1.6f, 1.6f); }
        }
    }
private:
    void timerCallback() override
    {
        const float t = source.load();
        const float next = t > shown ? t : juce::jmax(t, shown - 1.6f);          // instant rise, ~40 dB/s fall (display only)
        if (std::abs(next - shown) > 0.02f) { shown = next; repaint(); }
    }
    std::atomic<float>& source; float shown = 0.0f;
};

class MenuButton final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onClick;
    MenuButton() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat(); const float lineH = juce::jmax(1.4f, b.getHeight() * 0.12f);
        g.setColour(kText);
        for (int i = 0; i < 3; ++i) g.fillRoundedRectangle(b.getX(), b.getY() + b.getHeight() * (0.10f + (float) i * 0.35f), b.getWidth(), lineH, lineH * 0.5f);
    }
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
};

class LogoButton final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onClick;
    LogoButton() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (onClick) onClick(); }
};

// [<]  preset name  [>]  (157 x 21): arrows step through the saved presets, a click on the name opens the list.
class PresetBar final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onPrev, onNext, onMenu;
    PresetBar() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void setPresetName(const juce::String& n) { if (n != name) { name = n; repaint(); } }
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(Colour(0xff0e0f12)); g.fillRoundedRectangle(b, 4.0f);
        g.setColour(Colour(0xff4a4c52)); g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
        const float h = b.getHeight(), aw = h * 1.0f;
        g.setColour(kOrange);
        juce::Path l, r;
        l.addTriangle(aw * 0.62f, h * 0.28f, aw * 0.62f, h * 0.72f, aw * 0.30f, h * 0.5f);
        r.addTriangle(b.getRight() - aw * 0.62f, h * 0.28f, b.getRight() - aw * 0.62f, h * 0.72f, b.getRight() - aw * 0.30f, h * 0.5f);
        g.fillPath(l); g.fillPath(r);
        g.setColour(kText); g.setFont(uiFont(h * 0.56f));
        g.drawText(name, juce::Rectangle<float>(aw, 0.0f, b.getWidth() - 2.0f * aw, h), juce::Justification::centred, true);
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!contains(e.getPosition())) return;
        const float aw = (float) getHeight();
        if ((float) e.x < aw) { if (onPrev) onPrev(); }
        else if ((float) e.x > (float) getWidth() - aw) { if (onNext) onNext(); }
        else if (onMenu) onMenu();
    }
private:
    juce::String name { "Default" };
};

// ------------------------------------------------------------------------------------------------------------------------------
class BrutalEditor final : public juce::AudioProcessorEditor, private juce::ValueTree::Listener, private juce::AsyncUpdater
{
public:
    BrutalEditor(juce::AudioProcessor& owner, nfbrutal::BrutalCore& core, juce::Image logoImage);
    ~BrutalEditor() override { proc.apvts.state.removeListener(this); }
    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void resized() override;
    // Optional cover component (the plug-in puts its licence activation window here; the native Mixa insert uses none). Added above everything.
    void installOverlay(std::unique_ptr<juce::Component> c) { overlay = std::move(c); addChildComponent(*overlay); overlay->setBounds(getLocalBounds()); overlay->toFront(false); }

private:
    struct KnobSpec { ParamKnob* k; const char* label; float cx, cy; };
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& id) override { if (id.toString() == "presetName") triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { presetBar.setPresetName(presetName()); }
    juce::String presetName() const { return proc.apvts.state.getProperty("presetName", "Default").toString(); }
    juce::Rectangle<int> scaled(float x, float y, float w, float h) const { return juce::Rectangle<float>(x * layoutScale, y * layoutScale, w * layoutScale, h * layoutScale).toNearestInt(); }
    static juce::File presetsDir() { auto d = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NF Audio Tools").getChildFile("NF Brutal").getChildFile("Presets"); d.createDirectory(); return d; }
    juce::Array<juce::File> savedPresets() const { juce::Array<juce::File> f; presetsDir().findChildFiles(f, juce::File::findFiles, false, "*.xml"); f.sort(); return f; }
    void stepPreset(int dir);
    void showPresetMenu();
    void showMainMenu();
    void savePreset();
    void loadPresetFile(const juce::File& f);
    void restoreDefault();

    juce::AudioProcessor& owner;
    nfbrutal::BrutalCore& proc;
    float layoutScale = 1.0f;
    juce::Image logo;
    juce::TooltipWindow tooltipWindow { this, 500 };
    ParamKnob input, slay, gain, body, mix;
    LedButton lofiBtn { false }, powerBtn { true };
    juce::ButtonParameterAttachment lofiAtt, powerAtt;
    GrMeter meter;
    MenuButton menuButton; PresetBar presetBar; LogoButton logoButton;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ComponentBoundsConstrainer constrainer;
    std::vector<KnobSpec> specs;
    std::unique_ptr<juce::Component> overlay;
};

inline BrutalEditor::BrutalEditor(juce::AudioProcessor& ownerProc, nfbrutal::BrutalCore& p, juce::Image logoImage)
    : juce::AudioProcessorEditor(ownerProc), owner(ownerProc), proc(p),
      input(*p.apvts.getParameter("input"), 30.0f, false), slay(*p.apvts.getParameter("slay"), 44.0f, true), gain(*p.apvts.getParameter("gain"), 30.0f, false),
      body(*p.apvts.getParameter("body"), 30.0f, false), mix(*p.apvts.getParameter("mix"), 30.0f, false),
      lofiAtt(*p.apvts.getParameter("lofi"), lofiBtn), powerAtt(*p.apvts.getParameter("power"), powerBtn),
      meter(p.grDb)
{
    logo = logoImage;
    specs = { { &input, "INPUT", 250.0f, 166.0f }, { &slay, "SLAY", 372.0f, 160.0f }, { &gain, "GAIN", 498.0f, 166.0f }, { &body, "BODY", 590.0f, 166.0f }, { &mix, "MIX", 682.0f, 166.0f } };
    for (auto& s : specs) { addAndMakeVisible(*s.k); s.k->onChanged = [this] { repaint(); }; }
    addAndMakeVisible(lofiBtn); addAndMakeVisible(powerBtn);
    lofiBtn.setTooltip("Lofi: narrow band-pass"); powerBtn.setTooltip("On / off (clickless bypass)");
    addAndMakeVisible(meter); addAndMakeVisible(menuButton); addAndMakeVisible(presetBar); addAndMakeVisible(logoButton);
    menuButton.setTooltip("Menu"); logoButton.setTooltip("Click: back to the normal size"); presetBar.setTooltip("Presets");
    logoButton.onClick = [this] { setSize(kDefaultWidth, (int) std::lround(kDefaultWidth * kH / kW)); };
    menuButton.onClick = [this] { showMainMenu(); };
    presetBar.onPrev = [this] { stepPreset(-1); }; presetBar.onNext = [this] { stepPreset(1); }; presetBar.onMenu = [this] { showPresetMenu(); };
    presetBar.setPresetName(presetName());
    proc.apvts.state.addListener(this);
    const int savedW = juce::jlimit(405, 1620, (int) proc.apvts.state.getProperty("uiWidth", kDefaultWidth));
    constrainer.setFixedAspectRatio((double) kW / (double) kH); constrainer.setSizeLimits(405, 135, 1620, 540);
    setConstrainer(&constrainer); setResizable(true, true);
    setSize(savedW, (int) std::lround(savedW * kH / kW));
}

inline void BrutalEditor::resized()
{
    layoutScale = (float) getWidth() / kW;
    for (auto& s : specs)
    {
        const float m = s.k->r * 1.45f; s.k->setScale(layoutScale);
        s.k->setBounds(scaled(s.cx - m, s.cy - m, 2 * m, 2 * m));
    }
    meter.setBounds(scaled(205.0f, 46.0f, 440.0f, 22.0f));
    lofiBtn.setBounds(scaled(697.0f, 42.0f, 64.0f, 34.0f));
    powerBtn.setBounds(scaled(719.0f, 168.0f, 56.0f, 56.0f));
    logoButton.setBounds(scaled(26.0f, 18.0f, 126.0f, 72.0f));
    presetBar.setBounds(scaled(607.0f, 7.0f, 157.0f, 21.0f));
    menuButton.setBounds(scaled(774.0f, 9.0f, 20.0f, 16.0f));
    if (overlay != nullptr) overlay->setBounds(getLocalBounds());
    if (getWidth() > 0) proc.apvts.state.setProperty("uiWidth", getWidth(), nullptr);
}

// ---- layer 1: the chassis (static panel; no controls) ----
inline void BrutalEditor::paint(Graphics& g)
{
    g.addTransform(juce::AffineTransform::scale(layoutScale));
    const juce::Rectangle<float> panel(0.0f, 0.0f, kW, kH);
    g.setGradientFill(juce::ColourGradient(Colour(0xff25262b), 0.0f, 0.0f, Colour(0xff0d0e11), 0.0f, kH, false)); g.fillRect(panel);
    juce::Random rnd(7);
    for (int i = 0; i < 260; ++i)                                   // brushed grain
    {
        const float y = rnd.nextFloat() * kH, x = rnd.nextFloat() * kW, len = 40.0f + rnd.nextFloat() * 300.0f;
        g.setColour(rnd.nextBool() ? Colour(0x08ffffff) : Colour(0x0c000000)); g.drawLine(x, y, x + len, y, 0.6f);
    }
    g.setGradientFill(juce::ColourGradient(Colour(0x00000000), kW * 0.5f, kH * 0.55f, Colour(0x70000000), 0.0f, 0.0f, true)); g.fillRect(panel);   // vignette
    g.setColour(Colour(0xff050506)); g.drawRect(panel, 2.0f);
    g.setGradientFill(juce::ColourGradient(kOrangeHot, 0.0f, 0.0f, kRed, kW, 0.0f, false)); g.fillRect(0.0f, 0.0f, kW, 3.0f);                      // top accent
    g.setColour(Colour(0x40ffffff)); g.drawLine(2.0f, 3.5f, kW - 2.0f, 3.5f, 0.8f);
    for (auto p : { juce::Point<float>(14, 16), { 14, 254 }, { 796, 254 } })                                                          // screws
    {
        g.setGradientFill(juce::ColourGradient(Colour(0xff6c6e75), p.x - 3, p.y - 3, Colour(0xff17181c), p.x + 3, p.y + 3, false)); g.fillEllipse(p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);
        g.setColour(Colour(0xff0a0a0c)); g.drawLine(p.x - 2.6f, p.y + 1.4f, p.x + 2.6f, p.y - 1.4f, 1.3f);
    }
    // recessed windows
    auto well = [&](juce::Rectangle<float> r, float rad) { g.setColour(Colour(0xff08090b)); g.fillRoundedRectangle(r, rad); g.setColour(Colour(0xff34363c)); g.drawRoundedRectangle(r.reduced(0.5f), rad, 1.0f); };
    well({ 196.0f, 30.0f, 458.0f, 64.0f }, 6.0f);
    // logo + name
    if (logo.isValid()) { g.setOpacity(1.0f); g.setImageResamplingQuality(Graphics::highResamplingQuality); g.drawImage(logo, juce::Rectangle<float>(26.0f, 18.0f, 126.0f, 72.0f), juce::RectanglePlacement::centred); }
    g.setGradientFill(juce::ColourGradient(kOrangeHot, 0.0f, 108.0f, kRed, 0.0f, 140.0f, false));
    g.setFont(uiFont(33.0f, true, 0.05f)); g.drawText("BRUTAL", juce::Rectangle<float>(20.0f, 106.0f, 184.0f, 40.0f), juce::Justification::centredLeft, false);
    g.setColour(kDim); g.setFont(uiFont(10.0f, false, 0.22f)); g.drawText("VOCAL COMPRESSOR", juce::Rectangle<float>(22.0f, 146.0f, 150.0f, 14.0f), juce::Justification::centredLeft, false);
    // meter legend
    g.setColour(kDim); g.setFont(uiFont(8.6f, false, 0.12f));
    g.drawText("GAIN REDUCTION", juce::Rectangle<float>(204.0f, 31.0f, 160.0f, 12.0f), juce::Justification::centredLeft, false);
    {
        const float x0 = 205.0f, step = (440.0f + 2.0f) / 30.0f;
        for (int d : { 0, 5, 10, 15, 20, 25, 30 })
            centredText(g, juce::String(d), x0 + (float) d * step - (d == 30 ? 4.0f : 0.0f) + (d == 0 ? 4.0f : 0.0f), 80.0f, uiFont(8.4f));
        g.drawText("dB", juce::Rectangle<float>(612.0f, 31.0f, 38.0f, 12.0f), juce::Justification::centredRight, false);
    }
    // knob labels (static); the values are dynamic and drawn in paintOverChildren
    g.setColour(kText);
    for (auto& s : specs) centredText(g, s.label, s.cx, 218.0f, uiFont(s.k == &slay ? 11.0f : 10.0f, true, 0.14f));
    centredText(g, "LOFI", 729.0f, 86.0f, uiFont(9.0f, true, 0.14f));
    centredText(g, "ON", 747.0f, 232.0f, uiFont(9.0f, true, 0.14f));
}

inline void BrutalEditor::paintOverChildren(Graphics& g)
{
    if (overlay != nullptr && overlay->isVisible()) return;   // the licence window covers everything, including the value texts
    g.addTransform(juce::AffineTransform::scale(layoutScale));
    g.setColour(kOrangeHot);
    for (auto& s : specs)
        centredText(g, s.k->getTextFromValue(s.k->getValue()), s.cx, 236.0f, uiFont(10.5f, true));
}

// ---- menus / presets ----
inline void BrutalEditor::stepPreset(int dir)
{
    auto list = savedPresets(); if (list.isEmpty()) return;
    int idx = -1; for (int i = 0; i < list.size(); ++i) if (list[i].getFileNameWithoutExtension() == presetName()) idx = i;
    idx = idx < 0 ? (dir > 0 ? 0 : list.size() - 1) : (idx + dir + list.size()) % list.size();
    loadPresetFile(list[idx]);
}
inline void BrutalEditor::showPresetMenu()
{
    juce::PopupMenu m; auto saved = savedPresets();
    for (int i = 0; i < saved.size() && i < 200; ++i) m.addItem(2000 + i, saved[i].getFileNameWithoutExtension(), true, saved[i].getFileNameWithoutExtension() == presetName());
    if (!saved.isEmpty()) m.addSeparator();
    m.addItem(1000, "Save preset..."); m.addItem(1001, "Load preset..."); m.addItem(1002, "Restore default");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBar), [this, saved](int r)
    {
        if (r == 1000) savePreset();
        else if (r == 1001) { chooser = std::make_unique<juce::FileChooser>("Load preset", presetsDir(), "*.xml");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) loadPresetFile(fc.getResult()); }); }
        else if (r == 1002) restoreDefault();
        else if (r >= 2000 && r - 2000 < saved.size()) loadPresetFile(saved[r - 2000]);
    });
}
inline void BrutalEditor::savePreset()
{
    chooser = std::make_unique<juce::FileChooser>("Save preset", presetsDir().getChildFile("My preset.xml"), "*.xml");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc)
    {
        auto f = fc.getResult(); if (f == juce::File()) return; f = f.withFileExtension("xml");
        proc.apvts.state.setProperty("presetName", f.getFileNameWithoutExtension(), nullptr);
        if (auto xml = proc.apvts.copyState().createXml()) { xml->setAttribute("nfbrutalPreset", 1); xml->writeTo(f); }
    });
}
inline void BrutalEditor::loadPresetFile(const juce::File& f)
{
    auto xml = juce::XmlDocument::parse(f);
    if (xml == nullptr || xml->getIntAttribute("nfbrutalPreset", 0) != 1 || !xml->hasTagName(proc.apvts.state.getType())) { juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NF Brutal", "Invalid NF Brutal preset"); return; }
    const auto w = proc.apvts.state.getProperty("uiWidth");
    proc.apvts.replaceState(juce::ValueTree::fromXml(*xml));
    proc.apvts.state.setProperty("presetName", f.getFileNameWithoutExtension(), nullptr); proc.apvts.state.setProperty("uiWidth", w, nullptr);
}
inline void BrutalEditor::restoreDefault()
{
    for (auto* p : owner.getParameters()) p->setValueNotifyingHost(p->getDefaultValue());
    proc.apvts.state.setProperty("presetName", "Default", nullptr);
}
inline void BrutalEditor::showMainMenu()
{
    juce::PopupMenu m; m.addItem(1, "About NF Brutal");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton), [](int r)
    {
        if (r == 1) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "NF Brutal 0.1.0",
            "Vocal slayer: a very fast limiting compressor with make-up gain and saturation.\n\nInput, Slay (amount / hardness), Gain, Body (low-end cut), Mix, Lofi and On.\n\nNF Audio Tools by Nenno Fernando.");
    });
}
} // namespace nfbrutalui
