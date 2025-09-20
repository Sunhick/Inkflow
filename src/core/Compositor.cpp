#include "Compositor.h"
#include <cstring>
#include <algorithm>

Compositor::Compositor(int width, int height)
    : virtualSurface(nullptr)
    , dirtyRegions(nullptr)
    , surfaceWidth(width)
    , surfaceHeight(height)
    , bytesPerPixel(1)  // 8-bit grayscale
    , surfaceSize(0)
    , hasChanges(false) {

    surfaceSize = static_cast<size_t>(surfaceWidth) * surfaceHeight * bytesPerPixel;
}

Compositor::~Compositor() {
    cleanup();
}

bool Compositor::initialize() {
    // Clean up any existing allocation
    cleanup();

    // Allocate virtual surface buffer
    virtualSurface = new(std::nothrow) uint8_t[surfaceSize];
    if (!virtualSurface) {
        return false;
    }

    // Allocate dirty regions tracking (one bool per pixel for simplicity)
    dirtyRegions = new(std::nothrow) bool[surfaceWidth * surfaceHeight];
    if (!dirtyRegions) {
        delete[] virtualSurface;
        virtualSurface = nullptr;
        return false;
    }

    // Initialize buffers
    clear();
    resetChangeTracking();

    return true;
}

void Compositor::cleanup() {
    if (virtualSurface) {
        delete[] virtualSurface;
        virtualSurface = nullptr;
    }

    if (dirtyRegions) {
        delete[] dirtyRegions;
        dirtyRegions = nullptr;
    }

    changedAreas.clear();
    hasChanges = false;
}

void Compositor::clear() {
    if (!virtualSurface) return;

    // Clear to white (7 is white in 3-bit mode, 255 in 8-bit)
    std::memset(virtualSurface, 255, surfaceSize);

    // Mark entire surface as changed
    LayoutRegion fullSurface(0, 0, surfaceWidth, surfaceHeight);
    markRegionChanged(fullSurface);
}

void Compositor::clearRegion(const LayoutRegion& region) {
    if (!virtualSurface) return;

    // Clamp region to surface bounds
    int startX = std::max(0, region.x);
    int startY = std::max(0, region.y);
    int endX = std::min(surfaceWidth, region.x + region.width);
    int endY = std::min(surfaceHeight, region.y + region.height);

    // Clear the region to white
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            setPixel(x, y, 255);  // White
        }
    }

    markRegionChanged(region);
}

uint8_t* Compositor::getSurfaceBuffer() {
    return virtualSurface;
}

const uint8_t* Compositor::getSurfaceBuffer() const {
    return virtualSurface;
}

size_t Compositor::getPixelIndex(int x, int y) const {
    return static_cast<size_t>(y) * surfaceWidth + x;
}

bool Compositor::isValidCoordinate(int x, int y) const {
    return x >= 0 && x < surfaceWidth && y >= 0 && y < surfaceHeight;
}

void Compositor::setPixel(int x, int y, uint8_t color) {
    if (!virtualSurface || !isValidCoordinate(x, y)) return;

    size_t index = getPixelIndex(x, y);
    virtualSurface[index] = color;

    // Mark pixel as dirty
    if (dirtyRegions) {
        dirtyRegions[index] = true;
        hasChanges = true;
    }
}

uint8_t Compositor::getPixel(int x, int y) const {
    if (!virtualSurface || !isValidCoordinate(x, y)) return 255;  // Return white for invalid coordinates

    return virtualSurface[getPixelIndex(x, y)];
}

void Compositor::drawRect(int x, int y, int w, int h, uint8_t color) {
    if (!virtualSurface || w <= 0 || h <= 0) return;

    // Draw top and bottom edges
    for (int i = 0; i < w; i++) {
        setPixel(x + i, y, color);           // Top edge
        setPixel(x + i, y + h - 1, color);  // Bottom edge
    }

    // Draw left and right edges
    for (int i = 0; i < h; i++) {
        setPixel(x, y + i, color);           // Left edge
        setPixel(x + w - 1, y + i, color);  // Right edge
    }

    // Mark region as changed
    LayoutRegion region(x, y, w, h);
    markRegionChanged(region);
}

void Compositor::fillRect(int x, int y, int w, int h, uint8_t color) {
    if (!virtualSurface || w <= 0 || h <= 0) return;

    // Clamp to surface bounds
    int startX = std::max(0, x);
    int startY = std::max(0, y);
    int endX = std::min(surfaceWidth, x + w);
    int endY = std::min(surfaceHeight, y + h);

    // Fill the rectangle
    for (int py = startY; py < endY; py++) {
        for (int px = startX; px < endX; px++) {
            setPixel(px, py, color);
        }
    }

    // Mark region as changed
    LayoutRegion region(x, y, w, h);
    markRegionChanged(region);
}

void Compositor::markRegionChanged(const LayoutRegion& region) {
    // Add to changed areas list
    changedAreas.push_back(region);
    hasChanges = true;

    // Mark dirty regions if tracking is enabled
    if (dirtyRegions) {
        int startX = std::max(0, region.x);
        int startY = std::max(0, region.y);
        int endX = std::min(surfaceWidth, region.x + region.width);
        int endY = std::min(surfaceHeight, region.y + region.height);

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                dirtyRegions[getPixelIndex(x, y)] = true;
            }
        }
    }
}

void Compositor::resetChangeTracking() {
    changedAreas.clear();
    hasChanges = false;

    if (dirtyRegions) {
        std::memset(dirtyRegions, false, surfaceWidth * surfaceHeight * sizeof(bool));
    }
}

bool Compositor::hasChangedRegions() const {
    return hasChanges;
}

std::vector<LayoutRegion> Compositor::getChangedRegions() const {
    return changedAreas;
}

void Compositor::displayToInkplate(Inkplate& display) {
    if (!virtualSurface) return;

    // Clear the display buffer
    display.clearDisplay();

    // Copy virtual surface to display buffer
    // Note: This is a simplified implementation. In practice, you might need
    // to convert between different color formats or handle display-specific requirements

    for (int y = 0; y < surfaceHeight; y++) {
        for (int x = 0; x < surfaceWidth; x++) {
            uint8_t pixel = getPixel(x, y);

            // Convert 8-bit grayscale to Inkplate's expected format
            // Inkplate uses inverted values where 0 = black, 7 = white in 3-bit mode
            uint8_t inkplateColor;
            if (pixel >= 224) inkplateColor = 7;      // White
            else if (pixel >= 192) inkplateColor = 6;
            else if (pixel >= 160) inkplateColor = 5;
            else if (pixel >= 128) inkplateColor = 4;
            else if (pixel >= 96) inkplateColor = 3;
            else if (pixel >= 64) inkplateColor = 2;
            else if (pixel >= 32) inkplateColor = 1;
            else inkplateColor = 0;                   // Black

            display.drawPixel(x, y, inkplateColor);
        }
    }

    // Perform full display update
    display.display();

    // Reset change tracking after successful display
    resetChangeTracking();
}

void Compositor::partialDisplayToInkplate(Inkplate& display) {
    if (!virtualSurface || !hasChanges) return;

    // For now, implement as full display update
    // In a more sophisticated implementation, this would only update changed regions
    displayToInkplate(display);
}

size_t Compositor::getMemoryUsage() const {
    size_t usage = 0;

    if (virtualSurface) {
        usage += surfaceSize;
    }

    if (dirtyRegions) {
        usage += surfaceWidth * surfaceHeight * sizeof(bool);
    }

    usage += changedAreas.capacity() * sizeof(LayoutRegion);

    return usage;
}
