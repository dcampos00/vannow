/**
 * @file main.cpp
 * @brief Seeed Studio XIAO ESP32-C6 Remote Switch Panel (Battery Powered)
 * 
 * Manages remote button and encoder panels. Wakes up from Deep Sleep on activity,
 * maintains an active state for 1.5 seconds to handle continuous hold/turn events,
 * and enters Deep Sleep again once idle.
 */

#include <Arduino.h>
#include "BinaryMatrixHandler.h"
#include "ButtonHandler.h"
#include "EncoderHandler.h"
#include "PowerManager.h"
#include "RemoteSender.h"

// ==========================================
// CONFIGURATION AND TARGETS
// ==========================================
#define REMOTE_ID 1

#define PANEL_TYPE_BUTTONS  1
#define PANEL_TYPE_ENCODER  2
#define PANEL_TYPE_MATRIX   3
#define PANEL_TYPE_ENTRANCE 4

// Configure panel layout type here (PANEL_TYPE_ENTRANCE matches entrance-remote-pcb)
#define CONFIG_PANEL_TYPE PANEL_TYPE_ENTRANCE

// Provisioned Central & Remote MAC addresses
uint8_t centralMacAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

#if (REMOTE_ID == 1)
const uint8_t myMacAddress[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};
#elif (REMOTE_ID == 2)
const uint8_t myMacAddress[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x22};
#elif (REMOTE_ID == 3)
const uint8_t myMacAddress[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x33};
#else
const uint8_t myMacAddress[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};
#endif

// ==========================================
// PIN DEFINITIONS & INSTANTIATIONS
// ==========================================
#define BATTERY_ADC_PIN 255 // 2x AA direct 3.0V battery bay (no divider needed)
#define STATUS_LED_PIN  9   // D10 Status Micro-LED on XIAO ESP32-C6

// Visual feedback: 3 rapid pulses for Shower Mode, 1 pulse for standard actions
void indicateAction(ActionType action, uint8_t buttonIdx) {
    pinMode(STATUS_LED_PIN, OUTPUT);
    if ((buttonIdx == 2 || buttonIdx == 3) && (action == ActionType::DoubleClick || action == ActionType::StartHold)) {
        // Shower Mode: 3 rapid 80ms pulses
        for (int p = 0; p < 3; p++) {
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(80);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(80);
        }
    } else {
        // Standard action confirmation: single 50ms pulse
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(50);
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}

#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
// Master Entrance Remote PCB: 6 tactile buttons + rotary encoder + Diode-OR wakeup
ButtonHandler btnZone1(21, 0); // D3 -> Channel 0 (Lights Zone 1)
ButtonHandler btnZone2(22, 1); // D4 -> Channel 1 (Lights Zone 2)
ButtonHandler btnPump(17, 2);  // D7 -> Channel 4 (Water Pump)
ButtonHandler btnInvert(19, 3);// D8 -> Channel 9 (Inverter / Night)
ButtonHandler btnZone3(23, 4); // D5 -> Channel 2 (Lights Zone 3)
ButtonHandler btnZone4(16, 5); // D6 -> Channel 3 (Lights Zone 4)
ButtonHandler btnCenter(20, 6);// D9 -> Channel 8 (Ceiling Fan / Center Click)
ButtonHandler* buttons[7] = {
    &btnZone1, &btnZone2, &btnPump, &btnInvert, &btnZone3, &btnZone4, &btnCenter
};
const uint8_t NUM_BUTTONS = 7;
EncoderHandler encoder(1, 2, 20); // D1 (Enc A), D2 (Enc B), D9 (Enc SW)
const uint8_t wakeupPins[] = {0, 1, 2}; // Wake on Diode-OR bus (GPIO 0) or encoder rotation (GPIO 1, 2)

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
EncoderHandler encoder(0, 1, 2); // Pin A = D0 (GPIO 0), Pin B = D1 (GPIO 1), Pin SW = D2 (GPIO 2)
const uint8_t wakeupPins[] = {0, 1, 2}; // Wake up on rotation (GPIO 0/1) or button click (GPIO 2)

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_MATRIX)
BinaryMatrixHandler matrix(0, 1, 2); // 7-Button Diode Binary Matrix on LP-GPIO pins D0, D1, D2
const uint8_t wakeupPins[] = {0, 1, 2}; // All 3 pins wake up from Deep Sleep via EXT1

