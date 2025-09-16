#include <Inkplate.h>
#include "ImageUpdater.h"
#include "Config.h"

Inkplate display(INKPLATE_3BIT);
ImageUpdater updater(display, WIFI_SSID, WIFI_PASSWORD, SERVER_URL, REFRESH_MS);

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void handleWakeButton();

void setup() {
    delay(1000);

    // Initialize WAKE button
    pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

    // Also try other common button pins
    pinMode(34, INPUT_PULLUP);
    pinMode(39, INPUT_PULLUP);

    Serial.println("Button pins initialized: 36, 34, 39");

    updater.begin();
}

void loop() {
    updater.loop();
    handleWakeButton();
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

        Serial.println("Button pressed - refreshing image");
        updater.forceImageRefresh();
        delay(500); // Prevent multiple triggers
    }

    lastButton36 = button36;
    lastButton34 = button34;
    lastButton39 = button39;
}
