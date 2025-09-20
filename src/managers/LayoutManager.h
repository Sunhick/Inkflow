#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include "../core/LayoutRegion.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "WiFiManager.h"
#include <vector>
#include <memory>

class LayoutManager {
public:
    LayoutManager();
    ~LayoutManager();

    void begin();
    void loop();
    void forceRefresh(); // Manual refresh triggered by button

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

    // Legacy region getters for backward compatibility (returns pointers to regions in collection)
    LayoutRegion* getImageRegion() const;
    LayoutRegion* getSidebarRegion() const;
    LayoutRegion* getNameRegion() const;
    LayoutRegion* getTimeRegion() const;
    LayoutRegion* getWeatherRegion() const;
    LayoutRegion* getBatteryRegion() const;

    // Configuration getters for power management
    unsigned long getShortestUpdateInterval() const;
    int getWakeButtonPin() const;
    bool shouldEnterDeepSleep() const;
    unsigned long getDeepSleepThreshold() const;

private:
    // Core components
    Inkplate display;
    ConfigManager* configManager;
    DisplayManager* displayManager;
    WiFiManager* wifiManager;

    // Region collection system
    std::vector<std::unique_ptr<LayoutRegion>> regions;

    // Region indices for quick access
    enum RegionIndex {
        SIDEBAR_REGION = 0,
        IMAGE_REGION = 1,
        NAME_REGION = 2,
        TIME_REGION = 3,
        WEATHER_REGION = 4,
        BATTERY_REGION = 5
    };

    // Configuration
    unsigned long lastUpdate;

    // Private methods
    void calculateLayoutRegions();
    void assignWidgetTypesToRegions();
    void initializeComponents();
    void performInitialSetup();
    void handleScheduledUpdate();
    void handleWidgetUpdates();
    bool ensureConnectivity();
    void renderAllRegions();
    void clearRegion(const LayoutRegion& region);
    void drawLayoutBorders();
};

#endif
