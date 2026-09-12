# VanNOW — Breadboard Prototyping & Hardware Validation Guide

Authoritative bench procedure for the **24-channel firmware** on `fix/logic-audit-2026-09-11` (and later). It supersedes older pin tables in this file and in [`lista_compras_prototipo.md`](lista_compras_prototipo.md). Do **not** use that shopping list for wiring.

This is a **lab bring-up**, not a fabrication release. Do not order PCBs from this guide.

Two phases:

1. **Phase A — USB 5 V only (no 12 V):** ESP-NOW, entrance faceplate, encoder focus dimming, shower timer, NVS restore.
2. **Phase B — fused 12 V:** buck 5 V, battery ADC, logic-level MOSFET + flyback, optocoupler dry contact.

---

## 0. Identity (read before wiring)

| Item | Value |
| :--- | :--- |
| Central firmware | 24 channels. Zone 1 = **GPIO 12**, pump = **GPIO 4**. |
| Remote image | Entrance only: `seeed_xiao_esp32c6` (`REMOTE_ID=1`). Do **not** flash `cockpit_xiao_esp32c6` on this breadboard. |
| Encoder | Button index **7**. Turn dims the last focused lighting zone. Click sets that zone to **100%**. |
| Shower | **5 min** on the pump only. Double-click (or first hold) on **button 4**. Cancel with a single click. Chirp is the **pump output** (GPIO 4), not a buzzer pin. |
| Status LED | XIAO **D10 = GPIO 18**. Never drive GPIO 9 (BOOT). |
| DevKit RGB | GPIO **38** (v1.1) and **48** (v1.0) are channel outputs. Do not hang LEDs on them. |
| ESP-NOW MACs | Both sides program `AA:BB:CC:DD:EE:FF` (central) and `…:11` (entrance). Leave them matching. |
| XIAO `3V3` | Regulator **output**. Do not feed 2×AA into it (audit H-12). Phase A uses USB-C. |

XIAO ESP32-C6 silk ≠ GPIO number:

| Silk | GPIO | Entrance use |
| :---: | :---: | :--- |
| D0 | 0 | Diode-OR wakeup bus |
| D1 | 1 | Encoder A |
| D2 | 2 | Encoder B |
| D3 | 21 | Zone 1 (button 0) |
| D4 | 22 | Zone 2 (button 1) |
| D5 | 23 | Zone 3 (button 2) |
| D6 | 16 | Zone 4 (button 3) |
| D7 | 17 | Water pump (button 4) |
| D8 | 19 | Night off (button 5) |
| D9 | 20 | Encoder SW only |
| D10 | 18 | Status LED |

---

## 1. Prototype Bill of Materials

### 1.1 Phase A (USB only)

| Qty | Item | Purpose |
| :---: | :--- | :--- |
| 1 | Lonely Binary ESP32-S3 N16R8 (or DevKitC-1) | Central |
| 1 | Seeed XIAO ESP32-C6 | Entrance remote |
| 2 | USB-C cables | Flash + 5 V |
| 1 | Alps EC11 with push switch | Focus dimmer |
| 6 | Tactile buttons (or 3: Zone 1, pump, night) | Faceplate |
| 6 | BAT54 / BAS70 / 1N4148 | Diode-OR to D0 (one per button + encoder SW) |
| 1 | 47 kΩ | Wake bus pull-up to 3.3 V |
| 3 | 220 Ω or 330 Ω | LED current limit |
| 2 | 5 mm LEDs (green, red) | Zone 1 and pump on the central |
| 1 | 5 mm LED (green) | Remote status on D10 |
| 2 | Breadboards + jumper kit | Separate central / remote |

Encoder A/B wake on LP-GPIO 1/2; they do not need diodes. Buttons and encoder SW do.

### 1.2 Phase B (add when Phase A passes)

| Qty | Item | Purpose |
| :---: | :--- | :--- |
| 1 | 12 V source (1–2 A wall brick, bench PSU, or fused van battery) | Bus |
| 1 | MP1584EN (reputable) or Pololu D24V10F5 | 12 V → 5.00 V |
| 1 | Mini blade fuse **5 A** at the +12 V source | Protection |
| 1 | LR7843 / D4184 logic-level MOSFET module | Bench load switch |
| 1 | PC817 2-ch module | Inverter dry-contact sim |
| 1 | 1N5408 or 1N5822 | Pump flyback |
| 1 | 100 kΩ 1% + 18 kΩ 1% + 1 kΩ + 100 nF | ADC front-end (GPIO 1) |
| 1 | 12 V LED strip scrap or small motor | Load |

Phase B with an N-channel module is **low-side** (load to drain, source to GND). That is acceptable for an isolated bench strip. Van production switching is **high-side PROFET**. Do not extend a low-side breadboard into chassis-return van wiring.

---

## 2. Phase A wiring

Power both boards from USB-C. Common GND between boards is not required (ESP-NOW is wireless).

### 2.1 Central (ESP32-S3)

