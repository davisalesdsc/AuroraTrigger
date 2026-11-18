#include "PluginEditor.h"
#include <cstdlib>

using namespace AuroraColours;

namespace
{
    constexpr int kEditorW        = 1040;
    constexpr int kEditorHFull    = 640;
    constexpr int kEditorHCompact = 480;

    void drawShadowedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r,
                           float size, juce::Colour col = juce::Colour (0xfff1f3f6),
                           juce::Justification j = juce::Justification::centred, float kerning = 0.0f)
    {
        juce::Font f (juce::FontOptions (size, juce::Font::bold));
        if (kerning != 0.0f)
            f = f.withExtraKerningFactor (kerning);
        g.setFont (f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawText (text, r.translated (0, 1), j, false);
        g.setColour (col);
        g.drawText (text, r, j, false);
    }

    void drawBevelPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bottom,
                         float corner, juce::Colour border = juce::Colour (0xff25282e))
    {
        g.setGradientFill (juce::ColourGradient (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (r.reduced (1.0f), corner - 1.0f, 1.0f);
        g.setColour (border);
        g.drawRoundedRectangle (r, corner, 1.2f);
    }

    juce::String formatValue (double v, int decimals, const juce::String& suffix)
    {
        return juce::String (v, decimals) + suffix;
    }

    // dB -> 0..1 meter position, evenly spaced printed marks: -48, -24, -12, -6, -3, 0
    float meterPosition (float db)
    {
        static constexpr float marks[] = { -48.0f, -24.0f, -12.0f, -6.0f, -3.0f, 0.0f };
        if (db <= marks[0]) return 0.0f;
        if (db >= marks[5]) return 1.0f;
        for (int i = 0; i < 5; ++i)
            if (db <= marks[i + 1])
                return (static_cast<float> (i) + (db - marks[i]) / (marks[i + 1] - marks[i])) / 5.0f;
        return 1.0f;
    }
}

//==============================================================================
// Metallic hardware LookAndFeel
//==============================================================================
AuroraMetallicLookAndFeel::AuroraMetallicLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff4a4d54));
    setColour (juce::TextButton::buttonColourId,   juce::Colour (0xffdfe2e8));
    setColour (juce::TextButton::buttonOnColourId, aurora);
    setColour (juce::TextButton::textColourOffId,  ink);
    setColour (juce::TextButton::textColourOnId,   juce::Colours::white);

    setColour (juce::Slider::textBoxBackgroundColourId, readoutBg);
    setColour (juce::Slider::textBoxTextColourId,       juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  aurora.withAlpha (0.5f));

    setColour (juce::Label::textColourId, juce::Colour (0xfff1f3f6));
}

juce::Font AuroraMetallicLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (12.5f, static_cast<float> (buttonHeight) * 0.42f), juce::Font::bold));
}

void AuroraMetallicLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                      const juce::Colour& backgroundColour,
                                                      bool isMouseOver, bool isButtonDown)
{
    auto b = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = juce::jmin (5.0f, b.getHeight() * 0.28f);

    auto base = backgroundColour;
    if (isButtonDown)      base = base.darker (0.18f);
    else if (isMouseOver)  base = base.brighter (0.08f);

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (0.0f, 1.2f), corner);

    g.setGradientFill (juce::ColourGradient (base.brighter (0.28f), b.getX(), b.getY(),
                                             base.darker (0.24f),   b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, corner);

    g.setColour (juce::Colours::white.withAlpha (isButtonDown ? 0.10f : 0.45f));
    g.drawRoundedRectangle (b.reduced (1.0f), corner - 1.0f, 1.0f);

    g.setColour (juce::Colour (0xff23262c).withAlpha (0.95f));
    g.drawRoundedRectangle (b, corner, 1.1f);
}

void AuroraMetallicLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                  bool isMouseOver, bool isButtonDown)
{
    const bool on = button.getToggleState();
    auto bg = button.findColour (on ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);

    drawButtonBackground (g, button, bg, isMouseOver, isButtonDown);

    if (on)   // soft "lit" glow
    {
        g.setColour (bg.withAlpha (0.35f));
        g.drawRoundedRectangle (button.getLocalBounds().toFloat().reduced (0.5f), 5.0f, 2.2f);
    }

    g.setColour (bg.getPerceivedBrightness() > 0.62f ? ink : juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::jmin (12.5f, static_cast<float> (button.getHeight()) * 0.46f), juce::Font::bold)));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1);
}

void AuroraMetallicLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    auto bounds = label.getLocalBounds().toFloat();
    auto bg = label.findColour (juce::Label::backgroundColourId);

    if (bg.getAlpha() > 0)
    {
        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 3.5f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.5f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (bounds.reduced (1.5f), 2.5f, 0.8f);
    }

    if (label.isBeingEdited())
        return;

    const float alpha = label.isEnabled() ? 1.0f : 0.5f;
    auto font = label.getFont();
    if (label.getHeight() <= 26)
        font = juce::Font (juce::FontOptions (juce::jmin (12.5f, static_cast<float> (label.getHeight()) * 0.80f), juce::Font::bold));

    g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
    g.setFont (font);
    auto area = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
    g.drawFittedText (label.getText(), area, label.getJustificationType(),
                      juce::jmax (1, static_cast<int> (static_cast<float> (area.getHeight()) / font.getHeight())),
                      label.getMinimumHorizontalScale());
}

void AuroraMetallicLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                  juce::Slider& slider)
{
    const float r  = static_cast<float> (juce::jmin (width, height)) * 0.5f - 2.0f;
    const float cx = static_cast<float> (x) + static_cast<float> (width)  * 0.5f;
    const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // cast shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (cx - r + 0.8f, cy - r + 2.4f, r * 2.0f, r * 2.0f);

    // dark bezel
    g.setColour (juce::Colour (0xff15171b));
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

    // brushed metal ring
    const float rr = r - 1.3f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3f5f8), cx, cy - rr,
                                             juce::Colour (0xff5f656f), cx, cy + rr, false));
    g.fillEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);

    // coloured cap
    const float br = r * 0.70f;
    g.setGradientFill (juce::ColourGradient (accent.brighter (0.65f), cx - br * 0.40f, cy - br * 0.55f,
                                             accent.darker (0.60f),   cx + br * 0.30f, cy + br * 1.00f, true));
    g.fillEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f, 1.0f);

    // gloss
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), cx, cy - br,
                                             juce::Colours::white.withAlpha (0.0f),  cx, cy + br * 0.1f, false));
    g.fillEllipse (cx - br * 0.72f, cy - br * 0.92f, br * 1.44f, br * 0.95f);

    // pointer on the ring
    juce::Path p;
    p.addRoundedRectangle (-1.3f, -rr + 1.0f, 2.6f, rr * 0.36f, 1.1f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
    g.setColour (juce::Colour (0xff101216));
    g.fillPath (p);
}

void AuroraMetallicLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float, float,
                                                  const juce::Slider::SliderStyle style, juce::Slider&)
{
    if (style != juce::Slider::LinearVertical)
        return;

    const float fx = static_cast<float> (x), fy = static_cast<float> (y);
    const float fw = static_cast<float> (width), fh = static_cast<float> (height);
    const float centreX = fx + fw * 0.5f;

    // slot
    g.setColour (juce::Colour (0xff15181d));
    g.fillRoundedRectangle (centreX - 2.5f, fy, 5.0f, fh, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (centreX - 2.5f, fy, 5.0f, fh, 2.5f, 0.8f);

    // scale ticks on both sides
    g.setColour (juce::Colour (0xff23272e));
    for (int t = 0; t <= 10; ++t)
    {
        const int ty = static_cast<int> (fy + fh * static_cast<float> (t) / 10.0f);
        const float len = (t % 5 == 0) ? 8.0f : 5.0f;
        g.drawHorizontalLine (ty, centreX - 4.0f - len, centreX - 4.0f);
        g.drawHorizontalLine (ty, centreX + 4.0f,       centreX + 4.0f + len);
    }

    // thumb
    const float capW = 28.0f, capH = 17.0f;
    const float capX = centreX - capW * 0.5f;
    const float capY = juce::jlimit (fy, fy + fh - capH, sliderPos - capH * 0.5f);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (capX, capY + 2.0f, capW, capH, 3.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f8fa), capX, capY,
                                             juce::Colour (0xff9097a3), capX, capY + capH, false));
    g.fillRoundedRectangle (capX, capY, capW, capH, 3.0f);
    g.setColour (juce::Colour (0xff14161a));
    g.drawRoundedRectangle (capX, capY, capW, capH, 3.0f, 1.1f);

    const float midY = capY + capH * 0.5f;
    g.setColour (juce::Colour (0xff5d636e));
    g.drawHorizontalLine (static_cast<int> (midY - 4.0f), capX + 3.0f, capX + capW - 3.0f);
    g.drawHorizontalLine (static_cast<int> (midY + 4.0f), capX + 3.0f, capX + capW - 3.0f);
    g.setColour (aurora);
    g.fillRect (capX + 2.0f, midY - 1.0f, capW - 4.0f, 2.0f);
}

