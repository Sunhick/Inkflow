#ifndef LAYOUT_REGION_H
#define LAYOUT_REGION_H

// Forward declaration
class Widget;

// Layout region class with widget reference and dirty state tracking
class LayoutRegion {
public:
    // Constructor
    LayoutRegion(int x = 0, int y = 0, int w = 0, int h = 0);

    // Destructor
    ~LayoutRegion();

    // Geometry accessors
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Geometry setters
    void setX(int newX) { x = newX; markDirty(); }
    void setY(int newY) { y = newY; markDirty(); }
    void setWidth(int newWidth) { width = newWidth; markDirty(); }
    void setHeight(int newHeight) { height = newHeight; markDirty(); }
    void setBounds(int newX, int newY, int newWidth, int newHeight);

    // Widget management
    void setWidget(Widget* widget);
    Widget* getWidget() const { return widget; }
    void removeWidget();
    bool hasWidget() const { return widget != nullptr; }

    // Dirty state tracking
    void markDirty();
    void markClean();
    bool needsUpdate() const { return isDirty; }

    // Geometry helper methods
    bool contains(int pointX, int pointY) const;
    bool intersects(const LayoutRegion& other) const;
    bool intersects(int otherX, int otherY, int otherWidth, int otherHeight) const;

    // Utility methods
    int getRight() const { return x + width; }
    int getBottom() const { return y + height; }
    bool isEmpty() const { return width <= 0 || height <= 0; }

private:
    int x, y, width, height;
    Widget* widget;
    bool isDirty;
};

#endif
