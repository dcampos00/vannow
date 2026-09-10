# AI Agent Instructions

This repository follows strict software development guidelines. When working on this project, you must adhere to the following fundamental directive:

## Main Directive

> [!IMPORTANT]
> **Always prioritize high-performance, generic, and easy-to-maintain code over quick or easy-to-implement solutions.**

### Development Principles

1. **High Performance:**
   - Avoid premature optimizations, but design with efficiency in mind.
   - Choose optimal algorithms and data structures for the problem.
   - Minimize unnecessary resource usage (memory, CPU, network).

2. **Generic & Reusable Code:**
   - Design solutions that solve the problem generally, not just for the immediate use case.
   - Avoid code duplication by applying clean and parameterized abstractions when appropriate.

3. **Ease of Maintenance:**
   - Code must be clean, self-documented, and follow principles like SOLID.
   - Prefer readability and robust structure over temporary "shortcuts" or quick hacks.
   - Include unit tests and adequate documentation to ensure long-term stability.

4. **Language Requirement:**
   - All code (including variables, functions, classes, and comments) and documentation must be written entirely in English.

### Hardware & Prototyping Principles

1. **Safety First (Logic-Level MOSFET Selection):**
   - Avoid recommending or using basic IRF520 MOSFET modules with 3.3V logic microcontrollers (e.g., ESP32-C6). The IRF520 is not a true logic-level device and will fail to saturate at 3.3V, causing it to overheat and fail under load.
   - Always recommend true logic-level MOSFET modules (e.g., LR7843 or D4184) for low-voltage gates to minimize heat generation.

2. **Automotive High-Side Switching & Protection:**
   - For automotive or van power systems, prefer high-side switching (switching the positive rail) rather than low-side (switching the ground). This prevents hazardous bypasses if a wire short-circuits to the metal chassis.
   - Recommend smart high-side switches (e.g., Infineon PROFET) for integrated short-circuit, over-current, and over-temperature protection.

3. **Modular Reusability (Carrier Board Design):**
   - For DIY hardware projects, design custom final PCBs as carrier boards using female headers to accept the exact pre-assembled modules used in the breadboard prototype. This facilitates 100% reuse of components and avoids complex SMD soldering.

4. **Component Quality & Compatibility:**
   - Always prioritize high-quality, verified compatible components (e.g., modules from reputable brands like Pololu, Adafruit, SparkFun, or original components from distributors like Mouser/DigiKey) over cheap unbranded clones.
   - Avoid generic, counterfeit buck converters (e.g., fake LM2596 boards) which suffer from high voltage ripple, low efficiency, and high failure rates. Instead, recommend robust, high-efficiency regulators (like Pololu regulators or genuine MP1584EN boards) to prevent damage to downstream microcontrollers.

---

## Repository Knowledge Base & Documentation Index

Before implementing firmware modifications or hardware schematic revisions, agents MUST consult the relevant authoritative specifications in `docs/` (or the master navigation hub in [`docs/README.md`](docs/README.md)):

### 1. Mandatory Pre-Implementation Reading
- **Hardware & Electrical Constraints:** [`docs/logic_audit_2026-09-03.md`](docs/logic_audit_2026-09-03.md) (Mandatory audit: details 7 critical fixes including ESP32-S3 pinout socket inversions, BTS5008 200 Hz PWM limits, and ADC divider Zener leakage).
- **Physical Safety & Inductive Loads:** [`docs/analisis_critico_riesgos.md`](docs/analisis_critico_riesgos.md) (1N5408 pump flyback, PC817 diesel heater & fridge compressor control, external ON-OFF-AUTO bypass toggles).
- **Firmware Architecture & State Machines:** [`docs/firmware_documentation.md`](docs/firmware_documentation.md) (Unified specification of classes, FSMs, 200 Hz LEDC PWM, and binary matrix decoding).
- **Mechanical & Spatial Clearances:** [`docs/mechanical_and_enclosure_specs.md`](docs/mechanical_and_enclosure_specs.md) (230x190x85 mm cabinet envelope, 50 mm wiring drop, magnetic dockable remote retention).

### 2. Documentation Index by Functional Domain

