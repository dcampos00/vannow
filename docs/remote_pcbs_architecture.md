# VanNOW — Cockpit Switch Hub & Master Magnetic Remote Architectural Specification

This document presents the UX evaluation, electrical schematics, mechanical integration, and firmware protocol mapping for the two new remote units in the **VanNOW** camper ecosystem:
1. **Cockpit Dashboard Switch Box (`cockpit-pcb`):** A hidden, battery-powered interface transmitter behind the van dash connecting directly to user-supplied automotive Carling rocker switch blanks.
2. **Master Magnetic Entrance Remote (`entrance-remote-pcb`):** A dockable multi-button and rotary dimmer panel retained magnetically at the van sliding door.

---

## 1. UX & Ergonomic Evaluation: Master Entrance Remote Dimming & Configurations

The user requested an evaluation comparing **Option 1 (Buttons + Rotary Dimmer Knob)** versus **Option 3 (Expanded 10–12 Button Matrix / Dedicated Configuration Buttons)** for intuitive van lighting control.

```
+-----------------------------------------------------------------------------------------------+
|                       COMPARATIVE EVALUATION: VAN ENTRANCE CONTROLLER                         |
+------------------------------+--------------------------------+-------------------------------+
| CRITERIA                     | OPTION 1: 6 BUTTONS + ENCODER  | OPTION 3: 10-12 BUTTON MATRIX |
+------------------------------+--------------------------------+-------------------------------+
| Primary Interaction          | "Select Zone -> Turn Knob"     | "One-Touch Scene / Function"  |
| Dimming Granularity          | 1% steps (tactile detents)     | Timed Hold or 3-step presets  |
| Mechanical Complexity        | Rotary shaft + 6 push buttons  | 10-12 membrane / tactile keys |
| Darkness / Blind Ergonomics  | Tactile knob easily found      | Requires searching key grid   |
| Multi-Scene / Presets        | Click encoder for mode/preset  | Dedicated button per scene    |
| Existing 3D CAD Alignment    | 100% matched to 86x86 mm CAD   | Requires larger faceplate     |
+------------------------------+--------------------------------+-------------------------------+
```

### 1.1 The Challenge: Multi-Zone Dimming in a Camper Van
A camper van has 4 distinct dimmable lighting zones:
- **Zone 1:** Main Ceiling Downlights (200 Hz PWM)
- **Zone 2:** Kitchen Galley Worktop (200 Hz PWM)
- **Zone 3:** Bed / Reading Nook (200 Hz PWM)
- **Zone 4:** Porch / Exterior Awning (PROFET Aux 1)

In addition, it controls high-power appliances: **Water Pump (Seaflo 7.5A)**, **Inverter (Multiplus II)**, and **Ceiling Vent Fan**.

### 1.2 Paradigm Analysis

#### Paradigm A: "Select & Turn" (Option 1 — 6 Buttons + EC11 Rotary Encoder)
- **How It Works:**
  1. Each zone has its own tactile button with a label/icon (Ceiling, Galley, Bed, Porch).
  2. Tapping a button toggles that zone ON/OFF immediately and selects it as the *Active Focus Zone* for 3.0 seconds.
  3. Turning the EC11 rotary knob adjusts the brightness of the active zone in real time with continuous tactile feedback (clicks).
  4. Pushing the encoder knob itself operates as **Instant 100% Boost** or cycles through brightness presets (20% -> 50% -> 100%).
  5. Buttons 5 and 6 are dedicated to **Water Pump Toggle** and **Master Night Shutdown (All OFF)**.
- **Ergonomic Verdict:** **Superior for fine-grained lighting comfort.** When reading or cooking, you can set the exact lumen output in 0.5 seconds without waiting for a slow software ramp.

#### Paradigm B: "Dedicated Scene Presets" (Option 3 — 10–12 Button Keypad Matrix)
- **How It Works:**
  Instead of manual dimming adjustments every time, van lighting is organized into pre-programmed camper scenes:
  - **Button 1 (Day / Bright):** Ceiling 100%, Galley 100%, Bed OFF.
  - **Button 2 (Galley / Cooking):** Galley 100%, Ceiling 40%, Fan ON.
  - **Button 3 (Evening / Cozy):** Ceiling 20%, Bed 25%, Warm ambient.
  - **Button 4 (Night / Sleep):** All lights OFF, Water Pump OFF.
  - **Button 5 (Exterior / Campsite):** Porch ON 80%, Step lights ON.
  - **Buttons 6–10:** Individual toggles for Water Pump, Inverter, Vent Fan, Fridge, Aux.
- **Ergonomic Verdict:** **Superior for zero-cognitive-load operation.** Any guest, child, or tired traveler can hit "Bedtime" or "Cooking" and the entire van configures itself without touching individual dimmers.

