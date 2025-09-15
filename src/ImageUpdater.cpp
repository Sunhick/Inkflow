#include "ImageUpdater.h"

ImageUpdater::ImageUpdater(Inkplate &display, const char* ssid, const char* password,
                           const char* imageUrl, unsigned long refreshMs)
    : wifiManager(ssid, password),
      displayManager(display),
      imageFetcher(display, imageUrl),
      batteryManager(display),
      timeManager(display),
      refreshInterval(refreshMs),
      lastUpdate(0) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate Image Updater...");

    displayManager.initialize();
    batteryManager.begin();
    timeManager.begin();

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
    handleTimeUpdate();
}

void ImageUpdater::performInitialSetup() {
    displayManager.showStatus("Initializing...");

    if (wifiManager.connect()) {
        Serial.println("Initial setup complete");
        displayManager.showStatus("Connected", "WiFi", wifiManager.getIPAddress().c_str());

        // Now that WiFi is connected, sync time
        Serial.println("WiFi connected, syncing time...");
        Serial.printf("WiFi IP: %s\n", wifiManager.getIPAddress().c_str());
        Serial.printf("WiFi Signal: %d dBm\n", wifiManager.getSignalStrength());

        timeManager.syncTimeWithNTP();

        if (timeManager.isTimeInitialized()) {
            Serial.println("Time sync successful!");
        } else {
            Serial.println("Time sync failed - will retry later");
        }

        if (imageFetcher.fetchAndDisplay()) {
            Serial.println("Initial image loaded successfully");
            // Show battery and time status after image
            Serial.println("Adding battery and time display to image...");
            batteryManager.drawBatteryToBuffer();
            timeManager.drawTimeToBuffer();
            displayManager.update(); // Single display update
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
        } else {
            // WiFi reconnected, resync time if needed
            Serial.println("WiFi reconnected, resyncing time...");
            timeManager.syncTimeWithNTP();
        }
    }
    return true;
}

void ImageUpdater::processImageUpdate() {
    displayManager.showStatus("Loading image...");

    if (imageFetcher.fetchAndDisplay()) {
        Serial.println("Image update completed successfully");
        // Always show battery and time after successful image load
        Serial.println("Adding battery and time display to updated image...");
        batteryManager.drawBatteryToBuffer();
        timeManager.drawTimeToBuffer();
        displayManager.update(); // Single display update
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
    // Check if battery needs update (every 30 minutes)
    if (batteryManager.shouldUpdate()) {
        Serial.println("Updating battery display...");
        batteryManager.drawBatteryToBuffer();
        timeManager.drawTimeToBuffer(); // Also refresh time
        displayManager.update();
    }
}

void ImageUpdater::handleTimeUpdate() {
    // Check if time needs update (every 30 minutes)
    if (timeManager.shouldUpdate()) {
        Serial.println("Updating time display...");
        timeManager.drawTimeToBuffer();
        batteryManager.drawBatteryToBuffer(); // Also refresh battery
        displayManager.update();
    }
}
