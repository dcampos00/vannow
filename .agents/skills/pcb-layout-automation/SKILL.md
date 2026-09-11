---
name: pcb-layout-automation
description: >-
  Places, routes, and DRC-gates VanNOW carrier boards with KiCad pcbnew Python
  (IPC-2152 traces, PROFET thermal vias, headless DRC). Use when editing
  layout_and_route.py, layout_and_route_24ch.py, .kicad_pcb files, power copper,
  terminal footprints, or enclosure hole alignment.
---

# Procedural PCB Layout (VanNOW)

Code-first layout for the three carriers. **Production central board is 24-channel** (`profet_24ch`, 180×110 mm). The 11-channel `layouts/profet/` board (150×95 mm) is legacy — do not treat a DRC-clean 11-channel PCB as the 24-channel firmware target.

---

## Board roster

| Board | Script | PCB | Outline | Standoffs |
| :--- | :--- | :--- | :--- | :--- |
| Central 24-ch | `hardware/central-pcb/scripts/layout_and_route_24ch.py` | `layouts/profet_24ch/profet_24ch.kicad_pcb` | 180×110 mm | 170×100 mm pitch → `central_24ch_*` enclosure |
| Central 11-ch (legacy) | `hardware/central-pcb/scripts/layout_and_route.py` | `layouts/profet/profet.kicad_pcb` | 150×95 mm | 140×85 mm |
| Entrance remote | `hardware/entrance-remote-pcb/scripts/` | `layouts/default/default.kicad_pcb` | 80×80 mm R4 | ±30 mm M2.5 |
| Cockpit | `hardware/cockpit-pcb/scripts/` | `layouts/default/default.kicad_pcb` | 55×45 mm | 3.5 mm inset M3 |

After moving terminals or the outline, update `hardware/enclosures/scripts/generate_enclosures.py` in the same change.

---

## Hard rules

1. **Procedural placement** for repeating PROFET/opto columns. No hand-dragging of channel arrays.
2. **IPC-2152 widths** — [automotive_pcb_routing_rules.md](./references/automotive_pcb_routing_rules.md). Pump/aux ≥ 3.0 mm or a copper pour; do not run 7.5 A inrush on a 0.5 mm signal trace.
3. **Phoenix MPT 0,5 is 6 A.** The Seaflo pump is specified at 7.5 A inrush. Flag this until the footprint is upgraded (≥ 15 A, e.g. MKDS-1,5). Do not silently keep MPT 0,5 on Ch 4 / lightbar / sockets.
4. **BTS5008 EP** gets a 3×2 or 4×2 thermal via array (0.3 mm drill / 0.6 mm pad) to the 12 V pour. DEN may be tied to GND in the current design.
5. **F.Cu = 12 V / load, B.Cu = GND flood.** 45° bends on power. ADC divider stays off PWM edges; BAT54S clamp sits within 3 mm of GPIO 1.
6. **Headless DRC gate:** 0 violations, 0 unconnected. See [headless_drc_and_validation.md](./references/headless_drc_and_validation.md).
7. **Atopile nets must exist** before the script invents connectors. `VanCentralController24Ch` now has 2-pin load terminals — keep script pad nets aligned with those names.

API snippets: [pcbnew_api_cheat_sheet.md](./references/pcbnew_api_cheat_sheet.md).

---

## Workflow

```bash
# 24-channel production layout
flatpak run --command=python3 org.kicad.KiCad \
  hardware/central-pcb/scripts/layout_and_route_24ch.py

flatpak run --command=kicad-cli org.kicad.KiCad pcb drc \
  --output hardware/central-pcb/layouts/profet_24ch/drc_report.txt \
  --severity-all \
  hardware/central-pcb/layouts/profet_24ch/profet_24ch.kicad_pcb
```

If Flatpak is missing, use native `python3` / `kicad-cli` with the same arguments.

Units are nanometers (`mm * 1e6`). Fill zones before save: `ZONE_FILLER(board).Fill(board.Zones())`.

---

## Pre-commit checklist

- [ ] Script matches the board you intend to fabricate (24-ch vs legacy 11-ch)
- [ ] DRC: 0 unconnected, 0 clearance/courtyard errors
- [ ] Pump/high-current footprints ≥ 15 A **or** residual H-11 documented
- [ ] Thermal vias under every BTS5008 EP
- [ ] Terminal throats face outward, ≥ 3 mm from Edge.Cuts
- [ ] Outline and hole pitch match the enclosure generator
- [ ] GND zones refilled; no orphaned copper islands
