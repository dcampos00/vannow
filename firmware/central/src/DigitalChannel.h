#ifndef DIGITAL_CHANNEL_H
#define DIGITAL_CHANNEL_H

#include "Channel.h"

class DigitalChannel : public Channel {
public:
    DigitalChannel(const char* name, uint8_t pin, bool activeLow = false);

    void begin() override;
    void handleAction(ActionType action, int8_t rotationSteps) override;
    void setState(bool active) override;

private:
    bool _activeLow;
};

#endif // DIGITAL_CHANNEL_H
