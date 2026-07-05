# Evaluación de Hardware: Módulos ESP32 y Técnicas de Multiplexación

Este documento evalúa las opciones para simplificar el hardware de control central de la van utilizando técnicas de multiplexación de pines (expansores I2C) y realiza una recomendación técnica comparativa sobre qué variantes del chip ESP32 (S3, C6, C3, Classic) son más adecuadas para cada componente del sistema.

---

## 1. Multiplexación de Salidas (¿Cómo reducir el uso de GPIOs?)

¡Sí, es totalmente posible y muy recomendable! De hecho, usar multiplexación o expansión por I2C en la unidad central es una práctica de diseño excelente por tres razones:

1.  **Reduce el número de pines necesarios** en el ESP32 de 11 a solo **2 pines** (SDA y SCL).
2.  **Protege al ESP32** contra transitorios de corriente o fallas en la etapa de potencia (si un canal falla catastróficamente, destruye el chip expansor de $2, no al microcontrolador principal).
3.  **Facilita el ruteado de la placa (PCB)** y permite ampliar canales en el futuro sin cambiar el controlador.

A continuación, se evalúan las dos mejores alternativas para expandir pines vía I2C:

### Opción A: PCA9685 (Controlador PWM de 16 Canales I2C) - ¡RECOMENDADO!

Este chip se comunica por I2C y ofrece 16 canales de salida con modulación por ancho de pulsos (PWM) por hardware de 12 bits de resolución.

- **Por qué es ideal para tu Van:**
  - **Atenuación nativa de luces:** Puedes controlar el brillo de las 4 zonas de luces LED de forma independiente y ultra suave sin consumir ciclos del procesador del ESP32.
  - **Control de todo tipo de cargas:** Además de PWM para luces, sus salidas pueden conmutar a ON/OFF digital puro (brillo 0% o 100%) para activar los PROFETs de la bomba de agua, ventiladores, o los optoacopladores del inversor.
  - **Solo consume 2 pines** del ESP32 (SDA/SCL).

### Opción B: MCP23017 (Expansor de E/S Digitales de 16 Canales I2C)

Este chip ofrece 16 pines que pueden configurarse individualmente como entradas o salidas digitales.

- **Limitación:** Solo maneja estados lógicos llanos (HIGH/LOW). No soporta PWM por hardware.
- **Uso:** Excelente si solo vas a usar relés mecánicos de encendido/apagado absoluto, pero no sirve para hacer atenuación progresiva en tus zonas de luces de forma eficiente.

---

## 2. Evaluación Comparativa de Módulos ESP32

Analizamos las cuatro principales variantes de SoC (System on Chip) de Espressif disponibles para determinar cuál se adapta mejor al gabinete eléctrico (Central) y a los paneles de pulsadores (Remotos).

| Característica          | ESP32-C6                                                   | ESP32-S3                       | ESP32-C3                     | ESP32 Classic (WROOM)      |
| :---------------------- | :--------------------------------------------------------- | :----------------------------- | :--------------------------- | :------------------------- |
| **Arquitectura**        | RISC-V Single-core (160 MHz)                               | Xtensa LX7 Dual-core (240 MHz) | RISC-V Single-core (160 MHz) | Xtensa Dual-core (240 MHz) |
| **Conectividad**        | Wi-Fi 6 (802.11ax), BLE 5.3, **Zigbee 3.0, Thread/Matter** | Wi-Fi 4, BLE 5.0               | Wi-Fi 4, BLE 5.0             | Wi-Fi 4, BLE 4.2           |
| **Pines GPIO (DevKit)** | ~22 pines                                                  | **~45 pines**                  | ~15 pines                    | ~25 pines                  |
| **Consumo Deep Sleep**  | **Excelente (~15 $\mu$A)**                                 | Medio (~30-40 $\mu$A)          | Excelente (~20 $\mu$A)       | Alto (~100-150 $\mu$A)     |
| **Formatos XIAO**       | **Sí (Seeed Studio)**                                      | Sí (Seeed Studio)              | Sí (Seeed Studio)            | No (Solo placas grandes)   |
| **Soporte ESP-NOW**     | Sí (con Wi-Fi 6)                                           | Sí                             | Sí                           | Sí                         |

