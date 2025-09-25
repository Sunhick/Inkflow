#include "Compositor.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>

Compositor::Compositor(int width, int height)
    : virtualSurface(nullptr)
    , dirtyRegions(nullptr)
    , surfaceWidth(width)
    , surfaceHeight(height)
    , bytesPerPixel(1)  // 8-bit grayscale
    , surfaceSize(0)
    , hasChanges(false)
    , lastError(CompositorError::None)
    , fallbackMode(false)
    , memoryPressureThreshold(1024 * 1024)  // 1MB default threshold
    , maxRetryAttempts(3) {

    // Initialize performance metrics
    metrics = {0, 0, 0, 0, 0.0f, 0.0f};

    // Initialize optimization parameters with sensible defaults
    maxRegionMergeDistance = 50;  // pixels
    minRegionSizeForPartialUpdate = 100;  // minimum 100 pixels for partial update
    updateFrequencyThreshold = 1000;  // milliseconds
    regionMergeEfficiencyThreshold = 0.7f;  // 70% efficiency threshold

    // Validate dimensions
    if (width <= 0 || height <= 0 || width > 10000 || height > 10000) {
        setError(CompositorError::InvalidDimensions);
        LOG_ERROR("Compositor", "Invalid dimensions %dx%d", width, height);
        return;
    }

    surfaceSize = static_cast<size_t>(surfaceWidth) * surfaceHeight * bytesPerPixel;

    // Check for potential overflow
    if (surfaceSize / bytesPerPixel / surfaceHeight != static_cast<size_t>(surfaceWidth)) {
        setError(CompositorError::InvalidDimensions);
        LOG_ERROR("Compositor", "Surface size calculation overflow");
        return;
    }

    LOG_DEBUG("Compositor", "Created with dimensions %dx%d, surface size: %zu bytes",
              width, height, surfaceSize);
}

Compositor::~Compositor() {
    cleanup();
}

bool Compositor::initialize() {
    // Clear any previous errors
    clearError();

    // Check if already in error state from constructor
    if (lastError != CompositorError::None) {
        logError("initialize", lastError);
        return false;
    }

    // Clean up any existing allocation
    cleanup();

    // Check memory pressure before allocation
    if (checkMemoryPressure()) {
        setError(CompositorError::MemoryAllocationFailed);
        logError("initialize", lastError);
        return false;
    }

    // Allocate virtual surface buffer
    virtualSurface = new(std::nothrow) uint8_t[surfaceSize];
    if (!virtualSurface) {
        setError(CompositorError::MemoryAllocationFailed);
        logError("initialize", lastError);
        return false;
    }

    // Allocate dirty regions tracking (one bool per pixel for simplicity)
    size_t dirtySize = static_cast<size_t>(surfaceWidth) * surfaceHeight;
    dirtyRegions = new(std::nothrow) bool[dirtySize];
    if (!dirtyRegions) {
        delete[] virtualSurface;
        virtualSurface = nullptr;
        setError(CompositorError::MemoryAllocationFailed);
        logError("initialize", lastError);
        return false;
    }

    // Initialize buffers
    clear();
    resetChangeTracking();

    LOG_INFO("Compositor", "Successfully initialized %dx%d surface (%zu bytes)",
             surfaceWidth, surfaceHeight, surfaceSize);
    return true;
}

bool Compositor::initializeWithRetry(int maxAttempts) {
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        LOG_DEBUG("Compositor", "Initialization attempt %d/%d", attempt, maxAttempts);

        if (initialize()) {
            LOG_INFO("Compositor", "Successfully initialized on attempt %d", attempt);
            return true;
        }

        // Wait before retry (exponential backoff)
        if (attempt < maxAttempts) {
            int delayMs = 100 * (1 << (attempt - 1)); // 100ms, 200ms, 400ms, etc.
            LOG_DEBUG("Compositor", "Initialization failed, retrying in %dms", delayMs);
            delay(delayMs);
        }
    }

    LOG_ERROR("Compositor", "Failed to initialize after %d attempts", maxAttempts);
    setFallbackMode(true);
    return false;
}

