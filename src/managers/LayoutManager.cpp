#include "LayoutManager.h"

LayoutManager::LayoutManager()
    : display(INKPLATE_1BIT), lastUpdate(0) {

    // Initialize config manager
    configManager = new ConfigManager();
}

LayoutManager::~LayoutManager() {
    delete configManager;
    delete displayManager;
    delete wifiManager;
    delete imageWidget;
    delete batteryWidget;
    delete timeWidget;
    delete weatherWidget;
    delete nameWidget;
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

    // Create widgets with config values
    imageWidget = new ImageWidget(display, config.serverURL.c_str());
    batteryWidget = new BatteryWidget(display, config.batteryUpdateMs);
    timeWidget = new TimeWidget(display, config.timeUpdateMs);
    weatherWidget = new WeatherWidget(display, config.weatherLatitude, config.weatherLongitude,
                                    config.weatherCity, config.weatherUnits);
    nameWidget = new NameWidget(display, config.familyName);

    initializeComponents();
    performInitialSetup();
}

void LayoutManager::calculateLayoutRegions() {
    const AppConfig& config = configManager->getConfig();
    int displayWidth = config.displayWidth;
    int displayHeight = display.height();

    Serial.printf("Display dimensions: %dx%d\n", displayWidth, displayHeight);

    // Calculate sidebar width from config
    int sidebarWidth = (displayWidth * config.sidebarWidthPct) / 100;
    int imageAreaWidth = displayWidth - sidebarWidth;

    // Define main regions
    sidebarRegion = LayoutRegion(0, 0, sidebarWidth, displayHeight);
    imageRegion = LayoutRegion(sidebarWidth, 0, imageAreaWidth, displayHeight);

    // Divide sidebar into four sections (name, time, weather, battery)
    int sectionHeight = displayHeight / 4;

    nameRegion = LayoutRegion(0, 0, sidebarWidth, sectionHeight);
    timeRegion = LayoutRegion(0, sectionHeight, sidebarWidth, sectionHeight);
    weatherRegion = LayoutRegion(0, sectionHeight * 2, sidebarWidth, sectionHeight);
    batteryRegion = LayoutRegion(0, sectionHeight * 3, sidebarWidth, displayHeight - (sectionHeight * 3));

    Serial.printf("Layout regions calculated:\n");
    Serial.printf("  Sidebar: %dx%d at (%d,%d)\n", sidebarRegion.width, sidebarRegion.height, sidebarRegion.x, sidebarRegion.y);
    Serial.printf("  Image: %dx%d at (%d,%d)\n", imageRegion.width, imageRegion.height, imageRegion.x, imageRegion.y);
    Serial.printf("  Name: %dx%d at (%d,%d)\n", nameRegion.width, nameRegion.height, nameRegion.x, nameRegion.y);
    Serial.printf("  Time: %dx%d at (%d,%d)\n", timeRegion.width, timeRegion.height, timeRegion.x, timeRegion.y);
    Serial.printf("  Weather: %dx%d at (%d,%d)\n", weatherRegion.width, weatherRegion.height, weatherRegion.x, weatherRegion.y);
    Serial.printf("  Battery: %dx%d at (%d,%d)\n", batteryRegion.width, batteryRegion.height, batteryRegion.x, batteryRegion.y);
}

void LayoutManager::initializeComponents() {
    Serial.println("Initializing components...");

    displayManager->initialize();

    // Initialize all widgets
    imageWidget->begin();
    batteryWidget->begin();
    timeWidget->begin();
    weatherWidget->begin();
    nameWidget->begin();

    Serial.println("All widgets initialized");
}

void LayoutManager::loop() {
    wifiManager->checkConnection();
    handleScheduledUpdate();
    handleComponentUpdates();
}

void LayoutManager::performInitialSetup() {
    displayManager->showStatus("Initializing...");

    // Check configuration before attempting WiFi connection
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        Serial.printf("Configuration error: %s\n", errorMsg.c_str());

        // Show configuration error on display
        imageWidget->showErrorInRegion(imageRegion, "CONFIG ERROR",
                                     errorMsg.c_str(),
                                     "Please update your configuration");
        renderAllWidgets();
        return;
    }

    if (wifiManager->connect()) {
        Serial.println("Initial setup complete");
        displayManager->showStatus("Connected", "WiFi", wifiManager->getIPAddress().c_str());

        // Sync time and weather data
        Serial.println("WiFi connected, syncing time and weather...");
        timeWidget->syncTimeWithNTP();
        weatherWidget->fetchWeatherData();

        // Render complete layout with all widgets
        renderAllWidgets();
    } else {
        // Show error and render sidebar widgets only
        imageWidget->showErrorInRegion(imageRegion, "WIFI ERROR", "Failed to connect to network",
                                     wifiManager->getStatusString().c_str());
        renderAllWidgets();
    }

    lastUpdate = millis();
}

void LayoutManager::handleScheduledUpdate() {
    // Automatic image updates every 24 hours + manual refresh via WAKE button
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= configManager->getConfig().imageRefreshMs) {
        Serial.println("Starting scheduled daily image update...");

        if (ensureConnectivity()) {
            // Force refresh of weather and time data during scheduled update
            timeWidget->syncTimeWithNTP();
            weatherWidget->fetchWeatherData();

            // Render all widgets
            renderAllWidgets();
        }

        lastUpdate = currentTime;
    }
}

