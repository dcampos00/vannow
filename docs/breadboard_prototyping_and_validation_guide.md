# VanNOW — Breadboard Prototyping & Hardware Validation Guide

This document is the authoritative, progressive engineering guide for assembling, wiring, and validating the **VanNOW** camper van automation system on a solderless breadboard.

To guarantee maximum hardware safety and eliminate the risk of damaging microcontrollers, this guide is strictly partitioned into two progressive phases:
1. **Phase A: Safe 5V USB Bench Test (Zero 12V Required):** Validates 90% of the system—firmware state machines, ESP-NOW wireless links, 200 Hz PWM dimming curves, rotary encoder quadrature decoding, NVS flash persistence, and sub-18 µA Deep Sleep—using only USB power, 2x AA batteries, and standard 5mm indicator LEDs.
2. **Phase B: 12V Automotive Power & High-Current Switching:** Validates DC-DC buck regulation (MP1584EN), precision battery ADC telemetry (100k/18k divider), high-current logic MOSFET switching, inductive flyback clamping (1N5408), and optocoupler galvanic isolation once a 12V source (bench PSU, 12V wall adapter, or fused vehicle battery) is ready.

---

## 1. Prototype Bill of Materials (BOM)

### 1.1 Phase A: Low-Voltage Bring-Up (5V USB & 3V Battery Only)
| Qty | Item | Model / Specification | Purpose |
| :---: | :--- | :--- | :--- |
| **1** | Central Microcontroller | **Lonely Binary ESP32-S3 N16R8** (or DevKitC-1) | Receiver, 24-channel dispatch, PWM, NVS, WDT |
| **1** | Remote Microcontroller | **Seeed Studio XIAO ESP32-C6** | Transmitter, Deep Sleep (~15 µA), Diode-OR wake |
| **2** | Data Cables | USB-A to USB-C (or USB-C to USB-C) | 5V power supply, flashing, and serial telemetry |
| **1** | Remote Battery Holder | **2x AA Battery Holder** (with switch/leads) | Direct 3.0V supply to XIAO ESP32-C6 `3V3` rail |
| **2** | AA Alkaline Batteries | Standard 1.5V AA cells | Remote power source (~3.0V) |
| **1** | Rotary Encoder | **Alps EC11** with integrated push switch | Master dimmer dial and active zone toggle |
| **4** | Tactile Pushbuttons | Standard 6x6 mm or 12x12 mm breadboard buttons | Zone 1, Zone 2, Water Pump, All-OFF inputs |
| **4** | Small Signal Diodes | **BAT54**, **BAS70**, or **1N4148** | Hardware Diode-OR wakeup matrix |
| **1** | Pull-Up Resistor | **47 kΩ**, 1/4W | Diode-OR wakeup bus pull-up to 3.3V |
| **4** | Current Limiting Resistors | **220 Ω** or **330 Ω**, 1/4W | Current limiting for breadboard test LEDs |
| **3** | Indicator LEDs | Green, Red, Yellow 5mm LEDs | Simulates Zone 1 Dimmer, Water Pump, and Buzzer |
| **2** | Solderless Breadboards | Standard 400-tie or 830-tie breadboards | One for Central, one for Remote |
| **1** | Jumper Wire Kit | Male-to-Male & Male-to-Female jumper wires | Circuit interconnection |
| **1** | Digital Multimeter (DMM) | With µA DC measurement capability | Measuring Remote Deep Sleep current |

