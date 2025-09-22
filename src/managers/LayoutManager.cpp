#include "LayoutManager.h"
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
    : display(INKPLATE_3BIT), lastUpdate(0), layoutWidget(nullptr) {

    // Initialize config manager
    configManager = new ConfigManager();
}

LayoutManager::~LayoutManager() {
    delete configManager;
    delete displayManager;
    delete wifiManager;
    delete layoutWidget; // Clean up global layout widget

    // regions vector will automatically clean up unique_ptrs (and regions will clean up their widgets)
}

void LayoutManager::begin() {
    Serial.begin(115200);
    Serial.println("Starting Inkplate Layout Manager...");

    // Initialize configuration manager first
    if (!configManager->begin()) {
        Serial.println("Failed to initialize configuration manager!");
        return;
    }

    // Check if configuration is properly set up
    if (!configManager->isConfigured()) {
        Serial.println("Configuration validation failed!");
        Serial.println(configManager->getConfigurationError());
    }

    const AppConfig& config = configManager->getConfig();

    // Debug: Check widget counts in config
    Serial.printf("Config loaded - Widget counts: weather=%d, name=%d, dateTime=%d, battery=%d, image=%d, layout=%d\n",
                  config.weatherWidgets.size(), config.nameWidgets.size(), config.dateTimeWidgets.size(),
                  config.batteryWidgets.size(), config.imageWidgets.size(), config.layoutWidgets.size());

    // Calculate layout regions based on config
    calculateLayoutRegions();

    // Create display manager
    displayManager = new DisplayManager(display);

    // Create WiFi manager with config values
    wifiManager = new WiFiManager(config.wifiSSID.c_str(), config.wifiPassword.c_str());

    // Create widgets and assign them to regions
    Serial.println("About to call createAndAssignWidgets()...");
    createAndAssignWidgets();
    Serial.println("createAndAssignWidgets() completed");

    initializeComponents();
    performInitialSetup();
}

void LayoutManager::calculateLayoutRegions() {
    const AppConfig& config = configManager->getConfig();

    Serial.printf("Display dimensions: %dx%d\n", config.displayWidth, config.displayHeight);

    // Clear existing regions
    regions.clear();
    regionMap.clear();

    // Create regions from Layout section in config.json
    Serial.printf("Creating regions from config, found %d regions\n", config.regions.size());

    for (const auto& regionPair : config.regions) {
        const String& regionId = regionPair.first;
        const RegionConfig& regionConfig = regionPair.second;

        Serial.printf("Creating region '%s' at (%d,%d) %dx%d\n",
                     regionId.c_str(), regionConfig.x, regionConfig.y,
                     regionConfig.width, regionConfig.height);

        auto region = make_unique_helper<LayoutRegion>(
            regionConfig.x,
            regionConfig.y,
            regionConfig.width,
            regionConfig.height
        );

        Serial.printf("LayoutRegion created successfully for '%s'\n", regionId.c_str());

        // Add to region map for quick access
        regionMap[regionId] = region.get();
        Serial.printf("Added region '%s' to regionMap\n", regionId.c_str());

        // Add to regions vector
        regions.push_back(std::move(region));

        Serial.printf("Added region '%s' to regions vector\n", regionId.c_str());
    }

    Serial.printf("Created %d regions from configuration\n", regions.size());
}

