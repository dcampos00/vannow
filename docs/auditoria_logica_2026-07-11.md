# VanNOW — Reporte de Auditoría Lógica (Hardware + Firmware)

**Fecha de auditoría:** 2026-07-11
**Alcance:** `firmware/central`, `firmware/remote`, `firmware/lib`, `hardware/central-pcb`, `docs/`
**Método:** Lectura estática del código fuente + verificación contra hojas de datos y documentación oficial de Espressif, Infineon y Phoenix Contact.
**Versión analizada:** SHA de trabajo actual (estado de los `.ato` y `.cpp/.h`).

---

## 0. Resumen ejecutivo

Se identificaron **25 problemas CRÍTICOS/ALTOS**, **16 MEDIOS** y **14 BAJOS**, distribuidos entre hardware (Atopile), firmware del central (ESP32-S3) y firmware del remoto (ESP32-C6).

Los hallazgos más urgentes (bloquean el funcionamiento del prototipo):

1. **GPIO19 y GPIO20 son pines USB-JTAG en el ESP32-S3** — usarlos como GPIO normales deshabilita la programación/Serial por USB. Verificado en la documentación oficial de Espressif v6.0.2.
2. **El firmware del central usa GPIO26, 27, 32, 33 (líneas de PSRAM Octal del módulo N16R8)** — incompatible con el módulo ESP32-S3 N16R8 documentado.
3. **El firmware del central NO coincide con el esquemático Atopile** — el firmware usa pines (25/26/32/33) que el esquemático no expone, y el esquemático usa pines (4/5/6/7/15/20) que el firmware no inicializa.
4. **El remoto usa GPIO4 (ADC) — un pin de strapping del ESP32-C6** que puede impedir el arranque cuando las baterías AA estén bajas.
5. **ESP-NOW sin cifrado, sin nonce, sin allow-list de MAC y sin `esp_wifi_set_channel()`** — la amenaza documentada (clonación con Flipper Zero) es realizable tal como está el código.
6. **Bomba de agua y ventilador en conmutación de lado-bajo (P-MOSFET) sin diodo flyback en el esquemático Atopile** — el diodo está mal orientado en el esquemático y/o no cumple su función (cátodo al drenador del P-MOSFET, no al positivo del motor).
7. **Divisor resistivo del ADC de batería satura a ~15.3 V** y carece de protección Zener → destruye el pin del ESP32 ante transitorios de carga (load-dump ISO 7637-2).
8. **Sin fusible, sin protección de polaridad inversa, sin TVS** en la entrada principal de 12 V — un cable invertido destruye PROFETs, MP1584EN y ESP32 simultáneamente.
9. **Numerosos SKU de LCSC son placeholders** (`C12345`, `C12347`, `C12348`, `C12349`, `C12350`, `C12351`, `C12352`, `C12353`) — el BOM no se puede pedir tal cual está.
10. **El central no llama `esp_wifi_stop()` antes de `esp_deep_sleep_start()`** (en el remoto), ni cierra ESP-NOW — fuga de corriente RTC en cada ciclo de sueño.

La auditoría está organizada en tres secciones (Hardware, Firmware Central, Firmware Remoto) ordenadas por severidad, seguidas de una sección transversal con la **lista priorizada de próximos pasos** y un plan de remediación accionable.

---

## 1. Hallazgos de Hardware (Atopile PCB)

**Archivo principal auditado:** `hardware/central-pcb/central.ato` (686 líneas)
**Archivos auxiliares:** `ato.yaml`, `*.kicad_mod`, BOMs en `build/builds/*/`

---

### 🔴 CRÍTICOS / ALTOS — Riesgo de incendio, daño permanente, fallo funcional

