#include "EncoderHandler.h"

EncoderHandler::EncoderHandler(uint8_t pinA, uint8_t pinB, uint8_t pinSW)
    : _pinA(pinA),
      _pinB(pinB),
      _pinSW(pinSW),
      _steps(0),
      _lastPinAState(HIGH),
      _swState(SWState::Idle),
      _swPressTime(0),
      _lastPressedTime(0) {}

void EncoderHandler::begin() {
    pinMode(_pinA, INPUT_PULLUP);
    pinMode(_pinB, INPUT_PULLUP);
    pinMode(_pinSW, INPUT_PULLUP);
    _lastPinAState = digitalRead(_pinA);
}

void EncoderHandler::update() {
    // 1. Read and decode Gray code transitions
    bool currentA = digitalRead(_pinA);
    if (currentA != _lastPinAState) {
        _lastPinAState = currentA;

        // On falling edge of signal A, sample signal B to determine direction
        if (currentA == LOW) {
            bool currentB = digitalRead(_pinB);
            if (currentB == HIGH) {
                _steps++;
            } else {
                _steps--;
            }
        }
    }
}

bool EncoderHandler::checkEvent(ActionType& actionType, int8_t& rotationSteps) {
    // 1. Check for rotation events first
    if (_steps != 0) {
        actionType = ActionType::EncoderTurn;
        rotationSteps = _steps;
        _steps = 0; // Reset steps after reading
        return true;
    }

    // 2. Check for button click event
    bool pinSWPressed = (digitalRead(_pinSW) == LOW);
    uint32_t now = millis();

    if (pinSWPressed) {
        _lastPressedTime = now;
    }

    switch (_swState) {
        case SWState::Idle:
            if (pinSWPressed) {
                _swState = SWState::Debounce;
                _swPressTime = now;
            }
            break;

        case SWState::Debounce:
            if (now - _swPressTime >= SW_DEBOUNCE_MS) {
                if (pinSWPressed) {
                    _swState = SWState::Pressed;
                } else {
                    _swState = SWState::Idle;
                }
            }
            break;

        case SWState::Pressed:
            if (now - _lastPressedTime >= SW_DEBOUNCE_MS) {
                _swState = SWState::Idle;
                actionType = ActionType::Click;
                rotationSteps = 0;
                return true;
            }
            break;
    }

    return false;
}
