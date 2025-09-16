#include "TimeManager.h"
#include "ImageFetcher.h"
#include "Config.h"

const char* TimeManager::NTP_SERVER = "pool.ntp.org";

// Backup NTP servers
const char* ntpServers[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "0.pool.ntp.org"
};

TimeManager::TimeManager(Inkplate &display)
    : display(display), lastTimeUpdate(0), timeInitialized(false) {}

void TimeManager::begin() {
    Serial.println("Initializing time management...");
    // Don't sync time here - WiFi might not be connected yet
    timeInitialized = false;
    lastTimeUpdate = 0; // Force initial update
}

void TimeManager::updateTimeDisplay() {
    if (!shouldUpdate()) {
        return;
    }
    forceUpdate();
}

void TimeManager::forceUpdate() {
    Serial.println("Force updating time display...");

    if (!timeInitialized) {
        Serial.println("Time not initialized, syncing with NTP...");
        syncTimeWithNTP();
    }

    drawTimeDisplay();
    Serial.println("Time drawn, updating display...");
    display.display();
    lastTimeUpdate = millis();
}

bool TimeManager::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastTimeUpdate >= TIME_UPDATE_INTERVAL) || (lastTimeUpdate == 0);
}

void TimeManager::syncTimeWithNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot sync time");
        timeInitialized = false;
        return;
    }

    Serial.println("Syncing time with NTP server...");
    Serial.printf("GMT offset: %ld seconds (%.1f hours)\n", GMT_OFFSET_SEC, GMT_OFFSET_SEC / 3600.0);
    Serial.printf("DST offset: %d seconds\n", DAYLIGHT_OFFSET_SEC);

    // Try multiple NTP servers
    for (int serverIndex = 0; serverIndex < 4; serverIndex++) {
        Serial.printf("Trying NTP server: %s\n", ntpServers[serverIndex]);

        // Configure time with current NTP server
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServers[serverIndex]);

        // Wait for time to be set - check for a reasonable timestamp
        int attempts = 0;
        time_t now = 0;

        Serial.print("Waiting for NTP sync");
        while (attempts < 10) {
            now = time(nullptr);

            // Check if we have a reasonable timestamp (after year 2020)
            if (now > 1577836800) { // January 1, 2020 00:00:00 UTC
                timeInitialized = true;
                Serial.println("\nTime synchronized successfully!");

                // Print current time for verification
                struct tm* timeinfo = localtime(&now);
                Serial.printf("Server: %s\n", ntpServers[serverIndex]);
                Serial.printf("Current time: %s", asctime(timeinfo));
                Serial.printf("Unix timestamp: %ld\n", now);

                // Verify the time makes sense
                if (timeinfo->tm_year > 120) { // Year > 2020
                    Serial.println("Time validation passed");
                    return;
                } else {
                    Serial.println("Time validation failed - trying next server");
                    timeInitialized = false;
                    break;
                }
            }

            Serial.print(".");
            delay(1000);
            attempts++;
        }

        Serial.printf("\nServer %s failed, trying next...\n", ntpServers[serverIndex]);
    }

    Serial.println("All NTP servers failed - time sync unsuccessful");
    timeInitialized = false;
}

void TimeManager::drawTimeDisplay() {
    Serial.println("Drawing time display...");

    if (!timeInitialized) {
        Serial.println("Time not initialized, cannot draw time");
        return;
    }

    int displayHeight = display.height();

    // Position time in the top third of the left sidebar
    int sidebarHeight = displayHeight / 3; // Each section gets 1/3 of height
    int timeY = 10; // Top third with margin
    int timeX = 10; // Left margin

    Serial.printf("Time position in sidebar: x=%d, y=%d, height=%d\n", timeX, timeY, sidebarHeight);

    // Clear the time area in sidebar
    clearTimeArea();

    // Draw "DATE & TIME" label
    display.setCursor(timeX, timeY);
    display.setTextSize(2);
    display.setTextColor(0);
    display.print("DATE & TIME");

    // Get formatted strings
    String dateStr = getFormattedDate();
    String timeStr = getFormattedTime();
    String dayStr = getDayOfWeek();

    Serial.printf("Date: %s, Time: %s, Day: %s\n", dateStr.c_str(), timeStr.c_str(), dayStr.c_str());

    // Draw day of week
    display.setCursor(timeX, timeY + 30);
    display.setTextSize(2);
    display.setTextColor(0);
    dayStr.toUpperCase();
    display.print(dayStr);

    // Draw date
    display.setCursor(timeX, timeY + 60);
    display.setTextSize(2);
    display.setTextColor(0);
    display.print(dateStr);

    // Draw time (larger)
    display.setCursor(timeX, timeY + 90);
    display.setTextSize(3);
    display.setTextColor(0);
    display.print(timeStr);

    // Draw horizontal separator lines between sections
    int sectionHeight = displayHeight / 3;

    // Line between time and weather sections
    int line1Y = sectionHeight - 2;
    display.drawLine(5, line1Y, SIDEBAR_WIDTH - 5, line1Y, 0);
    display.drawLine(5, line1Y + 1, SIDEBAR_WIDTH - 5, line1Y + 1, 0);

    // Line between weather and battery sections
    int line2Y = (sectionHeight * 2) - 2;
    display.drawLine(5, line2Y, SIDEBAR_WIDTH - 5, line2Y, 0);
    display.drawLine(5, line2Y + 1, SIDEBAR_WIDTH - 5, line2Y + 1, 0);

    Serial.println("Time drawn to sidebar buffer");
}

