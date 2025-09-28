#include "LayoutManager.h"
#include "../core/Logger.h"
#include "../widgets/image/ImageWidget.h"
#include "../widgets/battery/BatteryWidget.h"
#include "../widgets/time/TimeWidget.h"
#include "../widgets/weather/WeatherWidget.h"
#include "../widgets/name/NameWidget.h"
#include "../widgets/layout/LayoutWidget.h"

// Helper function for C++11 compatibility (make_unique not available until C++14)
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_helper(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

LayoutManager::LayoutManager()
    : display(INKPLATE_3BIT), lastUpdate(0), layoutWidget(nullptr), compositor(nullptr), debugModeEnabled(false) {

    // Initialize config manager
    configManager = new ConfigManager();

    // Initialize compositor
    compositor = new Compositor(1200, 825); // Inkplate 10 dimensions
}

LayoutManager::~LayoutManager() {
    delete configManager;
    delete displayManager;
    delete wifiManager;
    delete layoutWidget; // Clean up global layout widget
    delete compositor;

    // regions vector will automatically clean up unique_ptrs (and regions will clean up their widgets)
}

void LayoutManager::begin() {
    Serial.begin(115200);

    // Set logger level (can be configured via config later)
    Logger::setLogLevel(LogLevel::INFO);

    LOG_INFO("LayoutManager", "Starting Inkplate Layout Manager...");

    // Initialize configuration manager first
    if (!configManager->begin()) {
        LOG_ERROR("LayoutManager", "Failed to initialize configuration manager!");
        return;
    }

    // Check if configuration is properly set up
    if (!configManager->isConfigured()) {
        LOG_ERROR("LayoutManager", "Configuration validation failed!");
        LOG_ERROR("LayoutManager", "Config error: %s", configManager->getConfigurationError().c_str());
    }

    const AppConfig& config = configManager->getConfig();

    // Enable debug mode if configured
    debugModeEnabled = config.showDebugOnScreen;

    // Debug: Check widget counts in config
    LOG_DEBUG("LayoutManager", "Config loaded - Widget counts: weather=%d, name=%d, dateTime=%d, battery=%d, image=%d, layout=%d",
              config.weatherWidgets.size(), config.nameWidgets.size(), config.dateTimeWidgets.size(),
              config.batteryWidgets.size(), config.imageWidgets.size(), config.layoutWidgets.size());

    // Calculate layout regions based on config
    calculateLayoutRegions();

    // Create display manager
    displayManager = new DisplayManager(display);

    // Enable debug mode if configured
    displayManager->enableDebugMode(debugModeEnabled);

    // Initialize and integrate compositor with display manager
    if (compositor && compositor->initialize()) {
        displayManager->setCompositor(compositor);
        LOG_INFO("LayoutManager", "Compositor initialized and integrated with DisplayManager");
    } else {
        LOG_WARN("LayoutManager", "Compositor initialization failed, falling back to direct rendering");
    }

    // Create WiFi manager with config values
    wifiManager = new WiFiManager(config.wifiSSID.c_str(), config.wifiPassword.c_str());

    // Create widgets and assign them to regions
    LOG_DEBUG("LayoutManager", "About to call createAndAssignWidgets()...");
    createAndAssignWidgets();
    LOG_DEBUG("LayoutManager", "createAndAssignWidgets() completed");

    initializeComponents();
    performInitialSetup();
}

void LayoutManager::calculateLayoutRegions() {
    const AppConfig& config = configManager->getConfig();

    LOG_DEBUG("LayoutManager", "Display dimensions: %dx%d", config.displayWidth, config.displayHeight);

    // Clear existing regions
    regions.clear();
    regionMap.clear();

    // Create regions from Layout section in config.json
    LOG_DEBUG("LayoutManager", "Creating regions from config, found %d regions", config.regions.size());

    for (const auto& regionPair : config.regions) {
        const String& regionId = regionPair.first;
        const RegionConfig& regionConfig = regionPair.second;

        LOG_DEBUG("LayoutManager", "Creating region '%s' at (%d,%d) %dx%d",
                  regionId.c_str(), regionConfig.x, regionConfig.y,
                  regionConfig.width, regionConfig.height);

        auto region = make_unique_helper<LayoutRegion>(
            regionConfig.x,
            regionConfig.y,
            regionConfig.width,
            regionConfig.height
        );

        LOG_DEBUG("LayoutManager", "LayoutRegion created successfully for '%s'", regionId.c_str());

        // Add to region map for quick access
        regionMap[regionId] = region.get();
        LOG_DEBUG("LayoutManager", "Added region '%s' to regionMap", regionId.c_str());

        // Add to regions vector
        regions.push_back(std::move(region));

        LOG_DEBUG("LayoutManager", "Added region '%s' to regions vector", regionId.c_str());
    }

    LOG_INFO("LayoutManager", "Created %d regions from configuration", regions.size());
}

void LayoutManager::createAndAssignWidgets() {
    const AppConfig& config = configManager->getConfig();

    LOG_INFO("LayoutManager", "Creating widgets and regions based on configuration...");

    // Create and assign weather widgets
    LOG_DEBUG("LayoutManager", "Creating %d weather widgets", config.weatherWidgets.size());
    for (const auto& weatherConfig : config.weatherWidgets) {
        WeatherWidget* widget = new WeatherWidget(display,
                                                 weatherConfig.latitude,
                                                 weatherConfig.longitude,
                                                 weatherConfig.city,
                                                 weatherConfig.units);

        LOG_DEBUG("LayoutManager", "Created WeatherWidget for region: %s", weatherConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(weatherConfig.region);
        if (region) {
            region->addWidget(widget);
            LOG_DEBUG("LayoutManager", "  WeatherWidget successfully assigned to region %s", weatherConfig.region.c_str());
        } else {
            LOG_ERROR("LayoutManager", "  ERROR: Failed to get region %s for WeatherWidget", weatherConfig.region.c_str());
        }
    }

    // Create and assign name widgets
    LOG_DEBUG("LayoutManager", "Creating %d name widgets", config.nameWidgets.size());
    for (const auto& nameConfig : config.nameWidgets) {
        NameWidget* widget = new NameWidget(display, nameConfig.familyName);

        LOG_DEBUG("LayoutManager", "Created NameWidget for region: %s", nameConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(nameConfig.region);
        if (region) {
            region->addWidget(widget);
            LOG_DEBUG("LayoutManager", "  NameWidget successfully assigned to region %s", nameConfig.region.c_str());
        } else {
            LOG_ERROR("LayoutManager", "  ERROR: Failed to get region %s for NameWidget", nameConfig.region.c_str());
        }
    }

    // Create and assign dateTime widgets
    LOG_DEBUG("LayoutManager", "Creating %d dateTime widgets", config.dateTimeWidgets.size());
    for (const auto& dateTimeConfig : config.dateTimeWidgets) {
        TimeWidget* widget = new TimeWidget(display, dateTimeConfig.timeUpdateMs);
        widget->begin(); // Initialize the widget immediately

        LOG_DEBUG("LayoutManager", "Created TimeWidget for region: %s", dateTimeConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(dateTimeConfig.region);
        if (region) {
            LOG_DEBUG("LayoutManager", "  TimeWidget region bounds: (%d,%d) %dx%d",
                      region->getX(), region->getY(), region->getWidth(), region->getHeight());
            region->addWidget(widget);
            LOG_DEBUG("LayoutManager", "  TimeWidget successfully assigned to region %s (region has %d widgets)",
                      dateTimeConfig.region.c_str(), region->getWidgetCount());
        } else {
            LOG_ERROR("LayoutManager", "  ERROR: Failed to get region %s for TimeWidget", dateTimeConfig.region.c_str());
        }
    }

    // Create and assign battery widgets
    LOG_DEBUG("LayoutManager", "Creating %d battery widgets", config.batteryWidgets.size());
    for (const auto& batteryConfig : config.batteryWidgets) {
        BatteryWidget* widget = new BatteryWidget(display, batteryConfig.batteryUpdateMs);
        widget->begin(); // Initialize the widget immediately

        LOG_DEBUG("LayoutManager", "Created BatteryWidget for region: %s", batteryConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(batteryConfig.region);
        if (region) {
            LOG_DEBUG("LayoutManager", "  BatteryWidget region bounds: (%d,%d) %dx%d",
                      region->getX(), region->getY(), region->getWidth(), region->getHeight());
            region->addWidget(widget);
            LOG_DEBUG("LayoutManager", "  BatteryWidget successfully assigned to region %s (region has %d widgets)",
                      batteryConfig.region.c_str(), region->getWidgetCount());
        } else {
            LOG_ERROR("LayoutManager", "  ERROR: Failed to get region %s for BatteryWidget", batteryConfig.region.c_str());
        }
    }

    // Create and assign image widgets
    LOG_DEBUG("LayoutManager", "Creating %d image widgets", config.imageWidgets.size());
    for (const auto& imageConfig : config.imageWidgets) {
        ImageWidget* widget = new ImageWidget(display, config.serverURL.c_str());

        LOG_DEBUG("LayoutManager", "Created ImageWidget for region: %s", imageConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(imageConfig.region);
        if (region) {
            region->addWidget(widget);
            LOG_DEBUG("LayoutManager", "  ImageWidget successfully assigned to region %s", imageConfig.region.c_str());
        } else {
            LOG_ERROR("LayoutManager", "  ERROR: Failed to get region %s for ImageWidget", imageConfig.region.c_str());
        }
    }

    // Create global layout widget (not assigned to any specific region)
    layoutWidget = nullptr;
    if (!config.layoutWidgets.empty()) {
        const auto& layoutConfig = config.layoutWidgets[0]; // Use first layout config
        layoutWidget = new LayoutWidget(display,
                                       layoutConfig.showRegionBorders,
                                       layoutConfig.showSeparators,
                                       layoutConfig.borderColor,
                                       layoutConfig.separatorColor,
                                       layoutConfig.borderThickness,
                                       layoutConfig.separatorThickness);

        // Give the layout widget access to all regions
        layoutWidget->setRegions(&regions);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<LayoutWidget>();
        LOG_DEBUG("LayoutManager", "  %s widget created as global layout renderer", typeName.c_str());
    }

    LOG_INFO("LayoutManager", "Widget and region creation complete");
}

LayoutRegion* LayoutManager::getRegionById(const String& regionId) const {
    auto it = regionMap.find(regionId);
    if (it != regionMap.end()) {
        return it->second;
    }
    return nullptr;
}

LayoutRegion* LayoutManager::getOrCreateRegion(const String& regionId) {
    // Check if region already exists (should exist from calculateLayoutRegions)
    LayoutRegion* existingRegion = getRegionById(regionId);
    if (existingRegion) {
        LOG_DEBUG("LayoutManager", "Using existing region %s", regionId.c_str());
        return existingRegion;
    }

    // If region doesn't exist, create it (fallback for dynamic regions)
    LOG_WARN("LayoutManager", "Region %s not found in config, creating dynamically", regionId.c_str());

    // Get region layout info from config
    RegionConfig regionConfig = configManager->getRegionConfig(regionId);

    // Create new region
    auto region = make_unique_helper<LayoutRegion>(
        regionConfig.x,
        regionConfig.y,
        regionConfig.width,
        regionConfig.height
    );

    LOG_INFO("LayoutManager", "Created dynamic region %s: %dx%d at (%d,%d)",
             regionId.c_str(),
             regionConfig.width, regionConfig.height,
             regionConfig.x, regionConfig.y);

    // Store region in map for easy access by ID
    LayoutRegion* regionPtr = region.get();
    regionMap[regionId] = regionPtr;

    // Add to regions vector
    regions.push_back(std::move(region));

    return regionPtr;
}

// Region collection management methods
size_t LayoutManager::addRegion(std::unique_ptr<LayoutRegion> region) {
    if (!region) {
        return SIZE_MAX; // Invalid index for null region
    }

    regions.push_back(std::move(region));
    return regions.size() - 1; // Return index of added region
}

bool LayoutManager::removeRegion(size_t index) {
    if (index >= regions.size()) {
        return false; // Invalid index
    }

    regions.erase(regions.begin() + index);
    return true;
}

LayoutRegion* LayoutManager::getRegion(size_t index) const {
    if (index >= regions.size()) {
        return nullptr; // Invalid index
    }

    return regions[index].get();
}

// Legacy region getters removed - LayoutManager is now fully data-driven

void LayoutManager::initializeComponents() {
    LOG_INFO("LayoutManager", "Initializing components...");

    displayManager->initialize();

    // Initialize widgets in all regions
    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region) {
            region->initializeWidgets();
        }
    }

    // Initialize global layout widget
    if (layoutWidget) {
        layoutWidget->begin();
    }

    LOG_INFO("LayoutManager", "All components and widgets initialized");
}

void LayoutManager::loop() {
    // In deep sleep mode, most work is done in setup()
    // This loop only handles immediate updates and prepares for sleep

    // Check for immediate widget updates (like time ticking)
    handleImmediateUpdates();

    // Check if we should enter deep sleep
    checkDeepSleepConditions();
}

void LayoutManager::performInitialSetup() {
    // Clear screen at startup to remove any previous status messages
    if (!debugModeEnabled) {
        displayManager->clear();
    }

    if (debugModeEnabled) {
        displayManager->showStatus("Initializing...");
    }

    // Check configuration before attempting WiFi connection
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        LOG_ERROR("LayoutManager", "Configuration error: %s", errorMsg.c_str());
        LOG_ERROR("LayoutManager", "Configuration error - widgets should handle error display");
        return;
    }

    // Perform all updates in setup for deep sleep optimization
    performScheduledUpdates();

    lastUpdate = millis();
}

void LayoutManager::performScheduledUpdates() {
    LOG_INFO("LayoutManager", "Performing scheduled updates in setup...");

    if (debugModeEnabled) {
        displayManager->showDebugMessage("Starting scheduled updates...");
    }

    if (wifiManager->connect()) {
        LOG_INFO("LayoutManager", "WiFi connected, performing full update");

        if (debugModeEnabled) {
            displayManager->showStatus("Connected", "WiFi", wifiManager->getIPAddress().c_str());
            displayManager->showDebugMessage(("WiFi: " + wifiManager->getIPAddress()).c_str());
        } else {
            // Clear any previous status messages when not in debug mode
            displayManager->clear();
        }

        // Force connectivity check and widget updates
        if (ensureConnectivity()) {
            LOG_INFO("LayoutManager", "Connectivity ensured - updating all widgets");

            if (debugModeEnabled) {
                displayManager->showDebugMessage("Updating widgets...");
            }

            // Force all widgets to update their data
            forceWidgetDataUpdate();

            // Render all regions with fresh data
            renderAllRegions();

            if (debugModeEnabled) {
                displayManager->showDebugMessage("Update complete");
            }
        }
    } else {
        LOG_ERROR("LayoutManager", "WiFi connection failed - rendering with cached data");

        if (debugModeEnabled) {
            displayManager->showDebugMessage("WiFi failed - using cache");
        }

        // Clear any status messages and render widgets directly
        if (!debugModeEnabled) {
            displayManager->clear();
        }

        renderAllRegions();
    }
}

void LayoutManager::forceWidgetDataUpdate() {
    LOG_INFO("LayoutManager", "Forcing widget data updates...");

    // Update all widgets in all regions
    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region) {
            // Force update for all widgets in this region
            for (size_t i = 0; i < region->getWidgetCount(); ++i) {
                Widget* widget = region->getWidget(i);
                if (widget) {
                    widget->forceUpdate(); // This will trigger data refresh
                }
            }

            // Handle legacy widget if present
            if (region->getLegacyWidget()) {
                region->getLegacyWidget()->forceUpdate();
            }
        }
    }
}

void LayoutManager::handleImmediateUpdates() {
    // Handle only time-sensitive updates that can't wait for deep sleep cycle
    bool needsImmediateRender = false;

    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region) {
            for (size_t i = 0; i < region->getWidgetCount(); ++i) {
                Widget* widget = region->getWidget(i);
                if (widget && widget->needsImmediateUpdate()) {
                    widget->update(); // Quick update for time-sensitive widgets
                    needsImmediateRender = true;
                }
            }
        }
    }

    if (needsImmediateRender) {
        LOG_DEBUG("LayoutManager", "Performing immediate render for time-sensitive updates");
        renderChangedRegions();
    }
}

