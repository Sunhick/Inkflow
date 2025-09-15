#include "TimeManager.h"

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

    int displayWidth = display.width();
    int displayHeight = display.height();

    // Calculate 8% bottom bar dimensions (increased from 5% for better visibility)
    int bottomBarHeight = displayHeight * 0.08; // 8% of display height
    int bottomBarY = displayHeight - bottomBarHeight;

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);
    Serial.printf("Bottom bar: height=%d, y=%d\n", bottomBarHeight, bottomBarY);

    // Clear the time area (left half of bottom bar) with WHITE background
    clearTimeArea();

    // Draw thick separator line above bottom bar (only draw once from time manager)
    display.drawLine(0, bottomBarY - 1, displayWidth, bottomBarY - 1, BLACK);
    display.drawLine(0, bottomBarY - 2, displayWidth, bottomBarY - 2, BLACK);
    display.drawLine(0, bottomBarY - 3, displayWidth, bottomBarY - 3, BLACK);

    // Draw vertical separators between the three sections
    int timeWeatherSeparator = displayWidth / 2; // 50% mark (time | weather)
    int weatherBatterySeparator = displayWidth * 7 / 10; // 70% mark (weather | battery)

    // Separator between time and weather sections
    display.drawLine(timeWeatherSeparator, bottomBarY, timeWeatherSeparator, bottomBarY + bottomBarHeight, BLACK);
    // Separator between weather and battery sections
    display.drawLine(weatherBatterySeparator, bottomBarY, weatherBatterySeparator, bottomBarY + bottomBarHeight, BLACK);

    // Get formatted strings
    String dateStr = getFormattedDate();
    String timeStr = getFormattedTime();
    String dayStr = getDayOfWeek();

    Serial.printf("Date: %s, Time: %s, Day: %s\n", dateStr.c_str(), timeStr.c_str(), dayStr.c_str());

    // Position time display in left half of bottom bar - LEFT ALIGNED
    int textSize = 2; // Match battery font size
    int lineHeight = textSize * 8; // Height of one line of text
    int timeX = 5; // Left margin for left alignment

    // Single line format: "September 14, 2026 11:24PM (SUN)"
    int textY = bottomBarY + (bottomBarHeight - lineHeight) / 2; // Center vertically

    Serial.printf("Time position: (%d,%d)\n", timeX, textY);

    display.setTextSize(textSize);
    display.setTextColor(BLACK, WHITE); // Explicitly set BLACK text on WHITE background

    // Full format: "September 14, 2026 11:24PM (SUN)"
    display.setCursor(timeX, textY);
    String fullDateTime = getFullDateTime();
    display.print(fullDateTime);

    Serial.println("Time drawn to buffer");
}

void TimeManager::clearTimeArea() {
    int areaX, areaY, areaWidth, areaHeight;
    getTimeArea(areaX, areaY, areaWidth, areaHeight);

    // Clear with white background
    display.fillRect(areaX, areaY, areaWidth, areaHeight, WHITE);
}

void TimeManager::getTimeArea(int &x, int &y, int &width, int &height) {
    int displayWidth = display.width();
    int displayHeight = display.height();

    // Time area is left 50% of 8% bottom bar (0% to 50%)
    int bottomBarHeight = displayHeight * 0.08;

    x = 0;
    y = displayHeight - bottomBarHeight;
    width = displayWidth / 2; // 50% width
    height = bottomBarHeight;
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
            int displayWidth = display.width();
            int displayHeight = display.height();

            // Calculate 8% bottom bar position
            int bottomBarHeight = displayHeight * 0.08;
            int bottomBarY = displayHeight - bottomBarHeight;
            int textHeight = 1 * 8; // text size 1
            int textY = bottomBarY + (bottomBarHeight - textHeight) / 2;
            int timeX = 5;

            clearTimeArea();
            display.setCursor(timeX, textY);
            display.setTextSize(1);
            display.setTextColor(BLACK, WHITE); // Explicitly set BLACK text on WHITE background
            display.print("Time Sync Failed");
            lastTimeUpdate = millis();
            return;
        }
    }

    drawTimeDisplay(); // This now only draws to buffer
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
