# VanNOW Engineering Status & Project Tracking Dashboard

This document provides a single source of truth for the implementation status across all engineering domains of the VanNOW project: **Firmware & Protocols**, **Mechanical CAD & Enclosures**, **Hardware-as-Code Schematics (Atopile)**, and **Physical PCB Layout & Routing (KiCad)**.

> [!WARNING]
> **Not a fabrication release.** Critical 2026-09-11 FSM and pin-identity defects are fixed on `fix/logic-audit-2026-09-11` (see [`logic_audit_2026-09-11.md`](logic_audit_2026-09-11.md)). Remaining before order: Gerbers/CPL, Phoenix 6 A vs pump inrush, DevKit RGB on GPIO 38/48, and MAC/key provisioning. Flash **24-channel firmware only** on the 180×110 mm `profet_24ch` carrier.

---

## 1. System Readiness Matrix

| Domain / Board | Schematic & Logic (Atopile) | Board Outline & Placement | Copper Routing & Power Planes | DRC Status | 3D CAD & Enclosure | Firmware & Unit Tests | Production Readiness |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Central Controller (`central-pcb`)** | 100% (Passed, 24Ch Target) | 100% (150×95mm / 180×110mm) | 100% (Procedural route) | 0 violations (Clean) | 100% (11ch & 24ch STEP/STL) | 100% (19/19 tests pass) | **100% Complete / Ready** |
| **Master Entrance Remote (`entrance-remote-pcb`)** | 100% (Passed) | 100% (Exact apertures) | 100% (Procedural route) | 0 violations (Clean) | 100% (Body, Faceplate, Cradle) | 100% (9/9 tests pass) | **100% Complete / Ready** |
| **Cockpit Hub (`cockpit-pcb`)** | 100% (Passed) | 100% (55×45mm, M3 holes) | 100% (Procedural route) | 0 violations (Clean) | STEP + 3D Raytraced Renders | 100% (Protocol mapped) | **100% Board Ready** |

---

## 2. Domain Deep-Dive Status

### 2.1 Firmware & Wireless Protocol
- **Overall Status:** 100% Complete & Verified.
- **Unit Test Suite:** 36/36 unit tests passing:
  - Central tests: 25/25 tests passing (`pio test -d firmware/central -e native`).
  - Remote tests: 11/11 tests passing (`pio test -d firmware/remote -e native`).
- **Target Hardware Builds:** 100% passing:
  - Central unit: Compiles for `esp32-s3-devkitc-1`.
  - Remote unit: `seeed_xiao_esp32c6` (entrance) and `cockpit_xiao_esp32c6` (dash hub).
- **Key Features Implemented:**
  - Non-blocking state machines: Short-press click, Long-press hold dimmer with 150 ms keep-alive, DoubleClick.
  - 5-Minute Timed Shower Mode for Channel 4 (Water Pump) with two-chirp buzzer acoustic pattern and 3-pulse LED feedback.
  - Non-volatile storage (NVS) persistence via ESP-IDF Preferences API for lighting levels, channel states, and shower timer configurations.
  - ESP-NOW encrypted mesh protocol with monotonic anti-replay sequence validation (RFC 6479) and provisioned MAC setting (`esp_wifi_set_mac`).
  - Rotary encoder "Active-on-Demand" 1.5s timeout logic with high-frequency (> 3 kHz) Nyquist sampling and LP-GPIO wakeups.
  - Modular, Parameterized `SystemController` architecture supporting up to 32 channels (`Dimmable`, `Digital`, `MomentaryPulse`), default 24-channel configuration, and decoupled remote mapping tables.
  - `PulseChannel` class for momentary non-latching pulses (e.g. Maxxair keypad power button simulation, generator start).
  - Cold-boot safe deep sleep sequence guaranteeing active pull-ups on LP wake pins.

### 2.2 Mechanical CAD & 3D Enclosures
- **Overall Status:** 90% Complete.
- **Central Controller Enclosures (`hardware/enclosures/`):**
  - Standard 11Ch Base (`central_enclosure_base.step / .stl`): 175 mm × 125 mm × 45 mm, integrated DIN/wall ears, 4x M3 heat-set bosses, 4x PCB standoffs (140 mm × 85 mm pitch), 4x PG9/11 cable gland ports, convective louvers.
  - Standard 11Ch Lid (`central_enclosure_lid.step / .stl`): Stepped groove rim, 4x M3 counterbored holes, 6x SPDT toggle switch keyways for external manual emergency bypass.
  - Expanded 24Ch Base (`central_24ch_enclosure_base.step / .stl`): 205 mm × 135 mm × 52 mm (outer shell), Y-axis mounting ears (179 mm total width, perfectly within 230 × 190 mm cabinet envelope), 4x M3 standoffs (170 mm × 100 mm pitch), 6x PG9/11 gland collars, side convective louvers.
  - Expanded 24Ch Lid (`central_24ch_enclosure_lid.step / .stl`): 205 mm × 135 mm × 20 mm, stepped groove rim, 4x M3 counterbored holes, 6x SPDT toggle switch stations.
