#ifndef IMAGE_UPDATER_H
#define IMAGE_UPDATER_H

#include "WiFiManager.h"
#include "DisplayManager.h"
#include "ImageFetcher.h"
#include "BatteryManager.h"
#include "TimeManager.h"

class ImageUpdater {
public:
    ImageUpdater(Inkplate &display, const char* ssid, const char* password,
                 const char* imageUrl, unsigned long refreshMs);

    void begin();
    void loop();

private:
    WiFiManager wifiManager;
    DisplayManager displayManager;
    ImageFetcher imageFetcher;
    BatteryManager batteryManager;
    TimeManager timeManager;

    unsigned long refreshInterval;
    unsigned long lastUpdate;

    void performInitialSetup();
    void handleScheduledUpdate();
    bool ensureConnectivity();
    void processImageUpdate();
    void handleBatteryUpdate();
    void handleTimeUpdate();
};

#endif
