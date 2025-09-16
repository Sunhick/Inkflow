#include "WeatherManager.h"
#include "ImageFetcher.h"
#include "Config.h"

const char* WeatherManager::WEATHER_API_URL = "https://api.open-meteo.com/v1/forecast";

WeatherManager::WeatherManager(Inkplate &display)
    : display(display), lastWeatherUpdate(0) {
    currentWeather.isValid = false;
}

void WeatherManager::begin() {
    Serial.println("Initializing weather monitoring...");
    currentWeather.isValid = false;
    lastWeatherUpdate = 0; // Force initial update
}

void WeatherManager::updateWeatherDisplay() {
    if (!shouldUpdate()) {
        return;
    }
    forceUpdate();
}

void WeatherManager::forceUpdate() {
    Serial.println("Force updating weather display...");

    if (!currentWeather.isValid) {
        Serial.println("Weather data not available, fetching...");
        fetchWeatherData();
    }

    drawWeatherDisplay();
    Serial.println("Weather drawn, updating display...");
    display.display();
    lastWeatherUpdate = millis();
}

bool WeatherManager::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) || (lastWeatherUpdate == 0);
}

void WeatherManager::fetchWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot fetch weather");
        currentWeather.isValid = false;
        return;
    }

    Serial.println("Fetching weather data...");

    HTTPClient http;
    String url = buildWeatherURL();
    Serial.printf("Weather URL: %s\n", url.c_str());

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("Weather response: %s\n", response.c_str());
        parseWeatherResponse(response);
    } else {
        Serial.printf("Weather API error: %d\n", httpCode);
        currentWeather.isValid = false;
    }

    http.end();
}

String WeatherManager::buildWeatherURL() {
    String url = String(WEATHER_API_URL);
    url += "?latitude=" + String(WEATHER_LATITUDE);
    url += "&longitude=" + String(WEATHER_LONGITUDE);
    url += "&current_weather=true&temperature_unit=" + String(WEATHER_UNITS);
    url += "&hourly=precipitation_probability&forecast_days=1";
    return url;
}

void WeatherManager::parseWeatherResponse(String response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response.c_str());

    if (error) {
        Serial.printf("JSON parsing error: %s\n", error.c_str());
        currentWeather.isValid = false;
        return;
    }

    // Extract weather data from Open-Meteo API
    if (doc["current_weather"]["temperature"]) {
        currentWeather.temperature = doc["current_weather"]["temperature"];
        currentWeather.humidity = 0; // Open-Meteo doesn't provide humidity in basic call

        // Convert weather code to description
        int weatherCode = doc["current_weather"]["weathercode"];
        currentWeather.description = getWeatherDescription(weatherCode);
        currentWeather.icon = String(weatherCode);

        // Extract precipitation probability from hourly data (current hour)
        currentWeather.precipitationProbability = 0;
        if (doc["hourly"]["precipitation_probability"]) {
            JsonArray precipArray = doc["hourly"]["precipitation_probability"];
            if (precipArray.size() > 0) {
                currentWeather.precipitationProbability = precipArray[0]; // Current hour
            }
        }

        currentWeather.isValid = true;

        Serial.printf("Weather: %.1f°F, %s, %d%% rain (code: %d)\n",
                      currentWeather.temperature,
                      currentWeather.description.c_str(),
                      currentWeather.precipitationProbability,
                      weatherCode);
    } else {
        Serial.println("Failed to parse weather data");
        currentWeather.isValid = false;
    }
}

String WeatherManager::getWeatherDescription(int weatherCode) {
    // Convert Open-Meteo weather codes to detailed descriptions
    switch (weatherCode) {
        case 0: return "Clear Sky";
        case 1: return "Mainly Clear";
        case 2: return "Partly Cloudy";
        case 3: return "Overcast";
        case 45: return "Fog";
        case 48: return "Depositing Rime Fog";
        case 51: return "Light Drizzle";
        case 53: return "Moderate Drizzle";
        case 55: return "Dense Drizzle";
        case 56: return "Light Freezing Drizzle";
        case 57: return "Dense Freezing Drizzle";
        case 61: return "Slight Rain";
        case 63: return "Moderate Rain";
        case 65: return "Heavy Rain";
        case 66: return "Light Freezing Rain";
        case 67: return "Heavy Freezing Rain";
        case 71: return "Slight Snow";
        case 73: return "Moderate Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Slight Rain Showers";
        case 81: return "Moderate Rain Showers";
        case 82: return "Violent Rain Showers";
        case 85: return "Slight Snow Showers";
        case 86: return "Heavy Snow Showers";
        case 95: return "Thunderstorm";
        case 96: return "Thunderstorm with Hail";
        case 99: return "Heavy Thunderstorm with Hail";
        default: return "Unknown Weather";
    }
}

