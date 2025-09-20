#include "LayoutRegion.h"
#include "Widget.h"
#include "../widgets/image/ImageWidget.h"
#include "../widgets/battery/BatteryWidget.h"
#include "../widgets/time/TimeWidget.h"
#include "../widgets/weather/WeatherWidget.h"
#include "../widgets/name/NameWidget.h"
#include "../managers/ConfigManager.h"
#include <vector>



// PIMPL implementation to hide widget collection details
class LayoutRegionImpl {
public:
    std::vector<Widget*> widgets;

    LayoutRegionImpl() {}
    ~LayoutRegionImpl() {
        // Delete widgets that we own (regions now own their widgets)
        for (Widget* widget : widgets) {
            delete widget;
        }
        widgets.clear();
    }
};

LayoutRegion::LayoutRegion(int x, int y, int w, int h)
    : x(x), y(y), width(w), height(h), impl(new LayoutRegionImpl()), legacyWidget(nullptr), isDirty(true) {
}

LayoutRegion::~LayoutRegion() {
    delete impl;
    // Note: We don't delete the legacyWidget as we don't own it
    legacyWidget = nullptr;
}

void LayoutRegion::setBounds(int newX, int newY, int newWidth, int newHeight) {
    x = newX;
    y = newY;
    width = newWidth;
    height = newHeight;
    markDirty();
}

// Widget collection management methods
size_t LayoutRegion::addWidget(Widget* widget) {
    if (!widget) {
        return SIZE_MAX; // Invalid index for null widget
    }

    impl->widgets.push_back(widget);
    markDirty();
    return impl->widgets.size() - 1; // Return index of added widget
}

bool LayoutRegion::removeWidget(size_t index) {
    if (index >= impl->widgets.size()) {
        return false; // Invalid index
    }

    impl->widgets.erase(impl->widgets.begin() + index);
    markDirty();
    return true;
}

Widget* LayoutRegion::getWidget(size_t index) const {
    if (index >= impl->widgets.size()) {
        return nullptr; // Invalid index
    }

    return impl->widgets[index];
}

size_t LayoutRegion::getWidgetCount() const {
    return impl->widgets.size();
}

void LayoutRegion::clearWidgets() {
    if (!impl->widgets.empty()) {
        impl->widgets.clear();
        markDirty();
    }
}



void LayoutRegion::initializeWidgets() {
    // Initialize all widgets in this region
    for (size_t i = 0; i < impl->widgets.size(); ++i) {
        Widget* widget = impl->widgets[i];
        if (widget) {
            widget->begin();
        }
    }
}

// Legacy widget management (for backward compatibility)
void LayoutRegion::setWidget(Widget* newWidget) {
    if (legacyWidget != newWidget) {
        legacyWidget = newWidget;
        markDirty();
    }
}

Widget* LayoutRegion::getLegacyWidget() const {
    return legacyWidget;
}

void LayoutRegion::removeLegacyWidget() {
    if (legacyWidget != nullptr) {
        legacyWidget = nullptr;
        markDirty();
    }
}

bool LayoutRegion::hasWidget() const {
    return legacyWidget != nullptr || !impl->widgets.empty();
}

// Rendering methods
void LayoutRegion::render() {
    // Render legacy widget if it exists
    if (legacyWidget) {
        legacyWidget->render(*this);
    }

    // Render all widgets in the collection
    for (size_t i = 0; i < impl->widgets.size(); ++i) {
        Widget* widget = impl->widgets[i];
        if (widget) {
            widget->render(*this);
        }
    }

    // Mark region as clean after rendering
    markClean();
}

void LayoutRegion::markDirty() {
    isDirty = true;
}

void LayoutRegion::markClean() {
    isDirty = false;
}

bool LayoutRegion::contains(int pointX, int pointY) const {
    return pointX >= x && pointX < (x + width) &&
           pointY >= y && pointY < (y + height);
}

bool LayoutRegion::intersects(const LayoutRegion& other) const {
    return intersects(other.getX(), other.getY(), other.getWidth(), other.getHeight());
}

bool LayoutRegion::intersects(int otherX, int otherY, int otherWidth, int otherHeight) const {
    // Check if rectangles don't intersect
    if (x >= otherX + otherWidth || otherX >= x + width ||
        y >= otherY + otherHeight || otherY >= y + height) {
        return false;
    }
    return true;
}