void Compositor::cleanup() {
    if (virtualSurface) {
        delete[] virtualSurface;
        virtualSurface = nullptr;
    }

    if (dirtyRegions) {
        delete[] dirtyRegions;
        dirtyRegions = nullptr;
    }

    changedAreas.clear();
    hasChanges = false;
    regionHistory.clear();
}

void Compositor::clear() {
    if (!virtualSurface) return;

    // Clear to white (7 is white in 3-bit mode, 255 in 8-bit)
    std::memset(virtualSurface, 255, surfaceSize);

    // Mark entire surface as changed
    LayoutRegion fullSurface(0, 0, surfaceWidth, surfaceHeight);
    markRegionChanged(fullSurface);
}

bool Compositor::clearRegion(const LayoutRegion& region) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("clearRegion", lastError);
        return false;
    }

    // Validate and correct region if needed
    if (!validateRegion(region)) {
        LayoutRegion correctedRegion = correctInvalidRegion(region);
        if (correctedRegion.getWidth() <= 0 || correctedRegion.getHeight() <= 0) {
            setError(CompositorError::InvalidRegion);
            logError("clearRegion", lastError);
            return false;
        }
        LOG_WARN("Compositor", "Corrected invalid region (%d,%d,%d,%d) to (%d,%d,%d,%d)",
                 region.getX(), region.getY(), region.getWidth(), region.getHeight(),
                 correctedRegion.getX(), correctedRegion.getY(),
                     correctedRegion.getWidth(), correctedRegion.getHeight());
        return clearRegion(correctedRegion);
    }

    // Clamp region to surface bounds
    LayoutRegion clampedRegion = clampRegionToBounds(region);
    int startX = clampedRegion.getX();
    int startY = clampedRegion.getY();
    int endX = clampedRegion.getX() + clampedRegion.getWidth();
    int endY = clampedRegion.getY() + clampedRegion.getHeight();

    // Clear the region to white
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (!setPixel(x, y, 255)) {  // White
                // Continue with other pixels even if one fails
                LOG_WARN("Compositor", "Failed to clear pixel at (%d,%d)", x, y);
            }
        }
    }

    return markRegionChanged(clampedRegion);
}

uint8_t* Compositor::getSurfaceBuffer() {
    return virtualSurface;
}

const uint8_t* Compositor::getSurfaceBuffer() const {
    return virtualSurface;
}

size_t Compositor::getPixelIndex(int x, int y) const {
    return static_cast<size_t>(y) * surfaceWidth + x;
}

bool Compositor::isValidCoordinate(int x, int y) const {
    return x >= 0 && x < surfaceWidth && y >= 0 && y < surfaceHeight;
}

bool Compositor::setPixel(int x, int y, uint8_t color) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        return false;
    }

    if (!isValidCoordinate(x, y)) {
        // Don't set error for out-of-bounds pixels as this is common during clipping
        return false;
    }

    size_t index = getPixelIndex(x, y);
    virtualSurface[index] = color;

    // Mark pixel as dirty
    if (dirtyRegions) {
        dirtyRegions[index] = true;
        hasChanges = true;
    }

    return true;
}

uint8_t Compositor::getPixel(int x, int y) const {
    if (!virtualSurface || !isValidCoordinate(x, y)) return 255;  // Return white for invalid coordinates

    return virtualSurface[getPixelIndex(x, y)];
}

bool Compositor::drawRect(int x, int y, int w, int h, uint8_t color) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("drawRect", lastError);
        return false;
    }

    if (w <= 0 || h <= 0) {
        setError(CompositorError::InvalidRegion);
        return false;
    }

    bool success = true;

    // Draw top and bottom edges
    for (int i = 0; i < w; i++) {
        if (!setPixel(x + i, y, color)) success = false;           // Top edge
        if (!setPixel(x + i, y + h - 1, color)) success = false;  // Bottom edge
    }

    // Draw left and right edges
    for (int i = 0; i < h; i++) {
        if (!setPixel(x, y + i, color)) success = false;           // Left edge
        if (!setPixel(x + w - 1, y + i, color)) success = false;  // Right edge
    }

    // Mark region as changed
    LayoutRegion region(x, y, w, h);
    markRegionChanged(region);

    return success;
}

