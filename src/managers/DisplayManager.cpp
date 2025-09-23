#include "DisplayManager.h"
#include "../core/Compositor.h"

DisplayManager::DisplayManager(Inkplate &display) : display(display), preferredDisplayMode(INKPLATE_3BIT), compositor(nullptr) {}

void DisplayManager::initialize() {
    display.begin();
    preferredDisplayMode = INKPLATE_3BIT;
    display.setDisplayMode(preferredDisplayMode); // 3-bit mode for better grayscale

    // Enable text smoothing and wrapping for better rendering
    display.setTextWrap(true);
    display.cp437(true); // Enable extended character set
}

void DisplayManager::showStatus(const char* message, const char* networkName, const char* ipAddress) {
    Serial.println(message);

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
    Serial.printf("ERROR: %s - %s\n", title, message);

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
    Serial.println("DisplayManager: Performing full display update...");
    display.display();
    Serial.println("DisplayManager: Display update complete");
}

void DisplayManager::partialUpdate() {
    Serial.println("DisplayManager: Performing partial display update...");

    // Switch to 1-bit mode for partial updates (required for Inkplate)
    if (display.getDisplayMode() != INKPLATE_1BIT) {
        Serial.println("DisplayManager: Switching to 1-bit mode for partial update");
        display.setDisplayMode(INKPLATE_1BIT);
    }

    display.partialUpdate();

    // Switch back to preferred mode
    if (preferredDisplayMode != INKPLATE_1BIT) {
        Serial.println("DisplayManager: Switching back to preferred display mode");
        display.setDisplayMode(preferredDisplayMode);
    }

    Serial.println("DisplayManager: Partial update complete");
}

void DisplayManager::smartPartialUpdate() {
    Serial.println("DisplayManager: Performing smart partial update...");

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

    Serial.println("DisplayManager: Smart partial update complete");
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
    Serial.println("DisplayManager: Compositor set");
}

Compositor* DisplayManager::getCompositor() const {
    return compositor;
}

bool DisplayManager::renderWithCompositor() {
    if (!compositor) {
        Serial.println("DisplayManager: No compositor available, falling back to direct rendering");
        try {
            update(); // Fallback to direct rendering
            return true;
        } catch (...) {
            Serial.println("DisplayManager: Direct rendering fallback failed");
            return false;
        }
    }

    if (!compositor->isInitialized()) {
        Serial.println("DisplayManager: Compositor not initialized, falling back to direct rendering");
        try {
            update(); // Fallback to direct rendering
            return true;
        } catch (...) {
            Serial.println("DisplayManager: Direct rendering fallback failed");
            return false;
        }
    }

    // Check for compositor errors before rendering
    if (compositor->getLastError() != CompositorError::None) {
        Serial.printf("DisplayManager: Compositor has error (%s), attempting recovery\n",
                     compositor->getErrorString(compositor->getLastError()));
        if (!compositor->recoverFromError()) {
            Serial.println("DisplayManager: Compositor recovery failed, falling back to direct rendering");
            try {
                update();
                return true;
            } catch (...) {
                Serial.println("DisplayManager: Direct rendering fallback failed");
                return false;
            }
        }
    }

    Serial.println("DisplayManager: Performing full render with compositor...");

    try {
        // Use compositor to render to display
        if (!compositor->displayToInkplate(display)) {
            Serial.printf("DisplayManager: Compositor display failed - %s\n",
                         compositor->getErrorString(compositor->getLastError()));

            // Attempt recovery
            if (compositor->recoverFromError()) {
                Serial.println("DisplayManager: Compositor recovered, retrying display");
                if (compositor->displayToInkplate(display)) {
                    Serial.println("DisplayManager: Compositor full render complete after recovery");
                    return true;
                }
            }

            // Final fallback to direct rendering
            Serial.println("DisplayManager: Falling back to direct rendering after compositor failure");
            update();
            return true; // Consider fallback as success
        }

        Serial.println("DisplayManager: Compositor full render complete");
        return true;
    } catch (...) {
        Serial.println("DisplayManager: Exception during compositor rendering, falling back to direct rendering");
        try {
            update();
            return true;
        } catch (...) {
            Serial.println("DisplayManager: All rendering methods failed");
            return false;
        }
    }
}

