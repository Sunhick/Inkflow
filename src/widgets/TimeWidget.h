#ifndef TIME_WIDGET_H
#define TIME_WIDGET_H

#include "../core/Widget.h"
#include <WiFi.h>

class TimeWidget : public Widget {
public:
    TimeWidget(Inkplate& display);
    TimeWidget(Inkplate& display, unsigned long updateInterval);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;

    // Time-specific methods
    void syncTimeWithNTP();
    void forceTimeSync();
    void forceUpdate();
    bool isTimeInitialized() const;
    String getFormattedDate();
    String getFormattedTime();
    String getDayOfWeek();

private:
    unsigned long lastTimeUpdate;
    bool timeInitialized;
    unsigned long timeUpdateInterval;

    static const unsigned long DEFAULT_TIME_UPDATE_INTERVAL = 900000; // 15 minutes
    static const char* NTP_SERVER;
    static const long GMT_OFFSET_SEC = -28800; // PST (UTC-8)
    static const int DAYLIGHT_OFFSET_SEC = 3600; // 1 hour

    void drawTimeDisplay(const LayoutRegion& region);
};

#endif
