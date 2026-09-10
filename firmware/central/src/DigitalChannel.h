#ifndef DIGITAL_CHANNEL_H
#define DIGITAL_CHANNEL_H

#include "Channel.h"

class DigitalChannel : public Channel {
public:
    DigitalChannel(const char* name, uint8_t pin, bool activeLow = false);

    void begin() override;
    void handleAction(ActionType action, int8_t rotationSteps) override;
    void setState(bool active) override;
    void update() override;

    // Auto-off safety timer (timeoutMs = 0 disables auto-off)
    void setAutoOffTimeout(uint32_t timeoutMs);
    uint32_t getAutoOffTimeout() const;
    uint32_t getRemainingTime() const;

private:
    bool _activeLow;
    uint32_t _autoOffTimeoutMs;
    uint32_t _onStartTime;
};

#endif // DIGITAL_CHANNEL_H
