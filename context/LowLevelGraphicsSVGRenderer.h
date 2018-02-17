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

// =============================================================================
/**
    An implementation of juce::LowLevelGraphicsContext that turns the drawing
    operations into an SVG document.
*/
// =============================================================================
class LowLevelGraphicsSVGRenderer : public juce::LowLevelGraphicsContext
{
public:
    /** Creates a new SVG renderer.

        @param svgD
    */
    LowLevelGraphicsSVGRenderer(
        juce::XmlElement *svgDocument,
        int totalWidth, int totalHeight
    );

    #pragma mark -
    // =========================================================================

    bool isVectorDevice() const override;

    float getPhysicalPixelScaleFactor() override;

    #pragma mark -
    // =========================================================================

    /** Moves the origin to a new position.

        The coordinates are relative to the current origin, and indicate the new
        position of (0, 0).
    */
    void setOrigin(juce::Point<int>) override;

    /** Adds a transform to the context to apply to subsequent elements.

        When the context has a transform applied it will write it as an SVG
        transform attribute (e.g. <rect transform="matrix(...)">).

        The transform passed in will be applied to the current transform.
    */
    void addTransform(const juce::AffineTransform&) override;

    #pragma mark -
    // =========================================================================

    /** Intersects the current clipping region with another region.
    */
    bool clipToRectangle(const juce::Rectangle<int>&) override;

    /** Intersects the current clipping region with a rectangle list region.
    */
    bool clipToRectangleList(const juce::RectangleList<int>&) override;

    /** Excludes a rectangle to stop it being drawn into.
    */
    void excludeClipRectangle(const juce::Rectangle<int>&) override;

    /** Sets the clip region to a given path.

        NOTE: This currently will not intersect current regions unlike the
        rectangle clipping does.
    */
    void clipToPath(const juce::Path&, const juce::AffineTransform&) override;

    /** Applies an image mask to subsequent elements.
    */
    void clipToImageAlpha(const juce::Image&, const juce::AffineTransform&) override;

    /** Checks whether a rectangle overlaps the context's clipping region.
    */
    bool clipRegionIntersects(const juce::Rectangle<int>&) override;

    /** Returns the position of the bounding box for the current clipping
        region.
    */
    juce::Rectangle<int> getClipBounds() const override;

    /** Returns true if the clip region bounding box is empty.
    */
    bool isClipEmpty() const override;

    #pragma mark -
    // =========================================================================

    /** Saves the current graphics state on an internal stack.
    */
    void saveState() override;

    /** Restores a graphics state that was previously saved with saveState().
    */
    void restoreState() override;

    #pragma mark -
    // =========================================================================

    /** Applies an opacity value to the current juce::FillType.

        When an opacity is applied it will export as the attribute fill-opacity.
    */
    void beginTransparencyLayer(float) override;

    /** Resets the opacity of the current juce::FillType.
    */
    void endTransparencyLayer() override;

    /** Sets the current fill to use for elements.

        Gradient fills will have a <linearGradient> or <radialGradient> element
        created inside of <defs>, and any element filled with the gradient will
        use the attribute fill="#gradientRef".

        If a gradient's colours and stop positions match a previous gradient,
        it will use an xlink to the previous gradient rather than creating new
        <stop> tags.
    */
    void setFill(const juce::FillType&) override;

    /** Sets the opacity to use for (non-gradient) fills.

        When an opacity is applied it will export as the attribute fill-opacity.
    */
    void setOpacity(float) override;

    /** Sets the quality to draw images at.

        This method will map a juce::Graphics::ResamplingQuality to a relevant
        image-rendering SVG attribute:

        - ResamplingQuality::lowResamplingQuality maps to
          image-rendering="optimizeSpeed".

        - ResamplingQuality::mediumResamplingQuality maps to
          image-rendering="auto"

        - ResamplingQuality::highResamplingQuality maps to
          image-rendering="optimizeQuality"

        By default the interpolation quality is
        ResamplingQuality::mediumResamplingQuality, or image-rendering="auto".

        This value is independent of the state stack.
    */
    void setInterpolationQuality(juce::Graphics::ResamplingQuality) override;

