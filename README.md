# Inkplate Image Renderer with Weather

A PlatformIO project for displaying images from a web URL on Inkplate e-paper displays with comprehensive status information. The project fetches images over WiFi and displays them with weather, battery, and time information in a clean status bar layout.

## Features

- Fetches JPEG images from web URLs
- **Weather information** with temperature and conditions from free Open-Meteo API (no API key required!)
- Configurable WiFi credentials
- Automatic refresh at specified intervals
- Support for Inkplate 10 (3-bit grayscale mode)
- Battery monitoring with percentage display and charging icon
- Time display with automatic updates
- **Three-section bottom status bar with white background and black text** (Time | Weather | Battery)
- Status updates every 30 minutes (weather, battery, time)
- Error handling and status messages

## Hardware Requirements

- Inkplate 10 e-paper display
- WiFi connection

## Setup

### 1. Install PlatformIO

Install PlatformIO Core or use the PlatformIO IDE extension for VS Code.

### 2. Build and Upload

Configuration is done at build time using command-line parameters. The Makefile automatically generates a `src/Config.h` file with your settings:

**Step-by-step workflow:**

```bash
# 1. Generate configuration file (weather works automatically - no API key needed!)
make generate-config WIFI_SSID='YourNetwork' WIFI_PASSWORD='YourPassword' SERVER_URL='http://yourserver.com/image.jpg'

# 2. Build the firmware
make build

# 3. Upload to device
make upload
```

**Quick deployment (all steps in one):**

```bash
# Complete workflow with weather and custom refresh interval (5 minutes)
make deploy WIFI_SSID='YourNetwork' WIFI_PASSWORD='YourPassword' SERVER_URL='http://yourserver.com/image.jpg' REFRESH_MS=300000
```

**Development workflow:**

```bash
# Generate config once
make generate-config WIFI_SSID='TestNet' WIFI_PASSWORD='test123' SERVER_URL='http://example.com/test.jpg'

# Then iterate quickly
make build    # Just compile
make upload   # Just flash (no rebuild)
make monitor  # Watch serial output
```

Or using PlatformIO directly with build flags:

```bash
# Build with configuration
pio run --environment inkplate10 --project-option='build_flags=-DWIFI_SSID="YourNetwork" -DWIFI_PASSWORD="YourPassword" -DSERVER_URL="http://yourserver.com/image.jpg" -DREFRESH_MS=60000'

# Upload
pio run --target upload --environment inkplate10 --project-option='build_flags=-DWIFI_SSID="YourNetwork" -DWIFI_PASSWORD="YourPassword" -DSERVER_URL="http://yourserver.com/image.jpg" -DREFRESH_MS=60000'
```

## Architecture

The project follows SOLID principles with separate classes for different responsibilities:

### Core Classes

- **`ImageUpdater`** - Main orchestrator that coordinates all components
- **`WiFiManager`** - Handles all WiFi connectivity (Single Responsibility)
- **`DisplayManager`** - Manages all display operations (Single Responsibility)
- **`ImageFetcher`** - Handles image downloading and display (Single Responsibility)
- **`WeatherManager`** - Manages weather API calls and data parsing (Single Responsibility)
- **`BatteryManager`** - Handles battery monitoring and display (Single Responsibility)
- **`TimeManager`** - Manages time display and formatting (Single Responsibility)

### SOLID Principles Applied

1. **Single Responsibility Principle (SRP)**
   - Each class has one reason to change
   - WiFiManager only handles network connectivity
   - DisplayManager only handles screen operations
   - ImageFetcher only handles image operations

2. **Open/Closed Principle (OCP)**
   - Classes are open for extension, closed for modification
   - Easy to add new display types or image sources

3. **Liskov Substitution Principle (LSP)**
   - Components can be easily swapped with compatible implementations

4. **Interface Segregation Principle (ISP)**
   - Classes depend only on methods they actually use
   - Clean, focused interfaces

5. **Dependency Inversion Principle (DIP)**
   - High-level ImageUpdater depends on abstractions
   - Easy to mock components for testing

## Project Structure

