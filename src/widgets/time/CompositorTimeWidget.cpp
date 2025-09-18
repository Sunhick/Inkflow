#include "CompositorTimeWidget.h"

const char* CompositorTimeWidget::NTP_SERVER = "pool.ntp.org";

// Backup NTP servers (static to avoid multiple definition)
static const char* compositorNtpServers[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "0.pool.ntp.org"
};

CompositorTimeWidget::CompositorTimeWidget(DisplayCompositor& compositor)
    : CompositorWidget(compositor), lastTimeUpdate(0), timeInitialized(false),
      timeUpdateInterval(DEFAULT_TIME_UPDATE_INTERVAL) {}

CompositorTimeWidget::CompositorTimeWidget(DisplayCompositor& compositor, unsigned long updateInterval)
    : CompositorWidget(compositor), lastTimeUpdate(0), timeInitialized(false),
      timeUpdateInterval(updateInterval) {
    Serial.printf("CompositorTimeWidget created with update interval: %lu ms (%lu seconds)\n",
                  updateInterval, updateInterval / 1000);
}

void CompositorTimeWidget::begin() {
    Serial.println("Initializing compositor time widget...");
    timeInitialized = false;
    lastTimeUpdate = 0;
}

bool CompositorTimeWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastTimeUpdate >= timeUpdateInterval) || (lastTimeUpdate == 0);
}

void CompositorTimeWidget::renderToSurface(VirtualSurface* surface, const LayoutRegion& region) {
    Serial.printf("Rendering time widget to surface in region: %dx%d at (%d,%d)\n",
                  region.width, region.height, region.x, region.y);

    // Clear the widget region on the surface
    surface->clearRegion(region);

    // Sync time if not initialized
    if (!timeInitialized) {
        Serial.println("Time not initialized, attempting NTP sync...");
        syncTimeWithNTP();
    }

    // Draw time content to the surface
    drawTimeDisplay(surface, region);

    lastTimeUpdate = millis();
}

void CompositorTimeWidget::syncTimeWithNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot sync time");
        timeInitialized = false;
        return;
    }

    Serial.println("Syncing time with NTP server...");

    // Try multiple NTP servers
    for (int serverIndex = 0; serverIndex < 4; serverIndex++) {
        Serial.printf("Trying NTP server: %s\n", compositorNtpServers[serverIndex]);

        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, compositorNtpServers[serverIndex]);

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

        Serial.printf("Server %s failed, trying next...\n", compositorNtpServers[serverIndex]);
    }

    Serial.println("All NTP servers failed - time sync unsuccessful");
    timeInitialized = false;
}

void CompositorTimeWidget::drawTimeDisplay(VirtualSurface* surface, const LayoutRegion& region) {
    int margin = 10;
    int labelX = region.x + margin;
    int labelY = region.y + margin;

    // Draw "DATE & TIME" label
    surface->setCursor(labelX, labelY);
    surface->setTextSize(2);
    surface->setTextColor(0);
    surface->setTextWrap(true);
    surface->print("DATE & TIME");

    if (!timeInitialized) {
        surface->setCursor(labelX, labelY + 40);
        surface->setTextSize(1);
        surface->setTextColor(0);
        surface->setTextWrap(true);
        surface->print("Time Sync Failed");
        return;
    }

    // Get formatted strings
    String dateStr = getFormattedDate();
    String timeStr = getFormattedTime();
    String dayStr = getDayOfWeek();

    // Draw day of week
    surface->setCursor(labelX, labelY + 30);
    surface->setTextSize(2);
    surface->setTextColor(0);
    surface->setTextWrap(true);
    dayStr.toUpperCase();
    surface->print(dayStr);

    // Draw date
    surface->setCursor(labelX, labelY + 60);
    surface->setTextSize(2);
    surface->setTextColor(0);
    surface->setTextWrap(true);
    surface->print(dateStr);

    // Draw time (larger)
    surface->setCursor(labelX, labelY + 90);
    surface->setTextSize(3);
    surface->setTextColor(0);
    surface->setTextWrap(true);
    surface->print(timeStr);
}

String CompositorTimeWidget::getFormattedDate() {
    if (!timeInitialized) return "No Date";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%B %d, %Y", timeinfo);
    return String(buffer);
}

String CompositorTimeWidget::getFormattedTime() {
    if (!timeInitialized) return "No Time";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%I:%M %p", timeinfo);
    return String(buffer);
}

String CompositorTimeWidget::getDayOfWeek() {
    if (!timeInitialized) return "No Day";

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char buffer[16];
    strftime(buffer, sizeof(buffer), "%A", timeinfo);
    return String(buffer);
}

void CompositorTimeWidget::forceTimeSync() {
    Serial.println("Forcing time synchronization...");
    timeInitialized = false;
    syncTimeWithNTP();
}

bool CompositorTimeWidget::isTimeInitialized() const {
    return timeInitialized;
}

void CompositorTimeWidget::forceUpdate() {
    Serial.println("Force updating compositor time widget...");
    lastTimeUpdate = 0; // Force next shouldUpdate() to return true
}
