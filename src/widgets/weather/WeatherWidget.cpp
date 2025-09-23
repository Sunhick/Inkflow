#include "WeatherWidget.h"
#include "../../core/Compositor.h"
#include "../../managers/ConfigManager.h"

const char* WeatherWidget::WEATHER_API_URL = "https://api.open-meteo.com/v1/forecast";

WeatherWidget::WeatherWidget(Inkplate& display, const String& latitude, const String& longitude,
                           const String& city, const String& units)
    : Widget(display), lastWeatherUpdate(0), weatherLatitude(latitude),
      weatherLongitude(longitude), weatherCity(city), weatherUnits(units) {
    currentWeather.isValid = false;
}

void WeatherWidget::begin() {
    Serial.println("Initializing weather widget...");
    currentWeather.isValid = false;
    lastWeatherUpdate = 0;
}

bool WeatherWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) || (lastWeatherUpdate == 0);
}

void WeatherWidget::render(const LayoutRegion& region) {
    Serial.printf("Rendering weather widget in region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the widget region
    clearRegion(region);

    // Fetch weather data if not valid
    if (!currentWeather.isValid) {
        Serial.println("Weather data not valid, attempting fetch...");
        fetchWeatherData();
    }

    // Draw weather content within the region
    drawWeatherDisplay(region);

    lastWeatherUpdate = millis();
}

void WeatherWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    Serial.printf("Rendering weather widget to compositor in region: %dx%d at (%d,%d)\n",
                  region.getWidth(), region.getHeight(), region.getX(), region.getY());

    // Clear the widget region on compositor
    clearRegionOnCompositor(compositor, region);

    // Fetch weather data if not valid
    if (!currentWeather.isValid) {
        Serial.println("Weather data not valid, attempting fetch...");
        fetchWeatherData();
    }

    // Draw weather content to compositor within the region
    drawWeatherDisplayToCompositor(compositor, region);

    lastWeatherUpdate = millis();
}

void WeatherWidget::fetchWeatherData() {
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
    http.setTimeout(5000); // 5 second timeout
    http.setReuse(false); // Don't keep connection alive
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

String WeatherWidget::buildWeatherURL() {
    String url = String(WEATHER_API_URL);
    url += "?latitude=" + weatherLatitude;
    url += "&longitude=" + weatherLongitude;
    url += "&current_weather=true&temperature_unit=" + weatherUnits;
    url += "&hourly=precipitation_probability&forecast_days=1";
    return url;
}

void WeatherWidget::parseWeatherResponse(String response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response.c_str());

    if (error) {
        Serial.printf("JSON parsing error: %s\n", error.c_str());
        currentWeather.isValid = false;
        return;
    }

    if (doc["current_weather"]["temperature"]) {
        currentWeather.temperature = doc["current_weather"]["temperature"];
        currentWeather.humidity = 0; // Open-Meteo doesn't provide humidity in basic call

        int weatherCode = doc["current_weather"]["weathercode"];
        currentWeather.description = getWeatherDescription(weatherCode);
        currentWeather.icon = String(weatherCode);

        currentWeather.precipitationProbability = 0;
        if (doc["hourly"]["precipitation_probability"]) {
            JsonArray precipArray = doc["hourly"]["precipitation_probability"];
            if (precipArray.size() > 0) {
                currentWeather.precipitationProbability = precipArray[0];
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

void WeatherWidget::drawWeatherDisplay(const LayoutRegion& region) {
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // Draw "WEATHER" title
    display.setCursor(labelX, labelY);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("WEATHER");

    // Draw city name
    display.setCursor(labelX, labelY + 25);
    display.setTextSize(3);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print(weatherCity);

    if (!currentWeather.isValid) {
        display.setCursor(labelX, labelY + 55);
        display.setTextSize(2);
        display.setTextColor(0);
        display.setTextWrap(true);
        display.print("No Data");

        display.setCursor(labelX, labelY + 85);
        display.setTextSize(1);
        display.setTextColor(0);
        display.print("Check WiFi");
        return;
    }

    // Draw temperature (large)
    display.setCursor(labelX, labelY + 55);
    display.setTextSize(4);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print((int)currentWeather.temperature);
    display.print("F");

    // Draw weather description
    display.setCursor(labelX, labelY + 105);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print(currentWeather.description);

    // Add precipitation probability
    display.setCursor(labelX, labelY + 135);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setTextWrap(true);
    display.print("Rain: ");
    display.print(currentWeather.precipitationProbability);
    display.print("%");
}

const char* WeatherWidget::getWeatherDescription(int weatherCode) {
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

bool WeatherWidget::isWeatherDataValid() const {
    return currentWeather.isValid;
}

void WeatherWidget::drawWeatherDisplayToCompositor(Compositor& compositor, const LayoutRegion& region) {
    int margin = 10;
    int labelX = region.getX() + margin;
    int labelY = region.getY() + margin;

    // Draw "WEATHER" title area (simplified as filled rectangle)
    compositor.fillRect(labelX, labelY, 80, 20, 0); // Black rectangle for "WEATHER"

    // Draw city name area
    int cityWidth = weatherCity.length() * 18; // Approximate width for size 3
    compositor.fillRect(labelX, labelY + 25, cityWidth, 25, 0); // Black rectangle for city

    if (!currentWeather.isValid) {
        // Draw "No Data" area
        compositor.fillRect(labelX, labelY + 55, 80, 20, 0); // Black rectangle for "No Data"

        // Draw "Check WiFi" area
        compositor.fillRect(labelX, labelY + 85, 100, 15, 0); // Black rectangle for "Check WiFi"
        return;
    }

    // Draw temperature area (large)
    String tempStr = String((int)currentWeather.temperature) + "F";
    int tempWidth = tempStr.length() * 24; // Approximate width for size 4
    compositor.fillRect(labelX, labelY + 55, tempWidth, 35, 0); // Black rectangle for temperature

    // Draw weather description area
    int descWidth = currentWeather.description.length() * 12; // Approximate width for size 2
    compositor.fillRect(labelX, labelY + 105, descWidth, 20, 0); // Black rectangle for description

    // Draw precipitation probability area
    String precipStr = "Rain: " + String(currentWeather.precipitationProbability) + "%";
    int precipWidth = precipStr.length() * 12; // Approximate width for size 2
    compositor.fillRect(labelX, labelY + 135, precipWidth, 20, 0); // Black rectangle for precipitation
}
WidgetType WeatherWidget::getWidgetType() const {
    return WidgetTypeTraits<WeatherWidget>::type();
}
