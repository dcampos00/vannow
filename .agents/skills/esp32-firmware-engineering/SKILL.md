---
name: esp32-firmware-engineering
description: >-
  Develops, refactors, and tests VanNOW firmware for the ESP32-S3 Central Controller and
  XIAO ESP32-C6 remotes. Use when changing channels, GPIOs, ESP-NOW packets, dimmer/shower
  FSMs, deep sleep, anti-replay, PlatformIO native tests, or flashing entrance/cockpit remotes.
---

# ESP32 Firmware Engineering (VanNOW)

Authoritative workflow for `firmware/central` (ESP32-S3 DevKitC-1 N16R8) and `firmware/remote` (Seeed XIAO ESP32-C6). **Alpha** target is the **24-channel** map (`VANNOW_CHANNEL_PROFILE=24`). Legacy 11-ch carrier: `-e esp32-s3-profet-11ch` only.

Pin/channel tables: [co_design_verification_matrix.md](../cross-domain-sync/references/co_design_verification_matrix.md).

---

## Hard Rules

1. **PROFET PWM is 200 Hz** (allowed 100–400 Hz). Never 5 kHz. See [profet_pwm_and_ledc.md](./references/profet_pwm_and_ledc.md).
2. **EXT1 wake = LP-GPIO 0–7 only.** XIAO `D3` is HP **GPIO 21** and cannot wake. `D10` is **GPIO 18**, not GPIO 9 (BOOT). See [esp32c6_deep_sleep_wakeups.md](./references/esp32c6_deep_sleep_wakeups.md).
3. **ESP-NOW `OnDataRecv` only enqueues.** Mutate channels in `loop()` after `xQueueReceive`.
4. **`StartHold` keep-alives (every 150 ms) are not new gestures.** Dimmers flip direction only on the first hold. Pump shower starts once; later `StartHold` is ignored. `PulseChannel` and generic `DigitalChannel` ignore `StartHold`.
5. **Shower/chirp is pump-only** via `SystemController` → `DigitalChannel::activateTimer()`. Never put shower logic in `DigitalChannel::handleAction`.
6. **Anti-replay:** `AntiReplayFilter::validateAndAdvance(remoteId, seq)`. Persist window to NVS (`vannow_replay`). Persist remote `seq` to NVS (`vannow_seq`) before sleep. `seq == 1` must not bypass the filter. See [esp_now_protocol_and_security.md](./references/esp_now_protocol_and_security.md).

---

## Workflow

1. Check GPIOs against the 24-channel matrix and DevKitC-1 v1.1 (avoid 19/20 USB, 0/3/45/46 straps, 33–37 octal SPI). GPIO 38 (v1.1) and 48 (v1.0) share the onboard RGB LED with optocoupler channels — do not initialize `RGB_BUILTIN`.
2. Implement the FSM. Protocol struct is `SwitchMessage` in `firmware/lib/protocol/protocol.h` (`__attribute__((packed))`): `remote_id`, `button_index`, `action`, `rotation_steps`, `battery_voltage`, `seq`.
3. Queue RX in `wifi_task`; dispatch in `loopTask`.
4. Add native tests that cover the gesture you changed (including keep-alive streams). Reset `Preferences::resetMockStorage()` in `setUp`. See [native_unit_testing_guide.md](./references/native_unit_testing_guide.md).
5. Run:

```bash
pio test -d firmware/central -e native
pio test -d firmware/remote -e native
pio run -d firmware/central -e esp32-s3-devkitc-1         # 24-ch (alpha default)
pio run -d firmware/central -e esp32-s3-profet-11ch       # legacy 11-ch PROFET only
pio run -d firmware/remote -e seeed_xiao_esp32c6          # REMOTE_ID=1 entrance
pio run -d firmware/remote -e cockpit_xiao_esp32c6        # REMOTE_ID=3 cockpit
```

---

## Gesture Contract

| Input | Remote emits | Central behavior |
| :--- | :--- | :--- |
| Short press (< 400 ms), no second tap | `Click` after 320 ms window | Toggle mapped channel |
| Second press within 320 ms | `DoubleClick` | Pump: start shower. Pulse: one pulse. Zones: toggle only |
| Hold > 400 ms | `StartHold`, then every 150 ms, `Release` on up | Dimmer: ramp (flip once). Pump: start shower **once**. Other digital: ignore |
| Encoder turn | `EncoderTurn`, `button_index = 7` | Dim last focused zone for that `remote_id` |
| Encoder click | `Click`, `button_index = 7` | Set focused zone to 100% |
| Entrance button 5 (D8) | `Click` | `TurnOffAllLights` |
| Cockpit rocker | `Click` on **each edge** (`InputMode::Latching`) | Toggle. Never emit `StartHold` |

Focus: last dimmable channel clicked on that remote (default Ch 0). Encoder SW is **exclusive** on GPIO 20 — do not also attach a `ButtonHandler` there.

Entrance buttons (faceplate order): 0–3 zones, 4 pump, 5 night. Build flag `CONFIG_PANEL_TYPE=4`. Cockpit: `CONFIG_PANEL_TYPE=5`, latching, wake on GPIO 0 only.

---

## Pre-Commit Checklist

- [ ] LEDC frequency 100–400 Hz (nominal 200)
- [ ] Wake pins are LP-GPIO only; status LED is GPIO 18
- [ ] No channel/NVS writes inside `OnDataRecv`
- [ ] Keep-alives do not retrigger shower, pulses, or dimmer direction
- [ ] Anti-replay persisted; `seq == 1` is not a backdoor
- [ ] Encoder events use button 7; GPIO 20 has one owner
- [ ] Cockpit changes use latching mode and `REMOTE_ID 3`
- [ ] `pio test -e native` passes on both projects
- [ ] Target env matches the board (`esp32-s3-devkitc-1` / `esp32-s3-profet-11ch` / `seeed_xiao_esp32c6` / `cockpit_xiao_esp32c6`)
