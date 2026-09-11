# VanNOW Expanded 24-Channel Central Controller Architecture

## 1. Overview & Architectural Motivation

The VanNOW system centralizes all camper van switching, dimming, and energy control inside the electrical cabinet. As van builds scale to incorporate comprehensive power systems (e.g., Victron MultiPlus inverter/chargers, dual Orion-XS DC-DC chargers, MPPT solar regulators), multi-zone dimmable lighting, smart ventilation (Maxxair Fan), plumbing automation, and climate control (diesel heater, 12V compressor fridge), a standard 8-to-11 channel controller quickly becomes saturated.

This architecture expands the Central Controller to **24 independent channels** while establishing a fully modular, parameterized firmware design that allows arbitrary channel configurations (from 1 to 32 channels) to be provisioned through parameters with zero firmware re-compilation.

---

## 2. Load Allocation & Channel Breakdown

The 24 channels are divided into two distinct electrical topologies:
1. **14 High-Side Power Channels (12V PROFET BTS5008-1EKB):** High-current automotive solid-state switching with integrated over-current, thermal shutdown, and short-circuit protection.
2. **10 Isolated Signal Channels (PC817 Optocouplers / Dry Contacts):** Galvanically isolated semiconductor contacts designed for control logic lines, remote terminals, thermostat signals, and momentary keypad simulation.
3. **1 Analog Telemetry Channel (GPIO 1):** Main 12V cabin battery voltage sensing.

### Master Channel Assignment Matrix

| Channel | Category | Hardware Type | Pin (ESP32-S3) | Designated Load / Device | Operational Mode / Safety Policy |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Ch 0** | Power | PROFET BTS5008 | GPIO 12 | Interior Lights Zone 1 | Dimmable (LEDC PWM 200 Hz), Restore ON |
| **Ch 1** | Power | PROFET BTS5008 | GPIO 13 | Interior Lights Zone 2 | Dimmable (LEDC PWM 200 Hz), Restore ON |
| **Ch 2** | Power | PROFET BTS5008 | GPIO 14 | Interior Lights Zone 3 | Dimmable (LEDC PWM 200 Hz), Restore ON |
| **Ch 3** | Power | PROFET BTS5008 | GPIO 15 | Interior Lights Zone 4 | Dimmable (LEDC PWM 200 Hz), Restore ON |
| **Ch 4** | Power | PROFET BTS5008 | GPIO 4 | Fresh Water Pump | Digital Switch, 10 min Auto-Off, Shower Mode, **Restore = False** |
| **Ch 5** | Power | PROFET BTS5008 | GPIO 5 | Exterior Driver Light | Digital Switch, Restore ON |
| **Ch 6** | Power | PROFET BTS5008 | GPIO 6 | Exterior Passenger Light | Digital Switch, Restore ON |
| **Ch 7** | Power | PROFET BTS5008 | GPIO 7 | Aux Lightbar / Roof Light | Digital Switch, Restore ON |
| **Ch 8** | Power | PROFET BTS5008 | GPIO 17 | Maxxair Fan 12V Main Power | Digital Switch (Standby Cut), 1N5408 Diode, Restore ON |
| **Ch 9** | Power | PROFET BTS5008 | GPIO 8 | Aux Power 1 (Water Boiler) | Digital Switch, Restore ON |
| **Ch 10** | Power | PROFET BTS5008 | GPIO 9 | Aux Power 2 (Grey Dump Valve) | Digital Switch, Motorized Ball Valve, **Restore = False** |
| **Ch 11** | Power | PROFET BTS5008 | GPIO 10 | Aux Power 3 (Tank Heating Pad) | Digital Switch, Low-temp Freeze Protection, **Restore = False** |
| **Ch 12** | Power | PROFET BTS5008 | GPIO 11 | Aux Power 4 (Aux 12V Sockets) | Digital Switch, USB-C PD / 12V Sockets, Restore ON |
| **Ch 13** | Power | PROFET BTS5008 | GPIO 18 | Aux Power 5 (Motorized Awning) | Digital Switch, Auxiliary / Awning, Restore ON |
| **Ch 14** | Signal | Optocoupler PC817 | GPIO 21 | Victron MultiPlus II Remote | Isolated Dry Contact (Remote Switch H/L), Restore ON |
| **Ch 15** | Signal | Optocoupler PC817 | GPIO 38 | Victron Orion-XS #1 Remote | Isolated Dry Contact (Remote Terminal), Restore ON |
| **Ch 16** | Signal | Optocoupler PC817 | GPIO 39 | Victron Orion-XS #2 Remote | Isolated Dry Contact (Remote Terminal), Restore ON |
| **Ch 17** | Signal | Optocoupler PC817 | GPIO 40 | Victron SmartSolar MPPT | Isolated Dry Contact (Remote Terminal), Restore ON |
| **Ch 18** | Signal | Optocoupler PC817 | GPIO 41 | Diesel Heater Thermostat | Isolated Dry Contact (Thermostat Signal Wire), Restore ON |
| **Ch 19** | Signal | Optocoupler PC817 | GPIO 42 | 12V Compressor Fridge | Isolated Dry Contact (Secop/Danfoss T and C), Restore ON |
| **Ch 20** | Signal | Optocoupler PC817 | GPIO 47 | Maxxair Keypad Power Pulse | Momentary Pulse (250 ms), Non-latching, **Restore = False** |
| **Ch 21** | Signal | Optocoupler PC817 | GPIO 48 | Aux Signal 1 (Alarm Chirp) | Digital Switch / Siren Trigger, **Restore = False** |
| **Ch 22** | Signal | Optocoupler PC817 | GPIO 2 | Aux Signal 2 (LPG Gas Solenoid) | Digital Switch, Solenoid Valve, **Restore = False** |
| **Ch 23** | Signal | Optocoupler PC817 | GPIO 16 | Aux Signal 3 (Generator Start) | Momentary Pulse (500 ms), Cranking Pulse, **Restore = False** |
| **ADC** | Telemetry | Analog Resistor Div | GPIO 1 | Main 12V Cabin Battery | 100k / 18k Divider (Ratio 6.555), 1k ESD Protection |

