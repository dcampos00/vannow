# VanNOW — Comprehensive Logic, Firmware & Cross-Domain Audit Report

**Date of Audit:** 2026-09-11  
**Auditor:** Cursor Grok 4.6 (static electromechanical and firmware review)  
**Scope:** `firmware/central/`, `firmware/remote/`, `firmware/lib/`, `hardware/central-pcb/`, `hardware/entrance-remote-pcb/`, `hardware/cockpit-pcb/`, `hardware/enclosures/`, `docs/`, `.agents/skills/cross-domain-sync/`  
**Methodology:** Cross-domain netlist/GPIO/channel-ID tracing (Atopile ↔ firmware ↔ KiCad scripts ↔ CAD), state-machine walkthroughs, comparison against the 2026-09-03 audit remediations, official Espressif / Seeed / Infineon pin and timing references, and review of native unit-test coverage gaps.

**Predecessor:** [`logic_audit_2026-09-03.md`](logic_audit_2026-09-03.md) (historical). This document supersedes it as the active audit.

**Remediation (same-day):** Firmware FSM defects (FW-09…FW-13, FW-15), encoder dual-dispatch (FW-11), DoubleClick (FW-12), cockpit image (X-03), anti-replay/seq NVS (SEC-01 / FW-14), status LED GPIO 18 (H-08), 11-channel flyback (H-07), 24-channel load terminals (X-02), BAT54S ADC clamp (H-10), and the co-design matrix (DOC-01 / X-01 identity) were corrected on branch `fix/logic-audit-2026-09-11`. Residual: DevKit RGB sharing GPIO 38/48 (H-09), Phoenix 6 A terminals until the next layout regen (H-11), 2×AA into `3V3_OUT` (H-12), hardcoded ESP-NOW keys (FW-16).

---

## 1. Executive Summary

The September 2026 critical blockers that would have **smoked the 5 V rail**, **oscillated dimmers**, and **bypassed replay protection** are **fixed in the current firmware and in the ESP32-S3 socket definition**. The project is no longer electrically dead on paper.

It is **not production-ready**. The repository currently describes **three incompatible machines** (11-channel PROFET carrier, 24-channel carrier, and the documented UX) while the default firmware implements only the 24-channel map. Several user-facing state machines are still logically wrong, and the tracking dashboard’s “100% Complete / Ready” claim is not supported by the sources of truth.

### The most important findings

