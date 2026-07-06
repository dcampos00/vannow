#include "DigitalChannel.h"
#include <Arduino.h>

DigitalChannel::DigitalChannel(const char* name, uint8_t pin, bool activeLow)
    : Channel(name, pin), _activeLow(activeLow) {}

void DigitalChannel::begin() {
    pinMode(_pin, OUTPUT);
    setState(false); // Default to off state on boot
}

void DigitalChannel::handleAction(ActionType action, int8_t rotationSteps) {
    // Digital outputs support basic ON/OFF toggle on a Click event
    if (action == ActionType::Click) {
        setState(!_isActive);
    }
}

void DigitalChannel::setState(bool active) {
    _isActive = active;
    bool pinState = _activeLow ? !_isActive : _isActive;
    digitalWrite(_pin, pinState ? HIGH : LOW);
}