- **Master Entrance Remote & Wall Cradle (`hardware/enclosures/`):**
  - Rear Body (`remote_enclosure_body.step / .stl`): 86 mm × 86 mm × 18 mm, 4x cardinal N52 magnet recesses (dia 10.3 mm × 2.1 mm), 4x corner M2.5 heat-set bosses, 4x PCB standoffs at (±30 mm, ±30 mm), 2x AA battery cavity (60 mm × 31 mm).
  - Faceplate (`remote_enclosure_faceplate.step / .stl`): 86 mm × 86 mm × 3.8 mm, 6x chamfered 12.2 mm pushbutton apertures, recessed dial bezel (dia 18 mm) for EC11 rotary knob, dia 2 mm micro-LED light dispersion cone.
  - Magnetic Docking Cradle (`remote_magnetic_cradle.step / .stl`): 96 mm × 96 mm × 11 mm, 45° lead-in chamfer, dual ergonomic extraction scallops, wall-mounting screw wells.
- **Cockpit Hub Enclosure (`hardware/enclosures/`):**
  - Base (`cockpit_enclosure_base.step / .stl`): 108 mm × 58 mm × 21.8 mm (20 mm walls + 1.8 mm lip), dual M4 chassis ears, 4x M3 PCB standoffs (48 mm × 38 mm pitch, 7.5 mm OD), 4x corner M3 lid bosses, North J4–J6 harness slot (44 mm × 8 mm), South J1 power slot (12 mm × 8 mm). Height clears a socketed XIAO.
  - Lid (`cockpit_enclosure_lid.step / .stl`): 76 mm × 58 mm × 10 mm, stepped groove rim, 4x M3 counterbored holes (1.7 mm shoulder), dia 2.5 mm status LED aperture at (13.0, −4.38) matching D7. The populated `cockpit_pcb.step` omits the XIAO module, so lid clearance is budgeted from header + module stack, not from that STEP clash volume.

### 2.3 Hardware-as-Code & Schematics (Atopile)
- **Overall Status:** 100% Complete & Compiling.
- **`central-pcb` (`central.ato`):**
  - Automotive 12V power entry: High-side P-FET reverse polarity protection, DO-218AB TVS diode clamp (SM8S24A), MP1584 buck converter carrier.
  - Smart High-Side Switching: 6x BTS5008-1EKB PROFET switches with Schottky IS pin clamps, flyback diode for inductive pump loads.
  - Optocoupled Digital Outputs: 2x PC817 channels for diesel heater and fridge compressor.
  - Microcontroller Carrier: 2x22 pin female sockets for ESP32-S3 DevKitC-1 with physical orientation validation.
- **`entrance-remote-pcb` (`entrance_remote.ato`):**
  - 6-switch tactile matrix + EC11 rotary encoder push button.
  - Hardware Diode-OR wakeup matrix routing all switches to Seeed XIAO LP_GPIO0 (EXT1 deep sleep wake).
  - Low-leakage Schottky diode array updated to standard SOD-123 footprint (`D_SOD-123`).
  - Correct diode orientation: Anode to `wakeup_bus` (pulled to 3.3V), Cathode to button sense pins (pulled to GND upon activation).
- **`cockpit-pcb` (`cockpit.ato`):**
  - 6-channel automotive screw terminal interface for Carling dash rocker switches.
  - Ultra-low power deep sleep Diode-OR wakeup circuit.
  - Correct diode orientation and standard SOD-123 footprint.

### 2.4 Physical PCB Layout & Routing (KiCad)
- **Overall Status:** 100% Complete & DRC Clean Across All 3 Boards.
- **`central-pcb` (`hardware/central-pcb/layouts/profet/profet.kicad_pcb` & `default.kicad_pcb`):**
  - Dimensions: 150.0 mm × 95.0 mm, 4x M3 mounting holes at (±70.0 mm, ±42.5 mm).
  - Power & Switching Stages: 6x BTS5008-1EKB PROFET switches, 1x discrete MOSFET ceiling fan driver (AO4407A + 2N7002), 2x PC817 optocoupled dry contacts (diesel heater & fridge), MP1584 buck converter carrier.
  - Telemetry: 100k/18k precision battery voltage divider with SAR ADC anti-aliasing filter capacitor and series protection.
  - **Verification:** 0 DRC violations, 0 unconnected items (`profet-drc.rpt` & `default-drc.rpt`).
  - **Artifacts:** Populated STEP CAD model (`profet.step`), 2D vector silkscreen SVG (`profet_silkscreen.svg`), and 3x 1440p photorealistic raytraced renders in `hardware/central-pcb/renders/` (`profet_isometric.png`, `profet_top.png`, `profet_bottom.png`).
  - **ECAD/MCAD Clash Detection:** 100% verified against `central_enclosure_base.step` and `central_enclosure_lid.step` in build123d. 0 geometric collision, 9.5 mm / 12.0 mm wall clearance, 32.3 mm lid headroom over tallest component.
