#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Action types for button clicks, holds, and encoder adjustments
enum class ActionType : uint8_t {
    Click = 0,      // Brief button press (Toggle ON/OFF)
    StartHold = 1,  // Button pressed and held down
    Release = 2,    // Button released after a hold
    EncoderTurn = 3 // Rotary encoder rotation step
};

// Unified message structure for ESP-NOW communications
struct __attribute__((packed)) SwitchMessage {
    uint8_t remote_id;       // Unique ID of the remote control panel
    uint8_t button_index;    // Index of the button or encoder switch (0 to 3)
    uint8_t action;          // ActionType cast to uint8_t
    int8_t rotation_steps;   // Encoder relative rotation step increment/decrement
    float battery_voltage;   // Voltage of remote battery for power monitoring
};

#endif // PROTOCOL_H
