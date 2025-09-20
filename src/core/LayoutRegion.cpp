#include "LayoutRegion.h"

LayoutRegion::LayoutRegion(int x, int y, int w, int h)
    : x(x), y(y), width(w), height(h), widget(nullptr), isDirty(true) {
}

LayoutRegion::~LayoutRegion() {
    // Note: We don't delete the widget as we don't own it
    widget = nullptr;
}

void LayoutRegion::setBounds(int newX, int newY, int newWidth, int newHeight) {
    x = newX;
    y = newY;
    width = newWidth;
    height = newHeight;
    markDirty();
}

void LayoutRegion::setWidget(Widget* newWidget) {
    if (widget != newWidget) {
        widget = newWidget;
        markDirty();
    }
}

void LayoutRegion::removeWidget() {
    if (widget != nullptr) {
        widget = nullptr;
        markDirty();
    }
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
