#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Inkplate.h>

class BatteryManager {
public:
    BatteryManager(Inkplate &display);

    void begin();
    void updateBatteryDisplay();
    void forceUpdate();
    void drawBatteryToBuffer(); // Draw without updating display
    bool shouldUpdate();
    float getBatteryVoltage();
    int getBatteryPercentage();

private:
    Inkplate &display;
    unsigned long lastBatteryUpdate;

    static const unsigned long BATTERY_UPDATE_INTERVAL = 1800000; // 30 minutes in milliseconds
    static constexpr float MIN_BATTERY_VOLTAGE = 3.2; // Minimum safe voltage
    static constexpr float MAX_BATTERY_VOLTAGE = 4.2; // Maximum voltage (fully charged)

    void drawBatteryIndicator();
    void drawBatteryPercentage(int percentage);
    void drawBatteryIcon(int x, int y, int percentage);
    void clearBatteryArea();
    void getBatteryArea(int &x, int &y, int &width, int &height);
};

#endif
