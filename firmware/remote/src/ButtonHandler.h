#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "protocol.h"

class ButtonHandler {
public:
    ButtonHandler(uint8_t pin, uint8_t buttonIndex);

    // Setup input pins
    void begin();

    // Check button status and outputs active events
    bool checkEvent(ActionType& actionType);

private:
    uint8_t _pin;
    uint8_t _buttonIndex;

    enum class State {
        Idle,
        Debounce,
        Pressed,
        Holding
    };

    State _state;
    uint32_t _pressStartTime;
    uint32_t _lastHoldTime;
    uint32_t _lastPressedTime;

    static constexpr uint32_t DEBOUNCE_MS = 15;
    static constexpr uint32_t HOLD_THRESHOLD_MS = 400; // Time in ms before click turns into a hold
    static constexpr uint32_t HOLD_PERIOD_MS = 150;     // Interval between periodic hold messages
};

#endif // BUTTON_HANDLER_H
