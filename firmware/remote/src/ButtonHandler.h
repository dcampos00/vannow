#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "protocol.h"

class ButtonHandler {
public:
    enum class InputMode : uint8_t {
        Momentary, // Click / DoubleClick / StartHold keep-alives (entrance panel)
        Latching   // Click on each edge only (cockpit Carling rockers)
    };

    ButtonHandler(uint8_t pin, uint8_t buttonIndex, InputMode mode = InputMode::Momentary);

    void begin();
    bool checkEvent(ActionType& actionType);

private:
    uint8_t _pin;
    uint8_t _buttonIndex;
    InputMode _mode;

    enum class State {
        Idle,
        Debounce,
        Pressed,
        WaitDoubleClick,
        DebounceSecond,
        Holding,
        LatchingHeld
    };

    State _state;
    uint32_t _pressStartTime;
    uint32_t _lastHoldTime;
    uint32_t _lastPressedTime;
    uint32_t _releaseTime;

    static constexpr uint32_t DEBOUNCE_MS = 15;
    static constexpr uint32_t HOLD_THRESHOLD_MS = 400;
    static constexpr uint32_t HOLD_PERIOD_MS = 150;
    static constexpr uint32_t DOUBLE_CLICK_MS = 320;

    bool handleMomentary(bool pinPressed, uint32_t now, ActionType& actionType);
    bool handleLatching(bool pinPressed, uint32_t now, ActionType& actionType);
};

#endif // BUTTON_HANDLER_H
