#include "Displays.h"

namespace ui
{

using namespace botanic;

namespace
{
    juce::Rectangle<float> insetPlot (juce::Component& c, float insetX = 10.0f, float insetY = 8.0f)
    {
        return c.getLocalBounds().toFloat().reduced (insetX, insetY);
    }

    float logNorm (float value, float lo, float hi)
    {
        return juce::jlimit (0.0f, 1.0f, (std::log (juce::jmax (lo, value)) - std::log (lo))
                                       / (std::log (hi) - std::log (lo)));
    }

    float invLogNorm (float norm, float lo, float hi)
    {
        return lo * std::pow (hi / lo, juce::jlimit (0.0f, 1.0f, norm));
    }
}

//==============================================================================
WavetableDisplay::WavetableDisplay (BotanicAudioProcessor& processor) : proc (processor)
{
    posParam = proc.apvts.getParameter (Params::oscId (0, "pos"));
    refresh();
}

void WavetableDisplay::setOscillator (int index)
{
    oscIndex = juce::jlimit (0, 2, index);
    posParam = proc.apvts.getParameter (Params::oscId (oscIndex, "pos"));
    refresh();
    repaint();
}

void WavetableDisplay::refresh()
{
    const auto t = (int) proc.paramValue (Params::oscId (oscIndex, "table"));
    const auto p = proc.paramValue (Params::oscId (oscIndex, "pos"));

    if (t != tableIndex || std::abs (p - position) > 0.0005f)
    {
        tableIndex = t;
        position = p;
        cacheDirty = true;
        repaint();
    }
}

juce::Rectangle<float> WavetableDisplay::plotArea() const
{
    return insetPlot (*const_cast<WavetableDisplay*> (this), 8.0f, 8.0f);
}

void WavetableDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    if (! staticCache.isValid() || staticCache.getWidth() != getWidth()
        || staticCache.getHeight() != getHeight() || cacheDirty)
    {
        staticCache = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true);
        juce::Graphics cg (staticCache);
        paintStatic (cg);
        cacheDirty = false;
    }

    g.drawImageAt (staticCache, 0, 0);

    auto plot = plotArea();

    // animated play dot
    const auto& tbl = WavetableBank::instance().getTable (tableIndex);
    const auto ms = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const auto animPhase = (float) std::fmod (ms * 0.22, 1.0);
    const auto animV = WavetableBank::sample (tbl, position, 5, animPhase);
    const auto ax = plot.getX() + plot.getWidth() * animPhase;
    const auto ay = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, animV) * plot.getHeight() * 0.44f;
    g.setColour (th::greenDeep.withAlpha (0.35f));
    g.fillEllipse (ax - 2.5f, ay - 2.5f, 5.0f, 5.0f);

    juce::ignoreUnused (r);
}

void WavetableDisplay::paintStatic (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    th::drawInnerWell (g, r, 9.0f);

    auto plot = plotArea();
    const auto& table = WavetableBank::instance().getTable (tableIndex);

    // grid
    g.setColour (th::ink.withAlpha (0.06f));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * (float) i / 8.0f), plot.getY(), plot.getBottom());

    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * (float) i / 4.0f), plot.getX(), plot.getRight());

    g.setColour (th::ink.withAlpha (0.10f));
    g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());

    // curve
    const auto steps = juce::jmax (32, (int) plot.getWidth());
    juce::Path curve, filled;

    for (int i = 0; i <= steps; ++i)
    {
        const auto phase = (float) i / (float) steps;
        const auto v = WavetableBank::sample (table, position, 5, phase);
        const auto x = plot.getX() + plot.getWidth() * phase;
        const auto y = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, v) * plot.getHeight() * 0.44f;

        if (i == 0)
        {
            curve.startNewSubPath (x, y);
            filled.startNewSubPath (x, plot.getBottom());
            filled.lineTo (x, y);
        }
        else
        {
            curve.lineTo (x, y);
            filled.lineTo (x, y);
        }
    }

    filled.lineTo (plot.getRight(), plot.getBottom());
    filled.closeSubPath();

    juce::ColourGradient fillGrad (th::greenMid.withAlpha (0.55f), plot.getX(), plot.getY(),
                                   th::greenLight.withAlpha (0.18f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (filled);

    g.setColour (juce::Colour (0xff5d9c1b));
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // morph node
    const auto nodeV = WavetableBank::sample (table, position, 5, position);
    const auto nodeX = plot.getX() + plot.getWidth() * position;
    const auto nodeY = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, nodeV) * plot.getHeight() * 0.44f;

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillEllipse (nodeX - 7.5f, nodeY - 7.5f, 15.0f, 15.0f);
    g.setColour (th::green.withAlpha (0.95f));
    g.drawEllipse (nodeX - 7.5f, nodeY - 7.5f, 15.0f, 15.0f, 1.8f);
}