void LayoutManager::checkDeepSleepConditions() {
    // This method determines if we should prepare for deep sleep
    unsigned long currentTime = millis();
    unsigned long timeSinceLastUpdate = currentTime - lastUpdate;

    // Check if it's time for the next scheduled update
    if (timeSinceLastUpdate >= getShortestUpdateInterval()) {
        LOG_INFO("LayoutManager", "Time for next scheduled update - preparing for deep sleep wake");

        if (debugModeEnabled) {
            displayManager->showDebugMessage("Preparing for deep sleep...", true);
        }

        prepareForDeepSleep();
    }
}

void LayoutManager::prepareForDeepSleep() {
    LOG_INFO("LayoutManager", "Preparing system for deep sleep...");

    // Ensure all pending operations are complete
    displayManager->update();

    // Save any necessary state
    // (Most state is preserved in config or can be reconstructed)

    LOG_INFO("LayoutManager", "System ready for deep sleep");
}

void LayoutManager::handleWidgetUpdates() {
    // Check if any regions need updates
    bool needsUpdate = false;

    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region && region->needsUpdate()) {
            needsUpdate = true;
            break;
        }
    }

    // Only update display if regions actually need updating
    if (needsUpdate) {
        LOG_DEBUG("LayoutManager", "Rendering updated regions...");
        renderChangedRegions(); // Use partial update for better performance
    }
}

