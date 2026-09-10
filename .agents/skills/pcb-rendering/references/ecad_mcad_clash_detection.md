# ECAD to MCAD Integration & Programmatic Clash Detection

This guide documents the electromechanical verification workflow for validating that populated PCB assemblies fit accurately inside 3D enclosures using **Python Code-as-CAD (`build123d` on OpenCASCADE)**.

---

## 1. The Electromechanical Design Gap

In traditional hardware workflows, PCB designers (ECAD) and mechanical engineers (MCAD) work in isolated silos:
* **Common Failure Modes:**
  1. Standoff screw holes on the PCB misaligned by 0.5–2.0 mm from enclosure bosses.
  2. Tall components (electrolytic capacitors, heatsinks, socketed microcontrollers) colliding with the enclosure lid.
  3. Edge connectors (screw terminals, USB-C jacks, RJ45 ports) colliding with enclosure walls or recessed too deeply to insert mating cables.
  4. Screw head clearances interfering with nearby surface-mount components.

By bridging KiCad's populated STEP export directly into `build123d`, electromechanical clearance testing can be **fully automated in software unit tests**.

---

## 2. Populated STEP Export Pipeline

KiCad footprints can associate both VRML (`.wrl`) visual meshes and CAD-accurate boundary representation (`.step`) solids.

When exporting for mechanical assembly, invoke:
```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb export step \
  --output pcb_assembly.step \
  --subst-models \
  board.kicad_pcb
```

* **`--subst-models`:** Automatically locates and substitutes STEP solids in place of VRML files with the same base name in `${KICAD_3DMODEL_DIR}`.
* **`--no-optimize-step`:** Preserves parametric spline geometries when needed.

---

## 3. Programmatic Clearance Verification in Python (`build123d`)

Using `build123d`, the populated board STEP and the enclosure base/lid STEP solids can be loaded into the same 3D coordinate space and tested for physical interference.

```python
#!/usr/bin/env python3
"""
Automated Electromechanical Fit & Clash Detection
Validates PCB assembly positioning inside parametric 3D enclosure.
"""

from build123d import *
import sys

# 1. Load mechanical components
print("Loading enclosure models...")
base = import_step("hardware/enclosures/models/central_enclosure_base.step")
lid = import_step("hardware/enclosures/models/central_enclosure_lid.step")

# 2. Load populated PCB assembly
print("Loading populated PCB solid...")
pcb = import_step("hardware/central-pcb/renders/profet.step")

# 3. Position PCB on enclosure standoffs
# Enclosure specs: Base floor = 3.5mm, Standoff height = 12.0mm
pcb_z_elevation = 3.5 + 12.0
pcb_mounted = pcb.locate(Location((0, 0, pcb_z_elevation)))

# 4. Check interference against base walls and standoffs
print("Testing interference with base enclosure...")
base_clash = base.intersect(pcb_mounted)
if base_clash.volume > 0.01:
    print(f"[FAIL] Physical collision detected with base! Volume: {base_clash.volume:.3f} mm³")
    sys.exit(1)
print(f"[PASS] Zero clash with base enclosure (Volume: {base_clash.volume:.3f} mm³)")

# 5. Check vertical clearance against lid
print("Testing interference with lid...")
# Lid mating plane is at Z = 45.0mm
lid_mounted = lid.locate(Location((0, 0, 45.0)))
lid_clash = lid_mounted.intersect(pcb_mounted)
if lid_clash.volume > 0.01:
    print(f"[FAIL] Physical collision detected with lid! Volume: {lid_clash.volume:.3f} mm³")
    sys.exit(1)
print(f"[PASS] Zero clash with lid (Volume: {lid_clash.volume:.3f} mm³)")

# 6. Measure minimum vertical air gap to lid roof
pcb_top_z = pcb_mounted.bounding_box().max.Z
lid_inner_roof_z = 45.0 + 3.0  # internal cavity height
vertical_airgap = lid_inner_roof_z - pcb_top_z

print(f"\nElectromechanical Summary:")
print(f"  - PCB Top Elevation:   {pcb_top_z:.2f} mm")
print(f"  - Inner Ceiling:       {lid_inner_roof_z:.2f} mm")
print(f"  - Remaining Air Gap:   {vertical_airgap:.2f} mm")
assert vertical_airgap >= 3.0, f"Air gap ({vertical_airgap:.2f} mm) is below 3.0 mm safety margin!"

print("\nAll electromechanical constraints satisfied successfully!")
```

---

## 4. Key Metrics & Clearance Rules

| Clearance Zone | Minimum Safe Distance | Engineering Justification |
| :--- | :--- | :--- |
| **Component to Lid Ceiling** | $\ge 3.0\,\text{mm}$ | Prevents contact vibration, thermal conduction to plastic, and accommodates component tolerances. |
| **Screw Head Land** | $\ge 1.0\,\text{mm}$ radial clearance | Prevents steel screw heads (DIN 912 / ISO 7380) from crushing adjacent SMD passives or breaking solder mask. |
| **Terminal Wire Entry to Gland** | $\ge 25.0\,\text{mm}$ | Respects automotive 12/14 AWG wire bend radius ($R_{bend} \ge 4 \times D_{wire}$). |
| **PCB Edge to Enclosure Wall** | $\ge 1.5\,\text{mm}$ | Accounts for CNC routing tolerances ($\pm 0.2\,\text{mm}$) and 3D printing thermal shrinkage. |
