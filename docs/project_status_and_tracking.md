# VanNOW Engineering Status & Project Tracking Dashboard

**Release train:** **Alpha** (not 1.0, not a fabrication or van-install release).

This document tracks implementation status. It is **not** a production-readiness certificate. Authoritative residuals: [`logic_audit_2026-09-11.md`](logic_audit_2026-09-11.md). Bench procedure: [`breadboard_prototyping_and_validation_guide.md`](breadboard_prototyping_and_validation_guide.md).

> [!WARNING]
> **Alpha.** Logic defects from the 2026-09-11 audit are fixed in firmware, but Gerbers/CPL, Phoenix 6 A vs pump inrush (H-11), DevKit RGB on GPIO 38/48 (H-09), 2×AA into XIAO `3V3` (H-12), and hardcoded ESP-NOW keys (FW-16) are still open. Do not order boards or install in the van from this tree.

---

## 0. Channel profile (firmware parameter)

The central already takes a `ChannelConfig[]`. The **default constructor** selects the table from the compile flag `VANNOW_CHANNEL_PROFILE`:

| Profile | PlatformIO env | Carrier | Flash command |
| :--- | :--- | :--- | :--- |
| **24** (default, alpha target) | `esp32-s3-devkitc-1` | 180×110 mm `profet_24ch` | `pio run -d firmware/central -e esp32-s3-devkitc-1 -t upload` |
| **11** (legacy) | `esp32-s3-profet-11ch` | 150×95 mm `VanCentralControllerPROFET` | `pio run -d firmware/central -e esp32-s3-profet-11ch -t upload` |

Serial banner: `VanNOW Central Alpha — channel profile 24` or `11`. Never flash the 24-ch image onto the 11-ch carrier (GPIO 18 is awning vs DC-DC opto; GPIO 2/16 are LPG/gen vs status LEDs).

Remotes are unchanged: entrance `seeed_xiao_esp32c6`, cockpit `cockpit_xiao_esp32c6`.

---

## 1. System Readiness Matrix

| Domain / Board | Schematic | Layout | Firmware / tests | Enclosure | Alpha status |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Central 24-ch** (`profet_24ch`) | Yes | DRC clean 180×110 mm | 24-ch default + native tests | `central_24ch_*` | **Alpha — target** |
| **Central 11-ch** (`profet/`) | Yes (legacy) | DRC clean 150×95 mm | Build with `esp32-s3-profet-11ch` | `central_enclosure_*` | **Alpha — legacy only** |
| **Entrance remote** | Yes | DRC clean 80×80 mm | `seeed_xiao_esp32c6` | Body + faceplate + cradle | **Alpha — bench first** |
| **Cockpit hub** | Yes | DRC clean 55×45 mm | `cockpit_xiao_esp32c6` | Base 76×58×20 mm + lid | **Alpha — unpowered in van** |

Native tests (last run on this branch): central **26/26** (`pio test -d firmware/central -e native`), remote **11/11**. Hardware images compile; they are **not** field-proven.

---

## 2. Domain Deep-Dive

### 2.1 Firmware (Alpha)

- Entrance / cockpit FSMs: Click, hold-ramp, DoubleClick, shower on pump only, encoder button 7 (focus + 100% boost), night off, latching cockpit.
- NVS for channel state, shower timeout, anti-replay window, remote `seq`.
- ESP-NOW with sliding-window anti-replay. **MAC/PMK/LMK still hardcoded** (`AA:BB:…`, `PMK_KEY_VANNOW12`).
- Dual channel map via `VANNOW_CHANNEL_PROFILE` (11 or 24).
- Open: H-09 (do not init `RGB_BUILTIN`), FW-16 provisioning, no PROFET IS diagnostics, no bed-panel hardware.

### 2.2 Mechanical CAD

- 11-ch cabinet 175×125 mm; 24-ch 205×135 mm (within 230×190 mm envelope).
- Entrance 86 mm body / faceplate + 96 mm magnetic cradle.
- Cockpit 76×58×20 mm base, 108 mm M4 ears, 48×38 mm standoff pitch, 44 mm J4–J6 slot. PCB STEP has no XIAO; lid clearance is stacked (header + module).

### 2.3 Schematics (Atopile)

- 12 V entry: P-FET RPP, SM8S24A, MP1584 carrier.
- 24-ch: PROFET stages + optos as in the co-design matrix. 11-ch: 6 PROFET + fan FET + 2 optos; flyback on pump output.
- ADC: 100k/18k + BAT54S clamp on the 24-ch module.
- Remotes: Diode-OR to LP-GPIO 0; D10 = GPIO 18.

### 2.4 KiCad layouts

DRC-clean layouts exist. **Gerbers / CPL / stencil packages are not generated (P10).** Phoenix MPT 0.5 is 6 A; pump inrush is ~7.5 A (H-11).

---

## 3. Work items

| Item | Status | Next action |
| :--- | :---: | :--- |
| P1–P9 PCB/CAD artifacts | Done for alpha | Keep in repo; do not treat as fab pack |
| **P10** Gerber / BOM / CPL | Pending | Blocked on H-11 terminal choice + 24-ch identity |
| **Lab A** USB breadboard | Next | Follow the breadboard guide (24-ch + entrance) |
| **H-09** DevKit RGB vs Ch 15/21 | Open | Isolate LED or move those channels |
| **H-11** Phoenix 6 A vs pump | Open | Larger terminals before 24-ch fab |
| **H-12** 2×AA on `3V3` | Open | Do not inject batteries into `3V3` |
| **FW-16** ESP-NOW keys | Open | Provisioning, not git constants |

---

## 4. Pre-order checklist (not started)

Do not place a PCB order until Lab A (and preferably Lab B) pass **and**:

- [ ] One carrier identity chosen (24-ch) and 11-ch clearly marked legacy
- [ ] Gerbers, drills, BOM, CPL generated and reviewed
- [ ] Pump terminals rated for inrush
- [ ] DevKit RGB isolated from GPIO 38/48
- [ ] ESP-NOW keys not the repo placeholders
- [ ] DRC 0/0 on the exact Gerber revision
