#include "WiFiManager.h"
#include "../core/Logger.h"

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
            LOG_WARN("WiFiManager", "WiFi connection lost, attempting reconnection...");
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
    LOG_INFO("WiFiManager", "Connecting to WiFi: %s", ssid);
}

bool WiFiManager::waitForConnection() {
    int attempts = 0;

    while (!isConnected() && attempts < MAX_RETRIES) {
        delay(500);
        attempts++;

        if (attempts % 10 == 0) {
            LOG_DEBUG("WiFiManager", "Connection attempt %d/%d", attempts, MAX_RETRIES);
        } else if (attempts % 5 == 0) {
            LOG_DEBUG("WiFiManager", "Connecting... (%d/%d)", attempts, MAX_RETRIES);
        }
    }

    if (isConnected()) {
        logSuccessfulConnection();
        return true;
    } else {
        LOG_ERROR("WiFiManager", "WiFi connection failed after %d attempts", attempts);
        return false;
    }
}

void WiFiManager::logSuccessfulConnection() {
    LOG_INFO("WiFiManager", "WiFi connected! IP: %s", getIPAddress().c_str());
    LOG_INFO("WiFiManager", "Signal strength: %d dBm", getSignalStrength());
}

void WiFiManager::monitorSignalStrength() {
    int rssi = getSignalStrength();
    if (rssi < -80) {
        LOG_WARN("WiFiManager", "Weak WiFi signal: %d dBm", rssi);
    }
}