void WavetableDisplay::applyPosition (float newPos)
{
    if (posParam != nullptr)
        posParam->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, newPos));

    refresh();
}

void WavetableDisplay::mouseDown (const juce::MouseEvent& e)
{
    mouseDrag (e);
}

void WavetableDisplay::mouseDrag (const juce::MouseEvent& e)
{
    const auto plot = plotArea();
    applyPosition ((e.position.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
}

void WavetableDisplay::mouseDoubleClick (const juce::MouseEvent&)
{
    if (posParam != nullptr)
    {
        posParam->setValueNotifyingHost (posParam->getDefaultValue());
        refresh();
    }
}

//==============================================================================
FilterDisplay::FilterDisplay (BotanicAudioProcessor& processor) : proc (processor)
{
    refresh();
}

juce::Rectangle<float> FilterDisplay::plotArea() const
{
    return getLocalBounds().toFloat().reduced (8.0f, 8.0f);
}

void FilterDisplay::refresh()
{
    const auto t = (int) proc.paramValue (Params::filterType);
    const auto c = proc.paramValue (Params::cutoff);
    const auto r = proc.paramValue (Params::resonance);

    if (t != type || std::abs (c - cutoff) > 0.5f || std::abs (r - resonance) > 0.0005f)
    {
        type = t;
        cutoff = c;
        resonance = r;
        repaint();
    }
}

void FilterDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    th::drawInnerWell (g, r, 9.0f);

    auto plot = plotArea();
    const auto sr = 48000.0;
    const auto q = 0.5f + resonance * 6.5f;

    juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

    switch (type)
    {
        case 0:
        case 1: coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, cutoff, q); break;
        case 2: coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, cutoff, q); break;
        case 3: coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, cutoff, q); break;
        default: coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch (sr, cutoff, q); break;
    }

    const auto steps = juce::jmax (48, (int) plot.getWidth());
    juce::Path curve, fill;

    for (int i = 0; i <= steps; ++i)
    {
        const auto x = (float) i / (float) steps;
        const auto freq = invLogNorm (x, 20.0f, 20000.0f);
        auto mag = coeffs->getMagnitudeForFrequency (freq, sr);

        if (type == 0)
            mag *= mag;   // 24 dB slope

        const auto db = juce::Decibels::gainToDecibels ((float) mag, -60.0f);
        const auto normY = juce::jlimit (0.0f, 1.0f, (db + 42.0f) / 60.0f);

        const auto px = plot.getX() + x * plot.getWidth();
        const auto py = plot.getBottom() - normY * plot.getHeight();

        if (i == 0)
        {
            curve.startNewSubPath (px, py);
            fill.startNewSubPath (px, plot.getBottom());
            fill.lineTo (px, py);
        }
        else
        {
            curve.lineTo (px, py);
            fill.lineTo (px, py);
        }
    }

    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.closeSubPath();

    juce::ColourGradient fillGrad (th::greenMid.withAlpha (0.6f), plot.getX(), plot.getY(),
                                   th::greenLight.withAlpha (0.12f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fill);

    g.setColour (juce::Colour (0xff4f8d16));
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // cutoff node
    const auto cx = plot.getX() + logNorm (cutoff, 20.0f, 20000.0f) * plot.getWidth();
    auto mag = coeffs->getMagnitudeForFrequency (cutoff, sr);

    if (type == 0)
        mag *= mag;

    const auto db = juce::Decibels::gainToDecibels ((float) mag, -60.0f);
    const auto cy = plot.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 42.0f) / 60.0f) * plot.getHeight();

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.fillEllipse (cx - 7.5f, cy - 7.5f, 15.0f, 15.0f);
    g.setColour (th::green.withAlpha (0.95f));
    g.drawEllipse (cx - 7.5f, cy - 7.5f, 15.0f, 15.0f, 1.8f);

    // subtle frequency guides
    g.setColour (th::ink.withAlpha (0.07f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const auto gx = plot.getX() + logNorm (f, 20.0f, 20000.0f) * plot.getWidth();
        g.drawVerticalLine (juce::roundToInt (gx), plot.getY(), plot.getBottom());
    }
}

