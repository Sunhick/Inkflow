#include "LayoutManager.h"

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

    // Tell regions what type of widgets they should create
    assignWidgetTypesToRegions();

    initializeComponents();
    performInitialSetup();
}

void LayoutManager::calculateLayoutRegions() {
    const AppConfig& config = configManager->getConfig();
    int displayWidth = config.displayWidth;
    int displayHeight = display.height();

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);

    // Clear existing regions
    regions.clear();

    // Calculate sidebar width from config
    int sidebarWidth = (displayWidth * config.sidebarWidthPct) / 100;
    int imageAreaWidth = displayWidth - sidebarWidth;

    // Create regions using the new collection system
    // Order must match RegionIndex enum

    // SIDEBAR_REGION = 0
    regions.push_back(make_unique_helper<LayoutRegion>(0, 0, sidebarWidth, displayHeight));

    // IMAGE_REGION = 1
    regions.push_back(make_unique_helper<LayoutRegion>(sidebarWidth, 0, imageAreaWidth, displayHeight));

    // Divide sidebar into four sections (name, time, weather, battery)
    int sectionHeight = displayHeight / 4;

    // NAME_REGION = 2
    regions.push_back(make_unique_helper<LayoutRegion>(0, 0, sidebarWidth, sectionHeight));

    // TIME_REGION = 3
    regions.push_back(make_unique_helper<LayoutRegion>(0, sectionHeight, sidebarWidth, sectionHeight));

    // WEATHER_REGION = 4
    regions.push_back(make_unique_helper<LayoutRegion>(0, sectionHeight * 2, sidebarWidth, sectionHeight));

    // BATTERY_REGION = 5
    regions.push_back(make_unique_helper<LayoutRegion>(0, sectionHeight * 3, sidebarWidth, displayHeight - (sectionHeight * 3)));

    Serial.printf("Layout regions calculated:\n");
    Serial.printf("  Sidebar: %dx%d at (%d,%d)\n", getSidebarRegion()->getWidth(), getSidebarRegion()->getHeight(), getSidebarRegion()->getX(), getSidebarRegion()->getY());
    Serial.printf("  Image: %dx%d at (%d,%d)\n", getImageRegion()->getWidth(), getImageRegion()->getHeight(), getImageRegion()->getX(), getImageRegion()->getY());
    Serial.printf("  Name: %dx%d at (%d,%d)\n", getNameRegion()->getWidth(), getNameRegion()->getHeight(), getNameRegion()->getX(), getNameRegion()->getY());
    Serial.printf("  Time: %dx%d at (%d,%d)\n", getTimeRegion()->getWidth(), getTimeRegion()->getHeight(), getTimeRegion()->getX(), getTimeRegion()->getY());
    Serial.printf("  Weather: %dx%d at (%d,%d)\n", getWeatherRegion()->getWidth(), getWeatherRegion()->getHeight(), getWeatherRegion()->getX(), getWeatherRegion()->getY());
    Serial.printf("  Battery: %dx%d at (%d,%d)\n", getBatteryRegion()->getWidth(), getBatteryRegion()->getHeight(), getBatteryRegion()->getX(), getBatteryRegion()->getY());
}