```
├── Makefile              # Build automation with config parameters
├── platformio.ini        # PlatformIO configuration
├── src/
│   ├── main.cpp         # Main application entry point
│   ├── ImageUpdater.h   # Main orchestrator class
│   ├── ImageUpdater.cpp # Main orchestrator implementation
│   ├── WiFiManager.h    # WiFi connectivity management
│   ├── WiFiManager.cpp  # WiFi implementation
│   ├── DisplayManager.h # Display operations
│   ├── DisplayManager.cpp # Display implementation
│   ├── ImageFetcher.h   # Image downloading
│   ├── ImageFetcher.cpp # Image fetching implementation
│   ├── WeatherManager.h # Weather API management
│   ├── WeatherManager.cpp # Weather implementation
│   ├── BatteryManager.h # Battery monitoring
│   ├── BatteryManager.cpp # Battery monitoring implementation
│   ├── TimeManager.h    # Time display management
│   ├── TimeManager.cpp  # Time implementation
│   └── Config.h         # Auto-generated configuration (excluded from git)
└── README.md
```

## Available Make Commands

- `make build` - Compile the project
- `make upload` - Upload firmware to device
- `make flash` - Build and upload in one step
- `make monitor` - Start serial monitor
- `make upload-monitor` - Upload and start monitoring
- `make clean` - Clean build files
- `make update` - Update libraries
- `make devices` - List connected devices
- `make help` - Show all available commands

## Configuration Parameters

Configuration is done at build time using command-line parameters:

### Required Parameters
- `WIFI_SSID` - Your WiFi network name
- `WIFI_PASSWORD` - Your WiFi password
- `SERVER_URL` - Direct URL to a JPEG image

### Optional Parameters
- `REFRESH_MS` - Image refresh interval in milliseconds (default: 3600000 = 1 hour)
- `WEATHER_LATITUDE` - Your latitude (default: "37.7749" = San Francisco)
- `WEATHER_LONGITUDE` - Your longitude (default: "-122.4194" = San Francisco)
- `WEATHER_UNITS` - Temperature units: "fahrenheit" or "celsius" (default: "fahrenheit")

### Example Configurations

```bash
# Basic setup with weather (1 hour refresh) - San Francisco by default
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/weather.jpg'

# Custom location (London) and units (celsius)
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/image.jpg' WEATHER_LATITUDE='51.5074' WEATHER_LONGITUDE='-0.1278' WEATHER_UNITS='celsius'

# Quick development build (30 seconds refresh)
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/test.jpg' REFRESH_MS=30000
```

## Display Layout

The display features a clean layout with a comprehensive status bar:

```
┌─────────────────────────────────────────┐
│                                         │
│            Main Image Area              │
│              (95% height)               │
│                                         │
├─────────────────────────────────────────┤
│ 2:30 PM │ 72°F Sunny │      85% ⚡     │ ← Status Bar (5%)
└─────────────────────────────────────────┘
```

### Status Bar Sections
- **Left (25%)**: Current time (e.g., "2:30 PM")
- **Center (25%)**: Weather info (e.g., "72°F Sunny")
- **Right (50%)**: Battery percentage with charging icon (e.g., "85% ⚡")

### Update Intervals
- **Images**: Every 1 hour (configurable via REFRESH_MS)
- **Status Info**: Every 30 minutes (weather, battery, time)
- **WiFi**: Auto-reconnection on connection loss

### Supported Image Formats
- JPEG images
- Images should be sized appropriately for the Inkplate 10 display (1200x825 pixels)

## Troubleshooting

### Build Issues
- Ensure PlatformIO is properly installed
- Check that all dependencies are installed with `make update`

### Upload Issues
- Verify the Inkplate is connected via USB
- Check the correct port is selected
- Try pressing the reset button on the Inkplate

### WiFi Connection Issues
- Verify WiFi credentials in `Config.h`
- Check WiFi signal strength
- Ensure the network supports 2.4GHz (ESP32 requirement)

### Image Loading Issues
- Verify the image URL is accessible
- Check image format (JPEG required)
- Ensure image size is reasonable for the display

## Serial Monitor Output

The device outputs status information via serial at 115200 baud:
- WiFi connection status
- Image fetch attempts
- Error messages
- Success confirmations

Use `make monitor` or `make upload-monitor` to view serial output.

## License

This project is open source. Please check individual library licenses for their respective terms.
