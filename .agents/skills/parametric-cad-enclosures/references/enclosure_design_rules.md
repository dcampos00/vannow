# Mechanical Enclosure Design Rules & Sizing Standards

This reference establishes engineering design rules, sizing standards, and clearance requirements for automotive and electronics enclosures fabricated via 3D printing (FDM / SLA) or plastic injection molding.

---

## 1. Wall Thickness Guidelines

| Application | Recommended Wall ($T_{wall}$) | Floor Thickness ($T_{floor}$) | Rationale |
| :--- | :--- | :--- | :--- |
| **Small Handheld Remote (86x86mm)** | $2.5\,\text{mm}$ | $3.5\,\text{mm}$ | Houses $2.1\,\text{mm}$ magnet pockets while preserving $1.4\,\text{mm}$ solid backing. |
| **Central Power Cabinet (175x125mm)** | $3.0\,\text{mm}$ | $3.5\,\text{mm}$ | Structural rigidity under automotive vibration; supports PG cable gland torque. |
| **Docking / Wall Cradle (96x96mm)** | $2.5\,\text{mm}$ | $4.5\,\text{mm}$ | Allows deep countersinks for wall mounting screws flush below pocket floor. |

---

## 2. Fastener Stackup & Counterbore Land Thickness

### The Failure Mode
If a screw counterbore is modeled too deep relative to the wall thickness, the remaining plastic shoulder under the screw head is paper-thin. Under screw torque or vehicle vibration, the screw head punches through the wall, dropping the lid.

```
         ┌───────────────────┐               ▲
         │                   │               │
    ─────┘                   └─────          │ Wall Thickness (3.5 mm)
      │  ▲ Counterbore (2.0mm) │             │
      │  ▼                     │             ▼
    ──┼────────────────────────┼───          ─
      │  Solid Shoulder (1.5mm)│             ▲
    ──┴────────────────────────┴───          ▼
```

### The Rule
$$\text{Clamping Shoulder} = T_{wall} - D_{counterbore} \ge 1.5\,\text{mm}$$

For M3 Socket Head Cap Screws (DIN 912):
* Head Diameter: $5.5\,\text{mm} \implies \text{Counterbore Diameter} = 6.2\,\text{mm}$
* Counterbore Depth: $2.0\,\text{mm}$
* Minimum Top Wall Thickness: $3.5\,\text{mm}$ (leaves $1.5\,\text{mm}$ solid shoulder)
* Through Hole: $3.4\,\text{mm}$ (free fit)

---

## 3. Brass Threaded Heat-Set Insert Sizing

Never screw metal fasteners directly into raw plastic for reusable panels. Heat-set inserts flow plastic into external helical knurls, yielding high pull-out ($>1.2\,\text{kN}$) and torque resistance.

```
         Boss Outer Dia (D_boss)
       ◄─────────────────────────►
       ┌─────┬─────────────┬─────┐
       │     │  Insert ID  │     │
       │     │ ◄── D_pilot ──►   │
       │     │             │     │
       │     │             │     │ ▲ Insert Depth + 1.0mm
       │     │             │     │ ▼
       │     └──────┬──────┘     │
       │            │ Solid Land │ ▲ Min 1.5mm
       └────────────┴────────────┘ ▼
```

* **M3 Standard Insert (Ruthex / McMaster M3x5.7):**
  * $D_{pilot} = 4.0 - 4.1\,\text{mm}$
  * $H_{hole} = 6.0\,\text{mm}$ ($0.3 - 0.5\,\text{mm}$ deeper than insert to capture displaced plastic)
  * $D_{boss} \ge 2.0 \cdot D_{pilot} \approx 8.0 - 8.5\,\text{mm}$
* **M2.5 Small Insert (Ruthex M2.5x4.0):**
  * $D_{pilot} = 3.4 - 3.5\,\text{mm}$
  * $H_{hole} = 4.5\,\text{mm}$
  * $D_{boss} \ge 6.5 - 7.0\,\text{mm}$

---

## 4. Interlocking Stepped Perimeter Joint (Lip-and-Groove)

Flat butt-joints between base and lid allow dust/spray ingress, light bleed, and lateral shifting. Every enclosure assembly must use an interlocking stepped flange:

```
        LID
      ┌─────────────────────────┐
      │  ┌───────────────────┐  │
      │  │                   │  │
      └──┼──────┐     ┌──────┼──┘
         │ 0.2mm│     │0.2mm │
         │ Gap  │     │Gap   │
      ┌──┼──────┘     └──────┼──┐
      │  │                   │  │
      │  └───────────────────┘  │
      └─────────────────────────┘
        BASE
```

* **Tongue Width ($W_{tongue}$):** $1.3 - 1.5\,\text{mm}$
* **Tongue Height ($H_{tongue}$):** $2.0 - 2.5\,\text{mm}$
* **Groove Width ($W_{groove}$):** $W_{tongue} + 2 \cdot \text{Tolerance} \approx 1.7 - 1.9\,\text{mm}$
* **Tolerance Gap:** $0.2\,\text{mm}$ per side for FDM 3D printing; $0.1\,\text{mm}$ for SLA / injection molding.

---

## 5. Ergonomic Magnetic Cradle Design

When mounting dockable remotes using strong NdFeB magnets (e.g. 4x N52 magnets generating $17.6\,\text{N}$ holding force):
1. **The Extraction Problem:** If a flush remote sits in a deep snug pocket with $0.4\,\text{mm}$ clearance, user fingers cannot grip any surface. Extracting the unit requires prying with a screwdriver or blade.
2. **The Solution:**
   * **Dual Finger Scallops:** Two semicircular grip reliefs ($R \ge 14\,\text{mm}$, depth $\ge 6.0\,\text{mm}$) cut through the side walls of the cradle pocket.
   * **Self-Centering Lead-In Chamfer:** A $45^\circ$ lead-in taper ($1.5 - 2.0\,\text{mm}$) around the top rim guides the remote blindly into position as magnetic attraction engages.
