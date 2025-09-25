#include "ConfigManager.h"
#include "../core/Logger.h"
#include <FS.h>
#include <SPIFFS.h>

// WidgetTypeRegistry implementation using template-based type traits
WidgetType WidgetTypeRegistry::fromString(const String& typeStr) {
    // Use template traits to map strings to types
    if (typeStr == WidgetTypeTraits<WeatherWidget>::name()) return WidgetTypeTraits<WeatherWidget>::type();
    if (typeStr == WidgetTypeTraits<NameWidget>::name()) return WidgetTypeTraits<NameWidget>::type();
    if (typeStr == WidgetTypeTraits<TimeWidget>::name()) return WidgetTypeTraits<TimeWidget>::type();
    if (typeStr == WidgetTypeTraits<BatteryWidget>::name()) return WidgetTypeTraits<BatteryWidget>::type();
    if (typeStr == WidgetTypeTraits<ImageWidget>::name()) return WidgetTypeTraits<ImageWidget>::type();
    if (typeStr == WidgetTypeTraits<LayoutWidget>::name()) return WidgetTypeTraits<LayoutWidget>::type();
    return WidgetType::UNKNOWN;
}

String WidgetTypeRegistry::toString(WidgetType type) {
    switch (type) {
        case WidgetType::WEATHER: return WidgetTypeTraits<WeatherWidget>::name();
        case WidgetType::NAME: return WidgetTypeTraits<NameWidget>::name();
        case WidgetType::DATE_TIME: return WidgetTypeTraits<TimeWidget>::name();
        case WidgetType::BATTERY: return WidgetTypeTraits<BatteryWidget>::name();
        case WidgetType::IMAGE: return WidgetTypeTraits<ImageWidget>::name();
        case WidgetType::LAYOUT: return WidgetTypeTraits<LayoutWidget>::name();
        default: return "unknown";
    }
}

ConfigManager::ConfigManager() : configFileExists(false) {
    setDefaults();
}

bool ConfigManager::begin() {
    if (!SPIFFS.begin(true)) {
        LOG_ERROR("ConfigManager", "Failed to mount SPIFFS");
        return false;
    }

    LOG_INFO("ConfigManager", "SPIFFS mounted successfully");

    return loadConfig();
}

