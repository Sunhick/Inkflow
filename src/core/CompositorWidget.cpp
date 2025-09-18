#include "CompositorWidget.h"

CompositorWidget::CompositorWidget(DisplayCompositor& compositor)
    : Widget(compositor.getDisplay()), compositor(compositor), surface(nullptr) {
}

void CompositorWidget::render(const LayoutRegion& region) {
    // Begin rendering to the compositor's virtual surface
    surface = compositor.beginWidgetRender(region);

    if (surface) {
        // Let the specific widget implementation draw to the surface
        renderToSurface(surface, region);

        // End rendering - compositor will handle the rest
        compositor.endWidgetRender(region);
    }
}

void CompositorWidget::clearRegion(const LayoutRegion& region) {
    if (surface) {
        surface->clearRegion(region);
    }
}

void CompositorWidget::setClipRegion(const LayoutRegion& region) {
    // Clipping is handled by the widget ensuring it draws within bounds
    // The virtual surface will clip automatically
}

void CompositorWidget::resetClipRegion() {
    // No action needed - virtual surface handles clipping
}