bool LayoutManager::ensureConnectivity() {
    // First check if configuration is valid
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        LOG_ERROR("LayoutManager", "Configuration error during connectivity check: %s", errorMsg.c_str());

        // Note: Error display would be handled by widgets in their respective regions
        LOG_ERROR("LayoutManager", "Configuration error - widgets should handle error display");
        return false;
    }

    if (!wifiManager->isConnected()) {
        LOG_WARN("LayoutManager", "WiFi disconnected, attempting reconnection...");

        if (debugModeEnabled) {
            displayManager->showStatus("Reconnecting WiFi...");
        }

        if (!wifiManager->connect()) {
            LOG_ERROR("LayoutManager", "WiFi reconnection failed - widgets should handle error display");
            return false;
        } else {
            LOG_INFO("LayoutManager", "WiFi reconnected - widgets can now sync data");
        }
    }
    return true;
}



void LayoutManager::renderAllRegions() {
    LOG_DEBUG("LayoutManager", "Rendering all regions...");

    // Use compositor if available, otherwise fall back to direct rendering
    if (compositor && compositor->isInitialized() && displayManager->getCompositor() && !compositor->isInFallbackMode()) {
        LOG_DEBUG("LayoutManager", "Using compositor for rendering all regions");

        // Clear compositor surface
        compositor->clear();

        bool compositorRenderingSuccessful = true;

        // Render each region to compositor with error handling
        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region) {
                LOG_DEBUG("LayoutManager", "Rendering region at (%d,%d) %dx%d with %d widgets to compositor",
                          region->getX(), region->getY(),
                          region->getWidth(), region->getHeight(),
                          region->getWidgetCount());

                // Clear region on compositor with error checking
                LOG_DEBUG("LayoutManager", "Clearing region (%d,%d) %dx%d on compositor",
                          region->getX(), region->getY(),
                          region->getWidth(), region->getHeight());
                if (!compositor->clearRegion(*region)) {
                    LOG_ERROR("LayoutManager", "Failed to clear region on compositor, error: %s",
                              compositor->getErrorString(compositor->getLastError()));
                    compositorRenderingSuccessful = false;
                    continue;
                }

                // Render all widgets in the region to compositor with error isolation
                for (size_t i = 0; i < region->getWidgetCount(); ++i) {
                    Widget* widget = region->getWidget(i);
                    if (widget) {
                        try {
                            widget->renderToCompositor(*compositor, *region);
                        } catch (...) {
                            LOG_ERROR("LayoutManager", "Widget rendering failed for widget %zu in region (%d,%d)",
                                      i, region->getX(), region->getY());
                            // Continue with other widgets
                        }
                    }
                }

                // Handle legacy widget if present with error isolation
                if (region->getLegacyWidget()) {
                    try {
                        region->getLegacyWidget()->renderToCompositor(*compositor, *region);
                    } catch (...) {
                        LOG_ERROR("LayoutManager", "Legacy widget rendering failed in region (%d,%d)",
                                  region->getX(), region->getY());
                        // Continue with other regions
                    }
                }

                // Mark region as clean after rendering
                region->markClean();
            }
        }

        // Render global layout elements (borders, separators) to compositor with error handling
        if (layoutWidget) {
            try {
                LayoutRegion fullDisplayRegion(0, 0, display.width(), display.height());
                layoutWidget->renderToCompositor(*compositor, fullDisplayRegion);
            } catch (...) {
                LOG_ERROR("LayoutManager", "Layout widget rendering failed");
                compositorRenderingSuccessful = false;
            }
        }

        // Display compositor content to Inkplate with error handling
        if (compositorRenderingSuccessful) {
            if (!displayManager->renderWithCompositor()) {
                LOG_WARN("LayoutManager", "Compositor display failed, falling back to direct rendering");
                compositor->setFallbackMode(true);
                renderAllRegions(); // Retry with direct rendering
                return;
            }
        } else {
            LOG_WARN("LayoutManager", "Compositor rendering had errors, attempting recovery");
            if (compositor->recoverFromError()) {
                LOG_INFO("LayoutManager", "Compositor recovery successful");
            } else {
                LOG_ERROR("LayoutManager", "Compositor recovery failed, enabling fallback mode");
                compositor->setFallbackMode(true);
                renderAllRegions(); // Retry with direct rendering
                return;
            }
        }
    } else {
        if (compositor && compositor->isInFallbackMode()) {
            LOG_DEBUG("LayoutManager", "Using direct rendering (compositor in fallback mode)");
        } else {
            LOG_DEBUG("LayoutManager", "Using direct rendering (compositor not available)");
        }

        // Fall back to direct rendering with error isolation
        bool directRenderingSuccessful = true;

        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region) {
                LOG_DEBUG("LayoutManager", "Region at (%d,%d) %dx%d has %d widgets, needsUpdate: %s",
                          region->getX(), region->getY(),
                          region->getWidth(), region->getHeight(),
                          region->getWidgetCount(),
                          region->needsUpdate() ? "true" : "false");

                if (region->needsUpdate()) {
                    LOG_DEBUG("LayoutManager", "  Region needs update - rendering");

                    try {
                        // Let the region render all its widgets with error isolation
                        region->render();
                        LOG_DEBUG("LayoutManager", "  Region rendering complete");
                    } catch (...) {
                        LOG_ERROR("LayoutManager", "  ERROR: Region rendering failed for region (%d,%d)",
                                  region->getX(), region->getY());
                        directRenderingSuccessful = false;
                        // Continue with other regions
                    }
                } else {
                    LOG_DEBUG("LayoutManager", "  Region does not need update - skipping");
                }
            }
        }

        // Render global layout elements (borders, separators) after all regions with error handling
        if (layoutWidget) {
            try {
                LayoutRegion fullDisplayRegion(0, 0, display.width(), display.height());
                layoutWidget->render(fullDisplayRegion);
            } catch (...) {
                LOG_ERROR("LayoutManager", "ERROR: Layout widget rendering failed in direct mode");
                directRenderingSuccessful = false;
            }
        }

        // Single display update for the entire layout with error handling
        try {
            displayManager->update();
            if (directRenderingSuccessful) {
                LOG_DEBUG("LayoutManager", "Direct rendering completed successfully");
            } else {
                LOG_WARN("LayoutManager", "Direct rendering completed with some errors");
            }
        } catch (...) {
            LOG_ERROR("LayoutManager", "ERROR: Display update failed in direct rendering mode");
        }
    }

    LOG_DEBUG("LayoutManager", "Region rendering complete");
}

