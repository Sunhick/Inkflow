#include "managers/LayoutManager.h"
#include "managers/PowerManager.h"
#include "core/Logger.h"

LayoutManager layoutManager;

void handleWakeButton();

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize logger
    Logger::setLogLevel(LogLevel::INFO);

    LOG_INFO("Main", "=== INKPLATE IMAGE DISPLAY STARTING ===");

    // Initialize WAKE button pins
    // pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
    // pinMode(34, INPUT_PULLUP);
    // pinMode(39, INPUT_PULLUP);

    LOG_INFO("Main", "Button pins initialized: 36, 34, 39");

    layoutManager.begin();

    // Demonstrate compositor integration
    LOG_INFO("Main", "Demonstrating compositor integration...");
    layoutManager.demonstrateCompositorIntegration();

    // Force an immediate refresh to load the image
    LOG_INFO("Main", "Waiting for WiFi connection...");
    delay(5000); // Give more time for WiFi to connect
    LOG_INFO("Main", "Forcing immediate refresh...");
    layoutManager.forceRefresh();
    LOG_INFO("Main", "Setup complete");
}

void loop() {
    layoutManager.loop();

    // Track activity for deep sleep
    static unsigned long lastActivity = millis();
    static unsigned long cachedShortestInterval = 0;
    static unsigned long lastConfigCheck = 0;

    // Check if deep sleep is enabled and we should enter it
    if (layoutManager.shouldEnterDeepSleep()) {
        // Cache configuration values to avoid repeated calls
        unsigned long currentTime = millis();
        if (cachedShortestInterval == 0 || currentTime - lastConfigCheck > 60000) { // Refresh cache every minute
            cachedShortestInterval = layoutManager.getShortestUpdateInterval();
            lastConfigCheck = currentTime;
        }

        unsigned long sleepThreshold = layoutManager.getDeepSleepThreshold();

        // If no updates needed for the threshold time, enter deep sleep
        if (currentTime - lastActivity > sleepThreshold) {
            LOG_INFO("Main", "Entering deep sleep mode...");
            LOG_INFO("Main", "Sleep threshold: %lu ms, shortest update interval: %lu ms", sleepThreshold, cachedShortestInterval);

            // Setup wake sources using config values
            int wakeButtonPin = layoutManager.getWakeButtonPin();
            PowerManager::enableWakeOnButton(wakeButtonPin);
            PowerManager::enableWakeOnTimer(cachedShortestInterval); // Wake based on shortest update interval

            // Enter deep sleep
            PowerManager::enterDeepSleep();
        }
    }

    // Debug: Print status every 2 minutes (reduced spam)
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 120000) { // 2 minutes instead of 30 seconds
        LOG_INFO("Main", "=== STATUS CHECK ===");
        LOG_INFO("Main", "WiFi Status: %s", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        if (WiFi.status() == WL_CONNECTED) {
            LOG_INFO("Main", "IP Address: %s", WiFi.localIP().toString().c_str());
            LOG_INFO("Main", "Signal Strength: %d dBm", WiFi.RSSI());
        }
        LOG_INFO("Main", "Free heap: %d bytes", ESP.getFreeHeap());
        LOG_INFO("Main", "Uptime: %lu seconds", millis() / 1000);
        lastStatusPrint = millis();
        lastActivity = millis(); // Reset activity timer
    }
}

void handleWakeButton() {
    // Test multiple button pins
    bool button36 = digitalRead(36);
    bool button34 = digitalRead(34);
    bool button39 = digitalRead(39);

    // Debug: Print button states periodically
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 5000) { // Every 5 seconds
        LOG_DEBUG("Main", "Button states - Pin 36: %d, Pin 34: %d, Pin 39: %d", button36, button34, button39);
        lastDebugPrint = millis();
    }

    // Check any button press (LOW because of INPUT_PULLUP)
    static bool lastButton36 = HIGH, lastButton34 = HIGH, lastButton39 = HIGH;

    if ((button36 == LOW && lastButton36 == HIGH) ||
        (button34 == LOW && lastButton34 == HIGH) ||
        (button39 == LOW && lastButton39 == HIGH)) {

        LOG_INFO("Main", "Button pressed - refreshing layout");
        layoutManager.forceRefresh();
        delay(500); // Prevent multiple triggers
    }

    lastButton36 = button36;
    lastButton34 = button34;
    lastButton39 = button39;
}