bool Compositor::fillRect(int x, int y, int w, int h, uint8_t color) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("fillRect", lastError);
        return false;
    }

    if (w <= 0 || h <= 0) {
        setError(CompositorError::InvalidRegion);
        return false;
    }

    // Clamp to surface bounds
    int startX = std::max(0, x);
    int startY = std::max(0, y);
    int endX = std::min(surfaceWidth, x + w);
    int endY = std::min(surfaceHeight, y + h);

    bool success = true;

    // Fill the rectangle
    for (int py = startY; py < endY; py++) {
        for (int px = startX; px < endX; px++) {
            if (!setPixel(px, py, color)) {
                success = false;
                // Continue filling other pixels
            }
        }
    }

    // Mark region as changed
    LayoutRegion region(x, y, w, h);
    if (!markRegionChanged(region)) {
        success = false;
    }

    return success;
}

bool Compositor::markRegionChanged(const LayoutRegion& region) {
    // Validate region bounds
    if (!validateRegion(region)) {
        LayoutRegion correctedRegion = correctInvalidRegion(region);
        if (correctedRegion.getWidth() <= 0 || correctedRegion.getHeight() <= 0) {
            setError(CompositorError::InvalidRegion);
            return false;
        }
        return markRegionChanged(correctedRegion);
    }

    // Clamp region to surface bounds
    LayoutRegion clampedRegion = clampRegionToBounds(region);

    // Skip if region is completely outside surface
    if (clampedRegion.getWidth() <= 0 || clampedRegion.getHeight() <= 0) {
        return true; // Not an error, just nothing to mark
    }

    try {
        // Add to changed areas list
        changedAreas.push_back(clampedRegion);
        hasChanges = true;

        // Mark dirty regions if tracking is enabled
        if (dirtyRegions) {
            int startX = clampedRegion.getX();
            int startY = clampedRegion.getY();
            int endX = clampedRegion.getX() + clampedRegion.getWidth();
            int endY = clampedRegion.getY() + clampedRegion.getHeight();

            for (int y = startY; y < endY; y++) {
                for (int x = startX; x < endX; x++) {
                    size_t index = getPixelIndex(x, y);
                    if (index < static_cast<size_t>(surfaceWidth * surfaceHeight)) {
                        dirtyRegions[index] = true;
                    }
                }
            }
        }

        // Optimize regions for partial updates
        optimizeRegionsForPartialUpdate();
        return true;
    } catch (...) {
        setError(CompositorError::MemoryAllocationFailed);
        logError("markRegionChanged", lastError);
        return false;
    }
}

void Compositor::resetChangeTracking() {
    changedAreas.clear();
    hasChanges = false;

    if (dirtyRegions) {
        std::memset(dirtyRegions, false, surfaceWidth * surfaceHeight * sizeof(bool));
    }
}

bool Compositor::hasChangedRegions() const {
    return hasChanges;
}

std::vector<LayoutRegion> Compositor::getChangedRegions() const {
    return changedAreas;
}

bool Compositor::displayToInkplate(Inkplate& display) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("displayToInkplate", lastError);
        return false;
    }

    try {
        LOG_DEBUG("Compositor", "Starting full display update");

        // Clear the display buffer
        display.clearDisplay();

        // Copy virtual surface to display buffer
        // Note: This is a simplified implementation. In practice, you might need
        // to convert between different color formats or handle display-specific requirements

        for (int y = 0; y < surfaceHeight; y++) {
            for (int x = 0; x < surfaceWidth; x++) {
                uint8_t pixel = getPixel(x, y);

                // Convert 8-bit grayscale to Inkplate's expected format
                // Inkplate uses inverted values where 0 = black, 7 = white in 3-bit mode
                uint8_t inkplateColor;
                if (pixel >= 224) inkplateColor = 7;      // White
                else if (pixel >= 192) inkplateColor = 6;
                else if (pixel >= 160) inkplateColor = 5;
                else if (pixel >= 128) inkplateColor = 4;
                else if (pixel >= 96) inkplateColor = 3;
                else if (pixel >= 64) inkplateColor = 2;
                else if (pixel >= 32) inkplateColor = 1;
                else inkplateColor = 0;                   // Black

                display.drawPixel(x, y, inkplateColor);
            }
        }

        // Perform full display update
        display.display();

        // Reset change tracking after successful display
        resetChangeTracking();

        LOG_DEBUG("Compositor", "Full display update completed successfully");
        return true;
    } catch (...) {
        setError(CompositorError::DisplayUpdateFailed);
        logError("displayToInkplate", lastError);
        return false;
    }
}

