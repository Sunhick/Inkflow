#include "ImageWidget.h"

ImageWidget::ImageWidget(Inkplate& display, const char* imageUrl)
    : Widget(display), imageUrl(imageUrl), consecutiveFailures(0), lastImageUpdate(0) {}

void ImageWidget::begin() {
    Serial.println("Initializing image widget...");
    consecutiveFailures = 0;
    lastImageUpdate = 0;
}

bool ImageWidget::shouldUpdate() {
    unsigned long currentTime = millis();
    return (currentTime - lastImageUpdate >= IMAGE_UPDATE_INTERVAL) || (lastImageUpdate == 0);
}

void ImageWidget::render(const LayoutRegion& region) {
    Serial.printf("=== IMAGE WIDGET RENDER START ===\n");
    Serial.printf("Region: %dx%d at (%d,%d)\n", region.getWidth(), region.getHeight(), region.getX(), region.getY());
    Serial.printf("Image URL: %s\n", imageUrl);
    Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    // Attempt to fetch and display image
    if (fetchAndDisplay(region)) {
        consecutiveFailures = 0;
        Serial.println("Image widget rendered successfully");
    } else {
        consecutiveFailures++;
        Serial.printf("Image widget render failed (attempt %d)\n", consecutiveFailures);

        // Show error in the image region
        String errorDetails = "URL: ";
        errorDetails += imageUrl;
        if (WiFi.status() != WL_CONNECTED) {
            errorDetails = "WiFi disconnected";
        }
        showErrorInRegion(region, "IMAGE ERROR", "Failed to load image", errorDetails.c_str());
    }

    lastImageUpdate = millis();
    Serial.printf("=== IMAGE WIDGET RENDER END ===\n");
}

bool ImageWidget::fetchAndDisplay(const LayoutRegion& region) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot fetch image");
        return false;
    }

    Serial.printf("Fetching image from: %s\n", imageUrl);

    // Clear the entire display first (like the old working code)
    display.clearDisplay();

    // Try to draw the image - no loading messages, just attempt silently
    bool success = display.drawImage(imageUrl, region.getX(), region.getY(), false, false);

    if (success) {
        Serial.println("Image downloaded and displayed successfully");
        return true;
    }

    // Try without specifying position
    success = display.drawImage(imageUrl, 0, 0, false, false);
    if (success) {
        Serial.println("Image displayed at origin successfully");
        return true;
    }

    // Try with dithering
    success = display.drawImage(imageUrl, 0, 0, true, false);
    if (success) {
        Serial.println("Image displayed with dithering");
        return true;
    }

    // Only if all attempts fail, run diagnostics
    Serial.println("All image drawing attempts failed - running diagnostics");
    Serial.printf("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("WiFi Signal: %d dBm\n", WiFi.RSSI());

    // Test HTTP connection to see what the issue might be
    HTTPClient http;
    http.begin(imageUrl);
    http.setTimeout(10000);

    int httpCode = http.GET();
    Serial.printf("HTTP Response Code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        String contentType = http.header("Content-Type");
        Serial.printf("Content-Length: %d bytes\n", contentLength);
        Serial.printf("Content-Type: %s\n", contentType.c_str());
    } else {
        Serial.printf("HTTP error: %d - %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
    return false;
}

void ImageWidget::showErrorInRegion(const LayoutRegion& region, const char* title, const char* message, const char* details) {
    Serial.printf("=== SHOWING ERROR IN IMAGE REGION ===\n");
    Serial.printf("Title: %s\n", title);
    Serial.printf("Message: %s\n", message);
    Serial.printf("Details: %s\n", details ? details : "None");

    // Clear the region with a light gray background to make it visible
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 6);

    // Draw a border around the region
    display.drawRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 0);
    display.drawRect(region.getX() + 1, region.getY() + 1, region.getWidth() - 2, region.getHeight() - 2, 0);

    // Calculate center position within region
    int centerX = region.getX() + region.getWidth() / 2;
    int centerY = region.getY() + region.getHeight() / 2;

    // Draw error title
    display.setTextSize(3);
    display.setTextColor(0);

    // Center the title text
    int titleWidth = strlen(title) * 18; // Approximate width for size 3
    display.setCursor(centerX - titleWidth/2, centerY - 80);
    display.print(title);

    // Draw error message
    display.setTextSize(2);
    int messageWidth = strlen(message) * 12; // Approximate width for size 2
    display.setCursor(centerX - messageWidth/2, centerY - 30);
    display.print(message);

    // Draw details if provided
    if (details) {
        display.setTextSize(1);
        // Split long details into multiple lines if needed
        String detailsStr = String(details);
        int maxCharsPerLine = region.getWidth() / 6; // Approximate chars per line for size 1

        int y = centerY + 10;
        int startIdx = 0;
        while (startIdx < detailsStr.length()) {
            String line = detailsStr.substring(startIdx, startIdx + maxCharsPerLine);
            int lineWidth = line.length() * 6;
            display.setCursor(centerX - lineWidth/2, y);
            display.print(line);
            startIdx += maxCharsPerLine;
            y += 15;
        }
    }

    Serial.printf("Error display complete\n");
}



void ImageWidget::showImagePlaceholder(const LayoutRegion& region, const char* title, const char* subtitle) {
    Serial.printf("Showing image placeholder: %s - %s\n", title, subtitle);

    // Clear with light gray background
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 6);

    // Draw border
    display.drawRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 0);
    display.drawRect(region.getX() + 1, region.getY() + 1, region.getWidth() - 2, region.getHeight() - 2, 0);

    // Draw a simple image icon (rectangle with X)
    int iconSize = 100;
    int iconX = region.getX() + (region.getWidth() - iconSize) / 2;
    int iconY = region.getY() + 50;

    display.drawRect(iconX, iconY, iconSize, iconSize, 0);
    display.drawLine(iconX, iconY, iconX + iconSize, iconY + iconSize, 0);
    display.drawLine(iconX + iconSize, iconY, iconX, iconY + iconSize, 0);

    // Draw title
    display.setTextSize(3);
    display.setTextColor(0);
    int titleWidth = strlen(title) * 18;
    display.setCursor(region.getX() + (region.getWidth() - titleWidth) / 2, iconY + iconSize + 30);
    display.print(title);

    // Draw subtitle
    if (subtitle) {
        display.setTextSize(2);
        int subtitleWidth = strlen(subtitle) * 12;
        display.setCursor(region.getX() + (region.getWidth() - subtitleWidth) / 2, iconY + iconSize + 70);
        display.print(subtitle);
    }
}

void ImageWidget::showDiagnosticsInRegion(const LayoutRegion& region, const char* ipAddress, int signalStrength) {
    Serial.println("Showing diagnostics in image region");

    // Clear the region
    clearRegion(region);

    int startY = region.getY() + 50;
    int lineHeight = 30;
    int currentY = startY;

    display.setTextSize(2);
    display.setTextColor(0);

    // Title
    display.setCursor(region.getX() + 20, currentY);
    display.print("DIAGNOSTICS");
    currentY += lineHeight * 2;

    // IP Address
    display.setCursor(region.getX() + 20, currentY);
    display.print("IP: ");
    display.print(ipAddress);
    currentY += lineHeight;

    // Signal strength
    display.setCursor(region.getX() + 20, currentY);
    display.print("Signal: ");
    display.print(signalStrength);
    display.print(" dBm");
    currentY += lineHeight;

    // Image URL
    display.setTextSize(1);
    display.setCursor(region.getX() + 20, currentY);
    display.print("URL: ");
    display.print(imageUrl);
}
