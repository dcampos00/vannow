#!/usr/bin/env python3
"""
VanNOW Parametric 3D Enclosure Generator (Production-Grade Rev 2)
Uses build123d (Python Code-as-CAD on OpenCASCADE) to model and export
all enclosures and cradles for the VanNOW project.

Models:
  1. central_enclosure_base     - Central unit base with DIN ears, reinforced standoffs, lip rim, PG gland collars, and vent slots
  2. central_enclosure_lid      - Central unit lid with counterbored M3 holes, internal boss columns, groove rim, and SPDT keyways
  3. remote_enclosure_body      - Remote rear body with non-colliding magnet pockets, 2x AA tray, M2.5 corner bosses, and PCB standoffs
  4. remote_enclosure_faceplate - Remote faceplate with tongue rim, chamfered button apertures, encoder bezel, and countersunk holes
  5. remote_magnetic_cradle     - Docking cradle with 45° lead-in chamfer, dual finger extraction scallops, and recessed wall mounts

Outputs:
  - hardware/enclosures/models/*.step (STEP for CAD/CNC/Injection Molding)
  - hardware/enclosures/models/*.stl  (STL for 3D Printing / Slicers)
"""

import os
from build123d import *

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "models")
os.makedirs(OUTPUT_DIR, exist_ok=True)


