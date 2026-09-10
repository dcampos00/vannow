#!/usr/bin/env python3
"""
VanNOW Electronic PCB Renderer & CAD Exporter
Automates generating photorealistic 3D raytraced renders, 2D vector silkscreen plots,
and populated 3D STEP CAD models from KiCad board layouts.

Supports both host-installed kicad-cli and Flatpak (org.kicad.KiCad).

Usage:
    python3 render_pcb.py [--board profet|modular] [--all]
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

def find_project_root(start_dir):
    d = os.path.abspath(start_dir)
    while d and d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, ".git")) or os.path.isdir(os.path.join(d, ".jj")):
            return d
        d = os.path.dirname(d)
    return os.path.abspath(os.path.join(start_dir, "..", ".."))

PROJECT_ROOT = find_project_root(SCRIPT_DIR)
DEFAULT_LAYOUT_DIR = os.path.join(PROJECT_ROOT, "hardware", "central-pcb", "layouts")
DEFAULT_OUT_DIR = os.path.join(PROJECT_ROOT, "hardware", "central-pcb", "renders")


def find_kicad_cli():
    """Detect KiCad CLI either in native PATH or via Flatpak."""
    # 1. Native CLI
    native = shutil.which("kicad-cli")
    if native:
        return [native]

    # 2. Flatpak org.kicad.KiCad
    flatpak = shutil.which("flatpak")
    if flatpak:
        # Check if org.kicad.KiCad is installed
        res = subprocess.run(
            [flatpak, "run", "--command=kicad-cli", "org.kicad.KiCad", "--version"],
            capture_output=True,
            text=True,
        )
        if res.returncode == 0:
            return [flatpak, "run", "--command=kicad-cli", "org.kicad.KiCad"]

    return None


def run_command(cmd, desc=""):
    """Execute command and stream output or raise on failure."""
    if desc:
        print(f"  -> {desc}...")
    t0 = time.time()
    res = subprocess.run(cmd, capture_output=True, text=True)
    dt = time.time() - t0
    if res.returncode != 0:
        print(f"     [ERROR] Command failed ({dt:.2f}s): {' '.join(cmd)}")
        if res.stderr:
            print(f"     Stderr: {res.stderr.strip()}")
        if res.stdout:
            print(f"     Stdout: {res.stdout.strip()}")
        return False
    print(f"     [OK] Finished in {dt:.2f}s")
    return True


def render_board(kicad_cmd, board_path, out_dir, width=2560, height=1440):
    """Generate all render views and CAD models for a PCB file."""
    os.makedirs(out_dir, exist_ok=True)
    board_name = os.path.splitext(os.path.basename(board_path))[0]
    print(f"\n========================================================")
    print(f" Rendering PCB: {board_name} ({board_path})")
    print(f" Target Output: {out_dir}")
    print(f"========================================================")

    # 1. 3D Raytraced Isometric View with Floor & Shadows
    iso_png = os.path.join(out_dir, f"{board_name}_isometric.png")
    run_command(
        kicad_cmd + [
            "pcb", "render",
            "--output", iso_png,
            "--rotate", "-50,0,40",
            "--perspective",
            "--floor",
            "--use-board-stackup-colors",
            "--quality", "high",
            "--width", str(width),
            "--height", str(height),
            board_path,
        ],
        desc="1. 3D Raytraced Isometric Perspective (with floor shadows)",
    )

    # 2. 3D Orthographic Top View
    top_png = os.path.join(out_dir, f"{board_name}_top.png")
    run_command(
        kicad_cmd + [
            "pcb", "render",
            "--output", top_png,
            "--side", "top",
            "--use-board-stackup-colors",
            "--quality", "high",
            "--width", str(width),
            "--height", str(height),
            board_path,
        ],
        desc="2. 3D Orthographic Top View",
    )

    # 3. 3D Orthographic Bottom View
    bottom_png = os.path.join(out_dir, f"{board_name}_bottom.png")
    run_command(
        kicad_cmd + [
            "pcb", "render",
            "--output", bottom_png,
            "--side", "bottom",
            "--use-board-stackup-colors",
            "--quality", "high",
            "--width", str(width),
            "--height", str(height),
            board_path,
        ],
        desc="3. 3D Orthographic Bottom View",
    )

    # 4. 2D Vector Silkscreen & Layout SVG
    svg_path = os.path.join(out_dir, f"{board_name}_silkscreen.svg")
    run_command(
        kicad_cmd + [
            "pcb", "export", "svg",
            "--mode-single",
            "--layers", "F.Cu,F.SilkS,F.Mask,Edge.Cuts",
            "--exclude-drawing-sheet",
            "--page-size-mode", "2",
            "-o", svg_path,
            board_path,
        ],
        desc="4. 2D Vector Silkscreen & Layout SVG",
    )

    # 5. Convert SVG to High-Resolution PNG if ImageMagick is available
    magick = shutil.which("magick")
    if magick and os.path.isfile(svg_path):
        silkscreen_png = os.path.join(out_dir, f"{board_name}_silkscreen.png")
        run_command(
            [magick, "-density", "300", svg_path, silkscreen_png],
            desc="5. High-Resolution 2D Silkscreen PNG",
        )

    # 6. Populated 3D STEP Assembly (for Enclosure CAD Fitting)
    step_path = os.path.join(out_dir, f"{board_name}.step")
    run_command(
        kicad_cmd + [
            "pcb", "export", "step",
            "--output", step_path,
            "--subst-models",
            board_path,
        ],
        desc="6. Populated 3D STEP Assembly (CAD Model)",
    )

    print(f"\n[SUCCESS] Completed renders for {board_name}!\n")


def main():
    parser = argparse.ArgumentParser(description="VanNOW PCB Renderer & CAD Exporter")
    parser.add_argument(
        "--board",
        choices=["profet", "modular", "all"],
        default="profet",
        help="Board layout to render (default: profet)",
    )
    parser.add_argument(
        "--custom",
        type=str,
        default="",
        help="Path to custom .kicad_pcb file",
    )
    parser.add_argument(
        "--outdir",
        type=str,
        default=DEFAULT_OUT_DIR,
        help=f"Output directory (default: {DEFAULT_OUT_DIR})",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=2560,
        help="Image width in pixels (default: 2560)",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=1440,
        help="Image height in pixels (default: 1440)",
    )

    args = parser.parse_args()

    kicad_cmd = find_kicad_cli()
    if not kicad_cmd:
        print("[FATAL] KiCad CLI was not found in PATH or Flatpak.")
        print("Please install KiCad or org.kicad.KiCad Flatpak.")
        sys.exit(1)

    print(f"[INFO] Using KiCad CLI command: {' '.join(kicad_cmd)}")

    if args.custom:
        if not os.path.isfile(args.custom):
            print(f"[FATAL] Board file not found: {args.custom}")
            sys.exit(1)
        render_board(kicad_cmd, args.custom, args.outdir, args.width, args.height)
        return

    boards_to_render = []
    if args.board in ["profet", "all"]:
        boards_to_render.append(os.path.join(DEFAULT_LAYOUT_DIR, "profet", "profet.kicad_pcb"))
    if args.board in ["modular", "all"]:
        boards_to_render.append(os.path.join(DEFAULT_LAYOUT_DIR, "modular", "modular.kicad_pcb"))

    for b in boards_to_render:
        if not os.path.isfile(b):
            print(f"[WARNING] Layout file not found: {b}, skipping...")
            continue
        render_board(kicad_cmd, b, args.outdir, args.width, args.height)


if __name__ == "__main__":
    main()
