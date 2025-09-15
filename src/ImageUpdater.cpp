#include "ImageUpdater.h"
#include <HTTPClient.h>

ImageUpdater::ImageUpdater(Inkplate &disp, const char* ssid, const char* password, const char* url, unsigned long refreshMs)
    : display(disp), ssid(ssid), password(password), imageUrl(url), refreshInterval(refreshMs), lastUpdate(0) {}

void ImageUpdater::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate 10 Image Renderer...");

    Serial.println("Initializing Inkplate 10 display...");
    display.begin();

    Serial.println("Display initialized. Checking display info...");
    Serial.print("Display width: ");
    Serial.println(display.width());
    Serial.print("Display height: ");
    Serial.println(display.height());

    // Test basic display functionality
    Serial.println("Testing display operations...");
    display.clearDisplay();
    display.fillScreen(BLACK);
    display.display();
    Serial.println("Display test complete - screen should be black!");

    Serial.println("Connecting to WiFi...");
    connectWiFi();

    Serial.println("Starting image updates...");
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
    Serial.println("Fetching and displaying image...");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, showing status");
        display.clearDisplay();
        showConnectionStatus();
        display.display();
        return;
    }

    Serial.println("Clearing display and drawing image...");
    display.clearDisplay();

    // Let drawImage handle the HTTP request internally
    Serial.println("Attempting to load image from URL...");
    Serial.print("URL: ");
    Serial.println(imageUrl);

    if (display.drawImage(imageUrl, 0, 0, display.width(), display.height())) {
        Serial.println("Image displayed successfully!");
    } else {
        Serial.println("Image loading failed. Possible causes:");
        Serial.println("1. JPEG format not supported by Inkplate");
        Serial.println("2. Image dimensions don't match display");
        Serial.println("3. Network timeout or connection issue");
        Serial.println("4. Insufficient memory for image processing");
        showImageError();
    }

    display.display();
    Serial.println("Display update complete");
}

void ImageUpdater::showConnectionStatus() {
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print("Inkplate 10 Status");

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print("WiFi: Disconnected");

    display.setCursor(10, 60);
    display.print("URL: ");
    display.print(imageUrl);
}

void ImageUpdater::showSuccessStatus(int contentLength) {
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print("Connection OK");

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print("WiFi: Connected (");
    display.print(WiFi.localIP());
    display.print(")");

    display.setCursor(10, 60);
    display.print("Image size: ");
    display.print(contentLength);
    display.print(" bytes");

    display.setCursor(10, 80);
    display.print("URL: ");
    display.print(imageUrl);

    display.setCursor(10, 100);
    display.print("Status: Image download successful");
    display.setCursor(10, 120);
    display.print("Note: Image display disabled due to");
    display.setCursor(10, 140);
    display.print("library crash issue");
}

void ImageUpdater::showErrorStatus(int httpCode) {
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print("HTTP Error");

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print("WiFi: Connected (");
    display.print(WiFi.localIP());
    display.print(")");

    display.setCursor(10, 60);
    display.print("Error code: ");
    display.print(httpCode);

    display.setCursor(10, 80);
    display.print("URL: ");
    display.print(imageUrl);
}

void ImageUpdater::showImageError() {
    display.setCursor(10, 10);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.print("Image Error");

    display.setCursor(10, 40);
    display.setTextSize(1);
    display.print("WiFi: Connected (");
    display.print(WiFi.localIP());
    display.print(")");

    display.setCursor(10, 60);
    display.print("Failed to decode/display image");

    display.setCursor(10, 80);
    display.print("URL: ");
    display.print(imageUrl);
}
