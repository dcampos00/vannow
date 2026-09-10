#!/usr/bin/env python3
"""
VanNOW Parametric 3D Enclosure Generator
Uses build123d (Python Code-as-CAD on OpenCASCADE) to model and export
all enclosures and cradles for the VanNOW project.

Outputs:
  - hardware/enclosures/models/*.step (STEP for CAD/CNC/Injection Molding)
  - hardware/enclosures/models/*.stl  (STL for 3D Printing / Slicers)
"""

import os
from build123d import *

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "..", "models")
os.makedirs(OUTPUT_DIR, exist_ok=True)


def build_central_base():
    """Central Controller Cabinet Enclosure - Base with PCB standoffs, mounting ears, and cable ports."""
    length = 175.0  # X
    width = 125.0   # Y
    height = 45.0   # Z
    wall = 3.0
    floor = 3.0

    with BuildPart() as base:
        # Main solid box with rounded vertical corners
        Box(length, width, height)
        fillet(base.edges().filter_by(Axis.Z), 6.0)

        # Hollow interior
        offset(amount=-wall, openings=base.faces().sort_by(Axis.Z)[-1])

        # Add exterior mounting ears (left and right)
        with Locations((-length / 2 - 12.0, 0, -height / 2 + floor / 2),
                       (length / 2 + 12.0, 0, -height / 2 + floor / 2)):
            Box(24.0, 50.0, floor)
            fillet(base.edges().filter_by(Axis.Z), 4.0)

        # Mounting holes in ears (dia 4.5 mm for M4 screws)
        with Locations((-length / 2 - 14.0, 0, -height / 2 + floor / 2),
                       (length / 2 + 14.0, 0, -height / 2 + floor / 2)):
            Cylinder(radius=2.25, height=floor * 2, mode=Mode.SUBTRACT)

        # 4x Internal PCB Standoff Bosses (for 150x95 mm PCB, hole spacing 140x85 mm)
        pcb_dx = 140.0 / 2
        pcb_dy = 85.0 / 2
        standoff_h = 15.0
        with Locations(
            (-pcb_dx, -pcb_dy, -height / 2 + floor + standoff_h / 2),
            (pcb_dx, -pcb_dy, -height / 2 + floor + standoff_h / 2),
            (-pcb_dx, pcb_dy, -height / 2 + floor + standoff_h / 2),
            (pcb_dx, pcb_dy, -height / 2 + floor + standoff_h / 2)
        ):
            Cylinder(radius=4.5, height=standoff_h)
            # M3 pilot holes (dia 2.6 mm for brass heat-set inserts)
            Cylinder(radius=1.3, height=standoff_h + 2, mode=Mode.SUBTRACT)

        # 4x Corner screw bosses for lid fastening (M3)
        corner_dx = length / 2 - wall - 4.5
        corner_dy = width / 2 - wall - 4.5
        boss_h = height - floor
        with Locations(
            (-corner_dx, -corner_dy, -height / 2 + floor + boss_h / 2),
            (corner_dx, -corner_dy, -height / 2 + floor + boss_h / 2),
            (-corner_dx, corner_dy, -height / 2 + floor + boss_h / 2),
            (corner_dx, corner_dy, -height / 2 + floor + boss_h / 2)
        ):
            Cylinder(radius=4.5, height=boss_h)
            Cylinder(radius=1.3, height=boss_h + 2, mode=Mode.SUBTRACT)

        # 4x Bottom Cable Gland Ports (dia 16.0 mm for PG9/PG11 glands)
        for i, x_offset in enumerate([-45.0, -15.0, 15.0, 45.0]):
            with Locations((x_offset, -width / 2, -height / 2 + 18.0)):
                with BuildSketch(Plane.XZ):
                    Circle(radius=8.0)
                extrude(amount=-wall * 2, mode=Mode.SUBTRACT)

    return base.part


def build_central_lid():
    """Central Controller Cabinet Enclosure - Lid with 5x SPDT toggle switch holes."""
    length = 175.0
    width = 125.0
    height = 20.0
    wall = 2.5

    with BuildPart() as lid:
        Box(length, width, height)
        fillet(lid.edges().filter_by(Axis.Z), 6.0)
        # Hollow interior opening downwards
        offset(amount=-wall, openings=lid.faces().sort_by(Axis.Z)[0])

        # 5x SPDT Toggle Switch Mounting Holes (dia 6.5 mm for 1/4" bushing, spaced 22 mm apart)
        for i, x_pos in enumerate([-44.0, -22.0, 0.0, 22.0, 44.0]):
            with Locations((x_pos, 0, height / 2)):
                Cylinder(radius=3.25, height=wall * 3, mode=Mode.SUBTRACT)

        # 4x Corner screw counterbored holes for M3 lid screws
        corner_dx = length / 2 - 3.0 - 4.5
        corner_dy = width / 2 - 3.0 - 4.5
        with Locations(
            (-corner_dx, -corner_dy, height / 2),
            (corner_dx, -corner_dy, height / 2),
            (-corner_dx, corner_dy, height / 2),
            (corner_dx, corner_dy, height / 2)
        ):
            # Through hole (dia 3.4 mm for M3)
            Cylinder(radius=1.7, height=wall * 4, mode=Mode.SUBTRACT)
            # Counterbore (dia 6.0 mm, depth 3.0 mm)
            with Locations((0, 0, -1.5)):
                Cylinder(radius=3.0, height=3.0, mode=Mode.SUBTRACT)

    return lid.part