### 1.3 Recommended Architectural Decision: The Unified Hybrid Model
We can achieve the best of both paradigms within the **exact 86 mm x 86 mm physical form factor** already designed in 3D CAD:
1. **Short Tap on Zone Button:** Toggles ON/OFF.
2. **Double Tap on Zone Button:** Activates that Zone's Favorite Preset (e.g. 30% Nightlight).
3. **Turn Rotary Knob:** Adjusts brightness of the active zone up/down.
4. **Dedicated Master Night Shutdown Button:** One-touch full van shutdown.
5. **Hardware Compatibility:** The carrier board PCB is engineered with standard matrix traces, allowing the exact same microcontroller firmware to support either:
   - The 6-Button + EC11 Encoder Faceplate (Dockable remote).
   - A 10-Keypad tactile membrane panel if preferred.

---

## 2. Cockpit Dashboard Switch Box (`cockpit-pcb`)

### 2.1 Concept & Vehicle Integration
Modern camper vans (Mercedes Sprinter, Ford Transit, Ram ProMaster, VW Crafter) feature standard blank switch plates on the center console and lower dash. Installing custom all-in-one screens or surface boxes on the dash disrupts vehicle aesthetics and creates clutter.

```
DASHBOARD INTERFACE ARCHITECTURE:
+-------------------------------------------------------------------------+
| VEHICLE COCKPIT DASHBOARD                                               |
|                                                                         |
|   [ Carling Blank 1 ]  [ Carling Blank 2 ]  [ Carling Blank 3 ]         |
|   (Exterior Driving)   (Orion-XS DC-DC)     (Cabin Main Lights)         |
|   [ Carling Blank 4 ]  [ Carling Blank 5 ]  [ Carling Blank 6 ]         |
|   (Water Pump)         (Inverter AC)        (Aux 12V / Fan)             |
|            |                    |                    |                  |
|            +--------------------+--------------------+                  |
|                                 | (Automotive Signal Harness, 22 AWG)   |
|                                 v                                       |
|               +-----------------------------------+                     |
|               | COCKPIT TRANSMITTER BOX (HIDDEN)  |                     |
|               | - 6x Screw Terminal Block         |                     |
|               | - Diode-OR Wakeup (BAS70-04)      |                     |
|               | - Seeed Studio XIAO ESP32-C6      |                     |
|               | - 2x AA Battery Bay (Internal)    |                     |
|               | - Sub-15 uA Deep Sleep            |                     |
|               +-----------------------------------+                     |
|                                 |                                       |
|                                 | ESP-NOW (Encrypted, 2.4 GHz)          |
|                                 v                                       |
|                  [ Central Relay Electrical Cabinet ]                   |
+-------------------------------------------------------------------------+
```

### 2.2 Technical Specifications

| Parameter | Specification | Notes |
| :--- | :--- | :--- |
| **Enclosure Envelope** | $75.0\,\text{mm} \times 52.0\,\text{mm} \times 24.0\,\text{mm}$ | Compact ABS enclosure with dual M4 mounting tabs. |
| **Power Supply** | $2\times\,\text{AA}$ Alkaline Cells ($3.0\,\text{V}$ nominal) | Direct connection to $3.3\,\text{V}$ rail. Shelf life $>5$ years. |
| **Standby Current** | **$14.8\,\mu\text{A}$** in Deep Sleep | $2600\,\text{mAh} / 0.015\,\text{mA} \approx 173,000\,\text{hours} \approx 19\,\text{years}$. |
| **Switch Inputs** | 6 independent channels via 3.5 mm pluggable terminal blocks | Compatible with momentary rocker switches, pushbuttons, and Carling V-Series. |
| **Wakeup Architecture** | Diode-OR Schottky matrix to `LP-GPIO 0` (Header D0) | Wakes in $<2\,\text{ms}$ upon pressing any dash switch. |
| **RF Protocol** | ESP-NOW with RFC 6479 anti-replay sequence tracking | Transmission latency $<12\,\text{ms}$ from button press to central relay toggle. |

### 2.3 Switch Mapping from Cockpit

| Channel | Dashboard Switch Target | Load Controlled at Central | Action |
| :---: | :--- | :--- | :--- |
| **SW1** | **Exterior Auxiliary Lights** | Aux 1 / Awning (BTS5008 PROFET) | Click: Toggle ON / OFF |
| **SW2** | **Victron Orion-XS DC-DC** | Channel 10 (Optocoupler Remote Enable) | Click: Force Enable / Disable Charger |
| **SW3** | **Cabin Interior Lights** | Zone 1 Main Ceiling (200 Hz LEDC) | Click: Toggle ON / OFF |
| **SW4** | **Water Pump (Seaflo)** | Channel 4 (BTS5008 PROFET + Flyback) | Click: Toggle ON / OFF (10-min safety timer) |
| **SW5** | **Inverter (Multiplus II)** | Channel 9 (Optocoupler Remote Switch) | Click: Toggle AC Inverter ON / OFF |
| **SW6** | **Master Driving Mode** | Central Controller Safety Logic | Click: Shuts down living area lights & pumps |

---

## 3. Master Magnetic Entrance Remote (`entrance-remote-pcb`)

