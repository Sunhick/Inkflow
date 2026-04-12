#include "PowerManager.h"
#include "../core/Logger.h"
#include <WiFi.h>
#include "esp_bt.h"
#include "esp_bt_main.h"

void PowerManager::enableDeepSleep(unsigned long sleepTimeMs) {
    esp_sleep_enable_timer_wakeup(sleepTimeMs * 1000); // Convert to microseconds
}

void PowerManager::enableWakeOnButton(int buttonPin) {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)buttonPin, 0); // Wake on LOW (button press)
    rtc_gpio_pullup_en((gpio_num_t)buttonPin);
}

void PowerManager::enableWakeOnTimer(unsigned long timeMs) {
    esp_sleep_enable_timer_wakeup(timeMs * 1000); // Convert to microseconds
}

void PowerManager::enterDeepSleep() {
    LOG_INFO("PowerManager", "Entering deep sleep...");
    Serial.flush();

    // Disable WiFi and Bluetooth to save power
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    disableUnusedPeripherals();

    esp_deep_sleep_start();
}

void PowerManager::configureLowPowerMode() {
    LOG_INFO("PowerManager", "Configuring low power mode - reducing CPU to 80MHz");
    // Reduce CPU frequency when not actively processing
    setCpuFrequencyMhz(80); // Down from 240MHz

    // Disable unused peripherals
    disableUnusedPeripherals();
}

void PowerManager::disableUnusedPeripherals() {
    // Keep RTC_PERIPH on — required for ext0 wakeup (button pin)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    // Keep RTC_SLOW_MEM on — required for RTC_DATA_ATTR variables across deep sleep
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    // RTC_FAST_MEM not needed (no RTC_FAST_ATTR usage)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
}

void PowerManager::disableBluetooth() {
    // Fully release the BT controller — saves ~50mA even when BT isn't actively used,
    // because the controller may still be initialized by the framework at boot.
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    LOG_INFO("PowerManager", "Bluetooth controller disabled and memory released");
}
