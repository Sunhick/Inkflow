# PlatformIO Makefile for Image Renderer Project

# Default target
.DEFAULT_GOAL := build

# Default values (can be overridden)
WIFI_SSID ?= "YOUR_SSID_HERE"
WIFI_PASSWORD ?= "YOUR_PASSWORD_HERE"
SERVER_URL ?= "http://SERVER_URL_HERE/image.jpg"
REFRESH_MS ?= 3600000
WEATHER_LATITUDE ?= "37.7749"
WEATHER_LONGITUDE ?= "-122.4194"
WEATHER_UNITS ?= "fahrenheit"

# Build flags with configuration
BUILD_FLAGS = -DWIFI_SSID='"$(WIFI_SSID)"' -DWIFI_PASSWORD='"$(WIFI_PASSWORD)"' -DSERVER_URL='"$(SERVER_URL)"' -DREFRESH_MS=$(REFRESH_MS)

# Build the project
build:
	pio run --environment esp32



# Interactive configuration setup
setup-config:
	python3 scripts/setup-config.py

# Upload filesystem (SPIFFS) with config file
upload-fs:
	pio run --target uploadfs --environment esp32

# Upload to device (requires firmware to be built)
upload:
	@if [ ! -f .pio/build/esp32/firmware.bin ]; then echo "Error: Firmware not built. Run 'make build' first."; exit 1; fi
	pio run --target upload --environment esp32

# Upload both filesystem and firmware
upload-all: upload-fs upload

# Clean build files
clean:
	pio run --target clean

# Build and upload in one command
flash: build upload

# New workflow: setup config file, upload filesystem and firmware
deploy-fs: setup-config upload-all

# Monitor serial output
monitor:
	pio device monitor

# Upload and immediately start monitoring
upload-monitor: upload monitor

# Check for updates to libraries
update:
	pio lib update

# Install dependencies
install:
	pio lib install

# Show project info
info:
	pio project data

# List connected devices
devices:
	pio device list

# Format code (if clang-format is available)
format:
	find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run unit tests
test:
	pio test --environment test

# Help target
help:
	@echo "Available targets:"
	@echo "  setup-config  - Interactive configuration setup (creates config.json)"
	@echo "  upload-fs     - Upload filesystem (SPIFFS) with config file"
	@echo "  upload-all    - Upload both filesystem and firmware"
	@echo "  build         - Compile the project"
	@echo "  upload        - Upload firmware to device (requires built firmware)"
	@echo "  flash         - Build and upload"
	@echo "  deploy-fs     - Complete workflow: setup config file, upload filesystem and firmware"
	@echo "  clean         - Clean build files"
	@echo "  monitor       - Start serial monitor"
	@echo "  upload-monitor- Upload and start monitoring"
	@echo "  update        - Update libraries"
	@echo "  install       - Install dependencies"
	@echo "  info          - Show project information"
	@echo "  devices       - List connected devices"
	@echo "  format        - Format source code"
	@echo "  test          - Run unit tests"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Configuration variables (can be set on command line):"
	@echo "  WIFI_SSID       - WiFi network name (default: YOUR_SSID_HERE)"
	@echo "  WIFI_PASSWORD   - WiFi password (default: YOUR_PASSWORD_HERE)"
	@echo "  SERVER_URL      - Image URL (default: http://SERVER_URL_HERE/image.jpg)"
	@echo "  REFRESH_MS      - Refresh interval in ms (default: 3600000 = 1 hour)"
	@echo "  WEATHER_LATITUDE  - Your latitude (default: 37.7749 = San Francisco)"
	@echo "  WEATHER_LONGITUDE - Your longitude (default: -122.4194 = San Francisco)"
	@echo "  WEATHER_UNITS     - Temperature units (default: fahrenheit)"
	@echo ""
	@echo "Recommended workflow (with config file):"
	@echo "  1. make setup-config    # Interactive configuration setup"
	@echo "  2. make build           # Build firmware"
	@echo "  3. make upload-all      # Upload filesystem and firmware"
	@echo ""
	@echo "Or use shortcut:"
	@echo "  make deploy-fs          # Complete workflow with config file"
	@echo ""


.PHONY: build upload upload-fs upload-all clean flash deploy-fs monitor upload-monitor update install info devices format test help setup-config
