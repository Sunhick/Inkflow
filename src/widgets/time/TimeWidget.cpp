#include "TimeWidget.h"
#include "../../core/Logger.h"
#include "../../core/Compositor.h"
#include "../../managers/ConfigManager.h"

const char* TimeWidget::NTP_SERVER = "pool.ntp.org";

// Backup NTP servers
const char* ntpServers[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com",
    "0.pool.ntp.org"
};

// Persist NTP sync state across deep sleep — RTC slow memory survives deep sleep
RTC_DATA_ATTR static unsigned long rtcLastNTPSyncEpoch = 0;
RTC_DATA_ATTR static bool rtcHasEverSynced = false;

// NTP re-sync every 12 hours — ESP32 RTC drifts ~5-20s/hr, invisible at HH:MM
static const unsigned long NTP_RESYNC_INTERVAL_SEC = 12 * 3600;

TimeWidget::TimeWidget(Inkplate& display)
    : Widget(display), lastTimeUpdate(0), timeInitialized(false), timeUpdateInterval(DEFAULT_TIME_UPDATE_INTERVAL) {}

TimeWidget::TimeWidget(Inkplate& display, unsigned long updateInterval)
    : Widget(display), lastTimeUpdate(0), timeInitialized(false), timeUpdateInterval(updateInterval) {
    LOG_INFO("TimeWidget", "Created with update interval: %lu ms (%lu seconds)", updateInterval, updateInterval / 1000);
}

void TimeWidget::begin() {
    LOG_INFO("TimeWidget", "Initializing time widget...");

    // The ESP32 RTC keeps time across deep sleep once configTime() has been
    // called at least once. Check if time(nullptr) returns a sane epoch —
    // if so, we can render the clock without any WiFi/NTP at all.
    time_t now = time(nullptr);
    if (rtcHasEverSynced && now > 1577836800) { // After Jan 1 2020
        unsigned long elapsed = (now > (time_t)rtcLastNTPSyncEpoch) ? (now - rtcLastNTPSyncEpoch) : 0;
        LOG_INFO("TimeWidget", "RTC time valid (epoch %lu, synced %lu sec ago)", (unsigned long)now, elapsed);
        timeInitialized = true;
        lastTimeUpdate = 0;
        return;
    }

    LOG_INFO("TimeWidget", "No valid RTC time — will need NTP sync");
    timeInitialized = false;
    lastTimeUpdate = 0;
}

bool TimeWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastTimeUpdate >= timeUpdateInterval) || (lastTimeUpdate == 0);
}

void TimeWidget::render(const LayoutRegion& region) {
    LOG_DEBUG("TimeWidget", "render() called - region: %dx%d at (%d,%d)",
              region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region before drawing to prevent text overwriting
    clearRegion(region);

    // Draw time content within the region (uses RTC time, no WiFi needed)
    drawTimeDisplay(region);

    lastTimeUpdate = millis();
}

void TimeWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    LOG_DEBUG("TimeWidget", "renderToCompositor() called - region: %dx%d at (%d,%d)",
              region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the region on compositor before drawing to prevent text overwriting
    clearRegionOnCompositor(compositor, region);

    // Draw time content to compositor (uses RTC time, no WiFi needed)
    drawTimeDisplayToCompositor(compositor, region);

    lastTimeUpdate = millis();
}

void TimeWidget::syncTimeWithNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN("TimeWidget", "WiFi not connected, cannot sync time");
        timeInitialized = false;
        return;
    }

    LOG_INFO("TimeWidget", "Syncing time with NTP server...");

    // Try 2 NTP servers with 3 attempts each (max ~6 seconds vs previous ~40 seconds)
    for (int serverIndex = 0; serverIndex < 2; serverIndex++) {
        LOG_DEBUG("TimeWidget", "Trying NTP server: %s", ntpServers[serverIndex]);

        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServers[serverIndex]);

        int attempts = 0;
        time_t now = 0;

        while (attempts < 3) {
            now = time(nullptr);

            if (now > 1577836800) { // January 1, 2020 00:00:00 UTC
                timeInitialized = true;
                rtcLastNTPSyncEpoch = (unsigned long)now;
                rtcHasEverSynced = true;
                LOG_INFO("TimeWidget", "Time synchronized successfully! Epoch saved to RTC memory.");

                struct tm* timeinfo = localtime(&now);
                LOG_DEBUG("TimeWidget", "Current time: %s", asctime(timeinfo));
                return;
            }

            delay(1000);
            attempts++;
        }

        LOG_WARN("TimeWidget", "Server %s failed, trying next...", ntpServers[serverIndex]);
    }

    LOG_ERROR("TimeWidget", "All NTP servers failed - time sync unsuccessful");
    timeInitialized = false;
}

