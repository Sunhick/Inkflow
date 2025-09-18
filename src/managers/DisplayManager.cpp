#include "DisplayManager.h"

DisplayManager::DisplayManager(Inkplate &display) : display(display), preferredDisplayMode(INKPLATE_3BIT) {}

void DisplayManager::initialize() {
    display.begin();
    preferredDisplayMode = INKPLATE_3BIT;
    display.setDisplayMode(preferredDisplayMode); // 3-bit mode for better grayscale

    // Enable text smoothing and wrapping for better rendering
    display.setTextWrap(true);
    display.cp437(true); // Enable extended character set
}

void DisplayManager::showStatus(const char* message, const char* networkName, const char* ipAddress) {
    Serial.println(message);

    clear();
    setTitle("Inkplate Status");
    setMessage(message);

    if (networkName) {
        setSmallText(("Network: " + String(networkName)).c_str(), 10, 60);
    }

    if (ipAddress) {
        setSmallText(("IP: " + String(ipAddress)).c_str(), 10, 80);
    }

    update();
}

void DisplayManager::showError(const char* title, const char* message, const char* wifiStatus) {
    Serial.printf("ERROR: %s - %s\n", title, message);

    clear();
    setTitle(title);
    setMessage(message);

    if (wifiStatus) {
        setSmallText(("WiFi Status: " + String(wifiStatus)).c_str(), 10, 70);
    }

    setSmallText("Will retry automatically", 10, 90);
    update();
}

void DisplayManager::showImageError(const char* url, int failures, int retrySeconds, const char* ipAddress, int signalStrength) {
    clear();
    setTitle("Image Load Failed");

    setSmallText(("URL: " + String(url)).c_str(), 10, 40);
    setSmallText(("WiFi: " + String(ipAddress) + " (" + String(signalStrength) + " dBm)").c_str(), 10, 60);
    setSmallText(("Failures: " + String(failures)).c_str(), 10, 80);
    setSmallText(("Next retry in " + String(retrySeconds) + " seconds").c_str(), 10, 100);

    update();
}

void DisplayManager::clear() {
    display.clearDisplay();
}

void DisplayManager::update() {
    Serial.println("DisplayManager: Performing full display update...");
    display.display();
    Serial.println("DisplayManager: Display update complete");
}

void DisplayManager::partialUpdate() {
    Serial.println("DisplayManager: Performing partial display update...");

    // Switch to 1-bit mode for partial updates (required for Inkplate)
    if (display.getDisplayMode() != INKPLATE_1BIT) {
        Serial.println("DisplayManager: Switching to 1-bit mode for partial update");
        display.setDisplayMode(INKPLATE_1BIT);
    }

    display.partialUpdate();

    // Switch back to preferred mode
    if (preferredDisplayMode != INKPLATE_1BIT) {
        Serial.println("DisplayManager: Switching back to preferred display mode");
        display.setDisplayMode(preferredDisplayMode);
    }

    Serial.println("DisplayManager: Partial update complete");
}

void DisplayManager::smartPartialUpdate() {
    Serial.println("DisplayManager: Performing smart partial update...");

    // For small widget updates, use partial update with mode switching
    // This is optimized for time/battery widget updates

    // Store current mode
    int currentMode = display.getDisplayMode();

    // Switch to 1-bit for partial update
    if (currentMode != INKPLATE_1BIT) {
        display.setDisplayMode(INKPLATE_1BIT);
    }

    // Perform partial update
    display.partialUpdate();

    // Restore original mode
    if (currentMode != INKPLATE_1BIT) {
        display.setDisplayMode(currentMode);
    }

    Serial.println("DisplayManager: Smart partial update complete");
}

void DisplayManager::setTitle(const char* title) {
    display.setCursor(10, 10);
    setupSmoothText(2, 0);
    display.print(title);
}

void DisplayManager::setMessage(const char* message, int y) {
    display.setCursor(10, y);
    setupSmoothText(1, 0);
    display.print(message);
}

void DisplayManager::setSmallText(const char* text, int x, int y) {
    display.setCursor(x, y);
    setupSmoothText(1, 0);
    display.print(text);
}

void DisplayManager::setupSmoothText(int size, int color) {
    display.setTextSize(size);
    display.setTextColor(color);
    display.setTextWrap(true);
}