bool Compositor::partialDisplayToInkplate(Inkplate& display) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("partialDisplayToInkplate", lastError);
        return false;
    }

    if (!hasChanges) {
        LOG_DEBUG("Compositor", "No changes to display");
        return true; // Not an error, just nothing to do
    }

    // Get changed regions and optimize them
    std::vector<LayoutRegion> regions = getChangedRegions();
    return partialDisplayToInkplate(display, regions);
}

bool Compositor::partialDisplayToInkplate(Inkplate& display, const std::vector<LayoutRegion>& specificRegions) {
    if (!virtualSurface) {
        setError(CompositorError::SurfaceNotInitialized);
        logError("partialDisplayToInkplate", lastError);
        return false;
    }

    if (specificRegions.empty()) {
        LOG_DEBUG("Compositor", "No regions to update");
        return true;
    }

    try {
        unsigned long startTime = millis();
        LOG_DEBUG("Compositor", "Performing optimized partial display update...");

        // Optimize regions for efficient partial updates
        std::vector<LayoutRegion> optimizedRegions = coalesceRegions(specificRegions);

        // Check if we should use partial update or fall back to full update
        if (!shouldUsePartialUpdate(optimizedRegions)) {
            LOG_DEBUG("Compositor", "Falling back to full display update for efficiency");
            return displayToInkplate(display);
        }

        LOG_DEBUG("Compositor", "Optimized %d regions to %d for update",
                  specificRegions.size(), optimizedRegions.size());

        size_t totalPixelsUpdated = 0;

        // Update each optimized region
        for (const auto& region : optimizedRegions) {
            LOG_DEBUG("Compositor", "Updating region (%d,%d) %dx%d",
                      region.getX(), region.getY(), region.getWidth(), region.getHeight());

            // Validate region before processing
            if (!validateRegion(region)) {
                LOG_WARN("Compositor", "Skipping invalid region (%d,%d) %dx%d",
                         region.getX(), region.getY(), region.getWidth(), region.getHeight());
                continue;
            }

            // Copy pixels from virtual surface to display for this region
            for (int y = region.getY(); y < region.getY() + region.getHeight(); y++) {
                for (int x = region.getX(); x < region.getX() + region.getWidth(); x++) {
                    if (isValidCoordinate(x, y)) {
                        uint8_t pixel = getPixel(x, y);

                        // Convert 8-bit grayscale to Inkplate's expected format
                        uint8_t inkplateColor;
                        if (pixel >= 224) inkplateColor = 7;      // White
                        else if (pixel >= 192) inkplateColor = 6;
                        else if (pixel >= 160) inkplateColor = 5;
                        else if (pixel >= 128) inkplateColor = 4;
                        else if (pixel >= 96) inkplateColor = 3;
                        else if (pixel >= 64) inkplateColor = 2;
                        else if (pixel >= 32) inkplateColor = 1;
                        else inkplateColor = 0;                   // Black

                        display.drawPixel(x, y, inkplateColor);
                        totalPixelsUpdated++;
                    }
                }
            }

            // Update region history for future optimization
            unsigned long regionUpdateTime = millis() - startTime;
            updateRegionHistory(region, regionUpdateTime);
        }

        // Perform partial display update
        display.partialUpdate();

        // Update performance metrics
        unsigned long totalUpdateTime = millis() - startTime;
        updatePerformanceMetrics(totalUpdateTime, totalPixelsUpdated);

        // Reset change tracking after successful partial display
        resetChangeTracking();

        LOG_DEBUG("Compositor", "Partial display update completed in %lums (%d pixels)",
                  totalUpdateTime, totalPixelsUpdated);
        return true;
    } catch (...) {
        setError(CompositorError::DisplayUpdateFailed);
        logError("partialDisplayToInkplate", lastError);
        return false;
    }
}