void LayoutManager::renderChangedRegions() {
    LOG_DEBUG("LayoutManager", "Rendering changed regions...");

    // Use compositor if available for efficient partial updates
    if (compositor && compositor->isInitialized() && displayManager->getCompositor() && !compositor->isInFallbackMode()) {
        LOG_DEBUG("LayoutManager", "Using compositor for partial region rendering");

        bool hasChanges = false;
        bool compositorRenderingSuccessful = true;

        // Check which regions need updates and render them to compositor with error handling
        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region && region->needsUpdate()) {
                LOG_DEBUG("LayoutManager", "Rendering changed region at (%d,%d) %dx%d with %d widgets to compositor",
                          region->getX(), region->getY(),
                          region->getWidth(), region->getHeight(),
                          region->getWidgetCount());

                // Clear region on compositor with error checking
                if (!compositor->clearRegion(*region)) {
                    LOG_ERROR("LayoutManager", "Failed to clear changed region on compositor, error: %s",
                              compositor->getErrorString(compositor->getLastError()));
                    compositorRenderingSuccessful = false;
                    continue;
                }

                // Render all widgets in the region to compositor with error isolation
                for (size_t i = 0; i < region->getWidgetCount(); ++i) {
                    Widget* widget = region->getWidget(i);
                    if (widget) {
                        try {
                            widget->renderToCompositor(*compositor, *region);
                        } catch (...) {
                            LOG_ERROR("LayoutManager", "Widget rendering failed for widget %zu in changed region (%d,%d)",
                                      i, region->getX(), region->getY());
                            // Continue with other widgets
                        }
                    }
                }

                // Handle legacy widget if present with error isolation
                if (region->getLegacyWidget()) {
                    try {
                        region->getLegacyWidget()->renderToCompositor(*compositor, *region);
                    } catch (...) {
                        LOG_ERROR("LayoutManager", "Legacy widget rendering failed in changed region (%d,%d)",
                                  region->getX(), region->getY());
                        // Continue with other regions
                    }
                }

                // Mark region as clean after rendering
                region->markClean();
                hasChanges = true;
            }
        }

        // Only update display if there were actual changes and no critical errors
        if (hasChanges && compositorRenderingSuccessful) {
            LOG_DEBUG("LayoutManager", "Changes detected, performing partial display update");
            if (!displayManager->partialRenderWithCompositor()) {
                LOG_WARN("LayoutManager", "Partial compositor display failed, falling back to full direct rendering");
                compositor->setFallbackMode(true);
                renderAllRegions(); // Fall back to full direct rendering
                return;
            }
        } else if (hasChanges && !compositorRenderingSuccessful) {
            LOG_WARN("LayoutManager", "Compositor rendering had errors during partial update, attempting recovery");
            if (compositor->recoverFromError()) {
                LOG_INFO("LayoutManager", "Compositor recovery successful, retrying partial update");
                renderChangedRegions(); // Retry
                return;
            } else {
                LOG_ERROR("LayoutManager", "Compositor recovery failed, falling back to direct rendering");
                compositor->setFallbackMode(true);
                renderAllRegions(); // Fall back to full direct rendering
                return;
            }
        } else {
            LOG_DEBUG("LayoutManager", "No changes detected, skipping display update");
        }
    } else {
        LOG_DEBUG("LayoutManager", "Using direct rendering for changed regions (compositor not available)");

        // Fall back to direct rendering with smart partial update
        bool hasChanges = false;

        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region && region->needsUpdate()) {
                LOG_DEBUG("LayoutManager", "Rendering changed region at (%d,%d) %dx%d with %d widgets",
                          region->getX(), region->getY(),
                          region->getWidth(), region->getHeight(),
                          region->getWidgetCount());

                // Let the region render all its widgets
                region->render();
                hasChanges = true;
            }
        }

        // Only update display if there were actual changes
        if (hasChanges) {
            LOG_DEBUG("LayoutManager", "Changes detected, performing smart partial update");
            displayManager->smartPartialUpdate();
        } else {
            LOG_DEBUG("LayoutManager", "No changes detected, skipping display update");
        }
    }

    LOG_DEBUG("LayoutManager", "Changed region rendering complete");
}

