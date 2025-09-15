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

    // Calculate 8% bottom bar dimensions (increased from 5% for better visibility)
    int bottomBarHeight = displayHeight * 0.08; // 8% of display height
    int bottomBarY = displayHeight - bottomBarHeight;

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);
    Serial.printf("Bottom bar: height=%d, y=%d\n", bottomBarHeight, bottomBarY);

    // Clear the entire battery area (rightmost 30% of bottom bar) with WHITE background
    int batteryAreaX = displayWidth * 7 / 10; // Start at 70% from left
    display.fillRect(batteryAreaX, bottomBarY, displayWidth * 3 / 10, bottomBarHeight, WHITE);

    // Position battery display in rightmost 30% of bottom bar - RIGHT ALIGNED
    int textSize = 2; // Larger text for better visibility
    int textHeight = textSize * 8; // Approximate text height
    int textY = bottomBarY + (bottomBarHeight - textHeight) / 2;

    // Calculate text width for right alignment
    int iconWidth = 20;
    int iconHeight = 12;
    int textWidth = 60; // Approximate width for "100%" text
    int totalWidth = textWidth + iconWidth + 5; // Text + icon + small gap
    int rightMargin = 10;

    // Right-align: start from right edge minus total width and margin
    int textX = displayWidth - totalWidth - rightMargin;
    int iconX = textX + textWidth + 5; // Icon after text with small gap
    int iconY = textY + 2;

    Serial.printf("Battery position (right-aligned): text=(%d,%d), icon=(%d,%d)\n", textX, textY, iconX, iconY);

    // Draw percentage text in BLACK on WHITE background - RIGHT ALIGNED
    display.setCursor(textX, textY);
    display.setTextSize(textSize);
    display.setTextColor(BLACK, WHITE); // Explicitly set BLACK text on WHITE background
    display.printf("%d%%", percentage);

    // Draw battery icon next to percentage in BLACK

    // Battery outline in BLACK
    display.drawRect(iconX, iconY, iconWidth, iconHeight, BLACK);
    // Battery tip in BLACK
    display.fillRect(iconX + iconWidth, iconY + 2, 2, iconHeight - 4, BLACK);

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

    // Battery area is rightmost 30% of 8% bottom bar (70% to 100%)
    int bottomBarHeight = displayHeight * 0.08;

    x = displayWidth * 7 / 10; // Start at 70%
    y = displayHeight - bottomBarHeight;
    width = displayWidth * 3 / 10; // 30% width
    height = bottomBarHeight;
}