### 1.2 Phase B: 12V Automotive Power & Load Switching (Add When Ready)
| Qty | Item | Model / Specification | Purpose |
| :---: | :--- | :--- | :--- |
| **1** | 12V Power Source | 12V 1A–2A wall adapter, bench PSU, or fused van battery | Main 12V supply for buck and loads |
| **1** | DC-DC Step-Down Regulator | **MP1584EN module** or **Pololu D24V10F5** | Steps 12V down to regulated 5.0V for Central |
| **1** | Inline Fuse Holder & Fuse | Automotive Mini blade fuse (**5A**) | Over-current protection on 12V rail |
| **1** | 4-Channel Logic MOSFET Module | **LR7843** or **AOD4184** (3.3V gate logic) | High-current N-channel PWM load switching |
| **1** | Optocoupler Breakout Module | **PC817 2-channel or 4-channel module** | Galvanically isolated dry contact for inverter |
| **1** | Power Diode | **1N5408** (3A, 1000V) or **1N5822** (3A Schottky) | Inductive flyback suppression across pump |
| **1** | Precision Resistor | **100 kΩ**, 1/4W, 1% tolerance | Voltage divider high-side (battery sensing) |
| **1** | Precision Resistor | **18 kΩ**, 1/4W, 1% tolerance | Voltage divider low-side (battery sensing) |
| **1** | Resistor | **1 kΩ**, 1/4W | Divider ADC series protection resistor |
| **1** | Ceramic Capacitor | **100 nF (0.1 µF)**, 50V | ADC anti-aliasing filter capacitor |
| **1** | SPDT Toggle Switch | 3-position ON-OFF-ON (10A-15A @ 12V) | External manual emergency bypass switch |
| **1** | 12V Test Load | 12V LED light strip segment (0.5m) or 12V motor | Validating high-current PWM and flyback |

---

## 2. Phase A: Safe 5V USB Bench Test (Zero 12V Required)

In this phase, **no 12V power is used**. The Central ESP32-S3 is powered directly via its USB-C port, and the Remote XIAO ESP32-C6 is powered either via 2x AA batteries or its own USB-C port. Standard 5mm LEDs simulate lighting and pump channels directly from the MCU GPIOs.

### 2.1 Central Controller (ESP32-S3) 5V Test Schematic

```
   [ PC / USB-C Charger (5V) ]
                |
          (USB-C Cable)
                |
                v
   LONELY BINARY ESP32-S3 N16R8
   +---------------------------------------+
   |                                       |
   |  GPIO 12 (PWM Zone 1) ---> [ 220 Ω ] ---> Anode (+) Green LED ---> GND
   |  GPIO 4  (Water Pump) ---> [ 220 Ω ] ---> Anode (+) Red LED   ---> GND
   |  GPIO 4  (Pump chirp is the pump output itself; no separate buzzer GPIO)
   |                                       |
   |  GND ---------------------------------+---> Common Ground Rail
   +---------------------------------------+
```

### 2.2 Remote Unit (Seeed XIAO ESP32-C6) Schematic

```
       2x AA Batteries (~3.0V)
           (+) --------------------------------> XIAO ESP32-C6 Pin 3V3
           (-) --------------------------------> XIAO ESP32-C6 Pin GND

       WAKEUP & BUTTON MATRIX:

       3V3 Rail ---[ 47k Pull-up ]---+
                                     |
                                     +---------> XIAO Pin D0 (LP-GPIO 0: WAKEUP BUS)
                                     |
       Button 1 (Zone 1)   ---|--|<|-+ (Diode D1: Cathode to Button, Anode to Bus)
       Button 2 (Zone 2)   ---|--|<|-+ (Diode D2: Cathode to Button, Anode to Bus)
       Button 3 (Pump)     ---|--|<|-+ (Diode D3: Cathode to Button, Anode to Bus)
       Encoder Push Button ---|--|<|-+ (Diode D4: Cathode to Button, Anode to Bus)

       SENSE PINS (Active LOW when button is pressed):
       Button 1 Output ----> XIAO Pin D1 (GPIO 1)
       Button 2 Output ----> XIAO Pin D2 (GPIO 2)
       Button 3 Output ----> XIAO Pin D3 (GPIO 21)
       Encoder Push    ----> XIAO Pin D6 (GPIO 19)

       ROTARY ENCODER (EC11):
       Terminal A (CLK) ---> XIAO Pin D4 (GPIO 22)
       Terminal B (DT)  ---> XIAO Pin D5 (GPIO 23)
       Terminal C (COM) ---> GND

       STATUS MICRO-LED:
       XIAO Pin D7 (GPIO 20) ---> [ 220 Ω ] ---> Anode (+) Green LED ---> GND
```

