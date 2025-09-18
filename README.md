# Inkplate Smart Display

![Build Status](https://github.com/Sunhick/Inkplate10-client/workflows/Build%20Firmware/badge.svg)

A smart e-paper display for Inkplate devices that shows web images with weather, time, and battery status in an elegant sidebar layout.

## Features

- Fetches JPEG images from web URLs
- **Weather information** with temperature and conditions from free Open-Meteo API (no API key required!)
- Configurable WiFi credentials
- Automatic refresh at specified intervals (default: 24 hours)
- Manual refresh via WAKE button
- Support for Inkplate 10 (3-bit grayscale mode)
- Battery monitoring with percentage display and charging icon
- Time display with automatic updates
- **Three-section left sidebar layout** (Time | Weather | Battery)
- Modular widget-based architecture with Layout Manager
- **Display Compositor** for optimized rendering and partial updates
- Error handling and status messages

## Hardware Requirements

- Inkplate 10 e-paper display
- WiFi connection
- Image server (Raspberry pi-5, personal choice)

## Setup

You have two options for getting the firmware onto your device:

### Option A: Download Pre-built Firmware (Easiest)

1. **Download firmware** from the [Releases page](../../releases) or [latest build artifacts](../../actions)
2. **Extract the firmware package**
3. **Install esptool**: `pip install esptool`
4. **Flash using included scripts**:
   - Linux/macOS: `./flash.sh [port]`
   - Windows: `flash.bat [COM_port]`

**Note**: Pre-built firmware includes a default configuration. After flashing, you can update settings using the interactive configuration tool.

### Option B: Build from Source (Recommended)

#### 1. Install PlatformIO

Install PlatformIO Core or use the PlatformIO IDE extension for VS Code.

#### 2. Configuration

**Interactive Configuration Setup:**
```bash
# Setup configuration (creates data/config.json)
make setup-config

# Build and upload everything
make upload-all
```

#### Configuration Parameters

The configuration uses a `config.json` file stored on the device's SPIFFS filesystem:

- **WiFi SSID/Password** - Your network credentials
- **Server URL** - Direct URL to a JPEG image
- **Weather Location** - Latitude, longitude, and city name for weather data
- **Weather Units** - "fahrenheit" or "celsius"
- **Refresh Interval** - How often to update in milliseconds (default: 86400000 = 24 hours)

The interactive setup script (`make setup-config`) will guide you through configuring these parameters.



## Quick Start

```bash
# 1. Setup configuration
make setup-config

# 2. Upload everything (filesystem + firmware)
make upload-all

# Or use PlatformIO directly
pio run --target uploadfs  # Upload config.json
pio run --target upload    # Upload firmware
```

## Pre-built Firmware

Download ready-to-flash firmware from [Releases](../../releases) or [latest builds](../../actions).



## Display Layout

The display uses a **sidebar layout** with the main image taking up 80% of the width and a 20% sidebar containing status information:

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                           Main Image Area                                    │
│                             (80% width)                                      │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

**With Left Sidebar (20% width):**

```
┌──────────────┬─────────────────────────────────────────────────────────────┐
│ DATE & TIME  │                                                             │
│ FRIDAY       │                                                             │
│ Sep 15, 2025 │                                                             │
│ 2:30 PM      │                                                             │
├──────────────┤                     Main Image Area                         │
│ WEATHER      │                       (80% width)                           │
│ Seattle      │                                                             │
│ 72F          │                                                             │
│ Partly Cloudy│                                                             │
│ Rain: 15%    │                                                             │
├──────────────┤                                                             │
│ BATTERY      │                                                             │
│ 85%          │                                                             │
│ [████████░░] │                                                             │
│ 3.85V        │                                                             │
└──────────────┴─────────────────────────────────────────────────────────────┘
```

### Update Intervals
- **Images**: Every 24 hours (86400000 ms)
- **Time**: Every 30 minutes (1800000 ms) - syncs with NTP servers
- **Weather**: Every 30 minutes (1800000 ms) - fetches from Open-Meteo API
- **Battery**: Every 30 minutes (1800000 ms) - reads actual battery voltage
- **Manual Refresh**: WAKE button triggers immediate update of all components

## Troubleshooting

### Build Issues
- Ensure PlatformIO is properly installed
- Check that all dependencies are installed with `make update`

### Upload Issues
- Verify the Inkplate is connected via USB
- Check the correct port is selected
- Try pressing the reset button on the Inkplate

### WiFi Connection Issues
- Verify WiFi credentials in your `config.json` file (run `make setup-config` to update)
- Check WiFi signal strength
- Ensure the network supports 2.4GHz (ESP32 requirement)
- Use serial monitor (`make monitor`) to see connection status

### Image Loading Issues
- Verify the image URL is accessible
- Check image format (JPEG required)
- Ensure image size is reasonable for the display

## Configuration Management

### Updating Configuration
To change settings after initial setup:
```bash
make setup-config    # Run interactive configuration
make upload-fs       # Upload updated config.json to device
```

### Configuration File Location
- **Local**: `data/config.json` (created by setup script)
- **Device**: Stored in SPIFFS filesystem on the ESP32

### Default Configuration
If no configuration file exists, the device will use built-in defaults and display an error message on the screen.

## Serial Monitor Output

The device outputs status information via serial at 115200 baud:
- WiFi connection status
- Configuration loading status
- Image fetch attempts
- Weather API responses
- Error messages and success confirmations

Use `make monitor` or `make upload-monitor` to view serial output.

## Display Compositor Architecture

The project includes a sophisticated display compositor system that optimizes rendering performance:

### Key Components

- **DisplayCompositor**: Central rendering engine that manages a virtual drawing surface
- **VirtualSurface**: Off-screen buffer where widgets draw their content
- **CompositorWidget**: Base class for widgets that work with the compositor
- **Dirty Region Tracking**: Only updates screen areas that have changed

### Benefits

- **Single Render Operation**: All widgets draw to a virtual surface, then the compositor performs one optimized render to the physical display
- **Partial Updates**: Only changed regions are updated, reducing refresh time and power consumption
- **Display Mode Management**: Automatically handles switching between 1-bit and 3-bit modes for optimal performance
- **Region Optimization**: Merges overlapping dirty regions to minimize update operations

### Widget Integration

Widgets can be adapted to work with the compositor by extending `CompositorWidget` instead of `Widget`:

```cpp
class MyCompositorWidget : public CompositorWidget {
public:
    MyCompositorWidget(DisplayCompositor& compositor) : CompositorWidget(compositor) {}

protected:
    void renderToSurface(VirtualSurface* surface, const LayoutRegion& region) override {
        // Draw to virtual surface instead of directly to display
        surface->setCursor(region.x + 10, region.y + 10);
        surface->setTextSize(2);
        surface->print("Hello World");
    }
};
```

### Performance Optimizations

- **Virtual Surface Buffering**: Eliminates flickering and reduces display update frequency
- **Intelligent Partial Updates**: Uses Inkplate's partial update capability efficiently
- **Region Merging**: Combines adjacent dirty regions to reduce the number of update operations
- **Mode Switching**: Automatically switches display modes for optimal performance

## Available Make Targets

```bash
make setup-config     # Interactive configuration setup
make build           # Build firmware only
make upload          # Upload firmware only
make upload-fs       # Upload filesystem (config.json) only
make upload-all      # Upload both filesystem and firmware
make monitor         # Open serial monitor
make upload-monitor  # Upload firmware and open monitor
make clean           # Clean build files
make update          # Update PlatformIO libraries
make help           # Show all available targets
```

## License

This project is open source. Please check individual library licenses for their respective terms.
