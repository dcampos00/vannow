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

    // Timed activation (e.g. 5-minute shower mode with acoustic chirp confirmation)
    void activateTimer(uint32_t durationMs, bool enableChirp = false);
    bool isTimedActive() const;
    void cancelTimer();

private:
    bool _activeLow;
    uint32_t _autoOffTimeoutMs;
    uint32_t _onStartTime;

    enum class ChirpState : uint8_t {
        Idle,
        Pulse1_ON,
        Pulse1_OFF,
        Pulse2_ON,
        Pulse2_OFF,
        Running
    };

    ChirpState _chirpState;
    uint32_t _chirpStepStartTime;
    uint32_t _timedRunDurationMs;
    bool _isTimedRun;
};

#endif // DIGITAL_CHANNEL_H
