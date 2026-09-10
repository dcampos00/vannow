# PCB Rendering, 3D Visualization, and Mechanical CAD Export

This guide documents the engineering workflow for generating photorealistic 3D raytraced renders, 2D vector silkscreen assembly drawings, and populated 3D STEP CAD models from KiCad / Atopile PCB designs.

---

## 1. Overview & Architecture

Modern electronic hardware development requires visual and mechanical verification before sending boards to fabrication (Gerbers/BOM/Pick-and-Place):
1. **Photorealistic 3D Raytracing:** Visual inspection of component placements, clearance between connectors, polarity markings, and aesthetic appearance (solder mask color, silkscreen readability).
2. **Mechanical CAD Assembly Integration:** Populated 3D STEP board export for direct import into enclosure CAD models (`build123d` / FreeCAD / OpenCASCADE) to verify standoff heights, connector aperture cutouts, and wire bend radii.
3. **Vector Assembly Documentation:** High-resolution SVG/PNG 2D engineering drawings for assembly manuals and field wiring diagrams.

```mermaid
flowchart LR
    A["Atopile Circuit Design<br/>(central.ato)"] -->|ato build| B["KiCad PCB Layout<br/>(*.kicad_pcb)"]
    B -->|kicad-cli pcb render| C["Photorealistic 3D Renders<br/>(PNG 1440p/4K)"]
    B -->|kicad-cli pcb export step| D["Populated 3D STEP Model<br/>(*.step)"]
    B -->|kicad-cli pcb export svg| E["Vector Silkscreen Drawing<br/>(*.svg)"]
    D -->|import_step()| F["Enclosure CAD Validation<br/>(build123d Assembly)"]
```

---

## 2. Tooling & Headless Execution

KiCad provides the `kicad-cli` tool for headless batch rendering on Linux workstations, CI/CD pipelines, and containerized environments.

### 2.1 Invocation (Native vs Flatpak)
When KiCad is installed via Flatpak (`org.kicad.KiCad`), execute `kicad-cli` inside the Flatpak sandbox:

```bash
# Native binary
kicad-cli pcb render [options] <file.kicad_pcb>

# Flatpak binary
flatpak run --command=kicad-cli org.kicad.KiCad pcb render [options] <file.kicad_pcb>
```

The automated script [`hardware/central-pcb/scripts/render_pcb.py`](../../../hardware/central-pcb/scripts/render_pcb.py) automatically discovers whether KiCad CLI is installed natively or via Flatpak.

---

## 3. Photorealistic 3D Raytracing (`pcb render`)

KiCad's internal raytracing engine builds bounding volume hierarchy (BVH) trees for pads, copper traces, and 3D component meshes, rendering realistic lighting, ambient occlusion, reflections, and contact shadows.

### 3.1 Recommended CLI Command for Isometric Perspective
```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output hardware/central-pcb/renders/profet_isometric.png \
  --rotate -50,0,40 \
  --perspective \
  --floor \
  --use-board-stackup-colors \
  --quality high \
  --width 2560 \
  --height 1440 \
  hardware/central-pcb/layouts/profet/profet.kicad_pcb
```

### 3.2 Key Parameters Explained

| Parameter | Recommended Value | Engineering Purpose |
| :--- | :--- | :--- |
| `--rotate` | `-50,0,40` | Sets camera rotation (pitch, yaw, roll) to standard 3/4 isometric perspective. |
| `--perspective` | Flag present | Replaces orthographic parallel projection with real camera perspective (depth foreshortening). |
| `--floor` | Flag present | Generates a neutral ground plane with soft contact shadows beneath the board. |
| `--use-board-stackup-colors` | Flag present | Renders the exact solder mask color (e.g., Matte Black, Purple, Blue) and pad finish (ENIG gold / HASL silver) specified in the board stackup setup. |
| `--quality` | `high` | Enables multi-sample anti-aliasing, soft shadow penumbras, and post-processing shaders. |
| `--side` | `top` or `bottom` | Generates pure orthographic inspection projections (no rotation). |
| `--width / --height` | `2560x1440` (1440p) | High pixel density for sharp silkscreen text and package details. |

---

## 4. Board Outline (`Edge.Cuts`) Requirements

If a `.kicad_pcb` file lacks an explicit closed polygon on the `Edge.Cuts` layer:
- KiCad CLI outputs: `Warning: Board outline is missing or malformed. Run DRC for a full analysis.`
- KiCad defaults the board substrate to an arbitrary bounding box surrounding all footprints, causing ragged or oversized board edges.

### 4.1 Defining Clean Board Outlines
A proper PCB boundary is defined on the `Edge.Cuts` layer. In S-expression syntax:
```scheme
(gr_rect (start 18 18) (end 292 172)
    (stroke (width 0.15) (type solid))
    (fill none)
    (layer "Edge.Cuts")
    (uuid "00000000-edge-cuts-rect-000000000001")
)
```

### 4.2 Integrated Mounting Holes
Physical through-holes (for M3 mounting screws) can be placed directly on `Edge.Cuts`:
```scheme
(gr_circle (center 24 24) (end 25.6 24)
    (stroke (width 0.15) (type solid))
    (fill none)
    (layer "Edge.Cuts")
    (uuid "00000000-edge-cuts-hole-000000000001")
)
```
In 3D raytracing, these circles are rendered as true physical cutouts, allowing light and floor reflections to pass through.

---

## 5. 3D STEP Export for Mechanical Enclosure Assembly

To verify that connectors clear enclosure apertures and that PCB mounting holes align with enclosure standoffs, export the board as a STEP solid:

```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb export step \
  --output hardware/central-pcb/renders/profet.step \
  --subst-models \
  hardware/central-pcb/layouts/profet/profet.kicad_pcb
```

### 5.1 Verification in Python Code-as-CAD (`build123d`)
The exported STEP model can be imported directly into `build123d` scripts:
```python
from build123d import *

# 1. Import enclosure base
enclosure = import_step("hardware/enclosures/models/central_enclosure_base.step")

# 2. Import populated PCB assembly
pcb = import_step("hardware/central-pcb/renders/profet.step")

# 3. Position PCB onto 12mm standoffs (standoff_z = floor + 12mm)
pcb_mounted = pcb.locate(Location((0, 0, 15.5)))

# 4. Check for interference
collision = enclosure.intersect(pcb_mounted)
assert collision.volume == 0, f"Interference detected: volume = {collision.volume} mm3"
```

---

## 6. 2D Vector Silkscreen & Fabrication Plots

For production manuals, assembly verification, and wiring cheat-sheets, generate vector SVGs:

```bash
# Export single SVG with copper, silkscreen, soldermask, and board edge
flatpak run --command=kicad-cli org.kicad.KiCad pcb export svg \
  --mode-single \
  --layers F.Cu,F.SilkS,F.Mask,Edge.Cuts \
  --exclude-drawing-sheet \
  --page-size-mode 2 \
  -o hardware/central-pcb/renders/profet_silkscreen.svg \
  hardware/central-pcb/layouts/profet/profet.kicad_pcb

# Rasterize to 300 DPI PNG
magick -density 300 hardware/central-pcb/renders/profet_silkscreen.svg hardware/central-pcb/renders/profet_silkscreen.png
```

---

## 7. Project Automated Script

To render all boards and views in one step, use the project script:

```bash
# Render all views for PROFET design
python3 hardware/central-pcb/scripts/render_pcb.py --board profet

# Render all views for both PROFET and Modular designs
python3 hardware/central-pcb/scripts/render_pcb.py --board all
```
