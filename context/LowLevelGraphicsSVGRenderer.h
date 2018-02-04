#pragma once

class LowLevelGraphicsSVGRenderer : public juce::LowLevelGraphicsContext
{
public:
    LowLevelGraphicsSVGRenderer(juce::XmlElement *svgDocument, const juce::String &documentTitle, int totalWidth, int totalHeight);

    ~LowLevelGraphicsSVGRenderer() {};

    bool isVectorDevice() const override;
    void setOrigin(juce::Point<int>) override;
    void addTransform(const juce::AffineTransform&) override;
    float getPhysicalPixelScaleFactor() override;

    bool clipToRectangle(const juce::Rectangle<int>&) override;
    bool clipToRectangleList(const juce::RectangleList<int>&) override;
    void excludeClipRectangle(const juce::Rectangle<int>&) override;
    void clipToPath(const juce::Path&, const juce::AffineTransform&) override;
    void clipToImageAlpha(const juce::Image&, const juce::AffineTransform&) override;
    bool clipRegionIntersects(const juce::Rectangle<int>&) override;
    juce::Rectangle<int> getClipBounds() const override;
    bool isClipEmpty() const override;

    void saveState() override;
    void restoreState() override;

    void beginTransparencyLayer(float) override;
    void endTransparencyLayer() override;

    void setFill(const juce::FillType&) override;
    void setOpacity(float) override;
    void setInterpolationQuality(juce::Graphics::ResamplingQuality) override;

    void fillRect(const juce::Rectangle<int>&, bool replaceExistingContents) override;
    void fillRect(const juce::Rectangle<float>&) override;
    void fillRectList(const juce::RectangleList<float>&) override;
    void fillPath(const juce::Path&, const juce::AffineTransform&) override;
    void drawImage(const juce::Image&, const juce::AffineTransform&) override;
    void drawLine(const juce::Line<float>&) override;

    void setFont(const juce::Font&) override;
    const juce::Font& getFont() override;
    void drawGlyph(int glyphNumber, const juce::AffineTransform&) override;

private:
    // String format helpers
    juce::String matrix(const juce::AffineTransform&);
    juce::String rgb(const juce::Colour&);
    juce::String fill();

    void setClip();
    void setMask(const juce::Image&);

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
    };

    juce::OwnedArray<SavedState> stateStack;
    SavedState* state;

    juce::XmlElement *document;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowLevelGraphicsSVGRenderer)
};
