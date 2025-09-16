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
- Error handling and status messages

## Hardware Requirements

- Inkplate 10 e-paper display
- WiFi connection

## Setup

You have two options for getting the firmware onto your device:

### Option A: Download Pre-built Firmware (Easiest)

1. **Download firmware** from the [Releases page](../../releases) or [latest build artifacts](../../actions)
2. **Extract the firmware package**
3. **Install esptool**: `pip install esptool`
4. **Flash using included scripts**:
   - Linux/macOS: `./flash.sh [port]`
   - Windows: `flash.bat [COM_port]`

**Note**: Pre-built firmware uses template configuration. You'll need to rebuild with your settings for it to work properly.

### Option B: Build from Source (Recommended)

#### 1. Install PlatformIO

Install PlatformIO Core or use the PlatformIO IDE extension for VS Code.

#### 2. Configuration

```bash
# Copy the template and edit manually
cp src/config/Config.h.template src/config/Config.h

# Edit src/config/Config.h with your settings
# Then build and upload
make build
make upload
```

#### Configuration Parameters

Edit `src/config/Config.h` with your settings:

- **WIFI_SSID** - Your WiFi network name
- **WIFI_PASSWORD** - Your WiFi password
- **SERVER_URL** - Direct URL to a JPEG image
- **WEATHER_LATITUDE** - Your latitude (default: Seattle)
- **WEATHER_LONGITUDE** - Your longitude (default: Seattle)
- **WEATHER_CITY** - City name for display
- **WEATHER_UNITS** - "fahrenheit" or "celsius"



## Quick Start

```bash
# Build and upload
make build
make upload

# Or use PlatformIO directly
pio run --target upload
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

### Left Sidebar Sections (20% width, divided into 3 equal parts)

#### **Time Section (Top 1/3)**
- **DATE & TIME** (title)
- **FRIDAY** (day of week)
- **September 15, 2025** (full date)
- **2:30 PM** (current time)

#### **Weather Section (Middle 1/3)**
- **WEATHER** (title)
- **Seattle** (city name, large text)
- **72F** (temperature, large text)
- **Partly Cloudy** (weather description)
- **Rain: 15%** (precipitation probability)

#### **Battery Section (Bottom 1/3)**
- **BATTERY** (title)
- **85%** (percentage, large text)
- **[████████░░]** (visual battery icon)
- **3.85V** (actual voltage)

### Update Intervals
- **Images**: Every 24 hours (86400000 ms)
- **Time**: Every 30 minutes (1800000 ms) - syncs with NTP servers
- **Weather**: Every 30 minutes (1800000 ms) - fetches from Open-Meteo API
- **Battery**: Every 30 minutes (1800000 ms) - reads actual battery voltage
- **Manual Refresh**: WAKE button triggers immediate update of all components

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
