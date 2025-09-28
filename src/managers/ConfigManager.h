#pragma once

#include <WiFi.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>

// Widget type enumeration for type-safe widget creation
enum class WidgetType {
    WEATHER,
    NAME,
    DATE_TIME,
    BATTERY,
    IMAGE,
    LAYOUT,
    UNKNOWN
};

// Forward declarations for widgets
class WeatherWidget;
class NameWidget;
class TimeWidget;
class BatteryWidget;
class ImageWidget;
class LayoutWidget;

// Macro to automatically generate widget type traits (like nameof)
#define DECLARE_WIDGET_TYPE(WidgetClass, TypeName, EnumValue) \
template<> \
struct WidgetTypeTraits<WidgetClass> { \
    static constexpr const char* name() { return TypeName; } \
    static constexpr WidgetType type() { return EnumValue; } \
};

// Even more automatic macro that uses the class name directly (closest to nameof)
#define REGISTER_WIDGET(WidgetClass, EnumValue) \
    DECLARE_WIDGET_TYPE(WidgetClass, #WidgetClass, EnumValue)

// Template-based widget type mapping using C++ type information
template<typename T>
struct WidgetTypeTraits {
    static constexpr const char* name() { return "unknown"; }
    static constexpr WidgetType type() { return WidgetType::UNKNOWN; }
};

// Widget type registration with correct config.json names
DECLARE_WIDGET_TYPE(WeatherWidget, "WeatherWidget", WidgetType::WEATHER)
DECLARE_WIDGET_TYPE(NameWidget, "NameWidget", WidgetType::NAME)
DECLARE_WIDGET_TYPE(TimeWidget, "TimeWidget", WidgetType::DATE_TIME)
DECLARE_WIDGET_TYPE(BatteryWidget, "BatteryWidget", WidgetType::BATTERY)
DECLARE_WIDGET_TYPE(ImageWidget, "ImageWidget", WidgetType::IMAGE)
DECLARE_WIDGET_TYPE(LayoutWidget, "LayoutWidget", WidgetType::LAYOUT)

// Widget type string mapping
class WidgetTypeRegistry {
public:
    static WidgetType fromString(const String& typeStr);
    static String toString(WidgetType type);

    // Template-based type name getter
    template<typename T>
    static String getTypeName() {
        return String(WidgetTypeTraits<T>::name());
    }

    template<typename T>
    static WidgetType getType() {
        return WidgetTypeTraits<T>::type();
    }

    // Note: typeid approach not available on ESP32 due to -fno-rtti
    // The template traits approach above is preferred for embedded systems
};

// Widget configuration structures
struct WeatherWidgetConfig {
    String region;
    String latitude;
    String longitude;
    String city;
    String units;
};

struct NameWidgetConfig {
    String region;
    String familyName;
};

struct DateTimeWidgetConfig {
    String region;
    unsigned long timeUpdateMs;
};

struct BatteryWidgetConfig {
    String region;
    unsigned long batteryUpdateMs;
};

struct ImageWidgetConfig {
    String region;
    unsigned long imageRefreshMs;
};

struct LayoutWidgetConfig {
    // No region field - LayoutWidget is global and not assigned to a specific region
    bool showRegionBorders;
    bool showSeparators;
    int borderColor;
    int separatorColor;
    int borderThickness;
    int separatorThickness;
};

// Region layout configuration
struct RegionConfig {
    int x;
    int y;
    int width;
    int height;
};

// Main application configuration
struct AppConfig {
    // WiFi Configuration
    String wifiSSID;
    String wifiPassword;

    // Server Configuration
    String serverURL;

    // Widget Configurations
    std::vector<WeatherWidgetConfig> weatherWidgets;
    std::vector<NameWidgetConfig> nameWidgets;
    std::vector<DateTimeWidgetConfig> dateTimeWidgets;
    std::vector<BatteryWidgetConfig> batteryWidgets;
    std::vector<ImageWidgetConfig> imageWidgets;
    std::vector<LayoutWidgetConfig> layoutWidgets;

    // Layout Configuration (region_id -> RegionConfig)
    std::map<String, RegionConfig> regions;

    // Display Configuration
    int displayWidth;
    int displayHeight;
    bool usePartialUpdates;

    // Hardware Configuration
    int wakeButtonPin;

    // Power Management Configuration
    bool enableDeepSleep;
    unsigned long deepSleepThresholdMs;

    // Debug Configuration
    bool showDebugOnScreen;
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

    // Widget access helpers
    const std::vector<WeatherWidgetConfig>& getWeatherWidgets() const { return config.weatherWidgets; }
    const std::vector<NameWidgetConfig>& getNameWidgets() const { return config.nameWidgets; }
    const std::vector<DateTimeWidgetConfig>& getDateTimeWidgets() const { return config.dateTimeWidgets; }
    const std::vector<BatteryWidgetConfig>& getBatteryWidgets() const { return config.batteryWidgets; }
    const std::vector<ImageWidgetConfig>& getImageWidgets() const { return config.imageWidgets; }
    const std::vector<LayoutWidgetConfig>& getLayoutWidgets() const { return config.layoutWidgets; }

    // Region access helpers
    const std::map<String, RegionConfig>& getRegions() const { return config.regions; }
    RegionConfig getRegionConfig(const String& regionId) const;

    // Configuration validation
    bool isConfigured() const;
    String getConfigurationError() const;

private:
    AppConfig config;
    const char* CONFIG_FILE = "/config.json";
    bool configFileExists;
    void setDefaults();
};
