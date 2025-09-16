#ifndef WEATHER_WIDGET_H
#define WEATHER_WIDGET_H

#include "../core/Widget.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
    WeatherWidget(Inkplate& display);

    // Widget interface implementation
    void render(const LayoutRegion& region) override;
    bool shouldUpdate() override;
    void begin() override;

    // Weather-specific methods
    void fetchWeatherData();
    bool isWeatherDataValid() const;

private:
    unsigned long lastWeatherUpdate;
    WeatherData currentWeather;

    static const unsigned long WEATHER_UPDATE_INTERVAL = 1800000; // 30 minutes
    static const char* WEATHER_API_URL;

    void drawWeatherDisplay(const LayoutRegion& region);
    String buildWeatherURL();
    void parseWeatherResponse(String response);
    String getWeatherDescription(int weatherCode);
};

#endif
