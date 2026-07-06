#ifndef ENCODER_HANDLER_H
#define ENCODER_HANDLER_H

#include <Arduino.h>
#include "protocol.h"

class EncoderHandler {
public:
    EncoderHandler(uint8_t pinA, uint8_t pinB, uint8_t pinSW);

    // Initialize physical encoder pins
    void begin();

    // Read and decode Gray code transitions (non-blocking, called in loop)
    void update();

    // Check if there is an encoder event (rotation or button click)
    bool checkEvent(ActionType& actionType, int8_t& rotationSteps);

private:
    uint8_t _pinA;
    uint8_t _pinB;
    uint8_t _pinSW;

    int8_t _steps;
    bool _lastPinAState;

    enum class SWState {
        Idle,
        Debounce,
        Pressed
    };
    SWState _swState;
    uint32_t _swPressTime;

    static constexpr uint32_t SW_DEBOUNCE_MS = 15;
};

#endif // ENCODER_HANDLER_H