#else
ButtonHandler btn0(0, 0); // D0 (LP-GPIO 0)
ButtonHandler btn1(1, 1); // D1 (LP-GPIO 1)
ButtonHandler btn2(2, 2); // D2 (LP-GPIO 2)
ButtonHandler* buttons[3] = { &btn0, &btn1, &btn2 };
const uint8_t NUM_BUTTONS = 3;
const uint8_t wakeupPins[] = {0, 1, 2}; // Valid LP pins
#endif

const uint8_t NUM_WAKEUP_PINS = sizeof(wakeupPins) / sizeof(wakeupPins[0]);

PowerManager powerManager(1500); // 1.5 seconds activity timeout
RemoteSender remoteSender(REMOTE_ID, centralMacAddress, BATTERY_ADC_PIN);

// ==========================================
// SETUP & LOOP
// ==========================================
void setup() {
    Serial.begin(115200);

    // 1. Initialize input handlers and configure GPIO pull-ups FIRST
    // This prevents floating inputs when transitioning into deep sleep.
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->begin();
    }
    encoder.begin();
    pinMode(0, INPUT_PULLUP); // D0 Diode-OR wakeup bus
#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
    encoder.begin();
#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_MATRIX)
    matrix.begin();
#else
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->begin();
    }
#endif

    // Status LED
    pinMode(STATUS_LED_PIN, OUTPUT);

    // 2. Verify wakeup cause.
    // On fresh boot (battery inserted), blink LED once and enter sleep cleanly with pull-ups active.
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    if (wakeupReason != ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Cold boot detected. Providing visual confirmation and entering sleep.");
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(150);
        digitalWrite(STATUS_LED_PIN, LOW);
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    // 3. Initialize RemoteSender (Wi-Fi, ESP-NOW, provisioned MAC, and Central peer)
    if (!remoteSender.begin(myMacAddress)) {
        Serial.println("Fatal: Error initializing RemoteSender. Going to sleep.");
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    // Start activity countdown
    powerManager.begin();
    Serial.println("Remote active. Listening for physical input transitions.");
}

void loop() {
    // 1. Process physical inputs
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
    encoder.update();
    ActionType encAction;
    int8_t encSteps = 0;
    if (encoder.checkEvent(encAction, encSteps)) {
        powerManager.feed();
        remoteSender.send(encAction, 0, encSteps);
        indicateAction(encAction, 0);
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {
            powerManager.feed();
            remoteSender.send(action, i, 0);
            indicateAction(action, i);
        }
    }

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
    encoder.update();
    ActionType action;
    int8_t steps = 0;
    if (encoder.checkEvent(action, steps)) {
        powerManager.feed();
        remoteSender.send(action, 0, steps); // Encoder SW button acts as button 0
        indicateAction(action, 0);
    }

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_MATRIX)
    uint8_t buttonIdx = 0;
    ActionType action;
    if (matrix.checkEvent(buttonIdx, action)) {
        powerManager.feed();
        remoteSender.send(action, buttonIdx, 0);
        indicateAction(action, buttonIdx);
    }

#else
    for (int i = 0; i < NUM_BUTTONS; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {
            powerManager.feed();
            remoteSender.send(action, i, 0);
            indicateAction(action, i);
        }
    }
#endif

    // 2. Check for inactivity timeout to go back to sleep
    if (powerManager.isExpired()) {
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    // Fast polling cycle (> 3 kHz) to satisfy Nyquist rate for EC11 rotary encoder
    delayMicroseconds(250);
}