### 3.1 Physical & Mechanical Alignment
This PCB matches the mechanical clearances of [`remote_enclosure_body.step`](file:///var/home/daniel/Projects/vannow/hardware/enclosures/models/remote_enclosure_body.step) and [`remote_enclosure_faceplate.step`](file:///var/home/daniel/Projects/vannow/hardware/enclosures/models/remote_enclosure_faceplate.step):

```
PCB MECHANICAL OUTLINE:
+-------------------------------------------------------------+ (80.0 mm)
|   [Mount M2.5]                                 [Mount M2.5] |
|                      (O) EC11 ROTARY ENCODER                |
|                           (Master Dimmer)                   |
|                                                             |
|                            (*) Status LED                   |
|                                                             |
|       [BTN 1: Zone 1]                  [BTN 2: Zone 2]      |
|       (Main Ceiling)                   (Kitchen Galley)     |
|                                                             |
|       [BTN 3: Zone 3]                  [BTN 4: Zone 4]      |
|       (Bed / Reading)                  (Porch / Awning)     |
|                                                             |
|       [BTN 5: Water Pump]              [BTN 6: Master OFF]  |
|       (Seaflo 12V)                     (Night Shutdown)     |
|                                                             |
|   +-----------------------------------------------------+   |
|   |         2x AA BATTERY BAY CLEARANCE CUTOUT          |   |
|   +-----------------------------------------------------+   |
|   [Mount M2.5]                                 [Mount M2.5] |
+-------------------------------------------------------------+ (80.0 mm)
```

- **PCB Dimensions:** $80.0\,\text{mm} \times 80.0\,\text{mm} \times 1.6\,\text{mm}$ (clears $2.5\,\text{mm}$ enclosure walls).
- **Mounting Standoffs:** 4x M2.5 holes located at $(\pm 30.0\,\text{mm}, \pm 30.0\,\text{mm})$ from center.
- **Magnet Keep-Out Zones:** 4x clearance areas along the edge centers $(X=\pm 28\,\text{mm}, Y=0)$ and $(X=0, Y=\pm 28\,\text{mm})$ to allow enclosure magnet pockets to seat without copper interference.
- **Component Height Budget:**
  - Omron B3F tactile switches: $5.0\,\text{mm}$ total height.
  - Low-profile EC11 rotary encoder: $5.0\,\text{mm}$ bushing, $15.0\,\text{mm}$ shaft.
  - Seeed Studio XIAO ESP32-C6: socketed via female headers on the rear/bottom.

---

## 4. Hardware Schematics & Wake-Up Circuitry

### 4.1 Cockpit Transmitter Schematics (`cockpit.ato`)
The cockpit PCB uses a 6-channel Diode-OR circuit to wake the ESP32-C6 from deep sleep on any pin transition, while routing each switch line to an independent GPIO for identification:

```
COCKPIT DIODE-OR WAKEUP MATRIX:
                       +3.3V
                         |
                      [47k R1]
                         |
      +------------------+-------------------> D0 / GPIO 0 (LP_GPIO EXT1 Wakeup)
      |         |        |        |        |        |
    [D1]      [D2]     [D3]     [D4]     [D5]     [D6]  (BAS70-04 Low-Leakage Schottky)
      |         |        |        |        |        |
      +---------+--------+--------+--------+--------+
      |         |        |        |        |        |
    (SW1)     (SW2)    (SW3)    (SW4)    (SW5)    (SW6) (Terminal Block Inputs from Dash)
      |         |        |        |        |        |
     GND       GND      GND      GND      GND      GND
      |         |        |        |        |        |
      v         v        v        v        v        v
     D1        D2       D3       D4       D5       D6   (Individual Sense Pins on XIAO)
  (GPIO 1)  (GPIO 2) (GPIO 21) (GPIO 22) (GPIO 23) (GPIO 16)
```

### 4.2 Master Remote Schematics (`entrance_remote.ato`)
The entrance remote incorporates both the 6 tactile push buttons (diode-ORed to D0) and the 2 EC11 quadrature signals:
- **D0 (GPIO 0 / LP-GPIO):** Wakeup line from Buttons 1–6 and Encoder Switch.
- **D1 (GPIO 1 / LP-GPIO):** Encoder Channel A (supports EXT1 wakeup on knob rotation).
- **D2 (GPIO 2 / LP-GPIO):** Encoder Channel B (supports EXT1 wakeup on knob rotation).
- **D3 (GPIO 21):** Sense Button 1 (Zone 1 Lights).
- **D4 (GPIO 22):** Sense Button 2 (Zone 2 Lights).
- **D5 (GPIO 23):** Sense Button 3 (Zone 3 Lights).
- **D6 (GPIO 16):** Sense Button 4 (Zone 4 Lights).
- **D7 (GPIO 17):** Sense Button 5 (Water Pump).
- **D8 (GPIO 19):** Sense Button 6 (Master Night Shutdown).
- **D9 (GPIO 20):** Sense Encoder Push Button.
- **D10 (GPIO 9):** Status Micro-LED (Green/Red feedback for ESP-NOW ACK).
