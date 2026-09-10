---
name: pcb-rendering
description: >-
  Comprehensive guide and automated workflow for generating photorealistic 3D raytraced renders, 2D vector silkscreen plots,
  and populated 3D STEP CAD models from KiCad PCB layouts, including ECAD/MCAD clash detection in Python build123d.
---

# PCB 3D Rendering, Visualization & CAD Assembly Guide

This skill provides an authoritative, automated methodology for generating photorealistic 3D raytraced renders, 2D vector fabrication drawings, and populated 3D STEP mechanical models from KiCad PCB layouts, with direct integration into Python Code-as-CAD (`build123d` / OpenCASCADE) for electromechanical clearance verification.

---

## Core Engineering Directives

1. **Closed Board Outline (`Edge.Cuts`) Integrity:** Never attempt to render a PCB without a continuous closed polygon on layer `Edge.Cuts`. Missing outlines trigger fallback bounding boxes, stripping chamfers, radius corners, and mounting hole apertures.
2. **Orphan Footprint Origin (0, 0) Audit:** Always inspect footprint coordinates before rendering. Netlist synchronization and Hardware-as-Code generators (e.g. Atopile `ato build`) often place unpositioned components at `(0, 0)`, artificially inflating the board bounding box by hundreds of millimeters.
3. **Physical Stackup Material Fidelity:** Enforce `--use-board-stackup-colors` during rendering so that realistic solder mask finishes (Matte Black, Forest Green, Deep Blue) and copper platings (ENIG gold vs. HASL silver) reflect physical reality.
4. **Populated 3D STEP Export for ECAD/MCAD Interlock:** Always export populated STEP models (`pcb export step --subst-models`) alongside visual images. A visual render verifies aesthetics; the STEP solid verifies spatial clearance inside the physical enclosure.
5. **Universal Headless CLI Discovery:** Scripts must transparently support both native `kicad-cli` and containerized / sandboxed Flatpak installations (`org.kicad.KiCad`), ensuring CI/CD and developer workstation reproducibility.

---

## End-to-End PCB Rendering & CAD Workflow

```mermaid
flowchart TD
    A["KiCad Layout (.kicad_pcb)"] --> B["Step 1: Sanity Audit (Edge.Cuts & Footprint Extents)"]
    B --> C["Step 2: CLI Discovery (Native / Flatpak)"]
    C --> D1["Step 3: 3D Raytraced Renders (Isometric & Ortho)"]
    C --> D2["Step 4: 2D Vector Silkscreen (SVG / PNG)"]
    C --> D3["Step 5: 3D STEP Assembly Export"]
    D3 --> E["Step 6: ECAD/MCAD Clash Detection (build123d)"]
```

---

## Step-by-Step Implementation Guide

### Step 1: Pre-Render Layout Sanity Checks

Before invoking the renderer, verify that the PCB layout satisfies three critical geometrical conditions:

#### 1.1 Closed Outline on `Edge.Cuts`
A valid board outline must exist on layer `Edge.Cuts`. In KiCad S-expression syntax:
```scheme
(gr_rect (start X1 Y1) (end X2 Y2)
    (stroke (width 0.15) (type solid))
    (fill none)
    (layer "Edge.Cuts")
    (uuid "...")
)
```

#### 1.2 Physical Mounting Hole Apertures
Screw mounting holes cut through the FR4 substrate should be explicitly present on `Edge.Cuts` as closed circles (e.g., dia 3.2 mm for M3 fasteners):
```scheme
(gr_circle (center X Y) (end X+1.6 Y)
    (stroke (width 0.15) (type solid))
    (fill none)
    (layer "Edge.Cuts")
    (uuid "...")
)
```
In 3D raytracing, these cutouts let light and ground plane shadows pass cleanly through the board.

#### 1.3 Audit Orphan Footprints at (0, 0)
Run a quick coordinate scan to confirm no parts are stranded at the coordinate origin:
```python
import re

with open("path/to/board.kicad_pcb") as f:
    text = f.read()

footprints = re.findall(r'\(footprint\s+"([^"]+)"[\s\S]*?\(at\s+([-\d.]+)\s+([-\d.]+)', text)
orphans = [fp for fp, x, y in footprints if float(x) == 0.0 and float(y) == 0.0]
assert not orphans, f"Found orphan footprints at origin (0, 0): {orphans}"
```

---

### Step 2: Portable Headless CLI Discovery

KiCad 7, 8, and 10 provide `kicad-cli`. On Linux developer workstations and immutable OS distributions (e.g. Fedora Silverblue/Kinoite), KiCad is frequently installed via Flatpak.

Implement robust discovery in Python:
```python
import shutil
import subprocess

def get_kicad_cli():
    # 1. Check native system PATH
    native = shutil.which("kicad-cli")
    if native:
        return [native]

    # 2. Check Flatpak sandbox
    flatpak = shutil.which("flatpak")
    if flatpak:
        res = subprocess.run(
            [flatpak, "run", "--command=kicad-cli", "org.kicad.KiCad", "--version"],
            capture_output=True, text=True
        )
        if res.returncode == 0:
            return [flatpak, "run", "--command=kicad-cli", "org.kicad.KiCad"]

    raise RuntimeError("kicad-cli not found in PATH or Flatpak org.kicad.KiCad")
```

---

### Step 3: Photorealistic 3D Raytracing (`pcb render`)

KiCad's internal raytracer computes ambient occlusion, multi-bounce reflections, and ground plane contact shadows.