    #pragma mark -
    // =========================================================================

    /** Draws a rectangle.

        The replaceExistingContents argument is required by
        juce::LowLevelGraphicsContext, but is unused in this implementation.
    */
    void fillRect(const juce::Rectangle<int>&, bool) override;

    /** Draws a rectangle.
    */
    void fillRect(const juce::Rectangle<float>&) override;

    /** Draws a rectangle list.

        Note, this will actually result in a single <path> element rather than
        individual <rect> elements.
    */
    void fillRectList(const juce::RectangleList<float>&) override;

    /** Draws a path.
    */
    void fillPath(const juce::Path&, const juce::AffineTransform&) override;

    /** Embeds an image into the document.

        The image-rendering attribute of the image will be set according to the
        value set by setInterpolationQuality().
    */
    void drawImage(const juce::Image&, const juce::AffineTransform&) override;

    /** Draws a line.
    */
    void drawLine(const juce::Line<float>&) override;

    #pragma mark -
    // =========================================================================

    /** Sets the font to use for subsequent text-drawing functions.
    */
    void setFont(const juce::Font&) override;

    /** Returns the current font
    */
    const juce::Font& getFont() override;


    /** Inserts a glyph as an SVG path transformed by a given
        juce::AffineTransform.

        @param glyphNumber the glyph number to use in the current typeface
    */
    void drawGlyph(int glyphNumber, const juce::AffineTransform&) override;

    #pragma mark -
    // =========================================================================

    /** Draws a one-line text string.

        This will use the current colour (or brush) to fill the text. The font
        is the last one specified by setFont().

        The text will be exported as a <text> element rather than a path.

        @param text          the string to draw
        @param startX        the position to draw the left-hand edge of the text
        @param baselineY     the position of the text's baseline
        @param justification the horizontal flags indicate which end of the text
                             string is anchored at the specified point.
    */
    void drawSingleLineText(
        const juce::String&,
        int startX,
        int baselineY,
        juce::Justification = juce::Justification::left
    );

    /** Draws text across multiple lines.

        This will break the text onto a new line where there's a new-line or
        carriage-return character, or at a word-boundary when the text becomes
        wider than the size specified by the maximumLineWidth parameter.

        The text will be exported as a <text> element, with nested <tspan>
        elements, rather than a path.
    */
    void drawMultiLineText(
        const juce::String&,
        int startX,
        int baselineY,
        int maximumLineWidth
    );

    /** Draws a line of text within a specified rectangle.

        The text will be positioned within the rectangle based on the
        justification flags passed-in. If the string is too long to fit inside
        the rectangle, it will either be truncated or will have ellipsis added
        to its end (if the useEllipsesIfTooBig flag is true).

        The text will be exported as a <text> element rather than a path.
    */
    void drawText(
        const juce::String&,
        int x,
        int y,
        int width,
        int height,
        juce::Justification,
        bool useEllipsesIfTooBig = true
    );

    /** Draws a line of text within a specified rectangle.

        The text will be positioned within the rectangle based on the
        justification flags passed-in. If the string is too long to fit inside
        the rectangle, it will either be truncated or will have ellipsis added
        to its end (if the useEllipsesIfTooBig flag is true).

        The text will be exported as a <text> element rather than a path.
    */
    void drawText(
        const juce::String&,
        juce::Rectangle<int>,
        juce::Justification,
        bool useEllipsesIfTooBig = true
    );

    /** Draws a line of text within a specified rectangle.

        The text will be positioned within the rectangle based on the
        justification flags passed-in. If the string is too long to fit inside
        the rectangle, it will either be truncated or will have ellipsis added
        to its end (if the useEllipsesIfTooBig flag is true).

        The text will be exported as a <text> element rather than a path.
    */
    void drawText(
        const juce::String&,
        juce::Rectangle<float>,
        juce::Justification,
        bool useEllipsesIfTooBig = true
    );

