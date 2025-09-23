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
    int startX = std::max(0, region.getX());
    int startY = std::max(0, region.getY());
    int endX = std::min(surfaceWidth, region.getX() + region.getWidth());
    int endY = std::min(surfaceHeight, region.getY() + region.getHeight());

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
    // Validate region bounds
    if (region.getWidth() <= 0 || region.getHeight() <= 0) return;

    // Clamp region to surface bounds
    LayoutRegion clampedRegion(
        std::max(0, region.getX()),
        std::max(0, region.getY()),
        std::min(region.getWidth(), surfaceWidth - std::max(0, region.getX())),
        std::min(region.getHeight(), surfaceHeight - std::max(0, region.getY()))
    );

    // Skip if region is completely outside surface
    if (clampedRegion.getWidth() <= 0 || clampedRegion.getHeight() <= 0) return;

    // Add to changed areas list
    changedAreas.push_back(clampedRegion);
    hasChanges = true;

    // Mark dirty regions if tracking is enabled
    if (dirtyRegions) {
        int startX = clampedRegion.getX();
        int startY = clampedRegion.getY();
        int endX = clampedRegion.getX() + clampedRegion.getWidth();
        int endY = clampedRegion.getY() + clampedRegion.getHeight();

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                dirtyRegions[getPixelIndex(x, y)] = true;
            }
        }
    }

    // Merge overlapping regions to optimize partial updates
    mergeOverlappingRegions();
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

    Serial.println("Compositor: Performing partial display update...");

    // Get changed regions
    std::vector<LayoutRegion> regions = getChangedRegions();

    if (regions.empty()) {
        Serial.println("Compositor: No changed regions to update");
        return;
    }

    Serial.printf("Compositor: Updating %d changed regions\n", regions.size());

    // Update each changed region
    for (const auto& region : regions) {
        Serial.printf("Compositor: Updating region (%d,%d) %dx%d\n",
                     region.getX(), region.getY(), region.getWidth(), region.getHeight());

        // Copy pixels from virtual surface to display for this region
        for (int y = region.getY(); y < region.getY() + region.getHeight(); y++) {
            for (int x = region.getX(); x < region.getX() + region.getWidth(); x++) {
                if (isValidCoordinate(x, y)) {
                    uint8_t pixel = getPixel(x, y);

                    // Convert 8-bit grayscale to Inkplate's expected format
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
        }
    }

    // Perform partial display update
    display.partialUpdate();

    // Reset change tracking after successful partial display
    resetChangeTracking();

    Serial.println("Compositor: Partial display update complete");
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

void Compositor::mergeOverlappingRegions() {
    if (changedAreas.size() <= 1) return;

    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 0; i < changedAreas.size() && !merged; i++) {
            for (size_t j = i + 1; j < changedAreas.size(); j++) {
                if (regionsOverlap(changedAreas[i], changedAreas[j])) {
                    // Merge regions
                    LayoutRegion mergedRegion = mergeRegions(changedAreas[i], changedAreas[j]);
                    changedAreas[i] = mergedRegion;
                    changedAreas.erase(changedAreas.begin() + j);
                    merged = true;
                    break;
                }
            }
        }
    }
}

bool Compositor::regionsOverlap(const LayoutRegion& a, const LayoutRegion& b) const {
    // Check if regions overlap or are adjacent (for merging efficiency)
    int a_right = a.getX() + a.getWidth();
    int a_bottom = a.getY() + a.getHeight();
    int b_right = b.getX() + b.getWidth();
    int b_bottom = b.getY() + b.getHeight();

    // Allow merging of adjacent regions (within 1 pixel) to reduce fragmentation
    return !(a_right < b.getX() - 1 || b_right < a.getX() - 1 ||
             a_bottom < b.getY() - 1 || b_bottom < a.getY() - 1);
}

LayoutRegion Compositor::mergeRegions(const LayoutRegion& a, const LayoutRegion& b) const {
    int left = std::min(a.getX(), b.getX());
    int top = std::min(a.getY(), b.getY());
    int right = std::max(a.getX() + a.getWidth(), b.getX() + b.getWidth());
    int bottom = std::max(a.getY() + a.getHeight(), b.getY() + b.getHeight());

    return LayoutRegion(left, top, right - left, bottom - top);
}
