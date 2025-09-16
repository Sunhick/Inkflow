#include "WeatherManager.h"
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
    return url;
}

void WeatherManager::parseWeatherResponse(String response) {
    DynamicJsonDocument doc(1024);
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
        currentWeather.isValid = true;

        Serial.printf("Weather: %.1f°F, %s (code: %d)\n",
                      currentWeather.temperature,
                      currentWeather.description.c_str(),
                      weatherCode);
    } else {
        Serial.println("Failed to parse weather data");
        currentWeather.isValid = false;
    }
}

String WeatherManager::getWeatherDescription(int weatherCode) {
    // Convert Open-Meteo weather codes to descriptions
    switch (weatherCode) {
        case 0: return "Clear";
        case 1: case 2: case 3: return "Partly Cloudy";
        case 45: case 48: return "Foggy";
        case 51: case 53: case 55: return "Drizzle";
        case 56: case 57: return "Freezing Drizzle";
        case 61: case 63: case 65: return "Rain";
        case 66: case 67: return "Freezing Rain";
        case 71: case 73: case 75: return "Snow";
        case 77: return "Snow Grains";
        case 80: case 81: case 82: return "Rain Showers";
        case 85: case 86: return "Snow Showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm";
        default: return "Unknown";
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
    display.print("WEATHER");

    if (!currentWeather.isValid) {
        Serial.println("Weather data not valid, showing error");

        display.setCursor(weatherX, weatherY + 40);
        display.setTextSize(2);
        display.setTextColor(0);
        display.print("N/A");
        return;
    }

    // Draw temperature (large)
    display.setCursor(weatherX, weatherY + 40);
    display.setTextSize(4);
    display.setTextColor(0);
    display.printf("%d°F", (int)currentWeather.temperature);

    // Draw weather description
    display.setCursor(weatherX, weatherY + 90);
    display.setTextSize(2);
    display.setTextColor(0);

    // Get first two words of description for better readability
    String twoWordDesc = currentWeather.description;
    int firstSpace = twoWordDesc.indexOf(' ');
    if (firstSpace > 0) {
        int secondSpace = twoWordDesc.indexOf(' ', firstSpace + 1);
        if (secondSpace > 0) {
            twoWordDesc = twoWordDesc.substring(0, secondSpace);
        }
    }

    display.print(twoWordDesc);

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
            display.print("N/A");
            lastWeatherUpdate = millis();
            return;
        }
    }

    drawWeatherDisplay();
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
