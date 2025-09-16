#ifndef LAYOUT_REGION_H
#define LAYOUT_REGION_H

// Layout region structure
struct LayoutRegion {
    int x, y, width, height;

    LayoutRegion(int x = 0, int y = 0, int w = 0, int h = 0)
        : x(x), y(y), width(w), height(h) {}
};

#endif