---

## 3. Specific Device Integration Specifications

### 3.1 Maxxair Fan Dual-Channel Architecture
The Maxxair Deluxe roof fan requires a specialized two-tier control strategy:
- **Quiescent Drain Problem:** When off, the internal logic board continuously draws ~15 mA (0.18 W) in standby.
- **Power Restoration Behavior:** Abruptly restoring 12V does not spin up the fan; the internal microcontroller boots up in Standby mode with the dome closed.
- **Solution:**
  1. **Channel 8 (GPIO 17, PROFET):** Hard 12V power cut. Completely eliminates standby drain during storage or parked mode.
  2. **Channel 20 (GPIO 47, PC817 Optocoupler):** Momentary pulse (250 ms). Optocoupler collector and emitter are wired in parallel across the keypad membrane `ON/OFF` button contacts. Emitting a 250 ms pulse simulates a physical keypress, toggling the fan on/off without modifying manufacturer firmware.

### 3.2 Victron Power System Control
- **MultiPlus II Inverter/Charger (Channel 14):** The MultiPlus remote connector features two terminals (`H` and `L`). Connecting them enables the inverter. PC817 collector connects to `H` and emitter to `L`.
- **Orion-XS DC-DC Chargers #1 and #2 (Channels 15 & 16):** Independent remote switching enables flexible alternator charging control (e.g., run 1 charger at 50A for city driving; run both at 100A for highway cruising).
- **SmartSolar MPPT (Channel 17):** Allows remote solar isolation when servicing electrical systems or when the lithium battery is at low temperatures.

### 3.3 Diesel Heater (Channel 18)
- **CRITICAL SAFETY DIRECTIVE:** The main 12V power supply to the diesel heater must NEVER be cut with a relay while operating, as this bypasses the essential cooldown purge cycle and melts the heat exchanger.
- **Implementation:** 12V power remains permanently connected to the fused battery bus. Channel 18 (PC817) switches ONLY the thermostat signal wire (or external on/off remote input on Autoterm/Webasto/Eberspächer units).