def build_remote_body():
    """Remote Switch Panel - Rear Body with Battery Compartment and Magnet Pockets."""
    size = 86.0
    height = 18.0
    wall = 2.0
    floor = 2.0

    with BuildPart() as body:
        Box(size, size, height)
        fillet(body.edges().filter_by(Axis.Z), 4.0)
        offset(amount=-wall, openings=body.faces().sort_by(Axis.Z)[-1])

        # Battery Cavity for 2x AA (58 mm x 32 mm x 14 mm deep)
        with Locations((0, -8.0, -height / 2 + floor + 7.0)):
            Box(58.0, 32.0, 14.0, mode=Mode.SUBTRACT)

        # 4x PCB Standoffs (for 76x76 mm carrier PCB, spacing 68x68 mm)
        pcb_d = 68.0 / 2
        standoff_h = 6.0
        with Locations(
            (-pcb_d, -pcb_d, -height / 2 + floor + standoff_h / 2),
            (pcb_d, -pcb_d, -height / 2 + floor + standoff_h / 2),
            (-pcb_d, pcb_d, -height / 2 + floor + standoff_h / 2),
            (pcb_d, pcb_d, -height / 2 + floor + standoff_h / 2)
        ):
            Cylinder(radius=3.0, height=standoff_h)
            Cylinder(radius=1.1, height=standoff_h + 2, mode=Mode.SUBTRACT)

        # 4x Neodymium Magnet Pockets on rear exterior (dia 10.2 mm x 2.2 mm deep)
        mag_d = 32.0
        with Locations(
            (-mag_d, -mag_d, -height / 2),
            (mag_d, -mag_d, -height / 2),
            (-mag_d, mag_d, -height / 2),
            (mag_d, mag_d, -height / 2)
        ):
            Cylinder(radius=5.1, height=2.2 * 2, mode=Mode.SUBTRACT)

    return body.part


def build_remote_faceplate():
    """Remote Switch Panel - Faceplate with 7 Tactile Button Openings and Micro-LED."""
    size = 86.0
    thickness = 4.0

    with BuildPart() as face:
        Box(size, size, thickness)
        fillet(face.edges().filter_by(Axis.Z), 4.0)

        # 6-Button Grid (2 columns x 3 rows, 14x14 mm buttons with 22 mm pitch)
        col_x = [-14.0, 14.0]
        row_y = [22.0, 0.0, -22.0]
        for x in col_x:
            for y in row_y:
                with Locations((x, y, 0)):
                    Box(12.0, 12.0, thickness * 2, mode=Mode.SUBTRACT)

        # 7th Central / Master Button (Top center or Bottom center)
        with Locations((0, 33.0, 0)):
            Cylinder(radius=4.5, height=thickness * 2, mode=Mode.SUBTRACT)

        # Status Micro-LED hole (dia 1.6 mm for 0805 light pipe)
        with Locations((0, 11.0, 0)):
            Cylinder(radius=0.8, height=thickness * 2, mode=Mode.SUBTRACT)

    return face.part


def build_magnetic_cradle():
    """Magnetic Wall Docking Cradle - Self-aligning mounting base for sliding door."""
    outer_size = 92.0
    height = 9.0
    wall = 2.5

    with BuildPart() as cradle:
        # Base plate with rounded corners
        Box(outer_size, outer_size, height)
        fillet(cradle.edges().filter_by(Axis.Z), 6.0)

        # Recessed receiver pocket (86.8 mm x 86.8 mm x 6.0 mm deep for 86 mm remote)
        with Locations((0, 0, height / 2 - 3.0)):
            Box(86.8, 86.8, 6.5, mode=Mode.SUBTRACT)

        # 4x Neodymium Magnet Pockets (dia 10.2 mm x 2.2 mm deep) matching remote
        mag_d = 32.0
        with Locations(
            (-mag_d, -mag_d, height / 2 - 6.0),
            (mag_d, -mag_d, height / 2 - 6.0),
            (-mag_d, mag_d, height / 2 - 6.0),
            (mag_d, mag_d, height / 2 - 6.0)
        ):
            Cylinder(radius=5.1, height=2.2 * 2, mode=Mode.SUBTRACT)

        # 2x Wall Mounting Countersunk Screw Holes (M3.5/M4 wood screws)
        with Locations((0, -24.0, -height / 2), (0, 24.0, -height / 2)):
            Cylinder(radius=2.1, height=height * 2, mode=Mode.SUBTRACT)
            with Locations((0, 0, height / 2 - 1.0)):
                Cone(bottom_radius=4.2, top_radius=2.1, height=2.5, mode=Mode.SUBTRACT)

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
    print(f"VanNOW 3D CAD Generator (build123d / OpenCASCADE)")
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
        print(f"    Bounding Box : {dx:.1f} mm x {dy:.1f} mm x {dz:.1f} mm")
        print(f"    Solid Volume : {vol_cm3:.2f} cm³")
        print(f"    Exported     : {name}.step | {name}.stl")

    print(f"\nAll 5 parametric models generated and exported successfully!")


if __name__ == "__main__":
    main()
