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
    Serial.printf("Region: %dx%d at (%d,%d)\n", region.width, region.height, region.x, region.y);
    Serial.printf("Image URL: %s\n", imageUrl);
    Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    // Always clear the region first
    clearRegion(region);

    // Show immediate visual feedback that we're trying to render
    showLoadingInRegion(region);
    display.display(); // Force immediate display update
    delay(1000); // Give user time to see the loading message

    // Attempt to fetch and display image
    if (fetchAndDisplay(region)) {
        consecutiveFailures = 0;
        Serial.println("✓ Image widget rendered successfully");
    } else {
        consecutiveFailures++;
        Serial.printf("✗ Image widget render failed (attempt %d)\n", consecutiveFailures);

        // Show error immediately, don't wait for 3 failures
        String errorDetails = "URL: ";
        errorDetails += imageUrl;
        if (WiFi.status() != WL_CONNECTED) {
            errorDetails = "WiFi disconnected";
        }
        showErrorInRegion(region, "IMAGE ERROR", "Failed to load image", errorDetails.c_str());
        display.display(); // Force display update for error message
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

    // First, show a "LOADING..." message
    showLoadingInRegion(region);
    display.display(); // Force display update

    HTTPClient http;
    http.begin(imageUrl);
    http.setTimeout(30000); // 30 second timeout

    int httpCode = http.GET();
    Serial.printf("HTTP Response Code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        Serial.printf("Image size: %d bytes\n", contentLength);

        if (contentLength > 0) {
            // Clear the image region
            clearRegion(region);

            // Try the Inkplate drawImage function
            Serial.println("Attempting to draw image with Inkplate library...");
            bool success = display.drawImage(imageUrl, region.x, region.y, false, false);

            if (success) {
                Serial.println("✓ Image downloaded and displayed successfully");
                display.display(); // Force display update
                http.end();
                return true;
            } else {
                Serial.println("✗ Inkplate drawImage failed, showing placeholder");
                showImagePlaceholder(region, "IMAGE LOADED", "But display failed");
                display.display(); // Force display update
            }
        } else {
            Serial.println("Invalid content length");
        }
    } else {
        Serial.printf("HTTP error: %d\n", httpCode);
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
    display.fillRect(region.x, region.y, region.width, region.height, 6);

    // Draw a border around the region
    display.drawRect(region.x, region.y, region.width, region.height, 0);
    display.drawRect(region.x + 1, region.y + 1, region.width - 2, region.height - 2, 0);

    // Calculate center position within region
    int centerX = region.x + region.width / 2;
    int centerY = region.y + region.height / 2;

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
        int maxCharsPerLine = region.width / 6; // Approximate chars per line for size 1

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

void ImageWidget::showLoadingInRegion(const LayoutRegion& region) {
    Serial.println("Showing LOADING message in image region");

    // Clear with light gray background
    display.fillRect(region.x, region.y, region.width, region.height, 6);

    // Draw border
    display.drawRect(region.x, region.y, region.width, region.height, 0);

    // Center the loading text
    display.setTextSize(4);
    display.setTextColor(0);

    String loadingText = "LOADING...";
    int textWidth = loadingText.length() * 24; // Approximate width for size 4
    int centerX = region.x + (region.width - textWidth) / 2;
    int centerY = region.y + (region.height - 32) / 2; // 32 is approximate height for size 4

    display.setCursor(centerX, centerY);
    display.print(loadingText);
}

void ImageWidget::showImagePlaceholder(const LayoutRegion& region, const char* title, const char* subtitle) {
    Serial.printf("Showing image placeholder: %s - %s\n", title, subtitle);

    // Clear with light gray background
    display.fillRect(region.x, region.y, region.width, region.height, 6);

    // Draw border
    display.drawRect(region.x, region.y, region.width, region.height, 0);
    display.drawRect(region.x + 1, region.y + 1, region.width - 2, region.height - 2, 0);

    // Draw a simple image icon (rectangle with X)
    int iconSize = 100;
    int iconX = region.x + (region.width - iconSize) / 2;
    int iconY = region.y + 50;

    display.drawRect(iconX, iconY, iconSize, iconSize, 0);
    display.drawLine(iconX, iconY, iconX + iconSize, iconY + iconSize, 0);
    display.drawLine(iconX + iconSize, iconY, iconX, iconY + iconSize, 0);

    // Draw title
    display.setTextSize(3);
    display.setTextColor(0);
    int titleWidth = strlen(title) * 18;
    display.setCursor(region.x + (region.width - titleWidth) / 2, iconY + iconSize + 30);
    display.print(title);

    // Draw subtitle
    if (subtitle) {
        display.setTextSize(2);
        int subtitleWidth = strlen(subtitle) * 12;
        display.setCursor(region.x + (region.width - subtitleWidth) / 2, iconY + iconSize + 70);
        display.print(subtitle);
    }
}

void ImageWidget::showDiagnosticsInRegion(const LayoutRegion& region, const char* ipAddress, int signalStrength) {
    Serial.println("Showing diagnostics in image region");

    // Clear the region
    clearRegion(region);

    int startY = region.y + 50;
    int lineHeight = 30;
    int currentY = startY;

    display.setTextSize(2);
    display.setTextColor(0);

    // Title
    display.setCursor(region.x + 20, currentY);
    display.print("DIAGNOSTICS");
    currentY += lineHeight * 2;

    // IP Address
    display.setCursor(region.x + 20, currentY);
    display.print("IP: ");
    display.print(ipAddress);
    currentY += lineHeight;

    // Signal strength
    display.setCursor(region.x + 20, currentY);
    display.print("Signal: ");
    display.print(signalStrength);
    display.print(" dBm");
    currentY += lineHeight;

    // Image URL
    display.setTextSize(1);
    display.setCursor(region.x + 20, currentY);
    display.print("URL: ");
    display.print(imageUrl);
}