size_t Compositor::getMemoryUsage() const {
    size_t usage = 0;

    if (virtualSurface) {
        usage += surfaceSize;
    }

    if (dirtyRegions) {
        usage += surfaceWidth * surfaceHeight * sizeof(bool);
    }

    usage += changedAreas.capacity() * sizeof(LayoutRegion);

    return usage;
}

void Compositor::mergeOverlappingRegions() {
    if (changedAreas.size() <= 1) return;

    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 0; i < changedAreas.size() && !merged; i++) {
            for (size_t j = i + 1; j < changedAreas.size(); j++) {
                if (regionsOverlap(changedAreas[i], changedAreas[j])) {
                    // Merge regions
                    LayoutRegion mergedRegion = mergeRegions(changedAreas[i], changedAreas[j]);
                    changedAreas[i] = mergedRegion;
                    changedAreas.erase(changedAreas.begin() + j);
                    merged = true;
                    break;
                }
            }
        }
    }
}

bool Compositor::regionsOverlap(const LayoutRegion& a, const LayoutRegion& b) const {
    // Check if regions overlap or are adjacent (for merging efficiency)
    int a_right = a.getX() + a.getWidth();
    int a_bottom = a.getY() + a.getHeight();
    int b_right = b.getX() + b.getWidth();
    int b_bottom = b.getY() + b.getHeight();

    // Allow merging of adjacent regions (within 1 pixel) to reduce fragmentation
    return !(a_right < b.getX() - 1 || b_right < a.getX() - 1 ||
             a_bottom < b.getY() - 1 || b_bottom < a.getY() - 1);
}

LayoutRegion Compositor::mergeRegions(const LayoutRegion& a, const LayoutRegion& b) const {
    int left = std::min(a.getX(), b.getX());
    int top = std::min(a.getY(), b.getY());
    int right = std::max(a.getX() + a.getWidth(), b.getX() + b.getWidth());
    int bottom = std::max(a.getY() + a.getHeight(), b.getY() + b.getHeight());

    return LayoutRegion(left, top, right - left, bottom - top);
}

// Error handling helper methods
void Compositor::setError(CompositorError error) {
    lastError = error;
    if (error != CompositorError::None) {
        LOG_ERROR("Compositor", "Error set - %s", getErrorString(error));
    }
}

void Compositor::logError(const char* operation, CompositorError error) const {
    LOG_ERROR("Compositor", "%s failed - %s", operation, getErrorString(error));
}

const char* Compositor::getErrorString(CompositorError error) const {
    switch (error) {
        case CompositorError::None:
            return "No error";
        case CompositorError::MemoryAllocationFailed:
            return "Memory allocation failed";
        case CompositorError::InvalidDimensions:
            return "Invalid dimensions";
        case CompositorError::SurfaceNotInitialized:
            return "Surface not initialized";
        case CompositorError::InvalidRegion:
            return "Invalid region";
        case CompositorError::DisplayUpdateFailed:
            return "Display update failed";
        case CompositorError::WidgetRenderingFailed:
            return "Widget rendering failed";
        default:
            return "Unknown error";
    }
}

bool Compositor::validateRegion(const LayoutRegion& region) const {
    return region.getWidth() > 0 && region.getHeight() > 0 &&
           region.getX() >= -surfaceWidth && region.getY() >= -surfaceHeight &&
           region.getX() < surfaceWidth * 2 && region.getY() < surfaceHeight * 2;
}

