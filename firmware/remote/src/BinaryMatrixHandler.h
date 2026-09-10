#ifndef BINARY_MATRIX_HANDLER_H
#define BINARY_MATRIX_HANDLER_H

#include <Arduino.h>
#include "protocol.h"

/**
 * @brief Handles up to 7 buttons multiplexed onto 3 digital pins using a diode binary matrix.
 * 
 * Uses pins D0, D1, D2 (LP-GPIOs on ESP32-C6).
 * Any pressed button pulls a unique combination of pins LOW.
 * Supports short click, continuous hold (repeating every 150 ms), and release events.
 */
class BinaryMatrixHandler {
public:
    /**
     * @param pin0 Least significant bit pin (D0 / LP-GPIO 0)
     * @param pin1 Second bit pin (D1 / LP-GPIO 1)
     * @param pin2 Most significant bit pin (D2 / LP-GPIO 2)
     */
    BinaryMatrixHandler(uint8_t pin0, uint8_t pin1, uint8_t pin2);

    void begin();

    /**
     * @brief Polls matrix status and emits events.
     * @param buttonIndex Output: decoded button index (0 to 6)
     * @param actionType Output: Click, StartHold, or Release
     * @return true if an event was generated, false otherwise
     */
    bool checkEvent(uint8_t& buttonIndex, ActionType& actionType);

    /**
     * @brief Reads the current raw decoded button index without debounce (0-6, or -1 if none).
     */
    int8_t readRawButton() const;

private:
    uint8_t _pin0;
    uint8_t _pin1;
    uint8_t _pin2;

    enum class State {
        Idle,
        Debounce,
        Pressed,
        Holding
    };

    State _state;
    int8_t _pendingButton;
    int8_t _activeButton;
    uint32_t _pressStartTime;
    uint32_t _lastHoldTime;
    uint32_t _lastPressedTime;

    static constexpr uint32_t DEBOUNCE_MS = 15;
    static constexpr uint32_t HOLD_THRESHOLD_MS = 400; // ms before click becomes hold
    static constexpr uint32_t HOLD_PERIOD_MS = 150;     // ms between repeated hold events
};

#endif // BINARY_MATRIX_HANDLER_H
