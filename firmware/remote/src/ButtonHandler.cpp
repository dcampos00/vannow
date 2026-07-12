#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(uint8_t pin, uint8_t buttonIndex)
    : _pin(pin),
      _buttonIndex(buttonIndex),
      _state(State::Idle),
      _pressStartTime(0),
      _lastHoldTime(0),
      _lastPressedTime(0) {}

void ButtonHandler::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

bool ButtonHandler::checkEvent(ActionType& actionType) {
    bool pinPressed = (digitalRead(_pin) == LOW);
    uint32_t now = millis();

    if (pinPressed) {
        _lastPressedTime = now;
    }

    switch (_state) {
        case State::Idle:
            if (pinPressed) {
                _state = State::Debounce;
                _pressStartTime = now;
            }
            break;

        case State::Debounce:
            if (now - _pressStartTime >= DEBOUNCE_MS) {
                if (pinPressed) {
                    _state = State::Pressed;
                } else {
                    _state = State::Idle;
                }
            }
            break;

        case State::Pressed:
            if (now - _lastPressedTime >= DEBOUNCE_MS) {
                // Released early -> Click event
                actionType = ActionType::Click;
                _state = State::Idle;
                return true;
            } else if (now - _pressStartTime >= HOLD_THRESHOLD_MS) {
                // Exceeded threshold -> Start holding
                _state = State::Holding;
                _lastHoldTime = now;
                actionType = ActionType::StartHold;
                return true;
            }
            break;

        case State::Holding:
            if (now - _lastPressedTime >= DEBOUNCE_MS) {
                // Button released after a hold -> Release event
                _state = State::Idle;
                actionType = ActionType::Release;
                return true;
            } else if (now - _lastHoldTime >= HOLD_PERIOD_MS) {
                // Periodically repeat hold signal to confirm it is still held
                _lastHoldTime = now;
                actionType = ActionType::StartHold;
                return true;
            }
            break;
    }

    return false;
}