//==============================================================================
// Segmented stereo LED meter (0 .. -48 dB)
//==============================================================================
void LedMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    constexpr float scaleW = 20.0f;
    auto barsArea = bounds.withTrimmedLeft (scaleW);

    g.setColour (readoutBg);
    g.fillRoundedRectangle (barsArea, 3.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (barsArea.reduced (0.5f), 3.0f, 1.0f);

    // dB scale
    static const char* labels[] = { "-0", "-3", "-6", "-12", "-24", "-48" };
    g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    const float usableTop = barsArea.getY() + 5.0f, usableH = barsArea.getHeight() - 10.0f;
    for (int i = 0; i < 6; ++i)
    {
        const float yc = usableTop + usableH * (static_cast<float> (i) / 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (labels[i], 0, static_cast<int> (yc - 5.0f) + 1, static_cast<int> (scaleW) - 2, 10, juce::Justification::centredRight, false);
        g.setColour (juce::Colour (0xffe4e8ee));
        g.drawText (labels[i], 0, static_cast<int> (yc - 5.0f), static_cast<int> (scaleW) - 2, 10, juce::Justification::centredRight, false);
    }

    constexpr int segments = 24;
    const float segH = usableH / static_cast<float> (segments);
    const float barW = (barsArea.getWidth() - 9.0f) * 0.5f;

    for (int ch = 0; ch < 2; ++ch)
    {
        // fast attack, slow fall
        const float level = levels[static_cast<size_t> (ch)].load();
        held[static_cast<size_t> (ch)] = juce::jmax (level, held[static_cast<size_t> (ch)] - 1.6f);
        const float pos = meterPosition (held[static_cast<size_t> (ch)]);

        const float bx = barsArea.getX() + 3.0f + static_cast<float> (ch) * (barW + 3.0f);
        for (int i = 0; i < segments; ++i)
        {
            const float segPos = (static_cast<float> (i) + 0.5f) / static_cast<float> (segments);
            const float sy = usableTop + usableH - static_cast<float> (i + 1) * segH;

            juce::Colour col = juce::Colour (0xff2f9bff).interpolatedWith (juce::Colour (0xff7fd8ff), segPos);
            if (i >= segments - 2)      col = juce::Colour (0xffff5252);
            else if (i >= segments - 5) col = juce::Colour (0xfffbbf24);

            g.setColour (pos >= segPos ? col : col.withAlpha (0.13f));
            g.fillRect (bx, sy + 0.8f, barW, segH - 1.6f);
        }
    }
}

//==============================================================================
// Waveform scope with hit markers, threshold lines and preset header
//==============================================================================
WaveformScopeComponent::WaveformScopeComponent (AuroraTriggerAudioProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void WaveformScopeComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10151c), 0.0f, 0.0f,
                                             juce::Colour (0xff070a0e), 0.0f, bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 5.0f);

    const float midY = bounds.getCentreY();
    const float amp  = bounds.getHeight() * 0.44f;

    // grid
    g.setColour (juce::Colour (0xff1a222d));
    for (int gx = 1; gx < 8; ++gx)
        g.drawVerticalLine (static_cast<int> (bounds.getX() + bounds.getWidth() * static_cast<float> (gx) / 8.0f), bounds.getY(), bounds.getBottom());
    g.drawHorizontalLine (static_cast<int> (midY), bounds.getX(), bounds.getRight());

    // preset header
    g.setColour (juce::Colour (0xff9aa4b4));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (processor.activePresetName, static_cast<int> (bounds.getX() + 9), static_cast<int> (bounds.getY() + 4), 200, 16, juce::Justification::left);

    const int readPos = processor.scopeWritePos.load();
    const float stepX = bounds.getWidth() / static_cast<float> (AuroraTriggerAudioProcessor::scopeSize);
    const int n = AuroraTriggerAudioProcessor::scopeSize;

    // mirrored filled waveform
    juce::Path wave;
    wave.startNewSubPath (bounds.getX(), midY);
    for (int i = 0; i < n; ++i)
    {
        const int idx = (readPos + i) % n;
        wave.lineTo (bounds.getX() + static_cast<float> (i) * stepX, midY - std::abs (processor.scopeData[static_cast<size_t> (idx)]) * amp);
    }
    for (int i = n - 1; i >= 0; --i)
    {
        const int idx = (readPos + i) % n;
        wave.lineTo (bounds.getX() + static_cast<float> (i) * stepX, midY + std::abs (processor.scopeData[static_cast<size_t> (idx)]) * amp);
    }
    wave.closeSubPath();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f6fe0), 0.0f, midY - amp,
                                             juce::Colour (0xff1f6fe0), 0.0f, midY + amp, false));
    g.fillPath (wave);
    g.setColour (juce::Colour (0xff5cc8ff));
    g.strokePath (wave, juce::PathStrokeType (1.0f));

    // hit / suppressed markers
    for (int i = 0; i < n; ++i)
    {
        const int idx = (readPos + i) % n;
        const int marker = processor.scopeMarkers[static_cast<size_t> (idx)];
        if (marker == 0)
            continue;

        const float xPos = bounds.getX() + static_cast<float> (i) * stepX;
        const float barH = std::abs (processor.scopeData[static_cast<size_t> (idx)]) * amp;

        if (marker == 1)
        {
            g.setColour (juce::Colours::white.withAlpha (0.28f));
            g.drawVerticalLine (static_cast<int> (xPos), bounds.getY() + 20.0f, bounds.getBottom() - 4.0f);
            g.setColour (juce::Colour (0xfff97316));
            g.fillEllipse (xPos - 2.5f, midY - barH - 3.0f, 5.0f, 5.0f);
            g.fillEllipse (xPos - 2.5f, midY + barH - 2.0f, 5.0f, 5.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xffef4444));
            g.drawVerticalLine (static_cast<int> (xPos), midY - barH, midY + barH);
            g.fillEllipse (xPos - 3.0f, bounds.getBottom() - 10.0f, 6.0f, 6.0f);
        }
    }

    // detail threshold (both polarities)
    const float detailPct = processor.apvts.getRawParameterValue ("detail")->load() / 100.0f;
    const float off = detailPct * amp;
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.drawHorizontalLine (static_cast<int> (midY - off), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
    g.drawHorizontalLine (static_cast<int> (midY + off), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

    // frame
    g.setColour (juce::Colour (0xff2a313b));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 4.0f, 1.0f);
}

void WaveformScopeComponent::updateDetailFromMouseY (int y)
{
    const float midY = static_cast<float> (getHeight()) * 0.5f;
    const float pct = juce::jlimit (1.0f, 100.0f, ((midY - static_cast<float> (y)) / (midY * 0.88f)) * 100.0f);
    if (auto* param = processor.apvts.getParameter ("detail"))
        param->setValueNotifyingHost (param->convertTo0to1 (pct));
    repaint();
}

