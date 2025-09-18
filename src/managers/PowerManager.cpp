#include "PowerManager.h"
#include <WiFi.h>

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
    Serial.println("Entering deep sleep...");
    Serial.flush();

    // Disable WiFi and Bluetooth to save power
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    disableUnusedPeripherals();

    esp_deep_sleep_start();
}

void PowerManager::configureLowPowerMode() {
    // Reduce CPU frequency when not actively processing
    setCpuFrequencyMhz(80); // Down from 240MHz

    // Disable unused peripherals
    disableUnusedPeripherals();
}

void PowerManager::disableUnusedPeripherals() {
    // Disable ADC, DAC, and other unused peripherals
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
}
