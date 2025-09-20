#include "BatteryWidget.h"

BatteryWidget::BatteryWidget(Inkplate& display)
    : Widget(display), lastBatteryUpdate(0), batteryUpdateInterval(DEFAULT_BATTERY_UPDATE_INTERVAL) {}

BatteryWidget::BatteryWidget(Inkplate& display, unsigned long updateInterval)
    : Widget(display), lastBatteryUpdate(0), batteryUpdateInterval(updateInterval) {
    Serial.printf("BatteryWidget created with update interval: %lu ms (%lu seconds)\n", updateInterval, updateInterval / 1000);
}

void BatteryWidget::begin() {
    Serial.println("Initializing battery widget...");
    lastBatteryUpdate = 0;
}

bool BatteryWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastBatteryUpdate >= batteryUpdateInterval) || (lastBatteryUpdate == 0);
}

void BatteryWidget::render(const LayoutRegion& region) {
    Serial.printf("Rendering battery widget in region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the widget region
    clearRegion(region);

    // Draw battery content within the region
    drawBatteryIndicator(region);

    lastBatteryUpdate = millis();
}

void BatteryWidget::forceUpdate() {
    Serial.println("Force updating battery widget...");
    lastBatteryUpdate = 0; // Force next shouldUpdate() to return true
}

float BatteryWidget::getBatteryVoltage() {
    return display.readBattery();
}

int BatteryWidget::getBatteryPercentage() {
    float voltage = getBatteryVoltage();

    if (voltage <= MIN_BATTERY_VOLTAGE) {
        return 0;
    } else if (voltage >= MAX_BATTERY_VOLTAGE) {
        return 100;
    } else {
        float range = MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE;
        float current = voltage - MIN_BATTERY_VOLTAGE;
        return (int)((current / range) * 100);
    }
}

void BatteryWidget::drawBatteryIndicator(const LayoutRegion& region) {
    int percentage = getBatteryPercentage();
    float voltage = getBatteryVoltage();

    Serial.printf("Drawing battery: %d%% (%.2fV)\n", percentage, voltage);

    // Calculate positions within the region
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // Draw "BATTERY" label
    display.setCursor(labelX, labelY);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("BATTERY");

    // Draw percentage text
    display.setCursor(labelX, labelY + 30);
    display.setTextSize(3);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.printf("%d%%", percentage);

    // Draw battery icon
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = labelX;
    int iconY = labelY + 70;

    drawBatteryIcon(iconX, iconY, percentage, iconWidth, iconHeight);

    // Draw voltage info
    display.setCursor(labelX, labelY + 100);
    display.setTextSize(1);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.printf("%.2fV", voltage);
}

void BatteryWidget::drawBatteryIcon(int x, int y, int percentage, int iconWidth, int iconHeight) {
    // Battery outline in black
    display.drawRect(x, y, iconWidth, iconHeight, 0);
    display.drawRect(x - 1, y - 1, iconWidth + 2, iconHeight + 2, 0); // Thicker outline

    // Battery tip in black
    display.fillRect(x + iconWidth, y + 4, 4, iconHeight - 8, 0);

    // Fill battery based on percentage in black
    int fillWidth = ((iconWidth - 4) * percentage) / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 2, y + 2, fillWidth, iconHeight - 4, 0);
    }
}
