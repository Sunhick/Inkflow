#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Inkplate.h>

class DisplayManager {
public:
    DisplayManager(Inkplate &display);

    void initialize();
    void showStatus(const char* message, const char* networkName = nullptr, const char* ipAddress = nullptr);
    void showError(const char* title, const char* message, const char* wifiStatus = nullptr);
    void showImageError(const char* url, int failures, int retrySeconds, const char* ipAddress, int signalStrength);
    void clear();
    void update();

private:
    Inkplate &display;

    void setTitle(const char* title);
    void setMessage(const char* message, int y = 40);
    void setSmallText(const char* text, int x, int y);
};

#endif
