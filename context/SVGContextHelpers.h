/*
    Copyright 2018 Antonio Lassandro

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#pragma once

static inline LowLevelGraphicsSVGRenderer* _ctx(const juce::Graphics &g)
{
  #if SVG_USE_CONTEXT_HELPERS
    return dynamic_cast<LowLevelGraphicsSVGRenderer*>(&g.getInternalContext());
  #else
    return nullptr;
  #endif
}

#pragma mark -
// =============================================================================

static inline void _pushGroup(juce::Graphics &g, const juce::String &text)
{
    if (auto ctx = _ctx(g))
        ctx->pushGroup(text);
}

static inline void _popGroup(juce::Graphics &g)
{
    if (auto ctx = _ctx(g))
        ctx->popGroup();
}

static inline void _setTags(juce::Graphics &g, const juce::StringPairArray &tags)
{
    if (auto ctx = _ctx(g))
        ctx->setTags(tags);
}

static inline void _clearTags(juce::Graphics &g)
{
    if (auto ctx = _ctx(g))
        ctx->clearTags();
}

#pragma mark -
// =============================================================================

static inline void _drawSingleLineText(
    const juce::Graphics &g,
    const juce::String &text,
    int startX, int baselineY,
    juce::Justification justification = juce::Justification::left)
{
    if (auto ctx = _ctx(g))
        ctx->drawSingleLineText(text, startX, baselineY, justification);
    else
        g.drawSingleLineText(text, startX, baselineY, justification);
}

static inline void _drawMultiLineText(
    const juce::Graphics &g,
    const juce::String &text,
    int startX, int baselineY,
    int maximumLineWidth)
{
    if (auto ctx = _ctx(g))
        ctx->drawMultiLineText(text, startX, baselineY, maximumLineWidth);
    else
        g.drawMultiLineText(text, startX, baselineY, maximumLineWidth);
}

static inline void _drawText(
    const juce::Graphics &g,
    const juce::String &text,
    int x, int y,
    int width, int height,
    juce::Justification justification,
    bool useEllipsesIfTooBig = true)
{
    if (auto ctx = _ctx(g))
        ctx->drawText(
            text,
            x, y,
            width, height,
            justification,
            useEllipsesIfTooBig
        );
    else
        g.drawText(
            text,
            x, y,
            width, height,
            justification,
            useEllipsesIfTooBig
        );
}

static inline void _drawText(
    const juce::Graphics &g,
    const juce::String &text,
    juce::Rectangle<int> area,
    juce::Justification justification,
    bool useEllipsesIfTooBig = true)
{
    if (auto ctx = _ctx(g))
        ctx->drawText(text, area, justification, useEllipsesIfTooBig);
    else
        g.drawText(text, area, justification, useEllipsesIfTooBig);
}

static inline void _drawText(
    const juce::Graphics &g,
    const juce::String &text,
    juce::Rectangle<float> area,
    juce::Justification justification,
    bool useEllipsesIfTooBig = true)
{
    if (auto ctx = _ctx(g))
        ctx->drawText(text, area, justification, useEllipsesIfTooBig);
    else
        g.drawText(text, area, justification, useEllipsesIfTooBig);
}

static inline void _drawFittedText(
    const juce::Graphics &g,
    const juce::String &text,
    int x, int y,
    int width, int height,
    juce::Justification justification,
    int maximumNumberOfLines,
    float minimumHorizontalScale = 0.0f)
{
    if (auto ctx = _ctx(g))
        ctx->drawFittedText(
            text,
            x, y,
            width, height,
            justification,
            maximumNumberOfLines,
            minimumHorizontalScale
        );
    else
        g.drawFittedText(
            text,
            x, y,
            width, height,
            justification,
            maximumNumberOfLines,
            minimumHorizontalScale
        );
}

static inline void _drawFittedText(
    const juce::Graphics &g,
    const juce::String &text,
    juce::Rectangle<int> area,
    juce::Justification justification,
    int maximumNumberOfLines,
    float minimumHorizontalScale = 0.0f)
{
    if (auto ctx = _ctx(g))
        ctx->drawFittedText(
            text,
            area,
            justification,
            maximumNumberOfLines,
            minimumHorizontalScale
        );
    else
        g.drawFittedText(
            text,
            area,
            justification,
            maximumNumberOfLines,
            minimumHorizontalScale
        );
}