```
USB-C 5 V → ESP32-S3

GPIO 12 ──[ 220 Ω ]──► green LED anode ──► GND     Zone 1 (Ch 0)
GPIO 4  ──[ 220 Ω ]──► red LED anode   ──► GND     Pump (Ch 4); shower chirp is this LED
GND ── common LED cathode rail
```

Do not wire a third LED to GPIO 38 or 48.

### 2.2 Entrance remote (XIAO ESP32-C6)

Diode-OR: **anode to the D0 bus**, **cathode to the sense pin**. Pressing a button pulls the sense pin and the bus to GND.

```
USB-C 5 V → XIAO          (do not connect 2×AA to 3V3)

3V3 ──[ 47 kΩ ]──┬──────── D0 (GPIO 0) wakeup bus
                 │
    Zone1 ──|<|──┤   cathode to D3, other button pin to GND
    Zone2 ──|<|──┤   cathode to D4
    Zone3 ──|<|──┤   cathode to D5
    Zone4 ──|<|──┤   cathode to D6
    Pump  ──|<|──┤   cathode to D7
    Night ──|<|──┤   cathode to D8
    EncSW ──|<|──┘   cathode to D9

EC11 A (CLK) → D1 (GPIO 1)
EC11 B (DT)  → D2 (GPIO 2)
EC11 COM     → GND

D10 (GPIO 18) ──[ 220 Ω ]──► status LED anode ──► GND
```

Minimum set if you are short on buttons: Zone 1, pump, night, encoder (plus diodes on those three buttons and EncSW).

### 2.3 Pin table

| From | Pin | To | Pin | Notes |
| :--- | :--- | :--- | :--- | :--- |
| USB | VBUS | ESP32-S3 | USB-C | Central 5 V |
| ESP32-S3 | GPIO 12 | 220 Ω → green LED | anode | Zone 1 |
| ESP32-S3 | GPIO 4 | 220 Ω → red LED | anode | Pump + shower chirp |
| USB | VBUS | XIAO | USB-C | Remote 5 V. Not 3V3. |
| XIAO 3V3 | 3V3 | 47 kΩ | one end | Pull-up only |
| 47 kΩ | other end | XIAO | D0 | Wake bus |
| Each button / EncSW | one side | GND | | Active low |
| Each button / EncSW | other side | diode cathode + sense silk | D3–D9 as above | |
| Diode anodes | | D0 bus | | |
| EC11 | A / B / COM | D1 / D2 / GND | | No diodes on A/B |
| XIAO | D10 | 220 Ω → LED | anode | Status flash |

---

## 3. Phase A protocol

### A1 — Build and flash

From the repo root, on this branch:

```bash
pio test -d firmware/central -e native
pio test -d firmware/remote -e native

pio run -d firmware/central -e esp32-s3-devkitc-1 -t upload
pio run -d firmware/remote -e seeed_xiao_esp32c6 -t upload
```

Serial (115200):

```bash
pio device monitor -d firmware/central -e esp32-s3-devkitc-1
pio device monitor -d firmware/remote -e seeed_xiao_esp32c6
```

Central should print `Starting VanNOW Central Controller...` then `System initialized. Awaiting wireless ESP-NOW commands.` and `Central MAC Address: AA:BB:CC:DD:EE:FF`.

The remote flashes D10 once on a cold boot, then sleeps. After a button wake it prints `Remote active. Listening for physical input transitions.`

If the remote never wakes, the diode-OR or D0 pull-up is wrong. If the central prints `Rejected packet: sender MAC address not in allow-list`, the remote image is not `REMOTE_ID=1` (you flashed cockpit).

### A2 — Link and Zone 1

1. Tap Zone 1 (D3).
2. Remote: `Sending payload: Remote 1 | Btn 0 | Act 0 | Steps 0 | Bat …`
3. Central:

```
--- Message from [AA:BB:CC:DD:EE:11] ---
Remote ID: 1 | Button: 0 | Action: 0 | Seq: …
Channel [Lights Zone 1] (Index 0) state updated.
```

4. Green LED on GPIO 12 turns on (default restore brightness is 80% if NVS is empty).
5. Tap Zone 1 again: LED off, `Seq` increments.

Action numbers: `0` Click, `1` StartHold, `3` EncoderTurn, `4` DoubleClick.

### A3 — Encoder focus dimming

1. Turn Zone 1 on.
2. Rotate the EC11: green LED should ramp. Central `Button: 7` and `Action: 3`.
3. Click the encoder shaft: LED goes to 100%. Central: `Encoder boost: channel [Lights Zone 1] set to 100%`.
4. Tap Zone 2 (if wired), then rotate: dimming follows Zone 2, not Zone 1.
5. Hold Zone 1 > 500 ms: keep-alive `StartHold` (`Action: 1`) every ~150 ms; brightness ramps. Release to lock.

The encoder SW must be **only** on D9. Do not also jumper it to a zone pin.

### A4 — Pump, shower, night