void TimeWidget::drawTimeDisplay(const LayoutRegion& region) {
    LOG_DEBUG("TimeWidget", "drawTimeDisplay() - Drawing normal time display");
    LOG_DEBUG("TimeWidget", "region bounds: (%d,%d) %dx%d",
              region.getX(), region.getY(), region.getWidth(), region.getHeight());

    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    LOG_DEBUG("TimeWidget", "drawing at labelX=%d, labelY=%d", labelX, labelY);

    // Draw "DATE TIME" label
    display.setCursor(labelX, labelY + 20);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print("DATE TIME");
    LOG_DEBUG("TimeWidget", "Drew DATE TIME label");

    if (!timeInitialized) {
        display.setCursor(labelX, labelY + 60);
        display.setTextSize(2);
        display.setTextColor(0); // Black text
        display.setTextWrap(false);
        display.print("SYNC FAIL");
        LOG_WARN("TimeWidget", "Drew SYNC FAIL message");
        return;
    }

    // Get formatted time string
    String timeStr = getFormattedTime();
    LOG_DEBUG("TimeWidget", "Time string: %s", timeStr.c_str());

    // Draw time
    display.setCursor(labelX, labelY + 60);
    display.setTextSize(3);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(timeStr);
    LOG_DEBUG("TimeWidget", "Drew time string: %s", timeStr.c_str());

    // Draw date (larger font)
    String dateStr = getFormattedDate();
    display.setCursor(labelX, labelY + 110);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(dateStr);
    LOG_DEBUG("TimeWidget", "Drew date string: %s", dateStr.c_str());

    // Draw day of week (larger font)
    String dayStr = getDayOfWeek();
    display.setCursor(labelX, labelY + 140);
    display.setTextSize(2);
    display.setTextColor(0); // Black text
    display.setTextWrap(false);
    display.print(dayStr);
    LOG_DEBUG("TimeWidget", "Drew day string: %s", dayStr.c_str());
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
    LOG_INFO("TimeWidget", "Forcing time synchronization...");
    timeInitialized = false;
    syncTimeWithNTP();
}

bool TimeWidget::isTimeInitialized() const {
    return timeInitialized;
}

void TimeWidget::forceUpdate() {
    LOG_INFO("TimeWidget", "Force updating time widget...");
    lastTimeUpdate = 0; // Force next shouldUpdate() to return true
}

void TimeWidget::fetchData() {
    // Only sync NTP if we've never synced or drift correction is due (every 12h)
    if (!rtcHasEverSynced) {
        LOG_INFO("TimeWidget", "First boot — NTP sync required");
        syncTimeWithNTP();
    } else {
        time_t now = time(nullptr);
        unsigned long elapsed = (now > (time_t)rtcLastNTPSyncEpoch) ? (now - rtcLastNTPSyncEpoch) : 0;
        if (elapsed >= NTP_RESYNC_INTERVAL_SEC) {
            LOG_INFO("TimeWidget", "NTP drift correction due (%lu sec since last sync)", elapsed);
            syncTimeWithNTP();
        } else {
            LOG_INFO("TimeWidget", "Skipping NTP — RTC drift OK (%lu sec since sync)", elapsed);
        }
    }
}

bool TimeWidget::needsNetworkFetch() const {
    if (!rtcHasEverSynced) return true;
    time_t now = time(nullptr);
    unsigned long elapsed = (now > (time_t)rtcLastNTPSyncEpoch) ? (now - rtcLastNTPSyncEpoch) : 0;
    return elapsed >= NTP_RESYNC_INTERVAL_SEC;
}

void TimeWidget::drawTimeDisplayToCompositor(Compositor& compositor, const LayoutRegion& region) {
    LOG_DEBUG("TimeWidget", "drawTimeDisplayToCompositor() - Drawing time to compositor");

    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // For compositor rendering, we need to draw text as filled rectangles
    // This is a simplified implementation - in a full implementation you'd want
    // to render actual text to the compositor surface

    // Draw "DATE TIME" label area (simplified as a rectangle)
    compositor.fillRect(labelX, labelY + 15, 120, 20, 0); // Black rectangle for label
    LOG_DEBUG("TimeWidget", "Drew DATE TIME label area to compositor");

    if (!timeInitialized) {
        // Draw "SYNC FAIL" area
        compositor.fillRect(labelX, labelY + 55, 100, 20, 0); // Black rectangle
        LOG_WARN("TimeWidget", "Drew SYNC FAIL area to compositor");
        return;
    }

    // Draw time area (larger rectangle)
    compositor.fillRect(labelX, labelY + 50, 180, 30, 0); // Black rectangle for time
    LOG_DEBUG("TimeWidget", "Drew time area to compositor");

    // Draw date area
    compositor.fillRect(labelX, labelY + 100, 200, 20, 0); // Black rectangle for date
    LOG_DEBUG("TimeWidget", "Drew date area to compositor");

    // Draw day area
    compositor.fillRect(labelX, labelY + 130, 150, 20, 0); // Black rectangle for day
    LOG_DEBUG("TimeWidget", "Drew day area to compositor");

    // Note: This is a simplified implementation. For proper text rendering to compositor,
    // you would need to implement a text rendering system that draws to the compositor
    // surface pixel by pixel, or use a graphics library that can render to a buffer.
}
WidgetType TimeWidget::getWidgetType() const {
    return WidgetTypeTraits<TimeWidget>::type();
}
