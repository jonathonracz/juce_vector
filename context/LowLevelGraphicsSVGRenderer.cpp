LowLevelGraphicsSVGRenderer::LowLevelGraphicsSVGRenderer(
    juce::XmlElement *svgDocument,
    const juce::String &documentTitle,
    int totalWidth,
    int totalHeight)
{
    stateStack.add(new SavedState());
    state = stateStack.getLast();

    state->clipRegions = juce::Rectangle<int>(totalWidth, totalHeight);
    state->clipPath = state->clipRegions.toPath();
    state->clipGroup = nullptr;

    document = svgDocument;

    jassert(document->getNumChildElements() == 0);

    document->setAttribute("xmlns", "http://www.w3.org/2000/svg");
    document->setAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");

    document->setAttribute("width", totalWidth);
    document->setAttribute("height", totalHeight);

    document->createNewChildElement("defs");
}

bool LowLevelGraphicsSVGRenderer::isVectorDevice() const
{
    return true;
}

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

float LowLevelGraphicsSVGRenderer::getPhysicalPixelScaleFactor()
{
    return 1.0f;
}

bool LowLevelGraphicsSVGRenderer::clipToRectangle(const juce::Rectangle<int> &r)
{
    state->clipRegions.clipTo(r.translated(state->xOffset, state->yOffset));

    setClip(state->clipRegions.toPath());

    return !isClipEmpty();
}

bool LowLevelGraphicsSVGRenderer::clipToRectangleList(const juce::RectangleList<int> &r)
{
    state->clipRegions.clipTo(r);

    setClip(state->clipRegions.toPath());

    return !isClipEmpty();
}

void LowLevelGraphicsSVGRenderer::excludeClipRectangle(const juce::Rectangle<int> &r)
{
    state->clipRegions.subtract(r.translated(state->xOffset, state->yOffset));

    setClip(state->clipRegions.toPath());
}

void LowLevelGraphicsSVGRenderer::clipToPath(const juce::Path &p, const juce::AffineTransform &t)
{
    auto temp = p;
    temp.applyTransform(t.translated(state->xOffset, state->yOffset));
    setClip(temp);
}

void LowLevelGraphicsSVGRenderer::clipToImageAlpha(const juce::Image &i, const juce::AffineTransform &t)
{
    juce::Image maskImage(i);

    if (i.getFormat() != juce::Image::SingleChannel)
        maskImage = i.convertedToFormat(juce::Image::SingleChannel);

    auto defs = document->getChildByName("defs");
    juce::String maskRef = juce::String::formatted("#Mask%d", defs->getNumChildElements());

    auto mask  = defs->createNewChildElement("mask");
    mask->setAttribute("id", maskRef.replace("#", ""));

    auto image = mask->createNewChildElement("image");
    image->setAttribute("x", state->xOffset);
    image->setAttribute("y", state->yOffset);
    image->setAttribute("width", i.getWidth());
    image->setAttribute("height", i.getHeight());

    if (!t.isIdentity())
        image->setAttribute("transform", matrix(state->transform.followedBy(t)));

    juce::MemoryOutputStream out;
    juce::PNGImageFormat png;
    png.writeImageToStream(i, out);

    juce::String base64Data = juce::Base64::toBase64(out.getData(), out.getDataSize());
    image->setAttribute("xlink:href", "data:image/png;base64," + base64Data);

    state->clipGroup = document->createNewChildElement("g");
    state->clipGroup->setAttribute("mask", "url(" + maskRef + ")");
}

bool LowLevelGraphicsSVGRenderer::clipRegionIntersects(const juce::Rectangle<int> &r)
{
    auto rect = r.translated(state->xOffset, state->yOffset).toFloat();
    return state->clipPath.getBounds().intersects(rect);
}

juce::Rectangle<int> LowLevelGraphicsSVGRenderer::getClipBounds() const
{
    return state->clipPath.getBounds().translated(-state->xOffset, -state->yOffset).toNearestInt();
}

bool LowLevelGraphicsSVGRenderer::isClipEmpty() const
{
    return state->clipPath.isEmpty();
}

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

        auto point1 = fill.gradient->point1.translated(state->xOffset, state->yOffset);
        auto point2 = fill.gradient->point2.translated(state->xOffset, state->yOffset);

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

                // FIXME: This will cause issues when prevRef is valid and gradients
                // start being reused when there isn't a current transform
                if (!state->transform.isIdentity())
                    stop->setAttribute("gradientTransform", matrix(state->transform));

                stop->setAttribute("stop-color", rgb(fill.gradient->getColour(i)));
                stop->setAttribute("stop-opacity", truncateFloat(fill.gradient->getColour(i).getFloatAlpha()));
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

void LowLevelGraphicsSVGRenderer::setInterpolationQuality(juce::Graphics::ResamplingQuality quality)
{

}

void LowLevelGraphicsSVGRenderer::fillRect(const juce::Rectangle<int> &r, bool replaceExistingContents)
{
    fillRect(r.toFloat());
}