LayoutRegion Compositor::clampRegionToBounds(const LayoutRegion& region) const {
    int x = std::max(0, region.getX());
    int y = std::max(0, region.getY());
    int maxWidth = surfaceWidth - x;
    int maxHeight = surfaceHeight - y;
    int width = std::min(region.getWidth(), maxWidth);
    int height = std::min(region.getHeight(), maxHeight);

    // Ensure non-negative dimensions
    width = std::max(0, width);
    height = std::max(0, height);

    return LayoutRegion(x, y, width, height);
}

bool Compositor::checkMemoryPressure() const {
    // Simple memory pressure check - in a real implementation,
    // you might check available heap memory
    size_t requiredMemory = surfaceSize + (surfaceWidth * surfaceHeight * sizeof(bool));
    return requiredMemory > memoryPressureThreshold;
}

bool Compositor::isValidRegion(const LayoutRegion& region) const {
    return validateRegion(region);
}

LayoutRegion Compositor::correctInvalidRegion(const LayoutRegion& region) const {
    // Correct negative dimensions
    int width = std::max(0, region.getWidth());
    int height = std::max(0, region.getHeight());

    // Clamp coordinates to reasonable bounds
    int x = std::max(-surfaceWidth, std::min(surfaceWidth * 2, region.getX()));
    int y = std::max(-surfaceHeight, std::min(surfaceHeight * 2, region.getY()));

    return LayoutRegion(x, y, width, height);
}

bool Compositor::recoverFromError() {
    LOG_INFO("Compositor", "Attempting error recovery");

    CompositorError originalError = lastError;
    clearError();

    switch (originalError) {
        case CompositorError::MemoryAllocationFailed:
            // Try to reinitialize with retry
            cleanup();
            if (initializeWithRetry(2)) {
                LOG_INFO("Compositor", "Recovered from memory allocation failure");
                setFallbackMode(false);
                return true;
            }
            setFallbackMode(true);
            return false;

        case CompositorError::SurfaceNotInitialized:
            // Try to initialize
            if (initialize()) {
                LOG_INFO("Compositor", "Recovered from uninitialized surface");
                return true;
            }
            return false;

        case CompositorError::DisplayUpdateFailed:
            // Clear error and try again next time
            LOG_INFO("Compositor", "Cleared display update error");
            return true;

        default:
            // For other errors, just clear and continue
            LOG_INFO("Compositor", "Cleared error: %s", getErrorString(originalError));
            return true;
    }
}

// Partial update optimization methods

void Compositor::optimizeRegionsForPartialUpdate() {
    if (changedAreas.size() <= 1) return;

    // First merge overlapping regions
    mergeOverlappingRegions();

    // Then apply intelligent coalescing
    changedAreas = coalesceRegions(changedAreas);
}

bool Compositor::shouldMergeRegions(const LayoutRegion& a, const LayoutRegion& b) const {
    // Calculate distance between regions
    int centerAX = a.getX() + a.getWidth() / 2;
    int centerAY = a.getY() + a.getHeight() / 2;
    int centerBX = b.getX() + b.getWidth() / 2;
    int centerBY = b.getY() + b.getHeight() / 2;

    int distance = abs(centerAX - centerBX) + abs(centerAY - centerBY); // Manhattan distance

    if (distance > static_cast<int>(maxRegionMergeDistance)) {
        return false;
    }

    // Calculate merge efficiency
    LayoutRegion merged = mergeRegions(a, b);
    float efficiency = calculateRegionMergeEfficiency(merged, a, b);

    return efficiency >= regionMergeEfficiencyThreshold;
}

std::vector<LayoutRegion> Compositor::coalesceRegions(const std::vector<LayoutRegion>& regions) const {
    if (regions.size() <= 1) return regions;

    std::vector<LayoutRegion> result = regions;

    // Iteratively merge regions that should be combined
    bool merged = true;
    while (merged && result.size() > 1) {
        merged = false;
        for (size_t i = 0; i < result.size() && !merged; i++) {
            for (size_t j = i + 1; j < result.size(); j++) {
                if (shouldMergeRegions(result[i], result[j])) {
                    // Merge regions
                    LayoutRegion mergedRegion = mergeRegions(result[i], result[j]);
                    result[i] = mergedRegion;
                    result.erase(result.begin() + j);
                    merged = true;
                    break;
                }
            }
        }
    }

    // Filter out regions that are too small for partial update
    std::vector<LayoutRegion> filteredResult;
    for (const auto& region : result) {
        size_t regionSize = static_cast<size_t>(region.getWidth()) * region.getHeight();
        if (regionSize >= minRegionSizeForPartialUpdate) {
            filteredResult.push_back(region);
        }
    }

    return filteredResult.empty() ? result : filteredResult;
}

