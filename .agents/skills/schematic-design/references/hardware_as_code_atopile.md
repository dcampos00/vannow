# Hardware-as-Code (Atopile / KiCad) Module Definitions & Pinout Integrity

This guide establishes strict architectural and verification rules for defining microcontroller carrier boards and module interfaces in Atopile and KiCad.

---

## 1. The Anatomy of Carrier Board Pinout Failures

Carrier board integration commonly suffers from three fatal errors:

1. **Perspective / Mirroring Inversion:**
   - Datasheets typically draw breakout modules from the **Top View** (looking down on ICs and LEDs).
   - The carrier board mates with the module via female pin headers mounted on the top layer of the carrier PCB.
   - The carrier board female sockets interface with the **bottom pins** of the module.
   - If a designer lays out female sockets using a top-view diagram without accounting for mating orientation, the pinout becomes mirrored or inverted, leading directly to $V_{CC}$-to-GND shorts upon module insertion.
2. **Sequential vs. Dual-Row Numbering Inconsistencies:**
   - Single-row headers use consecutive 1-to-$N$ numbering.
   - Dual-row headers vary between zig-zag numbering (Pin 1 opposite Pin 2) and perimeter numbering (Pin 1 to $N/2$ along one side, $N/2+1$ to $N$ along the other).
3. **Board Revision & Source Drift:**
   - Unofficial third-party pinout graphics frequently confuse revisions (e.g., Espressif ESP32-S3-DevKitC-1 v1.0 vs. v1.1 where Pin 1 of J3 transitioned from 3V3 to GND).
   - In the Seeed Studio XIAO family, XIAO ESP32-C3 maps pin `D3` to GPIO 3, whereas XIAO ESP32-C6 maps pin `D3` to GPIO 21 (since GPIO 3 is consumed internally by the Wi-Fi 6 RF switch).

---

## 2. Robust Atopile Architecture Pattern

To prevent pin mapping bugs, Atopile code must decouple the **physical connector** from the **logical module**, using explicit signal aliases and compile-time assertions.

### 2.1 The 3-Tier Layered Design Pattern

```ato
# ==============================================================================
# TIER 1: Physical Primitive Header Connector (Strict 1-to-N pin binding)
# ==============================================================================
component Header_1x22:
    trait is_atomic_part<manufacturer="Generic", partnumber="PinSocket_1x22_2.54mm">
    signal pin_1 ~ pin 1
    signal pin_2 ~ pin 2
    signal pin_3 ~ pin 3
    signal pin_4 ~ pin 4
    # ...
    signal pin_21 ~ pin 21
    signal pin_22 ~ pin 22

# ==============================================================================
# TIER 2: Semantic Interface Abstractions
# ==============================================================================
interface Power:
    signal vcc
    signal gnd

interface UART:
    signal tx
    signal rx

# ==============================================================================
# TIER 3: Semantic Module Wrapper
# ==============================================================================
module ESP32_S3_DevKitC_v1_1:
    # Physical hardware connectors (Sockets on carrier PCB)
    j1_left = new Header_1x22
    j3_right = new Header_1x22

    # Power Interfaces
    power_5v = new Power
    power_3v3 = new Power

    # J1 (Left Header) Pin Mapping (Espressif Official Schematic v1.1)
    power_3v3.vcc ~ j1_left.pin_1
    power_5v.vcc  ~ j1_left.pin_21
    power_5v.gnd  ~ j1_left.pin_22
    power_3v3.gnd ~ j1_left.pin_22

    # J3 (Right Header) Pin Mapping
    # Pin 1 = GND, Pin 21 = GND, Pin 22 = GND
    j3_right.pin_1  ~ power_5v.gnd
    j3_right.pin_21 ~ power_5v.gnd
    j3_right.pin_22 ~ power_5v.gnd

    # IO Aliases (Matching exact schematic nets)
    signal gpio1  ~ j3_right.pin_4
    signal gpio2  ~ j3_right.pin_5
    signal gpio21 ~ j3_right.pin_18

    # --------------------------------------------------------------------------
    # Compile-Time HaC Assertions
    # --------------------------------------------------------------------------
    assert power_5v.vcc != power_5v.gnd
    assert power_3v3.vcc != power_3v3.gnd
    assert j1_left.pin_21 != j3_right.pin_1
```

---

## 3. Golden Netlist Verification Protocol

Before finalizing any carrier board:

1. **Manufacturer Schematic Source:** Download the official PDF schematic directly from the module vendor (e.g., Espressif, Seeed Studio, Raspberry Pi). Never use community blog pinouts.
2. **Ground Net Verification:** Extract all pins mapped to `GND` from the PDF table. Verify that in Atopile, each of those physical pins is connected to the ground net.
3. **Power Pin Isolation:** Ensure the 5V input rail connects exclusively to the module's 5V input pin (e.g., J1 Pin 21 on ESP32-S3-DevKitC-1) and never to J3 Pin 1 or J1 Pin 1.
4. **KiCad Footprint Check:**
   - Pad 1 must be visually identifiable with a square pad and a silkscreen dot.
   - Verify that 3D step models match the orientation of the physical header socket.
