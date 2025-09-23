#include "BatteryWidget.h"
#include "../../core/Compositor.h"

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
    Serial.printf("BatteryWidget::render() called - region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region before drawing to prevent text overwriting
    clearRegion(region);

    // Draw battery content within the region
    Serial.println("About to call drawBatteryIndicator()...");
    drawBatteryIndicator(region);
    Serial.println("drawBatteryIndicator() completed");

    lastBatteryUpdate = millis();
    Serial.printf("BatteryWidget::render() completed - lastBatteryUpdate set to %lu\n", lastBatteryUpdate);
}

void BatteryWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    Serial.printf("BatteryWidget::renderToCompositor() called - region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region on compositor before drawing to prevent text overwriting
    clearRegionOnCompositor(compositor, region);

    // Draw battery content to compositor within the region
    Serial.println("About to call drawBatteryIndicatorToCompositor()...");
    drawBatteryIndicatorToCompositor(compositor, region);
    Serial.println("drawBatteryIndicatorToCompositor() completed");

    lastBatteryUpdate = millis();
    Serial.printf("BatteryWidget::renderToCompositor() completed - lastBatteryUpdate set to %lu\n", lastBatteryUpdate);
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

    Serial.printf("BatteryWidget::drawBatteryIndicator() - Drawing battery: %d%% (%.2fV)\n", percentage, voltage);
    Serial.printf("BatteryWidget region bounds: (%d,%d) %dx%d\n",
                  region.getX(), region.getY(), region.getWidth(), region.getHeight());

    // Calculate positions within the region
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    Serial.printf("BatteryWidget drawing at labelX=%d, labelY=%d\n", labelX, labelY);

    // Draw "BATTERY" label
    display.setCursor(labelX, labelY + 20);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print("BATTERY");
    Serial.println("Drew BATTERY label");

    // Draw percentage text (smaller size)
    display.setCursor(labelX, labelY + 60);
    display.setTextSize(3);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.printf("%d%%", percentage);
    Serial.printf("Drew percentage: %d%%\n", percentage);

    // Draw battery icon (smaller size)
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = labelX;
    int iconY = labelY + 100;

    drawBatteryIcon(iconX, iconY, percentage, iconWidth, iconHeight);
    Serial.println("Drew battery icon");

    // Draw voltage info (adjusted position for smaller icon)
    display.setCursor(labelX, labelY + 130);
    display.setTextSize(1);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.printf("%.2fV", voltage);
    Serial.printf("Drew voltage: %.2fV\n", voltage);
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

void BatteryWidget::drawBatteryIndicatorToCompositor(Compositor& compositor, const LayoutRegion& region) {
    int percentage = getBatteryPercentage();
    float voltage = getBatteryVoltage();

    Serial.printf("BatteryWidget::drawBatteryIndicatorToCompositor() - Drawing battery: %d%% (%.2fV)\n", percentage, voltage);

    // Calculate positions within the region
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // Draw "BATTERY" label area (simplified as a rectangle)
    compositor.fillRect(labelX, labelY + 15, 80, 20, 0); // Black rectangle for label
    Serial.println("Drew BATTERY label area to compositor");

    // Draw percentage area (larger rectangle)
    compositor.fillRect(labelX, labelY + 50, 60, 30, 0); // Black rectangle for percentage
    Serial.printf("Drew percentage area to compositor: %d%%\n", percentage);

    // Draw battery icon to compositor
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = labelX;
    int iconY = labelY + 100;

    drawBatteryIconToCompositor(compositor, iconX, iconY, percentage, iconWidth, iconHeight);
    Serial.println("Drew battery icon to compositor");

    // Draw voltage info area
    compositor.fillRect(labelX, labelY + 125, 50, 15, 0); // Black rectangle for voltage
    Serial.printf("Drew voltage area to compositor: %.2fV\n", voltage);
}

void BatteryWidget::drawBatteryIconToCompositor(Compositor& compositor, int x, int y, int percentage, int iconWidth, int iconHeight) {
    // Battery outline in black
    compositor.drawRect(x, y, iconWidth, iconHeight, 0);
    compositor.drawRect(x - 1, y - 1, iconWidth + 2, iconHeight + 2, 0); // Thicker outline

    // Battery tip in black
    compositor.fillRect(x + iconWidth, y + 4, 4, iconHeight - 8, 0);

    // Fill battery based on percentage in black
    int fillWidth = ((iconWidth - 4) * percentage) / 100;
    if (fillWidth > 0) {
        compositor.fillRect(x + 2, y + 2, fillWidth, iconHeight - 4, 0);
    }
}