void LayoutManager::createAndAssignWidgets() {
    const AppConfig& config = configManager->getConfig();

    Serial.println("Creating widgets and regions based on configuration...");

    // Create and assign weather widgets
    Serial.printf("Creating %d weather widgets\n", config.weatherWidgets.size());
    for (const auto& weatherConfig : config.weatherWidgets) {
        WeatherWidget* widget = new WeatherWidget(display,
                                                 weatherConfig.latitude,
                                                 weatherConfig.longitude,
                                                 weatherConfig.city,
                                                 weatherConfig.units);

        Serial.printf("Created WeatherWidget for region: %s\n", weatherConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(weatherConfig.region);
        if (region) {
            region->addWidget(widget);
            Serial.printf("  WeatherWidget successfully assigned to region %s\n", weatherConfig.region.c_str());
        } else {
            Serial.printf("  ERROR: Failed to get region %s for WeatherWidget\n", weatherConfig.region.c_str());
        }
    }

    // Create and assign name widgets
    Serial.printf("Creating %d name widgets\n", config.nameWidgets.size());
    for (const auto& nameConfig : config.nameWidgets) {
        NameWidget* widget = new NameWidget(display, nameConfig.familyName);

        Serial.printf("Created NameWidget for region: %s\n", nameConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(nameConfig.region);
        if (region) {
            region->addWidget(widget);
            Serial.printf("  NameWidget successfully assigned to region %s\n", nameConfig.region.c_str());
        } else {
            Serial.printf("  ERROR: Failed to get region %s for NameWidget\n", nameConfig.region.c_str());
        }
    }

    // Create and assign dateTime widgets
    Serial.printf("Creating %d dateTime widgets\n", config.dateTimeWidgets.size());
    for (const auto& dateTimeConfig : config.dateTimeWidgets) {
        TimeWidget* widget = new TimeWidget(display, dateTimeConfig.timeUpdateMs);
        widget->begin(); // Initialize the widget immediately

        Serial.printf("Created TimeWidget for region: %s\n", dateTimeConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(dateTimeConfig.region);
        if (region) {
            region->addWidget(widget);
            Serial.printf("  TimeWidget successfully assigned to region %s (region has %d widgets)\n",
                         dateTimeConfig.region.c_str(), region->getWidgetCount());
        } else {
            Serial.printf("  ERROR: Failed to get region %s for TimeWidget\n", dateTimeConfig.region.c_str());
        }
    }

    // Create and assign battery widgets
    Serial.printf("Creating %d battery widgets\n", config.batteryWidgets.size());
    for (const auto& batteryConfig : config.batteryWidgets) {
        BatteryWidget* widget = new BatteryWidget(display, batteryConfig.batteryUpdateMs);
        widget->begin(); // Initialize the widget immediately

        Serial.printf("Created BatteryWidget for region: %s\n", batteryConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(batteryConfig.region);
        if (region) {
            region->addWidget(widget);
            Serial.printf("  BatteryWidget successfully assigned to region %s (region has %d widgets)\n",
                         batteryConfig.region.c_str(), region->getWidgetCount());
        } else {
            Serial.printf("  ERROR: Failed to get region %s for BatteryWidget\n", batteryConfig.region.c_str());
        }
    }

    // Create and assign image widgets
    Serial.printf("Creating %d image widgets\n", config.imageWidgets.size());
    for (const auto& imageConfig : config.imageWidgets) {
        ImageWidget* widget = new ImageWidget(display, config.serverURL.c_str());

        Serial.printf("Created ImageWidget for region: %s\n", imageConfig.region.c_str());
        LayoutRegion* region = getOrCreateRegion(imageConfig.region);
        if (region) {
            region->addWidget(widget);
            Serial.printf("  ImageWidget successfully assigned to region %s\n", imageConfig.region.c_str());
        } else {
            Serial.printf("  ERROR: Failed to get region %s for ImageWidget\n", imageConfig.region.c_str());
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
        Serial.printf("  %s widget created as global layout renderer\n", typeName.c_str());
    }

    Serial.println("Widget and region creation complete");
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
        Serial.printf("Using existing region %s\n", regionId.c_str());
        return existingRegion;
    }

    // If region doesn't exist, create it (fallback for dynamic regions)
    Serial.printf("Region %s not found in config, creating dynamically\n", regionId.c_str());

    // Get region layout info from config
    RegionConfig regionConfig = configManager->getRegionConfig(regionId);

    // Create new region
    auto region = make_unique_helper<LayoutRegion>(
        regionConfig.x,
        regionConfig.y,
        regionConfig.width,
        regionConfig.height
    );

    Serial.printf("Created dynamic region %s: %dx%d at (%d,%d)\n",
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
    Serial.println("Initializing components...");

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

    Serial.println("All components and widgets initialized");
}

void LayoutManager::loop() {
    wifiManager->checkConnection();
    handleScheduledUpdate();
    handleWidgetUpdates();
}

void LayoutManager::performInitialSetup() {
    displayManager->showStatus("Initializing...");

    // Check configuration before attempting WiFi connection
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        Serial.printf("Configuration error: %s\n", errorMsg.c_str());

        // Note: Error display would be handled by widgets in their respective regions
        Serial.println("Configuration error - widgets should handle error display");
        return;
    }

    if (wifiManager->connect()) {
        Serial.println("Initial setup complete");
        displayManager->showStatus("Connected", "WiFi", wifiManager->getIPAddress().c_str());

        Serial.println("WiFi connected, syncing time and weather...");
        // Note: Widget syncing will be handled by the widgets themselves during render

        // Render all regions (each region will render its own widgets)
        renderAllRegions();
    } else {
        Serial.println("WiFi connection failed - widgets should handle error display");
        renderAllRegions();
    }

    lastUpdate = millis();
}

void LayoutManager::handleScheduledUpdate() {
    // Automatic updates every 24 hours + manual refresh via WAKE button
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= getShortestUpdateInterval()) {
        Serial.println("Starting scheduled update...");

        if (ensureConnectivity()) {
            Serial.println("Connectivity ensured - triggering region updates");

            // Force refresh of weather and time data during scheduled update
            // Note: Widget syncing will be handled by the widgets themselves during render

            // Render all regions (each region will handle its own widget updates)
            renderAllRegions();
        }

        lastUpdate = currentTime;
    }
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
        Serial.println("Rendering updated regions...");
        renderAllRegions();
    }
}

bool LayoutManager::ensureConnectivity() {
    // First check if configuration is valid
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        Serial.printf("Configuration error during connectivity check: %s\n", errorMsg.c_str());

        // Note: Error display would be handled by widgets in their respective regions
        Serial.println("Configuration error - widgets should handle error display");
        return false;
    }

    if (!wifiManager->isConnected()) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        displayManager->showStatus("Reconnecting WiFi...");

        if (!wifiManager->connect()) {
            Serial.println("WiFi reconnection failed - widgets should handle error display");
            return false;
        } else {
            Serial.println("WiFi reconnected - widgets can now sync data");
        }
    }
    return true;
}