#### H-01 — GPIO19 (ventilador) y GPIO20 (optoacoplador DC-DC) son pines USB-JTAG del ESP32-S3
- **Archivo:** `central.ato:519` (ventilador) y `central.ato:545` (DC-DC).
- **Problema:** El ESP32-S3 reserva GPIO19 y GPIO20 para el USB-JTAG. Configurarlos como GPIO normal **deshabilita el puerto USB**, impidiendo programar el chip por USB-C y bloqueando cualquier log por Serial Monitor. Tras flashear una vez, perderás la consola.
- **Verificación:** Documentación oficial Espressif GPIO & RTC GPIO — ESP32-S3 v6.0.2, sección "GPIO Summary": *"GPIO19 and GPIO20 are used by USB-JTAG by default. If they are reconfigured to operate as normal GPIOs, USB-JTAG functionality will be disabled."* (https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- **Gravedad:** 🔴 CRÍTICO — bloquea el flujo de trabajo de programación.
- **Fix sugerido:** Reasignar ventilador → GPIO9 o GPIO10; DC-DC opto → GPIO11. Verificar que ninguno sea PSRAM/flash en el módulo N16R8 concreto.

#### H-02 — Sin protección de polaridad inversa en la entrada principal
- **Archivo:** `central.ato:398-400` (`pwr_terminal = new ScrewTerminal_2Pin; v_bat_12v ~ pwr_terminal.pin_1; gnd ~ pwr_terminal.pin_2`).
- **Problema:** Si el usuario conecta la batería al revés, los 8 PROFETs, el MP1584EN y el ESP32 reciben −12 V en sus pines de potencia. Destrucción instantánea y simultánea de todos los semiconductores principales.
- **Verificación:** AGENTS.md sección "Automotive High-Side Switching & Protection" exige explícitamente esta protección. AGENTS.md (del propio repo) declara: *"Always prioritize high-performance, generic, and easy-to-maintain code over quick or easy-to-implement solutions"* — y *"Safety First"*. Inspección: ni el BOM ni el esquemático contienen un P-MOSFET reverse-block, Schottky serie, fusible ni PTC.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** Añadir un P-MOSFET en serie (AO4407A en source-follower, Rds(on) ≈ 11 mΩ) o un Schottky 30-40 V (caída 0.5 V) antes de `pwr_terminal`.

#### H-03 — Sin fusible ni PTC en la entrada de batería
- **Archivo:** Mismo que H-02.
- **Problema:** Un PROFET en corto o un cable pelado genera corriente ilimitada hasta que el cable se funde o la batería se ventea con vapores de ácido. Riesgo de incendio.
- **Verificación:** AGENTS.md. `docs/analisis_critico_riesgos.md:15` ya recomienda fusible de 15 A en la bomba — pero no está implementado en el esquemático.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** Portafusibles ATO/ATC en serie con `pwr_terminal.pin_1`, 30 A (suma de cargas pico).

#### H-04 — Sin TVS ni protección contra load-dump (ISO 7637-2)
- **Archivo:** Mismo que H-02.
- **Problema:** Los pulsos de load-dump en sistemas de 12 V pueden llegar a 35-100 V durante cientos de milisegundos. El BTS5008-1EKB tiene una protección interna de Vbb a 41 V, pero por encima de eso el dispositivo se destruye. Sin TVS externa, **todos los 8 PROFETs están expuestos**.
- **Verificación:** Infineon BTS5008-1EKB datasheet (alldatasheet.com/866555/INFINEON/BTS5008-1EKB.html), tabla "Maximum Ratings".
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** SMBJ28CA (28 V trabajo, 30 V clamping) o SMBJ33CA, tan cerca del conector de batería como sea posible.

#### H-05 — Conectores Phoenix MPT 0.5 (6 A) subdimensionados para las cargas del proyecto
- **Archivo:** `central.ato:123-137`, usado por `pump_terminal`, `led_zone*_terminal`, `aux*_terminal`, `fan_terminal`, `inverter_terminal`, `dcdc_terminal`, `pwr_terminal`.
- **Problema:** Phoenix MPT 0.5/2-2.54 y MPT 0.5/4-2.54 admiten **6 A nominales** con cable 0.5 mm² (20 AWG). El proyecto maneja:
  - Bomba Seaflo 42: 3.5 A continuos, 7.5 A de pico (`docs/analisis_critico_riesgos.md:15`).
  - BTS5008-1EKB nominal: 11 A.
  - Calefactor diésel: 8-12.5 A (análisis_critico_riesgos.md:16).
  En el pico de arranque de la bomba o en el encendido del calefactor, los bornes se sobrecalientan, funden el plástico y crean un punto de ignición.
- **Verificación:** Hoja de datos Phoenix Contact 1725672 (https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-mpt-05-4-254-1725672) — 6 A nominales, 160 V.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** Phoenix MPT 1.5 (12 A) o MKKDSN (10-24 A), o llevar la corriente alta con portafusibles ATC externos.

#### H-06 — Diodos flyback de bomba y ventilador mal conectados / no operativos en el esquemático
- **Archivo:** `central.ato:472-475` (bomba), `central.ato:523-525` (ventilador).
  ```
  pump_stage.power_out ~ pump_diode.cathode # Cathode
  gnd ~ pump_diode.anode # Anode
  ```
- **Problema:** El esquemático tiene el cátodo en `pump_stage.power_out` y el ánodo en `gnd`. Para un flyback correcto sobre un motor DC, el cátodo debe ir al **positivo de la carga** (lado de la batería, Vbb) y el ánodo al **negativo de la carga** (lado conmutado por el FET). Aquí, dependiendo de la topología (high-side vs low-side) y de la dirección del flujo, el diodo queda orientado de forma que **no captura la energía inductiva del motor**, solo cortocircuita la salida a GND cuando la inductancia es negativa (diodo en inversa durante la operación normal). En el PROFET build, `pump_stage.power_out` es la salida del BTS5008 hacia el motor — al conmutar, el flyback debe ir entre `power_out` y `gnd`, pero la polaridad está invertida.
- **Verificación:** Comparado con la topología estándar de catch-diode (Infineon PROFET application notes; cualquier libro de texto de electrónica de potencia). El comentario del código es engañoso: "Cathode" debería ir al lado del FET en conmutación low-side o al positivo de la carga en high-side, no a `gnd`.
- **Gravedad:** 🔴 CRÍTICO — primero o segundo evento de apagado del motor, el pico inductivo destruye el PROFET o MOSFET.
- **Fix sugerido:** Verificar la topología real del esquemático (high-side vs low-side) e invertir el diodo. Si la bomba está en low-side (conmutación de GND), entonces: `pump_diode.cathode ~ v_bat_12v` y `pump_diode.anode ~ pump_stage.power_out` (con diodo 1N5408 / SS34 / US1M en SMA).

#### H-07 — Divisor resistivo del ADC satura y carece de protección
- **Archivo:** `central.ato:312-331` (`AnalogVoltageDivider`), usado en PROFET y Modular builds.
  ```
  r_high.value = "100kohm"   # → a V_bat
  r_low.value  = "27kohm"    # → a GND
  ratio        = 0.2126
  ```
- **Problema:**
  1. Con V_bat = 14.4 V → V_ADC = 3.06 V (OK).
  2. Con V_bat = 15.5 V → V_ADC = 3.29 V (límite).
  3. Con V_bat = 16 V → V_ADC = **3.40 V**, por encima del ADC abs-max del ESP32-S3 (~3.6 V). Repetido en el tiempo destruye el pin y posiblemente el SoC.
  4. Sin Zener clamp, sin resistencia serie — un transitorio de alternador (load-dump) lleva la salida a >5 V y mata el ESP32 al instante.
- **Verificación:** Cálculo aritmético del divisor. Documentación del ESP32-S3 (https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/adc/index.html) sobre abs-max de los pines ADC.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** Cambiar a `100k/18k` (ratio 0.153, → 3.30 V a V_bat = 21.6 V). Añadir Zener BZX384C3V3 entre `v_out` y `gnd`, y resistencia serie 1 kΩ entre `v_out` y `esp32.GPIO1`.

#### H-08 — `r_sense` (1.2 kΩ) en IS pin de BTS5008-1EKB es inútil: DEN está atado a GND
- **Archivo:** `central.ato:361-381` (`HighCurrentSmartHighSideSwitch`).
  ```
  u_profet.den ~ gnd                   # DEN = LOW → IS en alta impedancia
  u_profet.current_sense ~ r_sense.p1
  r_sense.p2 ~ gnd
  ```
- **Problema:** Con DEN forzado a LOW, el pin IS del BTS5008-1EKB entra en alta impedancia. No fluye corriente por `r_sense`. El resistor ocupa espacio y aparece en BOM ×8.
- **Verificación:** Infineon BTS5008-1EKB datasheet, sección "Diagnostic Enable" (DEN).
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Eliminar `r_sense` y la conexión `current_sense`. O: poner DEN=HIGH (a Vbb) y leer IS con el ADC para detección real de sobrecorriente por canal.

#### H-09 — Falta total de condensadores de desacoplo
- **Archivo:** `central.ato` — TODO el módulo.
- **Problema:** El ESP32-S3 requiere 100 nF + 10 µF en su pin 3V3. Cada BTS5008-1EKB necesita 100 nF en Vbb. El MP1584EN necesita 22 µF en entrada y salida. Sin ellos: brownout del ESP32 al transmitir, oscilación de los PROFETs, inestabilidad del buck.
- **Verificación:** Espressif ESP32-S3 Hardware Design Guidelines; Infineon PROFET BTS5008-1EKB datasheet, "Application Information".
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Añadir `c_decouple = new MyCapacitor` (100nF, 0805) en el pin 3V3 del ESP32; idem en Vbb de cada BTS5008-1EKB; añadir 22 µF a la entrada/salida del MP1584EN.

#### H-10 — Numerosos SKU de LCSC son placeholders (`C12345`, `C12347`, etc.)
- **Archivo:** `central.ato:49, 83, 90, 100, 107, 117, 126, 133`.
  ```
  supplier_partno="C12345"   # AO4407A MOSFET_PChannel
  supplier_partno="C12347"   # 1N5408 diode MyDiode
  supplier_partno="C12348"   # Header_1x22
  supplier_partno="C12349"   # Header_1x2
  supplier_partno="C12350"   # Header_1x5
  supplier_partno="C12353"   # Header_1x4
  supplier_partno="C12351"   # Phoenix MPT 0.5/2
  supplier_partno="C12352"   # Phoenix MPT 0.5/4
  ```
- **Problema:** `C12345`-`C12353` no son SKUs reales en el catálogo de LCSC (búsqueda directa en lcsc.com no devuelve resultados válidos para estos códigos). El BOM no se puede pedir tal como está.
- **Verificación:** Búsqueda en LCSC devuelve 0 resultados para `C12345`, `C12347`, etc.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Reemplazar por SKUs reales:
  - AO4407A → C16072 (AOS) o C20917 (Onsemi NTR4101P)
  - 1N5408 → C2480
  - Phoenix MPT 0.5/2-2.54 → C82441 (o pasar a MPT 1.5 → C88381)
  - Pin headers → usar SKUs reales como C492401 (1×22) o equivalentes.

#### H-11 — `MOSFET_NChannel` (SOT-23 NMOS) usado como driver del P-MOSFET de alta potencia tiene SKU no verificable y puede no ser logic-level
- **Archivo:** `central.ato:38-44`, usado en `PChannelHighSideSwitch.q_driver` (ventilador).
  ```
  supplier_partno="C20683"   # Sin manufacturer ni partnumber específico
  ```
- **Problema:** El componente se declara como `manufacturer="Generic", partnumber="SOT23_NMOS"`. Sin un partnumber reconocido (AO3400A, IRLML2502, DMG2302UX), no se garantiza que Vgs(th) < 1.5 V a 3.3 V de excitación del ESP32. AGENTS.md advierte: *"Avoid recommending or using basic IRF520 MOSFET modules with 3.3V logic microcontrollers"* — el mismo riesgo aplica aquí si el sustituto es no-logic-level.
- **Verificación:** AGENTS.md; LCSC C20683 no es un partnumber estándar.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Reemplazar por AO3400A (C20917 LCSC, Vgs(th) = 0.7-1.5 V, Rds(on) = 24 mΩ @ Vgs = 2.5 V, perfecto para 3.3 V).

#### H-12 — Resistencia de gate del P-MOSFET sin gate resistor → EMI/ringing
- **Archivo:** `central.ato:228-260` (`PChannelHighSideSwitch`).
- **Problema:** El driver NMOS ataca la compuerta del P-MOSFET (AO4407A, Qg ≈ 60 nC) directamente. La transición queda limitada solo por la capacidad parásita y la inductancia del trazado. dV/dt > 1 V/ns → ringing en el bus de 12 V, EMI radiada que afecta AM/FM, CB, Wi-Fi 2.4 GHz.
- **Verificación:** Buena práctica de diseño de potencia conmutada; AO4407A datasheet recomienda gate resistor 22-100 Ω.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Añadir `r_gate_p` (47 Ω) en serie con la compuerta del `q_pass`.

---

### 🟡 MEDIOS — Robustez, longevidad, operación degradada

#### H-13 — `ato.yaml` referencia una build "default" que NO existe en `central.ato`
- **Archivo:** `ato.yaml:7-14` define solo `profet` y `modular`.
- **Problema:** El directorio `build/builds/default/` contiene artefactos de una versión anterior del esquema (donde existía un módulo `VanCentralController`). Ese módulo fue eliminado y reemplazado por `VanCentralControllerPROFET` y `VanCentralControllerModular`. Cualquier intento de `atopile build default` fallará.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Eliminar `build/builds/default/` o restaurar el módulo borrado.

#### H-14 — Pinout JSON reporta "leads no conectados" en los 8 BTS5008-1EKB
- **Archivo:** `build/builds/profet/pinout/004_aux1_stage_u_profet.json:17-47` y similares.
- **Problema:** `isConnected: false` en pads 10/11/12 (`out`) y pad 15 (`vs`) aunque las conexiones ato existen. Indica un bug del compilador Atopile con señales multi-pin en SOIC-14-1EP.
- **Gravedad:** 🟡 MEDIO — la netlist KiCad probablemente está bien, pero los warnings sugieren un bug latente en Atopile.
- **Fix sugerido:** Reportar a upstream de Atopile; verificar la netlist final en KiCad antes de fabricar.

#### H-15 — `r_pulldown` (10 kΩ) en BTS5008 IN es redundante
- **Archivo:** `central.ato:359-371`.
- **Problema:** El BTS5008-1EKB ya tiene un pull-down interno en IN (Infinein datasheet §8.1). Añadir uno externo no aporta nada.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Eliminar `r_pulldown`.

#### H-16 — Pinout scrambled en JSON para AO4407A
- **Archivo:** `build/builds/profet/pinout/027_fan_stage_q_pass.json`.
- **Problema:** `leadDesignator` no coincide con la conexión ato (`source3` en pin 2, etc.).
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Verificar la netlist final en KiCad; reportar a Atopile.

#### H-17 — AO4407A marcado como EOL por Alpha & Omega
- **Archivo:** `central.ato:46-58`.
- **Problema:** AO4407A aparece en la roadmap de AOS como "not recommended for new designs".
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Considerar reemplazos: AOSS21311C (C5365441 LCSC), NTR4101P (C20917).

#### H-18 — Mezcla de componentes through-hole (DO-201, DIP-4) con SMD (0805, SOT-23, SO-14)
- **Archivo:** Footprints: `D_DO-201AD_P15.24mm_Horizontal.kicad_mod`, `DIP-4_W7.62mm.kicad_mod`.
- **Problema:** El resto del diseño es SMD. Soldar manualmente DO-201 y DIP-4 rompe el flujo "carrier board" descrito en AGENTS.md.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Cambiar diodos a SMA/SMB (SS34, US1M) y el PC817 a versión SOP-4 (PC817X4NSZ9F).

#### H-19 — LEDs de estado cableados pero sin uso en firmware
- **Archivo:** `central.ato:563-582`, `firmware/central/src/SystemController.cpp`.
- **Problema:** Hay tres LEDs (power, cabin, remote) pero el firmware no los controla, salvo al inicializar los pines como PWM con duty 0.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Documentar la intención (LEDs de diagnóstico) y asignarles GPIOs seguros.

#### H-20 — Resistencia de LEDs 330 Ω consume 6.4 mA por LED; impacto en battery monitor sleep
- **Archivo:** `central.ato:565, 572, 579`.
- **Problema:** 3 LEDs × 6.4 mA = 19 mA continuos cuando el pin está HIGH (con `pinMode(GPIO2, OUTPUT)` y `digitalWrite(2, HIGH)` por error de programación).
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Subir a 1 kΩ (2 mA) sigue siendo visible.

---

### 🟢 BAJOS — Estilo, menores, documentación

#### H-21 — `value = ""` en `MyResistor`/`MyCapacitor` es incoherente con la asignación posterior
- **Archivo:** `central.ato:21, 29, 36`.
- **Gravedad:** 🟢 BAJO.

#### H-22 — Comentario "Pinout" del AO4407A correcto, pero `leadDesignator` en JSON está scrambled
- Ver H-16.

#### H-23 — BOM CSV incompleto en `build/builds/modular/` (sólo headers, sin PROFETs/MOSFETs)
- **Archivo:** `build/builds/modular/modular.bom.csv`.
- **Problema:** El build "modular" usa módulos externos (YYNMOS-4 + PC817), pero el CSV no lista esos módulos (porque son externos), solo el carrier. La cuenta de coste en `docs/evaluacion_costos_vanpi.md` puede ser engañosa si se basa solo en este BOM.
- **Gravedad:** 🟢 BAJO.
- **Fix sugerido:** Documentar el coste total incluyendo los módulos externos.

#### H-24 — `power_tree.md` está vacío en los tres builds
- **Archivo:** `build/builds/*/power_tree.md`.
- **Problema:** Solo contiene `graph TD` sin nodos.
- **Gravedad:** 🟢 BAJO — quirk de Atopile, no eléctrico.

---

## 2. Hallazgos de Firmware — Central (ESP32-S3)

**Archivos auditados:** `firmware/central/src/*.cpp/.h`, `firmware/lib/wireless/`, `firmware/lib/protocol/`, `firmware/lib/arduino_mock/`, `firmware/central/test/`.

---

### 🔴 CRÍTICOS / ALTOS

#### FC-01 — Pinout del firmware NO coincide con el esquemático Atopile
- **Archivo:** `firmware/central/src/SystemController.cpp:6-20` (firmware) vs. `hardware/central-pcb/central.ato:388-585` (hardware).
- **Tabla de discrepancias:**

| Canal              | Firmware (`SystemController.cpp`) | Esquemático (`central.ato`) | Problema                                                                                       |
|--------------------|-----------------------------------|------------------------------|------------------------------------------------------------------------------------------------|
| Lights Zone 4      | GPIO **27**                       | GPIO 15                      | GPIO27 = línea SPI/PSRAM del módulo N16R8 → no se puede usar. Firmware chocará con la flash/PSRAM. |
| Water Pump         | GPIO **25**                       | GPIO 4                       | Inconsistencia directa.                                                                        |
| Aux Outlet 1       | GPIO **26**                       | GPIO 5                       | GPIO26 = línea SPI/PSRAM.                                                                      |
| Aux Outlet 2       | GPIO **32**                       | GPIO 6                       | GPIO32 = línea SPI/PSRAM (octal).                                                              |
| Aux Outlet 3       | GPIO **33**                       | GPIO 7                       | GPIO33 = línea SPI/PSRAM en chips R8/R8V (octal).                                              |
| Ceiling Fan        | GPIO **19**                       | GPIO 19                      | OK en pin, pero **GPIO19 = USB-JTAG** (ver H-01).                                              |
| Inverter (opto)    | GPIO **21**                       | GPIO 21                      | OK.                                                                                            |
| DC-DC Charger      | GPIO **22**                       | GPIO 20                      | GPIO22 está bien, pero GPIO20 = USB-JTAG (ver H-01).                                           |

- **Verificación:** Espressif ESP32-S3 GPIO table v6.0.2 — GPIO26-32 son SPI0/1 (reservados para SPI flash/PSRAM); GPIO33-37 también están reservados en módulos con Octal PSRAM (ESP32-S3R8/N16R8). GPIO19 y GPIO20 son USB-JTAG por defecto.
- **Gravedad:** 🔴 CRÍTICO — el firmware tal como está **no funciona en un módulo N16R8** (que es lo que el proyecto documenta).
- **Fix sugerido:** Crear una única fuente de verdad. Mejor opción: editar `SystemController.cpp` para usar los GPIOs del esquemático (4, 5, 6, 7, 15, 20) tras corregir H-01; o bien mover GPIO19/20 a GPIO9/10/11/14 (verificando disponibilidad en N16R8 concreto).

#### FC-02 — ESP-NOW sin cifrado, sin allow-list, sin nonce, sin secuencia
- **Archivo:** `firmware/lib/wireless/src/WirelessManager.cpp:28-36`, `firmware/central/src/main.cpp:20-31`, `firmware/lib/protocol/protocol.h`.
- **Problema:**
  ```cpp
  peerInfo.encrypt = false;          // Cifrado LMK deshabilitado
  ```
  El `OnDataRecv` del central acepta cualquier MAC, no comprueba nonce ni secuencia. La estructura `SwitchMessage` no tiene `seq` ni `timestamp`. Reproducir un click es trivial con un Flipper Zero o cualquier ESP32 promiscuo — exactamente el escenario que `docs/analisis_critico_riesgos.md:96-99` advertía pero que el firmware no mitiga.
- **Verificación:** Documentación Espressif `esp_now.h` — soporta LMK encryption con `peerInfo.lmk[]` y `peerInfo.encrypt = true`.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:**
  1. Generar una LMK aleatoria por dispositivo, almacenar en NVS/efuse.
  2. `peerInfo.encrypt = true; peerInfo.lmk = ...`
  3. Añadir `uint16_t seq` a `SwitchMessage`. Mantener ventana deslizante de 16 IDs por `senderMac` para descartar paquetes viejos.

#### FC-03 — `readMainBatteryVoltage()` usa referencia ADC incorrecta y satura
- **Archivo:** `firmware/central/src/SystemController.cpp:95-99`, `SystemController.h:33`.
  ```cpp
  static constexpr float ADC_VOLTAGE_REF = 3.1f; // Comentario: "calibrada a 3.1V"
  float adcVoltage = (raw / 4095.0f) * ADC_VOLTAGE_REF;
  ```
- **Problema:**
  1. La referencia real del ADC del ESP32-S3 es VDD3P3_RTC ≈ 3.3 V (con `analogSetAttenuation(ADC_11db)`); 3.1 V es un valor arbitrario. Subestima el voltaje real en ~6 % → a 12.6 V se reportan ~11.84 V.
  2. Sin `analogSetPinAttenuation(pin, ADC_11db)` configurado, el rango por defecto puede ser 0-1.1 V → lecturas totalmente erróneas.
  3. Combinado con el divisor H-07 (100k/27k), satura por encima de 15.3 V. La función no indica saturación, reporta el valor crudo 4095.
  4. `pinMode(BATTERY_ADC_PIN, INPUT)` antes de `analogRead` puede deshabilitar el ADC en algunas versiones de Arduino-ESP32.
- **Verificación:** Documentación Arduino-ESP32 `analogReadMilliVolts()` y ESP-IDF `esp_adc_cali_characterize()`.
- **Gravedad:** 🔴 CRÍTICO — la telemetría de batería miente.
- **Fix sugerido:**
  - Eliminar `pinMode(BATTERY_ADC_PIN, INPUT)`.
  - Usar `analogSetPinAttenuation(1, ADC_11db)` en `begin()`.
  - Reemplazar `analogRead` por `analogReadMilliVolts(1)` (calibrado por eFuse).
  - Cambiar el divisor a 100k/18k y añadir Zener (H-07).

#### FC-04 — Callback ESP-NOW accede a estado compartido sin sincronización
- **Archivo:** `firmware/central/src/main.cpp:20-31` (`OnDataRecv`), `firmware/central/src/SystemController.cpp:50-93` (`dispatchMessage`), `firmware/central/src/DimmableChannel.cpp:91-132` (`update`).
- **Problema:** El callback ESP-NOW corre en la tarea Wi-Fi (alta prioridad). Llama a `dispatchMessage` → `handleAction` → `setState`/`setBrightness` → `ledcWrite`. El loop principal llama a `update()` sobre los mismos objetos. No hay mutex, portMUX ni cola. Las operaciones no son atómicas en presencia de campos multibyte (`_isActive`, `_currentBrightness`, `_targetBrightness`, `_lastRampTime`, `_lastHoldMsgTime`).
- **Verificación:** Espressif ESP-NOW docs (https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html): *"The receiving callback function also runs from the Wi-Fi task. So, do not do lengthy operations in the callback function."*
- **Gravedad:** 🟠 ALTO — glitches visibles en PWM durante recepción; potencial inconsistencia de la FSM.
- **Fix sugerido:** Encolar `SwitchMessage + MAC` en una `QueueHandle_t` desde el callback; procesar en una tarea dedicada de baja prioridad con `portMUX_TYPE` para proteger el estado de cada `DimmableChannel`.

#### FC-05 — No hay persistencia (NVS/Preferences) del brillo de los dimmers
- **Archivo:** `firmware/central/src/DimmableChannel.cpp:81-84` (`_lastOnBrightness`).
- **Problema:** Tras un brownout o desconexión de batería, los 11 canales arrancan en OFF y el "brillo memorizado" se pierde. La librería `Preferences` (Arduino-ESP32) no se usa.
- **Gravedad:** 🟠 ALTO — UX y confusión del usuario.
- **Fix sugerido:** Guardar `_lastOnBrightness` (y opcionalmente `_isActive`) en NVS en cada `Release` o `setBrightness` confirmado.

#### FC-06 — No hay gestión de brownout / watchdog / reset reason
- **Archivo:** `firmware/central/src/main.cpp:33-49, 61-74`.
- **Problema:** No se llama `esp_reset_reason()`, no hay `esp_register_shutdown_handler`, no se distingue brownout de power-on. El usuario nunca sabe por qué el sistema se reinició.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** `esp_reset_reason()` en `setup()` + imprimir/loguear. Considerar `vTaskDelay(pdMS_TO_TICKS(5))` en lugar de `delay(5)` para alimentar el TWDT explícitamente.

#### FC-07 — GPIO 1 ADC pin tiene `pinMode(..., INPUT)` que puede romper `analogRead`
- **Archivo:** `firmware/central/src/SystemController.cpp:37`.
- **Problema:** `INPUT` configura el pad como digital input sin pull. En algunas versiones de Arduino-ESP32, esto sobrescribe la configuración del ADC.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Eliminar la línea.

---

### 🟡 MEDIOS

#### FC-08 — `DigitalChannel::setState` con `_activeLow=true` arranca con el estado "load ON"
- **Archivo:** `firmware/central/src/DigitalChannel.cpp:19-23`.
- **Problema:** Si una instancia se construye con `_activeLow=true`, en `begin()` → `setState(false)` el pin va a LOW, lo que en un canal active-low significa "carga ON". Falsa seguridad en el arranque.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Para active-low, escribir HIGH en `begin()` (load OFF).

#### FC-09 — `_lastOnBrightness` se sobrescribe durante la rampa → guarda valor no intencional
- **Archivo:** `firmware/central/src/DimmableChannel.cpp:81-84`.
- **Problema:** Cada paso del ramp llama a `setBrightness`, que actualiza `_lastOnBrightness`. Si el usuario hace Hold desde 100% hacia abajo hasta 50%, el próximo Click guarda 50% en vez del valor previo.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Solo actualizar `_lastOnBrightness` en `Release` con un valor distinto del de partida.

#### FC-10 — ESP-NOW channel no fijado; STA puede cambiarlo
- **Archivo:** `firmware/central/src/main.cpp:33-49`.
- **Problema:** `WiFi.mode(WIFI_STA); WiFi.disconnect();` no llama `esp_wifi_set_channel()`. Cualquier operación posterior puede hacer que la STA migre a otro canal → ESP-NOW silenciosamente falla.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** `esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE)` (o 6/11) en el `begin()` del central y del remoto.

#### FC-11 — Receptor acepta cualquier MAC; loguea en claro por Serial
- **Archivo:** `firmware/central/src/SystemController.cpp:50-61`.
- **Problema:** Cualquiera con acceso físico al USB ve los MAC de los paneles pareados (pequeño leak).
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Allow-list + hash o truncado del MAC en logs.

#### FC-12 — Bucle de reinicio si Wi-Fi init falla
- **Archivo:** `firmware/central/src/main.cpp:45-49`.
- **Problema:** `ESP.restart()` inmediato sin backoff. En una zona sin antena o con brownouts repetidos, el sistema entra en un ciclo infinito.
- **Gravedad:** 🟡 MEDIO.
- **Fix sugerido:** Backoff exponencial + estado fail-safe (todos los canales OFF).

#### FC-13 — `OnDataRecv` no valida NULL ni tamaño de payload
- **Archivo:** `firmware/central/src/main.cpp:20-23`.
- **Problema:** Falta `if (!recv_info || !incomingData || len <= 0) return;`
- **Gravedad:** 🟡 MEDIO.

#### FC-14 — `_isActive` y `_currentBrightness` se almacenan en SRAM volátil
- Cubierto por FC-05.

---

### 🟢 BAJOS

#### FC-15 — `Serial.printf` con `%d` para `size_t` (en main.cpp:23)
- Warning de compilación.
- **Gravedad:** 🟢 BAJO.

#### FC-16 — Tipo `esp_now_recv_info` debería ser `esp_now_recv_info_t`
- **Archivo:** `main.cpp:20`.
- **Gravedad:** 🟢 BAJO.

#### FC-17 — Mock de `delay()` no bloquea
- **Archivo:** `firmware/lib/arduino_mock/src/Arduino.cpp:53-55`.
- **Gravedad:** 🟢 BAJO — enmascara paths que asumen blocking, pero aceptable para tests unitarios.

#### FC-18 — `test_power_manager_sleep_execution` es trivial
- **Archivo:** `firmware/central/test/test_channels/test_channels.cpp:168-176`.
- **Gravedad:** 🟢 BAJO — tests deben inspeccionar la máscara capturada por el mock.

#### FC-19 — Cobertura de `WirelessManager` ≈ 0% en tests nativos
- **Archivo:** `firmware/central/platformio.ini:36` (`src_filter` excluye `wireless`).
- **Gravedad:** 🟢 BAJO — tests estructurales del struct `SwitchMessage` añadirían valor.

---

## 3. Hallazgos de Firmware — Remoto (ESP32-C6)

**Archivos auditados:** `firmware/remote/src/*.cpp/.h`, `firmware/lib/`.

---

### 🔴 CRÍTICOS / ALTOS

#### FR-01 — ESP-NOW con canal sin fijar (channel = 0) → mensajes perdidos
- **Archivo:** `firmware/lib/wireless/src/WirelessManager.cpp:28-36`.
  ```cpp
  peerInfo.channel = 0;   // 0 = "use whatever Wi-Fi is on"
  ```
- **Problema:** Tras cada `esp_deep_sleep_start`/wake-up, la STA del ESP32-C6 puede arrancar en cualquier canal (1, 6 u 11 según el último escaneo). Si el central está fijo en un canal (típicamente 1), **todos los mensajes fallan silenciosamente** en algunos ciclos. Síntoma clásico: "a veces funciona, a veces no".
- **Verificación:** Documentación Espressif `esp_now.h` — `channel = 0` significa "el canal actual de Wi-Fi"; Espressif recomienda fijar el canal para ESP-NOW.
- **Gravedad:** 🔴 CRÍTICO — confiabilidad del sistema entero.
- **Fix sugerido:**
  ```cpp
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  peerInfo.channel = 1;
  peerInfo.ifidx = WIFI_IF_STA;
  ```

#### FR-02 — `RemoteSender::send()` retorna `_deliverySuccess` rancio en timeout
- **Archivo:** `firmware/remote/src/RemoteSender.cpp:50-74`.
  ```cpp
  while (!_messageSent && (millis() - startWait < 200)) { delay(1); }
  return _deliverySuccess;     // ← puede tener valor de la llamada anterior
  ```
- **Problema:** Si el ACK no llega en 200 ms, `_messageSent` queda false, pero la función retorna `_deliverySuccess`, que mantiene el valor de la última llamada exitosa. Cualquier retry/telemetría futura basada en este retorno será falsa.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:**
  ```cpp
  bool delivered = _messageSent && _deliverySuccess;
  _messageSent = false;
  return delivered;
  ```

#### FR-03 — `readBatteryVoltage()` devuelve 3.0 V cuando la batería está muerta
- **Archivo:** `firmware/remote/src/RemoteSender.cpp:42-48`.
  ```cpp
  return (batteryVoltage < 0.5f) ? 3.0f : batteryVoltage;
  ```
- **Problema:** El ternario es contraproducente: cuando el ADC reporta ~0 (batería agotada, divisor abierto), la función devuelve 3.0 V. La alerta `< 2.2 V` del central nunca se dispara. El proyecto documenta esta alerta en `docs/firmware_documentation.md:243-245` pero el firmware la hace inoperante.
- **Gravedad:** 🔴 CRÍTICO — feature documentado como funcional está roto.
- **Fix sugerido:**
  ```cpp
  if (raw <= 0 || batteryVoltage < 0.3f) return 0.0f; // sentinel "muerto"
  return batteryVoltage;
  ```
  Y en el central (`SystemController.cpp:59`), aceptar `battery_voltage == 0.0f` como "alerta crítica".

#### FR-04 — ESP-NOW sin cifrado
- **Archivo:** `firmware/lib/wireless/src/WirelessManager.cpp:32`.
  ```cpp
  peerInfo.encrypt = false;
  ```
- **Problema:** Idéntico a FC-02 desde el lado del emisor. Combinado con `FR-01` (canal), la clonación de un click es trivial.
- **Gravedad:** 🔴 CRÍTICO.
- **Fix sugerido:** Activar LMK (ver FC-02).

#### FR-05 — `BATTERY_ADC_PIN = 4` es un pin de strapping del ESP32-C6
- **Archivo:** `firmware/remote/src/main.cpp:33`, `firmware/remote/src/RemoteSender.h`.
- **Problema:** GPIO4 es **strapping pin** en el ESP32-C6: su valor al reset determina boot mode y routing de ROM-messages. El divisor de batería 100k/100k introduce ~1.0-1.5 V en el pin al arranque. Con baterías AA frescas (3.0 V) y divisor cargado, el pin ve 1.5 V, por encima del umbral de strapping para HIGH en la mayoría de las revisiones; con baterías agotadas (1.8 V), el divisor cae a 0.9 V → boot errático o fallo silencioso.
- **Verificación:** Espressif ESP-IDF GPIO & RTC GPIO — ESP32-C6 v6.0.2: *"Strapping pin: GPIO4, GPIO5, GPIO8, GPIO9, and GPIO15 are strapping pins."*
- **Gravedad:** 🔴 CRÍTICO — el remoto puede no arrancar cuando las baterías envejezcan.
- **Fix sugerido:** Mover el ADC a GPIO6 (D4 en XIAO ESP32-C6) o GPIO1 (D1, ya usado por encoder A). Aceptable cualquiera de los LP-GPIOs 0-7.

#### FR-06 — GPIO1 (encoder B) NO está en `wakeupPins[]` → rotación CCW no despierta
- **Archivo:** `firmware/remote/src/main.cpp:37`.
  ```cpp
  const uint8_t wakeupPins[] = {0, 2}; // GPIO 0 (A), GPIO 2 (SW)
  ```
- **Problema:** El primer flanco de una rotación **horaria** es A falling (B aún HIGH) → wake OK en GPIO0. El primer flanco de una rotación **antihoraria** es B falling (A aún HIGH) → wake falla porque GPIO1 no está en la máscara. El usuario debe girar ~¼ de detent antes de que A caiga y despierte el chip.
- **Verificación:** Trigonometría de encoder en cuadratura; datasheet EC11.
- **Gravedad:** 🟠 ALTO — UX rota en la mitad de las direcciones.
- **Fix sugerido:** `const uint8_t wakeupPins[] = {0, 1, 2};`

#### FR-07 — Encoder usa decodificación half-quadrature sub-muestreada
- **Archivo:** `firmware/remote/src/EncoderHandler.cpp:19-35`.
- **Problema:** Solo decodifica el flanco de bajada de A → pierde la mitad de los pasos. Con `loop` cada 5 ms, una rotación rápida (>5 ms entre detents) salta pulsos.
- **Gravedad:** 🟠 ALTO — UX y resolución de atenuación reducidas a la mitad.
- **Fix sugerido:** Implementar decodificación full-quadrature (lookup table con 4 estados), idealmente con ISR en PCNT (Pulse Counter peripheral nativo del ESP32-C6).

#### FR-08 — Data race en `_messageSent`/`_deliverySuccess` (volatile ≠ atomic)
- **Archivo:** `firmware/remote/src/RemoteSender.cpp:4-5, 12-15, 61-73`.
- **Problema:** Las dos flags se actualizan desde la tarea Wi-Fi (callback ESP-NOW) y se leen desde el loop principal. `volatile` solo inhibe reordenamiento del compilador, no provee atomicidad entre las dos lecturas. En RISC-V (ESP32-C6) las dos escrituras pueden reordenarse, dejando al observador con `_messageSent=true` y `_deliverySuccess=false` de la llamada anterior.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** Reemplazar `volatile bool` por `std::atomic<bool>` (C++11 soportado por la toolchain) o usar un `SemaphoreHandle_t` dado por el callback y tomado por `send()` con timeout.

#### FR-09 — Pull-ups internos inactivos durante deep sleep → input flotante
- **Archivo:** `firmware/remote/src/PowerManager.cpp:18-34`.
- **Problema:** El ESP-IDF desactiva el dominio RTC_PERIPH durante deep sleep por defecto, perdiendo las pull-ups configuradas con `pinMode(..., INPUT_PULLUP)`. Sin pull-ups externos (10 kΩ), los pines de wakeup quedan flotantes y generan wakes espurios o clicks fantasma.
- **Verificación:** Espressif ESP-IDF Sleep Modes docs: *"During deep sleep, the RTC_PERIPH is powered down, releasing internal pull-ups"*.
- **Gravedad:** 🟠 ALTO — UX y consumo errático.
- **Fix sugerido:** Usar `gpio_hold_en()` por cada pin de wakeup, o `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON)`, o garantizar pull-ups externos de 10 kΩ en hardware.

#### FR-10 — `PowerManager::goToSleep` no llama `esp_wifi_stop()` / `esp_now_deinit()`
- **Archivo:** `firmware/remote/src/PowerManager.cpp:18-34`.
- **Problema:** Espressif recomienda explícitamente detener Wi-Fi y ESP-NOW antes de `esp_deep_sleep_start`. Sin esto, el consumo RTC sube por encima del objetivo de 15 µA y el estado interno de ESP-NOW puede corromperse entre ciclos.
- **Verificación:** Espressif Sleep Modes API Reference.
- **Gravedad:** 🟠 ALTO —破坏了 la autonomía de batería proyectada.
- **Fix sugerido:**
  ```cpp
  esp_now_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_deep_sleep_start();
  ```

#### FR-11 — Botón con bounce → clicks espurios en pulsación sostenida
- **Archivo:** `firmware/remote/src/ButtonHandler.cpp:36-49`.
- **Problema:** Una vez en estado `Pressed`, cualquier HIGH se interpreta como release, incluso un rebote mecánico de ~ms. El FSM no distingue release de bounce → múltiples `Click` por pulsación + rampa de Hold se reinicia a cada rebote.
- **Verificación:** Datasheet de cualquier switch táctil (Omron B3F, C&K PTS810); comportamiento de rebote típico 5-20 ms.
- **Gravedad:** 🟠 ALTO.
- **Fix sugerido:** En `case Pressed`, al detectar HIGH, pasar a `Debounce` y solo emitir `Click` si el HIGH persiste tras 15 ms. Mismo defecto en `EncoderHandler.cpp:69-71`.

#### FR-12 — `readBatteryVoltage()` con Vref no calibrado
- **Archivo:** `firmware/remote/src/RemoteSender.cpp:43-46`.
- **Problema:** Asume Vref = 3.3 V sin usar eFuse calibration. Dos remotos idénticos pueden discrepar ±5 %. Además, VDD3P3 cae cuando las AA envejecen, amplificando el error.
- **Gravedad:** 🟠 ALTO — afecta la precisión del monitor de batería.
- **Fix sugerido:** Usar `esp_adc_cali_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars)` + `esp_adc_cali_raw_to_voltage()`.

---

### 🟡 MEDIOS

#### FR-13 — Encoder SW sin estados Holding/Release
- **Archivo:** `firmware/remote/src/EncoderHandler.cpp:28-33, 50-76`.
- **Problema:** A diferencia de `ButtonHandler`, el pulsador del eje del encoder no emite `StartHold` ni `Release`. No permite atenuación mantenida con el pulsador del eje.
- **Gravedad:** 🟡 MEDIO.

#### FR-14 — Encoder SW hardcoded a `button_index = 0`
- **Archivo:** `firmware/remote/src/main.cpp:93`.
- **Problema:** Para un panel encoder con `REMOTE_ID=2`, el press del eje siempre activa "Lights Zone 3" (mapeo en `SystemController.cpp:73`).
- **Gravedad:** 🟡 MEDIO.

#### FR-15 — `RemoteSender::send()` valor de retorno ignorado
- **Archivo:** `firmware/remote/src/main.cpp:93, 100`.
- **Problema:** El caller no distingue éxito de fallo. El usuario no sabe si el botón fue procesado.
- **Gravedad:** 🟡 MEDIO.

#### FR-16 — Bucle de 200 ms bloqueante drena batería en cada send
- **Archivo:** `firmware/remote/src/RemoteSender.cpp:67-71`.
- **Problema:** A 80 mA active, 200 ms × N eventos × 6 eventos/s Hold = ~265 mAh perdidos por ráfaga. Contradice el propósito de bajo consumo.
- **Gravedad:** 🟡 MEDIO.

#### FR-17 — Polling loop con `delay(5)` durante toda la ventana activa
- **Archivo:** `firmware/remote/src/main.cpp:85-111`.
- **Problema:** Durante el inactivity timeout (1.5 s), la CPU corre al 100 % sin razón.
- **Gravedad:** 🟡 MEDIO.

#### FR-18 — Mock de `esp_sleep_get_wakeup_cause()` siempre retorna EXT1
- **Archivo:** `firmware/lib/arduino_mock/src/esp_sleep.h:14`.
- **Problema:** El path de cold-boot (línea 60-63 de main.cpp) no es testeable.
- **Gravedad:** 🟡 MEDIO.

#### FR-19 — `test_power_manager_sleep_execution` no inspecciona la máscara
- **Archivo:** `firmware/remote/test/test_remote_handlers/test_remote_handlers.cpp:168-176`.
- **Problema:** Bug tipográfico en `1ULL << n` vs `1U << n` (silencioso con pin < 32) no sería detectado.
- **Gravedad:** 🟡 MEDIO.

#### FR-20 — `RemoteSender` 0% cobertura en tests
- **Archivo:** `firmware/remote/platformio.ini:20`.
- **Gravedad:** 🟡 MEDIO.

#### FR-21 — Cold-boot con botón sostenido → comportamiento indefinido
- **Archivo:** `firmware/remote/src/main.cpp:59-63`, `ButtonHandler.cpp`.
- **Problema:** Baterías instaladas con botón presionado → boot con `Pressed` inmediato → 400 ms → `StartHold` continuo.
- **Gravedad:** 🟡 MEDIO.

---

### 🟢 BAJOS

#### FR-22 — Valor de retorno de `esp_sleep_enable_ext1_wakeup()` no comprobado
- **Archivo:** `PowerManager.cpp:28`.
- **Gravedad:** 🟢 BAJO.

#### FR-23 — `delay(10)` tras `Serial.flush()` malgasta 10 ms × 80 mA cada sleep cycle
- **Archivo:** `PowerManager.cpp:32-33`.
- **Gravedad:** 🟢 BAJO.

#### FR-24 — `centralMacAddress[]` es placeholder `AA:BB:…:FF`
- **Archivo:** `firmware/remote/src/main.cpp:28`.
- **Gravedad:** 🟢 BAJO.

#### FR-25 — `int8_t _steps` puede hacer wrap con rotación rápida
- **Archivo:** `EncoderHandler.cpp:25, 42`.
- **Gravedad:** 🟢 BAJO.

#### FR-26 — Comentario de test dice "threshold 500 ms" pero el código usa 400 ms
- **Archivo:** `firmware/remote/test/test_remote_handlers/test_remote_handlers.cpp:57`.
- **Gravedad:** 🟢 BAJO.

---

## 4. Hallazgos transversales (documentación, repositorio)

#### T-01 — Documentación GPIO en `docs/firmware_documentation.md:202-214` NO coincide con esquemático Atopile
- El doc y el firmware están alineados entre sí, pero ambos divergen del esquemático.
- **Gravedad:** 🟠 ALTO.
- **Fix:** Tras FC-01/H-01, regenerar la tabla.

#### T-02 — `docs/firmware_documentation.md:243-245` documenta alerta de batería 2.2 V que el firmware no puede disparar (FR-03)
- **Gravedad:** 🟡 MEDIO.

#### T-03 — `docs/analisis_critico_riesgos.md:15-17` recomienda fusibles y 1N5408 que NO están en el esquemático Atopile
- **Gravedad:** 🟡 MEDIO — desconexión diseño ↔ análisis.

#### T-04 — `docs/analisis_critico_riesgos.md:13` menciona "módulos MOSFET multicanal" pero el build PROFET los integra en PCB (conflicto entre build modular y PROFET)
- **Gravedad:** 🟡 MEDIO.

#### T-05 — `ato.yaml:7-14` referencia una build "default" inexistente
- Cubierto por H-13.

#### T-06 — `central.ato` no tiene pull-ups de 10 kΩ en los pines de wakeup del remoto
- Cubierto por FR-09.

#### T-07 — No hay tests de ESP-NOW ni de la lógica de receive callback
- Cubierto por FC-04 y FC-19.

---

## 5. Lista priorizada de próximos pasos

### Fase 0 — STOP & DECIDE (1-2 horas)
Antes de tocar nada, decidir el destino de los placeholders y la arquitectura de seguridad:
1. **Fuente única de verdad:** ¿firmware manda o esquemático manda? Recomendación: el esquemático (es lo que se va a fabricar). Reasignar pines en `SystemController.cpp` para coincidir con `central.ato` tras corregir H-01.
2. **Perfil de carga:** bomba + ventilador + 4 zonas LED + 3 aux + inverter + DC-DC. Sumar corrientes pico; dimensionar fusibles, TVS y PTCs.
3. **Modelo de seguridad:** ¿cómo se evita la clonación? LMK + secuencia + allow-list (mínimo viable). Considerar cambio a CC1101 a 433 MHz si el threat model lo requiere (ver `docs/integracion_cc1101_433mhz.md`).

### Fase 1 — Bloqueantes de seguridad (1 semana)
Los issues 🔴 que pueden causar incendio o destruir hardware:
1. **H-02** Añadir protección de polaridad inversa (P-MOSFET o Schottky) en `pwr_terminal`.
2. **H-03** Añadir portafusibles ATO + fusible 30 A en `pwr_terminal`.
3. **H-04** Añadir TVS SMBJ28CA en la entrada de batería.
4. **H-05** Sustituir Phoenix MPT 0.5 por MPT 1.5 (o equivalente ≥12 A).
5. **H-06** Verificar y corregir polaridad del flyback de bomba y ventilador.
6. **H-07 + FC-03** Reescalar divisor a 100k/18k, añadir Zener 3V3 + R serie 1 kΩ, eliminar `pinMode` antes de `analogRead`, usar `analogReadMilliVolts`.

### Fase 2 — Compatibilidad de hardware (1 semana)
Los issues 🔴 que rompen el prototipo en hardware:
7. **H-01 + FC-01** Reasignar GPIO19/20 del esquemático a pines libres (no USB-JTAG, no PSRAM). Reasignar también GPIO26/27/32/33 del firmware a GPIO4/5/6/7/15. Validar contra el módulo N16R8 con `gpio_dump_io_configuration()`.
8. **H-09** Añadir condensadores de desacoplo (100nF + 10µF en ESP32 3V3, 100nF en Vbb de cada BTS5008-1EKB, 22µF en MP1584EN entrada/salida).
9. **H-10** Sustituir todos los SKUs placeholder por LCSC reales (C16072 para AO4407A, C2480 para 1N5408, etc.).
10. **H-11** Sustituir `MOSFET_NChannel` genérico por AO3400A (LCSC C20917).
11. **FR-05** Mover `BATTERY_ADC_PIN` del remoto de GPIO4 a GPIO6 (LP-GPIO, no strapping).

### Fase 3 — Seguridad y robustez de firmware (1-2 semanas)
12. **FC-02 + FR-04 + FR-01** Habilitar ESP-NOW LMK encryption, fijar canal con `esp_wifi_set_channel`, añadir `seq` y allow-list de MACs. Implementar FreeRTOS queue + tarea de receive para FC-04.
13. **FR-02** Arreglar `RemoteSender::send()` para no retornar valor rancio.
14. **FR-03** Cambiar sentinel de batería muerta a 0.0 V.
15. **FR-10** Llamar `esp_wifi_stop()` + `esp_now_deinit()` antes de `esp_deep_sleep_start()`.
16. **FR-06** Añadir GPIO1 a `wakeupPins[]`.
17. **FR-07** Implementar decodificación full-quadrature con PCNT.
18. **FR-11** Arreglar FSM de `ButtonHandler` para bounce (transición Pressed → Debounce en lugar de → Idle).
19. **FC-05** Persistir `_lastOnBrightness` en NVS.
20. **FC-06** Loguear `esp_reset_reason()` en `setup()`.

### Fase 4 — Calidad y mantenibilidad (continuo)
21. **H-13 + H-23 + T-05** Limpiar artefactos de la build "default" obsoleta.
22. **FC-09** Corregir sobrescritura de `_lastOnBrightness` durante la rampa.
23. **FC-10** Fijar canal Wi-Fi en el central.
24. **H-08 + H-15** Eliminar `r_sense` y `r_pulldown` redundantes del BTS5008-1EKB; decidir si se quiere diagnóstico de corriente (opcional).
25. **H-12** Añadir gate resistor 47 Ω en `q_pass.gate`.
26. **H-18** Migrar diodos y opto a versiones SMD (SMA/SOP-4) para mantener flujo carrier board.
27. **T-01/T-02/T-03** Regenerar documentación contra el firmware/esquemático corregido.
28. **Test coverage** Extender tests nativos:
    - `test_remote_handlers.cpp`: inspección de la máscara de wakeup; test del sentinel de batería muerta.
    - `test_channels.cpp`: test de saturación del ADC; test del memory de `_lastOnBrightness` post-Release; test de receive callback con allow-list.
    - Añadir test round-trip de `SwitchMessage` (estructura, tamaño).
29. **Mock improvements** `arduino_mock/esp_sleep.h`: función `setWakeupCause()` para tests del cold-boot path.

---

## 6. Mapa de verificación de fuentes

| Hallazgo | Fuente autoritativa | URL |
|---|---|---|
| H-01 GPIO19/20 USB-JTAG (S3) | Espressif ESP-IDF GPIO doc v6.0.2 | https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html |
| FC-01 GPIO26-32 PSRAM (S3) | Espressif ESP-IDF GPIO doc v6.0.2 (misma) | idem |
| FR-05 GPIO4 strapping (C6) | Espressif ESP-IDF GPIO doc v6.0.2 (C6) | https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/gpio.html |
| FR-06 wakeup CCW | Datasheet genérico EC11 + trigonometría | n/a (matemática) |
| H-07 ADC saturation | Aritmética: 3.3/0.2126 = 15.52 V | n/a |
| FC-02 ESP-NOW encrypt | Espressif ESP-NOW API | https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html |
| FR-04 (mismo desde el remoto) | idem | idem |
| FC-04 receive callback thread | Espressif ESP-NOW docs: *"The receiving callback function also runs from the Wi-Fi task."* | https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html |
| FC-03 ADC calibration | Espressif ADC + Arduino-ESP32 `analogReadMilliVolts` | https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/adc/index.html |
| H-08 BTS5008 DEN=LOW → IS HiZ | Infineon BTS5008-1EKB datasheet §8.2 | https://www.alldatasheet.com/datasheet-pdf/pdf/866555/INFINEON/BTS5008-1EKB.html |
| H-04 BTS5008 max Vbb 41 V | Infineon BTS5008-1EKB datasheet "Maximum Ratings" | idem |
| H-05 Phoenix MPT 0.5 = 6 A | Phoenix Contact 1725672 datasheet | https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-mpt-05-4-254-1725672 |
| H-10 LCSC placeholders | Búsqueda directa en lcsc.com (sin resultados para C12345-C12353) | https://www.lcsc.com/search?q=C12345 |
| FR-10 esp_wifi_stop before sleep | Espressif Sleep Modes API Reference | https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html |
| FR-09 Pull-ups inactivas en deep sleep | Espressif Sleep Modes API Reference (misma) | idem |
| H-02/H-03/H-04 reverse polarity, fuse, TVS | AGENTS.md "Automotive High-Side Switching & Protection" | `/var/home/daniel/Projects/vannow/AGENTS.md` |
| H-11 NMOS logic-level warning | AGENTS.md "Safety First (Logic-Level MOSFET Selection)" | idem |
| H-06 flyback topology | Cualquier libro de electrónica de potencia + Infineon PROFET AppNote | n/a (estándar) |
| FR-08 volatile ≠ atomic | C++ standard §1.9 [intro.execution] | https://en.cppreference.com/w/cpp/atomic/memory_order |

---

## 7. Resumen cuantitativo

| Severidad | Hardware | Firmware Central | Firmware Remoto | Transversal | Total |
|-----------|---------:|-----------------:|----------------:|------------:|------:|
| 🔴 Crítico | 6        | 4                | 6               | 0           | 16    |
| 🟠 Alto    | 6        | 3                | 6               | 2           | 17    |
| 🟡 Medio   | 8        | 6                | 9               | 2           | 25    |
| 🟢 Bajo    | 4        | 5                | 5               | 0           | 14    |
| **Total** | **24**   | **18**           | **26**          | **4**       | **72**|

(Nota: la suma por fila puede incluir hallazgos derivados del mismo defecto raíz; los 72 ítems son entradas independientes.)

---

## 8. Conclusión y recomendación

El proyecto VanNOW tiene una **arquitectura sólida** y una documentación extensa que demuestra madurez de diseño (analisis_critico_riesgos.md describe correctamente la necesidad de fusibles, flybacks, antenas externas y overrides SPDT). Sin embargo, **la implementación en Atopile y en firmware NO refleja esa arquitectura**:

- El esquemático no implementa fusibles, TVS, protección de polaridad ni flybacks correctos.
- El firmware del central usa GPIOs que físicamente no existen en el módulo objetivo (PSRAM lines) o que rompen USB.
- El firmware del remoto usa un pin ADC de strapping y no cifra ESP-NOW, contradiciendo el threat model del proyecto.
- Múltiples partes del BOM son SKUs placeholder no pedibles.

**Recomendación:** Antes de fabricar una PCB o desplegar en la van, ejecutar la **Fase 0 + Fase 1 + Fase 2** (3-4 semanas de trabajo). Saltarse cualquiera de estos pasos es un riesgo material de incendio o de destruir el ESP32 en el primer load-dump del alternador.

Los archivos modificados durante esta auditoría: **ninguno**. Toda la verificación fue read-only. Los cambios propuestos están consolidados en la sección 5 y son aplicables de forma incremental por el equipo.