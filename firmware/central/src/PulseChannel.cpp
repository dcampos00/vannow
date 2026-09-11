#include "PulseChannel.h"
#include <Arduino.h>

PulseChannel::PulseChannel(const char* name, uint8_t pin, uint32_t pulseDurationMs, bool activeLow)
    : DigitalChannel(name, pin, activeLow),
      _pulseDurationMs(pulseDurationMs) {
    // Momentary pulses must NEVER restore active across reboots
    _restoreOnBoot = false;
}

void PulseChannel::handleAction(ActionType action, int8_t rotationSteps) {
    (void)rotationSteps;
    // Any click, double click, or hold event triggers the momentary pulse
    if (action == ActionType::Click || action == ActionType::DoubleClick || action == ActionType::StartHold) {
        trigger();
    }
}

void PulseChannel::trigger() {
    Serial.printf("[PulseChannel] Triggered %s (Pin %d) for %u ms\n", _name, _pin, _pulseDurationMs);
    activateTimer(_pulseDurationMs, false);
}

uint32_t PulseChannel::getPulseDuration() const {
    return _pulseDurationMs;
}

void PulseChannel::setPulseDuration(uint32_t ms) {
    _pulseDurationMs = ms;
}
