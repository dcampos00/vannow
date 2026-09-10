# VanNOW — Comprehensive Logic & Hardware Audit Report

**Date of Audit:** 2026-09-03  
**Auditor:** Antigravity (Advanced Agentic Architecture & Firmware Review)  
**Scope:** `hardware/central-pcb/` (Atopile schematics & KiCad layouts), `firmware/central/`, `firmware/remote/`, `firmware/lib/`, `docs/`  
**Methodology:** Static code analysis, electrical netlist tracing, circuit boundary condition calculation, FreeRTOS task concurrency inspection, and verification against official datasheets and technical documentation from Espressif Systems, Infineon Technologies, Nexperia, Seeed Studio, and RFC standards.

---

## 1. Executive Summary

This audit examined the current state of the **VanNOW** project following the implementation of several patches from the preliminary July 2026 audit. While progress was made (such as transitioning from GPIO 19/20 on the central MCU and refining the telemetry math), **new and unresolved critical logical and electrical flaws were discovered that completely prevent the system from functioning, pose severe hardware smoke/short-circuit hazards, and bypass security protections.**

### The Most Critical Findings:
1. **Dead Short to Ground on Central PCB Power Input (ESP32-S3 DevKit Socket Inversion):**  
   In `central.ato`, the 5V buck converter output is connected to J3 Pin 1 of the ESP32-S3 DevKit. According to the official Espressif DevKitC-1 v1.1 schematic, **J3 Pin 1 is physical Ground (GND)**, resulting in an immediate dead short of the MP1584EN 5V regulator output to ground. J1 Pin 21 (the actual 5V VIN input) is left floating. Furthermore, all GPIOs on header J3 are shifted off-by-one, grounding GPIO 19, grounding UART0 TX (GPIO 43), and connecting the battery ADC to GPIO 2 instead of GPIO 1.
2. **Replay Protection Bypass in Central Firmware:**  
   In `firmware/central/src/main.cpp`, the replay check contains `if (msg.seq <= lastSeqNumbers[remoteIdx] && msg.seq != 1)`. Replaying a packet with sequence number `1` bypasses the check unconditionally, allowing an attacker to replay captured commands indefinitely and resetting the central's sequence tracker.
3. **5% Jitter Oscillation in Dimming State Machine:**  
   In `ButtonHandler.cpp`, hold events trigger periodic `StartHold` keep-alive packets every 150 ms. In `DimmableChannel.cpp`, every `StartHold` action flips `_rampDirection = -_rampDirection`. Consequently, during a hold press, the light brightness reverses direction every 150 ms, oscillating up and down by 5% endlessly rather than smoothly dimming.
4. **Physical Impossibility of Remote 4-Button Panel on XIAO ESP32-C6:**  
   In `firmware/remote/src/main.cpp`, button 3 is configured on pin `3` (`ButtonHandler btn3(3, 3)`). On the Seeed Studio XIAO ESP32-C6, header pin **D3 is GPIO 21**, whereas GPIO 3 is internally connected to `WIFI_ENABLE` (RF Switch power). Furthermore, only LP-GPIOs (GPIO 0–7) can wake the ESP32-C6 from deep sleep in EXT1 mode; GPIO 21 cannot wake the chip. In addition, `BATTERY_ADC_PIN` is set to GPIO 6, which is an unpopulated test pad on the bottom of the board.
5. **Inoperable 5 kHz PWM Frequency on BTS5008-1EKB PROFETs:**  
   `DimmableChannel.h` configures a 5 kHz PWM frequency (200 µs period). The Infineon BTS5008-1EKB smart switch has turn-on and turn-off times of up to **250 µs** each, making it physically incapable of modulating at 5 kHz and leading to extreme switching heat or complete loss of dimming control. PROFET+ 12V devices are rated for a maximum of 100–400 Hz.
6. **Telemetry Distortion via 3.3V Zener Diode:**  
   In `central.ato`, a `BZX84-C3V3` Zener diode is placed directly across the 16.25 kΩ Thévenin equivalent of the battery voltage divider. Due to the soft reverse-breakdown knee of low-voltage Zeners (5 µA leakage at only 1V, tens of µA at 2.2V), the Zener introduces a severe non-linear voltage drop of 0.5V to 1.0V at the ADC, distorting the 12V battery reading by several volts.
