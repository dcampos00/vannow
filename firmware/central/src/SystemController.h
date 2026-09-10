#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "Channel.h"
#include "DigitalChannel.h"
#include "DimmableChannel.h"
#include "protocol.h"

#include "AntiReplayFilter.h"
#include <Preferences.h>

class SystemController {
public:
    SystemController();
    ~SystemController();

    // Initialize all registered channels, safety timers, and restore persisted states
    void begin();

    // Update all channels (called in loop for transitions, timers, and debounce)
    void update();

    // Dispatch incoming SwitchMessage commands
    void dispatchMessage(const uint8_t* senderMac, const SwitchMessage& msg);

    // Read the main 12V battery voltage (analog divisor read on GPIO 1)
    float readMainBatteryVoltage();

    // Replay protection access
    AntiReplayFilter& getAntiReplayFilter();

    // Channel accessors for status inspection and testing
    Channel* getChannel(uint8_t index) const;
    static constexpr uint8_t NUM_CHANNELS = 11;

    // NVS state persistence operations
    void loadPersistedStates();
    void saveChannelState(uint8_t channelIndex);

private:
    static void onChannelChanged(uint8_t channelIndex, void* context);

    Channel* _channels[NUM_CHANNELS];
    AntiReplayFilter _antiReplay;

    // Configuration parameters for main battery reading
    static constexpr uint8_t BATTERY_ADC_PIN = 1;
    static constexpr float DIVIDER_RATIO = (100.0f + 18.0f) / 18.0f; // Divisor resistor values: 100k and 18k
};

#endif // SYSTEM_CONTROLLER_H