void LayoutManager::clearRegion(const LayoutRegion& region) {
    // Clear region with white background
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 7);
}

// drawLayoutBorders() method removed - layout visualization now handled by LayoutWidget

void LayoutManager::forceRefresh() {
    LOG_INFO("LayoutManager", "Manual layout refresh triggered by WAKE button");

    if (ensureConnectivity()) {
        LOG_INFO("LayoutManager", "Connectivity ensured - forcing region refresh");

        // Mark all regions as dirty to force refresh
        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region) {
                region->markDirty();
            }
        }

        // Use compositor-based rendering for full refresh
        renderAllRegions();

        // Update the last update time to reset the scheduled timer
        lastUpdate = millis();
    } else {
        LOG_ERROR("LayoutManager", "Cannot refresh layout - no connectivity");
    }
}

void LayoutManager::forceTimeAndBatteryUpdate() {
    LOG_INFO("LayoutManager", "Forcing time and battery widget updates with compositor partial rendering");

    bool hasTimeOrBatteryUpdates = false;

    // Find and mark only time and battery widget regions as dirty
    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region) {
            bool regionHasTimeOrBattery = false;

            // Check if region contains time or battery widgets
            for (size_t i = 0; i < region->getWidgetCount(); ++i) {
                Widget* widget = region->getWidget(i);
                if (widget) {
                    // Check widget type using getWidgetType method
                    WidgetType widgetType = widget->getWidgetType();

                    if (widgetType == WidgetType::DATE_TIME || widgetType == WidgetType::BATTERY) {
                        regionHasTimeOrBattery = true;
                        hasTimeOrBatteryUpdates = true;
                        LOG_DEBUG("LayoutManager", "Found %s widget in region (%d,%d)",
                                  widgetType == WidgetType::DATE_TIME ? "time" : "battery",
                                  region->getX(), region->getY());
                        break;
                    }
                }
            }

            // Also check legacy widget
            if (!regionHasTimeOrBattery && region->getLegacyWidget()) {
                WidgetType widgetType = region->getLegacyWidget()->getWidgetType();

                if (widgetType == WidgetType::DATE_TIME || widgetType == WidgetType::BATTERY) {
                    regionHasTimeOrBattery = true;
                    hasTimeOrBatteryUpdates = true;
                    LOG_DEBUG("LayoutManager", "Found legacy %s widget in region (%d,%d)",
                              widgetType == WidgetType::DATE_TIME ? "time" : "battery",
                              region->getX(), region->getY());
                }
            }

            // Mark region as dirty if it contains time or battery widgets
            if (regionHasTimeOrBattery) {
                region->markDirty();
                LOG_DEBUG("LayoutManager", "Marked region (%d,%d) as dirty for time/battery update",
                          region->getX(), region->getY());
            }
        }
    }

    if (hasTimeOrBatteryUpdates) {
        LOG_INFO("LayoutManager", "Time/battery widgets found - performing partial compositor update");

        // Use compositor partial rendering for efficient updates
        if (compositor && compositor->isInitialized() && displayManager->getCompositor() && !compositor->isInFallbackMode()) {
            LOG_DEBUG("LayoutManager", "Using compositor for partial time/battery update");
            renderChangedRegions(); // This will use compositor partial rendering
        } else {
            LOG_DEBUG("LayoutManager", "Compositor not available - using direct partial rendering for time/battery update");
            renderChangedRegions(); // This will fall back to direct rendering
        }
    } else {
        LOG_DEBUG("LayoutManager", "No time or battery widgets found - skipping update");
    }
}



