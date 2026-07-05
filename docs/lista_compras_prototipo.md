# Guía de Prototipado y Lista de Compras

Este documento detalla los componentes necesarios para construir un prototipo inicial de pruebas (una unidad central y una unidad remota) en una placa de pruebas (breadboard / protoboard). Esto te permitirá validar la comunicación inalámbrica ESP-NOW, el modo Deep Sleep (consumo) y la conmutación física antes de fabricar la placa PCB final.

---

## 1. Lista de Compras para el Prototipo

Para el prototipo en protoboard, utilizaremos componentes en formato "módulo" (fáciles de conectar con cables jumper) en lugar de chips de montaje superficial (SMD) que irán en la PCB definitiva.

### A. Cerebro y Comunicación (Microcontroladores)
*   **2x Seeed Studio XIAO ESP32-C6:** 
    *   *Nota:* Módulos de desarrollo ultra pequeños y de muy bajo consumo. Uno actuará como Central (conmutador) y el otro como Remoto (pulsadores inalámbricos).
*   **2x Cables USB-C:** Para programar y monitorear cada placa XIAO.
*   *Alternativa Opcional:* Si consideras expandir el gabinete central con más sensores o relés en el futuro, puedes adquirir **1x ESP32-S3 DevKitC** para la unidad Central y mantener el **XIAO ESP32-C6** para el Remoto.

### B. Unidad Central de Pruebas (Gabinete)
*   **1x Módulo Regulador Buck DC-DC (LM2596 o MP1584EN):**
    *   Para reducir los 12V de la batería de la van a 5V estables para alimentar el ESP32 Central. (El MP1584EN es ultra pequeño y eficiente).
*   **1x Módulo de MOSFETs de Canal N de niveles lógicos (ej. IRF520 o similar en módulo):**
    *   *Nota:* Para el prototipo, un módulo MOSFET pre-ensamblado es ideal para pruebas rápidas de cargas de 12V (como una bombilla o tira LED). En la placa definitiva usaremos PROFETs integrados de Infineon.
*   **2x Optoacopladores PC817 (o módulo de optoacopladores de 2/4 canales):**
    *   Para aislar y controlar la señal del Inversor Multiplus II y cargadores DC-DC de forma segura.
*   **1x Interruptor de Palanca de 3 Posiciones (SPDT ON-OFF-ON o ON-OFF-AUTO):**
    *   Para probar la lógica de redundancia física en una de las cargas.
*   **1x Portafusibles aéreo y fusibles de 5A o 10A:** Para proteger la línea de alimentación de 12V durante las pruebas.

### C. Unidad Remota de Pruebas (Pulsadores)
*   **4x Pulsadores Táctiles de Protoboard (momentáneos):** Botones pequeños estándar.
*   **1x Portapilas para 2 Baterías AA (o portapilas 18650):** Para alimentar el remoto de forma autónoma.
*   **2x Pilas AA (alcalinas normales o recargables):** Voltaje total ~3V.

### D. Elementos Generales de Laboratorio
*   **2x Protoboards (Placas de pruebas) medianas o grandes.**
*   **1x Kit de Cables Jumper (Macho-Macho y Macho-Hembra):** Para realizar las conexiones rápidamente.
*   **Resistencias varias (Kit de resistencias):**
    *   *4x 10k Ohm* (como pull-ups para los botones).
    *   *2x 100k Ohm* (para el divisor de tensión que mide la batería).
    *   *2x 220 Ohm* (para limitar corriente de LEDs de prueba).
*   **LEDs de colores (Rojo, Verde):** Para verificar visualmente el estado de las salidas y la confirmación de señal.

---

## 2. Diagrama de Conexiones del Prototipo

### Unidad Central (Receptor)
1.  **Alimentación:**
    *   Batería 12V $\rightarrow$ Entrada del regulador Buck (VIN+ / VIN-).
    *   Salida del regulador Buck ajustada a **5.0V** $\rightarrow$ Pin `VIN` (o `5V`) del ESP32 y `GND` del ESP32.
2.  **Carga de Prueba (Tira LED o Foco 12V):**
    *   `GPIO 12` del ESP32 $\rightarrow$ Pin de Control (SIG) del Módulo MOSFET.
    *   Módulo MOSFET `V+` / `V-` $\rightarrow$ Batería 12V.
    *   Módulo MOSFET `OUT+` / `OUT-` $\rightarrow$ Tira LED de 12V.
3.  **Control Inversor:**
    *   `GPIO 21` $\rightarrow$ Resistencia de 220 Ohm $\rightarrow$ Pin 1 (Ánodo) del PC817.
    *   `GND` $\rightarrow$ Pin 2 (Cátodo) del PC817.
    *   Los pines 3 y 4 (Emisor/Colector) del PC817 irán a los terminales de interruptor remoto del inversor.

### Unidad Remota (Emisor)
1.  **Alimentación:**
    *   Portapilas 2x AA (~3V) $\rightarrow$ Pin `3V3` del ESP32 y pin `GND`.
2.  **Pulsadores (Wake-up):**
    *   Botón 1 $\rightarrow$ Conectado entre `GPIO 12` y `GND`. Resistencia pull-up de 10k Ohm externa desde el botón a `3V3`.
    *   Botón 2 $\rightarrow$ Conectado entre `GPIO 13` y `GND`. Resistencia pull-up de 10k Ohm externa a `3V3`.
3.  **Medidor de Batería:**
    *   Batería (+3V) $\rightarrow$ Resistencia 100k $\rightarrow$ `GPIO 34` (ADC) $\rightarrow$ Resistencia 100k $\rightarrow$ `GND`.

---

## 3. Plan de Pruebas de 3 Pasos

Una vez comprados los componentes, realizaremos las siguientes pruebas para validar el prototipo:

1.  **Prueba de Comunicación Base:** Subir los códigos actuales y verificar en el Monitor Serial del Central que, al presionar un botón del remoto, el mensaje llega y se visualiza la información de botón y voltaje.
2.  **Prueba de Consumo (Deep Sleep):** Conectar un multímetro en serie con la batería del remoto para validar que consume microamperios ($\mu$A) mientras duerme y solo sube a miliamperios (mA) por una fracción de segundo al presionar el botón.
3.  **Prueba de Potencia y Aislamiento:** Conectar el MOSFET y el optoacoplador para validar que el ESP32 Central efectivamente enciende el circuito de luces de 12V y cierra el circuito del optoacoplador del inversor de forma segura.
