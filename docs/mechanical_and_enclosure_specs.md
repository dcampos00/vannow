# VanNOW: Mechanical Enclosure Specifications, 3D Architecture, and Spatial Budget

This specification defines the physical envelopes, 3D mechanical designs, spatial budgeting, mounting interfaces, and thermal/wire management requirements for the **VanNOW** camper van relay system.

---

## 1. Spatial Budget Summary

```
+-----------------------------------------------------------------------------------------+
|                               VAN INTERIOR SPATIAL BUDGET                               |
+------------------------------------+----------------------------------------------------+
| LOCATION                           | ENVELOPE (W x H x D)      | MOUNTING TYPE          |
+------------------------------------+----------------------------------------------------+
| 1. Main Electrical Cabinet         | 230 mm x 190 mm x 85 mm   | 35mm DIN-Rail / M4 Tab |
| 2. Bed Wall Panel (Fixed)          | 86 mm x 86 mm x 22 mm     | Surface / Semi-Flush   |
| 3. Sliding Door Panel (Dockable)   | 90 mm x 135 mm x 24 mm    | Magnetic Self-Aligning |
+------------------------------------+----------------------------------------------------+
```

---

## 2. Central Electrical Cabinet Enclosure

### 2.1 Dimensions & Clearances

| Component | Width ($X$) | Height ($Y$) | Depth ($Z$) | Notes |
| :--- | :---: | :---: | :---: | :--- |
| **Bare PCB Assembly** | **150 mm** | **95 mm** | **28 mm** | 2-layer FR4, 2 oz copper; includes socketed ESP32-S3 and MP1584EN. |
| **External Enclosure** | **175 mm** | **125 mm** | **65 mm** | Rugged industrial ABS/Polycarbonate housing with clear tinted lid. |
| **Required Installation Envelope** | **230 mm** | **190 mm** | **85 mm** | **Total free clearance needed inside cabinet** for wiring & switches. |

### 2.2 Wire Dress & Bend Radius Requirements
* **Automotive Wire Guages:** 12 AWG (Pump, Aux outlets) and 14 AWG (LED zones, ceiling fan) require a minimum static bend radius:
  $$R_{bend} \ge 4 \times D_{wire} \approx 25\,\text{mm} - 35\,\text{mm}$$
* **Vertical Wiring Clearance:** A dedicated clearance zone of **50 mm beneath the enclosure** is strictly mandated. This ensures that wire bundles routed out of the 5.08 mm pitch pluggable screw terminals exit cleanly into slotted cable ducting (40 mm $\times$ 40 mm) without placing mechanical cantilever strain on PCB solder joints.
* **Cable Ingress:** Bottom wall fitted with 4x **PG9 / PG11 cable glands** or silicone multi-cable pass-through grommets.

