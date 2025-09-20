#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <cstdint>
#include <vector>
#include <Inkplate.h>
#include "LayoutRegion.h"

/**
 * Compositor class manages a virtual surface for widget rendering
 * and coordinates the final display output to the Inkplate device.
 */
class Compositor {
private:
    uint8_t* virtualSurface;
    bool* dirtyRegions;
    int surfaceWidth;
    int surfaceHeight;
    int bytesPerPixel;
    size_t surfaceSize;

    // Change tracking
    std::vector<LayoutRegion> changedAreas;
    bool hasChanges;

    // Helper methods
    size_t getPixelIndex(int x, int y) const;
    bool isValidCoordinate(int x, int y) const;

public:
    // Constructor and destructor
    Compositor(int width = 1200, int height = 825);
    ~Compositor();

    // Surface management
    bool initialize();
    void cleanup();
    void clear();
    void clearRegion(const LayoutRegion& region);

    // Surface access
    uint8_t* getSurfaceBuffer();
    const uint8_t* getSurfaceBuffer() const;
    int getWidth() const { return surfaceWidth; }
    int getHeight() const { return surfaceHeight; }
    size_t getSurfaceSize() const { return surfaceSize; }

    // Pixel manipulation methods
    void setPixel(int x, int y, uint8_t color);
    uint8_t getPixel(int x, int y) const;
    void drawRect(int x, int y, int w, int h, uint8_t color);
    void fillRect(int x, int y, int w, int h, uint8_t color);

    // Change tracking
    void markRegionChanged(const LayoutRegion& region);
    void resetChangeTracking();
    bool hasChangedRegions() const;
    std::vector<LayoutRegion> getChangedRegions() const;

    // Display output
    void displayToInkplate(Inkplate& display);
    void partialDisplayToInkplate(Inkplate& display);

    // Status and diagnostics
    bool isInitialized() const { return virtualSurface != nullptr; }
    size_t getMemoryUsage() const;
};

#endif