### 2.3 Phase A Pin-by-Pin Connection Table

| Subsystem | Source Component | Pin | Target Component | Pin | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Central** | USB-C Port | VBUS | PC / USB Charger | USB | Powers ESP32-S3 with clean 5.0V |
| **Central** | ESP32-S3 | `GPIO 12` | Resistor 220 Ω | Pin 1 | Simulates Zone 1 Dimmer output |
| **Central** | Resistor 220 Ω | Pin 2 | 5mm Green LED | Anode (+) | Visual PWM dimming indicator |
| **Central** | 5mm Green LED | Cathode (-) | ESP32-S3 | `GND` | Ground return |
| **Central** | ESP32-S3 | `GPIO 4` | Resistor 220 Ω | Pin 1 | Simulates Water Pump output |
| **Central** | Resistor 220 Ω | Pin 2 | 5mm Red LED | Anode (+) | Visual Water Pump indicator |
| **Central** | 5mm Red LED | Cathode (-) | ESP32-S3 | `GND` | Ground return |
| **Central** | ESP32-S3 | `GPIO 4` | Resistor 220 Ω | Pin 1 | Pump / shower chirp (same output) |
| **Central** | Resistor 220 Ω | Pin 2 | 5mm Yellow LED | Anode (+) | Visual Shower Mode chirp indicator |
| **Central** | 5mm Yellow LED | Cathode (-) | ESP32-S3 | `GND` | Ground return |
| **Remote** | 2x AA Battery Holder | `+` (Red) | XIAO ESP32-C6 | `3V3` | Pure 3.0V supply (bypasses LDO) |
| **Remote** | 2x AA Battery Holder | `-` (Black)| XIAO ESP32-C6 | `GND` | Common ground |
| **Remote** | 3V3 Rail | `3V3` | Resistor 47 kΩ | Pin 1 | Pull-up for Diode-OR wakeup line |
| **Remote** | Resistor 47 kΩ | Pin 2 | XIAO ESP32-C6 | `D0` (LP_GPIO0)| Wakeup Bus input |
| **Remote** | Diode D1..D4 | Anodes | Wakeup Bus | `D0` | Combined wakeup trigger |
| **Remote** | Diode D1 | Cathode | Button 1 & XIAO | `D1` (GPIO 1) | Zone 1 sense line |
| **Remote** | Diode D2 | Cathode | Button 2 & XIAO | `D2` (GPIO 2) | Zone 2 sense line |
| **Remote** | Diode D3 | Cathode | Button 3 & XIAO | `D3` (GPIO 21)| Water Pump sense line |
| **Remote** | Diode D4 | Cathode | Encoder Push & XIAO| `D6` (GPIO 19)| Encoder button sense line |
| **Remote** | Buttons 1..3 & SW | Other Pin | XIAO ESP32-C6 | `GND` | Pulled to GND when pressed |
| **Remote** | EC11 Encoder | Pin A (CLK) | XIAO ESP32-C6 | `D4` (GPIO 22)| Quadrature phase A |
| **Remote** | EC11 Encoder | Pin B (DT) | XIAO ESP32-C6 | `D5` (GPIO 23)| Quadrature phase B |
| **Remote** | EC11 Encoder | Pin C (COM) | XIAO ESP32-C6 | `GND` | Common ground |
| **Remote** | XIAO ESP32-C6 | `D7` (GPIO 20)| Resistor 220 Ω | Pin 1 | Remote status feedback LED |

---