void FilterDisplay::applyFromPoint (juce::Point<float> p)
{
    auto plot = plotArea();
    const auto xNorm = juce::jlimit (0.0f, 1.0f, (p.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
    const auto yNorm = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - p.y) / juce::jmax (1.0f, plot.getHeight()));

    const auto freq = invLogNorm (xNorm, 20.0f, 20000.0f);
    const auto reso = juce::jlimit (0.0f, 1.0f, (yNorm * 60.0f - 42.0f + 18.0f) / 30.0f * 0.6f + 0.15f);

    if (auto* pCut = proc.apvts.getParameter (Params::cutoff))
        pCut->setValueNotifyingHost (pCut->convertTo0to1 (freq));

    if (auto* pRes = proc.apvts.getParameter (Params::resonance))
        pRes->setValueNotifyingHost (pRes->convertTo0to1 (juce::jlimit (0.0f, 1.0f, reso)));

    refresh();
}

void FilterDisplay::mouseDown (const juce::MouseEvent& e) { applyFromPoint (e.position); }
void FilterDisplay::mouseDrag (const juce::MouseEvent& e) { applyFromPoint (e.position); }

//==============================================================================
EnvelopeDisplay::EnvelopeDisplay (BotanicAudioProcessor& processor) : proc (processor)
{
    refresh();
}

void EnvelopeDisplay::setEnvelope (int index)
{
    envIndex = juce::jlimit (0, 2, index);
    refresh();
    repaint();
}

void EnvelopeDisplay::setHighlightStage (int stage)
{
    if (stage != highlight)
    {
        highlight = stage;
        repaint();
    }
}

void EnvelopeDisplay::refresh()
{
    const auto na = proc.paramValue (Params::envId (envIndex, "A"));
    const auto nd = proc.paramValue (Params::envId (envIndex, "D"));
    const auto ns = proc.paramValue (Params::envId (envIndex, "S"));
    const auto nr = proc.paramValue (Params::envId (envIndex, "R"));

    if (std::abs (na - a) > 1.0e-5f || std::abs (nd - d) > 1.0e-5f
        || std::abs (ns - s) > 1.0e-5f || std::abs (nr - r) > 1.0e-5f)
    {
        a = na; d = nd; s = ns; r = nr;
        repaint();
    }
}

