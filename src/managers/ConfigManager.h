#pragma once

#include <WiFi.h>
#include <ArduinoJson.h>

struct AppConfig {
    // WiFi Configuration
    String wifiSSID;
    String wifiPassword;
    String serverURL;

    // Weather Configuration
    String weatherLatitude;
    String weatherLongitude;
    String weatherCity;
    String weatherUnits;

    // Update Configuration
    unsigned long imageRefreshMs;
    unsigned long timeUpdateMs;
    unsigned long batteryUpdateMs;

    // Display Configuration
    int displayWidth;
    int sidebarWidthPct;
    String familyName;

    // Hardware Configuration
    int wakeButtonPin;
};

class ConfigManager {
public:
    ConfigManager();
    bool begin();
    bool loadConfig();
    bool saveConfig();
    const AppConfig& getConfig() const { return config; }
    void setConfig(const AppConfig& newConfig) { config = newConfig; }

    // Helper methods for easy access
    const char* getWiFiSSID() const { return config.wifiSSID.c_str(); }
    const char* getWiFiPassword() const { return config.wifiPassword.c_str(); }
    const char* getServerURL() const { return config.serverURL.c_str(); }
    unsigned long getImageRefreshMs() const { return config.imageRefreshMs; }
    unsigned long getTimeUpdateMs() const { return config.timeUpdateMs; }
    unsigned long getBatteryUpdateMs() const { return config.batteryUpdateMs; }
    const char* getFamilyName() const { return config.familyName.c_str(); }

    // Configuration validation
    bool isConfigured() const;
    String getConfigurationError() const;

private:
    AppConfig config;
    const char* CONFIG_FILE = "/config.json";
    bool configFileExists;
    void setDefaults();
};