| ID | Severity | One-line summary |
| :--- | :---: | :--- |
| **X-01** | 🔴 Critical | Firmware is 24-channel; the laid-out 11-channel PROFET board and several docs still describe a different pin/function map. Flashing default firmware onto the 11-channel PCB drives the wrong loads. |
| **FW-09** | 🔴 Critical | Holding the water-pump button **starts then cancels Shower Mode every 150 ms** because keep-alive `StartHold` packets are treated as cancel. |
| **FW-10** | 🔴 Critical | `DigitalChannel::handleAction` applies Shower Mode (5 min timer + load chirp) to **every** digital channel. Holding inverter / fan / heater / LPG / Orion will pulse those outputs. |
| **FW-11** | 🔴 Critical | Entrance encoder push is wired to **GPIO 20 twice** (`EncoderHandler` + `btnCenter`). One press toggles **Zone 1 and Maxxair power**. Encoder turns always dim Zone 1; there is no “active focus zone.” |
| **FW-12** | 🔴 Critical | `ButtonHandler` **never emits `DoubleClick`**. Documented shower / preset double-tap paths are dead code. |
| **H-07** | 🔴 Critical | 11-channel `VanCentralControllerPROFET` flyback diode is on **floating terminal pin 3**, not on the PROFET output. Inductive pump clamp is missing on that design. |
| **H-08** | 🟠 High | Status LED is labeled/driven as **GPIO 9**; XIAO ESP32-C6 **D10 is GPIO 18**. GPIO 9 is the **BOOT** strap. LED will not light; boot risk if GPIO 9 is driven at reset. |
| **H-09** | 🟠 High | GPIO **38** (Ch 15, Orion-XS #1) is the **onboard RGB LED** on ESP32-S3-DevKitC-1 **v1.1**. GPIO **48** (Ch 21) is the RGB LED on **v1.0**. |
| **DOC-01** | 🟠 High | Channel IDs, button legends, and “production ready” status disagree across `central_controller_24ch_architecture.md`, `remote_pcbs_architecture.md`, `project_status_and_tracking.md`, and the co-design matrix. |
| **SEC-01** | 🟠 High | Anti-replay window is RAM-only. Remote battery swap (seq reset) **permanently lockouts** that remote until the central is power-cycled; after a central reboot the first captured packet is accepted. |

**Verdict:** Do **not** order the 11-channel PROFET board as a 24-channel controller. Do **not** treat Shower Mode, Night Shutdown, or rotary dimming as verified. Close X-01 / FW-09 / FW-10 / FW-11 / H-07 before any van install.

---

## 2. Disposition of the 2026-09-03 Findings

| Old ID | 2026-09-03 issue | Status on 2026-09-11 | Evidence |
| :--- | :--- | :---: | :--- |
| **H-01** | J3 pin 1 = 5 V short to GND; off-by-one J3 map | ✅ Fixed | `LonelyBinary_ESP32S3_N16R8` now maps VIN to `left_header.pin_21` (5 V) and J3 pin 1/21/22 to GND. GPIO1 = J3 pin 4. |
| **H-02** | Remote D3 = GPIO 3 / WIFI_ENABLE; GPIO 6 ADC pad | ✅ Fixed (architecture) | Entrance PCB uses Diode-OR to LP-GPIO 0; buttons on GPIO 21/22/23/16/17/19. `BATTERY_ADC_PIN = 255`. Residual: D10/GPIO 9 error (H-08). |
| **H-03** | 5 kHz PWM vs BTS5008 250 µs edges | ✅ Fixed | `DimmableChannel` default frequency is **200 Hz**. |
| **H-04** | 3.3 V Zener leakage on ADC divider | ✅ Fixed (partial) | Zener removed; 100k/18k + 1 kΩ series + 100 nF remain. No BAT54S rail clamp (residual H-10). |
| **H-05** | SPDT pin 3 floating | ⚠️ Named only | `led_zoneX_sw_return` is a named net with **no load feedthrough**. Installer still cannot pass load+ through the PCB. |
| **H-06** | No TVS / reverse polarity; fake LCSC SKUs; 6 A terminals | ✅ / ⚠️ Partial | 24-channel and PROFET modules have SM8S24A + AO4407A P-FET RPP. Phoenix MPT 0,5 (**6 A**) is still used for the Seaflo pump (**7.5 A inrush**). `C20683` remains on the N-FET placeholder. |
| **FW-01** | `seq == 1` replay bypass | ✅ Fixed (partial) | RFC-6479 sliding window in `AntiReplayFilter`. Residual: no NVS checkpoint (SEC-01). |
| **FW-02** | Hold keep-alives invert dimmer direction | ✅ Fixed | `StartHold` flips `_rampDirection` only when `!_isRamping`. **Same keep-alive class of bug now lives on the pump** (FW-09). |
| **FW-03** | `OnDataRecv` mutates channels on `wifi_task` | ✅ Fixed | FreeRTOS queue, dispatch in `loop()`. |
| **FW-04** | Cold boot sleep before pull-ups | ✅ Fixed | `begin()` / `pinMode(INPUT_PULLUP)` run before `goToSleep`. |
| **FW-05** | `delay(5)` vs encoder Nyquist | ✅ Fixed | `delayMicroseconds(250)` (~4 kHz). Residual: blocking `delay()` in LED feedback and 200 ms ESP-NOW wait. |
| **FW-06** | Central default env = XIAO C6 | ✅ Fixed | `default_envs = esp32-s3-devkitc-1`. |
| **FW-07** | Placeholder MACs + encryption | ⚠️ Unchanged | Provisioned MACs `AA:BB:…` and LMK `01..10` still hardcoded. System will not link until both ends are programmed with matching keys. |
| **FW-08** | Unmapped aux channels | ⚠️ Transformed | 24-channel table exists; many channels still have **no remote button**. Cockpit / bed remotes are not buildable from default firmware. |

---

## 3. Detailed Findings — Cross-Domain Identity (What System Is This?)

### [X-01] Three Incompatible Channel Machines

The repository simultaneously maintains:

1. **Firmware default** (`SystemController::DEFAULT_24CH_CONFIG`): 24 channels, pins 12/13/14/15/4/5/6/7/17/8/9/10/11/18/21/38/39/40/41/42/47/48/2/16.
2. **11-channel PROFET schematic** (`VanCentralControllerPROFET`): 4 LED PROFETs, pump, one aux, discrete fan MOSFET on GPIO 17, inverter opto on GPIO 21, DC-DC opto on **GPIO 18**, status LEDs on **GPIO 2 and GPIO 16**.
3. **Documentation / co-design matrix**: an older 6–7 channel map (`PIN_CH0 = 4`, pump on Ch 3, inverter on GPIO 15) in [`.agents/skills/cross-domain-sync/references/co_design_verification_matrix.md`](../.agents/skills/cross-domain-sync/references/co_design_verification_matrix.md).

| Function | 24-ch firmware / `central.ato` 24-ch module | 11-ch PROFET module | Co-design matrix |
| :--- | :--- | :--- | :--- |
| Zone 1 dimmer | GPIO 12, Ch 0 | GPIO 12 | GPIO 4, Ch 0 |
| Water pump | GPIO 4, Ch 4 | GPIO 4 | GPIO 7, Ch 3 |
| Maxxair / fan | GPIO 17 PROFET, Ch 8 | GPIO 17 discrete P-FET | *(not listed)* |
| Inverter | GPIO 21, Ch 14 | GPIO 21 | GPIO 15, Ch 4 |
| GPIO 18 | Aux Power 5 / awning PROFET (Ch 13) | DC-DC **optocoupler** | — |
| GPIO 2 | LPG solenoid opto (Ch 22) | **Cabin status LED** | — |
| GPIO 16 | Generator start pulse (Ch 23) | **Remote status LED** | — |
| Diesel heater | GPIO 41, Ch 18 | **Does not exist** | GPIO 16, Ch 5 |

**Impact:** 🔴 **CRITICAL.** Default `pio run` firmware on the 11-channel board will:

- Drive the cabin and remote LEDs as if they were LPG valve and generator-start outputs.
- Drive the DC-DC optocoupler as a high-side awning PROFET command (or vice versa).
- Leave 14 of 24 software channels unconnected while still restoring some of them from NVS.

[`project_status_and_tracking.md`](project_status_and_tracking.md) marks **both** boards “100% Complete / Ready.” That is a process failure, not just a comment.

**Remediation:** Pick one production identity (recommend: 24-channel). Delete or clearly quarantine the 11-channel PROFET/modular modules from “ready” status. Rewrite the co-design matrix to the 24-channel table. Add a compile-time `BOARD_REVISION` so 11-channel hardware cannot boot the 24-channel pin table.

---

### [X-02] 24-Channel Schematic Has No Load Connectors

`VanCentralController24Ch` in `central.ato` instantiates 14 PROFETs and 10 optocouplers and **stops**. There are no screw terminals, no SPDT override blocks, and no heater/fridge/inverter connector nets on the outputs.

The KiCad script `layout_and_route_24ch.py` **invents** Phoenix MPT 0,5 2-pin terminals and flyback footprints that do not exist in the Atopile module. The Hardware-as-Code netlist and the procedural PCB are not the same design.

**Impact:** 🟠 **HIGH.** `ato build` cannot validate the connectors that will actually be fabricated. Cross-domain sync skill Phase 1 is broken for the 24-channel board.

---

### [X-03] Remote Product Set vs Firmware Builds

| Product | Hardware exists | Firmware image | `remote_id` in default map |
| :--- | :---: | :---: | :---: |
| Entrance remote | Yes (`entrance_remote.ato`) | Yes (`CONFIG_PANEL_TYPE = PANEL_TYPE_ENTRANCE`, `REMOTE_ID 1`) | 1 |
| Cockpit hub | Yes (`cockpit.ato`) | **No** (`PANEL_TYPE_COCKPIT` does not exist) | 3 (table only) |
| Bed panel | Docs only | **No** | 2 (table only) |

Cockpit sense pins (D1/D2/D3/D4/D5/D6) do not match the entrance pin table. Flashing the entrance firmware onto the cockpit PCB will read the wrong switches and advertise `remote_id = 1`, colliding with the door panel.

**Impact:** 🟠 **HIGH.** Dashboard claim “Cockpit Hub … 100% (Protocol mapped)” is mapping-table-only.

---

## 4. Detailed Findings — Firmware Logic

### [FW-09] Shower Mode Oscillates On Keep-Alive `StartHold`

**Files:** `firmware/central/src/SystemController.cpp` (dispatch), `firmware/remote/src/ButtonHandler.cpp`

`ButtonHandler` emits `StartHold` at 400 ms, then **again every 150 ms** while the pin is held.

`SystemController::dispatchMessage` treats **every** `StartHold` on the pump as a toggle:

```text
if (target == pump && (DoubleClick || StartHold)):
    if pump.isTimedActive():  cancel
    else:                     activateTimer(shower)
```

**Walkthrough:**

1. User holds Water Pump (> 400 ms) intending Shower Mode.
2. First `StartHold` → 5-minute timer starts, chirp begins (pump 12 V pulses).
3. 150 ms later → second `StartHold` → `isTimedActive() == true` → **pump forced OFF**.
4. 150 ms later → third `StartHold` → timer starts again.
5. Repeat for the entire hold.

Documented cancel path (“press again to stop”) is a **Click**, not a keep-alive. Tests only send a single `DoubleClick` (`test_water_pump_shower_mode_dispatch_and_cancel`) and never a hold stream, so CI stays green.

**Impact:** 🔴 **CRITICAL.** Shower Mode is unusable from a physical hold. The pump motor sees a ~3 Hz on/off cycle (inrush + flyback stress).

**Remediation:** Activate shower only on `DoubleClick`, or on the **first** `StartHold` while `!_isRamping`/`!isTimedActive()`, and ignore subsequent `StartHold` keep-alives. Cancel only on `Click` or `Release` after an explicit second gesture.

---

### [FW-10] Shower / Chirp Logic Is Generic to All Digital Channels

**File:** `firmware/central/src/DigitalChannel.cpp`

```text
Click        -> toggle
DoubleClick  -> 5 min timer + chirp (hardcoded 300000 ms)
StartHold    -> same 5 min timer + chirp
```

`SystemController` intercepts pump `StartHold`/`DoubleClick` first. **Every other digital channel** still receives `handleAction`:

| Physical hold (if a packet arrives) | Hardware effect |
| :--- | :--- |
| Entrance button 3 (Inverter, Ch 14) | Chirp the MultiPlus remote contact, then leave inverter ON for 5 min |
| Entrance button 6 (Maxxair power, Ch 8) | Pulse 12 V fan power as “chirp,” then 5 min ON |
| Cockpit SW2 (Orion, Ch 15) | Same on DC-DC remote enable |
| Diesel heater (Ch 18) if ever mapped | Thermostat contact chirped — **unsafe** vs diesel cooldown policy |
| LPG valve (Ch 22) | Solenoid chirped |

`PulseChannel` inherits `DigitalChannel` and **overrides** `handleAction` to `trigger()` on Click / DoubleClick / **StartHold**. Keep-alives would retrigger Maxxair keypad or generator crank every 150 ms if those channels are mapped to a held button.

**Impact:** 🔴 **CRITICAL.** Shower Mode must be pump-specific. Chirping a diesel heater thermostat or generator start line is a safety defect.

**Remediation:** Remove timer/chirp from `DigitalChannel::handleAction`. Keep it on an explicit `DigitalChannel::activateTimer` API used only by `SystemController` for the pump index. `PulseChannel` must ignore keep-alive `StartHold`.

---

### [FW-11] Encoder Dual-Dispatch and Missing Focus Zone

**Files:** `firmware/remote/src/main.cpp`, `EncoderHandler.cpp`, `SystemController.cpp`  
**Hardware:** `entrance_remote.ato` encoder SW → `mcu.GPIO20`

```text
ButtonHandler btnCenter(20, 6);          // D9, button index 6 → Ch 8 Maxxair
EncoderHandler encoder(1, 2, 20);        // same GPIO 20 as SW
remoteSender.send(encAction, 0, steps);  // ALL encoder events as button 0
```

**One encoder click produces two packets:**

1. `EncoderHandler` → `ActionType::Click`, `button_index = 0` → **Lights Zone 1 toggle**
2. `ButtonHandler` `btnCenter` → `Click`, `button_index = 6` → **Maxxair Fan Power toggle**

Encoder rotation is also sent as `button_index = 0` → always `DimmableChannel` Zone 1. Documented UX (“select zone, then turn knob”; encoder click = 100% boost / preset) is not implemented. There is no focus-zone state machine.

Hardware silkscreen/docs say D8 is **Master Night Shutdown**. Firmware maps that button (index 3) to **Inverter Ch 14**. Night-off exists only on **Remote 2 button 3**, a panel that has no firmware.

**Impact:** 🔴 **CRITICAL.** The physical “master dimmer” cannot dim zones 2–4. The encoder knob click turns lights **and** the roof fan.

**Remediation:** Give the encoder SW a single owner. Send encoder turns with a dedicated button index or a `rotation_steps` path that uses a software focus channel. Implement Night Shutdown on entrance button 3 **or** change the faceplate legend. Do not compile `btnCenter` on GPIO 20 if `EncoderHandler` already owns it.

---

### [FW-12] `DoubleClick` Is Protocol Fiction

**Files:** `firmware/lib/protocol/protocol.h`, `ButtonHandler.cpp` / `.h`, `docs/remote_pcbs_architecture.md`, `docs/atenuacion_remota.md`

`ActionType::DoubleClick` exists. Central dispatch and tests use it. `ButtonHandler` states are only `Idle → Debounce → Pressed → Holding`. It can emit **Click, StartHold, Release** only.

Documented behaviors that require double-tap:

- Shower Mode via double-click (docs §4.3)
- Zone favorite preset on double-tap (remote architecture §1.3)

are unreachable from hardware.

**Impact:** 🔴 **CRITICAL** for the documented UX contract; 🟠 **HIGH** if hold is accepted as the only shower gesture (but hold is broken by FW-09).

---

### [FW-13] Cockpit Latching Rockers vs Momentary FSM

**Files:** `ButtonHandler.cpp`, `docs/remote_pcbs_architecture.md` §2.2–2.3

The cockpit spec allows **Carling V-Series latching** rockers and also “momentary.” Firmware assumes a momentary active-low pulse:

- Pin stays LOW → after 400 ms, endless `StartHold` keep-alives
- `PowerManager.feed()` on every keep-alive → **never returns to deep sleep**
- If SW4 (pump) is latched ON → FW-09 oscillation forever

**Impact:** 🟠 **HIGH.** A latched “Water Pump” dash switch becomes a battery-draining RF jammer and a pump strobe.

**Remediation:** Cockpit firmware must treat inputs as **level** (send Click on rising/falling edge only, no hold). Specify momentary Carling blanks only, or implement edge detection with no `StartHold`.

---

### [FW-14] Anti-Replay Lockout After Remote Power Loss

**Files:** `AntiReplayFilter.cpp`, `RemoteSender.cpp` (`RTC_DATA_ATTR msgSequenceNumber`)

- Sequence lives in RTC memory on the remote (survives deep sleep, **dies on battery removal**).
- Window lives in RAM on the central (**dies on cabinet power cycle**, not written to NVS).

**Cases:**

1. **Remote batteries replaced** while central has `maxSeq = 5000`: remote restarts at seq 1. `1` is outside the 64-packet window → **all further packets rejected** until central reboot.
2. **Central brownout / firmware reset:** `initialized = false` → **first packet accepted unconditionally**, including a captured replay. This re-opens a weaker form of old FW-01.

**Impact:** 🟠 **HIGH.** Field lockout is certain after a battery change. Security guarantee does not survive central reboot.

**Remediation:** Persist `{maxSeq, window}` per remote in NVS. Require an authenticated re-pair (or a signed seq reset) when the remote’s seq is far behind. Do not accept an uninitialized window from the air without a pairing token.

---

### [FW-15] `PulseChannel` Retrigger on Hold

If button 6 is remapped to Ch 20 (as `test_remote_mapping_customization` demonstrates is intended), holding the key calls `trigger()` every 150 ms. Maxxair membrane “power” would be mashed; generator start (Ch 23, 500 ms) would crank repeatedly.

**Impact:** 🟠 **HIGH** when those mappings are used.

---

### [FW-16] Hardcoded Secrets and Unprovisioned Radios

- Central / remote MACs: `AA:BB:CC:DD:EE:FF` / `…11/22/33`
- PMK: `"PMK_KEY_VANNOW12"`
- LMK: `0x01 … 0x10`

ESP-NOW encryption will drop frames that do not match the peer table. The binary will not talk to factory eFuse MACs. The keys are trivial if a firmware image is extracted.

`esp_wifi_set_mac` return value is ignored. If the chip rejects a locally administered MAC, peers will not match and packets are silently dropped.

**Impact:** 🟡 **MEDIUM** (function) / 🟠 **HIGH** (if this image is installed in a van without a provisioning step).

---

### [FW-17] Battery Alert Logic and Dummy Voltage

```text
if (voltage < 2.2 && (voltage > 0.5 || voltage == 0.0))
```

`BATTERY_ADC_PIN = 255` makes `readBatteryVoltage()` return a constant **3.0 V**, so the alert never fires on the entrance remote. The `== 0.0` clause would false-alarm if any other panel reports “unmeasured.”

**Impact:** 🟡 **MEDIUM.** Low-battery warning is inert on the shipping remote firmware.

---

### [FW-18] Blocking Delays vs Encoder / Hold Timing

`RemoteSender::send` busy-waits up to **200 ms** for TX status. `indicateAction` adds 50–480 ms of `delay()`. Encoder sampling and button FSMs are frozen during that window. Hold keep-alives are scheduled at 150 ms, so they bunch or skip.

**Impact:** 🟡 **MEDIUM.** Missed encoder detents and bursty keep-alives (worsens FW-09).

---

### [FW-19] NVS Writes During Shower Chirp / Rapid Toggles

`activateTimer` and `setState` call `notifyStateChanged` → `Preferences` write. Pump chirp and FW-09 oscillation can wear the NVS partition. The 3 s settle rule exists only for encoder brightness.

**Impact:** 🟡 **MEDIUM** (flash wear); 🟠 if FW-09 ships.

---

## 5. Detailed Findings — Hardware & Electronics

### [H-07] 11-Channel Pump Flyback Attached to the Wrong Pin

**File:** `hardware/central-pcb/central.ato` (`VanCentralControllerPROFET`)

```text
pump_diode.cathode ~ pump_terminal.pin_3   # named return, no PROFET connection
pump_stage.power_out ~ pump_terminal.pin_1 # AUTO
# pin 3 is otherwise unconnected
```

The 1N5408 is across a floating SPDT return, **not** across the motor. The 24-channel Atopile module correctly ties the diode to `pump.power_out`.

**Impact:** 🔴 **CRITICAL** for any 11-channel PROFET fabrication. Inductive kick can destroy the BTS5008.

---

### [H-08] XIAO Status LED: GPIO 9 vs D10 = GPIO 18

Authoritative Seeed / Arduino map (`variants/XIAO_ESP32C6/pins_arduino.h`):

| Silk | GPIO | Notes |
| :--- | :---: | :--- |
| D10 | **18** | MOSI, header pin |
| — | **9** | **BOOT** strap, not D10 |

`entrance_remote.ato` and `cockpit.ato` declare `GPIO9 ~ header_right.pin_4 # D10`. Firmware `#define STATUS_LED_PIN 9`.

If the carrier socket pin 4 is physically D10, the LED is on **GPIO 18** and firmware toggles **GPIO 9** (BOOT). The LED stays dark. Driving BOOT as a totem-pole output is a strap hazard.

Docs (`remote_pcbs_architecture.md` §4.2, breadboard guide) repeat “D10 (GPIO 9).”

**Impact:** 🟠 **HIGH.** Visual shower / ACK feedback does not work. Boot reliability risk.

**Remediation:** Net D10 to GPIO 18 in Atopile; `#define STATUS_LED_PIN 18` (or `D10`). Never drive GPIO 9.

---

### [H-09] DevKit RGB LED vs Optocoupler Channels

Espressif ESP32-S3-DevKitC-1:

- **v1.1 (current official):** RGB LED on **GPIO 38** → firmware **Ch 15 Orion-XS #1**
- **v1.0:** RGB LED on **GPIO 48** → firmware **Ch 21 Alarm**

Arduino cores often attach RMT/WS2812 to the RGB pin at boot, fighting `digitalWrite`.

**Impact:** 🟠 **HIGH** on a stock DevKitC-1 carrier (the stated Lonely Binary N16R8 form factor). Orion remote enable or alarm output shares a LED load and may glitch at startup.

**Remediation:** Move Ch 15/21 off GPIO 38/48, or remove/isolate the DevKit RGB LED, or switch to a bare module without the LED.

---

### [H-10] ADC Clamp Still Missing for Load-Dump

After SM8S24A clamp (~26–30 V), the 100k/18k divider presents ~4.0–4.6 V at the tap. ESP32 ADC absolute maximum is 3.6 V. The 1 kΩ series resistor limits clamp current into the internal diodes (~1 mA) — usually survivable, not a precision measurement path, and not the BAT54S rail clamp required by the schematic-design skill.

**Impact:** 🟡 **MEDIUM.** Telemetry error and residual ADC abuse during ISO 7637-2 pulse 5a.

---

### [H-11] Phoenix MPT 0,5 Current Rating vs Pump

24-channel layout still uses `TerminalBlock_Phoenix_MPT-0,5-2-2.54` (6 A) for all 14 power outputs, including the Seaflo pump (7.5 A inrush, documented in `remote_pcbs_architecture.md`). Same finding as H-06, not closed.

**Impact:** 🟠 **HIGH** for the pump and any aux > 6 A (lightbar, sockets, boiler). Terminal heating / melting risk.

---

### [H-12] XIAO Powered From 2× AA Into `3V3_OUT`

Seeed documents `3V3` as **regulator output**. The design (and breadboard guide) feed 2× AA (~3.0 V alkaline, ~2.4 V NiMH) into that pin.

ESP32-C6 Wi-Fi TX is marginal at 3.0 V and illegal at 2.4 V. USB plugged in while batteries are connected can back-feed the cells. Deep-sleep current claims (~15 µA, “19 year” life) assume the regulator is not fighting an external source and ignore Wi-Fi TX bursts on every press.

**Impact:** 🟠 **HIGH** for reliability; 🟡 for the energy-budget documents that assume 0.15 W system / multi-year remotes.

---

### [H-13] SPDT ON-OFF-AUTO Still Not a Pass-Through

Pin 3 on LED zone terminals is a dangling named net. The load positive is not brought back onto the PCB. The mechanical lid still has six SPDT keyways (`generate_enclosures.py`). Electrical and mechanical designs disagree.

**Impact:** 🟡 **MEDIUM.** Cabinet bypass switches cannot be wired as drawn in `analisis_critico_riesgos.md` without extra splices.

---

### [H-14] Diode Symbol vs Footprint Pin-1 Convention

`MyDiode` binds **cathode to pin 1**, anode to pin 2. KiCad `Device:D` is **anode = pin 1**, cathode = pin 2. Many `D_DO-201AD` footprints use pin 1 as the banded cathode.

24-channel layout sets pad 1 = `CH4_OUT`, pad 2 = GND, which is correct **if and only if** pad 1 is cathode.

**Impact:** 🟠 **HIGH** if the footprint’s pad 1 is anode (dead short of PROFET output through the 1N5408). Must be verified on the exact KiCad library part before fab.

---

### [H-15] BTS5008 `DEN` Tied to GND, IS Dumped

Diagnostics disabled; current-sense resistor to GND. No software over-current / open-load detection. Acceptable for a first bring-up, not for an unattended pump in a van.

**Impact:** 🟢 **LOW** (feature gap) relative to the logic bugs above.

---

## 6. Detailed Findings — Documentation & Process

### [DOC-01] Authoritative Documents Contradict Each Other

| Claim | Source | Reality in code |
| :--- | :--- | :--- |
| Production 100% ready, 6× BTS5008 on 150×95 mm | `project_status_and_tracking.md` | Firmware is 24-ch; 24-ch PCB is 180×110 mm |
| Co-design Ch 0 = GPIO 4 | `co_design_verification_matrix.md` | Ch 0 = GPIO 12 |
| Entrance BTN 6 = Master OFF; encoder = focus dimmer | `remote_pcbs_architecture.md` | BTN 3 = inverter; encoder = Zone 1 only |
| Cockpit SW2 = Channel 10; SW5 = Channel 9; SW6 = driving mode | `remote_pcbs_architecture.md` §2.3 | SW2 = Ch 15; SW5 = Ch 14; SW6 = Ch 8 Maxxair |
| Zone 4 = Porch / Aux PROFET | Remote architecture §1.1 | Zone 4 = dimmable Ch 3 (GPIO 15) |
| Shower chirp on GPIO 48 buzzer | `breadboard_prototyping_and_validation_guide.md` | Chirp **is the pump output** (GPIO 4) |
| Brightness 0–255 | Breadboard expected serial | Firmware brightness 0–100 |
| 19/19 and 9/9 tests = logically correct | Status dashboard | Tests never emit keep-alive `StartHold` to the pump, never press GPIO 20 twice, never generate `DoubleClick` from `ButtonHandler` |
| Cockpit enclosure 75×52×24 mm | Remote architecture §2.2 | CAD generator: 108×58×17.8 mm |

**Impact:** 🟠 **HIGH.** Agents following AGENTS.md “mandatory reading” will “fix” the wrong pin table.

---

### [DOC-02] Status Dashboard Over-Claims Readiness

Manufacturing checklist items (Gerbers, BOM, CPL) are still unchecked, which is honest. The domain matrix scoring every board **100%** is not. DRC-clean copper is not system-logical correctness.

**Impact:** 🟡 **MEDIUM** (process). Creates false confidence to order PCBs.

---

## 7. What Is Actually Sound

These items were reviewed and are **logically consistent** with their own specs:

- ESP32-S3 DevKitC-1 v1.1 **header power/GND map** in `LonelyBinary_ESP32S3_N16R8`.
- Avoidance of USB-JTAG (GPIO 19/20), strapping (0/3/45/46), and octal SPI (33–37) in the 24-channel pin table.
- 200 Hz LEDC default; hold-ramp no longer reverses on keep-alives **for dimmers**.
- Pump `restoreOnBoot = false`; NVS restore test covers flood-prevention.
- FreeRTOS receive queue + TWDT on central.
- Entrance Diode-OR to LP-GPIO 0; encoder A/B on LP-GPIO 1/2 (valid EXT1 wake set).
- Packed `SwitchMessage` with 32-bit `seq`.
- 24-channel KiCad DRC report: 0 violations / 0 unconnected (copper-only; does not prove schematic identity).
- 24-channel enclosure standoff pitch 170×100 mm matches 180×110 mm PCB.

---

## 8. Verification Matrix (This Audit)

| ID | Issue | Authoritative reference |
| :--- | :--- | :--- |
| **H-01 (closed)** | DevKitC-1 J1 pin 21 = 5 V; J3 pin 1 = GND | [ESP32-S3-DevKitC-1 v1.1 User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html) |
| **H-08** | XIAO D10 = GPIO 18; GPIO 9 = BOOT | [Seeed XIAO ESP32-C6 pin map](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/); `pins_arduino.h` `D10 = 18` |
| **H-09** | RGB LED GPIO 38 (v1.1) / GPIO 48 (v1.0) | Same Espressif user guide, RGB LED note |
| **H-03 (closed)** | BTS5008 t_ON/t_OFF ≈ 250 µs → ≤ 400 Hz PWM | Infineon BTS5008-1EKB datasheet §5.3 |
| **H-11** | MPT 0,5 = 6 A | Phoenix Contact MPT 0,5 datasheet |
| **FW-01 (closed) / SEC-01** | Sliding window; persist counters | [RFC 6479](https://datatracker.ietf.org/doc/html/rfc6479) |
| **FW-09 / FW-10** | Keep-alive vs one-shot gestures | `ButtonHandler` `HOLD_PERIOD_MS = 150`; `DigitalChannel::handleAction` |
| **X-01** | Channel identity | `SystemController.cpp` vs `VanCentralControllerPROFET` vs verification matrix |

---

## 9. Test Coverage Gaps (Why CI Does Not Catch This)

Native suites (`19` central + `9` remote) pass **and still miss** the defects above:

| Missing test | Would catch |
| :--- | :--- |
| Pump `StartHold` + 150 ms + `StartHold` | FW-09 oscillation |
| `DigitalChannel` `StartHold` on a non-pump name | FW-10 |
| Two handlers on GPIO 20 both `checkEvent` | FW-11 |
| `ButtonHandler` double-tap sequence | FW-12 (would fail today) |
| Anti-replay after `resetAll` then seq=1 vs captured seq | SEC-01 |
| Firmware pin table vs a machine-readable 24-ch map | X-01 |
| Cockpit edge-only events / no keep-alive | FW-13 |

Until those exist, “28/28 tests pass” is not a logic-safety gate.

---

## 10. Prioritized Remediation Roadmap

### Phase A — Stop shipping a contradictory machine (before any PCB order)

1. Declare **one** production central: 24-channel **or** 11-channel. Mark the other `deprecated/` in schematic, layout, enclosure, and status dashboard.
2. Finish `VanCentralController24Ch` with real load terminals, flyback nets, and SPDT pass-through (or delete SPDT from the lid).
3. Replace Phoenix MPT 0,5 on pump / high-current outs with ≥ 15 A terminals.
4. Fix XIAO D10 → GPIO 18; verify 1N5408 pad 1 polarity against the KiCad footprint.
5. Move Orion/alarm off GPIO 38/48 or isolate the DevKit RGB LED.
6. Rewrite `co_design_verification_matrix.md` and demote `project_status_and_tracking.md` from “100% ready.”

### Phase B — Make the user-visible FSMs true

7. Pump shower: first `StartHold` or a real `DoubleClick` only; ignore keep-alives; cancel on `Click`.
8. Strip shower/chirp out of generic `DigitalChannel` / `PulseChannel`.
9. Implement `DoubleClick` in `ButtonHandler` **or** delete it from protocol and docs.
10. Single-owner encoder SW; implement focus-zone dimming **or** document “knob = Zone 1 only.”
11. Map entrance D8 to Night Shutdown **or** change the faceplate.
12. Add `PANEL_TYPE_COCKPIT` + `REMOTE_ID 3` with level/edge semantics. Do not reuse entrance firmware.

### Phase C — Security & field reliability

13. NVS-backed anti-replay + authenticated seq reset after remote battery change.
14. Provisioning story for MAC/PMK/LMK (not `01..10` in git).
15. BAT54S (or equivalent) ADC rail clamp; measure telemetry under 16 V and clamped load-dump.
16. Add the missing native tests listed in §9; then `pio test` on both projects.

---

## 11. Residual Risk After Phase A–C

Even after the logic defects close, the following remain engineering work (not in this audit’s “must-fix before breadboard” set):

- No software PROFET IS / DEN diagnostics.
- No bed-panel hardware.
- Encrypted ESP-NOW still has no pairing UI.
- 2× AA on `3V3` needs a proper boost or a board that accepts 1.8–3.3 V VDD.
- Mechanical clash numbers in the dashboard were not re-run in this audit.

---

## 12. Conclusion

VanNOW has successfully closed the **catastrophic 2026-09-03 electrical shorts and the dimmer keep-alive inversion**. The remaining failures are **identity and state-machine** failures: the firmware, the 11-channel carrier, the 24-channel carrier, and the UX specifications do not describe the same product, and the pump/encoder/digital-channel FSMs will misbehave on real buttons.

Treat this file as the active logic audit. Do not use [`project_status_and_tracking.md`](project_status_and_tracking.md) or [`logic_audit_2026-09-03.md`](logic_audit_2026-09-03.md) as evidence that the system is ready to fabricate or install.
