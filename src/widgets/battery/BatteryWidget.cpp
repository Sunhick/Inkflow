#include "BatteryWidget.h"
#include "../../core/Logger.h"
#include "../../core/Compositor.h"
#include "../../managers/ConfigManager.h"

BatteryWidget::BatteryWidget(Inkplate& display)
    : Widget(display), lastBatteryUpdate(0), batteryUpdateInterval(DEFAULT_BATTERY_UPDATE_INTERVAL) {}

BatteryWidget::BatteryWidget(Inkplate& display, unsigned long updateInterval)
    : Widget(display), lastBatteryUpdate(0), batteryUpdateInterval(updateInterval) {
    LOG_INFO("BatteryWidget", "Created with update interval: %lu ms (%lu seconds)", updateInterval, updateInterval / 1000);
}

void BatteryWidget::begin() {
    LOG_INFO("BatteryWidget", "Initializing battery widget...");
    lastBatteryUpdate = 0;
}

bool BatteryWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastBatteryUpdate >= batteryUpdateInterval) || (lastBatteryUpdate == 0);
}

void BatteryWidget::render(const LayoutRegion& region) {
    LOG_DEBUG("BatteryWidget", "render() called - region: %dx%d at (%d,%d)",
              region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region before drawing to prevent text overwriting
    clearRegion(region);

    // Draw battery content within the region
    LOG_DEBUG("BatteryWidget", "About to call drawBatteryIndicator()...");
    drawBatteryIndicator(region);
    LOG_DEBUG("BatteryWidget", "drawBatteryIndicator() completed");

    lastBatteryUpdate = millis();
    LOG_DEBUG("BatteryWidget", "render() completed - lastBatteryUpdate set to %lu", lastBatteryUpdate);
}

void BatteryWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    LOG_DEBUG("BatteryWidget", "renderToCompositor() called - region: %dx%d at (%d,%d)",
              region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region on compositor before drawing to prevent text overwriting
    clearRegionOnCompositor(compositor, region);

    // Draw battery content to compositor within the region
    LOG_DEBUG("BatteryWidget", "About to call drawBatteryIndicatorToCompositor()...");
    drawBatteryIndicatorToCompositor(compositor, region);
    LOG_DEBUG("BatteryWidget", "drawBatteryIndicatorToCompositor() completed");

    lastBatteryUpdate = millis();
    LOG_DEBUG("BatteryWidget", "renderToCompositor() completed - lastBatteryUpdate set to %lu", lastBatteryUpdate);
}

void BatteryWidget::forceUpdate() {
    LOG_INFO("BatteryWidget", "Force updating battery widget...");
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

    LOG_DEBUG("BatteryWidget", "drawBatteryIndicator() - Drawing battery: %d%% (%.2fV)", percentage, voltage);
    LOG_DEBUG("BatteryWidget", "region bounds: (%d,%d) %dx%d",
              region.getX(), region.getY(), region.getWidth(), region.getHeight());

    // Calculate positions within the region
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    LOG_DEBUG("BatteryWidget", "drawing at labelX=%d, labelY=%d", labelX, labelY);

    // Draw "BATTERY" label
    display.setCursor(labelX, labelY + 20);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print("BATTERY");
    LOG_DEBUG("BatteryWidget", "Drew BATTERY label");

    // Draw percentage text (smaller size)
    display.setCursor(labelX, labelY + 60);
    display.setTextSize(3);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.printf("%d%%", percentage);
    LOG_DEBUG("BatteryWidget", "Drew percentage: %d%%", percentage);

    // Draw battery icon (smaller size)
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = labelX;
    int iconY = labelY + 100;

    drawBatteryIcon(iconX, iconY, percentage, iconWidth, iconHeight);
    LOG_DEBUG("BatteryWidget", "Drew battery icon");

    // Draw voltage info (adjusted position for smaller icon)
    display.setCursor(labelX, labelY + 130);
    display.setTextSize(1);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.printf("%.2fV", voltage);
    LOG_DEBUG("BatteryWidget", "Drew voltage: %.2fV", voltage);
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

    LOG_DEBUG("BatteryWidget", "drawBatteryIndicatorToCompositor() - Drawing battery: %d%% (%.2fV)", percentage, voltage);

    // Calculate positions within the region
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // Draw "BATTERY" label area (simplified as a rectangle)
    compositor.fillRect(labelX, labelY + 15, 80, 20, 0); // Black rectangle for label
    LOG_DEBUG("BatteryWidget", "Drew BATTERY label area to compositor");

    // Draw percentage area (larger rectangle)
    compositor.fillRect(labelX, labelY + 50, 60, 30, 0); // Black rectangle for percentage
    LOG_DEBUG("BatteryWidget", "Drew percentage area to compositor: %d%%", percentage);

    // Draw battery icon to compositor
    int iconWidth = 40;
    int iconHeight = 20;
    int iconX = labelX;
    int iconY = labelY + 100;

    drawBatteryIconToCompositor(compositor, iconX, iconY, percentage, iconWidth, iconHeight);
    LOG_DEBUG("BatteryWidget", "Drew battery icon to compositor");

    // Draw voltage info area
    compositor.fillRect(labelX, labelY + 125, 50, 15, 0); // Black rectangle for voltage
    LOG_DEBUG("BatteryWidget", "Drew voltage area to compositor: %.2fV", voltage);
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
WidgetType BatteryWidget::getWidgetType() const {
    return WidgetTypeTraits<BatteryWidget>::type();
}
