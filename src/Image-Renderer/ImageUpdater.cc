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
    Serial.println("Fetching image...");
    display.clearDisplay();
    display.setDisplayMode(INKPLATE_3BIT);

    
    if (display.drawJpegFromWeb(imageUrl, 0, 0, true, false)) {
        Serial.println("Image loaded.");
    } else {
        Serial.println("Failed to load image.");
        display.setCursor(10, 10);
        display.setTextSize(2);
        display.setTextColor(BLACK);
        display.print("Failed to render image on Inkplate 10");
        display.print(imageUrl);
    }

    display.display();
}
