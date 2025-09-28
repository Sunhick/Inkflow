#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Inkplate.h>

// Forward declaration
class Compositor;

class DisplayManager {
public:
    DisplayManager(Inkplate &display);

    void initialize();
    void showStatus(const char* message, const char* networkName = nullptr, const char* ipAddress = nullptr);
    void showError(const char* title, const char* message, const char* wifiStatus = nullptr);
    void showImageError(const char* url, int failures, int retrySeconds, const char* ipAddress, int signalStrength);
    void clear();
    void update();
    void partialUpdate(); // Faster partial refresh (switches to 1-bit temporarily)
    void smartPartialUpdate(); // Optimized partial update for widget regions
    void setupSmoothText(int size, int color = 0); // Helper for smooth text rendering

    // Debug message functionality
    void showDebugMessage(const char* message, bool persistent = false);
    void clearDebugArea();
    void enableDebugMode(bool enable) { debugModeEnabled = enable; }

    // Compositor integration methods (with error handling)
    void setCompositor(Compositor* compositor);
    Compositor* getCompositor() const;
    bool renderWithCompositor(); // Full rendering using compositor
    bool partialRenderWithCompositor(); // Partial rendering using compositor

private:
    Inkplate &display;
    int preferredDisplayMode;
    Compositor* compositor; // Pointer to compositor for advanced rendering
    bool debugModeEnabled;

    // Debug message tracking
    static const int MAX_DEBUG_LINES = 10;
    String debugMessages[MAX_DEBUG_LINES];
    int debugLineCount;
    int debugStartY;

    void setTitle(const char* title);
    void setMessage(const char* message, int y = 40);
    void setSmallText(const char* text, int x, int y);
    void renderDebugMessages();
};

#endif