unsigned long LayoutManager::getShortestUpdateInterval() const {
    if (!configManager) return 3600000; // Default 1 hour if no config

    const AppConfig& config = configManager->getConfig();

    // Find the shortest update interval among all widgets
    unsigned long shortest = 3600000UL; // Default 1 hour

    for (const auto& imageConfig : config.imageWidgets) {
        shortest = min(shortest, imageConfig.imageRefreshMs);
    }

    for (const auto& dateTimeConfig : config.dateTimeWidgets) {
        shortest = min(shortest, dateTimeConfig.timeUpdateMs);
    }

    for (const auto& batteryConfig : config.batteryWidgets) {
        shortest = min(shortest, batteryConfig.batteryUpdateMs);
    }

    return shortest;
}

int LayoutManager::getWakeButtonPin() const {
    if (!configManager) return 36; // Default pin if no config

    const AppConfig& config = configManager->getConfig();
    return config.wakeButtonPin;
}

bool LayoutManager::shouldEnterDeepSleep() const {
    if (!configManager) return false; // Don't sleep if no config

    const AppConfig& config = configManager->getConfig();
    return config.enableDeepSleep;
}

unsigned long LayoutManager::getDeepSleepThreshold() const {
    if (!configManager) return 600000UL; // Default 10 minutes if no config

    const AppConfig& config = configManager->getConfig();
    return config.deepSleepThresholdMs;
}