void WaveformScopeComponent::mouseDown (const juce::MouseEvent& e) { updateDetailFromMouseY (e.y); }
void WaveformScopeComponent::mouseDrag (const juce::MouseEvent& e) { updateDetailFromMouseY (e.y); }

//==============================================================================
// Main editor
//==============================================================================
AuroraTriggerAudioProcessorEditor::AuroraTriggerAudioProcessorEditor (AuroraTriggerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      inputMeter (p.inputLevelDb), waveformScope (p), outputMeter (p.outputLevelDb)
{
    setLookAndFeel (&metallicLnf);
    setSize (kEditorW, kEditorHFull);

    // brushed-metal texture tile
    {
        brushedTile = juce::Image (juce::Image::ARGB, 160, 160, true);
        juce::Graphics tg (brushedTile);
        juce::Random rng (7);
        for (int row = 0; row < 160; ++row)
        {
            for (int seg = 0; seg < 3; ++seg)
            {
                const float a = rng.nextFloat() * 0.06f;
                tg.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (a));
                const int x0 = rng.nextInt (160), len = 30 + rng.nextInt (120);
                tg.drawHorizontalLine (row, static_cast<float> (x0), static_cast<float> (x0 + len));
                if (x0 + len > 160)
                    tg.drawHorizontalLine (row, 0.0f, static_cast<float> (x0 + len - 160));
            }
        }
    }

    // Left navigation column
    for (auto* b : { &browserBtn, &triggeringBtn, &settingsBtn, &viewMixerBtn, &viewCurvesBtn,
                     &flipPhasesBtn, &savePresetBtn, &clearBufferBtn })
        addAndMakeVisible (*b);
    addAndMakeVisible (linkTuneBtn);
    addAndMakeVisible (dragOnTrackBtn);

    for (auto* b : { &browserBtn, &triggeringBtn, &settingsBtn, &viewMixerBtn, &viewCurvesBtn })
    {
        b->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xfffbfcfe));   // lit = bright white
        b->setColour (juce::TextButton::textColourOnId,   ink);
    }
    browserBtn.onClick    = [this] { setActiveTab (0); };
    triggeringBtn.onClick = [this] { setActiveTab (1); };
    settingsBtn.onClick   = [this] { setActiveTab (2); };

    viewMixerBtn.onClick = [this] { mixerVisible = ! mixerVisible; applyMixerVisibility(); };

    linkTuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "linkTune", linkTuneBtn);
    flipPhasesBtn.onClick = [this] { audioProcessor.flipAllPhases(); };

    dragOnTrackBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xfff2c79a));
    dragOnTrackBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff6b2d00));
    dragOnTrackBtn.setTooltip ("Click and drag onto a DAW track to export the captured MIDI");
    dragOnTrackBtn.onDragStart = [this] { dragCapturedMidiToDaw(); };
    clearBufferBtn.onClick = [this] { audioProcessor.clearMidiCapture(); };

    // Knobs
    auto setupKnob = [this] (juce::Slider& s, juce::Colour capCol)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 17);
        s.setColour (juce::Slider::rotarySliderFillColourId, capCol);
        s.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (s);
    };

    addAndMakeVisible (gateOnBtn);
    gateOnBtn.setColour (juce::TextButton::buttonOnColourId, aurora);
    setupKnob (gateAttSlider,  pink);
    setupKnob (gateHoldSlider, pink);
    setupKnob (gateRelSlider,  pink);
    setupKnob (inVolSlider,    blueKnob);
    addAndMakeVisible (inputMeter);

    setupKnob (lowcutSlider,   blueKnob);
    setupKnob (highcutSlider,  blueKnob);
    setupKnob (suppressSlider, pink);
    addAndMakeVisible (waveformScope);

    setupKnob (sensSlider,   pink);
    setupKnob (retrigSlider, pink);
    setupKnob (detailSlider, pink);
    addAndMakeVisible (outputMeter);
    setupKnob (outVolSlider, blueKnob);
    setupKnob (mixSlider,    blueKnob);

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    auto& ap = audioProcessor.apvts;
    gateOnAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, "gateOn", gateOnBtn);
    gateAttAtt  = std::make_unique<SA> (ap, "gateAttack",  gateAttSlider);
    gateHoldAtt = std::make_unique<SA> (ap, "gateHold",    gateHoldSlider);
    gateRelAtt  = std::make_unique<SA> (ap, "gateRelease", gateRelSlider);
    inVolAtt    = std::make_unique<SA> (ap, "inputVol",    inVolSlider);
    lowcutAtt   = std::make_unique<SA> (ap, "lowcut",      lowcutSlider);
    highcutAtt  = std::make_unique<SA> (ap, "highcut",     highcutSlider);
    suppressAtt = std::make_unique<SA> (ap, "suppress",    suppressSlider);
    sensAtt     = std::make_unique<SA> (ap, "sensitivity", sensSlider);
    retrigAtt   = std::make_unique<SA> (ap, "retrigger",   retrigSlider);
    detailAtt   = std::make_unique<SA> (ap, "detail",      detailSlider);
    outVolAtt   = std::make_unique<SA> (ap, "outputVol",   outVolSlider);
    mixAtt      = std::make_unique<SA> (ap, "mix",         mixSlider);

    // Readout formats (set after the attachments so they win over the default text)
    auto fmt = [] (juce::Slider& s, int decimals, const juce::String& suffix)
    {
        s.textFromValueFunction = [decimals, suffix] (double v) { return formatValue (v, decimals, suffix); };
        s.updateText();
    };
    fmt (gateAttSlider, 1, " ms");   fmt (gateHoldSlider, 0, " ms");  fmt (gateRelSlider, 0, " ms");
    fmt (inVolSlider, 2, " dB");     fmt (lowcutSlider, 0, " hz");    fmt (highcutSlider, 0, " hz");
    fmt (suppressSlider, 0, "%");    fmt (sensSlider, 0, "");         fmt (retrigSlider, 0, " ms");
    fmt (detailSlider, 0, "");       fmt (outVolSlider, 2, " dB");    fmt (mixSlider, 0, "%");

    // Browser & Settings views
    addChildComponent (loadSampleBtn);
    addChildComponent (auditionSlotBtn);
    addChildComponent (resetChilliKitBtn);
    addChildComponent (browserViewLabel);
    addChildComponent (settingsViewLabel);

    loadSampleBtn.onClick     = [this] { loadSampleViaChooser (selectedSlot); };
    auditionSlotBtn.onClick   = [this] { audioProcessor.triggerAudition (selectedSlot, 0.92f); };
    resetChilliKitBtn.onClick = [this] { audioProcessor.loadDefaultChilliSnareKit(); };

    browserViewLabel.setText ("SAMPLE BROWSER & MULTI-MIC KIT LOADER\nClick any bottom channel pad to audition or load a custom .WAV / .AIFF file.",
                              juce::dontSendNotification);
    browserViewLabel.setJustificationType (juce::Justification::centred);

    settingsViewLabel.setText (juce::String (juce::CharPointer_UTF8 (
                                   "AURORA TRIGGER v0.2 - ENGINE SETTINGS\n"
                                   "\xe2\x80\xa2 Link Tune: Synchronizes pitch across all 8 multi-mic channels\n"
                                   "\xe2\x80\xa2 Independent Gate: Cleans original mic bleed even with Mix at 0%\n"
                                   "\xe2\x80\xa2 MIDI Capture: Records hits automatically for DRAG ON TRACK export")),
                               juce::dontSendNotification);
    settingsViewLabel.setJustificationType (juce::Justification::centred);

    // 8-channel mixer
    for (int i = 0; i < AuroraTriggerAudioProcessor::numSlots; ++i)
    {
        auto& st = strips[static_cast<size_t> (i)];
        auto prefix = "slot" + juce::String (i) + "_";

        st.panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        st.panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        st.panSlider.setColour (juce::Slider::rotarySliderFillColourId, purple);
        addAndMakeVisible (st.panSlider);

        st.panReadout.setText ("<C>", juce::dontSendNotification);
        st.panReadout.setJustificationType (juce::Justification::centred);
        st.panReadout.setColour (juce::Label::backgroundColourId, readoutBg);
        addAndMakeVisible (st.panReadout);

        st.tuneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        st.tuneSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        st.tuneSlider.setColour (juce::Slider::rotarySliderFillColourId, copper);
        st.tuneSlider.onValueChange = [this, i]
        {
            audioProcessor.applyLinkedTune (i, static_cast<float> (strips[static_cast<size_t> (i)].tuneSlider.getValue()));
        };
        addAndMakeVisible (st.tuneSlider);

        st.tuneReadout.setText ("0", juce::dontSendNotification);
        st.tuneReadout.setJustificationType (juce::Justification::centred);
        st.tuneReadout.setColour (juce::Label::backgroundColourId, readoutBg);
        addAndMakeVisible (st.tuneReadout);

        st.volSlider.setSliderStyle (juce::Slider::LinearVertical);
        st.volSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (st.volSlider);

        st.volReadout.setText ("0.00 dB", juce::dontSendNotification);
        st.volReadout.setJustificationType (juce::Justification::centred);
        st.volReadout.setColour (juce::Label::backgroundColourId, readoutBg);
        addAndMakeVisible (st.volReadout);

        st.soloBtn.setClickingTogglesState (true);
        st.muteBtn.setClickingTogglesState (true);
        st.phaseBtn.setClickingTogglesState (true);
        st.soloBtn.setColour  (juce::TextButton::buttonColourId, juce::Colour (0xfff4f5f7));
        st.muteBtn.setColour  (juce::TextButton::buttonColourId, juce::Colour (0xfff4f5f7));
        st.phaseBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xfff4f5f7));
        st.soloBtn.setColour  (juce::TextButton::buttonOnColourId, juce::Colour (0xfffbbf24));
        st.muteBtn.setColour  (juce::TextButton::buttonOnColourId, juce::Colour (0xffef4444));
        st.phaseBtn.setColour (juce::TextButton::buttonOnColourId, aurora);
        addAndMakeVisible (st.soloBtn);
        addAndMakeVisible (st.muteBtn);
        addAndMakeVisible (st.phaseBtn);

        st.artLabel.setJustificationType (juce::Justification::centred);
        st.artLabel.setColour (juce::Label::textColourId, juce::Colour (0xffeef1f5));
        addAndMakeVisible (st.artLabel);

        st.bottomPadBtn.onClick = [this, i]
        {
            selectedSlot = i;
            if (audioProcessor.slots[static_cast<size_t> (i)].loaded)
                audioProcessor.triggerAudition (i, 0.90f);
            else
                loadSampleViaChooser (i);
        };
        addAndMakeVisible (st.bottomPadBtn);

        st.panAtt   = std::make_unique<SA> (ap, prefix + "pan",  st.panSlider);
        st.tuneAtt  = std::make_unique<SA> (ap, prefix + "tune", st.tuneSlider);
        st.volAtt   = std::make_unique<SA> (ap, prefix + "vol",  st.volSlider);
        st.soloAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, prefix + "solo",  st.soloBtn);
        st.muteAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, prefix + "mute",  st.muteBtn);
        st.phaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, prefix + "phase", st.phaseBtn);
    }

    viewMixerBtn.setToggleState (true, juce::dontSendNotification);
    setActiveTab (1);

   #if AURORA_PREVIEW_DEMO
    // Preview builds only: fake a drum take so the scope / meters can be screenshotted without audio input.
    {
        juce::Random rng (3);
        const int n = AuroraTriggerAudioProcessor::scopeSize;
        const int hits[] = { 60, 130, 250, 282, 312, 342, 372, 420 };
        const float amps[] = { 0.62f, 0.45f, 0.78f, 0.70f, 0.74f, 0.66f, 0.80f, 0.55f };
        for (int i = 0; i < n; ++i)
        {
            float v = 0.01f * rng.nextFloat();
            for (int k = 0; k < 8; ++k)
                if (i >= hits[k])
                    v = juce::jmax (v, amps[k] * std::exp (-0.16f * static_cast<float> (i - hits[k])) * (0.7f + 0.3f * rng.nextFloat()));
            audioProcessor.scopeData[static_cast<size_t> (i)] = v;
            audioProcessor.scopeMarkers[static_cast<size_t> (i)] = 0;
        }
        for (int k = 0; k < 8; ++k)
            audioProcessor.scopeMarkers[static_cast<size_t> (hits[k])] = 1;
        audioProcessor.scopeMarkers[205] = 2;
        audioProcessor.scopeWritePos.store (0);
        audioProcessor.inputLevelDb[0] = -7.0f;  audioProcessor.inputLevelDb[1] = -9.5f;
        audioProcessor.outputLevelDb[0] = -4.0f; audioProcessor.outputLevelDb[1] = -4.5f;
        if (auto* t = std::getenv ("AURORA_DEMO_TAB")) setActiveTab (juce::String (t).getIntValue());
        if (std::getenv ("AURORA_DEMO_COMPACT") != nullptr) { mixerVisible = false; applyMixerVisibility(); }
    }
   #endif

    startTimerHz (30);
}

