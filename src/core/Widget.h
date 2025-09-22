#ifndef WIDGET_H
#define WIDGET_H

#include <Inkplate.h>
#include "LayoutRegion.h"

// Forward declaration to avoid circular dependency
class Compositor;

class Widget {
public:
    Widget(Inkplate& display) : display(display) {}
    virtual ~Widget() {}

    // Pure virtual methods that widgets must implement
    virtual void render(const LayoutRegion& region) = 0;
    virtual bool shouldUpdate() = 0;
    virtual void begin() = 0;

    // New compositor-based rendering method (default implementation for backward compatibility)
    virtual void renderToCompositor(Compositor& compositor, const LayoutRegion& region);

    // Virtual method for layout change notifications
    virtual void onRegionChanged(const LayoutRegion& oldRegion, const LayoutRegion& newRegion) {}

protected:
    Inkplate& display;

    // Helper methods for widgets (existing - for backward compatibility)
    void clearRegion(const LayoutRegion& region);
    void setClipRegion(const LayoutRegion& region);
    void resetClipRegion();

    // New compositor helper methods
    void clearRegionOnCompositor(Compositor& compositor, const LayoutRegion& region);
    void setClipRegionOnCompositor(Compositor& compositor, const LayoutRegion& region);
};

#endif
