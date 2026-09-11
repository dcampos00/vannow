# Headless DRC

A DRC-clean 11-channel board does **not** validate 24-channel firmware. Run DRC on the PCB that will be ordered.

## Commands

```bash
# Production 24-channel
flatpak run --command=kicad-cli org.kicad.KiCad pcb drc \
  --output hardware/central-pcb/layouts/profet_24ch/drc_report.txt \
  --severity-all \
  hardware/central-pcb/layouts/profet_24ch/profet_24ch.kicad_pcb

# Legacy 11-channel
flatpak run --command=kicad-cli org.kicad.KiCad pcb drc \
  --output hardware/central-pcb/layouts/profet/profet-drc.rpt \
  --severity-all \
  hardware/central-pcb/layouts/profet/profet.kicad_pcb

# Remotes
flatpak run --command=kicad-cli org.kicad.KiCad pcb drc \
  --output hardware/entrance-remote-pcb/layouts/default/drc_report.txt \
  --severity-all \
  hardware/entrance-remote-pcb/layouts/default/default.kicad_pcb
```

Native fallback: `kicad-cli pcb drc --output … --severity-all <pcb>`.

KiCad 8+ may support `--schematic-parity`. Use it when an Atopile netlist is exported; the 24-channel script historically created pads that were not in `*.ato` — that gap is closed for load terminals, keep it closed.

## Parse gate

Accept only reports with `Found 0 DRC violations` and `Found 0 unconnected pads` (or equivalent `Total violations: 0` / `Unconnected items: 0`). Fail the script if either count is non-zero.

Do not ignore courtyard collisions on PROFETs or Phoenix blocks. Ignoring “footprint has no courtyard” is acceptable if the project already excludes that check.
