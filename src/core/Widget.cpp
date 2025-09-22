#include "Widget.h"
#include "Compositor.h"
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

void Widget::clearRegionOnCompositor(Compositor& compositor, const LayoutRegion& region) {
    // Clear region on compositor surface with white background (color 7)
    compositor.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 7);

    // Mark the region as changed for partial updates
    compositor.markRegionChanged(region);
}

void Widget::setClipRegionOnCompositor(Compositor& compositor, const LayoutRegion& region) {
    // Set clipping region for compositor rendering
    // Since the compositor doesn't have built-in clipping, widgets should
    // manually ensure they don't draw outside their regions when using
    // compositor methods like setPixel, drawRect, fillRect
    //
    // This method serves as a documentation point and could be extended
    // in the future if compositor-level clipping is implemented

    // For now, widgets should check bounds manually:
    // if (x >= region.getX() && x < region.getX() + region.getWidth() &&
    //     y >= region.getY() && y < region.getY() + region.getHeight()) {
    //     compositor.setPixel(x, y, color);
    // }
}
void Widget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    // Default implementation for backward compatibility
    // This allows existing widgets to work without modification
    // Widgets can override this method to provide compositor-specific rendering

    // For now, we fall back to the existing render method
    // In a full implementation, this would render to the compositor surface
    // instead of directly to the display
    render(region);

    // Mark the region as changed in the compositor
    compositor.markRegionChanged(region);
}
