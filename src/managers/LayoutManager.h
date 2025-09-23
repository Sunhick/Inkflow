#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include "../core/LayoutRegion.h"
#include "../core/Compositor.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include <vector>
#include <memory>
#include <map>

// Forward declarations
class LayoutWidget;

class LayoutManager {
public:
    LayoutManager();
    ~LayoutManager();

    void begin();
    void loop();
    void forceRefresh(); // Manual refresh triggered by button
    void forceTimeAndBatteryUpdate(); // Force update of time and battery widgets using compositor partial rendering

    // Region collection management
    size_t addRegion(std::unique_ptr<LayoutRegion> region);
    bool removeRegion(size_t index);
    LayoutRegion* getRegion(size_t index) const;
    size_t getRegionCount() const { return regions.size(); }

    // Region iteration
    std::vector<std::unique_ptr<LayoutRegion>>::iterator regionsBegin() { return regions.begin(); }
    std::vector<std::unique_ptr<LayoutRegion>>::iterator regionsEnd() { return regions.end(); }
    std::vector<std::unique_ptr<LayoutRegion>>::const_iterator regionsBegin() const { return regions.begin(); }
    std::vector<std::unique_ptr<LayoutRegion>>::const_iterator regionsEnd() const { return regions.end(); }

    // Region access by ID
    LayoutRegion* getRegionById(const String& regionId) const;
    LayoutRegion* getOrCreateRegion(const String& regionId);

    // No more hardcoded region getters - use getRegionById() instead

    // Configuration getters for power management
    unsigned long getShortestUpdateInterval() const;
    int getWakeButtonPin() const;
    bool shouldEnterDeepSleep() const;
    unsigned long getDeepSleepThreshold() const;

    // Widget-region assignment methods
    bool assignWidgetToRegion(Widget* widget, const String& regionId);
    bool removeWidgetFromRegion(Widget* widget, const String& regionId);

    // Compositor integration demonstration
    void demonstrateCompositorIntegration();

private:
    // Core components
    Inkplate display;
    ConfigManager* configManager;
    DisplayManager* displayManager;
    WiFiManager* wifiManager;
    Compositor* compositor;

    // Region collection system
    std::vector<std::unique_ptr<LayoutRegion>> regions;
    std::map<String, LayoutRegion*> regionMap; // For quick access by region ID

    // Global layout widget for drawing borders/separators
    LayoutWidget* layoutWidget;

    // Configuration
    unsigned long lastUpdate;

    // Private methods
    void calculateLayoutRegions();
    void createAndAssignWidgets();
    void initializeComponents();
    void performInitialSetup();
    void handleScheduledUpdate();
    void handleWidgetUpdates();
    bool ensureConnectivity();
    void renderAllRegions();
    void renderChangedRegions(); // New method for partial updates
    void clearRegion(const LayoutRegion& region);
    // drawLayoutBorders() removed - now handled by LayoutWidget
};

#endif
