#include "DigitalChannel.h"
#include <Arduino.h>

DigitalChannel::DigitalChannel(const char* name, uint8_t pin, bool activeLow)
    : Channel(name, pin),
      _activeLow(activeLow),
      _autoOffTimeoutMs(0),
      _onStartTime(0),
      _chirpState(ChirpState::Idle),
      _chirpStepStartTime(0),
      _timedRunDurationMs(0),
      _isTimedRun(false) {}

void DigitalChannel::begin() {
    pinMode(_pin, OUTPUT);
    bool pinState = _activeLow ? !_isActive : _isActive;
    digitalWrite(_pin, pinState ? HIGH : LOW);
}

void DigitalChannel::handleAction(ActionType action, int8_t rotationSteps) {
    (void)rotationSteps;
    // Digital outputs only toggle on Click. Timed shower / chirp is pump-specific
    // and is invoked exclusively through SystemController::activateTimer().
    if (action == ActionType::Click) {
        setState(!_isActive);
    }
}

void DigitalChannel::setState(bool active) {
    if (!active) {
        _isTimedRun = false;
        _chirpState = ChirpState::Idle;
    }
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

void DigitalChannel::activateTimer(uint32_t durationMs, bool enableChirp) {
    uint32_t now = millis();
    _timedRunDurationMs = durationMs;
    _isTimedRun = true;
    _isActive = true;
    _onStartTime = now;

    if (enableChirp) {
        _chirpState = ChirpState::Pulse1_ON;
        _chirpStepStartTime = now;
        bool pinState = _activeLow ? false : true;
        digitalWrite(_pin, pinState ? HIGH : LOW);
    } else {
        _chirpState = ChirpState::Running;
        bool pinState = _activeLow ? false : true;
        digitalWrite(_pin, pinState ? HIGH : LOW);
    }
    notifyStateChanged();
}

bool DigitalChannel::isTimedActive() const {
    return _isActive && _isTimedRun;
}

void DigitalChannel::cancelTimer() {
    setState(false);
}

void DigitalChannel::setAutoOffTimeout(uint32_t timeoutMs) {
    _autoOffTimeoutMs = timeoutMs;
}

uint32_t DigitalChannel::getAutoOffTimeout() const {
    return _autoOffTimeoutMs;
}

uint32_t DigitalChannel::getRemainingTime() const {
    if (!_isActive) return 0;
    uint32_t now = millis();
    if (_isTimedRun) {
        uint32_t elapsed = now - _onStartTime;
        if (elapsed >= _timedRunDurationMs) return 0;
        return _timedRunDurationMs - elapsed;
    }
    if (_autoOffTimeoutMs == 0) return 0;
    uint32_t elapsed = now - _onStartTime;
    if (elapsed >= _autoOffTimeoutMs) return 0;
    return _autoOffTimeoutMs - elapsed;
}

void DigitalChannel::update() {
    uint32_t now = millis();

    if (_isActive && _isTimedRun) {
        switch (_chirpState) {
            case ChirpState::Pulse1_ON:
                if (now - _chirpStepStartTime >= 150) {
                    _chirpState = ChirpState::Pulse1_OFF;
                    _chirpStepStartTime = now;
                    bool pinState = _activeLow ? true : false;
                    digitalWrite(_pin, pinState ? HIGH : LOW);
                }
                break;

            case ChirpState::Pulse1_OFF:
                if (now - _chirpStepStartTime >= 120) {
                    _chirpState = ChirpState::Pulse2_ON;
                    _chirpStepStartTime = now;
                    bool pinState = _activeLow ? false : true;
                    digitalWrite(_pin, pinState ? HIGH : LOW);
                }
                break;

            case ChirpState::Pulse2_ON:
                if (now - _chirpStepStartTime >= 150) {
                    _chirpState = ChirpState::Pulse2_OFF;
                    _chirpStepStartTime = now;
                    bool pinState = _activeLow ? true : false;
                    digitalWrite(_pin, pinState ? HIGH : LOW);
                }
                break;

            case ChirpState::Pulse2_OFF:
                if (now - _chirpStepStartTime >= 120) {
                    _chirpState = ChirpState::Running;
                    _chirpStepStartTime = now;
                    bool pinState = _activeLow ? false : true;
                    digitalWrite(_pin, pinState ? HIGH : LOW);
                }
                break;

            case ChirpState::Running:
                if (now - _onStartTime >= _timedRunDurationMs) {
                    Serial.printf("[Shower Timer] Timed run completed for: %s (%u ms)\n",
                                  _name, _timedRunDurationMs);
                    setState(false);
                }
                break;

            default:
                break;
        }
        return;
    }

    if (_isActive && _autoOffTimeoutMs > 0) {
        if (now - _onStartTime >= _autoOffTimeoutMs) {
            Serial.printf("[Safety Timer] Auto-off timeout reached for channel: %s (%u ms)\n",
                          _name, _autoOffTimeoutMs);
            setState(false);
        }
    }
}

