#include "BinaryMatrixHandler.h"

BinaryMatrixHandler::BinaryMatrixHandler(uint8_t pin0, uint8_t pin1, uint8_t pin2)
    : _pin0(pin0),
      _pin1(pin1),
      _pin2(pin2),
      _state(State::Idle),
      _pendingButton(-1),
      _activeButton(-1),
      _pressStartTime(0),
      _lastHoldTime(0),
      _lastPressedTime(0) {}

void BinaryMatrixHandler::begin() {
    pinMode(_pin0, INPUT_PULLUP);
    pinMode(_pin1, INPUT_PULLUP);
    pinMode(_pin2, INPUT_PULLUP);
}

int8_t BinaryMatrixHandler::readRawButton() const {
    uint8_t bit0 = (digitalRead(_pin0) == LOW) ? 1 : 0;
    uint8_t bit1 = (digitalRead(_pin1) == LOW) ? 2 : 0;
    uint8_t bit2 = (digitalRead(_pin2) == LOW) ? 4 : 0;
    uint8_t code = bit0 | bit1 | bit2;

    if (code == 0) {
        return -1; // No button pressed
    }
    // Pattern 1 to 7 maps to button index 0 to 6
    return (int8_t)(code - 1);
}

bool BinaryMatrixHandler::checkEvent(uint8_t& buttonIndex, ActionType& actionType) {
    int8_t currentRaw = readRawButton();
    uint32_t now = millis();

    if (currentRaw != -1) {
        _lastPressedTime = now;
    }

    switch (_state) {
        case State::Idle:
            if (currentRaw != -1) {
                _state = State::Debounce;
                _pendingButton = currentRaw;
                _pressStartTime = now;
            }
            break;

        case State::Debounce:
            if (now - _pressStartTime >= DEBOUNCE_MS) {
                if (currentRaw == _pendingButton) {
                    _state = State::Pressed;
                    _activeButton = _pendingButton;
                } else if (currentRaw != -1) {
                    // Changed during debounce (e.g. diode contact skew settling)
                    _pendingButton = currentRaw;
                    _pressStartTime = now;
                } else {
                    _state = State::Idle;
                    _pendingButton = -1;
                }
            }
            break;

        case State::Pressed:
            if (currentRaw != _activeButton) {
                if (now - _lastPressedTime >= DEBOUNCE_MS) {
                    // Button released early -> Click event
                    buttonIndex = (uint8_t)_activeButton;
                    actionType = ActionType::Click;
                    _state = State::Idle;
                    _activeButton = -1;
                    _pendingButton = -1;
                    return true;
                }
            } else if (now - _pressStartTime >= HOLD_THRESHOLD_MS) {
                // Maintained pressed beyond threshold -> Start holding
                _state = State::Holding;
                _lastHoldTime = now;
                buttonIndex = (uint8_t)_activeButton;
                actionType = ActionType::StartHold;
                return true;
            }
            break;

        case State::Holding:
            if (currentRaw != _activeButton) {
                if (now - _lastPressedTime >= DEBOUNCE_MS) {
                    // Button released after a hold -> Release event
                    buttonIndex = (uint8_t)_activeButton;
                    actionType = ActionType::Release;
                    _state = State::Idle;
                    _activeButton = -1;
                    _pendingButton = -1;
                    return true;
                }
            } else if (now - _lastHoldTime >= HOLD_PERIOD_MS) {
                // Periodically repeat hold event while finger remains on button
                _lastHoldTime = now;
                buttonIndex = (uint8_t)_activeButton;
                actionType = ActionType::StartHold;
                return true;
            }
            break;
    }

    return false;
}
