# juce_vector

This modules contains a juce::LowLeveGraphicsContext subclass that renders to an SVG document.

** This module is a work in progress! **

## Usage

The context is useful for creating SVG snapshots of components to use in other programs.

The example below will render the component and all its children into an SVG document, with each child being contained in an SVG group.

```C++

XmlElement svg("svg");

LowLevelGraphicsSVGRenderer renderer(&svg, "", getWidth(), getHeight());
Graphics g(renderer);

paintEntireComponent(g, false);
```

### Grouping

You can control the grouping of SVG elements by using the `pushGroup()` and `popGroup()` methods.

The following will create a red rectangle inside of a group called 'Shapes' ( `<g id="Shapes">`):

```C++

renderer.pushGroup("Shapes");

g.setColour(juce::Colours::red);
g.fillRect(0, 0, 100, 100);

renderer.popGroup();
```

### Text

By default `juce::Graphics` text drawing methods will invoke the `drawGlyph()` method of the SVG context.

If you wish to have text rendered as a `<text>` element rather than paths, all text drawing methods
of `juce::Graphics` have a corresponding method here.

The text methods in this context will render `<text>` elements, with nested `<tspan>` elements
for multi-line text when needed.

### Macros

Preprocessor macros are a good way to be able to include SVG context commands in the same
drawing code that's used for the normal graphics context.

For example, you can create a macro to use the SVG context text methods if it's the current
low level context:

```C++

#define _drawSingleLineText(g, text, startX, baselineY)                                 \
            if (auto _ctx = dynamic_cast<LowLevelSVGRenderer*>(g.getInternalContext())) \
                _ctx.drawSingleLineText(text, startX, baselineY);                       \
            else                                                                        \
                g.drawSingleLineText(text, startX, baselineY);                          \

void paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::black);

    _drawSingleLineText(g, "foobar", 50, 10);
}
```


# License

Copyright 2018 Antonio Lassandro

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
