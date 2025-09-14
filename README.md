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

### 2. Configure Project

Copy the configuration template and update with your settings:

```bash
cp src/Config.h.template src/Config.h
```

Edit `src/Config.h` with your WiFi credentials and image URL:

```cpp
#define WIFI_SSID     "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
#define IMAGE_URL     "http://yourserver.com/image.jpg"
#define REFRESH_MS    60000  // Refresh every minute
```

### 3. Build and Upload

Using the Makefile (recommended):

```bash
# Build the project
make build

# Upload to device
make upload

# Build and upload in one step
make flash

# Upload and start serial monitoring
make upload-monitor
```

Or using PlatformIO directly:

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
├── Makefile              # Build automation
├── platformio.ini        # PlatformIO configuration
├── src/
│   ├── main.cpp         # Main application entry point
│   ├── ImageUpdater.h   # Image updater class header
│   ├── ImageUpdater.cpp # Image updater implementation
│   └── Config.h         # Configuration (create from template)
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

## Configuration Options

### WiFi Settings
- `WIFI_SSID` - Your WiFi network name
- `WIFI_PASSWORD` - Your WiFi password

### Image Settings
- `IMAGE_URL` - Direct URL to a JPEG image
- `REFRESH_MS` - Refresh interval in milliseconds

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
