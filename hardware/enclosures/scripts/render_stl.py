#!/usr/bin/env python3
"""
VanNOW 3D CAD Mesh Renderer & Verification Sheet Generator
High-performance software Z-buffer rasterizer with three-point Blinn-Phong lighting
and CAD feature crease edge detection. Eliminates Matplotlib 3D sorting and transparency artifacts.
"""

import os
import glob
import numpy as np
import trimesh
from PIL import Image, ImageDraw, ImageFont

MODELS_DIR = os.path.join(os.path.dirname(__file__), "..", "models")
RENDERS_DIR = os.path.join(os.path.dirname(__file__), "..", "renders")
os.makedirs(RENDERS_DIR, exist_ok=True)


def render_viewport(mesh, width, height, elev_deg, azim_deg, color_rgb=(0.32, 0.68, 0.92)):
    """Renders an STL mesh to a solid, depth-buffered PIL Image using pure NumPy software rasterization."""
    elev = np.radians(elev_deg)
    azim = np.radians(azim_deg)

    # Camera rotation matrices
    R_z = np.array([
        [np.cos(azim), -np.sin(azim), 0],
        [np.sin(azim), np.cos(azim), 0],
        [0, 0, 1]
    ])
    R_x = np.array([
        [1, 0, 0],
        [0, np.cos(elev), -np.sin(elev)],
        [0, np.sin(elev), np.cos(elev)]
    ])
    R = R_x @ R_z

    # Center vertices
    vertices = np.array(mesh.vertices) - np.array(mesh.bounds).mean(axis=0)
    v_cam = (R @ vertices.T).T

    # Fit within viewport margins (14% margin)
    span = (v_cam[:, :2].max(axis=0) - v_cam[:, :2].min(axis=0)).max()
    scale = 0.82 * min(width, height) / max(span, 1e-4)

    v_screen = np.zeros_like(v_cam)
    v_screen[:, 0] = v_cam[:, 0] * scale + width / 2.0
    v_screen[:, 1] = -v_cam[:, 1] * scale + height / 2.0  # invert Y for image coordinate space
    v_screen[:, 2] = -v_cam[:, 2]  # depth (more negative = closer)

    # Backface culling
    triangles = v_screen[mesh.faces]
    e1 = triangles[:, 1, :2] - triangles[:, 0, :2]
    e2 = triangles[:, 2, :2] - triangles[:, 0, :2]
    cross2d = e1[:, 0] * e2[:, 1] - e1[:, 1] * e2[:, 0]
    visible = cross2d < 0
    vis_tris = triangles[visible]
    vis_normals = (R @ np.array(mesh.face_normals).T).T[visible]

    # Three-point lighting in camera space (positive Z points towards camera/viewer)
    key_light = np.array([0.45, -0.35, 0.82])
    key_light /= np.linalg.norm(key_light)

    fill_light = np.array([-0.50, 0.40, 0.75])
    fill_light /= np.linalg.norm(fill_light)

    view_vec = np.array([0.0, 0.0, 1.0])
    half_vec = key_light + view_vec
    half_vec /= np.linalg.norm(half_vec)

    ambient = 0.28
    diffuse = 0.58 * np.clip(np.dot(vis_normals, key_light), 0.0, 1.0)
    fill = 0.20 * np.clip(np.dot(vis_normals, fill_light), 0.0, 1.0)
    specular = 0.22 * (np.clip(np.dot(vis_normals, half_vec), 0.0, 1.0) ** 20)

    intensity = ambient + diffuse + fill + specular
    base_color = np.array(color_rgb)
    tri_colors = (base_color[None, :] * intensity[:, None]).clip(0, 1)

    # Z-buffer and image buffer
    z_buffer = np.full((height, width), np.inf, dtype=np.float32)
    img = np.full((height, width, 3), [18, 19, 24], dtype=np.uint8)

    for i in range(len(vis_tris)):
        tri = vis_tris[i]
        color = (tri_colors[i] * 255).astype(np.uint8)

        min_x = max(0, int(np.floor(tri[:, 0].min())))
        max_x = min(width - 1, int(np.ceil(tri[:, 0].max())))
        min_y = max(0, int(np.floor(tri[:, 1].min())))
        max_y = min(height - 1, int(np.ceil(tri[:, 1].max())))

        if min_x > max_x or min_y > max_y:
            continue

        p0, p1, p2 = tri[0], tri[1], tri[2]
        denom = (p1[1] - p2[1]) * (p0[0] - p2[0]) + (p2[0] - p1[0]) * (p0[1] - p2[1])
        if abs(denom) < 1e-6:
            continue

        y, x = np.ogrid[min_y:max_y + 1, min_x:max_x + 1]
        w0 = ((p1[1] - p2[1]) * (x - p2[0]) + (p2[0] - p1[0]) * (y - p2[1])) / denom
        w1 = ((p2[1] - p0[1]) * (x - p2[0]) + (p0[0] - p2[0]) * (y - p2[1])) / denom
        w2 = 1.0 - w0 - w1

        mask = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not np.any(mask):
            continue

        z = w0 * p0[2] + w1 * p1[2] + w2 * p2[2]

        z_sub = z_buffer[min_y:max_y + 1, min_x:max_x + 1]
        closer = mask & (z < z_sub)
        z_sub[closer] = z[closer]
        img[min_y:max_y + 1, min_x:max_x + 1][closer] = color

    # Draw CAD feature crease edges (sharp creases > 30 deg)
    pil_img = Image.fromarray(img)
    draw = ImageDraw.Draw(pil_img)

    crease_mask = mesh.face_adjacency_angles > np.radians(30)
    crease_edges = mesh.face_adjacency_edges[crease_mask]

    for edge in crease_edges:
        pA = v_screen[edge[0]]
        pB = v_screen[edge[1]]
        xA, yA, zA = pA[0], pA[1], pA[2]
        xB, yB, zB = pB[0], pB[1], pB[2]

        xm = int(np.clip((xA + xB) / 2.0, 0, width - 1))
        ym = int(np.clip((yA + yB) / 2.0, 0, height - 1))
        zm = (zA + zB) / 2.0

        # Depth test threshold against Z-buffer to avoid drawing hidden rear edges
        if zm <= z_buffer[ym, xm] + 1.8:
            draw.line([(xA, yA), (xB, yB)], fill=(10, 14, 22), width=1)

    return pil_img