void Compositor::updateRegionHistory(const LayoutRegion& region, unsigned long updateTime) {
    unsigned long currentTime = millis();

    // Look for existing history entry for similar region
    for (auto& history : regionHistory) {
        if (abs(history.region.getX() - region.getX()) <= 10 &&
            abs(history.region.getY() - region.getY()) <= 10 &&
            abs(history.region.getWidth() - region.getWidth()) <= 10 &&
            abs(history.region.getHeight() - region.getHeight()) <= 10) {

            // Update existing entry
            history.lastUpdateTime = currentTime;
            history.updateFrequency++;
            history.totalUpdateTime += updateTime;
            return;
        }
    }

    // Add new history entry
    RegionUpdateHistory newHistory = {region, currentTime, 1, updateTime};
    regionHistory.push_back(newHistory);

    // Limit history size to prevent memory growth
    if (regionHistory.size() > 100) {
        // Remove oldest entries
        regionHistory.erase(regionHistory.begin(), regionHistory.begin() + 20);
    }
}

bool Compositor::shouldUsePartialUpdate(const std::vector<LayoutRegion>& regions) const {
    if (regions.empty()) return false;

    // Calculate total area to be updated
    size_t totalUpdateArea = 0;
    for (const auto& region : regions) {
        totalUpdateArea += static_cast<size_t>(region.getWidth()) * region.getHeight();
    }

    // Calculate total surface area
    size_t totalSurfaceArea = static_cast<size_t>(surfaceWidth) * surfaceHeight;

    // Use partial update if updating less than 30% of the surface
    float updateRatio = static_cast<float>(totalUpdateArea) / totalSurfaceArea;

    if (updateRatio > 0.3f) {
        LOG_DEBUG("Compositor", "Update ratio %.2f%% too high for partial update", updateRatio * 100);
        return false;
    }

    // Check if regions are too fragmented
    if (regions.size() > 10) {
        LOG_DEBUG("Compositor", "Too many regions (%d) for efficient partial update", regions.size());
        return false;
    }

    return true;
}

void Compositor::updatePerformanceMetrics(unsigned long updateTime, size_t pixelsUpdated) {
    metrics.lastUpdateTime = millis();
    metrics.updateCount++;
    metrics.totalUpdateTime += updateTime;
    metrics.totalPixelsUpdated += pixelsUpdated;

    // Calculate running averages
    if (metrics.updateCount > 0) {
        metrics.averageUpdateTime = static_cast<float>(metrics.totalUpdateTime) / metrics.updateCount;
        metrics.averagePixelsPerUpdate = static_cast<float>(metrics.totalPixelsUpdated) / metrics.updateCount;
    }
}

float Compositor::calculateRegionMergeEfficiency(const LayoutRegion& merged, const LayoutRegion& a, const LayoutRegion& b) const {
    // Calculate areas
    size_t areaA = static_cast<size_t>(a.getWidth()) * a.getHeight();
    size_t areaB = static_cast<size_t>(b.getWidth()) * b.getHeight();
    size_t areaMerged = static_cast<size_t>(merged.getWidth()) * merged.getHeight();

    // Calculate efficiency as ratio of useful area to total merged area
    size_t usefulArea = areaA + areaB;

    if (areaMerged == 0) return 0.0f;

    return static_cast<float>(usefulArea) / areaMerged;
}

void Compositor::resetPerformanceMetrics() {
    metrics = {0, 0, 0, 0, 0.0f, 0.0f};
    regionHistory.clear();
}
