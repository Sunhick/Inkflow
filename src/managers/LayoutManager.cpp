#include "LayoutManager.h"
#include "../widgets/image/ImageWidget.h"
#include "../widgets/battery/BatteryWidget.h"
#include "../widgets/time/TimeWidget.h"
#include "../widgets/weather/WeatherWidget.h"
#include "../widgets/name/NameWidget.h"

// Helper function for C++11 compatibility (make_unique not available until C++14)
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_helper(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

LayoutManager::LayoutManager()
    : display(INKPLATE_3BIT), lastUpdate(0) {

    // Initialize config manager
    configManager = new ConfigManager();
}

LayoutManager::~LayoutManager() {
    delete configManager;
    delete displayManager;
    delete wifiManager;

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

    // Calculate layout regions based on config
    calculateLayoutRegions();

    // Create display manager
    displayManager = new DisplayManager(display);

    // Create WiFi manager with config values
    wifiManager = new WiFiManager(config.wifiSSID.c_str(), config.wifiPassword.c_str());

    // Create widgets and assign them to regions
    createAndAssignWidgets();

    initializeComponents();
    performInitialSetup();
}

void LayoutManager::calculateLayoutRegions() {
    const AppConfig& config = configManager->getConfig();

    Serial.printf("Display dimensions: %dx%d\n", config.displayWidth, config.displayHeight);

    // Clear existing regions
    regions.clear();
    regionMap.clear();

    Serial.println("Regions will be created dynamically based on widget requirements");
}

void LayoutManager::createAndAssignWidgets() {
    const AppConfig& config = configManager->getConfig();

    Serial.println("Creating widgets and regions based on configuration...");

    // Create and assign weather widgets
    for (const auto& weatherConfig : config.weatherWidgets) {
        WeatherWidget* widget = new WeatherWidget(display,
                                                 weatherConfig.latitude,
                                                 weatherConfig.longitude,
                                                 weatherConfig.city,
                                                 weatherConfig.units);

        LayoutRegion* region = getOrCreateRegion(weatherConfig.region);
        region->addWidget(widget);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<WeatherWidget>();
        Serial.printf("  %s widget assigned to region %s\n", typeName.c_str(), weatherConfig.region.c_str());
    }

    // Create and assign name widgets
    for (const auto& nameConfig : config.nameWidgets) {
        NameWidget* widget = new NameWidget(display, nameConfig.familyName);

        LayoutRegion* region = getOrCreateRegion(nameConfig.region);
        region->addWidget(widget);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<NameWidget>();
        Serial.printf("  %s widget assigned to region %s\n", typeName.c_str(), nameConfig.region.c_str());
    }

    // Create and assign dateTime widgets
    for (const auto& dateTimeConfig : config.dateTimeWidgets) {
        TimeWidget* widget = new TimeWidget(display, dateTimeConfig.timeUpdateMs);

        LayoutRegion* region = getOrCreateRegion(dateTimeConfig.region);
        region->addWidget(widget);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<TimeWidget>();
        Serial.printf("  %s widget assigned to region %s\n", typeName.c_str(), dateTimeConfig.region.c_str());
    }

    // Create and assign battery widgets
    for (const auto& batteryConfig : config.batteryWidgets) {
        BatteryWidget* widget = new BatteryWidget(display, batteryConfig.batteryUpdateMs);

        LayoutRegion* region = getOrCreateRegion(batteryConfig.region);
        region->addWidget(widget);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<BatteryWidget>();
        Serial.printf("  %s widget assigned to region %s\n", typeName.c_str(), batteryConfig.region.c_str());
    }

    // Create and assign image widgets
    for (const auto& imageConfig : config.imageWidgets) {
        ImageWidget* widget = new ImageWidget(display, config.serverURL.c_str());

        LayoutRegion* region = getOrCreateRegion(imageConfig.region);
        region->addWidget(widget);

        // Using template-based type name instead of hardcoded string
        String typeName = WidgetTypeRegistry::getTypeName<ImageWidget>();
        Serial.printf("  %s widget assigned to region %s\n", typeName.c_str(), imageConfig.region.c_str());
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
    // Check if region already exists
    LayoutRegion* existingRegion = getRegionById(regionId);
    if (existingRegion) {
        return existingRegion;
    }

    // Get region layout info from config
    RegionConfig regionConfig = configManager->getRegionConfig(regionId);

    // Create new region
    auto region = make_unique_helper<LayoutRegion>(
        regionConfig.x,
        regionConfig.y,
        regionConfig.width,
        regionConfig.height
    );

    Serial.printf("Created region %s: %dx%d at (%d,%d)\n",
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
        if (region && region->needsUpdate()) {
            Serial.printf("Rendering region at (%d,%d) %dx%d\n",
                         region->getX(), region->getY(),
                         region->getWidth(), region->getHeight());

            // Clear the region first
            clearRegion(*region);

            // Let the region render all its widgets
            region->render();
        }
    }

    // Draw layout borders and separators
    drawLayoutBorders();

    // Single display update for the entire layout
    displayManager->update();

    Serial.println("Region rendering complete");
}

void LayoutManager::clearRegion(const LayoutRegion& region) {
    // Clear region with white background
    display.fillRect(region.getX(), region.getY(), region.getWidth(), region.getHeight(), 7);
}

void LayoutManager::drawLayoutBorders() {
    // Only draw borders if enabled in configuration
    if (!configManager || !configManager->getConfig().showRegionBorders) {
        return;
    }

    // Data-driven border drawing - iterate through regions and draw borders
    // This is now generic and doesn't assume specific layout structure

    for (size_t i = 0; i < regions.size(); ++i) {
        LayoutRegion* region = regions[i].get();
        if (!region) continue;

        // Draw region border
        int x = region->getX();
        int y = region->getY();
        int w = region->getWidth();
        int h = region->getHeight();

        // Draw a simple border around each region (1 pixel thick)
        // Top border
        display.drawLine(x, y, x + w - 1, y, 0);
        // Bottom border
        display.drawLine(x, y + h - 1, x + w - 1, y + h - 1, 0);
        // Left border
        display.drawLine(x, y, x, y + h - 1, 0);
        // Right border
        display.drawLine(x + w - 1, y, x + w - 1, y + h - 1, 0);
    }
}

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
