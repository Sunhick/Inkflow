#ifndef IMAGE_UPDATER_H
#define IMAGE_UPDATER_H

#include "WiFiManager.h"
#include "DisplayManager.h"
#include "ImageFetcher.h"
#include "BatteryManager.h"
#include "TimeManager.h"
#include "WeatherManager.h"

class ImageUpdater {
public:
    ImageUpdater(Inkplate &display, const char* ssid, const char* password,
                 const char* imageUrl, unsigned long refreshMs);

    void begin();
    void loop();
    void forceImageRefresh(); // Manual refresh triggered by button

private:
    void updateAllSidebarComponents(); // Update all sidebar components in one go
    WiFiManager wifiManager;
    DisplayManager displayManager;
    ImageFetcher imageFetcher;
    BatteryManager batteryManager;
    TimeManager timeManager;
    WeatherManager weatherManager;

    unsigned long refreshInterval;
    unsigned long lastUpdate;

    void performInitialSetup();
    void handleScheduledUpdate();
    bool ensureConnectivity();
    void processImageUpdate(bool showLoadingStatus = true);
    void handleBatteryUpdate();
    void handleTimeUpdate();
    void handleWeatherUpdate();
};

#endif
