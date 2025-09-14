# PlatformIO Makefile for Image Renderer Project

# Default target
.DEFAULT_GOAL := build

# Default values (can be overridden)
WIFI_SSID ?= "YOUR_SSID_HERE"
WIFI_PASSWORD ?= "YOUR_PASSWORD_HERE"
SERVER_URL ?= "http://SERVER_URL_HERE/image.jpg"
REFRESH_MS ?= 60000

# Build flags with configuration
BUILD_FLAGS = -DWIFI_SSID='"$(WIFI_SSID)"' -DWIFI_PASSWORD='"$(WIFI_PASSWORD)"' -DSERVER_URL='"$(SERVER_URL)"' -DREFRESH_MS=$(REFRESH_MS)

# Build the project (requires Config.h to exist)
build:
	@if [ ! -f src/Config.h ]; then echo "Error: src/Config.h not found. Run 'make generate-config' first."; exit 1; fi
	pio run --environment inkplate10

# Generate Config.h from parameters
generate-config:
	@echo "Generating src/Config.h with provided parameters..."
	@echo "#pragma once" > src/Config.h
	@echo "" >> src/Config.h
	@echo "// Auto-generated configuration file" >> src/Config.h
	@echo "// Generated on $$(date)" >> src/Config.h
	@echo "" >> src/Config.h
	@echo '#define WIFI_SSID     "$(WIFI_SSID)"' >> src/Config.h
	@echo '#define WIFI_PASSWORD "$(WIFI_PASSWORD)"' >> src/Config.h
	@echo '#define SERVER_URL    "$(SERVER_URL)"' >> src/Config.h
	@echo '#define REFRESH_MS    $(REFRESH_MS)' >> src/Config.h

# Upload to device (requires firmware to be built)
upload:
	@if [ ! -f .pio/build/inkplate10/firmware.bin ]; then echo "Error: Firmware not built. Run 'make build' first."; exit 1; fi
	pio run --target upload --environment inkplate10

# Clean build files
clean:
	pio run --target clean
	@if [ -f src/Config.h ]; then rm src/Config.h && echo "Removed generated Config.h"; fi

# Build and upload in one command (requires Config.h to exist)
flash: build upload

# Complete workflow: generate config, build, and upload
deploy: generate-config build upload

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

# Help target
help:
	@echo "Available targets:"
	@echo "  generate-config - Generate Config.h from parameters"
	@echo "  build         - Compile the project (requires Config.h)"
	@echo "  upload        - Upload firmware to device (requires built firmware)"
	@echo "  flash         - Build and upload (requires Config.h)"
	@echo "  deploy        - Complete workflow: generate config, build, and upload"
	@echo "  clean         - Clean build files and remove generated Config.h"
	@echo "  monitor       - Start serial monitor"
	@echo "  upload-monitor- Upload and start monitoring"
	@echo "  update        - Update libraries"
	@echo "  install       - Install dependencies"
	@echo "  info          - Show project information"
	@echo "  devices       - List connected devices"
	@echo "  format        - Format source code"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Configuration variables (can be set on command line):"
	@echo "  WIFI_SSID     - WiFi network name (default: YOUR_SSID_HERE)"
	@echo "  WIFI_PASSWORD - WiFi password (default: YOUR_PASSWORD_HERE)"
	@echo "  SERVER_URL    - Image URL (default: http://SERVER_URL_HERE/image.jpg)"
	@echo "  REFRESH_MS    - Refresh interval in ms (default: 60000)"
	@echo ""
	@echo "Typical workflow:"
	@echo "  1. make generate-config WIFI_SSID='MyNet' WIFI_PASSWORD='MyPass' SERVER_URL='http://example.com/image.jpg'"
	@echo "  2. make build"
	@echo "  3. make upload"
	@echo ""
	@echo "Or use shortcuts:"
	@echo "  make deploy WIFI_SSID='MyNet' WIFI_PASSWORD='MyPass' SERVER_URL='http://example.com/image.jpg'"

.PHONY: build upload clean flash deploy monitor upload-monitor update install info devices format help generate-config