void WeatherManager::drawWeatherDisplay() {
    Serial.println("Drawing weather display...");

    int displayHeight = display.height();

    // Position weather in the middle third of the left sidebar
    int sidebarHeight = displayHeight / 3; // Each section gets 1/3 of height
    int weatherY = sidebarHeight + 10; // Middle third with margin
    int weatherX = 10; // Left margin

    Serial.printf("Weather position in sidebar: x=%d, y=%d, height=%d\n", weatherX, weatherY, sidebarHeight);

    // Clear the weather area in sidebar
    clearWeatherArea();

    // Draw "WEATHER" label
    display.setCursor(weatherX, weatherY);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("WEATHER");

    if (!currentWeather.isValid) {
        Serial.println("Weather data not valid, showing error");

        display.setCursor(weatherX, weatherY + 40);
        display.setTextSize(2);
        display.setTextColor(0);
        display.setTextWrap(true);
        display.print("N/A");
        return;
    }

    // Draw temperature (large) - fix degree symbol issue
    display.setCursor(weatherX, weatherY + 40);
    display.setTextSize(4);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print((int)currentWeather.temperature);
    display.print("F");

    // Draw weather description (full verbose description)
    display.setCursor(weatherX, weatherY + 90);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print(currentWeather.description);

    // Add precipitation probability
    display.setCursor(weatherX, weatherY + 120);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("Rain: ");
    display.print(currentWeather.precipitationProbability);
    display.print("%");

    // Draw separator line below weather section
    int sectionHeight = displayHeight / 3;
    int line2Y = (sectionHeight * 2) - 2;
    display.drawLine(5, line2Y, SIDEBAR_WIDTH - 5, line2Y, 0);
    display.drawLine(5, line2Y + 1, SIDEBAR_WIDTH - 5, line2Y + 1, 0);

    Serial.println("Weather drawn to sidebar buffer");
}

void WeatherManager::clearWeatherArea() {
    int areaX, areaY, areaWidth, areaHeight;
    getWeatherArea(areaX, areaY, areaWidth, areaHeight);

    // Clear with white background
    display.fillRect(areaX, areaY, areaWidth, areaHeight, 7);
}

void WeatherManager::getWeatherArea(int &x, int &y, int &width, int &height) {
    int displayHeight = display.height();

    // Weather area is middle third of left sidebar, but leave space for separator lines
    int sidebarHeight = displayHeight / 3;

    x = 0;
    y = sidebarHeight + 2; // Start after the top separator line
    width = SIDEBAR_WIDTH;
    height = sidebarHeight - 4; // Leave space for both separator lines
}

void WeatherManager::drawWeatherToBuffer() {
    if (!currentWeather.isValid) {
        Serial.println("Weather data not valid, attempting fetch...");
        fetchWeatherData();

        // If still not valid, show error message
        if (!currentWeather.isValid) {
            Serial.println("Weather fetch failed, showing error message");
            int displayHeight = display.height();
            int sidebarHeight = displayHeight / 3;
            int weatherY = sidebarHeight + 10;
            int weatherX = 10;

            clearWeatherArea();
            display.setCursor(weatherX, weatherY);
            display.setTextSize(2);
            display.setTextColor(0);
            display.print("WEATHER");

            display.setCursor(weatherX, weatherY + 40);
            display.setTextSize(2);
            display.setTextColor(0);
            display.print("No Data");

            display.setCursor(weatherX, weatherY + 70);
            display.setTextSize(1);
            display.setTextColor(0);
            display.print("Check WiFi");

            ImageFetcher::drawVerticalSeparator(display); // Ensure separator is visible
            lastWeatherUpdate = millis();
            return;
        }
    }

    drawWeatherDisplay();
    ImageFetcher::drawVerticalSeparator(display); // Ensure separator is visible
    lastWeatherUpdate = millis();
}

bool WeatherManager::isWeatherDataValid() const {
    return currentWeather.isValid;
}

String WeatherManager::getWeatherIcon(String iconCode) {
    // Simple weather icon mapping (could be expanded)
    if (iconCode.startsWith("01")) return "☀"; // Clear sky
    if (iconCode.startsWith("02")) return "⛅"; // Few clouds
    if (iconCode.startsWith("03")) return "☁"; // Scattered clouds
    if (iconCode.startsWith("04")) return "☁"; // Broken clouds
    if (iconCode.startsWith("09")) return "🌧"; // Shower rain
    if (iconCode.startsWith("10")) return "🌦"; // Rain
    if (iconCode.startsWith("11")) return "⛈"; // Thunderstorm
    if (iconCode.startsWith("13")) return "❄"; // Snow
    if (iconCode.startsWith("50")) return "🌫"; // Mist
    return "?"; // Unknown
}
