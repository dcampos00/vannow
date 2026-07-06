/**
 * @file main.cpp
 * @brief Seeed Studio XIAO ESP32-C6 Remote Switch Panel (Battery Powered)
 * 
 * Manages remote button and encoder panels. Wakes up from Deep Sleep on activity,
 * maintains an active state for 1.5 seconds to handle continuous hold/turn events,
 * and enters Deep Sleep again once idle.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "ButtonHandler.h"
#include "EncoderHandler.h"
#include "PowerManager.h"
#include "protocol.h"

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

volatile bool messageSent = false;
volatile bool deliverySuccess = false;

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Read battery voltage using external 1:1 divisor (100k + 100k) on D4
float readBatteryVoltage() {
    int raw = analogRead(BATTERY_ADC_PIN);
    float adcVoltage = (raw / 4095.0f) * 3.3f;
    float batteryVoltage = adcVoltage * 2.0f;
    return (batteryVoltage < 0.5f) ? 3.0f : batteryVoltage;
}

// Callback when data is sent over ESP-NOW
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    deliverySuccess = (status == ESP_NOW_SEND_SUCCESS);
    messageSent = true;
}

// Pack and transmit ESP-NOW packet
void sendESPNowMessage(ActionType action, uint8_t buttonIndex, int8_t rotationSteps) {
    SwitchMessage msg;
    msg.remote_id = REMOTE_ID;
    msg.button_index = buttonIndex;
    msg.action = (uint8_t)action;
    msg.rotation_steps = rotationSteps;
    msg.battery_voltage = readBatteryVoltage();

    Serial.printf("Sending payload: Remote %d | Btn %d | Act %d | Steps %d | Bat %.2fV\n",
                  msg.remote_id, msg.button_index, msg.action, msg.rotation_steps, msg.battery_voltage);

    messageSent = false;
    esp_err_t result = esp_now_send(centralMacAddress, (uint8_t *)&msg, sizeof(msg));
    if (result != ESP_OK) {
        Serial.println("Error triggering ESP-NOW transmission.");
        return;
    }

    // Await delivery status confirmation (timeout 200ms)
    uint32_t startWait = millis();
    while (!messageSent && (millis() - startWait < 200)) {
        delay(1);
    }
}

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

    // Initialize Wi-Fi in Station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Fatal: Error initializing ESP-NOW. Going to sleep.");
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    esp_now_register_send_cb(OnDataSent);

    // Add Central as a peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, centralMacAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error adding Central peer. Going to sleep.");
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
        sendESPNowMessage(action, 0, steps); // Encoder SW button acts as button 0
    }
#else
    for (int i = 0; i < 4; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {
            powerManager.feed();
            sendESPNowMessage(action, i, 0);
        }
    }
#endif

    // 2. Check for inactivity timeout to go back to sleep
    if (powerManager.isExpired()) {
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    delay(5);
}
