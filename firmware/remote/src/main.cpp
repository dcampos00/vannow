/**
 * @file main.cpp
 * @brief Seeed Studio XIAO ESP32-C6 Remote Switch Panel (Battery Powered)
 *
 * Entrance panel (default) or Cockpit hub (build flag). Wakes from Deep Sleep,
 * stays awake 1.5 s for hold/turn events, then returns to sleep.
 */

#include <Arduino.h>
#include "BinaryMatrixHandler.h"
#include "ButtonHandler.h"
#include "EncoderHandler.h"
#include "PowerManager.h"
#include "RemoteSender.h"

#define PANEL_TYPE_BUTTONS  1
#define PANEL_TYPE_ENCODER  2
#define PANEL_TYPE_MATRIX   3
#define PANEL_TYPE_ENTRANCE 4
#define PANEL_TYPE_COCKPIT  5

#ifndef CONFIG_PANEL_TYPE
#define CONFIG_PANEL_TYPE PANEL_TYPE_ENTRANCE
#endif

#ifndef REMOTE_ID
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_COCKPIT)
#define REMOTE_ID 3
#else
#define REMOTE_ID 1
#endif
#endif

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

#define BATTERY_ADC_PIN 255
#define STATUS_LED_PIN  18  // XIAO ESP32-C6 D10 = GPIO 18 (not GPIO 9 / BOOT)
#define ENCODER_BUTTON_INDEX 7

void indicateAction(ActionType action, uint8_t buttonIdx) {
    pinMode(STATUS_LED_PIN, OUTPUT);
    if (buttonIdx == 4 && (action == ActionType::DoubleClick || action == ActionType::StartHold)) {
        for (int p = 0; p < 3; p++) {
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(80);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(80);
        }
    } else {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(50);
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}

#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
ButtonHandler btnZone1(21, 0); // D3  Zone 1
ButtonHandler btnZone2(22, 1); // D4  Zone 2
ButtonHandler btnZone3(23, 2); // D5  Zone 3
ButtonHandler btnZone4(16, 3); // D6  Zone 4
ButtonHandler btnPump(17, 4);  // D7  Water Pump
ButtonHandler btnNight(19, 5); // D8  Master Night Shutdown
ButtonHandler* buttons[6] = {
    &btnZone1, &btnZone2, &btnZone3, &btnZone4, &btnPump, &btnNight
};
const uint8_t NUM_BUTTONS = 6;
EncoderHandler encoder(1, 2, 20); // D1 Enc A, D2 Enc B, D9 Enc SW (exclusive owner of GPIO 20)
const uint8_t wakeupPins[] = {0, 1, 2};

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_COCKPIT)
ButtonHandler sw1(1, 0, ButtonHandler::InputMode::Latching);  // D1 Exterior
ButtonHandler sw2(2, 1, ButtonHandler::InputMode::Latching);  // D2 Orion-XS #1
ButtonHandler sw3(21, 2, ButtonHandler::InputMode::Latching); // D3 Zone 1
ButtonHandler sw4(22, 3, ButtonHandler::InputMode::Latching); // D4 Water Pump
ButtonHandler sw5(23, 4, ButtonHandler::InputMode::Latching); // D5 Inverter
ButtonHandler sw6(16, 5, ButtonHandler::InputMode::Latching); // D6 Maxxair power
ButtonHandler* buttons[6] = { &sw1, &sw2, &sw3, &sw4, &sw5, &sw6 };
const uint8_t NUM_BUTTONS = 6;
const uint8_t wakeupPins[] = {0};

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
EncoderHandler encoder(0, 1, 2);
const uint8_t wakeupPins[] = {0, 1, 2};

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_MATRIX)
BinaryMatrixHandler matrix(0, 1, 2);
const uint8_t wakeupPins[] = {0, 1, 2};

#else
ButtonHandler btn0(0, 0);
ButtonHandler btn1(1, 1);
ButtonHandler btn2(2, 2);
ButtonHandler* buttons[3] = { &btn0, &btn1, &btn2 };
const uint8_t NUM_BUTTONS = 3;
const uint8_t wakeupPins[] = {0, 1, 2};
#endif

const uint8_t NUM_WAKEUP_PINS = sizeof(wakeupPins) / sizeof(wakeupPins[0]);

PowerManager powerManager(1500);
RemoteSender remoteSender(REMOTE_ID, centralMacAddress, BATTERY_ADC_PIN);

void setup() {
    Serial.begin(115200);

#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->begin();
    }
    encoder.begin();
    pinMode(0, INPUT_PULLUP);
#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_COCKPIT)
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->begin();
    }
    pinMode(0, INPUT_PULLUP);
#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_ENCODER)
    encoder.begin();
#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_MATRIX)
    matrix.begin();
#else
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i]->begin();
    }
#endif

    pinMode(STATUS_LED_PIN, OUTPUT);

    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    if (wakeupReason != ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Cold boot detected. Providing visual confirmation and entering sleep.");
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(150);
        digitalWrite(STATUS_LED_PIN, LOW);
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    if (!remoteSender.begin(myMacAddress)) {
        Serial.println("Fatal: Error initializing RemoteSender. Going to sleep.");
        remoteSender.persistSequence();
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    powerManager.begin();
    Serial.println("Remote active. Listening for physical input transitions.");
}

void loop() {
#if (CONFIG_PANEL_TYPE == PANEL_TYPE_ENTRANCE)
    encoder.update();
    ActionType encAction;
    int8_t encSteps = 0;
    if (encoder.checkEvent(encAction, encSteps)) {
        powerManager.feed();
        remoteSender.send(encAction, ENCODER_BUTTON_INDEX, encSteps);
        indicateAction(encAction, ENCODER_BUTTON_INDEX);
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {
            powerManager.feed();
            remoteSender.send(action, i, 0);
            indicateAction(action, i);
        }
    }

#elif (CONFIG_PANEL_TYPE == PANEL_TYPE_COCKPIT)
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
        remoteSender.send(action, 0, steps);
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

    if (powerManager.isExpired()) {
        remoteSender.persistSequence();
        powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
    }

    delayMicroseconds(250);
}
