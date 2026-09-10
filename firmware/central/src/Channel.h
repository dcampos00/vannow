#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdint.h>
#include "protocol.h"

class Channel {
public:
    using StateChangeCallback = void (*)(uint8_t channelIndex, void* context);

    Channel(const char* name, uint8_t pin);
    virtual ~Channel() = default;

    // Initialize physical pin or hardware settings
    virtual void begin() = 0;

    // Handle incoming actions sent by the remote
    virtual void handleAction(ActionType action, int8_t rotationSteps) = 0;

    // Control the state directly
    virtual void setState(bool active) = 0;

    // Polymorphic update called in main loop for transitions, timers, and debounce
    virtual void update() {}

    // Retrieve state information
    bool getState() const;
    const char* getName() const;
    uint8_t getPin() const;

    // Check if the channel is dimmable (overridden by DimmableChannel)
    virtual bool isDimmable() const { return false; }

    // State persistence control across reboots
    void setRestoreOnBoot(bool restore);
    bool shouldRestoreOnBoot() const;

    // Register callback triggered when state or settled dimming level changes
    void setChangeCallback(StateChangeCallback cb, uint8_t channelIndex, void* context);

protected:
    void notifyStateChanged();

    const char* _name;
    uint8_t _pin;
    bool _isActive;
    bool _restoreOnBoot;

    StateChangeCallback _changeCallback;
    void* _changeContext;
    uint8_t _channelIndex;
};

#endif // CHANNEL_H