AuroraTriggerAudioProcessorEditor::~AuroraTriggerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AuroraTriggerAudioProcessorEditor::applyMixerVisibility()
{
    for (auto& st : strips)
        for (juce::Component* c : std::initializer_list<juce::Component*> { &st.panSlider, &st.panReadout, &st.tuneSlider, &st.tuneReadout,
                                                                            &st.volSlider, &st.volReadout, &st.soloBtn, &st.muteBtn, &st.phaseBtn,
                                                                            &st.artLabel, &st.bottomPadBtn })
            c->setVisible (mixerVisible);

    viewMixerBtn.setToggleState (mixerVisible, juce::dontSendNotification);
    setSize (kEditorW, mixerVisible ? kEditorHFull : kEditorHCompact);
    resized();
    repaint();
}

void AuroraTriggerAudioProcessorEditor::setActiveTab (int tabIndex)
{
    activeViewTab = tabIndex;

    browserBtn.setToggleState    (tabIndex == 0, juce::dontSendNotification);
    triggeringBtn.setToggleState (tabIndex == 1, juce::dontSendNotification);
    settingsBtn.setToggleState   (tabIndex == 2, juce::dontSendNotification);

    const bool isTrig = (tabIndex == 1);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &gateOnBtn, &gateAttSlider, &gateHoldSlider, &gateRelSlider, &inVolSlider, &inputMeter,
             &lowcutSlider, &highcutSlider, &suppressSlider, &waveformScope,
             &sensSlider, &retrigSlider, &detailSlider, &outputMeter, &outVolSlider, &mixSlider })
        c->setVisible (isTrig);

    const bool isBrowser = (tabIndex == 0);
    loadSampleBtn.setVisible (isBrowser);
    auditionSlotBtn.setVisible (isBrowser);
    resetChilliKitBtn.setVisible (isBrowser);
    browserViewLabel.setVisible (isBrowser);
    settingsViewLabel.setVisible (tabIndex == 2);

    repaint();
}