bool ConfigManager::loadConfig() {
    LOG_DEBUG("ConfigManager", "Looking for config file: %s", CONFIG_FILE);

    configFileExists = SPIFFS.exists(CONFIG_FILE);

    if (!configFileExists) {
        LOG_WARN("ConfigManager", "Config file %s not found, using defaults", CONFIG_FILE);
        return saveConfig(); // Create default config file
    }

    fs::File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        LOG_ERROR("ConfigManager", "Failed to open config file: %s", CONFIG_FILE);
        return false;
    }

    LOG_DEBUG("ConfigManager", "Config file opened successfully, size: %d bytes", file.size());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        LOG_ERROR("ConfigManager", "Failed to parse config file: %s", error.c_str());
        return false;
    }

    // Load basic configuration
    config.wifiSSID = doc["Wifi"]["SSID"] | "YOUR_WIFI_SSID";
    config.wifiPassword = doc["Wifi"]["Password"] | "YOUR_WIFI_PASSWORD";
    config.serverURL = doc["Server"]["Url"] | "http://example.com/image.jpg";

    // Clear existing widget configurations
    config.weatherWidgets.clear();
    config.nameWidgets.clear();
    config.dateTimeWidgets.clear();
    config.batteryWidgets.clear();
    config.imageWidgets.clear();
    config.layoutWidgets.clear();

    // Parse widgets array
    JsonArray widgets = doc["Widgets"];
    for (JsonObject widget : widgets) {
        String typeStr = widget["type"] | "";
        WidgetType widgetType = WidgetTypeRegistry::fromString(typeStr);

        LOG_DEBUG("ConfigManager", "Parsing widget type: '%s' -> %d", typeStr.c_str(), (int)widgetType);

        switch (widgetType) {
            case WidgetType::WEATHER: {
                WeatherWidgetConfig weatherConfig;
                weatherConfig.region = widget["region"] | "";
                weatherConfig.latitude = widget["latitude"] | "47.6062";
                weatherConfig.longitude = widget["longitude"] | "-122.3321";
                weatherConfig.city = widget["city"] | "Seattle";
                weatherConfig.units = widget["units"] | "fahrenheit";
                config.weatherWidgets.push_back(weatherConfig);
                break;
            }

            case WidgetType::NAME: {
                NameWidgetConfig nameConfig;
                nameConfig.region = widget["region"] | "";
                nameConfig.familyName = widget["familyName"] | "Family";
                config.nameWidgets.push_back(nameConfig);
                break;
            }

            case WidgetType::DATE_TIME: {
                String regionStr = widget["region"] | "";
                LOG_DEBUG("ConfigManager", "Parsing TimeWidget for region: %s", regionStr.c_str());
                DateTimeWidgetConfig dateTimeConfig;
                dateTimeConfig.region = widget["region"] | "";
                dateTimeConfig.timeUpdateMs = widget["timeUpdateMs"] | 900000UL;
                config.dateTimeWidgets.push_back(dateTimeConfig);
                LOG_DEBUG("ConfigManager", "Added TimeWidget to config, total: %d", config.dateTimeWidgets.size());
                break;
            }

            case WidgetType::BATTERY: {
                String regionStr = widget["region"] | "";
                LOG_DEBUG("ConfigManager", "Parsing BatteryWidget for region: %s", regionStr.c_str());
                BatteryWidgetConfig batteryConfig;
                batteryConfig.region = widget["region"] | "";
                batteryConfig.batteryUpdateMs = widget["batteryUpdateMs"] | 900000UL;
                config.batteryWidgets.push_back(batteryConfig);
                LOG_DEBUG("ConfigManager", "Added BatteryWidget to config, total: %d", config.batteryWidgets.size());
                break;
            }

            case WidgetType::IMAGE: {
                ImageWidgetConfig imageConfig;
                imageConfig.region = widget["region"] | "";
                imageConfig.imageRefreshMs = widget["imageRefreshMs"] | 86400000UL;
                config.imageWidgets.push_back(imageConfig);
                break;
            }

            case WidgetType::LAYOUT: {
                LayoutWidgetConfig layoutConfig;
                // No region assignment - LayoutWidget is global
                layoutConfig.showRegionBorders = widget["showRegionBorders"] | false;
                layoutConfig.showSeparators = widget["showSeparators"] | false;
                layoutConfig.borderColor = widget["borderColor"] | 0;
                layoutConfig.separatorColor = widget["separatorColor"] | 0;
                layoutConfig.borderThickness = widget["borderThickness"] | 1;
                layoutConfig.separatorThickness = widget["separatorThickness"] | 1;
                config.layoutWidgets.push_back(layoutConfig);
                break;
            }

            case WidgetType::UNKNOWN:
            default:
                LOG_WARN("ConfigManager", "Unknown widget type: %s", typeStr.c_str());
                break;
        }
    }

    // Parse layout regions
    config.regions.clear();
    JsonObject layout = doc["Layout"];
    for (JsonPair regionPair : layout) {
        String regionId = regionPair.key().c_str();
        JsonObject regionObj = regionPair.value();

        RegionConfig regionConfig;
        regionConfig.x = regionObj["X"] | 0;
        regionConfig.y = regionObj["Y"] | 0;
        regionConfig.width = regionObj["Width"] | 300;
        regionConfig.height = regionObj["Height"] | 300;

        config.regions[regionId] = regionConfig;
    }

    // Display configuration
    config.displayWidth = doc["Display"]["Width"] | 1200;
    config.displayHeight = doc["Display"]["Height"] | 825;
    config.usePartialUpdates = doc["Display"]["UsePartialUpdates"] | false;

    // Hardware configuration
    config.wakeButtonPin = doc["Hardware"]["WakeButtonPin"] | 36;

    // Power management configuration
    config.enableDeepSleep = doc["Power"]["EnableDeepSleep"] | true;
    config.deepSleepThresholdMs = doc["Power"]["DeepSleepThresholdMs"] | 600000UL;

    LOG_INFO("ConfigManager", "Configuration loaded successfully");
    LOG_INFO("ConfigManager", "WiFi SSID: %s", config.wifiSSID.c_str());
    LOG_INFO("ConfigManager", "Server URL: %s", config.serverURL.c_str());
    LOG_INFO("ConfigManager", "Loaded %d weather, %d name, %d dateTime, %d battery, %d image widgets",
             config.weatherWidgets.size(), config.nameWidgets.size(), config.dateTimeWidgets.size(),
             config.batteryWidgets.size(), config.imageWidgets.size());
    LOG_INFO("ConfigManager", "Loaded %d regions", config.regions.size());

    return true;
}

