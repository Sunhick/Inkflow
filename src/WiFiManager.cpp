#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password)
    : ssid(ssid), password(password), lastConnectionCheck(0) {}

bool WiFiManager::connect() {
    if (isConnected()) {
        return true;
    }

    return attemptConnection();
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::checkConnection() {
    unsigned long currentTime = millis();

    if (currentTime - lastConnectionCheck >= CHECK_INTERVAL) {
        if (!isConnected()) {
            Serial.println("WiFi connection lost, attempting reconnection...");
            connect();
        } else {
            monitorSignalStrength();
        }
        lastConnectionCheck = currentTime;
    }
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

int WiFiManager::getSignalStrength() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiManager::getStatusString() {
    switch (WiFi.status()) {
        case WL_CONNECTED: return "Connected";
        case WL_NO_SSID_AVAIL: return "Network not found";
        case WL_CONNECT_FAILED: return "Connection failed";
        case WL_CONNECTION_LOST: return "Connection lost";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown";
    }
}

bool WiFiManager::attemptConnection() {
    logConnectionAttempt();
    WiFi.begin(ssid, password);
    return waitForConnection();
}

void WiFiManager::logConnectionAttempt() {
    Serial.printf("Connecting to WiFi: %s\n", ssid);
}

bool WiFiManager::waitForConnection() {
    int attempts = 0;

    while (!isConnected() && attempts < MAX_RETRIES) {
        delay(500);
        Serial.print(".");
        attempts++;

        if (attempts % 10 == 0) {
            Serial.printf("\nAttempt %d/%d\n", attempts, MAX_RETRIES);
        }
    }

    if (isConnected()) {
        logSuccessfulConnection();
        return true;
    } else {
        Serial.println("\nWiFi connection failed");
        return false;
    }
}

void WiFiManager::logSuccessfulConnection() {
    Serial.printf("\nWiFi connected! IP: %s\n", getIPAddress().c_str());
    Serial.printf("Signal strength: %d dBm\n", getSignalStrength());
}

void WiFiManager::monitorSignalStrength() {
    int rssi = getSignalStrength();
    if (rssi < -80) {
        Serial.printf("Warning: Weak WiFi signal (%d dBm)\n", rssi);
    }
}
