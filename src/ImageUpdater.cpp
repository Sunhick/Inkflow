#include "ImageUpdater.h"

ImageUpdater::ImageUpdater(Inkplate &disp, const char* ssid, const char* password, const char* url, unsigned long refreshMs)
    : display(disp), ssid(ssid), password(password), imageUrl(url), refreshInterval(refreshMs),
      lastUpdate(0), lastWiFiCheck(0), consecutiveFailures(0), displayingError(false) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate Image Updater...");

    display.begin();
    display.setDisplayMode(INKPLATE_3BIT);

    showStatus("Initializing...");

    if (connectWiFi()) {
        Serial.println("Initial setup complete");
        if (fetchAndDisplayImage()) {
            consecutiveFailures = 0;
        }
    } else {
        showError("WiFi Error", "Failed to connect to network");
    }

    lastUpdate = millis();
    lastWiFiCheck = millis();
}

void ImageUpdater::loop() {
    unsigned long currentTime = millis();

    // Check WiFi connection periodically
    if (currentTime - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        checkWiFiConnection();
        lastWiFiCheck = currentTime;
    }

    // Update image at scheduled intervals
    if (currentTime - lastUpdate >= refreshInterval) {
        Serial.println("Starting scheduled image update...");

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            if (!connectWiFi()) {
                showError("Connection Lost", "WiFi reconnection failed");
                lastUpdate = currentTime; // Prevent rapid retries
                return;
            }
        }

        if (fetchAndDisplayImage()) {
            consecutiveFailures = 0;
            displayingError = false;
        } else {
            consecutiveFailures++;
            Serial.printf("Image update failed (attempt %d)\n", consecutiveFailures);

            if (consecutiveFailures >= MAX_IMAGE_RETRIES) {
                showError("Image Error", "Multiple fetch failures");
            }
        }

        lastUpdate = currentTime;
    }
}

bool ImageUpdater::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.printf("Connecting to WiFi: %s\n", ssid);
    showStatus("Connecting to WiFi...");

    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < MAX_WIFI_RETRIES) {
        delay(500);
        Serial.print(".");
        attempts++;

        // Show progress every 10 attempts
        if (attempts % 10 == 0) {
            Serial.printf("\nAttempt %d/%d\n", attempts, MAX_WIFI_RETRIES);
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        Serial.println("\nWiFi connection failed");
        return false;
    }
}

bool ImageUpdater::fetchAndDisplayImage() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No WiFi connection for image fetch");
        return false;
    }

    Serial.printf("Fetching image from: %s\n", imageUrl);
    showStatus("Loading image...");

    display.clearDisplay();

    bool success = display.drawJpegFromWeb(imageUrl, 0, 0, true, false);

    if (success) {
        Serial.println("Image loaded successfully");
        display.display();
        return true;
    } else {
        Serial.println("Failed to load image");

        // Don't immediately show error on display for first few failures
        if (consecutiveFailures < MAX_IMAGE_RETRIES - 1) {
            Serial.println("Will retry on next cycle");
            return false;
        }

        // Show detailed error after multiple failures
        display.clearDisplay();
        display.setCursor(10, 10);
        display.setTextSize(2);
        display.setTextColor(BLACK);
        display.print("Image Load Failed");

        display.setCursor(10, 40);
        display.setTextSize(1);
        display.print("URL: ");
        display.println(imageUrl);

        display.setCursor(10, 60);
        display.print("WiFi: ");
        display.print(WiFi.localIP());
        display.print(" (");
        display.print(WiFi.RSSI());
        display.print(" dBm)");

        display.setCursor(10, 80);
        display.printf("Failures: %d", consecutiveFailures);

        display.setCursor(10, 100);
        display.print("Next retry in ");
        display.print(refreshInterval / 1000);
        display.print(" seconds");

        display.display();
        return false;
    }
}

void ImageUpdater::showError(const char* title, const char* message) {
    Serial.printf("ERROR: %s - %s\n", title, message);

    display.clearDisplay();
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print(title);

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print(message);

    display.setCursor(10, 70);
    display.print("WiFi Status: ");
    switch (WiFi.status()) {
        case WL_CONNECTED:
            display.print("Connected");
            break;
        case WL_NO_SSID_AVAIL:
            display.print("Network not found");
            break;
        case WL_CONNECT_FAILED:
            display.print("Connection failed");
            break;
        case WL_CONNECTION_LOST:
            display.print("Connection lost");
            break;
        case WL_DISCONNECTED:
            display.print("Disconnected");
            break;
        default:
            display.print("Unknown");
    }

    display.setCursor(10, 90);
    display.print("Will retry automatically");

    display.display();
    displayingError = true;
}

void ImageUpdater::showStatus(const char* message) {
    Serial.println(message);

    display.clearDisplay();
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print("Inkplate Status");

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print(message);

    display.setCursor(10, 60);
    display.print("Network: ");
    display.print(ssid);

    if (WiFi.status() == WL_CONNECTED) {
        display.setCursor(10, 80);
        display.print("IP: ");
        display.print(WiFi.localIP());
    }

    display.display();
}

void ImageUpdater::checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, attempting reconnection...");
        connectWiFi();
    } else {
        // Optionally log signal strength periodically
        int rssi = WiFi.RSSI();
        if (rssi < -80) {
            Serial.printf("Warning: Weak WiFi signal (%d dBm)\n", rssi);
        }
    }
}
