# build123d Best Practices & Architectural Idioms

This reference details critical lessons, anti-patterns, and robust geometric idioms discovered when modeling production electronics enclosures with **build123d** on OpenCASCADE.

---

## 1. The Global Plane-Reset Trap

### The Issue
In `build123d`, global plane objects such as `Plane.XY`, `Plane.XZ`, and `Plane.YZ` are static geometric coordinate frames whose origin is permanently fixed at $(0, 0, 0)$.

When passing `Plane.XZ` into `BuildSketch(Plane.XZ)` inside a `with Locations((x, y, z)):` block:
```python
# ❌ DANGEROUS: The outer location (x, y, z) is silently discarded!
with Locations((50.0, -60.0, 25.0)):
    with BuildSketch(Plane.XZ):
        Circle(radius=8.0)
    extrude(amount=5.0)
```
The resulting cylinder will be created at $(0, 0, 0)$ on the XZ plane, causing invisible features inside the wrong walls or cutting through the middle of the enclosure!

### The Solution: 3D Primitives with Explicit Rotation
Use standard 3D primitives (`Cylinder`, `Box`, `Cone`) with the `rotation` parameter:
```python
# ✅ SAFE & IDIOMATIC:
with Locations((50.0, -60.0, 25.0)):
    Cylinder(radius=8.0, height=5.0, rotation=(90, 0, 0))
```
3D primitives are fully location-aware and correctly inherit translation, rotation, and alignment (`align=(Align.CENTER, ...)`).

---

## 2. 2D Sketch Profiles vs. Fragile 3D Booleans

### The Anti-Pattern
Creating a solid 3D box, applying a 3D corner fillet, using `offset(amount=-wall)` to hollow it, and then fusing separate 3D mounting lugs to the outside:
```python
# ❌ FRAGILE:
Box(length, width, height)
fillet(base.edges().filter_by(Axis.Z), 6.0)
offset(amount=-wall, openings=base.faces().sort_by(Axis.Z)[-1])

# Adding mounting ears separately:
with Locations((-length/2 - 10, 0), (length/2 + 10, 0)):
    Box(20, 50, floor)
    fillet(base.edges().filter_by(Axis.Z), 4.0) # Selects ALL Z-edges of the ENTIRE part!
```
**Why this fails:**
1. Global edge filters like `edges().filter_by(Axis.Z)` target all vertical edges in the entire model, repeatedly attempting to re-fillet already filleted edges.
2. `offset(openings=...)` on non-convex or pre-filleted solids frequently causes self-intersecting B-Rep surfaces or OpenCASCADE kernel crashes.
3. Butt-joining separate 3D blocks creates non-manifold lines along the tangent seams.

### The Idiomatic Pattern: Layered 2D Sketches
Construct the part bottom-up in continuous 2D sketch layers:
```python
with BuildPart() as base:
    # Layer 1: Floor plate with mounting ears as a SINGLE unified 2D sketch
    with BuildSketch(Plane.XY) as s_floor:
        RectangleRounded(length, width, 6.0)
        with Locations((-length / 2 - 11.0, 0), (length / 2 + 11.0, 0)):
            RectangleRounded(22.0, 52.0, 4.0)
            Circle(radius=2.25, mode=Mode.SUBTRACT) # M4 mounting holes
    extrude(amount=floor_thickness)

    # Layer 2: Hollow vertical walls extruded up from floor
    with BuildSketch(Plane.XY.offset(floor_thickness)) as s_walls:
        RectangleRounded(length, width, 6.0)
        RectangleRounded(length - 2 * wall, width - 2 * wall, 6.0 - wall, mode=Mode.SUBTRACT)
    extrude(amount=height - floor_thickness)

    # Layer 3: Stepped perimeter alignment lip
    with BuildSketch(Plane.XY.offset(height)) as s_lip:
        RectangleRounded(length - 2 * lip_w, width - 2 * lip_w, 6.0 - lip_w)
        RectangleRounded(length - 2 * wall, width - 2 * wall, 6.0 - wall, mode=Mode.SUBTRACT)
    extrude(amount=lip_h)
```
**Benefits:**
- 100% guaranteed watertight manifold solid (`is_valid == True`).
- Deterministic wall thickness at every point.
- Zero fragile topological edge selections.

---

## 3. Sketch Slot Geometry Constraints
In `build123d`, `RectangleRounded(width, height, radius)` enforces:
$$\text{width} > 2 \cdot \text{radius} \quad \text{and} \quad \text{height} > 2 \cdot \text{radius}$$
If `radius == height / 2`, it raises `ValueError("width and height must be > 2*radius")`.

* **To create slots with fully rounded semicircular ends:** Use `SlotOverall(length, width)`:
  ```python
  with BuildSketch(Plane.YZ):
      SlotOverall(length=18.0, width=3.2)
  ```
