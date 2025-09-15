#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Inkplate.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct WeatherData {
    String description;
    float temperature;
    int humidity;
    String icon;
    bool isValid;
};

class WeatherManager {
public:
    WeatherManager(Inkplate &display);

    void begin();
    void updateWeatherDisplay();
    void forceUpdate();
    void drawWeatherToBuffer(); // Draw without updating display
    bool shouldUpdate();
    void fetchWeatherData();
    bool isWeatherDataValid() const;

private:
    Inkplate &display;
    unsigned long lastWeatherUpdate;
    WeatherData currentWeather;

    static const unsigned long WEATHER_UPDATE_INTERVAL = 1800000; // 30 minutes in milliseconds
    static const char* WEATHER_API_URL;

    void drawWeatherDisplay();
    void getWeatherArea(int &x, int &y, int &width, int &height);
    void clearWeatherArea();
    String buildWeatherURL();
    void parseWeatherResponse(String response);
    String getWeatherIcon(String iconCode);
    String getWeatherDescription(int weatherCode);
};

#endif
