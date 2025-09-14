# Inkplate Image Renderer

A PlatformIO project for displaying images from a web URL on Inkplate e-paper displays. The project fetches images over WiFi and displays them on the Inkplate screen with configurable refresh intervals.

## Features

- Fetches JPEG images from web URLs
- Configurable WiFi credentials
- Automatic refresh at specified intervals
- Support for Inkplate 10 (3-bit grayscale mode)
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
# 1. Generate configuration file
make generate-config WIFI_SSID='YourNetwork' WIFI_PASSWORD='YourPassword' SERVER_URL='http://yourserver.com/image.jpg'

# 2. Build the firmware
make build

# 3. Upload to device
make upload
```

**Quick deployment (all steps in one):**

```bash
# Complete workflow with custom refresh interval (5 minutes)
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

## Project Structure

```
├── Makefile              # Build automation with config parameters
├── platformio.ini        # PlatformIO configuration
├── src/
│   ├── main.cpp         # Main application entry point
│   ├── ImageUpdater.h   # Image updater class header
│   ├── ImageUpdater.cpp # Image updater implementation
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

- `WIFI_SSID` - Your WiFi network name (required)
- `WIFI_PASSWORD` - Your WiFi password (required)
- `SERVER_URL` - Direct URL to a JPEG image (required)
- `REFRESH_MS` - Refresh interval in milliseconds (optional, default: 60000)

### Example Configurations

```bash
# Basic setup (1 minute refresh)
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/weather.jpg'

# Longer refresh interval (1 hour)
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/weather.jpg' REFRESH_MS=3600000

# Quick development build (30 seconds)
make flash WIFI_SSID='HomeNetwork' WIFI_PASSWORD='mypassword' SERVER_URL='http://myserver.com/test.jpg' REFRESH_MS=30000
```

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
