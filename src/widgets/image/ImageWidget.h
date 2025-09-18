#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include "../../core/Widget.h"
#include <WiFi.h>
#include <HTTPClient.h>

class ImageWidget : public Widget {
public:
    ImageWidget(Inkplate& display, const char* imageUrl);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;

    // Image-specific methods
    bool fetchAndDisplay(const LayoutRegion& region);
    void showErrorInRegion(const LayoutRegion& region, const char* title, const char* message, const char* details = nullptr);

    void showImagePlaceholder(const LayoutRegion& region, const char* title, const char* subtitle = nullptr);
    void showDiagnosticsInRegion(const LayoutRegion& region, const char* ipAddress, int signalStrength);
    int getConsecutiveFailures() const { return consecutiveFailures; }

private:
    const char* imageUrl;
    int consecutiveFailures;
    unsigned long lastImageUpdate;

    static const unsigned long IMAGE_UPDATE_INTERVAL = 86400000; // 24 hours

    bool downloadImage(const LayoutRegion& region);
    void drawImageInRegion(const LayoutRegion& region);
};

#endif
