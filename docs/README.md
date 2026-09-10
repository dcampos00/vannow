# VanNOW Documentation Hub

Welcome to the **VanNOW** (Wireless Camper Van Relay System) engineering knowledge base. This directory compiles all architecture proposals, hardware evaluations, firmware specifications, mechanical 3D blueprints, safety audits, and feasibility studies.

---

## 🧭 Role-Based Navigation ("Where Do I Start?")

- **Working on Firmware?** Start with [`firmware_documentation.md`](firmware_documentation.md) for class models and state machines, then check [`roadmap.md`](roadmap.md) for prioritized features and [`logic_audit_2026-09-03.md`](logic_audit_2026-09-03.md) for pending bugfixes.
- **Working on Hardware & PCB?** Review [`logic_audit_2026-09-03.md`](logic_audit_2026-09-03.md) for critical DevKit socket pinout corrections, [`evaluacion_hardware.md`](evaluacion_hardware.md) for MCU specs, [`analisis_critico_riesgos.md`](analisis_critico_riesgos.md) for flyback/bypass safety rules, and inspect [`references/`](references/) for component datasheets.
- **Working on 3D Printing & Enclosures?** Read [`mechanical_and_enclosure_specs.md`](mechanical_and_enclosure_specs.md) for DIN cabinet envelopes, 50 mm wiring clearances, and magnetic dock retention physics. Check `hardware/enclosures/` for Python CAD generators (`generate_enclosures.py`) and exported STEP/STL models.
- **Benchmarking & Power Budgets?** Review [`eficiencia_energetica.md`](eficiencia_energetica.md) for standby power analysis and [`evaluacion_costos_vanpi.md`](evaluacion_costos_vanpi.md) for BOM costing.

---

## 📚 Master Document Catalog

### 1. Architecture, System Design & Planning
| Document | Language | Status | Summary |
| :--- | :---: | :---: | :--- |
| [**propuesta_proyecto.md**](propuesta_proyecto.md) | ES | Active (Base) | Approved baseline project proposal: ESP-NOW wireless topology, 12V channel definitions, and Atopile introduction. |
| [**roadmap.md**](roadmap.md) | ES/EN | Active (Living) | Prioritized 4-tier feature roadmap (NVS persistence, WDT, anti-replay, scenes, LED feedback, web diagnostics). |
| [**gestion_repositorios_dual.md**](gestion_repositorios_dual.md) | ES | Active (Ops) | Private/public repo synchronization procedure and `sync_public.sh` documentation. |

### 2. Firmware & Wireless Protocols
| Document | Language | Status | Summary |
| :--- | :---: | :---: | :--- |
| [**firmware_documentation.md**](firmware_documentation.md) | ES | Active (Master) | Unified firmware architecture, class diagrams, 200 Hz PWM dimming, button/matrix FSMs, and PlatformIO environments. |
| [**atenuacion_remota.md**](atenuacion_remota.md) | ES | Active (Spec) | Remote dimmer button UX: Short-press toggle vs long-press ramp with 150 ms keep-alive packets. |
| [**control_encoder_rotativo.md**](control_encoder_rotativo.md) | ES | Active (Spec) | Rotary encoder (EC11) wireless logic, "Active-on-Demand" 1.5s timeout, and LP-GPIO pinouts. |
| [**analisis_seguridad_rf.md**](analisis_seguridad_rf.md) | ES | Decision | Threat model comparing 433 MHz ASK/OOK vs ESP-NOW; justifies discarding 433 MHz due to replay attacks. |
| [**integracion_cc1101_433mhz.md**](integracion_cc1101_433mhz.md) | ES | Evaluated | Technical evaluation of CC1101 Sub-GHz transceiver integration via SPI (alternative RF route). |

