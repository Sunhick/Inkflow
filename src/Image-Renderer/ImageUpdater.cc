#include "ImageUpdater.h"

ImageUpdater::ImageUpdater(Inkplate &disp, const char* ssid, const char* password, const char* url, unsigned long refreshMs)
    : display(disp), ssid(ssid), password(password), imageUrl(url), refreshInterval(refreshMs), lastUpdate(0) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    display.begin();
    connectWiFi();
    fetchAndDisplayImage();
    lastUpdate = millis();
}

void ImageUpdater::loop() {
    if (millis() - lastUpdate >= refreshInterval) {
        connectWiFi();
        fetchAndDisplayImage();
        lastUpdate = millis();
    }
}

void ImageUpdater::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.begin(ssid, password);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(500);
        Serial.print(".");
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi");
    } else {
        Serial.println("\nWi-Fi connection failed hello");
    }
}

void ImageUpdater::fetchAndDisplayImage() {
    HTTPClient http;
    http.begin(imageUrl);

    display.clearDisplay();
    display.setDisplayMode(INKPLATE_3BIT);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        if (len <= 0) {
            display.println("Invalid content length.");
            return;
        }

        // Allocate memory for JPEG buffer
        uint8_t* buffer = (uint8_t*)malloc(len);
        if (!buffer) {
            display.println("Memory allocation failed!");
            return;
        }

        // Stream into buffer
        WiFiClient* stream = http.getStreamPtr();
        int index = 0;
        while (http.connected() && index < len) {
            if (stream->available()) {
                buffer[index++] = stream->read();
            }
        }

        Serial.printf("Image downloaded: %d bytes\n", index);

        // Draw from buffer
        display.clearDisplay();
        if (display.drawJpegFromBuffer(buffer, len, 0, 0, true, false)) {
            Serial.println("Image displayed.");
        } else {
            Serial.println("Failed to draw image.");
        }
        display.display();

        free(buffer);
    } else {
        display.printf("HTTP error: %d\n", httpCode);
    }

    http.end();
    // Serial.println("Fetching image...");
    // display.clearDisplay();
    // display.setDisplayMode(INKPLATE_3BIT);

    
    // if (display.drawJpegFromWeb(imageUrl, 0, 0, true, false)) {
    //     Serial.println("Image loaded.");
    // } else {
    //     Serial.println("Failed to load image.");
    //     display.setCursor(10, 10);
    //     display.setTextSize(2);
    //     display.setTextColor(BLACK);
    //     display.print("<<<< Image failed with changes >>>");
    //     display.print(imageUrl);
    // }

    // display.display();

    //   delay(100);
    // esp_sleep_enable_timer_wakeup(15ll * 60 * 1000 * 1000);
    // esp_deep_sleep_start();
}
