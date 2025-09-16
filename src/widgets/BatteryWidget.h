#ifndef BATTERY_WIDGET_H
#define BATTERY_WIDGET_H

#include "../core/Widget.h"

class BatteryWidget : public Widget {
public:
    BatteryWidget(Inkplate& display);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;

    // Battery-specific methods
    void forceUpdate();
    float getBatteryVoltage();
    int getBatteryPercentage();

private:
    unsigned long lastBatteryUpdate;

    static const unsigned long BATTERY_UPDATE_INTERVAL = 1800000; // 30 minutes
    static constexpr float MIN_BATTERY_VOLTAGE = 3.2;
    static constexpr float MAX_BATTERY_VOLTAGE = 4.2;

    void drawBatteryIndicator(const LayoutRegion& region);
    void drawBatteryIcon(int x, int y, int percentage, int iconWidth, int iconHeight);
};

#endif