void LayoutManager::handleComponentUpdates() {
    // Check if time or battery widgets need updates
    bool needsUpdate = false;

    // Debug: Print current time and intervals every 30 seconds
    static unsigned long lastDebugPrint = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastDebugPrint > 30000) {
        Serial.printf("=== UPDATE CHECK DEBUG ===\n");
        Serial.printf("Current time: %lu ms\n", currentTime);
        Serial.printf("Time update interval: %lu ms\n", configManager->getConfig().timeUpdateMs);
        Serial.printf("Battery update interval: %lu ms\n", configManager->getConfig().batteryUpdateMs);
        lastDebugPrint = currentTime;
    }

    if (timeWidget->shouldUpdate()) {
        Serial.println("Time widget needs update");
        needsUpdate = true;
    }

    if (batteryWidget->shouldUpdate()) {
        Serial.println("Battery widget needs update");
        needsUpdate = true;
    }

    // Only update display if widgets actually need updating
    if (needsUpdate) {
        Serial.println("Updating time and battery widgets...");

        // Ensure WiFi is connected for time sync
        if (ensureConnectivity()) {
            // Force time sync if time widget needs update
            if (timeWidget->shouldUpdate()) {
                timeWidget->syncTimeWithNTP();
            }

            // Render only the widgets that need updating
            timeWidget->render(timeRegion);
            batteryWidget->render(batteryRegion);

            // Draw layout borders to maintain clean appearance
            drawLayoutBorders();

            // Use partial update for faster refresh
            displayManager->partialUpdate();

            Serial.println("Time and battery widgets updated");
        }
    }
}

bool LayoutManager::ensureConnectivity() {
    // First check if configuration is valid
    if (!configManager->isConfigured()) {
        String errorMsg = configManager->getConfigurationError();
        Serial.printf("Configuration error during connectivity check: %s\n", errorMsg.c_str());

        imageWidget->showErrorInRegion(imageRegion, "CONFIG ERROR",
                                     errorMsg.c_str(),
                                     "Please update your configuration");
        renderAllWidgets();
        return false;
    }

    if (!wifiManager->isConnected()) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        displayManager->showStatus("Reconnecting WiFi...");

        if (!wifiManager->connect()) {
            imageWidget->showErrorInRegion(imageRegion, "CONNECTION LOST", "WiFi reconnection failed",
                                         wifiManager->getStatusString().c_str());
            renderAllWidgets();
            return false;
        } else {
            // WiFi reconnected, resync time and weather
            Serial.println("WiFi reconnected, resyncing time and weather...");
            timeWidget->syncTimeWithNTP();
            weatherWidget->fetchWeatherData();
        }
    }
    return true;
}



void LayoutManager::renderAllWidgets() {
    Serial.println("Rendering all widgets in their layout regions...");

    // Render each widget in its designated region
    imageWidget->render(imageRegion);
    nameWidget->render(nameRegion);
    timeWidget->render(timeRegion);
    weatherWidget->render(weatherRegion);
    batteryWidget->render(batteryRegion);

    // Draw layout borders and separators
    drawLayoutBorders();

    // Single display update for the entire layout
    displayManager->update();

    Serial.println("Widget rendering complete");
}

void LayoutManager::clearRegion(const LayoutRegion& region) {
    // Clear region with white background
    display.fillRect(region.x, region.y, region.width, region.height, 7);
}

void LayoutManager::drawLayoutBorders() {
    // Draw vertical separator between sidebar and image area
    int separatorX = sidebarRegion.width - 1;
    display.drawLine(separatorX, 0, separatorX, display.height(), 0);
    display.drawLine(separatorX + 1, 0, separatorX + 1, display.height(), 0);

    // Draw horizontal separators between sidebar sections
    int nameBottom = nameRegion.y + nameRegion.height - 1;
    int timeBottom = timeRegion.y + timeRegion.height - 1;
    int weatherBottom = weatherRegion.y + weatherRegion.height - 1;

    // Line between name and time sections
    display.drawLine(5, nameBottom, sidebarRegion.width - 5, nameBottom, 0);
    display.drawLine(5, nameBottom + 1, sidebarRegion.width - 5, nameBottom + 1, 0);

    // Line between time and weather sections
    display.drawLine(5, timeBottom, sidebarRegion.width - 5, timeBottom, 0);
    display.drawLine(5, timeBottom + 1, sidebarRegion.width - 5, timeBottom + 1, 0);

    // Line between weather and battery sections
    display.drawLine(5, weatherBottom, sidebarRegion.width - 5, weatherBottom, 0);
    display.drawLine(5, weatherBottom + 1, sidebarRegion.width - 5, weatherBottom + 1, 0);
}

void LayoutManager::forceRefresh() {
    Serial.println("Manual layout refresh triggered by WAKE button");

    if (ensureConnectivity()) {
        // Force refresh of weather and time data
        timeWidget->syncTimeWithNTP();
        weatherWidget->fetchWeatherData();

        // Render all widgets
        renderAllWidgets();

        // Update the last update time to reset the scheduled timer
        lastUpdate = millis();
    } else {
        Serial.println("Cannot refresh layout - no connectivity");
    }
}

void LayoutManager::forceTimeAndBatteryUpdate() {
    Serial.println("Forcing time and battery widget updates");

    if (ensureConnectivity()) {
        // Force updates by resetting their timers
        timeWidget->forceUpdate();
        batteryWidget->forceUpdate();

        // Force time sync
        timeWidget->forceTimeSync();

        // Render the updated widgets
        timeWidget->render(timeRegion);
        batteryWidget->render(batteryRegion);

        // Draw layout borders
        drawLayoutBorders();

        // Update display
        displayManager->update();

        Serial.println("Time and battery widgets force updated");
    }
}
