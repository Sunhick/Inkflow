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
    display.drawLine(SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH, display.height(), 0);
    display.drawLine(SIDEBAR_WIDTH + 1, 0, SIDEBAR_WIDTH + 1, display.height(), 0);

    // Load image in the right 70% of the display (starting at SIDEBAR_WIDTH)
    bool success = display.drawJpegFromWeb(imageUrl, SIDEBAR_WIDTH + 2, 0, true, false);

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
    int photoAreaX = SIDEBAR_WIDTH + 2;
    int photoAreaWidth = IMAGE_AREA_WIDTH - 2;
    display.fillRect(photoAreaX, 0, photoAreaWidth, display.height(), 7);

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
    int photoAreaX = SIDEBAR_WIDTH + 2;
    int photoAreaWidth = IMAGE_AREA_WIDTH - 2;
    display.fillRect(photoAreaX, 0, photoAreaWidth, display.height(), 7);

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
