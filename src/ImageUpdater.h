#ifndef IMAGE_UPDATER_H
#define IMAGE_UPDATER_H

#include <Inkplate.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

    void connectWiFi();
    void fetchAndDisplayImage();
    void showConnectionStatus();
    void showSuccessStatus(int contentLength);
    void showErrorStatus(int httpCode);
    void showImageError();
};

#endif
