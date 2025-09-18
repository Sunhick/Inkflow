#include "managers/LayoutManager.h"
#include "managers/PowerManager.h"

LayoutManager layoutManager;

void handleWakeButton();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== INKPLATE IMAGE DISPLAY STARTING ===");

    // Initialize WAKE button pins
    // pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
    // pinMode(34, INPUT_PULLUP);
    // pinMode(39, INPUT_PULLUP);

    Serial.println("Button pins initialized: 36, 34, 39");

    layoutManager.begin();

    // Force an immediate refresh to load the image
    Serial.println("Waiting for WiFi connection...");
    delay(5000); // Give more time for WiFi to connect
    Serial.println("Forcing immediate refresh...");
    layoutManager.forceRefresh();
    Serial.println("Setup complete");
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
            Serial.println("Entering deep sleep mode...");
            Serial.printf("Sleep threshold: %lu ms, shortest update interval: %lu ms\n", sleepThreshold, cachedShortestInterval);

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
        Serial.println("=== STATUS CHECK ===");
        Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        }
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
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
        Serial.printf("Button states - Pin 36: %d, Pin 34: %d, Pin 39: %d\n", button36, button34, button39);
        lastDebugPrint = millis();
    }

    // Check any button press (LOW because of INPUT_PULLUP)
    static bool lastButton36 = HIGH, lastButton34 = HIGH, lastButton39 = HIGH;

    if ((button36 == LOW && lastButton36 == HIGH) ||
        (button34 == LOW && lastButton34 == HIGH) ||
        (button39 == LOW && lastButton39 == HIGH)) {

        Serial.println("Button pressed - refreshing layout");
        layoutManager.forceRefresh();
        delay(500); // Prevent multiple triggers
    }

    lastButton36 = button36;
    lastButton34 = button34;
    lastButton39 = button39;
}
