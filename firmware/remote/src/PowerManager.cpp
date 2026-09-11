#include "PowerManager.h"

#ifdef ARDUINO_ARCH_ESP32
#include <esp_now.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#endif

PowerManager::PowerManager(uint32_t inactivityTimeoutMs)
    : _inactivityTimeoutMs(inactivityTimeoutMs), _lastActivityTime(0) {}

void PowerManager::begin() {
    _lastActivityTime = millis();
}

void PowerManager::feed() {
    _lastActivityTime = millis();
}

bool PowerManager::isExpired() const {
    return (millis() - _lastActivityTime >= _inactivityTimeoutMs);
}

void PowerManager::goToSleep(const uint8_t* wakeupPins, uint8_t pinCount) {
    Serial.println("Preparing for Deep Sleep...");

    // Create the pin mask for EXT1 wake-up and ensure solid pull-ups on all wake pins
    uint64_t pinMask = 0;
    for (uint8_t i = 0; i < pinCount; i++) {
        pinMask |= (1ULL << wakeupPins[i]);
        pinMode(wakeupPins[i], INPUT_PULLUP);
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_LINUX)
        gpio_pullup_en((gpio_num_t)wakeupPins[i]);
#endif
    }

    // Enable wake-up on any specified pins going LOW (button switch press)
    esp_sleep_enable_ext1_wakeup(pinMask, ESP_EXT1_WAKEUP_ANY_LOW);

    Serial.println("Entering Deep Sleep mode now.");
    Serial.flush();
    delay(10);

#ifdef ARDUINO_ARCH_ESP32
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
#endif

    esp_deep_sleep_start();
}
