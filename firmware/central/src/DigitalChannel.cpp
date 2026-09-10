#include "DigitalChannel.h"
#include <Arduino.h>

DigitalChannel::DigitalChannel(const char* name, uint8_t pin, bool activeLow)
    : Channel(name, pin),
      _activeLow(activeLow),
      _autoOffTimeoutMs(0),
      _onStartTime(0) {}

void DigitalChannel::begin() {
    pinMode(_pin, OUTPUT);
    bool pinState = _activeLow ? !_isActive : _isActive;
    digitalWrite(_pin, pinState ? HIGH : LOW);
}

void DigitalChannel::handleAction(ActionType action, int8_t rotationSteps) {
    // Digital outputs support basic ON/OFF toggle on a Click event
    if (action == ActionType::Click) {
        setState(!_isActive);
    }
}

void DigitalChannel::setState(bool active) {
    if (active != _isActive) {
        if (active) {
            _onStartTime = millis();
        }
        _isActive = active;
        bool pinState = _activeLow ? !_isActive : _isActive;
        digitalWrite(_pin, pinState ? HIGH : LOW);
        notifyStateChanged();
    } else {
        bool pinState = _activeLow ? !_isActive : _isActive;
        digitalWrite(_pin, pinState ? HIGH : LOW);
    }
}

void DigitalChannel::setAutoOffTimeout(uint32_t timeoutMs) {
    _autoOffTimeoutMs = timeoutMs;
}

uint32_t DigitalChannel::getAutoOffTimeout() const {
    return _autoOffTimeoutMs;
}

uint32_t DigitalChannel::getRemainingTime() const {
    if (!_isActive || _autoOffTimeoutMs == 0) return 0;
    uint32_t elapsed = millis() - _onStartTime;
    if (elapsed >= _autoOffTimeoutMs) return 0;
    return _autoOffTimeoutMs - elapsed;
}

void DigitalChannel::update() {
    if (_isActive && _autoOffTimeoutMs > 0) {
        if (millis() - _onStartTime >= _autoOffTimeoutMs) {
            Serial.printf("[Safety Timer] Auto-off timeout reached for channel: %s (%u ms)\n",
                          _name, _autoOffTimeoutMs);
            setState(false);
        }
    }
}