std::array<juce::Point<float>, 5> EnvelopeDisplay::computeNodes() const
{
    auto plot = getLocalBounds().toFloat().reduced (14.0f, 10.0f);

    const auto aLen = 0.10f + 0.32f * std::pow (juce::jlimit (0.0f, 1.0f, std::log10 (1.0f + a * 9.0f) / 1.0f), 1.0f);
    const auto dLen = 0.10f + 0.26f * juce::jlimit (0.0f, 1.0f, std::log10 (1.0f + d * 9.0f));
    const auto sLen = 0.16f;
    const auto rLen = 0.12f + 0.30f * juce::jlimit (0.0f, 1.0f, std::log10 (1.0f + r * 1.5f));

    const auto total = aLen + dLen + sLen + rLen;
    const auto scale = 1.0f / total;

    std::array<juce::Point<float>, 5> nodes;
    float x = plot.getX();

    nodes[0] = { x, plot.getBottom() };
    x += aLen * scale * plot.getWidth();
    nodes[1] = { x, plot.getY() };
    x += dLen * scale * plot.getWidth();
    nodes[2] = { x, plot.getBottom() - s * plot.getHeight() };
    x += sLen * scale * plot.getWidth();
    nodes[3] = { x, plot.getBottom() - s * plot.getHeight() };
    x += rLen * scale * plot.getWidth();
    nodes[4] = { x, plot.getBottom() };

    return nodes;
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    th::drawInnerWell (g, r, 9.0f);

    auto plot = getLocalBounds().toFloat().reduced (14.0f, 10.0f);

    g.setColour (th::ink.withAlpha (0.06f));
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * (float) i / 4.0f), plot.getY(), plot.getBottom());
    for (int i = 1; i < 3; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * (float) i / 3.0f), plot.getX(), plot.getRight());

    const auto nodes = computeNodes();
    lastNodes = nodes;

    juce::Path line, fill;
    line.startNewSubPath (nodes[0]);
    fill.startNewSubPath (nodes[0].x, plot.getBottom());
    fill.lineTo (nodes[0]);

    for (size_t i = 1; i < nodes.size(); ++i)
    {
        line.lineTo (nodes[i]);
        fill.lineTo (nodes[i]);
    }

    fill.lineTo (nodes[4].x, plot.getBottom());
    fill.closeSubPath();

    juce::ColourGradient grad (th::greenMid.withAlpha (0.45f), plot.getX(), plot.getY(),
                               th::greenLight.withAlpha (0.10f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (fill);

    g.setColour (juce::Colour (0xff4f8d16));
    g.strokePath (line, juce::PathStrokeType (2.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const auto active = (int) i == highlight || (highlight == 0 && i == 1);
        const auto radius = active ? 7.0f : 5.5f;

        g.setColour (juce::Colours::white);
        g.fillEllipse (nodes[i].x - radius, nodes[i].y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (active ? th::greenDark : th::green.withAlpha (0.8f));
        g.drawEllipse (nodes[i].x - radius, nodes[i].y - radius, radius * 2.0f, radius * 2.0f, active ? 2.2f : 1.6f);
    }
}

int EnvelopeDisplay::hitNode (juce::Point<float> p) const
{
    const auto nodes = computeNodes();
    int best = -1;
    float bestDist = 22.0f;

    for (int i = 1; i < 5; ++i)
    {
        const auto dist = nodes[(size_t) i].getDistanceFrom (p);

        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }

    return best;
}

void EnvelopeDisplay::mouseDown (const juce::MouseEvent& e)
{
    draggedNode = hitNode (e.position);
}

void EnvelopeDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedNode < 0)
        return;

    auto plot = getLocalBounds().toFloat().reduced (14.0f, 10.0f);
    const auto xNorm = juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
    const auto yNorm = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - e.position.y) / juce::jmax (1.0f, plot.getHeight()));

    auto setParam = [this] (const char* field, float value, float lo, float hi)
    {
        if (auto* p = proc.apvts.getParameter (Params::envId (envIndex, field)))
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (lo, hi, value)));
    };

    switch (draggedNode)
    {
        case 1: setParam ("A", std::pow (10.0f, xNorm * 2.2f) * 0.06f, 0.001f, 8.0f); break;
        case 2:
            setParam ("D", std::pow (10.0f, xNorm * 2.2f) * 0.06f, 0.001f, 8.0f);
            setParam ("S", yNorm, 0.0f, 1.0f);
            break;
        case 3: setParam ("S", yNorm, 0.0f, 1.0f); break;
        case 4: setParam ("R", std::pow (10.0f, xNorm * 2.2f) * 0.06f, 0.001f, 12.0f); break;
        default: break;
    }

    refresh();
}

//==============================================================================
LfoDisplay::LfoDisplay (BotanicAudioProcessor& processor) : proc (processor)
{
    refresh();
}

void LfoDisplay::setLfo (int index)
{
    lfoIndex = juce::jlimit (0, 1, index);
    refresh();
    repaint();
}

void LfoDisplay::refresh()
{
    const auto sh = (int) proc.paramValue (Params::lfoId (lfoIndex, "shape"));
    const auto rt = proc.paramValue (Params::lfoId (lfoIndex, "rate"));
    const auto dp = proc.paramValue (Params::lfoId (lfoIndex, "depth"));
    const auto ph = proc.paramValue (Params::lfoId (lfoIndex, "phase"));

    if (sh != shape || std::abs (rt - rate) > 0.001f || std::abs (dp - depth) > 0.001f || std::abs (ph - phase) > 0.001f)
    {
        shape = sh; rate = rt; depth = dp; phase = ph;
        cacheDirty = true;
        repaint();
    }
}