7. **FreeRTOS Concurrency Race Condition:**  
   The ESP-NOW receive callback `OnDataRecv` runs in the high-priority `wifi_task`, mutating `DimmableChannel` state variables concurrently while `loopTask` reads and writes the same variables in `SystemController::update()` without synchronization.

---

## 2. Detailed Findings: Hardware & Electronics

### [H-01] Critical ESP32-S3 DevKit Socket Pinout Inversion & Short Circuit
- **Files Affected:** `hardware/central-pcb/central.ato:151-184`, `hardware/central-pcb/layouts/profet/profet.kicad_pcb`
- **Description:**  
  The sub-module `LonelyBinary_ESP32S3_N16R8` declares the socket headers for the ESP32-S3-DevKitC-1 development board as two 1x22 headers (`left_header` / J1 and `right_header` / J3).  
  In `central.ato`:
  ```ato
  # Power Rails
  signal V3P3 ~ left_header.pin_1
  signal VIN ~ right_header.pin_1
  signal GND ~ right_header.pin_2

  # J3 (Right Row)
  signal GPIO1 ~ right_header.pin_5
  signal GPIO2 ~ right_header.pin_6
  signal GPIO21 ~ right_header.pin_19
  signal GPIO20 ~ right_header.pin_20
  signal GPIO19 ~ right_header.pin_21
  ```
- **Official Specification Comparison (Espressif ESP32-S3-DevKitC-1 v1.1):**
  According to the official Espressif DevKitC-1 v1.1 User Guide and Board Schematic:
  | Header Pin | Official DevKit Pin Name | `central.ato` Declaration | Actual Result on Fabricated Board |
  | :--- | :--- | :--- | :--- |
  | **J1 Pin 21** | **5V (Power Input)** | *Unconnected (NC)* | **ESP32 receives NO 5V power** |
  | **J3 Pin 1** | **GND (Ground)** | `signal VIN` (5V Buck Output) | **DEAD SHORT: 5V buck output tied directly to GND!** |
  | **J3 Pin 2** | **TX (GPIO 43)** | `signal GND` | **UART TX grounded; serial communication disabled** |
  | **J3 Pin 4** | **GPIO 1** (ADC1_CH0) | *Unconnected* | Battery ADC in firmware reads floating pin |
  | **J3 Pin 5** | **GPIO 2** | `signal GPIO1` | Battery voltage divider connected to GPIO 2 |
  | **J3 Pin 6** | **GPIO 42** | `signal GPIO2` | Cabin status LED connected to GPIO 42 |
  | **J3 Pin 18** | **GPIO 21** | *Unconnected* | Inverter optocoupler not triggered by GPIO 21 |
  | **J3 Pin 19** | **GPIO 20** (USB_D+) | `signal GPIO21` | Inverter optocoupler connected to USB D+ |
  | **J3 Pin 20** | **GPIO 19** (USB_D-) | `signal GPIO20` | Optocoupler connected to USB D- |
  | **J3 Pin 21** | **GND (Ground)** | `signal GPIO19` | GPIO 19 tied directly to GND |
  | **J3 Pin 22** | **GND (Ground)** | `signal GND` | Ground |

- **Impact:** 🔴 **CRITICAL.** Plugging the ESP32-S3 board into this socket instantly shorts the 5V buck converter output to Ground through the ESP32 ground plane, risking damage to the MP1584EN module. The ESP32 will never receive power on its VIN pin. Serial logging is disabled, and all signals on header J3 are routed to the wrong physical pins.
- **Authoritative Reference:**
  - Espressif Systems: *ESP32-S3-DevKitC-1 v1.1 User Guide (Header Block & Pin Layout)*.  
    URL: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html
  - Espressif Systems: *SCH_ESP32-S3-DevKitC-1_V1.1 Board Schematic (PDF)*.  
    URL: https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf

---

