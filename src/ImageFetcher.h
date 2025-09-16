#ifndef IMAGE_FETCHER_H
#define IMAGE_FETCHER_H

#include <Inkplate.h>

class ImageFetcher {
public:
    ImageFetcher(Inkplate &display, const char* imageUrl);

    bool fetchAndDisplay();
    int getConsecutiveFailures() const;
    void resetFailureCount();
    void showDiagnosticsInPhotoArea(const char* wifiIP, int signalStrength);
    void showErrorInPhotoArea(const char* title, const char* message, const char* details = nullptr);

private:
    Inkplate &display;
    const char* imageUrl;
    int consecutiveFailures;

    static const int MAX_RETRIES = 3;

    bool attemptImageLoad();
    void handleSuccess();
    void handleFailure();
    bool shouldShowError() const;
};

#endif
