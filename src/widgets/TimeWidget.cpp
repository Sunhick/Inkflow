#include "TimeWidget.h"

const char* TimeWidget::NTP_SERVER = "pool.ntp.org";

// Backup NTP servers
const char* ntpServers[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "0.pool.ntp.org"
};

TimeWidget::TimeWidget(Inkplate& display)
    : Widget(display), lastTimeUpdate(0), timeInitialized(false) {}

void TimeWidget::begin() {
    Serial.println("Initializing time widget...");
    timeInitialized = false;
    lastTimeUpdate = 0;
}

bool TimeWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastTimeUpdate >= TIME_UPDATE_INTERVAL) || (lastTimeUpdate == 0);
}

void TimeWidget::render(const LayoutRegion& region) {
    Serial.printf("Rendering time widget in region: %dx%d at (%d,%d)\n",
                  region.width, region.height, region.x, region.y);

    // Clear the widget region
    clearRegion(region);

    // Sync time if not initialized
    if (!timeInitialized) {
        Serial.println("Time not initialized, attempting NTP sync...");
        syncTimeWithNTP();
    }

    // Draw time content within the region
    drawTimeDisplay(region);

    lastTimeUpdate = millis();
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
    int margin = 10;
    int labelX = region.x + margin;
    int labelY = region.y + margin;

    // Draw "DATE & TIME" label
    display.setCursor(labelX, labelY);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("DATE & TIME");

    if (!timeInitialized) {
        display.setCursor(labelX, labelY + 40);
        display.setTextSize(1);
        display.setTextColor(0);
        display.setTextWrap(true);
        display.print("Time Sync Failed");
        return;
    }

    // Get formatted strings
    String dateStr = getFormattedDate();
    String timeStr = getFormattedTime();
    String dayStr = getDayOfWeek();

    // Draw day of week
    display.setCursor(labelX, labelY + 30);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    dayStr.toUpperCase();
    display.print(dayStr);

    // Draw date
    display.setCursor(labelX, labelY + 60);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print(dateStr);

    // Draw time (larger)
    display.setCursor(labelX, labelY + 90);
    display.setTextSize(3);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print(timeStr);
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