#### Recommended 3D Isometric View (with Floor Shadows)
```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output central_pcb_isometric.png \
  --rotate -50,0,40 \
  --perspective \
  --floor \
  --use-board-stackup-colors \
  --quality high \
  --width 2560 \
  --height 1440 \
  board.kicad_pcb
```

#### Recommended Orthographic Top & Bottom Inspection Views
```bash
# Top Inspection
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output central_pcb_top.png \
  --side top \
  --use-board-stackup-colors \
  --quality high \
  --width 2560 \
  --height 1440 \
  board.kicad_pcb

# Bottom Inspection
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output central_pcb_bottom.png \
  --side bottom \
  --use-board-stackup-colors \
  --quality high \
  --width 2560 \
  --height 1440 \
  board.kicad_pcb
```

- Detailed reference: [raytracing_parameters_guide.md](./references/raytracing_parameters_guide.md)

---

### Step 4: 2D Vector Silkscreen & Fabrication Plots

Generate clean vector SVGs and high-resolution PNGs for wiring guides and assembly manuals:

```bash
# 1. Export SVG with copper, silkscreen, soldermask, and board boundary
flatpak run --command=kicad-cli org.kicad.KiCad pcb export svg \
  --mode-single \
  --layers F.Cu,F.SilkS,F.Mask,Edge.Cuts \
  --exclude-drawing-sheet \
  --page-size-mode 2 \
  -o central_pcb_silkscreen.svg \
  board.kicad_pcb

# 2. Rasterize to 300 DPI PNG via ImageMagick
magick -density 300 central_pcb_silkscreen.svg central_pcb_silkscreen.png
```

---

### Step 5: Populated 3D STEP Assembly Export

Export the complete 3D mechanical assembly of the board and its populated electronic components into a single STEP solid:

```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb export step \
  --output central_pcb.step \
  --subst-models \
  board.kicad_pcb
```
*Note: The `--subst-models` flag automatically substitutes VRML (`.wrl`) models with exact parametric STEP (`.step`) equivalents.*

---

### Step 6: Automated ECAD/MCAD Clash Detection (`build123d`)

Verify that the PCB fits inside the mechanical enclosure, standoffs align with holes, and tall components clear the lid:

```python
from build123d import *

# 1. Import enclosure base and lid
base = import_step("hardware/enclosures/models/central_enclosure_base.step")
lid = import_step("hardware/enclosures/models/central_enclosure_lid.step")

# 2. Import populated PCB assembly
pcb = import_step("hardware/central-pcb/renders/profet.step")

# 3. Position PCB on top of 12mm standoffs (Z = floor_thickness + standoff_height)
standoff_z = 3.5 + 12.0
pcb_positioned = pcb.locate(Location((0, 0, standoff_z)))

# 4. Programmatic collision test against enclosure walls and lid
base_clash = base.intersect(pcb_positioned)
assert base_clash.volume == 0, f"Clash with base enclosure detected: volume = {base_clash.volume} mm³"

lid_clash = lid.locate(Location((0, 0, 45.0))).intersect(pcb_positioned)
assert lid_clash.volume == 0, f"Clash with lid detected: volume = {lid_clash.volume} mm³"

print("ECAD/MCAD Verification Passed: Zero geometric interference detected!")
```

- Detailed reference: [ecad_mcad_clash_detection.md](./references/ecad_mcad_clash_detection.md)

---

## Troubleshooting & Common Pitfalls

| Symptom / Error | Root Cause | Engineering Solution |
| :--- | :--- | :--- |
| `Warning: Board outline is missing or malformed.` | Layer `Edge.Cuts` lacks a closed rectangle or polygon. | Add a closed `(gr_rect ...)` or `(gr_poly ...)` around the component area on `Edge.Cuts`. |
| Rendered board is huge and mostly empty space. | An unplaced footprint (e.g. newly added resistor) was defaulted to `(0, 0)`. | Scan footprints for `(at 0 0)` and move the component adjacent to its functional circuit group. |
| Component 3D models appear as blank or missing blocks. | Footprint references `${KICADx_3DMODEL_DIR}` which isn't mapped inside Flatpak or custom path. | Pass `--define-var KICAD10_3DMODEL_DIR=/app/extensions/Library/Packages3D/3dmodels` or use standard library packages. |
| Mounting holes do not let light through in raytracing. | Mounting holes are defined as pads or silkscreen rather than through-holes. | Define mounting hole cutouts as `(gr_circle ... (layer "Edge.Cuts"))` or use plated through-hole footprints with drill holes. |
| Board renders with default green rather than custom color. | Missing `--use-board-stackup-colors` flag. | Add `--use-board-stackup-colors` to respect the Matte Black / Purple stackup color defined in the board setup. |

---

## Pre-Render Verification Checklist

Before publishing renders or ordering manufactured boards, ensure:

- [ ] **Continuous Board Edge:** Closed rectangle/polygon on `Edge.Cuts` with proper corner radii (e.g., R=2.0 mm).
- [ ] **Mounting Hole Alignment:** Corner mounting hole centers match the 3D enclosure standoff centers (e.g. 140 mm × 85 mm pitch).
- [ ] **No (0, 0) Orphan Components:** All footprints explicitly positioned in functional blocks.
- [ ] **Component 3D Models:** Sockets, terminals, PROFETs, and diodes have valid 3D shapes attached.
- [ ] **Camera Angle:** Isometric 3/4 axonometric perspective (`--rotate -50,0,40 --perspective --floor`).
- [ ] **Visual Stackup Colors:** `--use-board-stackup-colors` enabled.
- [ ] **STEP Assembly Export:** 3D solid `.step` exported and verified against enclosure CAD.
