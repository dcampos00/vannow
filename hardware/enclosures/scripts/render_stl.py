#!/usr/bin/env python3
"""
VanNOW 3D Model STL Visualizer & Screenshot Renderer
Renders actual generated STL mesh geometry to PNG screenshots for design verification.
"""

import os
import glob
import numpy as np
import trimesh
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

MODELS_DIR = os.path.join(os.path.dirname(__file__), "..", "models")
RENDERS_DIR = os.path.join(os.path.dirname(__file__), "..", "renders")
os.makedirs(RENDERS_DIR, exist_ok=True)


def render_stl_views(stl_path, output_png):
    mesh = trimesh.load_mesh(stl_path)
    base_name = os.path.basename(stl_path).replace(".stl", "")

    fig = plt.figure(figsize=(12, 6), facecolor="#1e1e24")

    views = [
        {"title": f"{base_name} (Isometric 3D)", "elev": 30, "azim": -45, "sub": 121},
        {"title": f"{base_name} (Top / Front Angle)", "elev": 60, "azim": -30, "sub": 122},
    ]

    # Calculate face normals and simple directional lighting
    light_dir = np.array([0.5, 0.7, 1.0])
    light_dir /= np.linalg.norm(light_dir)

    normals = mesh.face_normals
    intensity = np.clip(np.dot(normals, light_dir), 0.2, 1.0)

    # Base color: metallic tech cyan/slate
    colors = np.zeros((len(mesh.faces), 4))
    colors[:, 0] = 0.2 + 0.5 * intensity  # R
    colors[:, 1] = 0.5 + 0.4 * intensity  # G
    colors[:, 2] = 0.7 + 0.3 * intensity  # B
    colors[:, 3] = 0.95                    # Alpha

    vertices = mesh.vertices
    triangles = vertices[mesh.faces]

    bounds = mesh.bounds
    max_range = np.array([bounds[1, 0] - bounds[0, 0],
                          bounds[1, 1] - bounds[0, 1],
                          bounds[1, 2] - bounds[0, 2]]).max() / 2.0
    mid = (bounds[0] + bounds[1]) * 0.5

    for v in views:
        ax = fig.add_subplot(v["sub"], projection="3d", facecolor="#1e1e24")
        poly = Poly3DCollection(triangles, facecolors=colors, edgecolors="#103040", linewidths=0.1)
        ax.add_collection3d(poly)

        ax.set_xlim(mid[0] - max_range, mid[0] + max_range)
        ax.set_ylim(mid[1] - max_range, mid[1] + max_range)
        ax.set_zlim(mid[2] - max_range, mid[2] + max_range)

        ax.view_init(elev=v["elev"], azim=v["azim"])
        ax.set_title(v["title"], color="#ffffff", fontsize=11, fontweight="bold", pad=10)

        # Subtle dark grid styling
        ax.xaxis.pane.fill = False
        ax.yaxis.pane.fill = False
        ax.zaxis.pane.fill = False
        ax.xaxis.pane.set_edgecolor("#333340")
        ax.yaxis.pane.set_edgecolor("#333340")
        ax.zaxis.pane.set_edgecolor("#333340")
        ax.tick_params(colors="#888899", labelsize=8)
        ax.set_xlabel("X (mm)", color="#aaaaaa", fontsize=8)
        ax.set_ylabel("Y (mm)", color="#aaaaaa", fontsize=8)
        ax.set_zlabel("Z (mm)", color="#aaaaaa", fontsize=8)

    plt.tight_layout()
    plt.savefig(output_png, dpi=150, facecolor=fig.get_facecolor(), edgecolor="none")
    plt.close(fig)
    print(f"  ✓ Rendered: {os.path.basename(output_png)}")


def main():
    stl_files = sorted(glob.glob(os.path.join(MODELS_DIR, "*.stl")))
    print(f"\n========================================================")
    print(f"Rendering CAD STL Verification Screenshots")
    print(f"========================================================")

    for stl in stl_files:
        name = os.path.basename(stl).replace(".stl", "")
        out_png = os.path.join(RENDERS_DIR, f"{name}_render.png")
        render_stl_views(stl, out_png)

    print(f"\nAll verification screenshots rendered to {RENDERS_DIR}!\n")


if __name__ == "__main__":
    main()
