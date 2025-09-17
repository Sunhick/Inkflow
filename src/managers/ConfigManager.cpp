#include "ConfigManager.h"
#include <FS.h>
#include <SPIFFS.h>

ConfigManager::ConfigManager() {
    setDefaults();
}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }

    Serial.println("SPIFFS mounted successfully");
    return loadConfig();
}

bool ConfigManager::loadConfig() {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Config file not found, using defaults");
        return saveConfig(); // Create default config file
    }

    fs::File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse config file: %s\n", error.c_str());
        return false;
    }

    // Load configuration values
    config.wifiSSID = doc["wifi"]["ssid"] | "YOUR_WIFI_SSID";
    config.wifiPassword = doc["wifi"]["password"] | "YOUR_WIFI_PASSWORD";
    config.serverURL = doc["server"]["url"] | "http://example.com/image.jpg";

    config.weatherLatitude = doc["weather"]["latitude"] | "47.6062";
    config.weatherLongitude = doc["weather"]["longitude"] | "-122.3321";
    config.weatherCity = doc["weather"]["city"] | "Seattle";
    config.weatherUnits = doc["weather"]["units"] | "fahrenheit";

    config.refreshMs = doc["update"]["refreshMs"] | 86400000UL;

    config.displayWidth = doc["display"]["width"] | 1200;
    config.sidebarWidthPct = doc["display"]["sidebarWidthPct"] | 20;

    config.wakeButtonPin = doc["hardware"]["wakeButtonPin"] | 36;

    Serial.println("Configuration loaded successfully");
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID.c_str());
    Serial.printf("Server URL: %s\n", config.serverURL.c_str());
    Serial.printf("Weather City: %s\n", config.weatherCity.c_str());

    return true;
}

bool ConfigManager::saveConfig() {
    JsonDocument doc;

    // WiFi configuration
    doc["wifi"]["ssid"] = config.wifiSSID;
    doc["wifi"]["password"] = config.wifiPassword;

    // Server configuration
    doc["server"]["url"] = config.serverURL;

    // Weather configuration
    doc["weather"]["latitude"] = config.weatherLatitude;
    doc["weather"]["longitude"] = config.weatherLongitude;
    doc["weather"]["city"] = config.weatherCity;
    doc["weather"]["units"] = config.weatherUnits;

    // Update configuration
    doc["update"]["refreshMs"] = config.refreshMs;

    // Display configuration
    doc["display"]["width"] = config.displayWidth;
    doc["display"]["sidebarWidthPct"] = config.sidebarWidthPct;

    // Hardware configuration
    doc["hardware"]["wakeButtonPin"] = config.wakeButtonPin;

    fs::File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Failed to create config file");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write config file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Configuration saved successfully");
    return true;
}

void ConfigManager::setDefaults() {
    config.wifiSSID = "YOUR_WIFI_SSID";
    config.wifiPassword = "YOUR_WIFI_PASSWORD";
    config.serverURL = "http://example.com/image.jpg";

    config.weatherLatitude = "47.6062";  // Seattle
    config.weatherLongitude = "-122.3321";
    config.weatherCity = "Seattle";
    config.weatherUnits = "fahrenheit";

    config.refreshMs = 86400000UL; // 24 hours

    config.displayWidth = 1200;
    config.sidebarWidthPct = 20;

    config.wakeButtonPin = 36;
}
