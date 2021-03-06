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

LowLevelGraphicsSVGRenderer::LowLevelGraphicsSVGRenderer(
    juce::XmlElement *svgDocument,
    int totalWidth,
    int totalHeight)
{
    stateStack.add(new SavedState());
    state = stateStack.getLast();

    state->clipRegions = juce::Rectangle<int>(totalWidth, totalHeight);
    state->clipPath = state->clipRegions.toPath();
    state->clipGroup = nullptr;

    resampleQuality = juce::Graphics::mediumResamplingQuality;

    document = svgDocument;

    // XmlElements that don't have the proper name or that already have children
    // will yield unusable or undefined results
    jassert(document->getTagName().toLowerCase() == "svg");
    jassert(document->getNumChildElements() == 0);

    document->setAttribute("xmlns", "http://www.w3.org/2000/svg");
    document->setAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");

    document->setAttribute("width", totalWidth);
    document->setAttribute("height", totalHeight);

    document->createNewChildElement("defs");
}

#pragma mark -
// =============================================================================

bool LowLevelGraphicsSVGRenderer::isVectorDevice() const
{
    return true;
}

float LowLevelGraphicsSVGRenderer::getPhysicalPixelScaleFactor()
{
    return 1.0f;
}

#pragma mark - 
// =============================================================================

void LowLevelGraphicsSVGRenderer::setOrigin(juce::Point<int> p)
{
    if (p.x != state->xOffset || p.y != state->yOffset)
    {
        state->xOffset += p.x;
        state->yOffset += p.y;
        setClip(state->clipPath);
    }
}

void LowLevelGraphicsSVGRenderer::addTransform(const juce::AffineTransform &t)
{
    state->transform = state->transform.followedBy(t);

    state->clipRegions.transformAll(t);
    state->clipPath.applyTransform(t);

    setClip(state->clipRegions.toPath());
}

#pragma mark -
// =============================================================================

bool LowLevelGraphicsSVGRenderer::clipToRectangle(const juce::Rectangle<int> &r)
{
    state->clipRegions.clipTo(r.translated(state->xOffset, state->yOffset));

    setClip(state->clipRegions.toPath());

    return !isClipEmpty();
}

bool LowLevelGraphicsSVGRenderer::clipToRectangleList(
    const juce::RectangleList<int> &r)
{
    state->clipRegions.clipTo(r);

    setClip(state->clipRegions.toPath());

    return !isClipEmpty();
}

void LowLevelGraphicsSVGRenderer::excludeClipRectangle(
    const juce::Rectangle<int> &r)
{
    state->clipRegions.subtract(r.translated(state->xOffset, state->yOffset));

    setClip(state->clipRegions.toPath());
}

void LowLevelGraphicsSVGRenderer::clipToPath(
    const juce::Path &p,
    const juce::AffineTransform &t)
{
    auto temp = p;
    temp.applyTransform(t.translated(state->xOffset, state->yOffset));
    setClip(temp);
}

void LowLevelGraphicsSVGRenderer::clipToImageAlpha(
    const juce::Image &i,
    const juce::AffineTransform &t)
{
    juce::Image maskImage(i);

    if (i.getFormat() != juce::Image::SingleChannel)
        maskImage = i.convertedToFormat(juce::Image::SingleChannel);

    auto defs = document->getChildByName("defs");
    auto maskRef = juce::String::formatted(
        "#Mask%d",
        defs->getNumChildElements()
    );

    auto mask  = defs->createNewChildElement("mask");
    mask->setAttribute("id", maskRef.replace("#", ""));

    auto image = mask->createNewChildElement("image");
    image->setAttribute("x", state->xOffset);
    image->setAttribute("y", state->yOffset);
    image->setAttribute("width", i.getWidth());
    image->setAttribute("height", i.getHeight());

    image->setAttribute("image-rendering", writeImageQuality());

    if (!t.isIdentity())
        image->setAttribute(
            "transform",
            writeTransform(state->transform.followedBy(t))
        );

    juce::MemoryOutputStream out;
    juce::PNGImageFormat png;
    png.writeImageToStream(i, out);

    auto base64Data = juce::Base64::toBase64(out.getData(), out.getDataSize());

    image->setAttribute("xlink:href", "data:image/png;base64," + base64Data);

    state->clipGroup = document->createNewChildElement("g");
    state->clipGroup->setAttribute("mask", "url(" + maskRef + ")");
}