### 3. Hardware, PCB & Mechanical Engineering
| Document | Language | Status | Summary |
| :--- | :---: | :---: | :--- |
| [**remote_pcbs_architecture.md**](remote_pcbs_architecture.md) | EN | Active (Spec) | Architectural specification, UX evaluation & Diode-OR wakeup schematics for Cockpit Hub and Master Entrance Remote. |
| [**mechanical_and_enclosure_specs.md**](mechanical_and_enclosure_specs.md) | EN | Active (Master) | Enclosure dimensions, DIN-rail mounting, 50 mm wiring clearances, magnetic dockable remote (10.5N retention), PETG/ASA printing specs. |
| [**evaluacion_hardware.md**](evaluacion_hardware.md) | ES | Decision | Hardware selection: Lonely Binary ESP32-S3 N16R8 for Central, XIAO ESP32-C6 for Remotes, PCA9685 vs direct GPIOs. |
| [**opciones_encoders_bajo_perfil.md**](opciones_encoders_bajo_perfil.md) | ES | Component | Low-profile (5 mm) encoder selection: Panasonic EVQ-WGD thumbwheel, Alps EC12E SMD, and Alps EC10E hollow-shaft. |
| [**lista_compras_prototipo.md**](lista_compras_prototipo.md) | ES | Lab Guide | Component checklist and wiring guide for the breadboard prototype, including 3-step test plan. |
| [**skills/schematic-design**](../.agents/skills/schematic-design/SKILL.md) | EN | Skill | Comprehensive guide for Hardware-as-Code (Atopile), automotive power entry, PROFET switching, ADC clamping, and automotive protection. |
| [**skills/pcb-rendering**](../.agents/skills/pcb-rendering/SKILL.md) | EN | Skill | Automated KiCad raytracing, 1440p/4K isometric visualization, populated 3D STEP solid export, and ECAD/MCAD clash detection in build123d. |
| [**skills/parametric-cad-enclosures**](../.agents/skills/parametric-cad-enclosures/SKILL.md) | EN | Skill | Production guide for Python Code-as-CAD (`build123d`), heat-set fastener sizing, stepped joints, and software Z-buffer mesh verification. |
| [**references/**](references/) | PDF/XLSX | Assets | Official component datasheets (BTS5008 PROFET, MP1584EN, PC817, ESP32-S3, ESP32-C6). |

### 4. Audits, Verification & Risk Analysis
| Document | Language | Status | Summary |
| :--- | :---: | :---: | :--- |
| [**logic_audit_2026-09-03.md**](logic_audit_2026-09-03.md) | EN | Active (Must Read) | Latest comprehensive audit identifying 7 critical flaws (ESP32-S3 DevKit socket GND short, replay bypass, 5% PWM jitter, 3.3V Zener distortion). |
| [**auditoria_logica_2026-07-11.md**](auditoria_logica_2026-07-11.md) | ES | Historical | Preliminary audit uncovering 25 hardware/firmware issues (USB-JTAG collisions on GPIO 19/20, PSRAM collisions). |
| [**analisis_critico_riesgos.md**](analisis_critico_riesgos.md) | ES | Active (Safety) | Electrical fire hazards, inductive flyback diode (1N5408), diesel heater/fridge PC817 logic, and ON-OFF-AUTO bypass panel mounting. |

### 5. Feasibility, Energy Budgets & Benchmarking
| Document | Language | Status | Summary |
| :--- | :---: | :---: | :--- |
| [**eficiencia_energetica.md**](eficiencia_energetica.md) | ES | Active (Budget) | Standby power budget: VanNOW (0.15 W, 0.25 Ah/day -> 400 days on 100Ah) vs Home Assistant/VanPi (10–15 W, 20–30 Ah/day -> 5 days). |
| [**evaluacion_costos_vanpi.md**](evaluacion_costos_vanpi.md) | ES | Reference | Cost estimation: VanNOW DIY (~$137 USD) vs Pekaway VanPi official hardware (~348 EUR). |
| [**proyectos_existentes_maduros.md**](proyectos_existentes_maduros.md) | ES | Reference | Industry benchmark comparing VanPi, Home Assistant + ESPHome, and Victron Venus OS. |
| [**estimacion_complejidad_tiempo.md**](estimacion_complejidad_tiempo.md) | ES | Reference | 4-phase project timeline breakdown: 30–50 active hours across 6–10 calendar weeks. |