    /** Tries to draw a text string inside a given space.

        If the text is too big, it'll be squashed horizontally or broken over
        multiple lines if the maximumLinesToUse value allows this. If the text
        just won't fit into the space, it'll fit as much as possible inside, and
        put some ellipsis at the end to show that it's been truncated.

        A Justification parameter lets you specify how the text is laid out
        within the rectangle, both horizontally and vertically.

        The minimumHorizontalScale parameter specifies how much the text can be
        squashed horizontally to try to squeeze it into the space. If you don't
        want any horizontal scaling to occur, you can set this value to 1.0f.
        Pass 0 if you want it to use a default value.

        The text will be exported as a <text> element rather than a path.
        Nested <tspan> elements will be used if the text is broken up into
        multiple lines.
    */
    void drawFittedText(
        const juce::String&,
        int x,
        int y,
        int width,
        int height,
        juce::Justification,
        int maximumNumberOfLines,
        float minimumHorizontalScale = 0.0f
    );

    /** Tries to draw a text string inside a given space.

        If the text is too big, it'll be squashed horizontally or broken over
        multiple lines if the maximumLinesToUse value allows this. If the text
        just won't fit into the space, it'll fit as much as possible inside, and
        put some ellipsis at the end to show that it's been truncated.

        A Justification parameter lets you specify how the text is laid out
        within the rectangle, both horizontally and vertically.

        The minimumHorizontalScale parameter specifies how much the text can be
        squashed horizontally to try to squeeze it into the space. If you don't
        want any horizontal scaling to occur, you can set this value to 1.0f.
        Pass 0 if you want it to use a default value.

        The text will be exported as a <text> element rather than a path.
        Nested <tspan> elements will be used if the text is broken up into
        multiple lines.
    */
    void drawFittedText(
        const juce::String&,
        juce::Rectangle<int>,
        juce::Justification,
        int maximumNumberOfLines,
        float minimumHorizontalScale = 0.0f
    );

    #pragma mark -
    // =========================================================================

    /** Pushes a group onto the document.

        All drawing done after this command will be under the group tag.

        If the context is clipped, the newly pushed group will be within the
        clip group.
    */
    void pushGroup(const juce::String&);

    /** Pops the current SVG group.

        This command is only valid after a pushGroup().

        If the current group being used is empty it will be
        removed from the document entirely.

        This command does not effect clip groups.
    */
    void popGroup();

    /** Sets the attributes to apply to every drawing command following
        setTags().

        Tags are applied at the end of the creation of an element, so attributes
        provided to this method may overwrite element attributes of the same
        name.
    */
    void setTags(const juce::StringPairArray&);

    /** Clears any custom tags from being applied.
    */
    void clearTags();

#pragma mark - 
// =============================================================================
private:
    juce::String truncateFloat(float);

    juce::String getPreviousGradientRef(juce::ColourGradient*);

    juce::String writeTransform(const juce::AffineTransform&);
    juce::String writeColour(const juce::Colour&);
    juce::String writeFill();
    juce::String writeImageQuality();

    void applyTags(juce::XmlElement*);

    #pragma mark -
    // =========================================================================

    void applyTextPos(
        juce::XmlElement*,
        int x,
        int y,
        const int width,
        const int height,
        const juce::Justification&
    );

    void setClip(const juce::Path&);

    #pragma mark -
    // =========================================================================

    struct SavedState
    {
        SavedState() { xOffset = 0; yOffset = 0; };
        SavedState& operator=(const SavedState&) = delete;
        ~SavedState() {};

        int xOffset, yOffset;
        juce::RectangleList<int> clipRegions;
        juce::Path clipPath;

        juce::XmlElement *clipGroup;

        juce::AffineTransform transform;

        juce::FillType fillType;
        juce::String gradientRef;

        juce::Font font;

        juce::StringPairArray tags;
    };

    struct GradientRef
    {
        juce::ColourGradient gradient;
        juce::String ref;
    };

    juce::OwnedArray<SavedState> stateStack;
    SavedState* state;

    juce::Array<GradientRef> previousGradients;

    juce::Graphics::ResamplingQuality resampleQuality;

    juce::XmlElement *document;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowLevelGraphicsSVGRenderer)
};