void LayoutManager::demonstrateCompositorIntegration() {
    LOG_INFO("LayoutManager", "=== COMPOSITOR INTEGRATION DEMONSTRATION ===");

    if (!compositor || !compositor->isInitialized()) {
        LOG_WARN("LayoutManager", "Compositor not available - demonstration skipped");
        return;
    }

    if (!displayManager) {
        LOG_WARN("LayoutManager", "DisplayManager not available - demonstration skipped");
        return;
    }

    LOG_INFO("LayoutManager", "Drawing test pattern on compositor...");

    // Clear compositor surface
    compositor->clear();

    // Draw a test pattern to demonstrate compositor functionality
    // Draw border around entire surface
    compositor->drawRect(0, 0, 1200, 825, 0); // Black border

    // Draw some test rectangles
    compositor->fillRect(50, 50, 200, 100, 128);   // Gray rectangle
    compositor->fillRect(300, 50, 200, 100, 64);   // Darker gray rectangle
    compositor->fillRect(550, 50, 200, 100, 192);  // Light gray rectangle

    // Draw text area simulation (just rectangles for now)
    compositor->drawRect(50, 200, 700, 50, 0);     // Text area border
    compositor->fillRect(52, 202, 696, 46, 255);   // White text background

    // Draw some "widget" areas
    compositor->drawRect(50, 300, 150, 150, 0);    // Widget 1 border
    compositor->fillRect(52, 302, 146, 146, 224);  // Light background

    compositor->drawRect(250, 300, 150, 150, 0);   // Widget 2 border
    compositor->fillRect(252, 302, 146, 146, 160); // Medium background

    compositor->drawRect(450, 300, 150, 150, 0);   // Widget 3 border
    compositor->fillRect(452, 302, 146, 146, 96);  // Darker background

    LOG_INFO("LayoutManager", "Test pattern drawn on compositor surface");

    // Demonstrate full rendering with compositor
    LOG_INFO("LayoutManager", "Performing full render with compositor...");
    displayManager->renderWithCompositor();

    delay(3000); // Show the pattern for 3 seconds

    // Demonstrate partial update
    LOG_INFO("LayoutManager", "Modifying small area for partial update demonstration...");

    // Modify a small area
    compositor->fillRect(600, 300, 100, 100, 32); // Dark area
    compositor->drawRect(600, 300, 100, 100, 0);  // Border

    LOG_INFO("LayoutManager", "Performing partial render with compositor...");
    displayManager->partialRenderWithCompositor();

    delay(2000);

    // Clear and return to normal operation
    LOG_INFO("LayoutManager", "Clearing compositor and returning to normal operation...");
    compositor->clear();
    displayManager->renderWithCompositor();

    LOG_INFO("LayoutManager", "=== COMPOSITOR DEMONSTRATION COMPLETE ===");
}

