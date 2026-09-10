#include "DimmableChannel.h"
#include <Arduino.h>

DimmableChannel::DimmableChannel(const char* name, uint8_t pin, uint32_t frequency, uint8_t resolution)
    : Channel(name, pin),
      _frequency(frequency),
      _resolution(resolution),
      _maxDuty((1 << resolution) - 1),
      _currentBrightness(0),
      _targetBrightness(0),
      _lastOnBrightness(80), // Default target brightness when turning ON
      _isRamping(false),
      _rampDirection(1),
      _lastRampTime(0),
      _lastHoldMsgTime(0),
      _dirty(false),
      _lastChangeTime(0) {}

void DimmableChannel::begin() {
    // Arduino ESP32 Core 3.x ledcAttach automatically configures and binds a channel to the pin
    ledcAttach(_pin, _frequency, _resolution);
    uint32_t duty = (uint32_t)((_currentBrightness / 100.0) * _maxDuty);
    ledcWrite(_pin, duty);
}

void DimmableChannel::handleAction(ActionType action, int8_t rotationSteps) {
    uint32_t now = millis();

    switch (action) {
        case ActionType::Click:
            _isRamping = false;
            setState(!_isActive);
            break;

        case ActionType::StartHold:
            // Invert ramp direction only at the start of hold, not on periodic keep-alives
            if (!_isRamping) {
                _isRamping = true;
                _rampDirection = -_rampDirection;
            }
            _lastHoldMsgTime = now;
            break;

        case ActionType::Release:
            if (_isRamping) {
                _isRamping = false;
                _lastOnBrightness = _currentBrightness > 5 ? _currentBrightness : 80;
                _dirty = false;
                notifyStateChanged();
            }
            break;

        case ActionType::EncoderTurn:
            _isRamping = false;
            if (rotationSteps != 0) {
                int16_t newBright = (int16_t)_currentBrightness + (rotationSteps * 5);
                if (newBright > 100) newBright = 100;
                if (newBright < 0) newBright = 0;
                setBrightness(newBright);
                _dirty = true;
                _lastChangeTime = now;
            }
            break;
    }
}

void DimmableChannel::setState(bool active) {
    if (_isActive != active) {
        _isActive = active;
        if (_isActive) {
            _targetBrightness = _lastOnBrightness;
        } else {
            _targetBrightness = 0;
        }
        notifyStateChanged();
    }
}

uint8_t DimmableChannel::getLastOnBrightness() const {
    return _lastOnBrightness;
}

void DimmableChannel::setLastOnBrightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    if (brightness < 5) brightness = 5;
    _lastOnBrightness = brightness;
}

void DimmableChannel::setBrightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    _targetBrightness = brightness;
    _currentBrightness = brightness;

    // Convert brightness percentage to raw LEDC duty cycle
    uint32_t duty = (uint32_t)((_currentBrightness / 100.0) * _maxDuty);
    ledcWrite(_pin, duty);

    if (_currentBrightness > 0) {
        _isActive = true;
        _lastOnBrightness = _currentBrightness;
    } else {
        _isActive = false;
    }

    if (!_isRamping) {
        _dirty = true;
        _lastChangeTime = millis();
    }
}

uint8_t DimmableChannel::getBrightness() const {
    return _currentBrightness;
}

void DimmableChannel::update() {
    uint32_t now = millis();

    // 1. Process continuous dimming hold ramp
    if (_isRamping) {
        // Safe timeout: release ramping if we lose hold messages
        if (now - _lastHoldMsgTime > HOLD_TIMEOUT_MS) {
            _isRamping = false;
            _lastOnBrightness = _currentBrightness > 5 ? _currentBrightness : 80;
            _dirty = false;
            notifyStateChanged();
        } else if (now - _lastRampTime >= RAMP_INTERVAL_MS) {
            _lastRampTime = now;
            int16_t newBright = (int16_t)_currentBrightness + _rampDirection;
            
            // Auto-reverse ramp direction at limits
            if (newBright >= 100) {
                newBright = 100;
                _rampDirection = -1;
            } else if (newBright <= 5) {
                newBright = 5; // Do not turn off light fully when ramping
                _rampDirection = 1;
            }
            setBrightness(newBright);
        }
        return;
    }

    // Debounce settle timer for encoder rotations: persist only after input stabilizes
    if (_dirty && (now - _lastChangeTime >= SETTLE_TIMEOUT_MS)) {
        _dirty = false;
        notifyStateChanged();
    }

    // 2. Process smooth click fade-to-target
    if (_currentBrightness != _targetBrightness) {
        if (now - _lastRampTime >= FADE_INTERVAL_MS) {
            _lastRampTime = now;
            if (_currentBrightness < _targetBrightness) {
                _currentBrightness++;
            } else {
                _currentBrightness--;
            }

            uint32_t duty = (uint32_t)((_currentBrightness / 100.0) * _maxDuty);
            ledcWrite(_pin, duty);

            _isActive = (_currentBrightness > 0);
        }
    }
}
