# ESP32-C6 Deep Sleep & XIAO Pin Rules

EXT1 wakeup works **only** on LP-GPIO 0–7. The HP core (Wi-Fi, most header pins) is off in deep sleep.

## Seeed XIAO ESP32-C6 header (authoritative)

From Seeed wiki and `variants/XIAO_ESP32C6/pins_arduino.h`:

| Silk | GPIO | Domain | EXT1 wake? |
| :--- | :---: | :--- | :---: |
| D0 | 0 | LP | Yes — Diode-OR wake bus |
| D1 | 1 | LP | Yes — encoder A (entrance) / SW1 (cockpit) |
| D2 | 2 | LP | Yes — encoder B / SW2 |
| D3 | **21** | HP | **No** |
| D4 | **22** | HP | **No** |
| D5 | **23** | HP | **No** |
| D6 | 16 | HP | No |
| D7 | 17 | HP | No |
| D8 | 19 | HP | No |
| D9 | 20 | HP | No — encoder SW (entrance) |
| D10 | **18** | HP | No — status LED. **Not GPIO 9** |
| — | 9 | BOOT strap | Do not drive as LED |
| — | 3 | WIFI_ENABLE (internal) | Do not use |

GPIO 4–7 are JTAG test pads, not D4/D5. Do not document D4 as GPIO 4.

## Diode-OR

All tactile/rocker lines diode-OR to D0 (GPIO 0), 47 kΩ pull-up to 3.3 V. Anode on the wake bus, cathode on the sense pin (pulled to GND when pressed). Sense GPIOs identify which switch after wake.

Entrance also wakes on encoder A/B (GPIO 1, 2). Cockpit wakes on GPIO 0 only.

## Sleep sequence

1. `pinMode(wakePin, INPUT_PULLUP)` and `gpio_pullup_en` **before** `esp_deep_sleep_start`.
2. Persist remote sequence (`RemoteSender::persistSequence()`) before sleep.
3. `esp_now_deinit` / `esp_wifi_stop` / `esp_wifi_deinit`, then `esp_deep_sleep_start`.
4. Cold boot (`wakeup != EXT1`): configure pull-ups, blink LED, sleep. Do not overwrite NVS `seq` with 0.

`PowerManager` inactivity timeout is **1500 ms**. Encoder poll is `delayMicroseconds(250)`, not `delay(5)`.

Remotes are powered from **2× AA into the `3V3` pin** (marginal for Wi-Fi TX). Do not assume a 500 mAh LiPo.
