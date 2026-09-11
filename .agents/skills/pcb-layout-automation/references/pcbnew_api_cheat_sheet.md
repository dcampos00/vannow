# KiCad `pcbnew` Python API Cheat Sheet

This guide provides tested, syntax-verified Python code patterns for manipulating KiCad PCB layouts programmatically using the `pcbnew` API in KiCad 7, 8, 9, and 10.

---

## 1. Execution Environment & Flatpak Invocation

On Linux systems where KiCad is installed via Flatpak (or system packages), run Python scripts using the Flatpak runner:

```bash
# 24-channel production board (default)
flatpak run --command=python3 org.kicad.KiCad \
  hardware/central-pcb/scripts/layout_and_route_24ch.py

# Legacy 11-channel only when explicitly editing that carrier
flatpak run --command=python3 org.kicad.KiCad \
  hardware/central-pcb/scripts/layout_and_route.py
```

---

## 2. Fundamental Units & Vector Math

KiCad internally represents all spatial coordinates in **nanometers** (1 mm = 1,000,000 nm).

```python
import pcbnew

def mm_to_nm(mm: float) -> int:
    return int(round(mm * 1e6))

def nm_to_mm(nm: int) -> float:
    return float(nm) / 1e6

# In KiCad 7+, positions use VECTOR2I (integer 2D vector in nanometers)
pos = pcbnew.VECTOR2I(mm_to_nm(100.0), mm_to_nm(50.0))

# Angles in KiCad 7/8/9/10 use EDA_ANGLE
angle = pcbnew.EDA_ANGLE(90.0, pcbnew.DEGREES_T)  # 90 degrees rotation
```

---

## 3. Loading, Saving, and Net Lookup

```python
import pcbnew

# 1. Load existing board (generated from Atopile or schematic netlist)
board = pcbnew.LoadBoard("hardware/central-pcb/layouts/profet_24ch/profet_24ch.kicad_pcb")

# 2. Lookup Nets by name
def get_net(board, net_name: str):
    net = board.FindNet(net_name)
    if not net:
        raise ValueError(f"Net '{net_name}' not found on board!")
    return net

net_12v = get_net(board, "VCC_12V")
net_gnd = get_net(board, "GND")

# 3. Save modified board
board.Save("hardware/central-pcb/layouts/profet_24ch/profet_24ch.kicad_pcb")
```

---

## 4. Programmatic Component Placement

```python
def place_footprint(board, reference: str, x_mm: float, y_mm: float, rotation_deg: float = 0.0):
    fp = board.FindFootprintByReference(reference)
    if not fp:
        print(f"Warning: Footprint {reference} not found!")
        return None
        
    # Set position
    fp.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x_mm), mm_to_nm(y_mm)))
    
    # Set orientation
    fp.SetOrientation(pcbnew.EDA_ANGLE(rotation_deg, pcbnew.DEGREES_T))
    return fp

# Example: Place terminal block along top edge
place_footprint(board, "J1", 25.4, 12.0, 0.0)
```

---

## 5. Trace Routing & Paths

```python
def route_segment(board, start_xy, end_xy, layer, width_mm: float, net=None):
    """Adds a single straight trace segment."""
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(pcbnew.VECTOR2I(mm_to_nm(start_xy[0]), mm_to_nm(start_xy[1])))
    track.SetEnd(pcbnew.VECTOR2I(mm_to_nm(end_xy[0]), mm_to_nm(end_xy[1])))
    track.SetLayer(layer)
    track.SetWidth(mm_to_nm(width_mm))
    if net:
        track.SetNet(net)
    board.Add(track)
    return track

def route_path(board, points, layer, width_mm: float, net=None):
    """Routes consecutive vertices with 45-degree or orthogonal bends."""
    tracks = []
    for i in range(len(points) - 1):
        tr = route_segment(board, points[i], points[i+1], layer, width_mm, net)
        tracks.append(tr)
    return tracks

# Standard Layers:
# pcbnew.F_Cu  (Top Copper Layer)
# pcbnew.B_Cu  (Bottom Copper Layer)
# pcbnew.In1_Cu, pcbnew.In2_Cu (Internal planes for 4-layer boards)
```

---

## 6. Vias & Thermal Via Arrays

```python
def add_via(board, x_mm: float, y_mm: float, net, drill_mm: float = 0.3, size_mm: float = 0.6):
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(pcbnew.VECTOR2I(mm_to_nm(x_mm), mm_to_nm(y_mm)))
    via.SetWidth(mm_to_nm(size_mm))
    via.SetDrill(mm_to_nm(drill_mm))
    if net:
        via.SetNet(net)
    board.Add(via)
    return via

def add_thermal_via_matrix(board, center_xy, rows: int, cols: int, pitch_mm: float, net):
    """Generates an N x M thermal via array under an exposed thermal pad."""
    x_c, y_c = center_xy
    x_start = x_c - ((cols - 1) * pitch_mm) / 2.0
    y_start = y_c - ((rows - 1) * pitch_mm) / 2.0
    
    for r in range(rows):
        for c in range(cols):
            x = x_start + c * pitch_mm
            y = y_start + r * pitch_mm
            add_via(board, x, y, net, drill_mm=0.3, size_mm=0.6)
```

---

## 7. Ground Plane Flood Zones & Stitching

```python
def create_copper_zone(board, outline_pts, layer, net, clearance_mm: float = 0.3):
    """Creates and fills a copper polygon zone (e.g. GND ground plane)."""
    zone = pcbnew.ZONE(board)
    zone.SetLayer(layer)
    zone.SetNet(net)
    zone.SetThermalReliefGap(mm_to_nm(clearance_mm))
    zone.SetThermalReliefCopperWidth(mm_to_nm(0.4))
    zone.SetClearance(mm_to_nm(clearance_mm))
    
    # Define polygon outline
    poly_chain = pcbnew.SHAPE_POLY_SET()
    poly_chain.NewOutline()
    for pt in outline_pts:
        poly_chain.Append(mm_to_nm(pt[0]), mm_to_nm(pt[1]))
    zone.SetOutline(poly_chain)
    
    board.Add(zone)
    return zone

def fill_all_zones(board):
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
```
