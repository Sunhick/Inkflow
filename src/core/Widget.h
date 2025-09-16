#ifndef WIDGET_H
#define WIDGET_H

#include <Inkplate.h>
#include "LayoutRegion.h"

class Widget {
public:
    Widget(Inkplate& display) : display(display) {}
    virtual ~Widget() {}

    // Pure virtual methods that widgets must implement
    virtual void render(const LayoutRegion& region) = 0;
    virtual bool shouldUpdate() = 0;
    virtual void begin() = 0;

protected:
    Inkplate& display;

    // Helper methods for widgets
    void clearRegion(const LayoutRegion& region);
    void setClipRegion(const LayoutRegion& region);
    void resetClipRegion();
};

#endif
