#include "DimmableChannel.h"

DimmableChannel::DimmableChannel(const char* name, uint8_t pin, uint8_t channel, uint32_t frequency, uint8_t resolution)
    : Channel(name, pin),
      _channel(channel),
      _frequency(frequency),
      _resolution(resolution),
      _maxDuty((1 << resolution) - 1),
      _currentBrightness(0),
      _targetBrightness(0),
      _lastOnBrightness(80), // Default target brightness when turning ON
      _isRamping(false),
      _rampDirection(1),
      _lastRampTime(0),
      _lastHoldMsgTime(0) {}

void DimmableChannel::begin() {
    // Configure and setup LEDC channel and attach physical pin
    ledcSetup(_channel, _frequency, _resolution);
    ledcAttachPin(_pin, _channel);
    ledcWrite(_channel, 0);
    _isActive = false;
    _currentBrightness = 0;
    _targetBrightness = 0;
}

void DimmableChannel::handleAction(ActionType action, int8_t rotationSteps) {
    uint32_t now = millis();

    switch (action) {
        case ActionType::Click:
            _isRamping = false;
            setState(!_isActive);
            break;

        case ActionType::StartHold:
            _isRamping = true;
            _lastHoldMsgTime = now;
            // Invert ramp direction at the start of hold to toggle dim/brighten
            _rampDirection = -_rampDirection;
            break;

        case ActionType::Release:
            if (_isRamping) {
                _isRamping = false;
                _lastOnBrightness = _currentBrightness > 5 ? _currentBrightness : 80;
            }
            break;

        case ActionType::EncoderTurn:
            _isRamping = false;
            if (rotationSteps != 0) {
                int16_t newBright = (int16_t)_currentBrightness + (rotationSteps * 5);
                if (newBright > 100) newBright = 100;
                if (newBright < 0) newBright = 0;
                setBrightness(newBright);
            }
            break;
    }
}

void DimmableChannel::setState(bool active) {
    _isActive = active;
    if (_isActive) {
        _targetBrightness = _lastOnBrightness;
    } else {
        _targetBrightness = 0;
    }
}

void DimmableChannel::setBrightness(uint8_t brightness) {
    if (brightness > 100) brightness = 100;
    _targetBrightness = brightness;
    _currentBrightness = brightness;

    // Convert brightness percentage to raw LEDC duty cycle
    uint32_t duty = (uint32_t)((_currentBrightness / 100.0) * _maxDuty);
    ledcWrite(_channel, duty);

    if (_currentBrightness > 0) {
        _isActive = true;
        _lastOnBrightness = _currentBrightness;
    } else {
        _isActive = false;
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
            ledcWrite(_channel, duty);

            _isActive = (_currentBrightness > 0);
        }
    }
}