bool LayoutManager::assignWidgetToRegion(Widget* widget, const String& regionId) {
    if (!widget) {
        LOG_ERROR("LayoutManager", "ERROR: Cannot assign null widget to region '%s'", regionId.c_str());
        return false;
    }

    LayoutRegion* region = getRegionById(regionId);
    if (!region) {
        LOG_ERROR("LayoutManager", "ERROR: Region '%s' not found for widget assignment", regionId.c_str());
        return false;
    }

    // Add widget to region
    size_t index = region->addWidget(widget);
    if (index == SIZE_MAX) {
        LOG_ERROR("LayoutManager", "ERROR: Failed to add widget to region '%s'", regionId.c_str());
        return false;
    }

    LOG_DEBUG("LayoutManager", "Successfully assigned widget to region '%s' (index %d)", regionId.c_str(), index);

    // Initialize the widget if it hasn't been initialized yet
    widget->begin();

    // Mark region as dirty to trigger re-render
    region->markDirty();

    return true;
}

bool LayoutManager::removeWidgetFromRegion(Widget* widget, const String& regionId) {
    if (!widget) {
        LOG_ERROR("LayoutManager", "ERROR: Cannot remove null widget from region '%s'", regionId.c_str());
        return false;
    }

    LayoutRegion* region = getRegionById(regionId);
    if (!region) {
        LOG_ERROR("LayoutManager", "ERROR: Region '%s' not found for widget removal", regionId.c_str());
        return false;
    }

    // Find and remove the widget from the region
    for (size_t i = 0; i < region->getWidgetCount(); ++i) {
        if (region->getWidget(i) == widget) {
            if (region->removeWidget(i)) {
                LOG_DEBUG("LayoutManager", "Successfully removed widget from region '%s' (was at index %d)", regionId.c_str(), i);

                // Mark region as dirty to trigger re-render
                region->markDirty();

                return true;
            } else {
                LOG_ERROR("LayoutManager", "ERROR: Failed to remove widget from region '%s' at index %d", regionId.c_str(), i);
                return false;
            }
        }
    }

    LOG_ERROR("LayoutManager", "ERROR: Widget not found in region '%s'", regionId.c_str());
    return false;
}
