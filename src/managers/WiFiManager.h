#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager(const char* ssid, const char* password);

    bool connect();
    bool isConnected();
    void checkConnection();
    String getIPAddress();
    int getSignalStrength();
    String getStatusString();

private:
    const char* ssid;
    const char* password;
    unsigned long lastConnectionCheck;

    static const int MAX_RETRIES = 30;
    static const unsigned long CHECK_INTERVAL = 30000; // 30 seconds

    bool attemptConnection();
    void logConnectionAttempt();
    bool waitForConnection();
    void logSuccessfulConnection();
    void monitorSignalStrength();
};

#endif