### 2.4 Phase A Step-by-Step Validation Protocol

#### Step A1: Firmware Build & Flash
1. Connect ESP32-S3 to computer via USB-C. Flash Central:
   ```bash
   pio run -d firmware/central -t upload
   ```
2. Connect XIAO ESP32-C6 to computer via USB-C. Flash Remote:
   ```bash
   pio run -d firmware/remote -t upload
   ```
3. Open serial monitors:
   ```bash
   pio device monitor -d firmware/central
   pio device monitor -d firmware/remote
   ```

#### Step A2: ESP-NOW Wireless Pairing & Anti-Replay Verification
1. Tap Button 1 on the Remote breadboard.
2. In the Remote serial terminal, verify:
   ```
   [Remote] Button 0 CLICK -> Sending packet seq=1
   ```
3. In the Central serial terminal, verify instant reception (< 12 ms):
   ```
   [Central] Received packet from Remote 1, Button 0, Action: Click, RSSI: -42 dBm
   [Central] Channel 0 (Zone 1) toggled -> ON (Brightness: 255)
   ```
4. Observe the **Green LED on GPIO 12**: It turns ON instantly.
5. Tap Button 1 again: Verify Central receives `seq=2` and turns the Green LED OFF.
6. Verify anti-replay: Replaying an old sequence number packet will be rejected with `Sequence out of window (RFC 6479)`.

#### Step A3: Smooth 200 Hz LEDC PWM Dimming & Encoder Ramping
1. Tap Button 1 to turn Zone 1 ON (Green LED lit).
2. Rotate the **EC11 rotary knob** clockwise:
   - Observe the Green LED brightness smoothly increasing.
   - The Central LEDC driver updates duty cycle in 1% steps.
3. Rotate counter-clockwise:
   - Observe the Green LED smoothly dimming down to 1% without extinguishing.
4. Press and hold Button 1 for > 500 ms:
   - Verify continuous hold-ramping dimmer mode (emits keep-alive packets every 150 ms).
   - The LED ramps brightness smoothly up and down. Release button to lock brightness.

#### Step A4: Shower Mode Acoustic Chirp & Water Pump Timer
1. Tap Button 3:
   - The **Red LED on GPIO 4** (Water Pump) turns ON.
   - Tap Button 3 again: Red LED turns OFF.
2. **Double-click Button 3** (within 350 ms):
   - Shower Mode activates!
   - Observe the **Yellow LED on GPIO 48** (Acoustic Chirp): It executes the exact two-pulse acoustic pattern:
     `150 ms ON -> 120 ms PAUSE -> 150 ms ON -> 120 ms PAUSE -> OFF`
   - The Red LED turns ON and runs on the 5-minute safety countdown.
3. Tap Button 3 once during Shower Mode: Verify it cancels immediately and turns the Red LED OFF.

#### Step A5: Non-Volatile Storage (NVS) State Persistence
1. Set Zone 1 to 40% brightness. Turn Zone 2 ON.
2. Unplug the USB-C cable from the Central ESP32-S3 (simulating van power loss / brownout).
3. Re-plug the USB-C cable.
4. In the Central serial monitor, verify:
   ```
   [NVS] Restoring channel 0: ON, brightness=102 (40%)
   [NVS] Restoring channel 4: OFF (Safety Policy: restoreOnBoot=false)
   ```
5. The Green LED immediately lights up at exactly 40% brightness, and the Red LED (pump) remains safely OFF.

#### Step A6: Ultra-Low-Power Deep Sleep Current Measurement (~15 µA)
1. Disconnect USB-C from the Remote XIAO ESP32-C6.
2. Set your digital multimeter to **µA DC mode**.
3. Wire the multimeter in series between the 2x AA battery (+) terminal and the XIAO `3V3` pin:
   ```
   Battery (+) ---> Multimeter RED probe
   Multimeter BLACK probe ---> XIAO ESP32-C6 pin 3V3
   Battery (-) ---> XIAO ESP32-C6 pin GND
   ```
