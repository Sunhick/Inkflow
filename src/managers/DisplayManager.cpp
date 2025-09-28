#include "DisplayManager.h"
#include "../core/Logger.h"
#include "../core/Compositor.h"

DisplayManager::DisplayManager(Inkplate &display) : display(display), preferredDisplayMode(INKPLATE_3BIT), compositor(nullptr), debugModeEnabled(false), debugLineCount(0), debugStartY(700) {}

void DisplayManager::initialize() {
    display.begin();
    preferredDisplayMode = INKPLATE_3BIT;
    display.setDisplayMode(preferredDisplayMode); // 3-bit mode for better grayscale

    // Enable text smoothing and wrapping for better rendering
    display.setTextWrap(true);
    display.cp437(true); // Enable extended character set
}

void DisplayManager::showStatus(const char* message, const char* networkName, const char* ipAddress) {
    LOG_INFO("DisplayManager", "Showing status: %s", message);

    clear();
    setTitle("Inkplate Status");
    setMessage(message);

    if (networkName) {
        setSmallText(("Network: " + String(networkName)).c_str(), 10, 60);
    }

    if (ipAddress) {
        setSmallText(("IP: " + String(ipAddress)).c_str(), 10, 80);
    }

    update();
}

void DisplayManager::showError(const char* title, const char* message, const char* wifiStatus) {
    LOG_ERROR("DisplayManager", "%s - %s", title, message);

    clear();
    setTitle(title);
    setMessage(message);

    if (wifiStatus) {
        setSmallText(("WiFi Status: " + String(wifiStatus)).c_str(), 10, 70);
    }

    setSmallText("Will retry automatically", 10, 90);
    update();
}

void DisplayManager::showImageError(const char* url, int failures, int retrySeconds, const char* ipAddress, int signalStrength) {
    clear();
    setTitle("Image Load Failed");

    setSmallText(("URL: " + String(url)).c_str(), 10, 40);
    setSmallText(("WiFi: " + String(ipAddress) + " (" + String(signalStrength) + " dBm)").c_str(), 10, 60);
    setSmallText(("Failures: " + String(failures)).c_str(), 10, 80);
    setSmallText(("Next retry in " + String(retrySeconds) + " seconds").c_str(), 10, 100);

    update();
}

void DisplayManager::clear() {
    display.clearDisplay();
}

void DisplayManager::update() {
    LOG_DEBUG("DisplayManager", "Performing full display update...");
    display.display();
    LOG_DEBUG("DisplayManager", "Display update complete");
}

void DisplayManager::partialUpdate() {
    LOG_DEBUG("DisplayManager", "Performing partial display update...");

    // Switch to 1-bit mode for partial updates (required for Inkplate)
    if (display.getDisplayMode() != INKPLATE_1BIT) {
        LOG_DEBUG("DisplayManager", "Switching to 1-bit mode for partial update");
        display.setDisplayMode(INKPLATE_1BIT);
    }

    display.partialUpdate();

    // Switch back to preferred mode
    if (preferredDisplayMode != INKPLATE_1BIT) {
        LOG_DEBUG("DisplayManager", "Switching back to preferred display mode");
        display.setDisplayMode(preferredDisplayMode);
    }

    LOG_DEBUG("DisplayManager", "Partial update complete");
}

void DisplayManager::smartPartialUpdate() {
    LOG_DEBUG("DisplayManager", "Performing smart partial update...");

    // For small widget updates, use partial update with mode switching
    // This is optimized for time/battery widget updates

    // Store current mode
    int currentMode = display.getDisplayMode();

    // Switch to 1-bit for partial update
    if (currentMode != INKPLATE_1BIT) {
        display.setDisplayMode(INKPLATE_1BIT);
    }

    // Perform partial update
    display.partialUpdate();

    // Restore original mode
    if (currentMode != INKPLATE_1BIT) {
        display.setDisplayMode(currentMode);
    }

    LOG_DEBUG("DisplayManager", "Smart partial update complete");
}

void DisplayManager::setTitle(const char* title) {
    display.setCursor(10, 10);
    setupSmoothText(2, 0);
    display.print(title);
}

void DisplayManager::setMessage(const char* message, int y) {
    display.setCursor(10, y);
    setupSmoothText(1, 0);
    display.print(message);
}

void DisplayManager::setSmallText(const char* text, int x, int y) {
    display.setCursor(x, y);
    setupSmoothText(1, 0);
    display.print(text);
}