| Category | File | Description |
| :--- | :--- | :--- |
| **Architecture** | [`docs/propuesta_proyecto.md`](docs/propuesta_proyecto.md) | Original approved system proposal and channel mapping |
| | [`docs/roadmap.md`](docs/roadmap.md) | Implementation roadmap & prioritized feature tiers (Tiers 1–4) |
| | [`docs/gestion_repositorios_dual.md`](docs/gestion_repositorios_dual.md) | Dual-repo private/public sync strategy & `sync_public.sh` usage |
| **Firmware & Protocol** | [`docs/firmware_documentation.md`](docs/firmware_documentation.md) | Unified firmware architecture, class design & test setup |
| | [`docs/atenuacion_remota.md`](docs/atenuacion_remota.md) | Remote dimmer UX (Click vs Hold ramp logic) |
| | [`docs/control_encoder_rotativo.md`](docs/control_encoder_rotativo.md) | EC11 rotary encoder FSM, 1.5s active timeout & protocol |
| | [`docs/analisis_seguridad_rf.md`](docs/analisis_seguridad_rf.md) | RF threat analysis; justification for discarding 433 MHz |
| | [`docs/integracion_cc1101_433mhz.md`](docs/integracion_cc1101_433mhz.md) | CC1101 sub-GHz transceiver SPI evaluation (alternative) |
| **Hardware & Mechanical** | [`docs/mechanical_and_enclosure_specs.md`](docs/mechanical_and_enclosure_specs.md) | Master mechanical specification, cabinet & magnetic dock |
| | [`docs/evaluacion_hardware.md`](docs/evaluacion_hardware.md) | ESP32-S3 vs C6 MCU selection & I2C expander trade-offs |
| | [`docs/opciones_encoders_bajo_perfil.md`](docs/opciones_encoders_bajo_perfil.md) | 5 mm low-profile encoder hardware selection |
| | [`docs/lista_compras_prototipo.md`](docs/lista_compras_prototipo.md) | Breadboard prototype bill of materials & wiring guide |
| | [`docs/references/`](docs/references/) | Component datasheets (PROFETs, MOSFETs, XIAO, ESP32) |
| **Audits & Verification** | [`docs/logic_audit_2026-09-03.md`](docs/logic_audit_2026-09-03.md) | **Active Audit:** 7 critical flaws in hardware & firmware |
| | [`docs/auditoria_logica_2026-07-11.md`](docs/auditoria_logica_2026-07-11.md) | Baseline audit (historical context & pinout evolution) |
| | [`docs/analisis_critico_riesgos.md`](docs/analisis_critico_riesgos.md) | Automotive safety, inductive spikes & manual bypass |
| **Feasibility & Budgets** | [`docs/eficiencia_energetica.md`](docs/eficiencia_energetica.md) | Standby energy budget (0.15 W vs 10–15 W Home Assistant) |
| | [`docs/evaluacion_costos_vanpi.md`](docs/evaluacion_costos_vanpi.md) | Cost evaluation ($137 DIY vs $380 VanPi) |
| | [`docs/proyectos_existentes_maduros.md`](docs/proyectos_existentes_maduros.md) | Benchmark of existing camper automation systems |
| | [`docs/estimacion_complejidad_tiempo.md`](docs/estimacion_complejidad_tiempo.md) | 4-phase project timeline & hours estimation |

### 3. Engineering Skills & Design Standards
- **Schematic & PCB Design:** [`.agents/skills/schematic-design/SKILL.md`](.agents/skills/schematic-design/SKILL.md) (Automotive 12V power entry, PROFET switching, ADC clamping, Atopile Hardware-as-Code assertions).
- **PCB 3D Rendering & CAD Export:** [`.agents/skills/pcb-rendering/SKILL.md`](.agents/skills/pcb-rendering/SKILL.md) (Headless KiCad raytracing, isometric perspective with floor shadows, populated 3D STEP solid export, and ECAD/MCAD clash detection in build123d).
- **Parametric 3D CAD & Mesh Verification:** [`.agents/skills/parametric-cad-enclosures/SKILL.md`](.agents/skills/parametric-cad-enclosures/SKILL.md) (build123d Code-as-CAD, heat-set fastener sizing, stepped joints, ergonomic extraction, and headless software Z-buffer rendering).

> [!NOTE]
> All new documentation and code must be authored in English per the directive in this file. Legacy Spanish documents remain authoritative for system logic unless explicitly superseded.

