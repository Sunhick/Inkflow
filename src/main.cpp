#include "managers/LayoutManager.h"
#include "config/Config.h"

LayoutManager layoutManager(WIFI_SSID, WIFI_PASSWORD, SERVER_URL, REFRESH_MS);

void handleWakeButton();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== INKPLATE IMAGE DISPLAY STARTING ===");
    Serial.printf("Server URL: %s\n", SERVER_URL);
    Serial.printf("WiFi SSID: %s\n", WIFI_SSID);
    Serial.printf("Refresh interval: %lu ms\n", REFRESH_MS);

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
    // handleWakeButton();

    // Debug: Print status every 30 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 30000) {
        Serial.println("=== STATUS CHECK ===");
        Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        }
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        lastStatusPrint = millis();
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