### [H-02] Physical & Architectural Remote Pinout Trap on XIAO ESP32-C6
- **Files Affected:** `firmware/remote/src/main.cpp:33-45`, `firmware/remote/src/PowerManager.cpp:27-34`
- **Description:**  
  The remote firmware targets the Seeed Studio XIAO ESP32-C6. In `main.cpp`:
  ```cpp
  #define BATTERY_ADC_PIN 6 
  ButtonHandler btn3(3, 3); // D3
  const uint8_t wakeupPins[] = {0, 1, 2, 3};
  ```
  1. **Pin D3 is NOT GPIO 3:** According to the official Seeed Studio pinout and Arduino core `pins_arduino.h`, the pin labeled `D3` on the board header is **GPIO 21**. GPIO 3 is NOT on the header; it is routed internally to the RF switch control (`WIFI_ENABLE = 3`). Initializing `pinMode(3, INPUT_PULLUP)` alters the RF front-end power instead of reading a button.
  2. **EXT1 Deep Sleep Wakeup Incompatibility:** On the ESP32-C6 architecture, deep sleep powers down the High-Power (HP) peripheral domain. External wakeups (`ESP_SLEEP_WAKEUP_EXT1`) are strictly restricted to the Low-Power (LP) domain, which only covers **LP-GPIO 0 through 7**. The XIAO ESP32-C6 brings out only **three** LP-GPIOs to its 14-pin external headers: **D0 (GPIO 0), D1 (GPIO 1), and D2 (GPIO 2)**. Pin D3 (GPIO 21) is an HP-GPIO and cannot wake the chip from deep sleep in EXT1 mode. A 4-button panel with individual EXT1 wakeups cannot be implemented using only the board headers.
  3. **Inaccessible Battery ADC Pin:** GPIO 6 (`BATTERY_ADC_PIN 6`) is an unpopulated SMD test pad on the bottom of the XIAO PCB (reserved for JTAG MTCK). It is impossible to connect via standard breadboards or carrier board female headers.
- **Impact:** 🔴 **CRITICAL.** Button 3 does not function, cannot wake the remote from deep sleep, and reading battery voltage on GPIO 6 fails on breadboard/carrier hardware.
- **Authoritative Reference:**
  - Seeed Studio: *XIAO ESP32C6 Getting Started & Pinout Table*.  
    URL: https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
  - Espressif Systems: *ESP32-C6 Technical Reference Manual (Section: Low-Power Management / Sleep Modes)*.  
    URL: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/sleep_modes.html
  - Arduino ESP32 Board Definitions: `variants/XIAO_ESP32C6/pins_arduino.h` (`D3 = 21`, `WIFI_ENABLE = 3`).

---

### [H-03] BTS5008-1EKB Switching Time Incompatibility with 5 kHz PWM
- **Files Affected:** `firmware/central/src/DimmableChannel.h:8`, `firmware/central/src/SystemController.cpp:6-9`, `hardware/central-pcb/central.ato:355-399`
- **Description:**  
  `DimmableChannel` initializes ESP32 LEDC PWM with `frequency = 5000` (5 kHz).  
  At 5 kHz, the PWM period is:
  $$T_{\text{PWM}} = \frac{1}{5000\text{ Hz}} = 200\text{ µs}$$
  According to the Infineon BTS5008-1EKB datasheet:
  - Maximum turn-on time ($t_{\text{ON}}$): **250 µs**
  - Maximum turn-off time ($t_{\text{OFF}}$): **250 µs**
  
  Because the total switching transition time ($t_{\text{ON}} + t_{\text{OFF}} \le 500\text{ µs}$) exceeds the entire PWM period ($200\text{ µs}$), the internal power MOSFET never reaches steady-state ON or OFF.
- **Impact:** 🔴 **CRITICAL.** The BTS5008-1EKB will either output solid DC, completely fail to turn on at low duty cycles, or operate predominantly in its linear region, dissipating excessive power and triggering thermal shutdown or permanent device failure. Infineon PROFET+ 12V switches are rated for a maximum PWM frequency of **100 Hz to 400 Hz** (nominally 200 Hz for automotive lighting).
- **Authoritative Reference:**
  - Infineon Technologies: *BTS5008-1EKB Smart High-Side Power Switch Datasheet (Section 5.3: Switching Characteristics)*.  
    URL: https://www.infineon.com/cms/en/product/power/smart-power-switches/high-side-switches/12-v-high-side-switches/profet-12v/bts5008-1ekb/
  - Infineon Technologies: *Application Note: Driving Inductive and Resistive Loads with PROFET™+ 12V Devices*.