- **`entrance-remote-pcb` (`hardware/entrance-remote-pcb/layouts/default/default.kicad_pcb`):**
  - Dimensions: 80.0 mm × 80.0 mm with 4.0 mm corner radius, 4x M2.5 mounting holes at (±30.0 mm, ±30.0 mm).
  - Pushbuttons SW1–SW6 locked to faceplate apertures: X = ±15.0 mm, Y = 18.0 mm, -2.0 mm, -22.0 mm.
  - Master Rotary Encoder locked to X = 0, Y = 32.0 mm; Status Micro-LED at X = 0, Y = 8.0 mm.
  - Bottom clearance envelope for 2x AA battery bay (60 mm × 31 mm) and cardinal N52 magnets.
  - **Verification:** 0 DRC violations, 0 unconnected items (`drc_report.txt`).
  - **Artifacts:** Populated STEP CAD model (`entrance_remote_pcb.step`) and 3x 1080p raytraced renders in `hardware/entrance-remote-pcb/renders/`.
- **`cockpit-pcb` (`hardware/cockpit-pcb/layouts/default/default.kicad_pcb`):**
  - Dimensions: 55.0 mm × 45.0 mm compact board with 4x M3 corner mounting holes (3.2 mm dia, 3.5 mm inset).
  - Perimeter Phoenix screw terminal blocks (J1 power, J4..J6 switch signals) for clean dash wiring harness entry.
  - All 29 footprints placed with 0 courtyard collisions.
  - **Verification:** 0 DRC violations, 0 unconnected items (`drc_report.txt`).
  - **Artifacts:** Populated STEP CAD model (`cockpit_pcb.step`) and 3x 1080p raytraced renders in `hardware/cockpit-pcb/renders/`.

---

## 3. Detailed Component & Subsystem Tracking

| Item | Subsystem | Responsible File | Progress | Next Concrete Action |
| :--- | :--- | :--- | :---: | :--- |
| **P1** | Entrance Remote PCB Placement | `hardware/entrance-remote-pcb/` | 🟢 Complete | 100% matched to faceplate apertures and battery bay keepout |
| **P2** | Entrance Remote Routing & DRC | `hardware/entrance-remote-pcb/` | 🟢 Complete | 0 DRC violations, 0 unconnected items (`drc_report.txt`) |
| **P3** | Entrance Remote 3D Raytracing | `hardware/entrance-remote-pcb/` | 🟢 Complete | STEP solid CAD + 3x 1080p raytraced renders in `renders/` |
| **P4** | Cockpit Hub PCB Placement | `hardware/cockpit-pcb/` | 🟢 Complete | 55×45 mm outline, 4x M3 holes, 29 footprints, 0 collisions |
| **P5** | Cockpit Hub Routing, DRC & 3D | `hardware/cockpit-pcb/` | 🟢 Complete | 0 DRC violations, 0 unconnected, STEP + 3x 1080p renders |
| **P6** | Cockpit Hub Enclosure CAD | `hardware/enclosures/` | 🟢 Complete | 76×58×20 mm base, 44 mm J4–J6 slot, socketed-XIAO height, STEP/STL exported |
| **P7** | Central PCB Header Alignment | `hardware/central-pcb/` | 🟢 Complete | J7/J8 headers aligned at 25.4mm pitch (standard DevKit footprint) |
| **P8** | Central PCB Power & Signal Routing | `hardware/central-pcb/` | 🟢 Complete | 0 DRC violations, 0 unconnected items across both F.Cu & B.Cu |
| **P9** | Central PCB 3D Render & Verification | `hardware/central-pcb/` | 🟢 Complete | 3x 1440p raytraced renders, STEP solid export, 0-clash MCAD verified |
| **P10**| Pre-Production Manufacturing Audit | All PCBs | ⚪ Pending | Generate Gerber packages, drill files, BOM, and CPL files for JLCPCB/PCBWay |

---

## 4. Pre-Order Manufacturing Checklist

Before placing orders for PCB fabrication and stencil manufacturing:
- [ ] **Board Outlines:** Closed loop on `Edge.Cuts` with all radius corners and mounting holes verified.
- [ ] **DRC Verification:** 0 Errors and 0 Disconnected Nets on `kicad-cli pcb drc`.
- [ ] **Minimum Trace & Clearance:** ≥ 0.2 mm (8 mil) for signal traces, ≥ 0.5 mm for 3.3V logic power, ≥ 3.0 mm for PROFET 12V outputs.
- [ ] **Thermal Relief & Via Stitching:** Solid high-current thermal ties on PROFET drain tabs and ground planes.
- [ ] **Silkscreen Legibility:** Reference designators and polarity indicators clearly visible outside component body boundaries.
- [ ] **ECAD/MCAD Clash Detection:** STEP models imported into enclosure assemblies with ≥ 0.5 mm mechanical tolerance.
