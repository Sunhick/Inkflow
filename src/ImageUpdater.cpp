#include "ImageUpdater.h"

ImageUpdater::ImageUpdater(Inkplate &display, const char* ssid, const char* password,
                           const char* imageUrl, unsigned long refreshMs)
    : wifiManager(ssid, password),
      displayManager(display),
      imageFetcher(display, imageUrl),
      batteryManager(display),
      refreshInterval(refreshMs),
      lastUpdate(0) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate Image Updater...");

    displayManager.initialize();
    batteryManager.begin();

    // Test battery display immediately
    Serial.println("Testing battery display...");
    batteryManager.forceUpdate();
    delay(3000);  // Show battery for 3 seconds

    performInitialSetup();
}

void ImageUpdater::loop() {
    wifiManager.checkConnection();
    handleScheduledUpdate();
    handleBatteryUpdate();
}

void ImageUpdater::performInitialSetup() {
    displayManager.showStatus("Initializing...");

    if (wifiManager.connect()) {
        Serial.println("Initial setup complete");
        displayManager.showStatus("Connected", "WiFi", wifiManager.getIPAddress().c_str());

        if (imageFetcher.fetchAndDisplay()) {
            Serial.println("Initial image loaded successfully");
            // Show battery status after image
            Serial.println("Adding battery display to image...");
            batteryManager.forceUpdate();
        } else {
            displayManager.showError("Image Error", "Failed to load initial image");
        }
    } else {
        displayManager.showError("WiFi Error", "Failed to connect to network",
                                wifiManager.getStatusString().c_str());
    }

    lastUpdate = millis();
}

void ImageUpdater::handleScheduledUpdate() {
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= refreshInterval) {
        Serial.println("Starting scheduled image update...");

        if (ensureConnectivity()) {
            processImageUpdate();
        }

        lastUpdate = currentTime;
    }
}

bool ImageUpdater::ensureConnectivity() {
    if (!wifiManager.isConnected()) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        displayManager.showStatus("Reconnecting WiFi...");

        if (!wifiManager.connect()) {
            displayManager.showError("Connection Lost", "WiFi reconnection failed",
                                   wifiManager.getStatusString().c_str());
            return false;
        }
    }
    return true;
}

void ImageUpdater::processImageUpdate() {
    displayManager.showStatus("Loading image...");

    if (imageFetcher.fetchAndDisplay()) {
        Serial.println("Image update completed successfully");
        // Always show battery after successful image load
        Serial.println("Adding battery display to updated image...");
        batteryManager.forceUpdate();
    } else {
        Serial.printf("Image update failed (attempt %d)\n", imageFetcher.getConsecutiveFailures());

        if (imageFetcher.getConsecutiveFailures() >= 3) {
            displayManager.showImageError(
                "Image URL",
                imageFetcher.getConsecutiveFailures(),
                refreshInterval / 1000,
                wifiManager.getIPAddress().c_str(),
                wifiManager.getSignalStrength()
            );
        }
    }
}

void ImageUpdater::handleBatteryUpdate() {
    // Battery manager handles its own timing (every 30 minutes)
    batteryManager.updateBatteryDisplay();
}