4. Observe the quiescent current on the DMM:
   - **Active Wake (Button press & RF transmit):** 30–45 mA for ~180 ms.
   - **Deep Sleep:** **14.2 µA – 17.8 µA**.
5. Press Button 1: Current briefly spikes to ~35 mA, the Central responds, and current instantly collapses back to **~15 µA**.
6. This confirms battery life of **> 2.5 years** on two standard AA cells.

---

## 3. Phase B: 12V Automotive Power & Load Switching Stage

Once you have completed Phase A and have access to a 12V power source, proceed to Phase B.

### 3.1 Safe Ways to Obtain 12V for Bench Testing (Without Risking the Van)
1. **Old 12V Wall Power Adapter (Recommended):** An unused 12V 1A–2A power brick from a discarded Wi-Fi router, TV set-top box, or external hard drive. These adapters are isolated, current-limited, and safe to use on a breadboard.
2. **Standard 9V Battery (PP3 / 6F22):** A small 9V transistor battery can power the MP1584EN buck converter and the 100k/18k voltage divider with zero risk of damage.
3. **Bench Power Supply:** Set output to 12.0V with current limit set to 1.0A.
4. **Fused Van Battery:** If using the vehicle battery, **always wire an inline 5A blade fuse** right at the positive terminal before running wires to the breadboard.

---

### 3.2 Phase B Central Controller Wiring (12V Integration)

```
                       +-------------------------------+
       +12V Source ----| IN+      MP1584EN        OUT+ |---+ (+5.0V Regulated)
       GND  Source ----| IN-   (Tuned to 5.0V)    OUT- |---| (GND Return)
                       +-------------------------------+   |
                                                           |
          +------------------------------------------------+
          |
          |         LONELY BINARY ESP32-S3 N16R8
          |         +---------------------------+
          +-------->| 5V (or VIN)               |
          +-------->| GND                       |
                    |                           |
                    |    GPIO 12 (PWM Zone 1)   |---> 1k ---> Gate (LR7843 MOSFET Module)
                    |    GPIO 4  (Water Pump)   |---> IN1 (PROFET / MOSFET Module)
                    |    GPIO 21 (Inverter Dry) |---> IN1 (PC817 Optocoupler Module)
                    |    GPIO 48 (Status/Buzzer)|---> 220R -> Anode (+) Buzzer / LED -> GND
                    |                           |
                    |    GPIO 1  (Battery ADC)  |<---+
                    +---------------------------+    |
                                                     |
  +12V Battery Bus ----[ 100k (1%) ]---+             |
                                       |             |
                                       +---[ 1k ]----+
                                       |
                                       +---[ 18k (1%) ]---> GND
                                       |
                                       +---[ 100nF ]------> GND

  HIGH-CURRENT 12V LOAD SWITCHING:
  +12V Bus ----------+-------------------------+
                     |                         |
               [ +12V LED Strip ]        [ 12V Water Pump ]
                     |                         |   ^
                     |                         |   | 1N5408 Diode (Cathode to +12V)
                     v                         v   |
               Drain (LR7843)            Drain (PROFET / MOSFET)
                     |                         |
               Source to GND             Source to GND
```

---

### 3.3 Phase B Validation Protocol

#### Step B1: MP1584EN DC-DC Buck Regulator Calibration
1. **CRITICAL SAFETY STEP:** **DO NOT connect the ESP32-S3 yet.**
2. Connect your 12V source to MP1584EN `IN+` and `IN-`.
3. Measure `OUT+` with your multimeter in DC Volts mode.
4. Using a small flathead screwdriver, slowly turn the brass trimmer potentiometer until the output reads **5.00V ± 0.05V**.
5. Disconnect 12V power. Now connect `OUT+` to ESP32-S3 `5V` and `OUT-` to `GND`.
6. Reapply 12V. Verify the ESP32-S3 boots cleanly from the buck converter.

