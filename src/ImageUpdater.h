#ifndef IMAGE_UPDATER_H
#define IMAGE_UPDATER_H

#include <Inkplate.h>
#include <WiFi.h>

class ImageUpdater {
public:
    ImageUpdater(Inkplate &display, const char* ssid, const char* password, const char* imageUrl, unsigned long refreshMs);

    void begin();
    void loop();

private:
    Inkplate &display;
    const char* ssid;
    const char* password;
    const char* imageUrl;
    unsigned long refreshInterval;
    unsigned long lastUpdate;
    unsigned long lastWiFiCheck;
    int consecutiveFailures;
    bool displayingError;

    static const int MAX_WIFI_RETRIES = 30;
    static const int MAX_IMAGE_RETRIES = 3;
    static const unsigned long WIFI_CHECK_INTERVAL = 30000; // 30 seconds
    static const unsigned long ERROR_DISPLAY_TIME = 60000;  // 1 minute

    bool connectWiFi();
    bool fetchAndDisplayImage();
    void showError(const char* title, const char* message);
    void showStatus(const char* message);
    void checkWiFiConnection();
};

#endif
