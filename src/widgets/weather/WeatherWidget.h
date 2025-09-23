#ifndef WEATHER_WIDGET_H
#define WEATHER_WIDGET_H

#include "../../core/Widget.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Forward declaration
class Compositor;

struct WeatherData {
    float temperature;
    int humidity;
    String description;
    String icon;
    int precipitationProbability;
    bool isValid;
};

class WeatherWidget : public Widget {
public:
    WeatherWidget(Inkplate& display, const String& latitude, const String& longitude,
                  const String& city, const String& units);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    void renderToCompositor(Compositor& compositor, const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;
    WidgetType getWidgetType() const override;

    // Weather-specific methods
    void fetchWeatherData();
    bool isWeatherDataValid() const;

private:
    unsigned long lastWeatherUpdate;
    WeatherData currentWeather;

    // Configuration
    String weatherLatitude;
    String weatherLongitude;
    String weatherCity;
    String weatherUnits;

    static const unsigned long WEATHER_UPDATE_INTERVAL = 1800000; // 30 minutes
    static const char* WEATHER_API_URL;

    void drawWeatherDisplay(const LayoutRegion& region);
    void drawWeatherDisplayToCompositor(Compositor& compositor, const LayoutRegion& region);
    String buildWeatherURL();
    void parseWeatherResponse(String response);
    const char* getWeatherDescription(int weatherCode);
};

#endif