---

### [H-04] Severe Battery Telemetry Non-Linearity from 3.3V Zener Diode
- **Files Affected:** `hardware/central-pcb/central.ato:319-350`
- **Description:**  
  The module `AnalogVoltageDivider` adds a `BZX84-C3V3` Zener diode across `v_out` to GND for overvoltage clamping.  
  The Thévenin resistance driving the Zener is:
  $$R_{\text{TH}} = (100\text{ k}\Omega \parallel 18\text{ k}\Omega) + 1\text{ k}\Omega \approx 16.25\text{ k}\Omega$$
  Low-voltage Zener diodes (< 5.1V) operate via quantum tunneling rather than avalanche breakdown and have an exceptionally soft "knee." According to the Nexperia BZX84-C3V3 datasheet:
  - Reverse leakage current $I_R$ is already up to **5 µA at only 1.0 V**.
  - At normal operating voltages ($V_{\text{ADC}} \approx 2.1\text{ V} - 2.3\text{ V}$, corresponding to a 13.8V–14.4V battery), the leakage current rises to $30\text{ µA} - 60\text{ µA}$.
  
  Calculating the resulting voltage error across $R_{\text{TH}}$:
  $$\Delta V = 50\text{ µA} \times 16.25\text{ k}\Omega = 0.8125\text{ V}$$
  Scaling this through the divider ratio ($6.556\times$):
  $$\Delta V_{\text{battery\_error}} = 0.8125\text{ V} \times 6.556 \approx 5.3\text{ V}$$
- **Impact:** 🟠 **HIGH.** Telemetry will under-report the 12V battery voltage by several volts as battery voltage rises, rendering cabin battery state-of-charge monitoring completely inaccurate.
- **Remediation:** Remove the 3.3V Zener across the high-impedance divider. Instead, use a low-leakage rail clamp diode (such as a BAT54S dual Schottky diode clamping `v_out` to `V3P3` and `GND`) or increase the Zener rating to 5.1V with a dedicated current-limiting resistor to protect against load dump.
- **Authoritative Reference:**
  - Nexperia: *BZX84 series Voltage regulator diodes datasheet (Table 7: Characteristics)*.  
    URL: https://www.nexperia.com/products/diodes/zener-diodes/BZX84-C3V3.html

---

### [H-05] Unconnected SPDT Override Terminals on LED Zones
- **Files Affected:** `hardware/central-pcb/central.ato:459-479`
- **Description:**  
  Terminals `led_zone1_terminal` through `led_zone4_terminal` are 4-pin screw terminals. In `central.ato`:
  - Pin 1 is connected to `power_out` (PROFET output / AUTO).
  - Pin 2 is connected to `v_bat_12v` (12V Battery Rail).
  - Pin 4 is connected to `gnd` (Load GND return).
  - **Pin 3 has no connection whatsoever (floating copper net `led_zoneX_terminal-3`).**
- **Impact:** 🟡 **MEDIUM.** When wiring an external 3-position toggle switch (SPDT ON-OFF-AUTO) on the cabinet door, the switch common wire cannot pass through Pin 3 to reach the load via the PCB. If an installer plugs the LED strip positive lead into Pin 3 expecting a pass-through terminal, the light will receive zero power.

---