### 2.3 Mechanical Redundancy Panel (Front Cover)
* **Toggle Switches:** 5x heavy-duty SPDT miniature toggle switches (**ON-OFF-ON / ON-OFF-AUTO**) mounted through 6.35 mm (1/4") panel holes on the lid.
* **Faston Terminal Clearance:** The 65 mm enclosure depth guarantees a minimum 18 mm air gap between the switch rear crimp terminals and the tallest PCB components, eliminating electrical flashover risks.

### 2.4 Thermal Dissipation
* The 8 Infineon BTS5008-1EKB PROFET smart switches utilize large continuous top and bottom copper pours interconnected with $0.3\,\text{mm}$ thermal vias.
* The PCB must be elevated on **M3 $\times$ 15 mm brass threaded standoffs** to permit convective airflow beneath the board.

---

## 3. Remote Wall Switch Panels (Bed & Living Area)

### 3.1 Dimensions & Form Factor
* **Outer Faceplate:** **86 mm $\times$ 86 mm** (standard European / Berker Integro RV switch plate footprint).
* **Overall Thickness:** **22 mm total depth** (housing the carrier PCB, Seeed Studio XIAO ESP32-C6, and 2x AA alkaline batteries).
* **Weight:** $\approx 110\,\text{g}$ (including batteries).

### 3.2 Mounting Modes
1. **Full Surface Mount (Recommended):**
   * Base plate fixed directly to the plywood wall with 4x countersunk $3.0 \times 12\,\text{mm}$ wood screws or **3M VHB 5952 automotive tape**.
   * Protrudes only 22 mm, preventing snagging in camper hallways.
2. **Semi-Recessed Mount:**
   * For 15 mm plywood wall panels, a shallow blind pocket of **55 mm $\times$ 55 mm $\times$ 12 mm** reduces external projection to just **10 mm**.

---

## 4. Dockable Magnetic Entrance / Porch Remote

### 4.1 Concept & Operational Overview
The sliding door entrance panel is engineered as a **dockable dual-purpose controller**:
* **Docked Mode:** Snaps magnetically into its wall cradle on the B-pillar or kitchen galley side. Functions as a permanent tactile entrance switch plate.
* **Handheld / Outdoor Mode:** Effortlessly detached with one hand to control porch lights, exterior awning lights, water pump, or execute a master shutdown from the campsite table, campfire, or outdoor shower.

```
DOCKABLE MAGNETIC ASSEMBLY (EXPLODED VIEW):
                                               +-------------------------+
                                               | HANDHELD REMOTE UNIT    |
                                               | - Seeed XIAO ESP32-C6   |
                                               | - 7-Button Diode Matrix |
                                               | - 2x AA Battery Bay     |
+--------------------------+                   | - 4x Ferrous Discs      |
| WALL CRADLE (FIXED)      |                   +-------------------------+
| - Screwed to Van Pillar  |        Snap Pull               |
| - 4x Neodymium N52 Discs | <====================>         v
| - Self-Centering Bezel   |        Snap Dock       [ Ergonomic Grip ]
+--------------------------+
```

### 4.2 Magnetic Retention & Vehicular Vibration Physics
* **Vibration Acceleration:** Off-road driving and washboard gravel roads induce accelerations exceeding $3.0\,g$ ($a \approx 30\,\text{m/s}^2$).
* **Retention Force Calculation:**
  For a remote mass $m = 0.12\,\text{kg}$, the inertial detachment force is:
  $$F_{inertial} = m \cdot a = 0.12\,\text{kg} \times 30\,\text{m/s}^2 = 3.6\,\text{N}$$
  Applying a safety factor $S_f = 2.5$ against road shocks:
  $$F_{retention} \ge 9.0\,\text{N} \quad (\approx 0.92\,\text{kgf})$$
* **Magnet Configuration:**
  * 4x **Neodymium N52 countersunk magnets** ($\varnothing 10\,\text{mm} \times 2\,\text{mm}$ thickness) positioned at the four corners of the wall cradle.
  * Combined magnetic attraction provides **$10.5\,\text{N}$ of holding force**, guaranteeing the remote will never rattle or dislodge during severe off-road driving, while allowing clean one-handed intentional detachment.
  * **Self-Centering Geometry:** The cradle incorporates a $3\,\text{mm}$ perimeter chamfered lip that physically guides the remote into perfect alignment upon proximity.

### 4.3 Zero Electrical Contacts (Corrosion-Free Design)
* Standard pogo-pin dock contacts rapidly corrode and pit in outdoor marine and humid van environments.
* Because the Seeed Studio XIAO ESP32-C6 consumes only **~15 µA in Deep Sleep**, the remote operates for **$>2.5$ years on 2x standard AA alkaline cells**.
* The dock contains **zero electrical contacts**—it is 100% mechanical and magnetic, making it immune to dust, rain, and oxidation.
* The battery hatch uses a toolless magnetic sliding cover for fast battery replacement without disassembling the enclosure.

### 4.4 Exterior Campsite Keypad Mapping (7-Button Matrix)
The 7-button diode binary matrix provides dedicated control tailored for outdoor camping:

| Button Index | Function | Remote Action | Central Channel Target |
| :---: | :--- | :--- | :--- |
| **0** | **Porch / Awning Light** | Click: Toggle / Hold: Dim | Aux 1 (12V High-Side PROFET) |
| **1** | **Entry Step / Puddle Light** | Click: Toggle ON/OFF | Aux 2 (12V High-Side PROFET) |
| **2** | **Kitchen Galley Lights** | Click: Toggle / Hold: Dim | Zone 2 (PWM 200 Hz) |
| **3** | **Cabin Main Ceiling Lights** | Click: Toggle / Hold: Dim | Zone 1 (PWM 200 Hz) |
| **4** | **Water Pump (Seaflo)** | Click: Toggle ON/OFF | Channel 4 (PROFET + Flyback) |
| **5** | **Ceiling Ventilation Fan** | Click: Toggle ON/OFF | Channel 8 (P-MOSFET) |
| **6** | **Master Night Shutdown** | Click: All Lights & Pump OFF | Global Central Safety Action |

### 4.5 RF Propagation & Open-Air ESP-NOW Range
* **Frequency:** 2.4 GHz ISM band, 802.11 DSSS/OFDM PHY.
* **Line-of-Sight Range (Outdoor Campsite):** **35 to 50 meters** line-of-sight using the integrated ceramic/PCB antenna of the XIAO ESP32-C6.
* **Penetration Range (Through closed van body/windows):** **15 to 25 meters**.
* **Protocol Resilience:** Includes automatic hardware retry (up to 5 attempts) and cryptographic replay protection.

---

## 5. 3D Printing & Manufacturing Guidelines

For DIY fabrication via 3D printing:

| Parameter | Central Enclosure | Remote Faceplates & Cradles |
| :--- | :--- | :--- |
| **Recommended Material** | **PETG** or **ABS / ASA** | **PETG** or **ASA** (UV & Heat resistant) |
| **Prohibited Material** | **PLA** (Warps at $>50^\circ\text{C}$ in hot van interiors) | **PLA** |
| **Layer Height** | $0.20\,\text{mm}$ (Structural) | $0.12\,\text{mm} - 0.16\,\text{mm}$ (Smooth finish) |
| **Infill Density** | $30\% - 40\%$ (Gyroid or Grid) | $25\%$ (Gyroid) |
| **Threaded Fasteners** | M3 $\times$ 4.0 mm brass heat-set inserts | M2.5 $\times$ 3.0 mm brass heat-set inserts |
| **Wall Perimeters** | 4 perimeters ($1.6\,\text{mm}$ wall thickness) | 3 perimeters ($1.2\,\text{mm}$ wall thickness) |

---

## 6. Parametric CAD Models & 3D Engineering Verification

All enclosures and cradles are generated programmatically using **`build123d` (Python Code-as-CAD on OpenCASCADE)** and verified with a custom headless **software Z-buffer rasterizer**.

### 6.1 Generated CAD Inventory

| Part Name | File (STEP / STL) | Outer Envelope ($X \times Y \times Z$) | Solid Volume | Key Engineering Features |
| :--- | :--- | :---: | :---: | :--- |
| **Central Enclosure Base** | [`central_enclosure_base.step`](../hardware/enclosures/models/central_enclosure_base.step) | $219.0 \times 127.2 \times 47.2\,\text{mm}$ | $166.6\,\text{cm}^3$ | Integrated M4 mounting ears, stepped alignment lip ($+2.2\,\text{mm}$), 4x gusseted M3 standoff towers, 4x PG9/PG11 gland collars, passive side vents. |
| **Central Enclosure Lid** | [`central_enclosure_lid.step`](../hardware/enclosures/models/central_enclosure_lid.step) | $175.0 \times 125.0 \times 18.0\,\text{mm}$ | $100.4\,\text{cm}^3$ | $3.5\,\text{mm}$ top wall, $1.5\,\text{mm}$ counterbore clamping shoulder, internal boss columns, perimeter mating groove, 5x SPDT keyway notches. |
| **Remote Enclosure Body** | [`remote_enclosure_body.step`](../hardware/enclosures/models/remote_enclosure_body.step) | $86.0 \times 86.0 \times 18.0\,\text{mm}$ | $39.6\,\text{cm}^3$ | $3.5\,\text{mm}$ floor ($1.4\,\text{mm}$ solid backing behind magnets), 4x edge-centered magnet pockets, 2x AA battery bay, 4x M2.5 corner bosses. |
| **Remote Faceplate** | [`remote_enclosure_faceplate.step`](../hardware/enclosures/models/remote_enclosure_faceplate.step) | $86.0 \times 86.0 \times 5.4\,\text{mm}$ | $33.4\,\text{cm}^3$ | Stepped alignment tongue, 4x M2.5 countersinks, 6x chamfered button apertures, recessed rotary dial pocket with anti-rotation tab, LED light cone. |
| **Magnetic Wall Cradle** | [`remote_magnetic_cradle.step`](../hardware/enclosures/models/remote_magnetic_cradle.step) | $96.0 \times 96.0 \times 11.0\,\text{mm}$ | $47.9\,\text{cm}^3$ | $45^\circ$ self-centering lead-in chamfer, **dual ergonomic finger extraction scallops** ($R=16\,\text{mm}$), 4x magnet pockets, 2x recessed wall countersinks. |

### 6.2 Automation Scripts & Skills
* **Parametric Generator:** [`hardware/enclosures/scripts/generate_enclosures.py`](../hardware/enclosures/scripts/generate_enclosures.py)
* **Software Z-Buffer Renderer:** [`hardware/enclosures/scripts/render_stl.py`](../hardware/enclosures/scripts/render_stl.py)
* **CAD Design Skill & Reference Standards:** [`.agents/skills/parametric-cad-enclosures/SKILL.md`](../.agents/skills/parametric-cad-enclosures/SKILL.md)