void LowLevelGraphicsSVGRenderer::fillRect(const juce::Rectangle<float> &r)
{
    juce::XmlElement *rect;

    if (state->clipGroup)
        rect = state->clipGroup->createNewChildElement("rect");
    else
        rect = document->createNewChildElement("rect");

    rect->setAttribute("fill", fill());
    rect->setAttribute("fill-opacity", truncateFloat(state->fillType.getOpacity()));

    rect->setAttribute("x", truncateFloat(r.getX() + state->xOffset));
    rect->setAttribute("y", truncateFloat(r.getY() + state->yOffset));
    rect->setAttribute("width",  truncateFloat(r.getWidth()));
    rect->setAttribute("height", truncateFloat(r.getHeight()));
}

void LowLevelGraphicsSVGRenderer::fillRectList(const juce::RectangleList<float> &r)
{
    fillPath(r.toPath(), juce::AffineTransform());
}

void LowLevelGraphicsSVGRenderer::fillPath(const juce::Path &p, const juce::AffineTransform &t)
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
    path->setAttribute("fill", fill());
    path->setAttribute("fill-opacity", truncateFloat(state->fillType.getOpacity()));

    if (!p.isUsingNonZeroWinding())
        path->setAttribute("fill-rule", "evenodd");
}

void LowLevelGraphicsSVGRenderer::drawImage(const juce::Image &i, const juce::AffineTransform &t)
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

    if (!t.isIdentity())
        image->setAttribute("transform", matrix(state->transform.followedBy(t)));

    juce::MemoryOutputStream out;
    juce::PNGImageFormat png;
    png.writeImageToStream(i, out);

    juce::String base64Data = juce::Base64::toBase64(out.getData(), out.getDataSize());
    image->setAttribute("xlink:href", "data:image/png;base64," + base64Data);
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
    line->setAttribute("stroke", fill());
    line->setAttribute("stroke-opacity", truncateFloat(state->fillType.getOpacity()));

    if (!state->transform.isIdentity())
        line->setAttribute("transform", matrix(state->transform));
}

void LowLevelGraphicsSVGRenderer::setFont(const juce::Font &f)
{
    state->font = f;
}

const juce::Font& LowLevelGraphicsSVGRenderer::getFont()
{
    return state->font;
}

void LowLevelGraphicsSVGRenderer::drawGlyph(int glyphNumber, const juce::AffineTransform &t)
{
    juce::Path p;
    juce::Font &f = state->font;
    f.getTypeface()->getOutlineForGlyph(glyphNumber, p);
    auto glyphTransform = juce::AffineTransform::scale(f.getHeight() * f.getHorizontalScale(), f.getHeight()).followedBy(t);

    p.applyTransform(glyphTransform);

    fillPath(p, juce::AffineTransform());
}

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

juce::String LowLevelGraphicsSVGRenderer::matrix(const juce::AffineTransform &t)
{
    return juce::String::formatted(
        "matrix(%f,%f,%f,%f,%f,%f)",
        t.mat00, t.mat01, t.mat02,
        t.mat10, t.mat11, t.mat12
    );
}

juce::String LowLevelGraphicsSVGRenderer::rgb(const juce::Colour &c)
{
    return juce::String::formatted(
        "rgb(%d,%d,%d)",
        c.getRed(), c.getGreen(), c.getBlue()
    );
}

juce::String LowLevelGraphicsSVGRenderer::fill()
{
    if (state->fillType.isGradient())
        return "url(" + state->gradientRef + ")";
    else
        return rgb(state->fillType.colour);
}

juce::String LowLevelGraphicsSVGRenderer::truncateFloat(float value)
{
    auto string = juce::String(value, 2);

    while(string.getLastCharacters(1) == "." || (string.getLastCharacters(1) == "0" && string.contains(".")))
        string = string.dropLastCharacters(1);

    return string;
}

juce::String LowLevelGraphicsSVGRenderer::getPreviousGradientRef(juce::ColourGradient *g)
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

void LowLevelGraphicsSVGRenderer::setClip(const juce::Path &p)
{
    state->clipPath = p;

    auto defs = document->getChildByName("defs");
    juce::String clipRef = juce::String::formatted("#ClipPath%d", defs->getNumChildElements());

    auto clipPath = defs->createNewChildElement("clipPath");
    clipPath->setAttribute("id", clipRef.replace("#", ""));

    auto path = clipPath->createNewChildElement("path");
    path->setAttribute("d", state->clipPath.toString().toUpperCase());

    if (!state->transform.isIdentity())
        path->setAttribute(
            "transform",
            matrix(
                state->transform
            )
        );

    if (!state->clipGroup)
        state->clipGroup = document->createNewChildElement("g");

    state->clipGroup = state->clipGroup->createNewChildElement("g");

    state->clipGroup->setAttribute("clip-path", "url(" + clipRef + ")");
}