### [H-06] Unaddressed Automotive Protections & Placeholder BOM Items
- **Files Affected:** `hardware/central-pcb/central.ato:49, 83, 97, 107, 114, 124, 133, 140, 415-418`
- **Description:**  
  1. **Absence of Reverse Polarity Protection:** Connecting the main 12V battery in reverse directly exposes all 8 BTS5008-1EKB PROFETs and the MP1584EN buck converter to negative voltage, destroying them.
  2. **Absence of TVS Diode:** No transient voltage suppressor (such as SMBJ28CA) is present on the 12V rail to clamp alternator load-dump spikes (ISO 7637-2 pulse 5a).
  3. **Connector Current Rating:** Phoenix MPT 0.5 terminals are rated for **6 A maximum**. The water pump inrush current reaches 7.5 A, and aux loads easily exceed 6 A.
  4. **Fictitious LCSC SKUs:** Parts `C12345` through `C12353` and `C20683` remain in `central.ato`. These parts cannot be ordered from LCSC.
- **Impact:** 🟠 **HIGH.** Direct violation of automotive safety directives in `AGENTS.md`.
- **Authoritative Reference:**
  - ISO 7637-2: *Road vehicles — Electrical disturbances from conduction and coupling — Part 2: Electrical transient conduction along supply lines only*.
  - Phoenix Contact: *MPT 0,5/4-2,54 Product Datasheet (6A / 160V)*.  
    URL: https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-mpt-05-4-254-1725672

---

## 3. Detailed Findings: Firmware & Software Logic

### [FW-01] Critical Anti-Replay Bypass in Central Controller
- **File Affected:** `firmware/central/src/main.cpp:54-60`
- **Description:**  
  The sequence number validation in `OnDataRecv` is implemented as:
  ```cpp
  // 2. Sequence number validation (replay protection)
  // Allow reset to 1 (when remote is power-cycled) or check if seq increases.
  if (msg.seq <= lastSeqNumbers[remoteIdx] && msg.seq != 1) {
      Serial.printf("Rejected replay packet: received seq %d, last seq was %d\n", msg.seq, lastSeqNumbers[remoteIdx]);
      return;
  }
  lastSeqNumbers[remoteIdx] = msg.seq;
  ```
  **Vulnerability Analysis:**
  1. If an attacker captures the initial transmission from any remote panel where `msg.seq == 1`, the expression `msg.seq != 1` evaluates to `false`.
  2. The entire `if` condition evaluates to `false`. The packet is **accepted unconditionally**, even if `lastSeqNumbers[remoteIdx]` was at 5,000.
  3. `lastSeqNumbers[remoteIdx]` is then overwritten with `1`.
  4. Now, the attacker can replay any previously recorded packets (packets with seq 2, 3, 4, etc.) in sequential order, or replay packet 1 again.
  5. In addition, when `msgSequenceNumber` on the remote wraps around from 65,535 to 0 (`uint16_t`), packet `0` is rejected, and subsequent packets may experience lockouts until resynchronization.
- **Impact:** 🔴 **CRITICAL.** Completely invalidates replay protection against RF capturing devices (such as Flipper Zero or software-defined radios).
- **Remediation:** Implement an authentic sliding window protocol (RFC 6479) and store monotonic counter checkpoints in NVS, or require a challenge-response handshake upon remote cold-boot before accepting new counter sequences.
- **Authoritative Reference:**
  - IETF: *RFC 6479 — IPsec Anti-Replay Algorithm without Bit-Shift Operations*.  
    URL: https://datatracker.ietf.org/doc/html/rfc6479

---

### [FW-02] Ramping Direction Oscillation Bug in DimmableChannel
- **Files Affected:** `firmware/remote/src/ButtonHandler.cpp:62-67`, `firmware/central/src/DimmableChannel.cpp:35-40`
- **Description:**  
  In `ButtonHandler.cpp`, when a button is held down, the state machine enters `State::Holding` and periodically re-emits `ActionType::StartHold` every 150 ms (`HOLD_PERIOD_MS`):
  ```cpp
  } else if (now - _lastHoldTime >= HOLD_PERIOD_MS) {
      // Periodically repeat hold signal to confirm it is still held
      _lastHoldTime = now;
      actionType = ActionType::StartHold;
      return true;
  }
  ```
  In `DimmableChannel.cpp`:
  ```cpp
  case ActionType::StartHold:
      _isRamping = true;
      _lastHoldMsgTime = now;
      // Invert ramp direction at the start of hold to toggle dim/brighten
      _rampDirection = -_rampDirection;
      break;
  ```
  **Failure Walkthrough:**
  1. The user presses and holds a button to dim the lights.
  2. At 400 ms, the first `StartHold` packet is sent. `_rampDirection` flips from +1 to -1. The light begins dimming down.
  3. Over the next 150 ms (at 30 ms per step), the brightness decreases by 5 steps (5%).
  4. At 550 ms, `ButtonHandler` sends the second `StartHold` packet.
  5. `DimmableChannel::handleAction` executes `_rampDirection = -_rampDirection`. `_rampDirection` flips from -1 back to +1!
  6. Over the next 150 ms, the light brightens by 5 steps (+5%).
  7. At 700 ms, the third packet flips it back to -1 (-5%).