def render_verification_sheet(stl_path, output_png):
    mesh = trimesh.load_mesh(stl_path)
    base_name = os.path.basename(stl_path).replace(".stl", "")

    sheet_w = 1600
    sheet_h = 800
    header_h = 70
    view_w = sheet_w // 2
    view_h = sheet_h - header_h

    # Color mapping per part
    if "cradle" in base_name:
        part_color = (0.35, 0.75, 0.88)
        view_a_cfg = {"elev": 35, "azim": -45, "lbl": "VIEW A: ISOMETRIC 3D (FINGER SCALLOPS & CHAMFER)"}
        view_b_cfg = {"elev": 65, "azim": -30, "lbl": "VIEW B: CAVITY TOP DETAIL (MAGNET & MOUNT POCKETS)"}
    elif "lid" in base_name:
        part_color = (0.45, 0.78, 0.96)
        view_a_cfg = {"elev": 35, "azim": -45, "lbl": "VIEW A: TOP EXTERIOR (SPDT SWITCH BEZELS & KEYWAYS)"}
        view_b_cfg = {"elev": -45, "azim": -45, "lbl": "VIEW B: UNDERSIDE DETAIL (MATING GROOVE & BOSS COLUMNS)"}
    elif "faceplate" in base_name:
        part_color = (0.45, 0.78, 0.96)
        view_a_cfg = {"elev": 35, "azim": -45, "lbl": "VIEW A: TOP FRONT (BUTTON BEZELS, ENCODER & LED CONE)"}
        view_b_cfg = {"elev": -45, "azim": -45, "lbl": "VIEW B: UNDERSIDE (MATING TONGUE & COUNTERSINKS)"}
    elif "body" in base_name:
        part_color = (0.32, 0.68, 0.92)
        view_a_cfg = {"elev": 35, "azim": -45, "lbl": "VIEW A: INTERIOR (2x AA BATTERY BAY & PCB STANDOFFS)"}
        view_b_cfg = {"elev": -45, "azim": -45, "lbl": "VIEW B: REAR EXTERIOR (NON-COLLIDING MAGNET POCKETS)"}
    else:  # central base
        part_color = (0.32, 0.68, 0.92)
        view_a_cfg = {"elev": 35, "azim": -45, "lbl": "VIEW A: ISOMETRIC 3D (STANDOFFS, LIP & MOUNTING EARS)"}
        view_b_cfg = {"elev": 65, "azim": -30, "lbl": "VIEW B: INTERIOR DETAIL (GLAND PORTS & VENT SLOTS)"}

    # Render viewports
    img_a = render_viewport(mesh, view_w, view_h, elev_deg=view_a_cfg["elev"], azim_deg=view_a_cfg["azim"], color_rgb=part_color)
    img_b = render_viewport(mesh, view_w, view_h, elev_deg=view_b_cfg["elev"], azim_deg=view_b_cfg["azim"], color_rgb=part_color)

    # Master sheet
    sheet = Image.new("RGB", (sheet_w, sheet_h), (20, 20, 26))
    sheet.paste(img_a, (0, header_h))
    sheet.paste(img_b, (view_w, header_h))

    draw = ImageDraw.Draw(sheet)

    # Header banner styling
    draw.rectangle([(0, 0), (sheet_w, header_h)], fill=(14, 15, 19))
    draw.line([(0, header_h), (sheet_w, header_h)], fill=(45, 48, 60), width=1)
    draw.line([(view_w, header_h), (view_w, sheet_h)], fill=(32, 34, 44), width=1)

    # Metrics
    bb = mesh.bounds
    dx = bb[1, 0] - bb[0, 0]
    dy = bb[1, 1] - bb[0, 1]
    dz = bb[1, 2] - bb[0, 2]
    vol_cm3 = mesh.volume / 1000.0 if mesh.is_volume else (mesh.bounding_box.volume / 1000.0)

    title_text = f"VanNOW 3D CAD Production Verification: {base_name}.stl"
    specs_text = f"Envelope: {dx:.1f} x {dy:.1f} x {dz:.1f} mm   |   Volume: {vol_cm3:.2f} cm³   |   Watertight Manifold: {mesh.is_watertight}"

    draw.text((24, 14), title_text, fill=(245, 245, 255))
    draw.text((24, 38), specs_text, fill=(130, 155, 185))

    # Viewport labels
    draw.text((24, header_h + 16), view_a_cfg["lbl"], fill=(90, 180, 245))
    draw.text((view_w + 24, header_h + 16), view_b_cfg["lbl"], fill=(90, 180, 245))

    sheet.save(output_png, quality=95)
    print(f"  ✓ Rendered: {os.path.basename(output_png)}")


def main():
    stl_files = sorted(glob.glob(os.path.join(MODELS_DIR, "*.stl")))
    print(f"\n========================================================")
    print(f"Rendering CAD STL Verification Sheets (Software Z-Buffer)")
    print(f"========================================================")

    for stl in stl_files:
        name = os.path.basename(stl).replace(".stl", "")
        out_png = os.path.join(RENDERS_DIR, f"{name}_render.png")
        render_verification_sheet(stl, out_png)

    print(f"\nAll verification sheets rendered successfully to {RENDERS_DIR}!\n")


if __name__ == "__main__":
    main()

