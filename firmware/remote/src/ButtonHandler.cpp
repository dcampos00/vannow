#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(uint8_t pin, uint8_t buttonIndex, InputMode mode)
    : _pin(pin),
      _buttonIndex(buttonIndex),
      _mode(mode),
      _state(State::Idle),
      _pressStartTime(0),
      _lastHoldTime(0),
      _lastPressedTime(0),
      _releaseTime(0) {}

void ButtonHandler::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

bool ButtonHandler::checkEvent(ActionType& actionType) {
    bool pinPressed = (digitalRead(_pin) == LOW);
    uint32_t now = millis();

    if (pinPressed) {
        _lastPressedTime = now;
    }

    if (_mode == InputMode::Latching) {
        return handleLatching(pinPressed, now, actionType);
    }
    return handleMomentary(pinPressed, now, actionType);
}

bool ButtonHandler::handleLatching(bool pinPressed, uint32_t now, ActionType& actionType) {
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
                    _state = State::LatchingHeld;
                    actionType = ActionType::Click;
                    return true;
                }
                _state = State::Idle;
            }
            break;

        case State::LatchingHeld:
            if (!pinPressed && (now - _lastPressedTime >= DEBOUNCE_MS)) {
                _state = State::Idle;
                actionType = ActionType::Click;
                return true;
            }
            break;

        default:
            _state = State::Idle;
            break;
    }
    return false;
}

bool ButtonHandler::handleMomentary(bool pinPressed, uint32_t now, ActionType& actionType) {
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
                _state = State::WaitDoubleClick;
                _releaseTime = now;
            } else if (now - _pressStartTime >= HOLD_THRESHOLD_MS) {
                _state = State::Holding;
                _lastHoldTime = now;
                actionType = ActionType::StartHold;
                return true;
            }
            break;

        case State::WaitDoubleClick:
            if (pinPressed) {
                _state = State::DebounceSecond;
                _pressStartTime = now;
            } else if (now - _releaseTime >= DOUBLE_CLICK_MS) {
                _state = State::Idle;
                actionType = ActionType::Click;
                return true;
            }
            break;

        case State::DebounceSecond:
            if (now - _pressStartTime >= DEBOUNCE_MS) {
                if (pinPressed) {
                    _state = State::Idle;
                    actionType = ActionType::DoubleClick;
                    return true;
                }
                _state = State::WaitDoubleClick;
            }
            break;

        case State::Holding:
            if (now - _lastPressedTime >= DEBOUNCE_MS) {
                _state = State::Idle;
                actionType = ActionType::Release;
                return true;
            }
            if (now - _lastHoldTime >= HOLD_PERIOD_MS) {
                _lastHoldTime = now;
                actionType = ActionType::StartHold;
                return true;
            }
            break;

        default:
            _state = State::Idle;
            break;
    }
    return false;
}