void DisplayManager::setupSmoothText(int size, int color) {
    display.setTextSize(size);
    display.setTextColor(color);
    display.setTextWrap(true);
}

// Compositor integration methods
void DisplayManager::setCompositor(Compositor* comp) {
    compositor = comp;
    LOG_INFO("DisplayManager", "Compositor set");
}

Compositor* DisplayManager::getCompositor() const {
    return compositor;
}

bool DisplayManager::renderWithCompositor() {
    if (!compositor) {
        LOG_DEBUG("DisplayManager", "No compositor available, falling back to direct rendering");
        try {
            update(); // Fallback to direct rendering
            return true;
        } catch (...) {
            LOG_ERROR("DisplayManager", "Direct rendering fallback failed");
            return false;
        }
    }

    if (!compositor->isInitialized()) {
        LOG_DEBUG("DisplayManager", "Compositor not initialized, falling back to direct rendering");
        try {
            update(); // Fallback to direct rendering
            return true;
        } catch (...) {
            LOG_ERROR("DisplayManager", "Direct rendering fallback failed");
            return false;
        }
    }

    // Check for compositor errors before rendering
    if (compositor->getLastError() != CompositorError::None) {
        LOG_WARN("DisplayManager", "Compositor has error (%s), attempting recovery",
                 compositor->getErrorString(compositor->getLastError()));
        if (!compositor->recoverFromError()) {
            LOG_ERROR("DisplayManager", "Compositor recovery failed, falling back to direct rendering");
            try {
                update();
                return true;
            } catch (...) {
                LOG_ERROR("DisplayManager", "Direct rendering fallback failed");
                return false;
            }
        }
    }

    LOG_DEBUG("DisplayManager", "Performing full render with compositor...");

    try {
        // Use compositor to render to display
        if (!compositor->displayToInkplate(display)) {
            LOG_ERROR("DisplayManager", "Compositor display failed - %s",
                      compositor->getErrorString(compositor->getLastError()));

            // Attempt recovery
            if (compositor->recoverFromError()) {
                LOG_INFO("DisplayManager", "Compositor recovered, retrying display");
                if (compositor->displayToInkplate(display)) {
                    LOG_DEBUG("DisplayManager", "Compositor full render complete after recovery");
                    return true;
                }
            }

            // Final fallback to direct rendering
            LOG_WARN("DisplayManager", "Falling back to direct rendering after compositor failure");
            update();
            return true; // Consider fallback as success
        }

        LOG_DEBUG("DisplayManager", "Compositor full render complete");
        return true;
    } catch (...) {
        LOG_ERROR("DisplayManager", "Exception during compositor rendering, falling back to direct rendering");
        try {
            update();
            return true;
        } catch (...) {
            LOG_FATAL("DisplayManager", "All rendering methods failed");
            return false;
        }
    }
}