#### Step B2: Precision Battery Voltage Divider Calibration (GPIO 1)
1. Connect the 100 kΩ / 18 kΩ divider to the 12V bus and GPIO 1 as shown in the schematic.
2. Measure actual 12V source voltage with DMM: e.g., V_in = 12.50V.
3. Measure scaled voltage at GPIO 1:
   V_GPIO1 = 12.50V * (18 kΩ / (100 kΩ + 18 kΩ)) = 1.907V
4. Open the Central serial monitor. Verify the reported cabin battery voltage matches your DMM within 1.5%:
   ```
   [Telemetry] Cabin Battery = 12.51V (ADC raw: 1912)
   ```

#### Step B3: High-Current Logic-Level MOSFET Switching & Thermal Check
1. Connect a 12V LED strip segment to the LR7843 MOSFET module.
2. Drive the gate from GPIO 12 with the 200 Hz PWM dimmer.
3. Turn Zone 1 to 100% brightness. Let it run for 10 minutes.
4. Touch the LR7843 MOSFET: It should remain **cool to the touch (< 35°C)**.
   *(Note: An improper MOSFET like the IRF520 would overheat because 3.3V logic cannot saturate its gate).*

#### Step B4: Inductive Flyback Diode Verification (1N5408)
1. Connect a 12V inductive load (pump motor or relay) to Channel 4.
2. Ensure the **1N5408 diode is connected in parallel with the load** (Cathode to +12V, Anode to the switched negative side).
3. If an oscilloscope is available, probe the switched negative line:
   - When Channel 4 turns OFF, verify the inductive voltage spike is clamped to **< 13.5V**.
   - If no oscilloscope is available, toggle the pump 20 times rapidly. The MOSFET/driver must not degrade or fail.

#### Step B5: Optocoupler Galvanic Isolation (PC817)
1. Connect GPIO 21 to the PC817 input channel.
2. Connect your multimeter in **Continuity / Resistance mode** across the PC817 output pins (Pins 3 and 4).
3. Send Inverter toggle command from the Remote:
   - **Inverter ON:** Multimeter beeps / resistance drops to < 5 Ω.
   - **Inverter OFF:** Resistance reads open circuit (OL / > 10 MΩ).
4. Verify complete electrical isolation: No continuity exists between the ESP32 ground and the PC817 output terminals.

---

## 4. Pre-Manufacturing Sign-Off Matrix

Before submitting Gerber files to JLCPCB or PCBWay, ensure all items below are checked:

| Test Item | Pass Criteria | Phase A Verified | Phase B Verified |
| :--- | :--- | :---: | :---: |
| **ESP-NOW RF Link** | 0 dropped packets over 10 meters | [ ] | [ ] |
| **Remote Deep Sleep** | Quiescent current <= 18 µA on 2x AA | [ ] | N/A |
| **Diode-OR Wakeup** | Wakes MCU in < 2 ms upon keypress | [ ] | N/A |
| **Dimmer PWM Response** | Smooth 1% brightness fading at 200 Hz | [ ] | [ ] |
| **NVS Persistence** | Restores lights, keeps pump OFF after boot | [ ] | [ ] |
| **Safety Auto-Off** | Water pump cuts off automatically at 10 min | [ ] | [ ] |
| **5V Buck Rail** | MP1584 delivers 5.00V ± 0.05V | N/A | [ ] |
| **ADC Telemetry** | Reading accurate within ± 1.5% vs DMM | N/A | [ ] |
| **Flyback Clamp** | 1N5408 clamps inductive kickback < 14V | N/A | [ ] |
| **MOSFET Gate Saturation** | LR7843 runs cool (< 35°C) at full load | N/A | [ ] |
| **Optocoupler Isolation** | PC817 dry contact switches cleanly | [ ] | [ ] |
