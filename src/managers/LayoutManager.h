#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include "../core/LayoutRegion.h"
#include "DisplayManager.h"
#include "../widgets/ImageWidget.h"
#include "../widgets/BatteryWidget.h"
#include "../widgets/TimeWidget.h"
#include "../widgets/WeatherWidget.h"
#include "WiFiManager.h"

class LayoutManager {
public:
    LayoutManager(const char* ssid, const char* password, const char* imageUrl, unsigned long refreshMs);
    ~LayoutManager();

    void begin();
    void loop();
    void forceRefresh(); // Manual refresh triggered by button

    // Layout region getters for widgets
    LayoutRegion getImageRegion() const { return imageRegion; }
    LayoutRegion getSidebarRegion() const { return sidebarRegion; }
    LayoutRegion getTimeRegion() const { return timeRegion; }
    LayoutRegion getWeatherRegion() const { return weatherRegion; }
    LayoutRegion getBatteryRegion() const { return batteryRegion; }

private:
    // Core components
    Inkplate display;
    DisplayManager* displayManager;
    WiFiManager* wifiManager;

    // Widgets
    ImageWidget* imageWidget;
    BatteryWidget* batteryWidget;
    TimeWidget* timeWidget;
    WeatherWidget* weatherWidget;

    // Layout regions
    LayoutRegion imageRegion;
    LayoutRegion sidebarRegion;
    LayoutRegion timeRegion;
    LayoutRegion weatherRegion;
    LayoutRegion batteryRegion;

    // Configuration
    const char* imageUrl;
    unsigned long refreshInterval;
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
