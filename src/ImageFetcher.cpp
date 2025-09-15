#include "ImageFetcher.h"

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
    // Clear display but preserve space for battery indicator
    display.clearDisplay();

    // Load image with some margin at bottom for battery display
    bool success = display.drawJpegFromWeb(imageUrl, 0, 0, true, false);

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
