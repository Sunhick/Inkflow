#include "ImageWidget.h"
#include "../../core/Compositor.h"
#include "../../managers/ConfigManager.h"

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

void ImageWidget::renderToCompositor(Compositor& compositor, const LayoutRegion& region) {
    Serial.printf("=== IMAGE WIDGET RENDER TO COMPOSITOR START ===\n");
    Serial.printf("Region: %dx%d at (%d,%d)\n", region.getWidth(), region.getHeight(), region.getX(), region.getY());
    Serial.printf("Image URL: %s\n", imageUrl);
    Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    // Clear the region on compositor before drawing
    clearRegionOnCompositor(compositor, region);

    // Attempt to fetch and display image to compositor
    if (fetchAndDisplayToCompositor(compositor, region)) {
        consecutiveFailures = 0;
        Serial.println("Image widget rendered to compositor successfully");
    } else {
        consecutiveFailures++;
        Serial.printf("Image widget render to compositor failed (attempt %d)\n", consecutiveFailures);

        // Show error in the image region on compositor
        String errorDetails = "URL: ";
        errorDetails += imageUrl;
        if (WiFi.status() != WL_CONNECTED) {
            errorDetails = "WiFi disconnected";
        }
        showErrorInRegionToCompositor(compositor, region, "IMAGE ERROR", "Failed to load image", errorDetails.c_str());
    }

    lastImageUpdate = millis();
    Serial.printf("=== IMAGE WIDGET RENDER TO COMPOSITOR END ===\n");
}

bool ImageWidget::fetchAndDisplay(const LayoutRegion& region) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot fetch image");
        return false;
    }

    Serial.printf("Fetching image from: %s\n", imageUrl);

    // DON'T clear the entire display - only draw in our region!
    // The LayoutManager already cleared our specific region before calling render()

    // Try to draw the image only at the correct region position
    bool success = display.drawImage(imageUrl, region.getX(), region.getY(), false, false);

    if (success) {
        Serial.println("Image downloaded and displayed successfully at correct position");
        return true;
    }

    // Try with dithering at the correct position
    success = display.drawImage(imageUrl, region.getX(), region.getY(), true, false);
    if (success) {
        Serial.println("Image displayed with dithering at correct position");
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

bool ImageWidget::fetchAndDisplayToCompositor(Compositor& compositor, const LayoutRegion& region) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot fetch image");
        return false;
    }

    Serial.printf("Fetching image from: %s for compositor\n", imageUrl);

    // For compositor rendering, we need to simulate image display
    // Since the Inkplate library's drawImage method works directly with the display,
    // we need to create a placeholder representation for the compositor

    // In a full implementation, you would:
    // 1. Download the image data
    // 2. Decode it to pixel data
    // 3. Draw each pixel to the compositor using setPixel()

    // For now, we'll create a placeholder that shows the image would be there
    showImagePlaceholderToCompositor(compositor, region, "IMAGE", "Loading...");

    Serial.println("Image placeholder rendered to compositor");
    return true;
}

