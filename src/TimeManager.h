#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Inkplate.h>
#include <WiFi.h>
#include <time.h>

class TimeManager {
public:
    TimeManager(Inkplate &display);

    void begin();
    void updateTimeDisplay();
    void forceUpdate();
    void drawTimeToBuffer(); // Draw without updating display
    bool shouldUpdate();
    void syncTimeWithNTP();
    void forceTimeSync(); // Force immediate time sync
    bool isTimeInitialized() const;

private:
    Inkplate &display;
    unsigned long lastTimeUpdate;
    bool timeInitialized;

    static const unsigned long TIME_UPDATE_INTERVAL = 1800000; // 30 minutes in milliseconds
    static const char* NTP_SERVER;
    static const long GMT_OFFSET_SEC = -28800; // PST (UTC-8) - adjust for your timezone
    static const int DAYLIGHT_OFFSET_SEC = 3600; // DST offset (1 hour)

    void drawTimeDisplay();
    void getTimeArea(int &x, int &y, int &width, int &height);
    void clearTimeArea();
    String getFormattedDate();
    String getFormattedTime();
    String getCompactDate();
    String getFullDateTime();
    String getDayOfWeek();
};

#endif
