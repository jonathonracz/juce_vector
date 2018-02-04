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

# License

Copyright 2018 Antonio Lassandro

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
