#include "Channel.h"

Channel::Channel(const char* name, uint8_t pin)
    : _name(name),
      _pin(pin),
      _isActive(false),
      _restoreOnBoot(true),
      _changeCallback(nullptr),
      _changeContext(nullptr),
      _channelIndex(0) {}

bool Channel::getState() const {
    return _isActive;
}

const char* Channel::getName() const {
    return _name;
}

uint8_t Channel::getPin() const {
    return _pin;
}

void Channel::setRestoreOnBoot(bool restore) {
    _restoreOnBoot = restore;
}

bool Channel::shouldRestoreOnBoot() const {
    return _restoreOnBoot;
}

void Channel::setChangeCallback(StateChangeCallback cb, uint8_t channelIndex, void* context) {
    _changeCallback = cb;
    _channelIndex = channelIndex;
    _changeContext = context;
}

void Channel::notifyStateChanged() {
    if (_changeCallback) {
        _changeCallback(_channelIndex, _changeContext);
    }
}