- **Impact:** 🔴 **CRITICAL.** Dimming via button hold does not work. The light continuously oscillates up and down by 5% in a rapid flicker loop for as long as the button is held.
- **Remediation:** In `DimmableChannel::handleAction`, only invert `_rampDirection` if `!_isRamping` (i.e. on the very first hold packet, not on subsequent keep-alive packets while ramping is already in progress).

---

### [FW-03] Data Race & Thread-Safety Violation in Central Firmware
- **Files Affected:** `firmware/central/src/main.cpp:31-64`, `firmware/central/src/SystemController.cpp:50-93`, `firmware/central/src/DimmableChannel.cpp:91-133`
- **Description:**  
  In ESP-IDF and Arduino-ESP32, the ESP-NOW receive callback (`OnDataRecv`) is executed directly in the context of `wifi_task` (priority 23).  
  Inside `OnDataRecv`:
  - `systemController.dispatchMessage()` is called.
  - `DimmableChannel::handleAction()` directly mutates `_isRamping`, `_lastHoldMsgTime`, `_rampDirection`, `_isActive`, and `_currentBrightness`.
  
  Concurrently, in the main Arduino task (`loopTask`, priority 1):
  - `systemController.update()` continuously executes `DimmableChannel::update()`, reading and writing those exact same member variables.
  
  No `portMUX_TYPE`, `SemaphoreHandle_t`, mutex, or FreeRTOS queue is used to guard this shared state.
- **Impact:** 🟠 **HIGH.** Race conditions cause visual PWM glitches, corrupted duty cycles, and potential Watchdog Timer (TWDT) panics if Wi-Fi callback execution is delayed by serial prints or state updates.
- **Authoritative Reference:**
  - Espressif Systems: *ESP-NOW API Reference (Section: Receive Callback Rules)*:  
    *"The receiving callback function also runs from the Wi-Fi task. So, do not do lengthy operations in the callback function. Instead, post the necessary data to a queue and handle it from a lower priority task."*  
    URL: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

---

### [FW-04] Cold Boot Deep Sleep Logic Trap in Remote
- **Files Affected:** `firmware/remote/src/main.cpp:58-63`
- **Description:**  
  In `remote/src/main.cpp`:
  ```cpp
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  if (wakeupReason != ESP_SLEEP_WAKEUP_EXT1) {
      Serial.println("Cold boot detected. Entering sleep immediately.");
      powerManager.goToSleep(wakeupPins, NUM_WAKEUP_PINS);
  }
  ```
  When batteries are inserted or power is connected, `wakeupReason` is `ESP_SLEEP_WAKEUP_UNDEFINED`. The code executes `goToSleep` **before** calling `buttons[i]->begin()`.  
  Because `buttons[i]->begin()` was never called, `pinMode(pin, INPUT_PULLUP)` was never configured. The pins remain in their default floating/high-impedance state. Entering deep sleep with floating pins and `ESP_EXT1_WAKEUP_ANY_LOW` immediately triggers a spurious wake-up or excessive leakage current.
- **Impact:** 🟠 **HIGH.** Erroneous startup behavior and rapid battery drain on cold boot.

---