### 3.4 12V Compressor Refrigerator (Channel 19)
- Operates Secop / Danfoss BD35F/BD50F controllers via terminals `T` (thermostat) and `C` (common). Closing the contact runs the compressor; opening it shuts down the compressor safely while keeping internal display/thermostat logic active.

### 3.5 Water Pump Safety Automation (Channel 4)
- **10-Minute Safety Auto-Off:** Prevents catastrophic van flooding in case of burst PEX pipe or faucet left on.
- **5-Minute Shower Mode:** Activated via remote `DoubleClick` or `StartHold`. Emits an acoustic double-pulse confirmation chirp (150 ms on, 120 ms pause, 150 ms on, 120 ms pause, continuous run) and automatically terminates after the configured duration (restorable via NVS).
- **Flood Prevention Boot Policy:** `restoreOnBoot = false`. Following a brownout or MCU restart, the pump remains OFF.

---

## 4. ESP32-S3 DevKitC-1 N16R8 Hardware Pinout Matrix

To ensure 100% hardware reliability, all 24 GPIO assignments strictly avoid:
- **Boot Strapping Pins:** GPIO 0, 3, 45, 46.
- **Octal SPI Flash & PSRAM Pins:** GPIO 33, 34, 35, 36, 37 (connected internally on N16R8 modules).
- **Native USB PHY Pins:** GPIO 19, 20 (reserved for native USB debugging and flashing).
- **UART0 Pins:** GPIO 43 (TX), 44 (RX) (reserved for serial telemetry).

### DevKit Header Pin Distribution

```
                 ESP32-S3 DevKitC-1 v1.1
                 +---------------------+
           3V3 --| 1  (J1)     (J3)  1 |-- GND
           3V3 --| 2                 2 |-- TX (GPIO 43) [DEBUG]
            EN --| 3                 3 |-- RX (GPIO 44) [DEBUG]
 [CH 4] GPIO 4 --| 4                 4 |-- GPIO 1  [BATTERY ADC]
 [CH 5] GPIO 5 --| 5                 5 |-- GPIO 2  [CH 22: LPG Solenoid]
 [CH 6] GPIO 6 --| 6                 6 |-- GPIO 42 [CH 19: 12V Fridge]
 [CH 7] GPIO 7 --| 7                 7 |-- GPIO 41 [CH 18: Diesel Heater]
 [CH 3] GPIO 15 -| 8                 8 |-- GPIO 40 [CH 17: MPPT Remote]
[CH 23] GPIO 16 -| 9                 9 |-- GPIO 39 [CH 16: Orion #2]
 [CH 8] GPIO 17 -| 10               10 |-- GPIO 38 [CH 15: Orion #1]
[CH 13] GPIO 18 -| 11               11 |-- GPIO 37 [OCTAL FLASH - NC]
 [CH 9] GPIO 8 --| 12               12 |-- GPIO 36 [OCTAL FLASH - NC]
  [USB] GPIO 19 -| 13               13 |-- GPIO 35 [OCTAL PSRAM - NC]
  [USB] GPIO 20 -| 14               14 |-- GPIO 0  [BOOT - NC]
 [STRAP] GPIO 3 -| 15               15 |-- GPIO 45 [STRAP - NC]
[STRAP] GPIO 46 -| 16               16 |-- GPIO 48 [CH 21: Aux Signal 1]
[CH 10] GPIO 9 --| 17               17 |-- GPIO 47 [CH 20: Maxxair Pulse]
[CH 11] GPIO 10 -| 18               18 |-- GPIO 21 [CH 14: Inverter Remote]
[CH 12] GPIO 11 -| 19               19 |-- GPIO 14 [CH 2: Light Zone 3]
 [CH 0] GPIO 12 -| 20               20 |-- GPIO 13 [CH 1: Light Zone 2]
           5V  --| 21               21 |-- GND
           GND --| 22               22 |-- GND
                 +---------------------+
```

---

## 5. Modular Firmware Architecture

The firmware decouples physical hardware configuration from switching logic:

