#ifndef DIMMABLE_CHANNEL_H
#define DIMMABLE_CHANNEL_H

#include "Channel.h"

class DimmableChannel : public Channel {
public:
    DimmableChannel(const char* name, uint8_t pin, uint8_t channel, uint32_t frequency = 5000, uint8_t resolution = 8);

    void begin() override;
    void handleAction(ActionType action, int8_t rotationSteps) override;
    void setState(bool active) override;

    // Must be called in loop to update smooth fade transition and holds
    void update();

    // Sets target and current brightness directly (0 to 100)
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;

    bool isDimmable() const override { return true; }

private:
    uint8_t _channel;
    uint32_t _frequency;
    uint8_t _resolution;
    uint32_t _maxDuty;

    uint8_t _currentBrightness;   // 0 to 100
    uint8_t _targetBrightness;    // 0 to 100
    uint8_t _lastOnBrightness;    // Memory for toggle

    bool _isRamping;
    int8_t _rampDirection;        // 1 = up, -1 = down
    uint32_t _lastRampTime;
    uint32_t _lastHoldMsgTime;

    static constexpr uint32_t RAMP_INTERVAL_MS = 30; // Milliseconds per brightness step in hold mode
    static constexpr uint32_t FADE_INTERVAL_MS = 5;  // Milliseconds per brightness step in click mode
    static constexpr uint32_t HOLD_TIMEOUT_MS = 300; // Timeout to auto-release hold if packages stop
};

#endif // DIMMABLE_CHANNEL_H