void AuroraTriggerAudioProcessorEditor::loadSampleViaChooser (int slotIndex)
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select Drum Mic Sample (.wav / .aiff)",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                       "*.wav;*.aiff;*.flac");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this, slotIndex] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
            audioProcessor.loadSampleIntoSlot (slotIndex, file);
    });
}

void AuroraTriggerAudioProcessorEditor::dragCapturedMidiToDaw()
{
    auto tempMid = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("AuroraTrigger_Capture.mid");
    if (audioProcessor.exportCapturedMidiToFile (tempMid))
        performExternalDragDropOfFiles ({ tempMid.getFullPathName() }, false);
}

void AuroraTriggerAudioProcessorEditor::timerCallback()
{
    if (activeViewTab == 1)
    {
        waveformScope.repaint();
        inputMeter.repaint();
        outputMeter.repaint();
    }

    if (! mixerVisible)
        return;

    for (int i = 0; i < AuroraTriggerAudioProcessor::numSlots; ++i)
    {
        auto& st = strips[static_cast<size_t> (i)];
        const auto& slot = audioProcessor.slots[static_cast<size_t> (i)];

        st.artLabel.setText (slot.articulationName, juce::dontSendNotification);
        st.bottomPadBtn.setButtonText (slot.micLabel);

        if (padWasLoaded[static_cast<size_t> (i)] != slot.loaded || ! st.bottomPadBtn.isShowing())
        {
            padWasLoaded[static_cast<size_t> (i)] = slot.loaded;
            st.bottomPadBtn.setColour (juce::TextButton::buttonColourId, slot.loaded ? juce::Colour (0xffdadde4) : juce::Colour (0xff3d4148));
            st.bottomPadBtn.setColour (juce::TextButton::textColourOffId, slot.loaded ? ink : juce::Colour (0xff7a808c));
            st.bottomPadBtn.repaint();
        }

        const float panVal = static_cast<float> (st.panSlider.getValue());
        if (std::abs (panVal) < 0.03f)
            st.panReadout.setText ("<C>", juce::dontSendNotification);
        else if (panVal < 0.0f)
            st.panReadout.setText ("L" + juce::String (static_cast<int> (-panVal * 100.0f)), juce::dontSendNotification);
        else
            st.panReadout.setText ("R" + juce::String (static_cast<int> (panVal * 100.0f)), juce::dontSendNotification);

        st.volReadout.setText (juce::String (st.volSlider.getValue(), 2) + " dB", juce::dontSendNotification);

        const float tune = static_cast<float> (st.tuneSlider.getValue());
        st.tuneReadout.setText (std::abs (tune) < 0.05f ? juce::String ("0") : juce::String (tune, 1), juce::dontSendNotification);
    }
}

void AuroraTriggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int h = getHeight();

    // Chassis
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5e67), 0.0f, 0.0f, juce::Colour (0xff3b3f46), 0.0f, static_cast<float> (h), false));
    g.fillAll();
    g.setTiledImageFill (brushedTile, 0, 0, 1.0f);
    g.fillAll();

    // Left sidebar
    auto side = juce::Rectangle<float> (6.0f, 6.0f, 136.0f, static_cast<float> (h - 12));
    drawBevelPanel (g, side, juce::Colour (0xff80858f), juce::Colour (0xff5b606a), 4.0f);
    g.setTiledImageFill (brushedTile, 6, 6, 1.0f);
    g.fillRoundedRectangle (side, 4.0f);

    // Aurora wordmark
    drawShadowedText (g, "AURORA", { 14, 14, 120, 24 }, 21.0f, juce::Colours::white, juce::Justification::centred, 0.18f);
    drawShadowedText (g, "TRIGGER", { 14, 36, 120, 14 }, 11.0f, juce::Colour (0xffd5dae3), juce::Justification::centred, 0.42f);
    {
        juce::ColourGradient ag (auroraGreen, 22.0f, 0.0f, auroraViolet, 126.0f, 0.0f, false);
        ag.addColour (0.5, aurora);
        g.setGradientFill (ag);
        g.fillRoundedRectangle (22.0f, 54.0f, 104.0f, 3.0f, 1.5f);
    }

    // MIDI capture group
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawRoundedRectangle (12.0f, static_cast<float> (midiBoxY), 124.0f, 82.0f, 5.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (13.0f, static_cast<float> (midiBoxY) + 1.0f, 124.0f, 82.0f, 5.0f, 1.0f);
        g.setColour (juce::Colour (0xff6c717b));
        g.fillRect (34, midiBoxY - 7, 80, 14);
        drawShadowedText (g, "MIDI CAPTURE", { 16, midiBoxY - 7, 116, 14 }, 10.5f);
    }

    auto panel = [&] (float x, float y, float w, float ht)
    {
        drawBevelPanel (g, { x, y, w, ht }, panelTop, panelBottom, 5.0f);
        g.setTiledImageFill (brushedTile, static_cast<int> (x), static_cast<int> (y), 0.8f);
        g.fillRoundedRectangle (x, y, w, ht, 5.0f);
    };

    if (activeViewTab == 1)
    {
        panel (148.0f, 6.0f, 146.0f, 250.0f);   // Gate + Input
        panel (300.0f, 6.0f, 520.0f, 250.0f);   // Filters + Scope
        panel (826.0f, 6.0f, 208.0f, 250.0f);   // Sensitivity + Output

        drawShadowedText (g, "GATE",     { 152, 10, 64, 14 },  11.0f);
        drawShadowedText (g, "ATTACK",   { 152, 54, 64, 14 },  11.0f);
        drawShadowedText (g, "HOLD",     { 152, 122, 64, 14 }, 11.0f);
        drawShadowedText (g, "RELEASE",  { 152, 190, 64, 14 }, 11.0f);
        drawShadowedText (g, "INPUT",    { 224, 10, 64, 14 },  11.0f);
        drawShadowedText (g, "VOLUME",   { 224, 122, 64, 14 }, 11.0f);

        drawShadowedText (g, "LOWCUT",   { 306, 12, 86, 14 },  11.0f);
        drawShadowedText (g, "HIGHCUT",  { 306, 86, 86, 14 },  11.0f);
        drawShadowedText (g, "SUPPRESS", { 306, 178, 86, 14 }, 11.0f);

        drawShadowedText (g, "SENSITIVITY", { 834, 24, 114, 14 },  12.0f);
        drawShadowedText (g, "RETRIGGER",   { 834, 98, 114, 14 },  12.0f);
        drawShadowedText (g, "DETAIL",      { 834, 172, 114, 14 }, 12.0f);
        drawShadowedText (g, "OUTPUT",      { 956, 10, 72, 14 },   11.0f);
        drawShadowedText (g, "VOLUME",      { 956, 122, 72, 14 },  11.0f);
        drawShadowedText (g, "MIX",         { 956, 188, 72, 14 },  11.0f);
    }
    else
    {
        panel (148.0f, 6.0f, 886.0f, 250.0f);
    }

    if (mixerVisible)
    {
        const int startX = 148, stripW = 107, gap = 4;

        for (int i = 0; i < AuroraTriggerAudioProcessor::numSlots; ++i)
        {
            const int sx = startX + i * (stripW + gap);
            auto r = juce::Rectangle<float> (static_cast<float> (sx), 264.0f, static_cast<float> (stripW), 312.0f);

            drawBevelPanel (g, r, stripTop, stripBottom, 5.0f);
            g.setTiledImageFill (brushedTile, sx, 264, 0.9f);
            g.fillRoundedRectangle (r, 5.0f);

            // label bar
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a707b), 0.0f, 553.0f, juce::Colour (0xff4d525c), 0.0f, 573.0f, false));
            g.fillRoundedRectangle (static_cast<float> (sx + 3), 553.0f, static_cast<float> (stripW - 6), 20.0f, 3.0f);

            g.setColour (ink);
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            g.drawText ("PAN",  sx, 268, stripW, 14, juce::Justification::centred);
            g.drawText ("TUNE", sx, 336, stripW, 14, juce::Justification::centred);
        }
    }
}