### 5.1 Polymorphic Channel Classes
- `Channel` (Abstract Base): Defines `handleAction(ActionType, int8_t)`, `setState(bool)`, `update()`, persistence flags, and state-change callbacks.
- `DimmableChannel`: Implements 200 Hz LEDC PWM, 1% per 5 ms smooth exponential fading, 30 ms hold ramping, and rotary encoder velocity scaling.
- `DigitalChannel`: Implements latching on/off switching, configurable auto-off safety timers, and timed shower mode with acoustic chirping.
- `PulseChannel`: Implements non-latching momentary pulse generation with configurable active duration (e.g., 250 ms for Maxxair, 500 ms for generator start).

### 5.2 Parameterized SystemController
```cpp
enum class ChannelType : uint8_t { Dimmable, Digital, MomentaryPulse };

struct ChannelConfig {
    const char* name;
    uint8_t pin;
    ChannelType type;
    uint32_t autoOffTimeoutMs;
    bool restoreOnBoot;
    uint8_t defaultBrightness;
    bool activeLow;
};

// Parameterized initialization supports custom configurations:
SystemController controller(customConfigs, channelCount, customMappings, mappingCount);
```

### 5.3 Decoupled Remote Routing Table
Remote panels emit packets containing `remote_id`, `button_index`, `action`, and `seq`. The routing table maps `(remote_id, button_index)` to target channels or global macro actions:
- `Remote 1 (Entry Panel)`: Zone 1, Zone 2, Water Pump, Inverter, Zone 3, Zone 4, Maxxair Power.
- `Remote 2 (Bed Panel)`: Zone 3, Zone 4, Maxxair Power, **TurnOffAllLights (Macro)**, Zone 1, Zone 2, Water Pump.
- `Remote 3 (Cockpit Panel)`: Exterior Driver, Orion-XS #1, Zone 1, Water Pump, Inverter, Maxxair Power.

---

## 6. Physical Packaging & Parametric 3D Enclosure

### 6.1 Carrier Board Packaging
- **PCB Dimensions:** 180.0 mm × 110.0 mm (FR4 double-sided, 2 oz copper).
- **Connector Strategy:** High-density two-tier (double-decker) 5.08 mm pitch screw terminal blocks. Placing terminals in two vertical tiers accommodates 48 connection poles within a 140 mm run along the board edges.
- **Mounting Holes:** 4x M3 at (±85.0 mm, ±50.0 mm) (170.0 mm × 100.0 mm pitch).

### 6.2 Parametric Enclosure Dimensions (`build123d`)
The 3D model is generated parametrically by `hardware/enclosures/scripts/generate_enclosures.py`:
- **Outer Shell Dimensions:** 205.0 mm (Length) × 135.0 mm (Width) × 52.0 mm (Height).
- **Y-Axis Mounting Ears:** Dual mounting ears placed along the 135 mm width axis with 4.5 mm M4 holes:
  $$\text{Total Width with Ears} = 135.0\,\text{mm} + 2 \times 22.0\,\text{mm} = 179.0\,\text{mm}$$
- **Cabinet Envelope Verification:**
  - Length: 205.0 mm $\le$ 230.0 mm cabinet limit (**25 mm clearance**).
  - Width: 179.0 mm $\le$ 190.0 mm cabinet limit (**11 mm clearance**).
  - Height: 54.2 mm (base + lid) $\le$ 85.0 mm cabinet limit (**30.8 mm clearance**).
  - Wiring Drop: Front cable gland ports preserve $\ge 50.0\,\text{mm}$ drop clearance.
- **Mechanical Features:**
  - Stepped interlocking lip and groove (+2.2 mm / -2.4 mm) for dust and splash sealing.
  - 4x M3 brass heat-set insert corner bosses (dia 4.0 mm × depth 6.0 mm).
  - 4x M3 PCB standoffs with triangular reinforcing gusset ribs.
  - 6x PG9/PG11 cable gland collars (dia 16.0 mm with 22.0 mm outer boss).
  - Convective ventilation slots for PROFET thermal dissipation.
  - 6x SPDT toggle switch mounting stations on the lid with anti-rotation keyways.