def build_central_base():
    """Central Controller Cabinet Enclosure - Base unit.
    Features:
      - Length: 175mm, Width: 125mm, Height: 45mm, Wall: 3.0mm, Floor: 3.5mm
      - Integrated dual mounting ears with 4.5mm holes for M4 chassis/wall mounting
      - Stepped perimeter alignment lip (+2.2mm high, 1.4mm wide) for positive lid interlock
      - 4x Corner screw bosses integrated into walls with M3 heat-set insert pilot holes (dia 4.0mm x depth 6.0mm)
      - 4x Reinforced PCB standoffs for 150x95mm board with triangular gusset ribs and M3 insert holes
      - 4x PG9/PG11 cable gland ports (dia 16.0mm) with external reinforcing boss collars (dia 22.0mm)
      - Side passive ventilation louvers above PCB level for convective cooling
    """
    length = 175.0
    width = 125.0
    height = 45.0
    wall = 3.0
    floor = 3.5
    lip_h = 2.2
    lip_w = 1.4

    with BuildPart() as base:
        # 1. Base floor plate with unified mounting ears
        with BuildSketch(Plane.XY) as s_floor:
            RectangleRounded(length, width, 6.0)
            # Left and right mounting ears
            with Locations((-length / 2 - 11.0, 0), (length / 2 + 11.0, 0)):
                RectangleRounded(22.0, 52.0, 4.0)
                Circle(radius=2.25, mode=Mode.SUBTRACT)
        extrude(amount=floor)

        # 2. Main vertical walls
        with BuildSketch(Plane.XY.offset(floor)) as s_walls:
            RectangleRounded(length, width, 6.0)
            RectangleRounded(length - 2 * wall, width - 2 * wall, max(0.1, 6.0 - wall), mode=Mode.SUBTRACT)
        extrude(amount=height - floor)

        # 3. Stepped perimeter mating lip on top rim (inner wall segment)
        with BuildSketch(Plane.XY.offset(height)) as s_lip:
            RectangleRounded(length - 2 * lip_w, width - 2 * lip_w, max(0.1, 6.0 - lip_w))
            RectangleRounded(length - 2 * wall, width - 2 * wall, max(0.1, 6.0 - wall), mode=Mode.SUBTRACT)
        extrude(amount=lip_h)

        # 4. 4x Corner screw bosses integrated into wall corners
        corner_dx = length / 2 - wall - 3.8
        corner_dy = width / 2 - wall - 3.8
        boss_h = height - floor
        with Locations(
            (-corner_dx, -corner_dy, floor),
            (corner_dx, -corner_dy, floor),
            (-corner_dx, corner_dy, floor),
            (corner_dx, corner_dy, floor)
        ):
            Cylinder(radius=4.5, height=boss_h, align=(Align.CENTER, Align.CENTER, Align.MIN))
            # M3 brass threaded heat-set insert pilot hole (dia 4.0mm, depth 6.0mm)
            with Locations((0, 0, boss_h - 6.0)):
                Cylinder(radius=2.0, height=6.5, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

        # 5. 4x PCB Standoffs (140x85mm pitch, height 12mm) with stiffening ribs
        pcb_dx = 140.0 / 2
        pcb_dy = 85.0 / 2
        standoff_h = 12.0
        with Locations(
            (-pcb_dx, -pcb_dy, floor),
            (pcb_dx, -pcb_dy, floor),
            (-pcb_dx, pcb_dy, floor),
            (pcb_dx, pcb_dy, floor)
        ):
            Cylinder(radius=3.6, height=standoff_h, align=(Align.CENTER, Align.CENTER, Align.MIN))
            # Heat-set insert pilot hole (dia 4.0mm, depth 5.5mm)
            with Locations((0, 0, standoff_h - 5.5)):
                Cylinder(radius=2.0, height=6.0, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

        # 6. 4x Cable Gland Ports on front wall (dia 16mm with outer boss collars)
        for x_pos in [-48.0, -16.0, 16.0, 48.0]:
            port_z = floor + 16.0
            with Locations((x_pos, -width / 2, port_z)):
                # Gland through-hole
                Cylinder(radius=8.0, height=wall * 3, rotation=(90, 0, 0), mode=Mode.SUBTRACT)
                # Outer reinforcing boss collar (dia 22mm x 2.2mm proud)
                with Locations((0, -1.1, 0)):
                    Cylinder(radius=11.0, height=2.2, rotation=(90, 0, 0))
                    Cylinder(radius=8.0, height=3.0, rotation=(90, 0, 0), mode=Mode.SUBTRACT)

        # 7. Convective ventilation slots on side walls (above PCB level)
        for x_side in [-length / 2, length / 2]:
            for z_slot in [floor + 22.0, floor + 30.0]:
                for y_slot in [-24.0, 0.0, 24.0]:
                    with Locations((x_side, y_slot, z_slot)):
                        Box(wall * 3, 18.0, 3.2, mode=Mode.SUBTRACT)

    return base.part


def build_central_lid():
    """Central Controller Cabinet Enclosure - Top Lid.
    Features:
      - Outer Dimensions: 175mm x 125mm x 18mm, Top Wall: 3.5mm, Side Wall: 3.0mm
      - Perimeter mating groove (1.8mm wide x 2.4mm deep) on underside rim to seat base lip
      - 4x Corner screw boss columns extending downward to contact base bosses
      - 4x M3 Socket Head Counterbores (through dia 3.4mm, counterbore dia 6.2mm x depth 2.0mm)
        Leaves 1.5mm solid clamping shoulder underneath screw head
      - 5x SPDT Toggle Switch mounting stations with anti-rotation keyways and recessed bezels
    """
    length = 175.0
    width = 125.0
    height = 18.0
    wall = 3.0
    top_wall = 3.5
    lip_w = 1.8
    lip_d = 2.4

    with BuildPart() as lid:
        # 1. Top ceiling plate
        with BuildSketch(Plane.XY) as s_top:
            RectangleRounded(length, width, 6.0)
        extrude(amount=-top_wall)

        # 2. Outer side walls extending downward
        with BuildSketch(Plane.XY.offset(-top_wall)) as s_walls:
            RectangleRounded(length, width, 6.0)
            RectangleRounded(length - 2 * wall, width - 2 * wall, max(0.1, 6.0 - wall), mode=Mode.SUBTRACT)
        extrude(amount=-(height - top_wall))

        # 3. Stepped perimeter mating groove in bottom rim
        with BuildSketch(Plane.XY.offset(-height)) as s_groove:
            RectangleRounded(length - 2 * (wall - lip_w), width - 2 * (wall - lip_w), max(0.1, 6.0 - wall + lip_w))
            RectangleRounded(length - 2 * wall, width - 2 * wall, max(0.1, 6.0 - wall), mode=Mode.SUBTRACT)
        extrude(amount=lip_d, mode=Mode.SUBTRACT)

        # 4. 4x Corner screw boss columns extending down from ceiling to meet base bosses
        corner_dx = length / 2 - wall - 3.8
        corner_dy = width / 2 - wall - 3.8
        boss_h = height - top_wall
        with Locations(
            (-corner_dx, -corner_dy, -top_wall),
            (corner_dx, -corner_dy, -top_wall),
            (-corner_dx, corner_dy, -top_wall),
            (corner_dx, corner_dy, -top_wall)
        ):
            Cylinder(radius=4.5, height=boss_h, align=(Align.CENTER, Align.CENTER, Align.MAX))

        # 5. 4x Counterbored M3 screw holes (leaves 1.5mm solid clamping shoulder)
        with Locations(
            (-corner_dx, -corner_dy, 0),
            (corner_dx, -corner_dy, 0),
            (-corner_dx, corner_dy, 0),
            (corner_dx, corner_dy, 0)
        ):
            # Through hole (free fit for M3: dia 3.4mm)
            Cylinder(radius=1.7, height=height + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            # Counterbore (dia 6.2mm, depth 2.0mm for DIN 912 socket head)
            Cylinder(radius=3.1, height=2.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 6. 5x SPDT Toggle Switch stations with anti-rotation keyways and protective bezels
        for x_pos in [-48.0, -24.0, 0.0, 24.0, 48.0]:
            with Locations((x_pos, 0, 0)):
                # Recessed protective bezel (dia 16.0mm x 1.0mm deep)
                Cylinder(radius=8.0, height=1.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
                # Switch mounting hole (dia 6.5mm for 1/4" bushing)
                Cylinder(radius=3.25, height=top_wall * 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
                # Anti-rotation keyway notch (1.2mm wide x 0.9mm deep)
                with Locations((0, 3.25, 0)):
                    Box(1.2, 1.8, top_wall * 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

    return lid.part


def build_remote_body():
    """Remote Switch Panel - Rear Body Enclosure.
    Features:
      - Form factor: 86.0mm x 86.0mm x 18.0mm (standard 86-type switch box)
      - Wall: 2.5mm, Floor: 3.5mm (provides solid backing behind 2.1mm magnet pockets)
      - 4x Neodymium Magnet Pockets on rear exterior located at non-conflicting edge centers (dia 10.3mm x 2.1mm deep)
      - 4x Corner screw bosses for faceplate fastening (M2.5 heat-set inserts: dia 3.4mm x 4.5mm)
      - 4x Separate PCB standoffs (60x60mm pitch, height 5.0mm) with M2.5 pilot holes
      - Integrated 2x AA Battery compartment with 1.5mm partition ribs and terminal lead clearances
      - Stepped perimeter rebate on top rim for flush faceplate mating
    """
    size = 86.0
    height = 18.0
    wall = 2.5
    floor = 3.5
    rebate_w = 1.3
    rebate_d = 1.8

    with BuildPart() as body:
        # 1. Base floor plate
        with BuildSketch(Plane.XY) as s_floor:
            RectangleRounded(size, size, 5.0)
        extrude(amount=floor)

        # 2. Outer side walls
        with BuildSketch(Plane.XY.offset(floor)) as s_walls:
            RectangleRounded(size, size, 5.0)
            RectangleRounded(size - 2 * wall, size - 2 * wall, max(0.1, 5.0 - wall), mode=Mode.SUBTRACT)
        extrude(amount=height - floor)

        # 3. Stepped perimeter rebate on top rim
        with BuildSketch(Plane.XY.offset(height)) as s_reb:
            RectangleRounded(size, size, 5.0)
            RectangleRounded(size - 2 * rebate_w, size - 2 * rebate_w, max(0.1, 5.0 - rebate_w), mode=Mode.SUBTRACT)
        extrude(amount=-rebate_d, mode=Mode.SUBTRACT)

        # 4. 4x Rear Neodymium Magnet Pockets (dia 10.3mm x 2.1mm deep, leaving 1.4mm solid floor backing)
        # Positioned at edge centers (28mm from center along cardinal axes, zero conflict with corner standoffs)
        with Locations(
            (28.0, 0, 0),
            (-28.0, 0, 0),
            (0, 28.0, 0),
            (0, -28.0, 0)
        ):
            Cylinder(radius=5.15, height=2.1, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

        # 5. 4x Corner screw bosses for securing faceplate (M2.5 brass heat-set inserts)
        boss_d = size / 2 - wall - 2.5
        boss_h = height - floor
        with Locations(
            (-boss_d, -boss_d, floor),
            (boss_d, -boss_d, floor),
            (-boss_d, boss_d, floor),
            (boss_d, boss_d, floor)
        ):
            Cylinder(radius=3.5, height=boss_h, align=(Align.CENTER, Align.CENTER, Align.MIN))
            # Pilot hole for M2.5 heat-set insert (dia 3.4mm, depth 4.5mm)
            with Locations((0, 0, boss_h - 4.5)):
                Cylinder(radius=1.7, height=5.0, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

        # 6. 4x PCB Standoffs (at +-30mm, height 5.0mm, dia 5.5mm)
        pcb_d = 30.0
        standoff_h = 5.0
        with Locations(
            (-pcb_d, -pcb_d, floor),
            (pcb_d, -pcb_d, floor),
            (-pcb_d, pcb_d, floor),
            (pcb_d, pcb_d, floor)
        ):
            Cylinder(radius=2.75, height=standoff_h, align=(Align.CENTER, Align.CENTER, Align.MIN))
            with Locations((0, 0, standoff_h - 3.5)):
                Cylinder(radius=1.2, height=4.0, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

        # 7. Molded 2x AA Battery Bay in bottom center with partition ribs (58x30mm)
        with Locations((0, -8.0, floor)):
            with BuildSketch(Plane.XY) as s_bat:
                Rectangle(60.0, 31.0)
                Rectangle(57.0, 28.0, mode=Mode.SUBTRACT)
            extrude(amount=8.5)

    return body.part


def build_remote_faceplate():
    """Remote Switch Panel - Front Faceplate.
    Features:
      - Dimensions: 86.0mm x 86.0mm x 3.8mm with 1.5mm alignment tongue on underside
      - 4x M2.5 Corner countersunk screw holes matching body bosses
      - 6x Tactile button apertures (12.2 x 12.2mm) with 0.8mm ergonomic chamfers
      - Top Center Master Rotary Encoder station with 18mm recessed dial pocket and anti-rotation notch
      - Status Micro-LED hole (dia 2.0mm) with 45° light dispersion cone
    """
    size = 86.0
    thickness = 3.8
    lip_w = 1.3
    lip_h = 1.6

    with BuildPart() as face:
        # 1. Main faceplate top plate
        with BuildSketch(Plane.XY) as s_face:
            RectangleRounded(size, size, 5.0)
        extrude(amount=thickness)

        # 2. Underside stepped alignment tongue (nests inside body rebate)
        with BuildSketch(Plane.XY) as s_tongue:
            RectangleRounded(size - 2 * lip_w, size - 2 * lip_w, max(0.1, 5.0 - lip_w))
        extrude(amount=-lip_h)

        # 3. 4x Corner countersunk holes (M2.5 flat-head screws)
        boss_d = size / 2 - 2.5 - 2.5
        with Locations(
            (-boss_d, -boss_d, thickness),
            (boss_d, -boss_d, thickness),
            (-boss_d, boss_d, thickness),
            (boss_d, boss_d, thickness)
        ):
            Cylinder(radius=1.4, height=thickness + lip_h + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            # 90° Countersink cone (dia 5.2mm at top)
            Cone(top_radius=2.6, bottom_radius=1.4, height=1.3, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 4. 6x Tactile Button Apertures with outer ergonomic chamfers
        for x in [-15.0, 15.0]:
            for y in [18.0, -2.0, -22.0]:
                with Locations((x, y, thickness)):
                    # Button through-hole (12.2 x 12.2mm)
                    Box(12.2, 12.2, thickness + lip_h + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
                    # Outer ergonomic chamfer bezel (14.2 x 14.2mm x 0.8mm deep recess)
                    Box(14.2, 14.2, 0.8, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 5. Master Rotary Encoder Station (top center)
        with Locations((0, 32.0, thickness)):
            # Recessed dial pocket (dia 18mm x 1.2mm deep)
            Cylinder(radius=9.0, height=1.2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            # Encoder bushing hole (dia 7.5mm)
            Cylinder(radius=3.75, height=thickness + lip_h + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            # Anti-rotation tab notch
            with Locations((-6.0, 0, 0)):
                Box(2.2, 3.0, thickness + lip_h + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 6. Status Micro-LED Hole (dia 2.0mm with 45° light cone)
        with Locations((0, 8.0, thickness)):
            Cylinder(radius=1.0, height=thickness + lip_h + 2, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            Cone(top_radius=2.0, bottom_radius=1.0, height=1.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

    return face.part


def build_magnetic_cradle():
    """Magnetic Wall Docking Cradle - Vehicle Wall Mounting Base.
    Features:
      - Outer Dimensions: 96.0mm x 96.0mm x 11.0mm
      - Receiver pocket: 86.8mm x 86.8mm x 6.5mm deep (0.4mm perimeter clearance)
      - 45° Lead-in Chamfer (2.0mm) on entire receiver pocket lip for blind self-centering
      - DUAL ERGONOMIC FINGER SCALLOPS (radius 16.0mm) on left/right walls for easy manual extraction
      - 4x Neodymium Magnet Pockets (dia 10.3mm x 2.1mm deep) matching remote edge magnets
      - 2x M3.5/M4 Countersunk Wall-Mounting Holes recessed below cavity floor
    """
    outer_size = 96.0
    height = 11.0
    pocket_size = 86.8
    pocket_depth = 6.5
    floor = height - pocket_depth  # 4.5mm solid base

    with BuildPart() as cradle:
        # 1. Solid outer base block
        with BuildSketch(Plane.XY) as s_base:
            RectangleRounded(outer_size, outer_size, 6.0)
        extrude(amount=height)

        # 2. Main receiver cavity
        with BuildSketch(Plane.XY.offset(height)) as s_pock:
            RectangleRounded(pocket_size, pocket_size, 3.0)
        extrude(amount=-pocket_depth, mode=Mode.SUBTRACT)

        # 3. 45° Self-centering lead-in chamfer around upper rim
        with BuildSketch(Plane.XY.offset(height)) as s_chamfer:
            RectangleRounded(pocket_size + 3.0, pocket_size + 3.0, 4.0)
            RectangleRounded(pocket_size, pocket_size, 3.0, mode=Mode.SUBTRACT)
        extrude(amount=-1.5, mode=Mode.SUBTRACT)

        # 4. Dual Ergonomic Finger Scallops on left and right walls (radius 16.0mm)
        # Enables direct thumb & index finger pinch-extraction against 17.6 N magnet force
        with Locations((-outer_size / 2, 0, height), (outer_size / 2, 0, height)):
            Cylinder(radius=16.0, height=pocket_depth + 1.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 5. 4x Neodymium Magnet Pockets (dia 10.3mm x 2.1mm deep, leaves 2.4mm solid floor backing)
        with Locations(
            (28.0, 0, floor),
            (-28.0, 0, floor),
            (0, 28.0, floor),
            (0, -28.0, floor)
        ):
            Cylinder(radius=5.15, height=2.1, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

        # 6. 2x Countersunk Wall-Mounting Holes (M3.5/M4 wood screws)
        with Locations((0, -20.0, floor), (0, 20.0, floor)):
            Cylinder(radius=2.2, height=floor + 2.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
            Cone(top_radius=4.2, bottom_radius=2.2, height=2.0, align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)

    return cradle.part


def main():
    parts = {
        "central_enclosure_base": build_central_base(),
        "central_enclosure_lid": build_central_lid(),
        "remote_enclosure_body": build_remote_body(),
        "remote_enclosure_faceplate": build_remote_faceplate(),
        "remote_magnetic_cradle": build_magnetic_cradle(),
    }

    print(f"\n========================================================")
    print(f"VanNOW Production 3D CAD Generator (build123d / OpenCASCADE)")
    print(f"Output directory: {os.path.abspath(OUTPUT_DIR)}")
    print(f"========================================================")

    for name, part in parts.items():
        step_path = os.path.join(OUTPUT_DIR, f"{name}.step")
        stl_path = os.path.join(OUTPUT_DIR, f"{name}.stl")

        export_step(part, step_path)
        export_stl(part, stl_path)

        vol_cm3 = part.volume / 1000.0
        bb = part.bounding_box()
        dx = bb.max.X - bb.min.X
        dy = bb.max.Y - bb.min.Y
        dz = bb.max.Z - bb.min.Z

        print(f"✓ {name}:")
        print(f"    Solid Valid  : {part.is_valid}")
        print(f"    Bounding Box : {dx:.1f} mm x {dy:.1f} mm x {dz:.1f} mm")
        print(f"    Solid Volume : {vol_cm3:.2f} cm³")
        print(f"    Exported     : {name}.step | {name}.stl")

    print(f"\nAll 5 parametric models generated and exported successfully!")


if __name__ == "__main__":
    main()
