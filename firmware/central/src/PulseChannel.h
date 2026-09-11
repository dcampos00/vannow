#ifndef PULSE_CHANNEL_H
#define PULSE_CHANNEL_H

#include "DigitalChannel.h"

class PulseChannel : public DigitalChannel {
public:
    PulseChannel(const char* name, uint8_t pin, uint32_t pulseDurationMs = 250, bool activeLow = false);

    void handleAction(ActionType action, int8_t rotationSteps) override;
    void trigger();

    uint32_t getPulseDuration() const;
    void setPulseDuration(uint32_t ms);

private:
    uint32_t _pulseDurationMs;
};

#endif // PULSE_CHANNEL_H