void LfoDisplay::paint (juce::Graphics& g)
{
    if (! staticCache.isValid() || staticCache.getWidth() != getWidth()
        || staticCache.getHeight() != getHeight() || cacheDirty)
    {
        staticCache = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true);
        juce::Graphics cg (staticCache);
        paintStatic (cg);
        cacheDirty = false;
    }

    g.drawImageAt (staticCache, 0, 0);

    auto r = getLocalBounds().toFloat();
    auto plot = r.reduced (10.0f, 8.0f);

    const auto ms = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const auto animPhase = (float) std::fmod (ms * rate * 0.5, 1.0);
    g.setColour (th::greenDeep.withAlpha (0.28f));
    g.fillEllipse (plot.getX() + plot.getWidth() * animPhase - 2.5f, plot.getCentreY() - 2.5f, 5.0f, 5.0f);
}

void LfoDisplay::paintStatic (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    th::drawInnerWell (g, r, 9.0f);

    auto plot = r.reduced (10.0f, 8.0f);

    g.setColour (th::ink.withAlpha (0.06f));
    g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());
    g.drawVerticalLine (juce::roundToInt (plot.getCentreX()), plot.getY(), plot.getBottom());

    const auto steps = juce::jmax (64, (int) plot.getWidth());
    const auto cycles = juce::jlimit (0.75f, 4.0f, 0.6f + std::log10 (1.0f + rate) * 1.6f);

    juce::Path line, fill;
    fill.startNewSubPath (plot.getX(), plot.getCentreY());

    for (int i = 0; i <= steps; ++i)
    {
        const auto x = (float) i / (float) steps;
        const auto p = std::fmod (x * cycles + phase, 1.0f);
        float v = 0.0f;

        switch (shape)
        {
            case 0: v = 4.0f * std::abs (p - 0.5f) - 1.0f; break;
            case 1: v = std::sin (p * juce::MathConstants<float>::twoPi); break;
            case 2: v = 2.0f * p - 1.0f; break;
            case 3: v = 1.0f - 2.0f * p; break;
            case 4: v = p < 0.5f ? 1.0f : -1.0f; break;
            case 5: v = std::sin ((float) (int) (p * 4.0f) * 2.3f); break;
            default: v = std::sin (p * 6.0f) * 0.8f; break;
        }

        v *= depth;
        const auto px = plot.getX() + x * plot.getWidth();
        const auto py = plot.getCentreY() - v * plot.getHeight() * 0.46f;

        if (i == 0)
            line.startNewSubPath (px, py);
        else
            line.lineTo (px, py);

        fill.lineTo (px, py);
    }

    fill.lineTo (plot.getRight(), plot.getCentreY());
    fill.closeSubPath();

    juce::ColourGradient grad (th::greenMid.withAlpha (0.40f), plot.getX(), plot.getY(),
                               th::greenLight.withAlpha (0.10f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (fill);

    g.setColour (juce::Colour (0xff4f8d16));
    g.strokePath (line, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void LfoDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragStart = proc.paramValue (Params::lfoId (lfoIndex, "depth"));
    mouseDrag (e);
}

void LfoDisplay::mouseDrag (const juce::MouseEvent& e)
{
    auto plot = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    const auto norm = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - e.position.y) / juce::jmax (1.0f, plot.getHeight()) * 1.6f);

    if (auto* p = proc.apvts.getParameter (Params::lfoId (lfoIndex, "depth")))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, norm)));

    refresh();
}

//==============================================================================
ScopeDisplay::ScopeDisplay (BotanicAudioProcessor& processor) : proc (processor)
{
    ring.fill (0.0f);
}

void ScopeDisplay::pushSamples (const float* left, const float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        ring[(size_t) writePos] = 0.5f * (left[i] + right[i]);
        writePos = (writePos + 1) % size;
    }
}

void ScopeDisplay::paint (juce::Graphics& g)
{
    juce::ignoreUnused (proc);

    auto r = getLocalBounds().toFloat();
    th::drawInnerWell (g, r, 9.0f);

    auto plot = r.reduced (10.0f, 8.0f);
    juce::Path p;

    for (int i = 0; i < size; ++i)
    {
        const auto idx = (writePos + i) % size;
        const auto x = plot.getX() + plot.getWidth() * (float) i / (float) size;
        const auto y = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, ring[(size_t) idx]) * plot.getHeight() * 0.48f;

        if (i == 0)
            p.startNewSubPath (x, y);
        else
            p.lineTo (x, y);
    }

    g.setColour (th::green.withAlpha (0.85f));
    g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace ui
