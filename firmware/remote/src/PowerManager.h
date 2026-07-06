#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>

class PowerManager {
public:
    PowerManager(uint32_t inactivityTimeoutMs = 1500);

    // Initialize/refresh the timer
    void begin();

    // Reset inactivity countdown timer
    void feed();

    // Check if the inactivity timer has expired
    bool isExpired() const;

    // Configures GPIO pins for EXT1 wakeups (active LOW) and triggers Deep Sleep
    void goToSleep(const uint8_t* wakeupPins, uint8_t pinCount);

private:
    uint32_t _inactivityTimeoutMs;
    uint32_t _lastActivityTime;
};

#endif // POWER_MANAGER_H
