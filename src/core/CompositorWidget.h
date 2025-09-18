#ifndef COMPOSITOR_WIDGET_H
#define COMPOSITOR_WIDGET_H

#include "Widget.h"
#include "DisplayCompositor.h"

// Adapter class that allows widgets to work with the compositor
// while maintaining compatibility with the existing Widget interface
class CompositorWidget : public Widget {
public:
    CompositorWidget(DisplayCompositor& compositor);
    virtual ~CompositorWidget() {}

    // Widget interface - these will be overridden by actual widget implementations
    virtual void render(const LayoutRegion& region) override;
    virtual bool shouldUpdate() = 0;
    virtual void begin() = 0;

protected:
    DisplayCompositor& compositor;
    VirtualSurface* surface;

    // Compositor-aware rendering method that widgets should implement
    virtual void renderToSurface(VirtualSurface* surface, const LayoutRegion& region) = 0;

    // Helper methods for widgets using the compositor
    void clearRegion(const LayoutRegion& region);
    void setClipRegion(const LayoutRegion& region);
    void resetClipRegion();
};

#endif
