# PlatformIO Makefile for Image Renderer Project

# Default target
.DEFAULT_GOAL := build

# Build the project
build:
	pio run

# Upload to device
upload:
	pio run --target upload

# Clean build files
clean:
	pio run --target clean

# Build and upload in one command
flash: build upload

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
	@echo "  build         - Compile the project"
	@echo "  upload        - Upload firmware to device"
	@echo "  clean         - Clean build files"
	@echo "  flash         - Build and upload"
	@echo "  monitor       - Start serial monitor"
	@echo "  upload-monitor- Upload and start monitoring"
	@echo "  update        - Update libraries"
	@echo "  install       - Install dependencies"
	@echo "  info          - Show project information"
	@echo "  devices       - List connected devices"
	@echo "  format        - Format source code"
	@echo "  help          - Show this help"

.PHONY: build upload clean flash monitor upload-monitor update install info devices format help