### [FW-05] Rotary Encoder Polling Rate Violates Nyquist Limit
- **Files Affected:** `firmware/remote/src/main.cpp:110`, `firmware/remote/src/EncoderHandler.cpp:20-36`
- **Description:**  
  In `remote/src/main.cpp`, `loop()` calls `delay(5)` on every cycle. `EncoderHandler::update()` is only called every ~5 to 6 ms.  
  A standard EC11 rotary encoder produces 20 or 24 pulses per rotation. Rotating the knob rapidly (2 to 3 turns per second) produces pulse widths as narrow as 1 to 2 ms. Polling at 5 ms samples below the Nyquist rate, resulting in dropped steps and false reverse-direction detections. Furthermore, the half-quadrature decoding algorithm only checks the falling edge of signal A.
- **Impact:** 🟡 **MEDIUM.** Poor rotary encoder responsiveness and erratic brightness steps during normal to fast turns.
- **Remediation:** Remove `delay(5)` or implement quadrature decoding using GPIO pin change interrupts (ISRs) or the ESP32-C6 hardware PCNT (Pulse Counter) peripheral.

---

### [FW-06] PlatformIO Configuration Trap in Central
- **Files Affected:** `firmware/central/platformio.ini:9-17`
- **Description:**  
  In `firmware/central/platformio.ini`, `[env:seeed_xiao_esp32c6]` is declared as the first environment before `[env:esp32-s3-devkitc-1]`. PlatformIO treats the first defined environment as the default target. Anyone running `pio run` or using IDE build shortcuts without specifying `-e esp32-s3-devkitc-1` will build the central firmware for a Seeed Studio XIAO ESP32-C6 board instead of the ESP32-S3.
- **Impact:** 🟡 **MEDIUM.** Developer confusion, erroneous uploads, and build mismatches.

---

### [FW-07] Hardcoded Placeholder MAC Addresses Break Out-of-the-Box Communication
- **Files Affected:** `firmware/central/src/main.cpp:20-23`, `firmware/remote/src/main.cpp:28`
- **Description:**  
  `centralMacAddress` in `remote` is set to `{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}`, and `remoteMacs` in `central` is set to `{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11}`.  
  Because ESP-NOW link encryption is active (`peerInfo.encrypt = true`), the ESP32 hardware radio will silently drop all encrypted frames whose sender MAC address does not match an existing entry in the hardware peer table. The system will fail completely until both devices are manually reprogrammed with matching eFuse MACs.
- **Impact:** 🟡 **MEDIUM.** No out-of-the-box pairing; difficult remote replacement.

---

### [FW-08] Unmapped Hardware Output Channels (Dead Code)
- **Files Affected:** `firmware/central/src/SystemController.cpp:11-21, 65-85`
- **Description:**  
  Channels 5, 6, 7 (Aux Outlets 1–3) and Channel 10 (DC-DC Charger) are allocated in memory, configured as GPIO outputs, and powered on the PCB, but `dispatchMessage()` has zero mapping logic to activate or toggle them from any remote panel.
- **Impact:** 🟢 **LOW.** Wasted resources and unfunctional auxiliary hardware.

---

## 4. Verification & Authoritative References Matrix