void ImageWidget::showErrorInRegionToCompositor(Compositor& compositor, const LayoutRegion& region, const char* title, const char* message, const char* details) {
    Serial.printf("=== SHOWING ERROR IN IMAGE REGION TO COMPOSITOR ===\n");
    Serial.printf("Title: %s\n", title);
    Serial.printf("Message: %s\n", message);
    Serial.printf("Details: %s\n", details ? details : "None");

    // Clear the region with a light gray background (color 200 for 8-bit grayscale)
    compositor.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 200);

    // Draw a border around the region (black)
    compositor.drawRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 0);
    compositor.drawRect(region.getX() + 1, region.getY() + 1, region.getWidth() - 2, region.getHeight() - 2, 0);

    // Calculate center position within region
    int centerX = region.getX() + region.getWidth() / 2;
    int centerY = region.getY() + region.getHeight() / 2;

    // Draw error title area (simplified as filled rectangles)
    int titleWidth = strlen(title) * 18; // Approximate width for size 3
    compositor.fillRect(centerX - titleWidth/2, centerY - 80, titleWidth, 25, 0); // Black rectangle for title

    // Draw error message area
    int messageWidth = strlen(message) * 12; // Approximate width for size 2
    compositor.fillRect(centerX - messageWidth/2, centerY - 30, messageWidth, 20, 0); // Black rectangle for message

    // Draw details area if provided
    if (details) {
        String detailsStr = String(details);
        int maxCharsPerLine = region.getWidth() / 6; // Approximate chars per line for size 1

        int y = centerY + 10;
        int startIdx = 0;
        while (startIdx < detailsStr.length()) {
            String line = detailsStr.substring(startIdx, startIdx + maxCharsPerLine);
            int lineWidth = line.length() * 6;
            compositor.fillRect(centerX - lineWidth/2, y, lineWidth, 12, 0); // Black rectangle for details line
            startIdx += maxCharsPerLine;
            y += 15;
        }
    }

    Serial.printf("Error display to compositor complete\n");
}

void ImageWidget::showImagePlaceholderToCompositor(Compositor& compositor, const LayoutRegion& region, const char* title, const char* subtitle) {
    Serial.printf("Showing image placeholder to compositor: %s - %s\n", title, subtitle ? subtitle : "");

    // Clear with light gray background (color 200 for 8-bit grayscale)
    compositor.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 200);

    // Draw border (black)
    compositor.drawRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 0);
    compositor.drawRect(region.getX() + 1, region.getY() + 1, region.getWidth() - 2, region.getHeight() - 2, 0);

    // Draw a simple image icon (rectangle with X)
    int iconSize = 100;
    int iconX = region.getX() + (region.getWidth() - iconSize) / 2;
    int iconY = region.getY() + 50;

    compositor.drawRect(iconX, iconY, iconSize, iconSize, 0);
    // Draw X inside the rectangle
    for (int i = 0; i < iconSize; i++) {
        compositor.setPixel(iconX + i, iconY + i, 0); // Diagonal line
        compositor.setPixel(iconX + iconSize - 1 - i, iconY + i, 0); // Other diagonal
    }

    // Draw title area (simplified as filled rectangle)
    int titleWidth = strlen(title) * 18;
    compositor.fillRect(region.getX() + (region.getWidth() - titleWidth) / 2, iconY + iconSize + 30, titleWidth, 25, 0);

    // Draw subtitle area if provided
    if (subtitle) {
        int subtitleWidth = strlen(subtitle) * 12;
        compositor.fillRect(region.getX() + (region.getWidth() - subtitleWidth) / 2, iconY + iconSize + 70, subtitleWidth, 20, 0);
    }
}

void ImageWidget::showDiagnosticsInRegionToCompositor(Compositor& compositor, const LayoutRegion& region, const char* ipAddress, int signalStrength) {
    Serial.println("Showing diagnostics in image region to compositor");

    // Clear the region on compositor
    clearRegionOnCompositor(compositor, region);

    int startY = region.getY() + 50;
    int lineHeight = 30;
    int currentY = startY;

    // Title area
    compositor.fillRect(region.getX() + 20, currentY, 120, 20, 0); // "DIAGNOSTICS"
    currentY += lineHeight * 2;

    // IP Address area
    compositor.fillRect(region.getX() + 20, currentY, 200, 20, 0); // "IP: xxx.xxx.xxx.xxx"
    currentY += lineHeight;

    // Signal strength area
    compositor.fillRect(region.getX() + 20, currentY, 150, 20, 0); // "Signal: -xx dBm"
    currentY += lineHeight;

    // Image URL area (smaller text)
    compositor.fillRect(region.getX() + 20, currentY, region.getWidth() - 40, 15, 0); // URL text
}
WidgetType ImageWidget::getWidgetType() const {
    return WidgetTypeTraits<ImageWidget>::type();
}
