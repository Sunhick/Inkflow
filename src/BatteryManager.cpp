#include "BatteryManager.h"

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

    int displayWidth = display.width();
    int displayHeight = display.height();

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);

    // Position battery display in bottom-right corner
    int textX = displayWidth - 150;
    int textY = displayHeight - 50;

    Serial.printf("Battery position: (%d,%d)\n", textX, textY);

    // Clear the battery area with white background
    display.fillRect(textX - 10, textY - 10, 140, 40, WHITE);

    // Draw border for visibility (debug)
    display.drawRect(textX - 10, textY - 10, 140, 40, BLACK);

    // Draw percentage text
    display.setCursor(textX, textY);
    display.setTextSize(3);
    display.setTextColor(BLACK);
    display.printf("%d%%", percentage);

    // Draw simple battery icon
    int iconX = textX + 80;
    int iconY = textY + 5;
    display.drawRect(iconX, iconY, 30, 15, BLACK);
    display.fillRect(iconX + 30, iconY + 4, 4, 7, BLACK);

    // Fill battery based on percentage
    int fillWidth = (28 * percentage) / 100;
    if (fillWidth > 0) {
        display.fillRect(iconX + 1, iconY + 1, fillWidth, 13, BLACK);
    }

    Serial.println("Battery drawn, updating display...");
    display.display();
}

void BatteryManager::drawBatteryPercentage(int percentage) {
    // This method is now integrated into drawBatteryIndicator
}

void BatteryManager::drawBatteryIcon(int x, int y, int percentage) {
    // This method is now integrated into drawBatteryIndicator
}

void BatteryManager::clearBatteryArea() {
    // This method is now integrated into drawBatteryIndicator
}

void BatteryManager::getBatteryArea(int &x, int &y, int &width, int &height) {
    int displayWidth = display.width();
    int displayHeight = display.height();

    x = displayWidth - 160;
    y = displayHeight - 60;
    width = 160;
    height = 60;
}
