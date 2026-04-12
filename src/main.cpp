#include "managers/LayoutManager.h"
#include "managers/PowerManager.h"
#include "core/Logger.h"
#include <esp_sleep.h>
#include <WiFi.h>

LayoutManager layoutManager;

void handleWakeButton();

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize logger — ERROR only in production to save power (UART draws current)
    Logger::setLogLevel(LogLevel::ERROR);

    LOG_INFO("Main", "=== INKPLATE IMAGE DISPLAY STARTING ===");

    // Disable Bluetooth immediately — not used, saves ~50mA
    PowerManager::disableBluetooth();

    // Start at 80MHz — sufficient for init, rendering, and config loading
    setCpuFrequencyMhz(80);
    LOG_INFO("Main", "CPU frequency set to 80MHz (low power)");

    // Check wake reason to determine if this is a scheduled wake or button wake
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            LOG_INFO("Main", "Wakeup caused by button press");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            LOG_INFO("Main", "Wakeup caused by timer (scheduled update)");
            break;
        default:
            LOG_INFO("Main", "Initial boot or reset");
            break;
    }

    // Initialize WAKE button pins
    pinMode(36, INPUT_PULLUP);
    pinMode(34, INPUT_PULLUP);
    pinMode(39, INPUT_PULLUP);

    LOG_INFO("Main", "Button pins initialized: 36, 34, 39");

    // Initialize layout manager - this now does all the heavy lifting
    layoutManager.begin();

    // Demonstrate compositor integration (only on initial boot)
    if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
        LOG_INFO("Main", "Demonstrating compositor integration...");
        layoutManager.demonstrateCompositorIntegration();
    }

    // Force refresh on button wake or do scheduled update on timer wake
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        LOG_INFO("Main", "Button wake - forcing immediate refresh...");
        layoutManager.forceRefresh();
    } else {
        LOG_INFO("Main", "Performing scheduled update...");
        // The scheduled update is now handled in layoutManager.begin()
    }

    LOG_INFO("Main", "Setup complete - entering main loop");
}

void loop() {
    // Handle button presses for manual refresh
    handleWakeButton();

    // Let layout manager handle immediate updates and sleep preparation
    layoutManager.loop();

    // Enter deep sleep immediately — all work is already done in setup()
    // No reason to burn 30 seconds of active time doing nothing
    if (layoutManager.shouldEnterDeepSleep()) {
        unsigned long cachedUpdateInterval = layoutManager.getShortestUpdateInterval();

        LOG_INFO("Main", "Entering deep sleep mode...");
        LOG_INFO("Main", "Next wake in: %lu ms", cachedUpdateInterval);

        // Setup wake sources
        int wakeButtonPin = layoutManager.getWakeButtonPin();
        PowerManager::enableWakeOnButton(wakeButtonPin);
        PowerManager::enableWakeOnTimer(cachedUpdateInterval);

        // Enter deep sleep - execution will resume in setup() on wake
        PowerManager::enterDeepSleep();
    }

    // If deep sleep is disabled, use light sleep between loop iterations
    // instead of active delay() to save ~30mA
    esp_sleep_enable_timer_wakeup(1000000); // 1 second
    esp_light_sleep_start();
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
