#include "DisplayCompositor.h"
#include <algorithm>

// VirtualSurface Implementation
VirtualSurface::VirtualSurface(int width, int height)
    : width(width), height(height), cursorX(0), cursorY(0),
      textSize(1), textColor(0), textWrap(false) {

    // Allocate buffer for 3-bit grayscale (8 levels: 0-7)
    // Each pixel needs 3 bits, so we'll use 1 byte per pixel for simplicity
    buffer = new uint8_t[width * height];
    clear();
}

VirtualSurface::~VirtualSurface() {
    delete[] buffer;
}

void VirtualSurface::fillRect(int x, int y, int w, int h, int color) {
    // Clamp color to valid range (0-7 for 3-bit)
    color = constrain(color, 0, 7);

    for (int py = y; py < y + h && py < height; py++) {
        for (int px = x; px < x + w && px < width; px++) {
            if (isValidCoordinate(px, py)) {
                buffer[py * width + px] = color;
            }
        }
    }
}

void VirtualSurface::drawLine(int x0, int y0, int x1, int y1, int color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void VirtualSurface::drawPixel(int x, int y, int color) {
    if (isValidCoordinate(x, y)) {
        color = constrain(color, 0, 7);
        buffer[y * width + x] = color;
    }
}

void VirtualSurface::setCursor(int x, int y) {
    cursorX = x;
    cursorY = y;
}

void VirtualSurface::setTextSize(int size) {
    textSize = constrain(size, 1, 4);
}

void VirtualSurface::setTextColor(int color) {
    textColor = constrain(color, 0, 7);
}

void VirtualSurface::setTextWrap(bool wrap) {
    textWrap = wrap;
}

void VirtualSurface::print(const char* text) {
    if (!text) return;

    int x = cursorX;
    int y = cursorY;

    while (*text) {
        if (*text == '\n') {
            x = cursorX;
            y += textSize * 8;
        } else {
            drawChar(x, y, *text, textColor, textSize);
            x += textSize * 6; // Character width

            if (textWrap && x >= width) {
                x = cursorX;
                y += textSize * 8;
            }
        }
        text++;
    }

    // Update cursor position
    cursorX = x;
    cursorY = y;
}

void VirtualSurface::print(const String& text) {
    print(text.c_str());
}

void VirtualSurface::clear() {
    // Fill with white (color 7 in 3-bit mode)
    memset(buffer, 7, width * height);
}

void VirtualSurface::clearRegion(const LayoutRegion& region) {
    fillRect(region.x, region.y, region.width, region.height, 7);
}

void VirtualSurface::drawChar(int x, int y, char c, int color, int size) {
    // Simple 5x7 font implementation
    // This is a minimal implementation - in practice you'd use a proper font
    if (c < 32 || c > 126) return;

    // Basic character patterns (simplified)
    static const uint8_t font5x7[][5] = {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // space
        {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
        // Add more characters as needed...
    };

    // For now, just draw a simple rectangle for each character
    fillRect(x, y, size * 5, size * 7, color);
}

bool VirtualSurface::isValidCoordinate(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

// DisplayCompositor Implementation
DisplayCompositor::DisplayCompositor(Inkplate& display)
    : display(display), surface(nullptr), currentDisplayMode(INKPLATE_3BIT), needsFullRender(true) {
}

DisplayCompositor::~DisplayCompositor() {
    delete surface;
}

void DisplayCompositor::begin() {
    Serial.println("Initializing Display Compositor...");

    // Initialize display
    display.begin();
    currentDisplayMode = INKPLATE_3BIT;
    display.setDisplayMode(currentDisplayMode);

    // Create virtual surface matching display dimensions
    int displayWidth = display.width();
    int displayHeight = display.height();

    Serial.printf("Creating virtual surface: %dx%d\n", displayWidth, displayHeight);
    surface = new VirtualSurface(displayWidth, displayHeight);

    // Enable text features
    display.setTextWrap(true);
    display.cp437(true);

    Serial.println("Display Compositor initialized");
}

VirtualSurface* DisplayCompositor::beginWidgetRender(const LayoutRegion& region) {
    if (!surface) {
        Serial.println("ERROR: Surface not initialized");
        return nullptr;
    }

    // Mark this region as dirty for later rendering
    markRegionDirty(region);

    // Return the virtual surface for the widget to draw on
    return surface;
}

void DisplayCompositor::endWidgetRender(const LayoutRegion& region) {
    // Widget has finished drawing to the virtual surface
    // The region is already marked as dirty from beginWidgetRender
    Serial.printf("Widget finished rendering in region: %dx%d at (%d,%d)\n",
                  region.width, region.height, region.x, region.y);
}

void DisplayCompositor::compose() {
    // Composition happens automatically as widgets draw to the virtual surface
    // This method could be used for post-processing effects if needed
    Serial.println("Compositing widget drawings...");
}

void DisplayCompositor::render() {
    if (!surface) {
        Serial.println("ERROR: Cannot render - surface not initialized");
        return;
    }

    Serial.println("Rendering composed display to Inkplate...");

    if (needsFullRender || dirtyRegions.empty()) {
        // Full screen render
        copyRegionToDisplay(LayoutRegion(0, 0, surface->getWidth(), surface->getHeight()));
        display.display();
        needsFullRender = false;
    } else {
        // Partial render of dirty regions
        renderPartial();
    }

    clearDirtyRegions();
    Serial.println("Display render complete");
}

void DisplayCompositor::renderPartial() {
    if (dirtyRegions.empty()) return;

    Serial.printf("Performing partial render of %d dirty regions\n", dirtyRegions.size());

    // Optimize dirty regions by merging overlapping ones
    optimizeDirtyRegions();

    // Switch to 1-bit mode for partial updates
    int originalMode = currentDisplayMode;
    if (currentDisplayMode != INKPLATE_1BIT) {
        display.setDisplayMode(INKPLATE_1BIT);
    }

    // Copy dirty regions to display
    for (const auto& dirtyRegion : dirtyRegions) {
        if (dirtyRegion.needsUpdate) {
            copyRegionToDisplay(dirtyRegion.region);
        }
    }

    // Perform partial update
    display.partialUpdate();

    // Restore original display mode
    if (originalMode != INKPLATE_1BIT) {
        display.setDisplayMode(originalMode);
        currentDisplayMode = originalMode;
    }
}

void DisplayCompositor::markRegionDirty(const LayoutRegion& region) {
    dirtyRegions.emplace_back(region);
}

void DisplayCompositor::clearDirtyRegions() {
    dirtyRegions.clear();
}

bool DisplayCompositor::hasUpdates() const {
    return !dirtyRegions.empty() || needsFullRender;
}

void DisplayCompositor::setDisplayMode(int mode) {
    if (mode != currentDisplayMode) {
        display.setDisplayMode(mode);
        currentDisplayMode = mode;
        needsFullRender = true; // Mode change requires full render
    }
}

int DisplayCompositor::getDisplayMode() const {
    return currentDisplayMode;
}

void DisplayCompositor::clear() {
    if (surface) {
        surface->clear();
    }
    display.clearDisplay();
    needsFullRender = true;
}

void DisplayCompositor::drawLayoutBorders(const std::vector<LayoutRegion>& regions) {
    if (!surface) return;

    // Draw borders between regions on the virtual surface
    for (size_t i = 0; i < regions.size(); i++) {
        const LayoutRegion& region = regions[i];

        // Draw border around region
        surface->drawLine(region.x, region.y, region.x + region.width - 1, region.y, 0);
        surface->drawLine(region.x, region.y, region.x, region.y + region.height - 1, 0);
        surface->drawLine(region.x + region.width - 1, region.y,
                         region.x + region.width - 1, region.y + region.height - 1, 0);
        surface->drawLine(region.x, region.y + region.height - 1,
                         region.x + region.width - 1, region.y + region.height - 1, 0);
    }
}

void DisplayCompositor::copyRegionToDisplay(const LayoutRegion& region) {
    if (!surface) return;

    uint8_t* buffer = surface->getBuffer();
    int surfaceWidth = surface->getWidth();

    // Copy pixels from virtual surface to display buffer
    for (int y = region.y; y < region.y + region.height && y < surface->getHeight(); y++) {
        for (int x = region.x; x < region.x + region.width && x < surfaceWidth; x++) {
            uint8_t color = buffer[y * surfaceWidth + x];
            display.drawPixel(x, y, color);
        }
    }
}

void DisplayCompositor::optimizeDirtyRegions() {
    if (dirtyRegions.size() <= 1) return;

    // Simple optimization: merge overlapping regions
    for (size_t i = 0; i < dirtyRegions.size(); i++) {
        for (size_t j = i + 1; j < dirtyRegions.size(); j++) {
            if (regionsOverlap(dirtyRegions[i].region, dirtyRegions[j].region)) {
                // Merge regions
                dirtyRegions[i].region = mergeRegions(dirtyRegions[i].region, dirtyRegions[j].region);
                dirtyRegions.erase(dirtyRegions.begin() + j);
                j--; // Adjust index after erase
            }
        }
    }
}

bool DisplayCompositor::regionsOverlap(const LayoutRegion& a, const LayoutRegion& b) {
    return !(a.x + a.width <= b.x || b.x + b.width <= a.x ||
             a.y + a.height <= b.y || b.y + b.height <= a.y);
}

LayoutRegion DisplayCompositor::mergeRegions(const LayoutRegion& a, const LayoutRegion& b) {
    int minX = min(a.x, b.x);
    int minY = min(a.y, b.y);
    int maxX = max(a.x + a.width, b.x + b.width);
    int maxY = max(a.y + a.height, b.y + b.height);

    return LayoutRegion(minX, minY, maxX - minX, maxY - minY);
}
