#!/usr/bin/env python3
"""
Configuration setup script for Inkplate Smart Display
Creates a config.json file with user-provided settings
"""

import json
import os
import sys

def get_input(prompt, default=None):
    """Get user input with optional default value"""
    if default:
        response = input(f"{prompt} [{default}]: ").strip()
        return response if response else default
    else:
        response = input(f"{prompt}: ").strip()
        while not response:
            response = input(f"{prompt} (required): ").strip()
        return response

def main():
    print("=== Inkplate Smart Display Configuration Setup ===")
    print()

    # WiFi Configuration
    print("WiFi Configuration:")
    wifi_ssid = get_input("WiFi SSID")
    wifi_password = get_input("WiFi Password")
    print()

    # Server Configuration
    print("Image Server Configuration:")
    server_url = get_input("Image URL (JPEG)", "http://httpbin.org/image/jpeg")
    print()

    # Weather Configuration
    print("Weather Configuration:")
    print("(You can find coordinates at https://www.latlong.net/)")
    weather_city = get_input("City name", "Seattle")
    weather_lat = get_input("Latitude", "47.6062")
    weather_lon = get_input("Longitude", "-122.3321")
    weather_units = get_input("Temperature units (fahrenheit/celsius)", "fahrenheit")
    print()

    # Update Configuration
    print("Update Configuration:")
    refresh_hours = get_input("Refresh interval (hours)", "24")
    refresh_ms = int(refresh_hours) * 60 * 60 * 1000
    print()

    # Create configuration object
    config = {
        "wifi": {
            "ssid": wifi_ssid,
            "password": wifi_password
        },
        "server": {
            "url": server_url
        },
        "weather": {
            "latitude": weather_lat,
            "longitude": weather_lon,
            "city": weather_city,
            "units": weather_units
        },
        "update": {
            "refreshMs": refresh_ms
        },
        "display": {
            "width": 1200,
            "sidebarWidthPct": 20
        },
        "hardware": {
            "wakeButtonPin": 36
        }
    }

    # Ensure data directory exists
    os.makedirs("data", exist_ok=True)

    # Write configuration file
    config_path = "data/config.json"
    with open(config_path, 'w') as f:
        json.dump(config, f, indent=2)

    print(f"Configuration saved to {config_path}")
    print()
    print("Next steps:")
    print("1. Upload filesystem: make upload-fs")
    print("2. Build and upload firmware: make build upload")
    print("   Or use: make upload-all")
    print()
    print("Your configuration:")
    print(json.dumps(config, indent=2))

if __name__ == "__main__":
    main()