void AuroraTriggerAudioProcessorEditor::resized()
{
    const int lx = 14, lw = 120;
    auto Y = [this] (int full, int compact) { return mixerVisible ? full : compact; };

    browserBtn.setBounds     (lx, 74, lw, 28);
    triggeringBtn.setBounds  (lx, 110, lw, 28);
    settingsBtn.setBounds    (lx, 146, lw, 28);

    viewMixerBtn.setBounds   (lx, Y (268, 200), lw, 28);
    viewCurvesBtn.setBounds  (lx, Y (304, 236), lw, 28);

    linkTuneBtn.setBounds    (lx, Y (434, 290), lw, 26);
    flipPhasesBtn.setBounds  (lx, Y (468, 324), lw, 26);
    savePresetBtn.setBounds  (lx, Y (502, 358), lw, 26);

    midiBoxY = Y (546, 396);
    dragOnTrackBtn.setBounds (lx + 4, midiBoxY + 14, lw - 8, 26);
    clearBufferBtn.setBounds (lx + 4, midiBoxY + 46, lw - 8, 26);

    // Triggering page
    gateOnBtn.setBounds      (166, 28, 40, 22);
    gateAttSlider.setBounds  (154, 68, 60, 54);
    gateHoldSlider.setBounds (154, 136, 60, 54);
    gateRelSlider.setBounds  (154, 204, 60, 50);

    inputMeter.setBounds     (224, 28, 62, 90);
    inVolSlider.setBounds    (226, 138, 60, 60);

    lowcutSlider.setBounds   (312, 28, 80, 54);
    highcutSlider.setBounds  (312, 102, 80, 54);
    suppressSlider.setBounds (312, 194, 80, 54);
    waveformScope.setBounds  (402, 14, 410, 234);

    sensSlider.setBounds     (834, 38, 112, 54);
    retrigSlider.setBounds   (834, 112, 112, 54);
    detailSlider.setBounds   (834, 186, 112, 54);

    outputMeter.setBounds    (958, 28, 62, 90);
    outVolSlider.setBounds   (956, 136, 68, 52);
    mixSlider.setBounds      (956, 202, 68, 52);

    // Browser & Settings
    browserViewLabel.setBounds  (220, 40, 740, 60);
    loadSampleBtn.setBounds     (290, 120, 200, 34);
    auditionSlotBtn.setBounds   (510, 120, 180, 34);
    resetChilliKitBtn.setBounds (710, 120, 180, 34);
    settingsViewLabel.setBounds (220, 50, 740, 150);

    // 8-channel mixer strips + bottom pads
    if (mixerVisible)
    {
        const int startX = 148, stripW = 107, gap = 4;

        for (int i = 0; i < AuroraTriggerAudioProcessor::numSlots; ++i)
        {
            const int sx = startX + i * (stripW + gap);
            auto& st = strips[static_cast<size_t> (i)];

            st.panSlider.setBounds   (sx + 52, 282, 36, 34);
            st.panReadout.setBounds  (sx + 42, 317, 58, 16);

            st.tuneSlider.setBounds  (sx + 52, 350, 36, 34);
            st.tuneReadout.setBounds (sx + 42, 385, 58, 16);

            st.volSlider.setBounds   (sx + 6, 274, 34, 246);
            st.volReadout.setBounds  (sx + 10, 528, stripW - 20, 18);
            st.soloBtn.setBounds     (sx + 56, 418, 30, 28);
            st.muteBtn.setBounds     (sx + 56, 452, 30, 28);
            st.phaseBtn.setBounds    (sx + 56, 486, 30, 28);

            st.artLabel.setBounds    (sx + 4, 554, stripW - 8, 18);
            st.bottomPadBtn.setBounds (sx, 582, stripW, 48);
        }
    }
}
