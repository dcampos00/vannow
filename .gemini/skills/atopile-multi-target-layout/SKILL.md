---
name: atopile-multi-target-layout
description: Orchestrates and compiles multiple parallel hardware designs (targets) in an atopile project, manages local KiCad footprints, and automatically arranges components in a clean 2D grid.
---

# Atopile Multi-Target Design & Layout Arrangement

Use this skill when you need to configure multiple parallel build targets in atopile, define custom components and local footprints, compile targeted builds, or automatically arrange the component footprints of a KiCad PCB layout into a structured 2D grid.

---

## 1. Multi-Target Configuration (`ato.yaml`)

To compile multiple parallel designs (e.g., an integrated board vs. a modular carrier board) from the same repository, define separate targets under `builds` in [ato.yaml](file:///home/daniel/Projects/vannow/hardware/central-pcb/ato.yaml).

Configure separate output layout paths for each build to prevent layouts from overwriting each other:

```yaml
builds:
  profet:
    entry: central.ato:VanCentralControllerPROFET
    paths:
      layout: ./layouts/profet
  modular:
    entry: central.ato:VanCentralControllerModular
    paths:
      layout: ./layouts/modular
```

---

## 2. Defining Components & Local Footprints

### Local Footprint Files
In atopile projects without global library path mappings, footprints must be referenced as local `.kicad_mod` files in the project source directory.
If a standard KiCad footprint (e.g., `Package_SO:SOIC-14_3.9x8.7mm_P1.27mm`) is needed:
1. Locate the footprint file in the system library path (e.g., Flatpak KiCad path: `/var/lib/flatpak/runtime/org.kicad.KiCad.Library.Footprints/...`).
2. Copy it into the project directory (e.g., `hardware/central-pcb/`).
3. If necessary, edit the `.kicad_mod` file (a plain text S-expression) to add custom pads, such as an exposed thermal pad.
4. Reference the local filename in the `footprint` attribute:
   ```ato
   component PROFET_BTS5008_1EKB:
       trait is_atomic_part<manufacturer="Infineon", partnumber="BTS5008-1EKB", footprint="SOIC-14-1EP_3.9x8.7mm_P1.27mm_EP4.3x4.7mm.kicad_mod", symbol="Power_Switch:BTS5008-1EKB">
   ```

### Keyword Collision Avoidance
Avoid naming atopile signals or variables with reserved compiler/python keywords (such as `is`). Rename current sense diagnostic signals to `current_sense` or similar to avoid parser syntax errors.

---

## 3. Compiling Targeted Builds

To build a specific configuration target from [ato.yaml](file:///home/daniel/Projects/vannow/hardware/central-pcb/ato.yaml), run the `ato build` command with the `--build` (or `-b`) flag.
*(Do not use the `--target` flag, which is reserved for specifying output formats like `netlist` or `bom`.)*

```bash
# Compile the integrated PROFET board
/home/daniel/.local/bin/ato build --build profet

# Compile the modular connector-only board
/home/daniel/.local/bin/ato build --build modular
```

---

## 4. Automatic Grid Layout Arrangement

After compilation, new footprints in KiCad layouts will initially render stacked in a single line. Use the automated python arranger script to group components by their parent modules (based on the `atopile_address` property) and layout their positions in a clean 2D grid.

### Running the Arranger Script
Execute the script from the workspace directory:
```bash
# Arrange the PROFET design layout
python3 ./.gemini/skills/atopile-multi-target-layout/scripts/arrange_grid.py hardware/central-pcb/layouts/profet/profet.kicad_pcb

# Arrange the Modular design layout
python3 ./.gemini/skills/atopile-multi-target-layout/scripts/arrange_grid.py hardware/central-pcb/layouts/modular/modular.kicad_pcb
```

Once arranged, re-run the `ato build` commands to verify that the compiler successfully compiles the netlist and updates nets while fully preserving the newly assigned coordinate positions of all footprints.
