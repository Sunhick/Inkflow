#ifndef DISPLAY_COMPOSITOR_H
#define DISPLAY_COMPOSITOR_H

#include <Inkplate.h>
#include "LayoutRegion.h"
#include <vector>

// Forward declarations
class Widget;

// Structure to track dirty regions for partial updates
struct DirtyRegion {
    LayoutRegion region;
    bool needsUpdate;

    DirtyRegion(const LayoutRegion& r) : region(r), needsUpdate(true) {}
};

// Virtual drawing surface for widgets
class VirtualSurface {
public:
    VirtualSurface(int width, int height);
    ~VirtualSurface();

    // Drawing operations that widgets can use
    void fillRect(int x, int y, int w, int h, int color);
    void drawLine(int x0, int y0, int x1, int y1, int color);
    void drawPixel(int x, int y, int color);
    void setCursor(int x, int y);
    void setTextSize(int size);
    void setTextColor(int color);
    void setTextWrap(bool wrap);
    void print(const char* text);
    void print(const String& text);

    // Surface management
    void clear();
    void clearRegion(const LayoutRegion& region);
    uint8_t* getBuffer() const { return buffer; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    uint8_t* buffer;
    int width;
    int height;
    int cursorX, cursorY;
    int textSize;
    int textColor;
    bool textWrap;

    // Helper methods
    void drawChar(int x, int y, char c, int color, int size);
    bool isValidCoordinate(int x, int y) const;
};

class DisplayCompositor {
public:
    DisplayCompositor(Inkplate& display);
    ~DisplayCompositor();

    // Initialization
    void begin();

    // Widget rendering interface
    VirtualSurface* beginWidgetRender(const LayoutRegion& region);
    void endWidgetRender(const LayoutRegion& region);

    // Composition and rendering
    void compose();
    void render();
    void renderPartial();

    // Dirty region management
    void markRegionDirty(const LayoutRegion& region);
    void clearDirtyRegions();
    bool hasUpdates() const;

    // Display mode management
    void setDisplayMode(int mode);
    int getDisplayMode() const;

    // Utility methods
    void clear();
    void drawLayoutBorders(const std::vector<LayoutRegion>& regions);

    // Access to underlying display for compatibility
    Inkplate& getDisplay() { return display; }

private:
    Inkplate& display;
    VirtualSurface* surface;
    std::vector<DirtyRegion> dirtyRegions;
    int currentDisplayMode;
    bool needsFullRender;

    // Private methods
    void copyRegionToDisplay(const LayoutRegion& region);
    void optimizeDirtyRegions();
    bool regionsOverlap(const LayoutRegion& a, const LayoutRegion& b);
    LayoutRegion mergeRegions(const LayoutRegion& a, const LayoutRegion& b);
};

#endif
