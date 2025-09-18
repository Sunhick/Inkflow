#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include "../core/LayoutRegion.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "../widgets/ImageWidget.h"
#include "../widgets/BatteryWidget.h"
#include "../widgets/TimeWidget.h"
#include "../widgets/WeatherWidget.h"
#include "../widgets/NameWidget.h"
#include "WiFiManager.h"

class LayoutManager {
public:
    LayoutManager();
    ~LayoutManager();

    void begin();
    void loop();
    void forceRefresh(); // Manual refresh triggered by button
    void forceTimeAndBatteryUpdate(); // Force update of time and battery widgets

    // Layout region getters for widgets
    LayoutRegion getImageRegion() const { return imageRegion; }
    LayoutRegion getSidebarRegion() const { return sidebarRegion; }
    LayoutRegion getNameRegion() const { return nameRegion; }
    LayoutRegion getTimeRegion() const { return timeRegion; }
    LayoutRegion getWeatherRegion() const { return weatherRegion; }
    LayoutRegion getBatteryRegion() const { return batteryRegion; }

private:
    // Core components
    Inkplate display;
    ConfigManager* configManager;
    DisplayManager* displayManager;
    WiFiManager* wifiManager;

    // Widgets
    ImageWidget* imageWidget;
    BatteryWidget* batteryWidget;
    TimeWidget* timeWidget;
    WeatherWidget* weatherWidget;
    NameWidget* nameWidget;

    // Layout regions
    LayoutRegion imageRegion;
    LayoutRegion sidebarRegion;
    LayoutRegion nameRegion;
    LayoutRegion timeRegion;
    LayoutRegion weatherRegion;
    LayoutRegion batteryRegion;

    // Configuration
    unsigned long lastUpdate;

    // Private methods
    void calculateLayoutRegions();
    void initializeComponents();
    void performInitialSetup();
    void handleScheduledUpdate();
    void handleComponentUpdates();
    bool ensureConnectivity();
    void processImageUpdate(bool showLoadingStatus = true);
    void renderAllWidgets();
    void clearRegion(const LayoutRegion& region);
    void drawLayoutBorders();
};

#endif