| ID | Issue Description | Authoritative Source / Standard | URL Citation / Reference |
| :--- | :--- | :--- | :--- |
| **H-01** | ESP32-S3 J3 Pin 1 is GND; J1 Pin 21 is 5V VIN; off-by-one pinout | Espressif Systems ESP32-S3-DevKitC-1 v1.1 Official User Guide & Schematic | [Espressif User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html) / [Schematic PDF](https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf) |
| **H-02** | XIAO ESP32-C6 D3 is GPIO 21; EXT1 limited to LP-GPIOs (0–7); GPIO 3 is RF switch | Seeed Studio XIAO ESP32C6 Docs & Espressif ESP32-C6 Sleep Modes API | [Seeed Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/) / [ESP-IDF Sleep Modes](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/sleep_modes.html) |
| **H-03** | BTS5008-1EKB switching times (250 µs) prohibit 5 kHz PWM (200 µs period) | Infineon BTS5008-1EKB Official Datasheet (Section 5.3) | [Infineon Product Page](https://www.infineon.com/cms/en/product/power/smart-power-switches/high-side-switches/12-v-high-side-switches/profet-12v/bts5008-1ekb/) |
| **H-04** | Low-voltage 3.3V Zener tunneling leakage distorts 16.25 kΩ ADC divider | Nexperia BZX84-C3V3 Voltage Regulator Diodes Datasheet (Table 7) | [Nexperia Datasheet](https://www.nexperia.com/products/diodes/zener-diodes/BZX84-C3V3.html) |
| **H-06** | Automotive reverse polarity, load dump (ISO 7637-2), connector ratings | ISO 7637-2 Standard & Phoenix Contact MPT 0,5/4-2,54 Datasheet | [Phoenix Contact 1725672](https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-mpt-05-4-254-1725672) |
| **FW-01** | `msg.seq != 1` bypasses replay protection; counter wrapping | IETF RFC 6479 (IPsec Anti-Replay Algorithm) | [RFC 6479](https://datatracker.ietf.org/doc/html/rfc6479) |
| **FW-02** | Periodic hold keep-alives invert ramp direction every 150 ms | C++ State Machine Review & `DimmableChannel.cpp` logic | Verified through code execution flow |
| **FW-03** | ESP-NOW receive callback runs in `wifi_task`, causing race condition with `loopTask` | Espressif ESP-NOW API Documentation (Receive Callback Rules) | [Espressif ESP-NOW API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html) |

---

## 5. Prioritized Remediation Roadmap

### Phase 1: Hardware Safety & Electrical Correction (Mandatory before PCB Fabrication)
1. **Fix `LonelyBinary_ESP32S3_N16R8` in `central.ato`:**
   - Move `VIN` from `right_header.pin_1` to `left_header.pin_21`.
   - Re-align all pins on `right_header`:
     - Pin 1: `GND`
     - Pin 4: `signal GPIO1` (Battery ADC)
     - Pin 5: `signal GPIO2` (Cabin LED)
     - Pin 18: `signal GPIO21` (Inverter Opto)
     - Pin 19: `signal GPIO20` (Reserved / NC)
     - Pin 20: `signal GPIO19` (Reserved / NC)
     - Pins 21 and 22: `GND`
2. **Replace 3.3V Zener Diode in `AnalogVoltageDivider`:**
   - Remove `zener = new ZenerDiode` across `v_out`.
   - Implement dual Schottky clamp diodes (BAT54S) from `v_out` to `V3P3` and `GND` to clamp transients without quiescent leakage error.
3. **Correct Dimming PWM Frequency:**
   - In `DimmableChannel.h`, change default frequency from `5000` to `200` (200 Hz).
4. **Resolve Remote Hardware Constraints:**
   - Use a microcontroller board with sufficient exposed LP-GPIOs (such as Seeed Studio XIAO ESP32-S3, which exposes GPIOs 1–6 on its headers, all supporting ADC and deep sleep wakeup) or use a 3-button + encoder layout that fits within pins D0, D1, and D2.

### Phase 2: Firmware Logic & Security Fixes
5. **Fix Dimming Ramping Inversion in `DimmableChannel.cpp`:**
   ```cpp
   case ActionType::StartHold:
       if (!_isRamping) {
           _isRamping = true;
           _rampDirection = -_rampDirection; // Only toggle direction on the FIRST hold event
       }
       _lastHoldMsgTime = now;
       break;
   ```
6. **Eliminate Anti-Replay Loophole in `main.cpp`:**
   - Remove `&& msg.seq != 1`.
   - Implement a cryptographic challenge or pairing handshake for initial connection.
7. **Thread-Safe ESP-NOW Dispatching:**
   - Allocate a FreeRTOS queue (`QueueHandle_t xMessageQueue = xQueueCreate(16, sizeof(QueueItem))`).
   - In `OnDataRecv`, only enqueue the incoming struct and return immediately.
   - Process messages and call `systemController.dispatchMessage()` inside `loop()` or a dedicated worker task.
8. **Fix Cold Boot Sequence in `remote/main.cpp`:**
   - Call `buttons[i]->begin()` and configure pull-ups *before* checking the wakeup cause and entering sleep.
9. **Remove `env:seeed_xiao_esp32c6` from `firmware/central/platformio.ini`:**
   - Ensure `[env:esp32-s3-devkitc-1]` is the default environment for central.
