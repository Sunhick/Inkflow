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

    int displayWidth = display.width();
    int displayHeight = display.height();

    // Calculate 5% bottom bar dimensions
    int bottomBarHeight = displayHeight * 0.05; // 5% of display height
    int bottomBarY = displayHeight - bottomBarHeight;

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);
    Serial.printf("Bottom bar: height=%d, y=%d\n", bottomBarHeight, bottomBarY);

    // Clear the entire battery area (right half of bottom bar) with WHITE background
    int batteryAreaX = displayWidth / 2;
    display.fillRect(batteryAreaX, bottomBarY, displayWidth / 2, bottomBarHeight, WHITE);

    // Position battery display in right half of bottom bar, centered vertically
    int textSize = 2; // Smaller text to fit in 5% height
    int textHeight = textSize * 8; // Approximate text height
    int textY = bottomBarY + (bottomBarHeight - textHeight) / 2;
    int textX = displayWidth - 120; // Right-aligned with margin

    Serial.printf("Battery position: (%d,%d)\n", textX, textY);

    // Draw percentage text in BLACK on WHITE background
    display.setCursor(textX, textY);
    display.setTextSize(textSize);
    display.setTextColor(BLACK, WHITE); // Explicitly set BLACK text on WHITE background
    display.printf("%d%%", percentage);

    // Draw battery icon next to percentage in BLACK
    int iconX = textX + 50;
    int iconY = textY + 2;
    int iconWidth = 24;
    int iconHeight = 12;

    // Battery outline in BLACK
    display.drawRect(iconX, iconY, iconWidth, iconHeight, BLACK);
    // Battery tip in BLACK
    display.fillRect(iconX + iconWidth, iconY + 3, 3, iconHeight - 6, BLACK);

    // Fill battery based on percentage in BLACK
    int fillWidth = ((iconWidth - 2) * percentage) / 100;
    if (fillWidth > 0) {
        display.fillRect(iconX + 1, iconY + 1, fillWidth, iconHeight - 2, BLACK);
    }

    Serial.println("Battery drawn to buffer");
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

void BatteryManager::drawBatteryToBuffer() {
    drawBatteryIndicator(); // This now only draws to buffer
    lastBatteryUpdate = millis();
}

void BatteryManager::getBatteryArea(int &x, int &y, int &width, int &height) {
    int displayWidth = display.width();
    int displayHeight = display.height();

    // Battery area is right half of 5% bottom bar
    int bottomBarHeight = displayHeight * 0.05;

    x = displayWidth / 2;
    y = displayHeight - bottomBarHeight;
    width = displayWidth / 2;
    height = bottomBarHeight;
}
