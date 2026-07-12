#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "Channel.h"
#include "DigitalChannel.h"
#include "DimmableChannel.h"
#include "protocol.h"

class SystemController {
public:
    SystemController();
    ~SystemController();

    // Initialize all registered channels and system pins
    void begin();

    // Update all channels (called in loop for transitions and timeouts)
    void update();

    // Dispatch incoming SwitchMessage commands
    void dispatchMessage(const uint8_t* senderMac, const SwitchMessage& msg);

    // Read the main 12V battery voltage (analog divisor read on GPIO 1)
    float readMainBatteryVoltage();

private:
    static constexpr uint8_t NUM_CHANNELS = 11;
    Channel* _channels[NUM_CHANNELS];

    // Configuration parameters for main battery reading
    static constexpr uint8_t BATTERY_ADC_PIN = 1;
    static constexpr float DIVIDER_RATIO = (100.0f + 18.0f) / 18.0f; // Divisor resistor values: 100k and 18k
};

#endif // SYSTEM_CONTROLLER_H
