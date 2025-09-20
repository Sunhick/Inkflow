#ifndef LAYOUT_REGION_H
#define LAYOUT_REGION_H

#include <cstddef>

// Forward declarations
class Widget;
class LayoutRegionImpl;
class Inkplate;
struct AppConfig;



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



    // Widget collection management (using raw pointers for simplicity)
    size_t addWidget(Widget* widget);
    bool removeWidget(size_t index);
    Widget* getWidget(size_t index) const;
    size_t getWidgetCount() const;
    void clearWidgets();

    // Widget initialization
    void initializeWidgets();

    // Legacy widget management (for backward compatibility)
    void setWidget(Widget* widget);
    Widget* getLegacyWidget() const;
    void removeLegacyWidget();
    bool hasWidget() const;

    // Dirty state tracking
    void markDirty();
    void markClean();
    bool needsUpdate() const { return isDirty; }

    // Geometry helper methods
    bool contains(int pointX, int pointY) const;
    bool intersects(const LayoutRegion& other) const;
    bool intersects(int otherX, int otherY, int otherWidth, int otherHeight) const;

    // Rendering methods
    void render(); // Render all widgets in this region

    // Utility methods
    int getRight() const { return x + width; }
    int getBottom() const { return y + height; }
    bool isEmpty() const { return width <= 0 || height <= 0; }

private:
    int x, y, width, height;
    LayoutRegionImpl* impl; // PIMPL to hide widget collection implementation
    Widget* legacyWidget; // For backward compatibility
    bool isDirty;
};

#endif