void LayoutManager::assignWidgetTypesToRegions() {
    const AppConfig& config = configManager->getConfig();

    Serial.println("Assigning widget types to regions...");

    // Tell each region what type of widget it should create
    if (getImageRegion()) {
        getImageRegion()->setWidgetType(WidgetType::IMAGE, display, config);
    }

    if (getNameRegion()) {
        getNameRegion()->setWidgetType(WidgetType::NAME, display, config);
    }

    if (getTimeRegion()) {
        getTimeRegion()->setWidgetType(WidgetType::TIME, display, config);
    }

    if (getWeatherRegion()) {
        getWeatherRegion()->setWidgetType(WidgetType::WEATHER, display, config);
    }

    if (getBatteryRegion()) {
        getBatteryRegion()->setWidgetType(WidgetType::BATTERY, display, config);
    }

    Serial.println("Widget type assignment complete");
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

// Legacy region getters for backward compatibility
LayoutRegion* LayoutManager::getImageRegion() const {
    return getRegion(IMAGE_REGION);
}

LayoutRegion* LayoutManager::getSidebarRegion() const {
    return getRegion(SIDEBAR_REGION);
}

LayoutRegion* LayoutManager::getNameRegion() const {
    return getRegion(NAME_REGION);
}

LayoutRegion* LayoutManager::getTimeRegion() const {
    return getRegion(TIME_REGION);
}

LayoutRegion* LayoutManager::getWeatherRegion() const {
    return getRegion(WEATHER_REGION);
}

LayoutRegion* LayoutManager::getBatteryRegion() const {
    return getRegion(BATTERY_REGION);
}

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
        Serial.println("WiFi connection failed - showing error");
        if (imageWidget && getImageRegion()) {
            // Use legacy widget API for error display
            getImageRegion()->setWidget(imageWidget);
        }
        renderAllRegions();
    }

    lastUpdate = millis();
}

void LayoutManager::handleScheduledUpdate() {
    // Automatic updates every 24 hours + manual refresh via WAKE button
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= configManager->getConfig().imageRefreshMs) {
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
    // Check if regions with widgets need updates
    bool needsUpdate = false;

    // Check time widget
    if (getTimeRegion() && getTimeRegion()->getWidgetCount() > 0) {
        Widget* timeWidget = getTimeRegion()->getWidget(0);
        if (timeWidget && timeWidget->shouldUpdate()) {
            Serial.println("Time widget needs update");
            if (ensureConnectivity()) {
                // For time widget, we need to call syncTimeWithNTP
                // This is a limitation of the current architecture - we might need a better way
                getTimeRegion()->markDirty();
                needsUpdate = true;
            }
        }
    }

    // Check battery widget
    if (getBatteryRegion() && getBatteryRegion()->getWidgetCount() > 0) {
        Widget* batteryWidget = getBatteryRegion()->getWidget(0);
        if (batteryWidget && batteryWidget->shouldUpdate()) {
            Serial.println("Battery widget needs update");
            getBatteryRegion()->markDirty();
            needsUpdate = true;
        }
    }

    // Only update display if widgets actually need updating
    if (needsUpdate) {
        Serial.println("Rendering updated widgets...");
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
    // Draw vertical separator between sidebar and image area
    int separatorX = getSidebarRegion()->getWidth() - 1;
    display.drawLine(separatorX, 0, separatorX, display.height(), 0);
    display.drawLine(separatorX + 1, 0, separatorX + 1, display.height(), 0);

    // Draw horizontal separators between sidebar sections
    int nameBottom = getNameRegion()->getY() + getNameRegion()->getHeight() - 1;
    int timeBottom = getTimeRegion()->getY() + getTimeRegion()->getHeight() - 1;
    int weatherBottom = getWeatherRegion()->getY() + getWeatherRegion()->getHeight() - 1;

    // Line between name and time sections
    display.drawLine(5, nameBottom, getSidebarRegion()->getWidth() - 5, nameBottom, 0);
    display.drawLine(5, nameBottom + 1, getSidebarRegion()->getWidth() - 5, nameBottom + 1, 0);

    // Line between time and weather sections
    display.drawLine(5, timeBottom, getSidebarRegion()->getWidth() - 5, timeBottom, 0);
    display.drawLine(5, timeBottom + 1, getSidebarRegion()->getWidth() - 5, timeBottom + 1, 0);

    // Line between weather and battery sections
    display.drawLine(5, weatherBottom, getSidebarRegion()->getWidth() - 5, weatherBottom, 0);
    display.drawLine(5, weatherBottom + 1, getSidebarRegion()->getWidth() - 5, weatherBottom + 1, 0);
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

    // Return the main refresh interval - widgets handle their own update intervals
    return config.imageRefreshMs;
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
