#include "ImageFetcher.h"
#include "Config.h"

ImageFetcher::ImageFetcher(Inkplate &display, const char* imageUrl)
    : display(display), imageUrl(imageUrl), consecutiveFailures(0) {}

bool ImageFetcher::fetchAndDisplay() {
    Serial.printf("Fetching image from: %s\n", imageUrl);

    if (attemptImageLoad()) {
        handleSuccess();
        return true;
    } else {
        handleFailure();
        return false;
    }
}

int ImageFetcher::getConsecutiveFailures() const {
    return consecutiveFailures;
}

void ImageFetcher::resetFailureCount() {
    consecutiveFailures = 0;
}

bool ImageFetcher::attemptImageLoad() {
    // Clear the entire display first
    display.clearDisplay();

    // Fill the left sidebar area with white background for status info
    display.fillRect(0, 0, SIDEBAR_WIDTH, display.height(), 7);

    // Draw vertical separator line between sidebar and image area
    drawVerticalSeparator(display);

    // Debug info
    Serial.printf("Display dimensions: %dx%d\n", display.width(), display.height());
    Serial.printf("Sidebar width: %d\n", SIDEBAR_WIDTH);
    Serial.printf("Image area width: %d\n", IMAGE_AREA_WIDTH);
    Serial.printf("Loading image at position: x=%d, y=%d\n", SIDEBAR_WIDTH + 2, 0);
    Serial.printf("Available space: %dx%d\n", IMAGE_AREA_WIDTH - 2, display.height());

    // Load image in the right 80% of the display (starting at SIDEBAR_WIDTH + 3)
    Serial.printf("Attempting to load image at position (%d, 0)\n", SIDEBAR_WIDTH + 3);
    bool success = display.drawJpegFromWeb(imageUrl, SIDEBAR_WIDTH + 3, 0, true, false);
    Serial.printf("Image load result: %s\n", success ? "SUCCESS" : "FAILED");

    if (!success) {
        Serial.println("Positioned image load failed, trying fallback at (0,0)...");
        // Clear display and try loading at origin as fallback
        display.clearDisplay();
        success = display.drawJpegFromWeb(imageUrl, 0, 0, true, false);

        if (success) {
            Serial.println("Fallback image load succeeded - display dimensions may be incorrect");
            // Redraw sidebar over the image
            display.fillRect(0, 0, SIDEBAR_WIDTH, display.height(), 7);
            drawVerticalSeparator(display);
        } else {
            Serial.println("Both positioned and fallback image loads failed - possible causes:");
            Serial.println("1. Network connectivity issue");
            Serial.println("2. Image too large for display");
            Serial.println("3. Invalid image format");
            Serial.println("4. Memory allocation failure");
        }
    }

    return success;
}

void ImageFetcher::handleSuccess() {
    Serial.println("Image loaded successfully");
    consecutiveFailures = 0;
    display.display();
}

void ImageFetcher::handleFailure() {
    consecutiveFailures++;
    Serial.printf("Image load failed (attempt %d)\n", consecutiveFailures);
}

bool ImageFetcher::shouldShowError() const {
    return consecutiveFailures >= MAX_RETRIES;
}

void ImageFetcher::showDiagnosticsInPhotoArea(const char* wifiIP, int signalStrength) {
    // Clear only the photo area (right side)
    int photoAreaX = SIDEBAR_WIDTH + 3;
    int photoAreaWidth = IMAGE_AREA_WIDTH - 3;
    display.fillRect(photoAreaX, 0, photoAreaWidth, display.height(), 7);

    // Ensure vertical separator is visible
    drawVerticalSeparator(display);

    // Draw diagnostics text in the photo area
    int textX = photoAreaX + 20;
    int textY = 50;

    display.setTextColor(0);

    // Title
    display.setCursor(textX, textY);
    display.setTextSize(3);
    display.print("IMAGE LOAD FAILED");

    textY += 60;

    // URL
    display.setCursor(textX, textY);
    display.setTextSize(2);
    display.print("URL:");
    textY += 30;
    display.setCursor(textX, textY);
    display.setTextSize(1);
    display.print(imageUrl);

    textY += 40;

    // WiFi Info
    display.setCursor(textX, textY);
    display.setTextSize(2);
    display.print("NETWORK STATUS:");
    textY += 30;
    display.setCursor(textX, textY);
    display.setTextSize(1);
    display.printf("IP Address: %s", wifiIP);
    textY += 20;
    display.setCursor(textX, textY);
    display.printf("Signal Strength: %d dBm", signalStrength);

    textY += 40;

    // Failure count
    display.setCursor(textX, textY);
    display.setTextSize(2);
    display.print("ATTEMPTS:");
    textY += 30;
    display.setCursor(textX, textY);
    display.setTextSize(1);
    display.printf("Failed attempts: %d", consecutiveFailures);

    textY += 40;

    // Status
    display.setCursor(textX, textY);
    display.setTextSize(2);
    display.print("STATUS:");
    textY += 30;
    display.setCursor(textX, textY);
    display.setTextSize(1);
    display.print("Will retry automatically");
    display.setCursor(textX, textY + 20);
    display.print("Check network connection");
    display.setCursor(textX, textY + 40);
    display.print("Verify image server is running");
}

void ImageFetcher::showErrorInPhotoArea(const char* title, const char* message, const char* details) {
    // Clear only the photo area (right side)
    int photoAreaX = SIDEBAR_WIDTH + 3;
    int photoAreaWidth = IMAGE_AREA_WIDTH - 3;
    display.fillRect(photoAreaX, 0, photoAreaWidth, display.height(), 7);

    // Ensure vertical separator is visible
    drawVerticalSeparator(display);

    // Draw error text in the photo area
    int textX = photoAreaX + 20;
    int textY = 50;

    display.setTextColor(0);

    // Title
    display.setCursor(textX, textY);
    display.setTextSize(3);
    display.print(title);

    textY += 60;

    // Message
    display.setCursor(textX, textY);
    display.setTextSize(2);
    display.print(message);

    textY += 40;

    // Details if provided
    if (details) {
        display.setCursor(textX, textY);
        display.setTextSize(1);
        display.print(details);
        textY += 30;
    }

    // Status
    display.setCursor(textX, textY);
    display.setTextSize(1);
    display.print("System will retry automatically");
}

void ImageFetcher::drawVerticalSeparator(Inkplate &display) {
    // Draw a thick vertical line between sidebar and image area
    display.drawLine(SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH, display.height(), 0);
    display.drawLine(SIDEBAR_WIDTH + 1, 0, SIDEBAR_WIDTH + 1, display.height(), 0);
    display.drawLine(SIDEBAR_WIDTH + 2, 0, SIDEBAR_WIDTH + 2, display.height(), 0);

    Serial.printf("Drew vertical separator at x=%d (sidebar width: %d)\n", SIDEBAR_WIDTH, SIDEBAR_WIDTH);
}
