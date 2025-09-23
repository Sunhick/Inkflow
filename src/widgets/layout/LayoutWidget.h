#pragma once

#include "../../core/Widget.h"
#include "../../core/LayoutRegion.h"
#include <vector>

// LayoutWidgetConfig is defined in ConfigManager.h

// Widget that handles layout visualization (borders, separators, etc.)
class LayoutWidget : public Widget {
public:
    LayoutWidget(Inkplate& display,
                 bool showBorders = true,
                 bool showSeparators = false,
                 int borderColor = 0,
                 int separatorColor = 0,
                 int borderThickness = 1,
                 int separatorThickness = 1);

    void begin() override;
    void render(const LayoutRegion& region) override;
    void renderToCompositor(Compositor& compositor, const LayoutRegion& region) override;
    bool shouldUpdate() override;
    WidgetType getWidgetType() const override;

    // Configuration methods
    void setShowBorders(bool show) { showRegionBorders = show; }
    void setShowSeparators(bool show) { showSeparators = show; }
    void setBorderColor(int color) { borderColor = color; }
    void setSeparatorColor(int color) { separatorColor = color; }
    void setBorderThickness(int thickness) { borderThickness = thickness; }
    void setSeparatorThickness(int thickness) { separatorThickness = thickness; }

    // Set reference to all regions for drawing borders/separators
    void setRegions(const std::vector<std::unique_ptr<LayoutRegion>>* regions) {
        allRegions = regions;
    }

private:
    bool showRegionBorders;
    bool showSeparators;
    int borderColor;
    int separatorColor;
    int borderThickness;
    int separatorThickness;

    // Reference to all regions for drawing layout elements
    const std::vector<std::unique_ptr<LayoutRegion>>* allRegions;

    void drawRegionBorder(const LayoutRegion& region);
    void drawSeparators();
};