bool DisplayManager::partialRenderWithCompositor() {
    if (!compositor) {
        Serial.println("DisplayManager: No compositor available for partial render, falling back to smart partial update");
        try {
            smartPartialUpdate(); // Fallback to existing partial update
            return true;
        } catch (...) {
            Serial.println("DisplayManager: Smart partial update fallback failed");
            return false;
        }
    }

    if (!compositor->isInitialized()) {
        Serial.println("DisplayManager: Compositor not initialized for partial render, falling back to smart partial update");
        try {
            smartPartialUpdate(); // Fallback to existing partial update
            return true;
        } catch (...) {
            Serial.println("DisplayManager: Smart partial update fallback failed");
            return false;
        }
    }

    // Check if there are any changes to render
    if (!compositor->hasChangedRegions()) {
        Serial.println("DisplayManager: No changes detected in compositor, skipping partial render");
        return true; // No changes is not an error
    }

    // Check for compositor errors before rendering
    if (compositor->getLastError() != CompositorError::None) {
        Serial.printf("DisplayManager: Compositor has error (%s) during partial render, attempting recovery\n",
                     compositor->getErrorString(compositor->getLastError()));
        if (!compositor->recoverFromError()) {
            Serial.println("DisplayManager: Compositor recovery failed, falling back to smart partial update");
            try {
                smartPartialUpdate();
                return true;
            } catch (...) {
                Serial.println("DisplayManager: Smart partial update fallback failed");
                return false;
            }
        }
    }

    Serial.println("DisplayManager: Performing partial render with compositor...");

    // Store current display mode
    int currentMode = display.getDisplayMode();
    bool modeChanged = false;

    try {
        // Switch to 1-bit mode for partial updates (required for Inkplate partial updates)
        if (currentMode != INKPLATE_1BIT) {
            Serial.println("DisplayManager: Switching to 1-bit mode for compositor partial update");
            display.setDisplayMode(INKPLATE_1BIT);
            modeChanged = true;
        }

        // Use compositor for partial rendering
        if (!compositor->partialDisplayToInkplate(display)) {
            Serial.printf("DisplayManager: Compositor partial display failed - %s\n",
                         compositor->getErrorString(compositor->getLastError()));

            // Restore display mode before attempting recovery
            if (modeChanged) {
                display.setDisplayMode(currentMode);
                modeChanged = false;
            }

            // Attempt recovery
            if (compositor->recoverFromError()) {
                Serial.println("DisplayManager: Compositor recovered, retrying partial display");
                if (currentMode != INKPLATE_1BIT) {
                    display.setDisplayMode(INKPLATE_1BIT);
                    modeChanged = true;
                }

                if (compositor->partialDisplayToInkplate(display)) {
                    // Restore original display mode
                    if (modeChanged) {
                        display.setDisplayMode(currentMode);
                    }
                    Serial.println("DisplayManager: Compositor partial render complete after recovery");
                    return true;
                }
            }

            // Final fallback to smart partial update
            Serial.println("DisplayManager: Falling back to smart partial update after compositor failure");
            smartPartialUpdate();
            return true; // Consider fallback as success
        }

        // Restore original display mode
        if (modeChanged) {
            Serial.println("DisplayManager: Restoring display mode after compositor partial update");
            display.setDisplayMode(currentMode);
        }

        Serial.println("DisplayManager: Compositor partial render complete");
        return true;
    } catch (...) {
        // Ensure display mode is restored even if an exception occurs
        if (modeChanged) {
            try {
                display.setDisplayMode(currentMode);
            } catch (...) {
                Serial.println("DisplayManager: Failed to restore display mode after exception");
            }
        }

        Serial.println("DisplayManager: Exception during compositor partial rendering, falling back to smart partial update");
        try {
            smartPartialUpdate();
            return true;
        } catch (...) {
            Serial.println("DisplayManager: All partial rendering methods failed");
            return false;
        }
    }
}
