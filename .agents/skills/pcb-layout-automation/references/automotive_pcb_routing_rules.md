# Automotive Trace Sizing (IPC-2152)

External layer, 1 oz (35 µm), ΔT ≤ 15 °C:

| Net | Current | Minimum width | VanNOW practice |
| :--- | :--- | :--- | :--- |
| Battery infeed `v_bat_12v` | 15–20 A | 5.0 mm | Top+bottom pour + via stitch |
| PROFET out (pump, lightbar, sockets, fan) | 8–12 A | 2.5 mm | **3.0 mm** or pour. Pump inrush ~7.5 A |
| LED PWM zones | 3–5 A | 1.2 mm | 1.5 mm |
| Optocoupler LED / GPIO | < 20 mA | 0.25 mm | 0.35 mm |
| ADC divider / IS | < 10 mA | 0.25 mm | 0.35 mm, away from PWM |
| Logic | < 5 mA | 0.20 mm | 0.25 mm |

Clearance: 12 V to GND ≥ 0.30 mm (prefer 0.40 mm). 3.3 V ≥ 0.20 mm.

BTS5008 EP is **VS (12 V)**, not GND. Thermal vias stitch the 12 V pour, not the ground plane.

## Connectors

Phoenix **MPT 0,5** = **6 A**. Too small for Ch 4 (pump) and any aux that can exceed 6 A. Prefer MKDS-1,5 / 5.08 mm (≥ 16 A) or equivalent on those nets. SPDT 4-pin overrides exist only on the legacy 11-channel schematic; 24-channel uses 2-pin load+GND.

Flyback 1N5408: cathode on load+/PROFET out, anode on GND. Confirm KiCad `D_DO-201AD` pad 1 is the banded cathode before fab.
