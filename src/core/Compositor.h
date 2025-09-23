#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <cstdint>
#include <vector>
#include <Inkplate.h>
#include "LayoutRegion.h"

/**
 * Error codes for Compositor operations
 */
enum class CompositorError {
    None = 0,
    MemoryAllocationFailed,
    InvalidDimensions,
    SurfaceNotInitialized,
    InvalidRegion,
    DisplayUpdateFailed,
    WidgetRenderingFailed
};

/**
 * Compositor class manages a virtual surface for widget rendering
 * and coordinates the final display output to the Inkplate device.
 * Includes comprehensive error handling and recovery mechanisms.
 */
class Compositor {
private:
    uint8_t* virtualSurface;
    bool* dirtyRegions;
    int surfaceWidth;
    int surfaceHeight;
    int bytesPerPixel;
    size_t surfaceSize;

    // Change tracking
    std::vector<LayoutRegion> changedAreas;
    bool hasChanges;

    // Error handling and recovery
    CompositorError lastError;
    bool fallbackMode;
    size_t memoryPressureThreshold;
    int maxRetryAttempts;

    // Enhanced change tracking
    void mergeOverlappingRegions();
    bool regionsOverlap(const LayoutRegion& a, const LayoutRegion& b) const;
    LayoutRegion mergeRegions(const LayoutRegion& a, const LayoutRegion& b) const;

    // Helper methods
    size_t getPixelIndex(int x, int y) const;
    bool isValidCoordinate(int x, int y) const;

    // Error handling helpers
    void logError(const char* operation, CompositorError error) const;
    bool validateRegion(const LayoutRegion& region) const;
    LayoutRegion clampRegionToBounds(const LayoutRegion& region) const;
    bool checkMemoryPressure() const;

public:
    // Constructor and destructor
    Compositor(int width = 1200, int height = 825);
    ~Compositor();

    // Surface management
    bool initialize();
    bool initializeWithRetry(int maxAttempts = 3);
    void cleanup();
    void clear();
    bool clearRegion(const LayoutRegion& region);

    // Surface access
    uint8_t* getSurfaceBuffer();
    const uint8_t* getSurfaceBuffer() const;
    int getWidth() const { return surfaceWidth; }
    int getHeight() const { return surfaceHeight; }
    size_t getSurfaceSize() const { return surfaceSize; }

    // Pixel manipulation methods (with error handling)
    bool setPixel(int x, int y, uint8_t color);
    uint8_t getPixel(int x, int y) const;
    bool drawRect(int x, int y, int w, int h, uint8_t color);
    bool fillRect(int x, int y, int w, int h, uint8_t color);

    // Change tracking
    bool markRegionChanged(const LayoutRegion& region);
    void resetChangeTracking();
    bool hasChangedRegions() const;
    std::vector<LayoutRegion> getChangedRegions() const;

    // Display output (with error handling)
    bool displayToInkplate(Inkplate& display);
    bool partialDisplayToInkplate(Inkplate& display);

    // Status and diagnostics
    bool isInitialized() const { return virtualSurface != nullptr; }
    size_t getMemoryUsage() const;

    // Error handling and recovery
    CompositorError getLastError() const { return lastError; }
    const char* getErrorString(CompositorError error) const;
    void clearError() { lastError = CompositorError::None; }
    void setError(CompositorError error); // Made public for testing
    bool isInFallbackMode() const { return fallbackMode; }
    void setFallbackMode(bool enabled) { fallbackMode = enabled; }
    bool recoverFromError();
    void setMemoryPressureThreshold(size_t threshold) { memoryPressureThreshold = threshold; }

    // Bounds checking and validation
    bool isValidRegion(const LayoutRegion& region) const;
    LayoutRegion correctInvalidRegion(const LayoutRegion& region) const;
};

#endif
