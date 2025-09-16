#include "BatteryManager.h"
#include "ImageFetcher.h"
#include "Config.h"

BatteryManager::BatteryManager(Inkplate &display)
    : display(display), lastBatteryUpdate(0) {}

void BatteryManager::begin() {
    Serial.println("Initializing battery monitoring...");
    lastBatteryUpdate = 0;
}

void BatteryManager::updateBatteryDisplay() {
    if (!shouldUpdate()) {
        return;
    }
    forceUpdate();
}

void BatteryManager::forceUpdate() {
    Serial.println("Force updating battery display...");

    float voltage = getBatteryVoltage();
    int percentage = getBatteryPercentage();

    Serial.printf("Battery: %.2fV (%d%%)\n", voltage, percentage);

    drawBatteryIndicator();
    Serial.println("Battery drawn, updating display...");
    display.display();
    lastBatteryUpdate = millis();
}

bool BatteryManager::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) || (lastBatteryUpdate == 0);
}

float BatteryManager::getBatteryVoltage() {
    return display.readBattery();
}

int BatteryManager::getBatteryPercentage() {
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

void BatteryManager::drawBatteryIndicator() {
    int percentage = getBatteryPercentage();

    Serial.printf("Drawing battery indicator: %d%%\n", percentage);

    int displayHeight = display.height();

    // Position battery in the bottom third of the left sidebar
    int sidebarHeight = displayHeight / 3; // Each section gets 1/3 of height
    int batteryY = displayHeight - sidebarHeight; // Bottom third
    int batteryX = 10; // Left margin

    Serial.printf("Battery position in sidebar: x=%d, y=%d, height=%d\n", batteryX, batteryY, sidebarHeight);

    // Clear the battery area in sidebar
    clearBatteryArea();

    // Draw "BATTERY" label
    display.setCursor(batteryX, batteryY + 10);
    display.setTextSize(2);
    display.setTextColor(0);
    display.print("BATTERY");

    // Draw percentage text
    display.setCursor(batteryX, batteryY + 40);
    display.setTextSize(3); // Larger text for sidebar
    display.setTextColor(0);
    display.printf("%d%%", percentage);

    // Draw battery icon below percentage
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = batteryX;
    int iconY = batteryY + 80;

    // Battery outline in black
    display.drawRect(iconX, iconY, iconWidth, iconHeight, 0);
    display.drawRect(iconX - 1, iconY - 1, iconWidth + 2, iconHeight + 2, 0); // Thicker outline

    // Battery tip in black
    display.fillRect(iconX + iconWidth, iconY + 4, 4, iconHeight - 8, 0);

    // Fill battery based on percentage in black
    int fillWidth = ((iconWidth - 4) * percentage) / 100;
    if (fillWidth > 0) {
        display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4, 0);
    }

    // Draw voltage info
    float voltage = getBatteryVoltage();
    display.setCursor(batteryX, batteryY + 110);
    display.setTextSize(1);
    display.setTextColor(0);
    display.printf("%.2fV", voltage);

    Serial.println("Battery drawn to sidebar buffer");
}

void BatteryManager::drawBatteryPercentage(int percentage) {
    // This method is now integrated into drawBatteryIndicator
}

void BatteryManager::drawBatteryIcon(int x, int y, int percentage) {
    // This method is now integrated into drawBatteryIndicator
}

void BatteryManager::clearBatteryArea() {
    int areaX, areaY, areaWidth, areaHeight;
    getBatteryArea(areaX, areaY, areaWidth, areaHeight);

    // Clear with white background
    display.fillRect(areaX, areaY, areaWidth, areaHeight, 7);
}

void BatteryManager::drawBatteryToBuffer() {
    drawBatteryIndicator(); // This now only draws to buffer
    ImageFetcher::drawVerticalSeparator(display); // Ensure separator is visible
    lastBatteryUpdate = millis();
}

void BatteryManager::getBatteryArea(int &x, int &y, int &width, int &height) {
    int displayHeight = display.height();

    // Battery area is bottom third of left sidebar, but leave space for separator line
    int sidebarHeight = displayHeight / 3;

    x = 0;
    y = displayHeight - sidebarHeight + 2; // Start after the separator line
    width = SIDEBAR_WIDTH;
    height = sidebarHeight - 2; // Leave space for separator line above
}
