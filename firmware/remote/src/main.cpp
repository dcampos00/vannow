/**
 * @file main.cpp
 * @brief Seeed Studio XIAO ESP32-C6 Remote Switch Panel (Battery Powered)
 * 
 * Manages remote button and encoder panels. Wakes up from Deep Sleep on activity,
 * maintains an active state for 1.5 seconds to handle continuous hold/turn events,
 * and enters Deep Sleep again once idle.
 */

#include <Arduino.h>
#include "ButtonHandler.h"
#include "EncoderHandler.h"
#include "PowerManager.h"
#include "RemoteSender.h"

// ==========================================
// CONFIGURATION AND TARGETS
// ==========================================
#define REMOTE_ID 1

#define PANEL_TYPE_BUTTONS 1
#define PANEL_TYPE_ENCODER 2

// Configure panel layout type here
#define CONFIG_PANEL_TYPE PANEL_TYPE_BUTTONS

// Central MAC address (replace with your receiver's address if different)
uint8_t centralMacAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// ==========================================
// PIN DEFINITIONS & INSTANTIATIONS
// ==========================================
#define BATTERY_ADC_PIN 4 

#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
EncoderHandler encoder(0, 1, 2); // Pin A = D0 (GPIO 0), Pin B = D1 (GPIO 1), Pin SW = D2 (GPIO 2)
const uint8_t wakeupPins[] = {0, 2}; // Wake up on rotation (GPIO 0) or button click (GPIO 2)
#else
ButtonHandler btn0(0, 0); // D0
ButtonHandler btn1(1, 1); // D1
ButtonHandler btn2(2, 2); // D2
ButtonHandler btn3(3, 3); // D3
ButtonHandler* buttons[4] = { &btn0, &btn1, &btn2, &btn3 };
const uint8_t wakeupPins[] = {0, 1, 2, 3}; // Wake up on any button press
#endif

const uint8_t NUM_WAKEUP_PINS = sizeof(wakeupPins) / sizeof(wakeupPins[0]);

PowerManager powerManager(1500); // 1.5 seconds activity timeout
RemoteSender remoteSender(REMOTE_ID, centralMacAddress, BATTERY_ADC_PIN);

// ==========================================
// SETUP & LOOP
// ==========================================
void setup() {
    Serial.begin(115200);

    // Verify wakeup cause. Go straight to sleep on fresh boot to preserve power.
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    if (wakeupReason != ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Cold boot detected. Entering sleep immediately.");
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    // Initialize inputs
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
    encoder.begin();
#else
    for (int i = 0; i < 4; i++) {
        buttons[i]->begin();
    }
#endif

    // Initialize RemoteSender (Wi-Fi, ESP-NOW, battery reading pin, and Central peer)
    if (!remoteSender.begin()) {
        Serial.println("Fatal: Error initializing RemoteSender. Going to sleep.");
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    // Start activity countdown
    powerManager.begin();
    Serial.println("Remote active. Listening for physical input transitions.");
}

void loop() {
    // 1. Process physical inputs
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
    encoder.update();
    ActionType action;
    int8_t steps = 0;
    if (encoder.checkEvent(action, steps)) {
        powerManager.feed();
        remoteSender.send(action, 0, steps); // Encoder SW button acts as button 0
    }
#else
    for (int i = 0; i < 4; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {
            powerManager.feed();
            remoteSender.send(action, i, 0);
        }
    }
#endif

    // 2. Check for inactivity timeout to go back to sleep
    if (powerManager.isExpired()) {
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    delay(5);
}
