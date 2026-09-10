# KiCad CLI Raytracing & Rendering Parameters Reference Guide

This reference guide details the parameters, lighting models, camera projections, and material configurations available in `kicad-cli pcb render` for headless rendering.

---

## 1. Camera Control & Projection Models

The KiCad raytracer supports both orthographic (parallel) and perspective (pinhole camera) viewing models.

### 1.1 Rotation Angles (`--rotate X,Y,Z`)
Rotations are applied in degrees around the Euler axes:
* **$X$ (Pitch):** Tilts the board toward or away from the camera.
* **$Y$ (Roll):** Tilts the board sideways.
* **$Z$ (Yaw):** Spins the board around its normal axis.

#### Recommended Presets:
| View Style | `--rotate` Argument | Description |
| :--- | :--- | :--- |
| **Standard Isometric (Hero View)** | `-50,0,40` | Classic 3/4 axonometric perspective. Balances visibility of top traces, connector heights, and side terminals. |
| **Steep Axonometric (Component Focus)** | `-65,0,30` | Steeper angle, ideal for densely populated boards where top text legibility is primary. |
| **Low Horizon (Connector Profile View)** | `-30,0,45` | Low grazing angle, highlighting vertical component clearances (capacitors, heat sinks, terminals). |
| **Top Orthographic** | `--side top` (no rotate) | Pure 2D perpendicular top view for dimensional validation. |
| **Bottom Orthographic** | `--side bottom` (no rotate) | Bottom copper traces and solder joint inspection. |

### 1.2 Perspective vs. Orthographic (`--perspective`)
* **Without `--perspective` (Orthographic):** Parallel rays with no foreshortening. True to scale; parallel lines never converge. Useful for technical drawings and dimensional overlay.
* **With `--perspective` (Perspective):** Simulates a real-world focal lens. Components further away appear smaller, enhancing 3D depth perception and realistic presentation.

### 1.3 Pan, Zoom, and Pivot
* `--zoom <factor>`: Multiplier for camera distance (e.g. `1.2` zooms in by 20%, `0.8` zooms out).
* `--pan X,Y,Z`: Shifts the camera target vector.
* `--pivot X,Y,Z`: Sets the center of rotation relative to board center in centimeters (e.g. `0,0,0`).

---

## 2. Lighting & Ground Shading Architecture

KiCad's raytracer uses a directional and ambient lighting model with an optional ground reflection plane.

### 2.1 Floor & Shadows (`--floor`)
* When `--floor` is enabled:
  1. A horizontal ground plane is inserted beneath the lowest point of the board assembly.
  2. Ambient occlusion and contact shadows are cast onto this floor.
  3. Specular highlights and reflections ground the electronic assembly visually, eliminating the "floating in void" look.

### 2.2 Background Modes (`--background`)
* `transparent` (Default for PNG): Perfect for embedding into documentation, white papers, and dark/light mode web interfaces.
* `opaque` (Default for JPEG): Fills background with the environment color.
* `default`: Uses KiCad's default viewport background gradient.

### 2.3 Fine-Grained Light Source Control
When default lighting requires artistic or high-contrast adjustments:
* `--light-top <R,G,B>` (range 0.0 to 1.0): Direct overhead sun/key light.
* `--light-side <R,G,B>`: Fill light from side quadrants.
* `--light-side-elevation <angle>` (0° to 90°, default 60°): Elevation angle of side fill lights.
* `--light-camera <R,G,B>`: Front-facing fill light originating from the camera lens to prevent deep black shadows on vertical walls.

---

## 3. Materials, Stackup & Surface Finishes

To produce authentic renders matching manufactured prototypes, KiCad utilizes physical material properties configured in the board stackup (`setup.stackup` in `.kicad_pcb`):

### 3.1 Stackup Color Enforcement (`--use-board-stackup-colors`)
Always pass `--use-board-stackup-colors`. This forces the raytracer to read the specific colors defined in the layout rather than the generic green user preset:
* **Matte Black Mask:** `(color "Black")` with low specular roughness.
* **Classic Forest Green:** `(color "Green")`.
* **Deep Blue / Purple:** Popular for open-source and developer boards.

### 3.2 Copper & Plating Reflections
* **ENIG (Electroless Nickel Immersion Gold):** `(copper_finish "ENIG")` renders pads and through-hole barrels with realistic warm metallic gold reflectance.
* **HASL (Hot Air Solder Leveling):** Renders pads with bright silver-tin metallic specular highlights.

---

## 4. Production Shell Recipes

### Recipe A: 4K Ultra-HD Marketing Hero Render
```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output pcb_hero_4k.png \
  --rotate -50,0,40 \
  --perspective \
  --floor \
  --use-board-stackup-colors \
  --quality high \
  --width 3840 \
  --height 2160 \
  board.kicad_pcb
```

### Recipe B: Transparent Background Cutout for Documentation
```bash
flatpak run --command=kicad-cli org.kicad.KiCad pcb render \
  --output pcb_transparent.png \
  --rotate -48,0,38 \
  --perspective \
  --background transparent \
  --use-board-stackup-colors \
  --quality high \
  --width 2560 \
  --height 1440 \
  board.kicad_pcb
```