bool LowLevelGraphicsSVGRenderer::clipRegionIntersects(
    const juce::Rectangle<int> &r)
{
    auto rect = r.translated(state->xOffset, state->yOffset).toFloat();
    return state->clipPath.getBounds().intersects(rect);
}

juce::Rectangle<int> LowLevelGraphicsSVGRenderer::getClipBounds() const
{
    return state->clipPath.getBounds()
        .translated(-state->xOffset, -state->yOffset).toNearestInt();
}

bool LowLevelGraphicsSVGRenderer::isClipEmpty() const
{
    return state->clipPath.isEmpty();
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::saveState()
{
    stateStack.add(new SavedState(*stateStack.getLast()));
    state = stateStack.getLast();
}

void LowLevelGraphicsSVGRenderer::restoreState()
{
    jassert(stateStack.size() > 0);
    stateStack.removeLast();
    state = stateStack.getLast();
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::beginTransparencyLayer(float opacity)
{
    state->fillType.setOpacity(opacity);
}

void LowLevelGraphicsSVGRenderer::endTransparencyLayer()
{
    state->fillType.setOpacity(1.0f);
}

void LowLevelGraphicsSVGRenderer::setFill(const juce::FillType &fill)
{
    state->fillType = fill;

    if (fill.isGradient())
    {
        auto defs = document->getChildElement(0);
        jassert(defs);

        juce::String gradientType = (fill.gradient->isRadial)
            ? "radialGradient"
            : "linearGradient";

        auto e = defs->createNewChildElement(gradientType);

        state->gradientRef = juce::String::formatted(
            "#Gradient%d",
            defs->getNumChildElements()
        );

        e->setAttribute("id", state->gradientRef.replace("#", ""));

        e->setAttribute("gradientUnits", "userSpaceOnUse");

        auto point1 = fill.gradient->point1
            .translated(state->xOffset, state->yOffset);

        auto point2 = fill.gradient->point2
            .translated(state->xOffset, state->yOffset);

        if (fill.gradient->isRadial)
        {
            e->setAttribute("cx", truncateFloat(point1.x));
            e->setAttribute("cy", truncateFloat(point1.y));
            e->setAttribute("r",  truncateFloat(point1.getDistanceFrom(point2)));
            e->setAttribute("fx", truncateFloat(point2.x));
            e->setAttribute("fy", truncateFloat(point2.y));
        }
        else
        {
            e->setAttribute("x1", truncateFloat(point1.x));
            e->setAttribute("y1", truncateFloat(point1.y));
            e->setAttribute("x2", truncateFloat(point2.x));
            e->setAttribute("y2", truncateFloat(point2.y));
        }

        if (!state->transform.isIdentity())
            e->setAttribute(
                "gradientTransform",
                writeTransform(state->transform)
            );

        auto prevRef = getPreviousGradientRef(fill.gradient);

        if (prevRef.isNotEmpty())
        {
            e->setAttribute("xlink:href", prevRef);
        }
        else
        {
            for (int i = 0; i < fill.gradient->getNumColours(); ++i)
            {
                auto stop = e->createNewChildElement("stop");
                stop->setAttribute(
                    "offset",
                    truncateFloat((float)fill.gradient->getColourPosition(i))
                );

                stop->setAttribute(
                    "stop-color",
                    writeColour(fill.gradient->getColour(i))
                );

                stop->setAttribute(
                    "stop-opacity",
                    truncateFloat(fill.gradient->getColour(i).getFloatAlpha())
                );
            }
        }
    }
    else
    {
        state->gradientRef = "";
    }
}

void LowLevelGraphicsSVGRenderer::setOpacity(float opacity)
{
    state->fillType.setOpacity(opacity);
}

void LowLevelGraphicsSVGRenderer::setInterpolationQuality(
    juce::Graphics::ResamplingQuality quality)
{
    resampleQuality = quality;
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::fillRect(
    const juce::Rectangle<int> &r,
    bool replaceExistingContents)
{
    // TODO: Utilize replaceExistingContents
    fillRect(r.toFloat());
}

void LowLevelGraphicsSVGRenderer::fillRect(const juce::Rectangle<float> &r)
{
    juce::XmlElement *rect;

    if (state->clipGroup)
        rect = state->clipGroup->createNewChildElement("rect");
    else
        rect = document->createNewChildElement("rect");

    rect->setAttribute("fill", writeFill());
    rect->setAttribute(
        "fill-opacity",
        truncateFloat(state->fillType.getOpacity())
    );

    rect->setAttribute("x", truncateFloat(r.getX() + state->xOffset));
    rect->setAttribute("y", truncateFloat(r.getY() + state->yOffset));
    rect->setAttribute("width",  truncateFloat(r.getWidth()));
    rect->setAttribute("height", truncateFloat(r.getHeight()));

    applyTags(rect);
}

void LowLevelGraphicsSVGRenderer::fillRectList(
    const juce::RectangleList<float> &r)
{
    fillPath(r.toPath(), juce::AffineTransform());
}

void LowLevelGraphicsSVGRenderer::fillPath(
    const juce::Path &p,
    const juce::AffineTransform &t)
{
    juce::XmlElement *path;

    if (state->clipGroup)
        path = state->clipGroup->createNewChildElement("path");
    else
        path = document->createNewChildElement("path");

    auto temp = p;
    temp.applyTransform(t.translated(state->xOffset, state->yOffset));

    juce::String d = temp.toString().removeCharacters("a");
    path->setAttribute("d", d.toUpperCase());

    path->setAttribute("fill", writeFill());
    path->setAttribute(
        "fill-opacity",
        truncateFloat(state->fillType.getOpacity())
    );

    if (!p.isUsingNonZeroWinding())
        path->setAttribute("fill-rule", "evenodd");

    applyTags(path);
}

void LowLevelGraphicsSVGRenderer::drawImage(
    const juce::Image &i,
    const juce::AffineTransform &t)
{
    juce::XmlElement *image;

    if (state->clipGroup)
        image = state->clipGroup->createNewChildElement("image");
    else
        image = document->createNewChildElement("image");

    image->setAttribute("x", state->xOffset);
    image->setAttribute("y", state->yOffset);
    image->setAttribute("width", i.getWidth());
    image->setAttribute("height", i.getHeight());

    image->setAttribute("image-rendering", writeImageQuality());

    if (!t.isIdentity())
        image->setAttribute(
            "transform",
            writeTransform(state->transform.followedBy(t))
        );

    juce::MemoryOutputStream out;
    juce::PNGImageFormat png;
    png.writeImageToStream(i, out);

    auto base64Data = juce::Base64::toBase64(out.getData(), out.getDataSize());
    image->setAttribute("xlink:href", "data:image/png;base64," + base64Data);

    applyTags(image);
}

void LowLevelGraphicsSVGRenderer::drawLine(const juce::Line<float> &l)
{
    juce::XmlElement *line;

    if (state->clipGroup)
        line = state->clipGroup->createNewChildElement("line");
    else
        line = document->createNewChildElement("line");

    line->setAttribute("x1", truncateFloat(l.getStartX() + state->xOffset));
    line->setAttribute("y1", truncateFloat(l.getStartY() + state->yOffset));
    line->setAttribute("x2", truncateFloat(l.getEndX()   + state->xOffset));
    line->setAttribute("y2", truncateFloat(l.getEndY()   + state->yOffset));

    line->setAttribute("stroke", writeFill());
    line->setAttribute(
        "stroke-opacity",
        truncateFloat(state->fillType.getOpacity())
    );

    if (!state->transform.isIdentity())
        line->setAttribute("transform", writeTransform(state->transform));

    applyTags(line);
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::setFont(const juce::Font &f)
{
    state->font = f;
}

const juce::Font& LowLevelGraphicsSVGRenderer::getFont()
{
    return state->font;
}

void LowLevelGraphicsSVGRenderer::drawGlyph(
    int glyphNumber,
    const juce::AffineTransform &t)
{
    juce::Path p;
    juce::Font &f = state->font;
    f.getTypeface()->getOutlineForGlyph(glyphNumber, p);

    auto glyphTransform = juce::AffineTransform::scale(
        f.getHeight() * f.getHorizontalScale(),
        f.getHeight()
    ).followedBy(t);

    p.applyTransform(glyphTransform);

    fillPath(p, juce::AffineTransform());
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::drawSingleLineText(
    const juce::String &t,
    int startX,
    int baselineY,
    juce::Justification justification)
{
    juce::XmlElement *text;

    if (state->clipGroup)
        text = state->clipGroup->createNewChildElement("text");
    else
        text = document->createNewChildElement("text");

    auto f = state->font;
    auto tf = f.getTypeface();

    text->setAttribute("x", startX);
    text->setAttribute("y", baselineY - f.getHeight());
    text->setAttribute("font-family", tf->getName());
    text->setAttribute("font-style", tf->getStyle());
    text->setAttribute("font-size", f.getHeight());
    text->setAttribute("fill", writeFill());

    if (justification.testFlags(justification.left))
        text->setAttribute("text-anchor", "start");

    else if (justification.testFlags(justification.horizontallyCentred))
        text->setAttribute("text-anchor", "middle");

    else if (justification.testFlags(justification.right))
        text->setAttribute("text-anchor", "end");

    else
        text->setAttribute("text-anchor", "inherited");


    if (!state->transform.isIdentity())
        text->setAttribute("transform", writeTransform(state->transform));

    text->addTextElement(t);

    applyTags(text);
}

void LowLevelGraphicsSVGRenderer::drawMultiLineText(
    const juce::String &t,
    int startX,
    int baselineY,
    int maximumLineWidth)
{
    juce::XmlElement *text;

    if (state->clipGroup)
        text = state->clipGroup->createNewChildElement("text");
    else
        text = document->createNewChildElement("text");

    auto f = state->font;
    auto tf = f.getTypeface();

    text->setAttribute("x", startX);
    text->setAttribute("y", baselineY - f.getHeight());
    text->setAttribute("font-family", tf->getName());
    text->setAttribute("font-style", tf->getStyle());
    text->setAttribute("font-size", f.getHeight());
    text->setAttribute("fill", writeFill());

    if (!state->transform.isIdentity())
        text->setAttribute("transform", writeTransform(state->transform));

    auto t2 = t;

    while (t2.isNotEmpty())
    {
        int len = f.getStringWidth(t2);

        if (len > maximumLineWidth)
        {
            auto line = t2;

            int i = 0;
            while (f.getStringWidth(line) > maximumLineWidth)
            {
                line = line.dropLastCharacters(1);
                i++;
            }

            auto tspan = text->createNewChildElement("tspan");
            tspan->setAttribute("x", startX);
            tspan->setAttribute("y", baselineY);
            tspan->addTextElement(line);

            t2 = t2.substring(i);

            baselineY += (int)f.getHeight();
        }
        else
        {
            auto tspan = text->createNewChildElement("tspan");
            tspan->setAttribute("x", startX);
            tspan->setAttribute("y", baselineY);
            tspan->addTextElement(t2);

            t2 = "";
        }
    }

    applyTags(text);
}

void LowLevelGraphicsSVGRenderer::drawText(
    const juce::String &t,
    int x,
    int y,
    int width,
    int height,
    juce::Justification justification,
    bool useEllipsesIfTooBig)
{
    juce::XmlElement *text;

    if (state->clipGroup)
        text = state->clipGroup->createNewChildElement("text");
    else
        text = document->createNewChildElement("text");

    auto f = state->font;
    auto tf = f.getTypeface();

    applyTextPos(text, x, y, width, height, justification);

    text->setAttribute("font-family", tf->getName());
    text->setAttribute("font-style", tf->getStyle());
    text->setAttribute("font-size", f.getHeight());
    text->setAttribute("fill", writeFill());

    if (!state->transform.isIdentity())
        text->setAttribute("transform", writeTransform(state->transform));

    auto t2 = t;

    auto len = f.getStringWidth(t);
    if (len > width)
    {
        juce::String ellipses = (useEllipsesIfTooBig)
            ? juce::String(juce::CharPointer_UTF8("\xe2\x80\xa6"))
            : "";

        while (f.getStringWidth(t2 + ellipses) > width)
        {
            t2 = t2.dropLastCharacters(1);
        }

        t2 += ellipses;
    }

    text->addTextElement(t2);

    applyTags(text);
}

void LowLevelGraphicsSVGRenderer::drawText(
    const juce::String &t,
    juce::Rectangle<int> area,
    juce::Justification justification,
    bool useEllipsesIfTooBig)
{
    drawText(
        t,
        area.getX(),
        area.getY(),
        area.getWidth(),
        area.getHeight(),
        justification,
        useEllipsesIfTooBig
    );
}

void LowLevelGraphicsSVGRenderer::drawText(
    const juce::String &t,
    juce::Rectangle<float> area,
    juce::Justification justification,
    bool useEllipsesIfTooBig)
{
    drawText(
        t,
        (int)area.getX(),
        (int)area.getY(),
        (int)area.getWidth(),
        (int)area.getHeight(),
        justification,
        useEllipsesIfTooBig
    );
}

void LowLevelGraphicsSVGRenderer::drawFittedText(
    const juce::String &t,
    int x,
    int y,
    int width,
    int height,
    juce::Justification justification,
    int maximumNumberOfLines,
    float minimumHorizontalScale)
{
    juce::XmlElement *text;

    if (state->clipGroup)
        text = state->clipGroup->createNewChildElement("text");
    else
        text = document->createNewChildElement("text");

    auto f = state->font;
    auto tf = f.getTypeface();

    applyTextPos(text, x, y, width, height, justification);

    // TODO: support minimumHorizontalScale values
    if (minimumHorizontalScale == 0.0f)
    {
        text->setAttribute("textLength", width);
        text->setAttribute("lengthAdjust", "spacingAndGlyphs");
    }

    text->setAttribute("font-family", tf->getName());
    text->setAttribute("font-style", tf->getStyle());
    text->setAttribute("font-size", f.getHeight());
    text->setAttribute("fill", writeFill());

    auto t2 = t;

    if (maximumNumberOfLines > 1)
    {
        while (t2.isNotEmpty())
        {
            int len = t2.length();

            if (len > width)
            {
                auto line = t2;

                int i = 0;
                while (f.getStringWidth(line) > width)
                {
                    line = line.dropLastCharacters(1);
                    i++;
                }

                auto tspan = text->createNewChildElement("tspan");
                tspan->setAttribute("x", x);
                tspan->setAttribute("y", y);
                tspan->setAttribute("text-anchor", "auto");
                tspan->setAttribute("dominant-baseline", "auto");
                tspan->addTextElement(line);

                t2 = t2.substring(i);

                y += (int)f.getHeight();
            }
            else
            {
                auto tspan = text->createNewChildElement("tspan");
                tspan->setAttribute("x", x);
                tspan->setAttribute("y", y);
                tspan->setAttribute("text-anchor", "auto");
                tspan->setAttribute("dominant-baseline", "auto");
                tspan->addTextElement(t2);

                t2 = "";
            }
        }
    }
    else
    {
        auto len = f.getStringWidth(t2);
        if (len > width)
        {
            while (f.getStringWidth(t2) > width)
            {
                t2 = t2.dropLastCharacters(1);
            }
        }
        text->addTextElement(t2);
    }

    applyTags(text);
}

void LowLevelGraphicsSVGRenderer::drawFittedText(
    const juce::String &t,
    juce::Rectangle<int> area,
    juce::Justification justification,
    int maximumNumberOfLines,
    float minimumHorizontalScale)
{
    drawFittedText(
        t,
        area.getX(),
        area.getY(),
        area.getWidth(),
        area.getHeight(),
        justification,
        maximumNumberOfLines,
        minimumHorizontalScale
    );
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::pushGroup(const juce::String& groupID)
{
    if (!state->clipGroup)
        state->clipGroup = document->createNewChildElement("g");
    else
        state->clipGroup = state->clipGroup->createNewChildElement("g");

    state->clipGroup->setAttribute("id", groupID);
}

void LowLevelGraphicsSVGRenderer::popGroup()
{
    jassert(state->clipGroup);

    if (state->clipGroup->hasAttribute("id"))
    {
        auto temp = state->clipGroup;

        state->clipGroup = document->findParentElementOf(state->clipGroup);

        if (temp->getNumChildElements() == 0)
            state->clipGroup->removeChildElement(temp, true);
    }
    else
    {
        jassertfalse;  // More popGroup() calls than pushGroup()!
    }
}

void LowLevelGraphicsSVGRenderer::setTags(const juce::StringPairArray &s)
{
    state->tags = s;
}

void LowLevelGraphicsSVGRenderer::clearTags()
{
    state->tags.clear();
}

#pragma mark -
// =============================================================================

juce::String LowLevelGraphicsSVGRenderer::truncateFloat(float value)
{
    auto string = juce::String(value, 2);

    while (string.getLastCharacters(1) == "." ||
          (string.getLastCharacters(1) == "0" && string.contains(".")))
            string = string.dropLastCharacters(1);

    return string;
}

juce::String LowLevelGraphicsSVGRenderer::getPreviousGradientRef(
    juce::ColourGradient *g)
{
    if (previousGradients.size() == 0)
    {
        jassert(state->gradientRef.isNotEmpty());

        GradientRef newRef;
        newRef.gradient = *g;
        newRef.ref = state->gradientRef;

        previousGradients.add(newRef);
        return "";
    }

    for (auto r : previousGradients)
    {
        auto previousGradient = &r.gradient;

        if (previousGradient->getNumColours() != g->getNumColours())
            continue;

        bool match = false;

        for (int i = 0; i < previousGradient->getNumColours(); ++i)
        {
            auto colour = previousGradient->getColour(i);
            auto pos    = previousGradient->getColourPosition(i);

            if (g->getColour(i) != colour)
                break;

            if (g->getColourPosition(i) != pos)
                break;

            match = true;
        }

        if (match)
            return r.ref;
    }

    jassert(state->gradientRef.isNotEmpty());

    GradientRef newRef;
    newRef.gradient = *g;
    newRef.ref = state->gradientRef;

    previousGradients.add(newRef);
    return "";
}

juce::String LowLevelGraphicsSVGRenderer::writeTransform(
    const juce::AffineTransform &t)
{
    return juce::String::formatted(
        "matrix(%f,%f,%f,%f,%f,%f)",
        t.mat00, t.mat01, t.mat02,
        t.mat10, t.mat11, t.mat12
    );
}

juce::String LowLevelGraphicsSVGRenderer::writeColour(const juce::Colour &c)
{
    return juce::String::formatted(
        "rgb(%d,%d,%d)",
        c.getRed(), c.getGreen(), c.getBlue()
    );
}

juce::String LowLevelGraphicsSVGRenderer::writeFill()
{
    if (state->fillType.isGradient())
        return "url(" + state->gradientRef + ")";
    else
        return writeColour(state->fillType.colour);
}

juce::String LowLevelGraphicsSVGRenderer::writeImageQuality()
{
    switch (resampleQuality)
    {
        case juce::Graphics::lowResamplingQuality:
            return "optimizeSpeed";

        case juce::Graphics::mediumResamplingQuality:
            return "auto";

        case juce::Graphics::highResamplingQuality:
            return "optimizeQuality";
    }
}

void LowLevelGraphicsSVGRenderer::applyTags(juce::XmlElement *e)
{
    if (state->tags.size() == 0)
        return;

    auto keys   = state->tags.getAllKeys();
    auto values = state->tags.getAllValues();

    for (int i = 0; i < state->tags.size(); ++i)
        e->setAttribute(keys[i], values[i]);
}

#pragma mark -
// =============================================================================

void LowLevelGraphicsSVGRenderer::applyTextPos(
    juce::XmlElement *text,
    int x, int y,
    const int width, const int height,
    const juce::Justification &j)
{
    if (j.testFlags(j.horizontallyCentred))
    {
        text->setAttribute("text-anchor", "middle");
        x += width / 2;
    }
    else if (j.testFlags(j.right))
    {
        text->setAttribute("text-anchor", "end");
        x += width;
    }
    else
    {
        text->setAttribute("text-anchor", "start");
    }

    if (j.testFlags(j.verticallyCentred))
    {
        text->setAttribute("dominant-baseline", "central");
        y += height / 2;
    }
    else if (j.testFlags(j.bottom))
    {
        text->setAttribute("dominant-baseline", "ideographic");
        y += height;
    }
    else
    {
        text->setAttribute("dominant-baseline", "hanging");
    }

    text->setAttribute("x", x);
    text->setAttribute("y", y);
}

void LowLevelGraphicsSVGRenderer::setClip(const juce::Path &p)
{
    state->clipPath = p;

    auto defs = document->getChildByName("defs");
    auto clipRef = juce::String::formatted(
        "#ClipPath%d",
        defs->getNumChildElements()
    );

    auto clipPath = defs->createNewChildElement("clipPath");
    clipPath->setAttribute("id", clipRef.replace("#", ""));

    auto path = clipPath->createNewChildElement("path");
    path->setAttribute("d", state->clipPath.toString().toUpperCase());

    if (!state->transform.isIdentity())
        path->setAttribute(
            "transform",
            writeTransform(
                state->transform
            )
        );

    if (!state->clipGroup)
        state->clipGroup = document->createNewChildElement("g");

    state->clipGroup = state->clipGroup->createNewChildElement("g");

    state->clipGroup->setAttribute("clip-path", "url(" + clipRef + ")");
}
