#ifndef CHANNEL_H
#define CHANNEL_H

#include <Arduino.h>
#include "protocol.h"

class Channel {
public:
    Channel(const char* name, uint8_t pin);
    virtual ~Channel() = default;

    // Initialize physical pin or hardware settings
    virtual void begin() = 0;

    // Handle incoming actions sent by the remote
    virtual void handleAction(ActionType action, int8_t rotationSteps) = 0;

    // Control the state directly
    virtual void setState(bool active) = 0;

    // Retrieve state information
    bool getState() const;
    const char* getName() const;
    uint8_t getPin() const;

protected:
    const char* _name;
    uint8_t _pin;
    bool _isActive;
};

#endif // CHANNEL_H