void LayoutManager::renderAllRegions() {
    Serial.println("Rendering all regions...");

    // Each region is responsible for rendering its own widgets
    for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
        LayoutRegion* region = it->get();
        if (region) {
            Serial.printf("Region at (%d,%d) %dx%d has %d widgets, needsUpdate: %s\n",
                         region->getX(), region->getY(),
                         region->getWidth(), region->getHeight(),
                         region->getWidgetCount(),
                         region->needsUpdate() ? "true" : "false");

            if (region->needsUpdate()) {
                Serial.printf("  Region needs update - rendering WITHOUT clearing\n");
                // TEMPORARILY DISABLE CLEARING TO TEST
                // clearRegion(*region);

                // Let the region render all its widgets
                region->render();
                Serial.printf("  Region rendering complete\n");
            } else {
                Serial.printf("  Region does not need update - skipping\n");
            }
        }
    }

    // Render global layout elements (borders, separators) after all regions
    if (layoutWidget) {
        // Create a dummy region that covers the entire display for layout widget
        LayoutRegion fullDisplayRegion(0, 0, display.width(), display.height());
        layoutWidget->render(fullDisplayRegion);
    }

    // Single display update for the entire layout
    displayManager->update();

    Serial.println("Region rendering complete");
}

void LayoutManager::clearRegion(const LayoutRegion& region) {
    // Clear region with white background
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 7);
}

// drawLayoutBorders() method removed - layout visualization now handled by LayoutWidget

void LayoutManager::forceRefresh() {
    Serial.println("Manual layout refresh triggered by WAKE button");

    if (ensureConnectivity()) {
        Serial.println("Connectivity ensured - forcing region refresh");

        // Mark all regions as dirty to force refresh
        for (auto it = regionsBegin(); it != regionsEnd(); ++it) {
            LayoutRegion* region = it->get();
            if (region) {
                region->markDirty();
            }
        }

        // Render all regions
        renderAllRegions();

        // Update the last update time to reset the scheduled timer
        lastUpdate = millis();
    } else {
        Serial.println("Cannot refresh layout - no connectivity");
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
