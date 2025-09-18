#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <esp_sleep.h>
#include <driver/rtc_io.h>

class PowerManager {
public:
    static void enableDeepSleep(unsigned long sleepTimeMs);
    static void enableWakeOnButton(int buttonPin);
    static void enableWakeOnTimer(unsigned long timeMs);
    static void enterDeepSleep();
    static void configureLowPowerMode();

private:
    static void disableUnusedPeripherals();
};

#endif
