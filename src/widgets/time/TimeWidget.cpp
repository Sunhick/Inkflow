#include "TimeWidget.h"
#include "../../core/Compositor.h"

const char* TimeWidget::NTP_SERVER = "pool.ntp.org";

// Backup NTP servers
const char* ntpServers[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "0.pool.ntp.org"
};

TimeWidget::TimeWidget(Inkplate& display)
    : Widget(display), lastTimeUpdate(0), timeInitialized(false), timeUpdateInterval(DEFAULT_TIME_UPDATE_INTERVAL) {}

TimeWidget::TimeWidget(Inkplate& display, unsigned long updateInterval)
    : Widget(display), lastTimeUpdate(0), timeInitialized(false), timeUpdateInterval(updateInterval) {
    Serial.printf("TimeWidget created with update interval: %lu ms (%lu seconds)\n", updateInterval, updateInterval / 1000);
}

void TimeWidget::begin() {
    Serial.println("Initializing time widget...");
    timeInitialized = false;
    lastTimeUpdate = 0;
}

bool TimeWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastTimeUpdate >= timeUpdateInterval) || (lastTimeUpdate == 0);
}

void TimeWidget::render(const LayoutRegion& region) {
    Serial.printf("TimeWidget::render() called - region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region before drawing to prevent text overwriting
    clearRegion(region);

    // Sync time if not initialized
    if (!timeInitialized) {
        Serial.println("Time not initialized, attempting NTP sync...");
        syncTimeWithNTP();
    }

    // Draw time content within the region
    Serial.println("About to call drawTimeDisplay()...");
    drawTimeDisplay(region);
    Serial.println("drawTimeDisplay() completed");

    lastTimeUpdate = millis();
    Serial.printf("TimeWidget::render() completed - lastTimeUpdate set to %lu\n", lastTimeUpdate);
}

void TimeWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    Serial.printf("TimeWidget::renderToCompositor() called - region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region on compositor before drawing to prevent text overwriting
    clearRegionOnCompositor(compositor, region);

    // Sync time if not initialized
    if (!timeInitialized) {
        Serial.println("Time not initialized, attempting NTP sync...");
        syncTimeWithNTP();
    }

    // Draw time content to compositor within the region
    Serial.println("About to call drawTimeDisplayToCompositor()...");
    drawTimeDisplayToCompositor(compositor, region);
    Serial.println("drawTimeDisplayToCompositor() completed");

    lastTimeUpdate = millis();
    Serial.printf("TimeWidget::renderToCompositor() completed - lastTimeUpdate set to %lu\n", lastTimeUpdate);
}

void TimeWidget::syncTimeWithNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot sync time");
        timeInitialized = false;
        return;
    }

    Serial.println("Syncing time with NTP server...");

    // Try multiple NTP servers
    for (int serverIndex = 0; serverIndex < 4; serverIndex++) {
        Serial.printf("Trying NTP server: %s\n", ntpServers[serverIndex]);

        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServers[serverIndex]);

        int attempts = 0;
        time_t now = 0;

        while (attempts < 10) {
            now = time(nullptr);

            if (now > 1577836800) { // January 1, 2020 00:00:00 UTC
                timeInitialized = true;
                Serial.println("Time synchronized successfully!");

                struct tm* timeinfo = localtime(&now);
                Serial.printf("Current time: %s", asctime(timeinfo));
                return;
            }

            delay(1000);
            attempts++;
        }

        Serial.printf("Server %s failed, trying next...\n", ntpServers[serverIndex]);
    }

    Serial.println("All NTP servers failed - time sync unsuccessful");
    timeInitialized = false;
}

void TimeWidget::drawTimeDisplay(const LayoutRegion& region) {
    Serial.println("TimeWidget::drawTimeDisplay() - Drawing normal time display");
    Serial.printf("TimeWidget region bounds: (%d,%d) %dx%d\n",
                  region.getX(), region.getY(), region.getWidth(), region.getHeight());

    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    Serial.printf("TimeWidget drawing at labelX=%d, labelY=%d\n", labelX, labelY);

    // Draw "DATE TIME" label
    display.setCursor(labelX, labelY + 20);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print("DATE TIME");
    Serial.println("Drew DATE TIME label");

    if (!timeInitialized) {
        display.setCursor(labelX, labelY + 60);
        display.setTextSize(2);
        display.setTextColor(0); // Black text
        display.setTextWrap(false);
        display.print("SYNC FAIL");
        Serial.println("Drew SYNC FAIL message");
        return;
    }

    // Get formatted time string
    String timeStr = getFormattedTime();
    Serial.printf("Time string: %s\n", timeStr.c_str());

    // Draw time
    display.setCursor(labelX, labelY + 60);
    display.setTextSize(3);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(timeStr);
    Serial.printf("Drew time string: %s\n", timeStr.c_str());

    // Draw date (larger font)
    String dateStr = getFormattedDate();
    display.setCursor(labelX, labelY + 110);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(dateStr);
    Serial.printf("Drew date string: %s\n", dateStr.c_str());

    // Draw day of week (larger font)
    String dayStr = getDayOfWeek();
    display.setCursor(labelX, labelY + 140);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(dayStr);
    Serial.printf("Drew day string: %s\n", dayStr.c_str());
}

String TimeWidget::getFormattedDate() {
    if (!timeInitialized) return "No Date";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%B %d, %Y", timeinfo);
    return String(buffer);
}

String TimeWidget::getFormattedTime() {
    if (!timeInitialized) return "No Time";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%I:%M %p", timeinfo);
    return String(buffer);
}

String TimeWidget::getDayOfWeek() {
    if (!timeInitialized) return "No Day";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%A", timeinfo);
    return String(buffer);
}

void TimeWidget::forceTimeSync() {
    Serial.println("Forcing time synchronization...");
    timeInitialized = false;
    syncTimeWithNTP();
}

bool TimeWidget::isTimeInitialized() const {
    return timeInitialized;
}

void TimeWidget::forceUpdate() {
    Serial.println("Force updating time widget...");
    lastTimeUpdate = 0; // Force next shouldUpdate() to return true
}

void TimeWidget::drawTimeDisplayToCompositor(Compositor& compositor, const LayoutRegion& region) {
    Serial.println("TimeWidget::drawTimeDisplayToCompositor() - Drawing time to compositor");

    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // For compositor rendering, we need to draw text as filled rectangles
    // This is a simplified implementation - in a full implementation you'd want
    // to render actual text to the compositor surface

    // Draw "DATE TIME" label area (simplified as a rectangle)
    compositor.fillRect(labelX, labelY + 15, 120, 20, 0); // Black rectangle for label
    Serial.println("Drew DATE TIME label area to compositor");

    if (!timeInitialized) {
        // Draw "SYNC FAIL" area
        compositor.fillRect(labelX, labelY + 55, 100, 20, 0); // Black rectangle
        Serial.println("Drew SYNC FAIL area to compositor");
        return;
    }

    // Draw time area (larger rectangle)
    compositor.fillRect(labelX, labelY + 50, 180, 30, 0); // Black rectangle for time
    Serial.println("Drew time area to compositor");

    // Draw date area
    compositor.fillRect(labelX, labelY + 100, 200, 20, 0); // Black rectangle for date
    Serial.println("Drew date area to compositor");

    // Draw day area
    compositor.fillRect(labelX, labelY + 130, 150, 20, 0); // Black rectangle for day
    Serial.println("Drew day area to compositor");

    // Note: This is a simplified implementation. For proper text rendering to compositor,
    // you would need to implement a text rendering system that draws to the compositor
    // surface pixel by pixel, or use a graphics library that can render to a buffer.
}
