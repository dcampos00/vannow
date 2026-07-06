#include "Channel.h"

Channel::Channel(const char* name, uint8_t pin)
    : _name(name), _pin(pin), _isActive(false) {}

bool Channel::getState() const {
    return _isActive;
}

const char* Channel::getName() const {
    return _name;
}

uint8_t Channel::getPin() const {
    return _pin;
}