1. Tap pump (D7, button 4): red LED on GPIO 4 on; tap again: off.
2. Double-tap pump within ~320 ms: `[Shower Mode] Activated: 300000 ms with acoustic chirp`. The **red** LED does two short pulses, then stays on. Remote D10 flashes three times.
3. Further holds during shower print `[Shower Mode] Keep-alive ignored; use Click to cancel.`
4. Single tap pump: shower cancels, red LED off.
5. Turn Zone 1 on, tap night (D8, button 5): `Action: Turn off all dimmable lights`. Green off; pump unchanged.

Pump safety auto-off is **10 min** (`restoreOnBoot=false`). Shower timer is **5 min**.

### A5 — NVS

1. Set Zone 1 to a visible mid brightness (encoder or hold). Leave pump off.
2. Unplug central USB. Plug back in.
3. Expect `[NVS] Restored dimmable channel 0 (Lights Zone 1) ON at N%` and the green LED at that level. Pump stays off.

Brightness in logs is **0–100**, not 0–255.

### A6 — Sleep current (optional, after A2–A5)

Do **not** inject 2×AA into `3V3`.

1. Disconnect XIAO USB.
2. Power the XIAO **5V** pin from 4.5–5.0 V (3×AA alkaline or a small USB pack) with the DMM in series on the positive lead. GND to XIAO GND.
3. After 1.5 s idle the remote prints `Entering Deep Sleep mode now.`
4. Expect **tens to hundreds of µA** on a breadboard (XIAO LDO + leakage). The ~15 µA chip figure will not show up here.
5. A press should spike to tens of mA, then collapse again.

Skip A6 if you only have 2×AA and no 4.5–5 V source.

---

## 4. Phase B — 12 V

Start only after A2–A5 pass. Fuse the +12 V lead at the source.

### 4.1 Buck first (no ESP32 connected)

1. 12 V → MP1584 `IN+` / `IN-`.
2. Adjust the trimmer to **5.00 V ± 0.05 V** on `OUT+`.
3. Remove 12 V. Then `OUT+` → ESP32-S3 `5V`, `OUT-` → `GND`.
4. Re-apply 12 V. Central must boot as in A1. USB-C can stay disconnected.

### 4.2 Battery ADC (GPIO 1)

```
+12 V ──[ 100 kΩ 1% ]──┬──[ 1 kΩ ]── GPIO 1
                       ├──[ 18 kΩ 1% ]── GND
                       └──[ 100 nF ]──── GND
```

At 12.50 V the GPIO node is `12.50 × 18 / 118 ≈ 1.91 V`. Heartbeat line: `[Heartbeat] Central Active | Cabin Battery: … V`.

### 4.3 Zone 1 MOSFET (bench low-side only)

GPIO 12 → MOSFET module IN (or 1 kΩ to gate). Isolated 12 V LED strip between +12 V and drain; source to GND. 200 Hz dimming. After 10 min at 100% the logic-level FET should stay cool. An IRF520 will get hot at 3.3 V gate — do not use one.

### 4.4 Pump flyback

Inductive source on Ch 4 (GPIO 4). **1N5408 across the load**: cathode to +12 V, anode to the switched node. Toggle twenty times. Scope (optional): spike clamped near the rail.

### 4.5 Optocoupler (inverter sim)

GPIO **21** (Ch 14) → PC817 LED input. Continuity on the output: closed when the channel is on, open when off. No continuity from ESP32 GND to the output pins.

Cockpit SW5 is what drives this channel in the van. On this entrance breadboard, GPIO 21 only changes if you send a cockpit packet or drive the pin from a temporary mapping. For a first bench check you may jumper a momentary 3.3 V into the PC817 input through its module resistor, or add a one-line test mapping later. Do not use GPIO 15 (that is Zone 4 PWM).

---

## 5. Sign-off (before any PCB order)

| Test | Pass | A | B |
| :--- | :--- | :---: | :---: |
| ESP-NOW entrance → central | Button 0 toggles GPIO 12; seq increases | [ ] | [ ] |
| Diode-OR wake | Sleeps, wakes on Zone 1 / pump / EncSW | [ ] | — |
| Encoder | Button 7 dims focus zone; click = 100% | [ ] | [ ] |
| Shower | Double-click pump; GPIO 4 chirps; click cancels | [ ] | [ ] |
| Night | Button 5 clears dimmers; pump stays | [ ] | [ ] |
| NVS | Lights restore; pump stays off | [ ] | [ ] |
| No GPIO 9 / 38 / 48 LEDs | Status on D10; no RGB pins used | [ ] | [ ] |
| Sleep current | Optional; not via 3V3 | [ ] | — |
| Buck 5.00 V ± 0.05 V | Before connecting the S3 | — | [ ] |
| ADC ± 1.5% vs DMM | GPIO 1 divider | — | [ ] |
| FET cool at full PWM | LR7843 / D4184, not IRF520 | — | [ ] |
| Flyback | 1N5408 on pump load | — | [ ] |
| PC817 isolation | GPIO 21 path; output isolated | — | [ ] |

Still open after a green sheet (do not treat as van-ready): Phoenix MPT 0.5 vs pump inrush (H-11), DevKit RGB vs Ch 15/21 (H-09), hardcoded ESP-NOW keys (FW-16), 11-ch vs 24-ch PCB identity.