bool ConfigManager::saveConfig() {
    JsonDocument doc;

    // WiFi configuration
    doc["Wifi"]["SSID"] = config.wifiSSID;
    doc["Wifi"]["Password"] = config.wifiPassword;

    // Server configuration
    doc["Server"]["Url"] = config.serverURL;

    // Widgets array
    JsonArray widgets = doc["Widgets"].to<JsonArray>();

    // Add weather widgets
    for (const auto& weather : config.weatherWidgets) {
        JsonObject widget = widgets.add<JsonObject>();
        widget["WeatherWidget"]["region"] = weather.region;
        widget["WeatherWidget"]["latitude"] = weather.latitude;
        widget["WeatherWidget"]["longitude"] = weather.longitude;
        widget["WeatherWidget"]["city"] = weather.city;
        widget["WeatherWidget"]["units"] = weather.units;
    }

    // Add name widgets
    for (const auto& name : config.nameWidgets) {
        JsonObject widget = widgets.add<JsonObject>();
        widget["NameWidget"]["region"] = name.region;
        widget["NameWidget"]["familyName"] = name.familyName;
    }

    // Add time widgets
    for (const auto& dateTime : config.dateTimeWidgets) {
        JsonObject widget = widgets.add<JsonObject>();
        widget["TimeWidget"]["region"] = dateTime.region;
        widget["TimeWidget"]["timeUpdateMs"] = dateTime.timeUpdateMs;
    }

    // Add battery widgets
    for (const auto& battery : config.batteryWidgets) {
        JsonObject widget = widgets.add<JsonObject>();
        widget["BatteryWidget"]["region"] = battery.region;
        widget["BatteryWidget"]["batteryUpdateMs"] = battery.batteryUpdateMs;
    }

    // Add image widgets
    for (const auto& image : config.imageWidgets) {
        JsonObject widget = widgets.add<JsonObject>();
        widget["ImageWidget"]["region"] = image.region;
        widget["ImageWidget"]["imageRefreshMs"] = image.imageRefreshMs;
    }

    // Layout configuration
    JsonObject layout = doc["Layout"].to<JsonObject>();
    for (const auto& regionPair : config.regions) {
        JsonObject regionObj = layout[regionPair.first].to<JsonObject>();
        regionObj["X"] = regionPair.second.x;
        regionObj["Y"] = regionPair.second.y;
        regionObj["Width"] = regionPair.second.width;
        regionObj["Height"] = regionPair.second.height;
    }

    // Display configuration
    doc["Display"]["Width"] = config.displayWidth;
    doc["Display"]["Height"] = config.displayHeight;
    doc["Display"]["UsePartialUpdates"] = config.usePartialUpdates;

    // Hardware configuration
    doc["Hardware"]["WakeButtonPin"] = config.wakeButtonPin;

    // Power management configuration
    doc["Power"]["EnableDeepSleep"] = config.enableDeepSleep;
    doc["Power"]["DeepSleepThresholdMs"] = config.deepSleepThresholdMs;

    fs::File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        LOG_ERROR("ConfigManager", "Failed to create config file");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        LOG_ERROR("ConfigManager", "Failed to write config file");
        file.close();
        return false;
    }

    file.close();
    LOG_INFO("ConfigManager", "Configuration saved successfully");
    return true;
}

void ConfigManager::setDefaults() {
    config.wifiSSID = "YOUR_WIFI_SSID";
    config.wifiPassword = "YOUR_WIFI_PASSWORD";
    config.serverURL = "http://example.com/image.jpg";

    // Clear widget collections
    config.weatherWidgets.clear();
    config.nameWidgets.clear();
    config.dateTimeWidgets.clear();
    config.batteryWidgets.clear();
    config.imageWidgets.clear();
    config.layoutWidgets.clear();
    config.regions.clear();

    config.displayWidth = 1200;
    config.displayHeight = 825;
    config.usePartialUpdates = false;

    config.wakeButtonPin = 36;

    config.enableDeepSleep = true;
    config.deepSleepThresholdMs = 600000UL; // 10 minutes
}
bool ConfigManager::isConfigured() const {
    // Check if config file existed when loaded
    if (!configFileExists) {
        return false;
    }

    // Check for default/unconfigured values
    if (config.wifiSSID == "YOUR_WIFI_SSID" ||
        config.wifiSSID == "DEFAULT_SSID" ||
        config.wifiSSID.isEmpty()) {
        return false;
    }

    if (config.wifiPassword == "YOUR_WIFI_PASSWORD" ||
        config.wifiPassword == "DEFAULT_PASSWORD" ||
        config.wifiPassword.isEmpty()) {
        return false;
    }

    if (config.serverURL == "http://example.com/image.jpg" ||
        config.serverURL.isEmpty()) {
        return false;
    }

    return true;
}

String ConfigManager::getConfigurationError() const {
    if (!configFileExists) {
        return "Configuration file missing. Please upload config.json to device.";
    }

    if (config.wifiSSID == "YOUR_WIFI_SSID" ||
        config.wifiSSID == "DEFAULT_SSID" ||
        config.wifiSSID.isEmpty()) {
        return "WiFi SSID not configured. Please update your configuration.";
    }

    if (config.wifiPassword == "YOUR_WIFI_PASSWORD" ||
        config.wifiPassword == "DEFAULT_PASSWORD" ||
        config.wifiPassword.isEmpty()) {
        return "WiFi password not configured. Please update your configuration.";
    }

    if (config.serverURL == "http://example.com/image.jpg" ||
        config.serverURL.isEmpty()) {
        return "Image server URL not configured. Please update your configuration.";
    }

    return "Configuration appears valid.";
}

RegionConfig ConfigManager::getRegionConfig(const String& regionId) const {
    auto it = config.regions.find(regionId);
    if (it != config.regions.end()) {
        return it->second;
    }

    // Return default region config if not found
    RegionConfig defaultConfig;
    defaultConfig.x = 0;
    defaultConfig.y = 0;
    defaultConfig.width = 300;
    defaultConfig.height = 300;
    return defaultConfig;
}
