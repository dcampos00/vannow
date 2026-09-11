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
    // Only discrete press events trigger a pulse. Ignore StartHold keep-alives
    // so a held button cannot retrigger Maxxair / generator crank every 150 ms.
    if (action == ActionType::Click || action == ActionType::DoubleClick) {
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
