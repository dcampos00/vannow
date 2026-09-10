# VanNOW Engineering Status & Project Tracking Dashboard

This document provides a single source of truth for the implementation status across all engineering domains of the VanNOW project: **Firmware & Protocols**, **Mechanical CAD & Enclosures**, **Hardware-as-Code Schematics (Atopile)**, and **Physical PCB Layout & Routing (KiCad)**.

---

## 1. System Readiness Matrix

| Domain / Board | Schematic & Logic (Atopile) | Board Outline & Placement | Copper Routing & Power Planes | DRC Status | 3D CAD & Enclosure | Firmware & Unit Tests | Production Readiness |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Central Controller (`central-pcb`)** | 100% (Passed) | 80% (Header J7/J8 fix needed) | 0% (Unrouted) | 181 violations | 100% (Base + Lid modelled) | 100% (15/15 tests pass) | **Prototyping / Routing** |
| **Master Entrance Remote (`entrance-remote-pcb`)** | 100% (Passed) | 100% (Exact apertures) | 100% (Procedural route) | 0 violations (Clean) | 100% (Body, Faceplate, Cradle) | 100% (9/9 tests pass) | **100% Complete / Ready** |
| **Cockpit Hub (`cockpit-pcb`)** | 100% (Passed) | 100% (55×45mm, M3 holes) | 100% (Procedural route) | 0 violations (Clean) | STEP + 3D Raytraced Renders | 100% (Protocol mapped) | **100% Board Ready** |

---

## 2. Domain Deep-Dive Status

### 2.1 Firmware & Wireless Protocol
- **Overall Status:** 100% Complete & Verified.
- **Unit Test Suite:** 24/24 unit tests passing:
  - Central tests: 15/15 tests passing (`pio test -d firmware/central -e native`).
  - Remote tests: 9/9 tests passing (`pio test -d firmware/remote -e native`).
- **Target Hardware Builds:** 100% passing:
  - Central unit: Compiles cleanly for `esp32-s3-devkitc-1` with zero warnings.
  - Remote unit: Compiles cleanly for `seeed_xiao_esp32c6` with zero warnings.
- **Key Features Implemented:**
  - Non-blocking state machines: Short-press click, Long-press hold dimmer with 150 ms keep-alive, DoubleClick.
  - 5-Minute Timed Shower Mode for Channel 4 (Water Pump) with two-chirp buzzer acoustic pattern and 3-pulse LED feedback.
  - Non-volatile storage (NVS) persistence via ESP-IDF Preferences API for lighting levels, channel states, and shower timer configurations.
  - ESP-NOW encrypted mesh protocol with monotonic anti-replay sequence validation and MAC allow-listing.
  - Rotary encoder "Active-on-Demand" 1.5s timeout logic with LP-GPIO wakeups.

### 2.2 Mechanical CAD & 3D Enclosures
- **Overall Status:** 80% Complete.
- **Central Controller Box (`hardware/enclosures/`):**
  - Base (`central_enclosure_base.step / .stl`): 175 mm × 125 mm × 45 mm, integrated DIN/wall ears, 4x M3 heat-set bosses, 4x PCB standoffs (140 mm × 85 mm pitch), 4x PG9/11 cable gland ports, convective louvers.
  - Lid (`central_enclosure_lid.step / .stl`): Stepped groove rim, 4x M3 counterbored holes, 6x SPDT toggle switch keyways for external manual emergency bypass.
- **Master Entrance Remote & Wall Cradle (`hardware/enclosures/`):**
  - Rear Body (`remote_enclosure_body.step / .stl`): 86 mm × 86 mm × 18 mm, 4x cardinal N52 magnet recesses (dia 10.3 mm × 2.1 mm), 4x corner M2.5 heat-set bosses, 4x PCB standoffs at (±30 mm, ±30 mm), 2x AA battery cavity (60 mm × 31 mm).
  - Faceplate (`remote_enclosure_faceplate.step / .stl`): 86 mm × 86 mm × 3.8 mm, 6x chamfered 12.2 mm pushbutton apertures, recessed dial bezel (dia 18 mm) for EC11 rotary knob, dia 2 mm micro-LED light dispersion cone.
  - Magnetic Docking Cradle (`remote_magnetic_cradle.step / .stl`): 96 mm × 96 mm × 11 mm, 45° lead-in chamfer, dual ergonomic extraction scallops, wall-mounting screw wells.
- **Cockpit Hub Enclosure:**
  - Pending: Model compact 3D housing in `generate_enclosures.py` to protect the cockpit PCB behind the dashboard.

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
- **Overall Status:** 10% Complete (Unrouted).
- **`central-pcb` (`hardware/central-pcb/layouts/default/default.kicad_pcb`):**
  - Dimensions: 150 mm × 95 mm, 4x M3 mounting holes at (±70 mm, ±42.5 mm).
  - Current Issue: 181 DRC violations. Socket headers J7 and J8 overlap at (55.4 mm, 30.0 mm).
  - Needs: Separation of headers, routing of high-current 12V bus (min 3.0 mm copper / polygon pour), 3.3V logic signals, and unbroken ground plane on B.Cu.
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
| **P6** | Cockpit Hub Enclosure CAD | `hardware/enclosures/scripts/` | ⚪ Pending | Add `build_cockpit_enclosure()` to `generate_enclosures.py` |
| **P7** | Central PCB Header Collision Fix | `hardware/central-pcb/` | ⚪ Pending | Relocate headers J7/J8 in `central.kicad_pcb` to eliminate DRC overlap |
| **P8** | Central PCB Power & Signal Routing | `hardware/central-pcb/` | ⚪ Pending | Route PROFET power stages (12V) and 3.3V logic signals |
| **P9** | Central PCB 3D Render & Verification | `hardware/central-pcb/` | ⚪ Pending | Generate updated 3D renders and export populated STEP for MCAD clash check |
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