bool DisplayManager::partialRenderWithCompositor() {
    if (!compositor) {
        LOG_DEBUG("DisplayManager", "No compositor available for partial render, falling back to smart partial update");
        try {
            smartPartialUpdate(); // Fallback to existing partial update
            return true;
        } catch (...) {
            LOG_ERROR("DisplayManager", "Smart partial update fallback failed");
            return false;
        }
    }

    if (!compositor->isInitialized()) {
        LOG_DEBUG("DisplayManager", "Compositor not initialized for partial render, falling back to smart partial update");
        try {
            smartPartialUpdate(); // Fallback to existing partial update
            return true;
        } catch (...) {
            LOG_ERROR("DisplayManager", "Smart partial update fallback failed");
            return false;
        }
    }

    // Check if there are any changes to render
    if (!compositor->hasChangedRegions()) {
        LOG_DEBUG("DisplayManager", "No changes detected in compositor, skipping partial render");
        return true; // No changes is not an error
    }

    // Check for compositor errors before rendering
    if (compositor->getLastError() != CompositorError::None) {
        LOG_WARN("DisplayManager", "Compositor has error (%s) during partial render, attempting recovery",
                 compositor->getErrorString(compositor->getLastError()));
        if (!compositor->recoverFromError()) {
            LOG_ERROR("DisplayManager", "Compositor recovery failed, falling back to smart partial update");
            try {
                smartPartialUpdate();
                return true;
            } catch (...) {
                LOG_ERROR("DisplayManager", "Smart partial update fallback failed");
                return false;
            }
        }
    }

    LOG_DEBUG("DisplayManager", "Performing partial render with compositor...");

    // Store current display mode
    int currentMode = display.getDisplayMode();
    bool modeChanged = false;

    try {
        // Switch to 1-bit mode for partial updates (required for Inkplate partial updates)
        if (currentMode != INKPLATE_1BIT) {
            LOG_DEBUG("DisplayManager", "Switching to 1-bit mode for compositor partial update");
            display.setDisplayMode(INKPLATE_1BIT);
            modeChanged = true;
        }

        // Use compositor for partial rendering
        if (!compositor->partialDisplayToInkplate(display)) {
            LOG_ERROR("DisplayManager", "Compositor partial display failed - %s",
                      compositor->getErrorString(compositor->getLastError()));

            // Restore display mode before attempting recovery
            if (modeChanged) {
                display.setDisplayMode(currentMode);
                modeChanged = false;
            }

            // Attempt recovery
            if (compositor->recoverFromError()) {
                LOG_INFO("DisplayManager", "Compositor recovered, retrying partial display");
                if (currentMode != INKPLATE_1BIT) {
                    display.setDisplayMode(INKPLATE_1BIT);
                    modeChanged = true;
                }

                if (compositor->partialDisplayToInkplate(display)) {
                    // Restore original display mode
                    if (modeChanged) {
                        display.setDisplayMode(currentMode);
                    }
                    LOG_DEBUG("DisplayManager", "Compositor partial render complete after recovery");
                    return true;
                }
            }

            // Final fallback to smart partial update
            LOG_WARN("DisplayManager", "Falling back to smart partial update after compositor failure");
            smartPartialUpdate();
            return true; // Consider fallback as success
        }

        // Restore original display mode
        if (modeChanged) {
            LOG_DEBUG("DisplayManager", "Restoring display mode after compositor partial update");
            display.setDisplayMode(currentMode);
        }

        LOG_DEBUG("DisplayManager", "Compositor partial render complete");
        return true;
    } catch (...) {
        // Ensure display mode is restored even if an exception occurs
        if (modeChanged) {
            try {
                display.setDisplayMode(currentMode);
            } catch (...) {
                LOG_ERROR("DisplayManager", "Failed to restore display mode after exception");
            }
        }

        LOG_ERROR("DisplayManager", "Exception during compositor partial rendering, falling back to smart partial update");
        try {
            smartPartialUpdate();
            return true;
        } catch (...) {
            LOG_FATAL("DisplayManager", "All partial rendering methods failed");
            return false;
        }
    }
}
void DisplayManager::showDebugMessage(const char* message, bool persistent) {
    if (!debugModeEnabled) {
        return; // Debug mode disabled
    }

    // Add message to debug buffer
    if (debugLineCount < MAX_DEBUG_LINES) {
        debugMessages[debugLineCount] = String(message);
        debugLineCount++;
    } else {
        // Shift messages up and add new one at bottom
        for (int i = 0; i < MAX_DEBUG_LINES - 1; i++) {
            debugMessages[i] = debugMessages[i + 1];
        }
        debugMessages[MAX_DEBUG_LINES - 1] = String(message);
    }

    // Render debug messages if persistent or immediately
    if (persistent) {
        renderDebugMessages();
        update();
    }
}

void DisplayManager::clearDebugArea() {
    if (!debugModeEnabled) {
        return;
    }

    // Clear the debug area on screen
    display.fillRect(0, debugStartY, display.width(), display.height() - debugStartY, 7); // White background

    // Clear debug message buffer
    debugLineCount = 0;
    for (int i = 0; i < MAX_DEBUG_LINES; i++) {
        debugMessages[i] = "";
    }
}

void DisplayManager::renderDebugMessages() {
    if (!debugModeEnabled || debugLineCount == 0) {
        return;
    }

    // Clear debug area first
    display.fillRect(0, debugStartY, display.width(), display.height() - debugStartY, 7); // White background

    // Draw debug border
    display.drawRect(0, debugStartY - 2, display.width(), display.height() - debugStartY + 2, 0); // Black border

    // Set small font for debug messages
    display.setTextSize(1);
    display.setTextColor(0); // Black text

    // Render each debug message
    int y = debugStartY + 5;
    for (int i = 0; i < debugLineCount && i < MAX_DEBUG_LINES; i++) {
        if (debugMessages[i].length() > 0) {
            display.setCursor(5, y);
            display.print(debugMessages[i]);
            y += 12; // Line spacing
        }
    }
}
