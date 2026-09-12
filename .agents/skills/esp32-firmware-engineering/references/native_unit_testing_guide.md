# Native Unit Testing (PlatformIO + Arduino mock)

Host tests must stay under ~1 s and must exercise the gesture that the production bug would hit (especially **repeated `StartHold`**).

## Layout

```
firmware/
├── central/  platformio.ini  [env:native]  test/test_channels/
├── remote/   platformio.ini  [env:native]  test/test_remote_handlers/
└── lib/arduino_mock/         Arduino.h, Preferences.h, esp_sleep.h
```

Central native env ignores `main.cpp` and `wireless`. Remote native env also ignores `RemoteSender.cpp`.

## Mock APIs (real names)

```cpp
#include <unity.h>
#include <Arduino.h>
#include <Preferences.h>
#include "SystemController.h"

void setUp(void) {
    ArduinoMock::reset();
    Preferences::resetMockStorage();  // required: anti-replay now writes NVS
}

void test_shower_keepalives_do_not_cancel(void) {
    SystemController controller;
    controller.begin();
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 4; // entrance pump
    msg.action = (uint8_t)ActionType::StartHold;
    msg.seq = 10;
    uint8_t mac[6] = {1,2,3,4,5,6};
    controller.dispatchMessage(mac, msg);
    msg.seq = 11;
    controller.dispatchMessage(mac, msg);
    DigitalChannel* pump = static_cast<DigitalChannel*>(controller.getChannel(4));
    TEST_ASSERT_TRUE(pump->isTimedActive());
}
```

Time: `ArduinoMock::advanceMillis(ms)` then `channel.update()` / `controller.update()`.  
Pins: `ArduinoMock::setPinState(pin, LOW)` / `getPinState` / `getLEDCWrite`.

`DimmableChannel` constructor is `(const char* name, uint8_t pin)`. There is no `handleHoldStart()` — use `handleAction(ActionType::StartHold, 0)`.

## Required coverage when touching FSMs

| Change | Test that must exist |
| :--- | :--- |
| Pump shower | `StartHold` + 150 ms + `StartHold` still timed-active; `Click` cancels |
| Any `DigitalChannel` | `StartHold` / `DoubleClick` do **not** start a timer |
| `PulseChannel` | `StartHold` does not `trigger()` |
| Encoder | button 7 dims focused zone, not Ch 0 by default after another zone click |
| `ButtonHandler` | `Click` after 320 ms; `DoubleClick` on second press; latching emits Click on both edges |
| Anti-replay | Reboot `SystemController` rejects a repeated `seq` |

## Commands

```bash
pio test -d firmware/central -e native
pio test -d firmware/remote -e native
```

Gate: 100% pass before commit. Current suites: 25 central + 11 remote.