void TimeManager::clearTimeArea() {
    int areaX, areaY, areaWidth, areaHeight;
    getTimeArea(areaX, areaY, areaWidth, areaHeight);

    // Clear with white background
    display.fillRect(areaX, areaY, areaWidth, areaHeight, 7);
}

void TimeManager::getTimeArea(int &x, int &y, int &width, int &height) {
    int displayHeight = display.height();

    // Time area is top third of left sidebar, but leave space for separator line
    int sidebarHeight = displayHeight / 3;

    x = 0;
    y = 0; // Top third
    width = SIDEBAR_WIDTH;
    height = sidebarHeight - 2; // Leave space for separator line below
}

String TimeManager::getFormattedDate() {
    if (!timeInitialized) return "No Date";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%B %d, %Y", timeinfo);
    return String(buffer);
}

String TimeManager::getFormattedTime() {
    if (!timeInitialized) return "No Time";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%I:%M %p", timeinfo);
    return String(buffer);
}

String TimeManager::getCompactDate() {
    if (!timeInitialized) return "No Date";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%b %d", timeinfo); // "Dec 15" format
    return String(buffer);
}

String TimeManager::getFullDateTime() {
    if (!timeInitialized) return "Time Sync Failed";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[64];
    // Format: "September 14, 2026 11:24PM (SUN)"
    strftime(buffer, sizeof(buffer), "%B %d, %Y %I:%M%p (%a)", timeinfo);

    // Convert to uppercase for day abbreviation
    String result = String(buffer);
    result.replace("(sun)", "(SUN)");
    result.replace("(mon)", "(MON)");
    result.replace("(tue)", "(TUE)");
    result.replace("(wed)", "(WED)");
    result.replace("(thu)", "(THU)");
    result.replace("(fri)", "(FRI)");
    result.replace("(sat)", "(SAT)");

    return result;
}

void TimeManager::drawTimeToBuffer() {
    if (!timeInitialized) {
        Serial.println("Time not initialized, attempting NTP sync...");
        syncTimeWithNTP();

        // If still not initialized, show error message
        if (!timeInitialized) {
            Serial.println("NTP sync failed, showing error message");
            int displayHeight = display.height();
            int sidebarHeight = displayHeight / 3;
            int timeY = 10;
            int timeX = 10;

            clearTimeArea();
            display.setCursor(timeX, timeY);
            display.setTextSize(2);
            display.setTextColor(0);
            display.print("DATE & TIME");

            display.setCursor(timeX, timeY + 40);
            display.setTextSize(1);
            display.setTextColor(0);
            display.print("Time Sync Failed");
            ImageFetcher::drawVerticalSeparator(display); // Ensure separator is visible
            lastTimeUpdate = millis();
            return;
        }
    }

    drawTimeDisplay(); // This now only draws to buffer
    ImageFetcher::drawVerticalSeparator(display); // Ensure separator is visible
    lastTimeUpdate = millis();
}

void TimeManager::forceTimeSync() {
    Serial.println("Forcing time synchronization...");
    timeInitialized = false;
    syncTimeWithNTP();
}

bool TimeManager::isTimeInitialized() const {
    return timeInitialized;
}

String TimeManager::getDayOfWeek() {
    if (!timeInitialized) return "No Day";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%A", timeinfo);
    return String(buffer);
}
