#ifndef WIDGET_H
#define WIDGET_H

#include <Inkplate.h>
#include "LayoutRegion.h"
#include "DisplayList.h"

// Forward declaration to avoid circular dependency
class Compositor;

// Forward declaration for WidgetType
enum class WidgetType;

class Widget {
public:
    Widget(Inkplate& display) : display(display) {}
    virtual ~Widget() {}

    // Pure virtual methods that widgets must implement
    virtual void render(const LayoutRegion& region) = 0;
    virtual bool shouldUpdate() = 0;
    virtual void begin() = 0;

    // Widget type identification (must be implemented by each widget)
    virtual WidgetType getWidgetType() const = 0;

    // Compositor rendering — records draw commands to a DisplayList.
    // Default implementation delegates to renderToDisplayList().
    virtual void renderToCompositor(Compositor& compositor, const LayoutRegion& region);

    // Display list rendering — override to record draw commands.
    // Default implementation calls render() for backward compatibility.
    virtual void renderToDisplayList(DisplayList& dl, const LayoutRegion& region) {
        render(region); // fallback: draw directly to display
    }

    // Virtual method for layout change notifications
    virtual void onRegionChanged(const LayoutRegion& oldRegion, const LayoutRegion& newRegion) {}

    // Deep sleep optimization methods
    virtual void forceUpdate();
    virtual bool needsImmediateUpdate() const { return false; }
    virtual void update();

    // Network data fetching
    virtual void fetchData() {}
    virtual bool needsNetworkFetch() const { return false; }

protected:
    Inkplate& display;

    void clearRegion(const LayoutRegion& region);
    void setClipRegion(const LayoutRegion& region);
    void resetClipRegion();

    void clearRegionOnCompositor(Compositor& compositor, const LayoutRegion& region);
    void setClipRegionOnCompositor(Compositor& compositor, const LayoutRegion& region);
};

#endif
