#include "LayoutWidget.h"

LayoutWidget::LayoutWidget(Inkplate& display,
                           bool showBorders,
                           bool showSeparators,
                           int borderColor,
                           int separatorColor,
                           int borderThickness,
                           int separatorThickness)
    : Widget(display),
      showRegionBorders(showBorders),
      showSeparators(showSeparators),
      borderColor(borderColor),
      separatorColor(separatorColor),
      borderThickness(borderThickness),
      separatorThickness(separatorThickness),
      allRegions(nullptr) {
}

void LayoutWidget::begin() {
    // No initialization needed for layout widget
}

void LayoutWidget::render(const LayoutRegion& region) {
    // Layout widget renders layout elements across the entire display
    // The region parameter is ignored since we draw global layout elements

    Serial.printf("LayoutWidget::render() called - showBorders: %s, regions: %d\n",
                  showRegionBorders ? "true" : "false",
                  allRegions ? allRegions->size() : 0);

    if (!allRegions) {
        Serial.println("LayoutWidget: No regions available to draw");
        return; // No regions to draw
    }

    // Draw borders for all regions if enabled
    if (showRegionBorders) {
        Serial.printf("LayoutWidget: Drawing borders for %d regions\n", allRegions->size());
        for (const auto& regionPtr : *allRegions) {
            if (regionPtr) {
                Serial.printf("Drawing border for region at (%d,%d) %dx%d\n",
                             regionPtr->getX(), regionPtr->getY(),
                             regionPtr->getWidth(), regionPtr->getHeight());
                drawRegionBorder(*regionPtr);
            }
        }
    } else {
        Serial.println("LayoutWidget: Region borders disabled");
    }

    // Draw separators between regions if enabled
    if (showSeparators) {
        Serial.println("LayoutWidget: Drawing separators");
        drawSeparators();
    }
}

bool LayoutWidget::shouldUpdate() {
    // Layout widget doesn't need periodic updates
    // It only renders when the layout changes
    return false;
}

void LayoutWidget::drawRegionBorder(const LayoutRegion& region) {
    int x = region.getX();
    int y = region.getY();
    int w = region.getWidth();
    int h = region.getHeight();

    Serial.printf("Drawing border: x=%d, y=%d, w=%d, h=%d, color=%d, thickness=%d\n",
                  x, y, w, h, borderColor, borderThickness);

    // Draw border with specified thickness
    for (int t = 0; t < borderThickness; t++) {
        // Top border
        display.drawLine(x - t, y - t, x + w - 1 + t, y - t, borderColor);
        // Bottom border
        display.drawLine(x - t, y + h - 1 + t, x + w - 1 + t, y + h - 1 + t, borderColor);
        // Left border
        display.drawLine(x - t, y - t, x - t, y + h - 1 + t, borderColor);
        // Right border
        display.drawLine(x + w - 1 + t, y - t, x + w - 1 + t, y + h - 1 + t, borderColor);
    }
}

void LayoutWidget::drawSeparators() {
    if (!allRegions || allRegions->size() < 2) {
        return; // Need at least 2 regions for separators
    }

    // Draw separators between adjacent regions
    // This is a simple implementation - could be made more sophisticated

    for (size_t i = 0; i < allRegions->size(); i++) {
        const LayoutRegion* region1 = (*allRegions)[i].get();
        if (!region1) continue;

        for (size_t j = i + 1; j < allRegions->size(); j++) {
            const LayoutRegion* region2 = (*allRegions)[j].get();
            if (!region2) continue;

            // Check if regions are adjacent and draw separator
            int x1 = region1->getX();
            int y1 = region1->getY();
            int w1 = region1->getWidth();
            int h1 = region1->getHeight();

            int x2 = region2->getX();
            int y2 = region2->getY();
            int w2 = region2->getWidth();
            int h2 = region2->getHeight();

            // Vertical separator (regions side by side)
            if (x1 + w1 == x2 && y1 < y2 + h2 && y2 < y1 + h1) {
                // Draw vertical separator
                int sepX = x1 + w1;
                int sepY = max(y1, y2);
                int sepH = min(y1 + h1, y2 + h2) - sepY;

                for (int t = 0; t < separatorThickness; t++) {
                    display.drawLine(sepX + t, sepY, sepX + t, sepY + sepH - 1, separatorColor);
                }
            }
            // Horizontal separator (regions stacked)
            else if (y1 + h1 == y2 && x1 < x2 + w2 && x2 < x1 + w1) {
                // Draw horizontal separator
                int sepY = y1 + h1;
                int sepX = max(x1, x2);
                int sepW = min(x1 + w1, x2 + w2) - sepX;

                for (int t = 0; t < separatorThickness; t++) {
                    display.drawLine(sepX, sepY + t, sepX + sepW - 1, sepY + t, separatorColor);
                }
            }
        }
    }
}
