#include "Widget.h"
#include "../managers/LayoutManager.h"

void Widget::clearRegion(const LayoutRegion& region) {
    // Clear region with white background
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 7);
}

void Widget::setClipRegion(const LayoutRegion& region) {
    // Set clipping rectangle to prevent drawing outside widget bounds
    // Note: Inkplate library may not support clipping, so widgets should
    // manually ensure they don't draw outside their regions
}

void Widget::resetClipRegion() {
    // Reset clipping to full display
}