---

## 3. Recomendación Arquitectónica del Sistema

### A. Para los Módulos Remotos (Pulsadores)

- **Módulo Recomendado:** **Seeed Studio XIAO ESP32-C6**
- **Razón:**
  1.  **Consumo Ultra Bajo:** Al contar con la tecnología de procesamiento de Wi-Fi 6 y la arquitectura RISC-V, consume muy poca corriente en deep sleep (~15 $\mu$A). Tus 2 baterías AA durarán años.
  2.  **Tamaño Mini:** Con solo 17.5 x 21 mm, cabe perfectamente en el espacio interior de cualquier cajetín de interruptor de pared de la van.
  3.  **Futuro Matter/Zigbee:** Si decides integrar el sistema a un hub domótico Matter más adelante, el C6 ya tiene el transceptor de radio necesario (802.15.4) integrado físicamente.

### B. Para la Unidad Central (Gabinete Eléctrico) - ¡SELECCIONADO: Lonely Binary ESP32-S3 N16R8!

Se ha seleccionado el **Lonely Binary ESP32-S3 N16R8** para la unidad central debido a su facilidad de conexionado directo, alta calidad de fabricación (acabado de oro por inmersión) y gran capacidad de almacenamiento y memoria de trabajo (16MB Flash, 8MB PSRAM), descartando el uso de expansores de pines I2C en esta etapa.

- **Componentes:** 1x Lonely Binary ESP32-S3 N16R8.
- **Justificación de Consumo y Compatibilidad:**
  - **Consumo Diario Continuo (24/7):** Al ser un receptor, la central debe mantener su radio de Wi-Fi encendida en modo de escucha activo (Rx) constantemente. A 3.3V, el ESP32-S3 consume ~90 mA, mientras que el C6 consume ~50 mA. Pasado por un regulador buck eficiente (90%), el S3 consume **~0.66 Ah al día a 12V** (frente a los ~0.37 Ah del C6). Esta diferencia de ~0.29 Ah diarios es completamente despreciable en una batería de van típica de 100Ah-200Ah (menos del 0.3% diario).
  - **Compatibilidad ESP-NOW:** La comunicación inalámbrica cruzada entre el emisor (XIAO ESP32-C6, Wi-Fi 6) y el receptor (ESP32-S3, Wi-Fi 4) mediante ESP-NOW es **100% compatible nativamente**. El C6 baja automáticamente a modo de compatibilidad de tramas 802.11b/g/n para comunicarse.
  - **Simplicidad de Hardware:** Con más de 40 GPIOs disponibles en el S3, se conectan directamente todos los canales MOSFET, relés y optoacopladores sin necesidad de integrar un chip multiplexor PCA9685, reduciendo la complejidad del circuito y del firmware.

---

## 4. Próximos Pasos de Diseño (Con ESP32-S3 Central)

Al optar por el **ESP32-S3** para la unidad central:

1.  **Mapeo de Pines:** Definiremos las salidas GPIO directas del ESP32-S3 hacia las señales de control de los optoacopladores (para inversor/calefactor) y los módulos MOSFET (para iluminación y bomba).
2.  **Simplificación del Firmware:** El control se realizará mediante llamadas directas `digitalWrite()` y `ledcWrite()` (para atenuación PWM en canales de iluminación), eliminando la necesidad de gestionar el bus I2C y librerías externas de multiplexación.
3.  **Esquema en Atopile:** El diseño del hardware en Atopile conectará los pines del ESP32-S3 directamente a las etapas de potencia, facilitando la creación de la PCB final (placa base/carrier board) donde se pinchará el DevKitC.
