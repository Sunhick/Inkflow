#include "ImageUpdater.h"

ImageUpdater::ImageUpdater(Inkplate &display, const char* ssid, const char* password,
                           const char* imageUrl, unsigned long refreshMs)
    : wifiManager(ssid, password),
      displayManager(display),
      imageFetcher(display, imageUrl),
      batteryManager(display),
      timeManager(display),
      weatherManager(display),
      refreshInterval(refreshMs),
      lastUpdate(0) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate Image Updater...");

    displayManager.initialize();
    batteryManager.begin();
    timeManager.begin();
    weatherManager.begin();

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
    handleWeatherUpdate();
}

void ImageUpdater::performInitialSetup() {
    displayManager.showStatus("Initializing...");

    if (wifiManager.connect()) {
        Serial.println("Initial setup complete");
        displayManager.showStatus("Connected", "WiFi", wifiManager.getIPAddress().c_str());

        // Now that WiFi is connected, sync time and weather
        Serial.println("WiFi connected, syncing time and weather...");
        Serial.printf("WiFi IP: %s\n", wifiManager.getIPAddress().c_str());
        Serial.printf("WiFi Signal: %d dBm\n", wifiManager.getSignalStrength());

        timeManager.syncTimeWithNTP();
        weatherManager.fetchWeatherData();

        if (timeManager.isTimeInitialized()) {
            Serial.println("Time sync successful!");
        } else {
            Serial.println("Time sync failed - will retry later");
        }

        if (weatherManager.isWeatherDataValid()) {
            Serial.println("Weather fetch successful!");
        } else {
            Serial.println("Weather fetch failed - will retry later");
        }

        if (imageFetcher.fetchAndDisplay()) {
            Serial.println("Initial image loaded successfully");
            // Show battery, time, and weather status after image
            Serial.println("Adding battery, time, and weather display to image...");
            updateAllSidebarComponents();
        } else {
            // Show error in photo area and draw sidebar
            imageFetcher.showErrorInPhotoArea("IMAGE ERROR", "Failed to load initial image");
            updateAllSidebarComponents();
        }
    } else {
        // Show error in photo area and draw sidebar
        imageFetcher.showErrorInPhotoArea("WIFI ERROR", "Failed to connect to network",
                                        wifiManager.getStatusString().c_str());
        updateAllSidebarComponents();
    }

    lastUpdate = millis();
}

void ImageUpdater::handleScheduledUpdate() {
    // Automatic image updates every 24 hours + manual refresh via WAKE button
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= refreshInterval) {
        Serial.println("Starting scheduled daily image update...");

        if (ensureConnectivity()) {
            // Force refresh of weather and time data during scheduled update
            timeManager.syncTimeWithNTP();
            weatherManager.fetchWeatherData();

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
            // Show error in photo area and draw sidebar
            imageFetcher.showErrorInPhotoArea("CONNECTION LOST", "WiFi reconnection failed",
                                            wifiManager.getStatusString().c_str());
            updateAllSidebarComponents();
            return false;
        } else {
            // WiFi reconnected, resync time and weather if needed
            Serial.println("WiFi reconnected, resyncing time and weather...");
            timeManager.syncTimeWithNTP();
            weatherManager.fetchWeatherData();
        }
    }
    return true;
}

void ImageUpdater::processImageUpdate(bool showLoadingStatus) {
    if (showLoadingStatus) {
        displayManager.showStatus("Loading image...");
    }

    if (imageFetcher.fetchAndDisplay()) {
        Serial.println("Image update completed successfully");
        // Always show battery and time after successful image load
        Serial.println("Adding battery, time, and weather display to updated image...");
        updateAllSidebarComponents();
    } else {
        Serial.printf("Image update failed (attempt %d)\n", imageFetcher.getConsecutiveFailures());

        if (imageFetcher.getConsecutiveFailures() >= 3) {
            // Show diagnostics in photo area instead of full screen error
            imageFetcher.showDiagnosticsInPhotoArea(
                wifiManager.getIPAddress().c_str(),
                wifiManager.getSignalStrength()
            );
            // Still draw the sidebar status
            updateAllSidebarComponents();
        }
    }
}

void ImageUpdater::handleBatteryUpdate() {
    // Automatic sidebar updates disabled - only manual refresh via WAKE button
    // Uncomment the code below to re-enable automatic sidebar updates

    /*
    // Check if battery needs update (every 30 minutes)
    if (batteryManager.shouldUpdate()) {
        updateAllSidebarComponents();
    }
    */
}

void ImageUpdater::handleTimeUpdate() {
    // Automatic sidebar updates disabled - only manual refresh via WAKE button
    // Uncomment the code below to re-enable automatic sidebar updates

    /*
    // Check if time needs update (every 30 minutes)
    if (timeManager.shouldUpdate()) {
        updateAllSidebarComponents();
    }
    */
}

void ImageUpdater::handleWeatherUpdate() {
    // Automatic sidebar updates disabled - only manual refresh via WAKE button
    // Uncomment the code below to re-enable automatic sidebar updates

    /*
    // Check if weather needs update (every 30 minutes)
    if (weatherManager.shouldUpdate()) {
        updateAllSidebarComponents();
    }
    */
}

void ImageUpdater::updateAllSidebarComponents() {
    Serial.println("Updating all sidebar components...");
    batteryManager.drawBatteryToBuffer();
    timeManager.drawTimeToBuffer();
    weatherManager.drawWeatherToBuffer();
    displayManager.update(); // Single display update for all components
}

void ImageUpdater::forceImageRefresh() {
    Serial.println("Manual image refresh triggered by WAKE button");

    if (ensureConnectivity()) {
        // Force refresh of weather and time data
        timeManager.syncTimeWithNTP();
        weatherManager.fetchWeatherData();

        // Process image update without showing loading status
        processImageUpdate(false);

        // Update the last update time to reset the scheduled timer
        lastUpdate = millis();
    } else {
        Serial.println("Cannot refresh image - no connectivity");
    }
}
