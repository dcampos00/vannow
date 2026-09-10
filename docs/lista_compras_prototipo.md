# Guía de Prototipado y Lista de Compras

Este documento detalla los componentes necesarios para construir un prototipo inicial de pruebas (una unidad central y una unidad remota) en una placa de pruebas (breadboard / protoboard). Esto te permitirá validar la comunicación inalámbrica ESP-NOW, el modo Deep Sleep (consumo) y la conmutación física antes de fabricar la placa PCB final.

---

## 1. Lista de Compras para el Prototipo

Para el prototipo en protoboard, utilizaremos componentes en formato "módulo" (fáciles de conectar con cables jumper) en lugar de chips de montaje superficial (SMD) que irán en la PCB definitiva.

### A. Cerebro y Comunicación (Microcontroladores)

- [x] **1x Seeed Studio XIAO ESP32-C6:**
  - _Nota:_ Módulo de desarrollo ultra pequeño y de muy bajo consumo que actuará como la unidad Remota (pulsadores inalámbricos y encoder).
- [x] **1x Lonely Binary ESP32-S3 N16R8 (16MB Flash / 8MB PSRAM):**
  - _Nota:_ Placa premium seleccionada y adquirida para la unidad Central en el gabinete. Cuenta con 16MB de Flash, 8MB de PSRAM, acabado de oro por inmersión y múltiples GPIOs para conexión directa sin necesidad de expansores.
- [x] **2x Cables USB-C:** Para programar y monitorear cada placa.

### B. Unidad Central de Pruebas (Gabinete)

- [x] **1x Módulo Regulador Buck DC-DC (se recomienda MP1584EN de calidad o regulador Pololu de 5V):**
  - _Nota:_ Para reducir los 12V de la batería a 5V estables para el ESP32 Central. Evitar clones baratos de LM2596 que suelen tener chips falsificados, generan mucho ruido/ripple de voltaje y se calientan en exceso.
- [x] **1x Módulo MOSFET de 4 Canales de Canal N de nivel lógico real (ej. LR7843 o D4184):**
  - _Nota:_ Aunque inicialmente solo se usan 2 canales para las cargas del prototipo, se recomienda un módulo de 4 canales. Tienen un costo y tamaño similar, simplifican el cableado al compartir las borneras de alimentación/tierra, y dejan 2 canales libres para futuras ampliaciones en la van (sensores, ventiladores, luces extra, etc.). Se puede socketear en la PCB final.
- [x] **1x Módulo Optoacoplador PC817 de 2 o 4 canales (con resistencias integradas):**
  - _Nota:_ Se prefiere una placa de módulo pre-ensamblada en lugar de chips optoacopladores PC817 sueltos. Esto garantiza resistencias de entrada/salida correctamente dimensionadas para 3.3V/5V, reduce fallos de cableado y proporciona aislamiento galvánico limpio para controlar el inversor de forma segura.
- [x] **1x Interruptor de Palanca de 3 Posiciones para Montaje en Panel (SPDT ON-OFF-ON, 10A-15A a 12V):**
  - _Nota:_ Se recomienda comprar directamente la versión final de grado industrial para panel (con terminales de pala/Faston o rosca) en lugar de una versión miniatura de protoboard. Para probarlo en el prototipo, simplemente soldaremos o crimparemos cables jumper a sus terminales para poder pincharlo en la protoboard. Esto evita gastos redundantes y valida la robustez del cableado real.
- [x] **1x Portafusibles aéreo y fusibles de 5A o 10A:** Para proteger la línea de alimentación de 12V durante las pruebas.

### C. Unidad Remota de Pruebas (Pulsadores)

- [x] **4x Pulsadores Táctiles de Protoboard (momentáneos):** Botones pequeños estándar.
- [x] **1x Encoder Rotativo EC11 (con botón pulsador integrado, compatible con protoboard):**
  - _Nota:_ Altamente recomendado si deseas probar el sistema de atenuación (dimmer) táctil y premium para las luces. Permite regular el brillo girando el eje y encender/apagar pulsando la perilla. La lógica de consumo y conexión se detalla en [control_encoder_rotativo.md](control_encoder_rotativo.md).
- [x] **1x Portapilas para 2 Baterías AA (o portapilas 18650):** Para alimentar el remoto de forma autónoma.
- [x] **2x Pilas AA (alcalinas normales o recargables):** Voltaje total ~3V.

### D. Elementos Generales de Laboratorio

- [x] **2x Protoboards (Placas de pruebas) medianas o grandes.**
- [x] **1x Kit de Cables Jumper (Macho-Macho y Macho-Hembra):** Para realizar las conexiones rápidamente.
- [ ] **Resistencias varias (Kit de resistencias):**
  - _4x 10k Ohm_ (como pull-ups para los botones).
  - _2x 100k Ohm_ (para el divisor de tensión que mide la batería).
  - _2x 220 Ohm_ (para limitar corriente de LEDs de prueba).
- [x] **LEDs de colores (Rojo, Verde):** Para verificar visualmente el estado de las salidas y la confirmación de señal.

---

## 2. Diagrama de Conexiones del Prototipo

### Unidad Central (Receptor - ESP32-S3)

1.  **Alimentación:**
    - Batería 12V $\rightarrow$ Entrada del regulador Buck (VIN+ / VIN-).
    - Salida del regulador Buck ajustada a **5.0V** $\rightarrow$ Pin `5V` (o `VIN`) del ESP32-S3 y `GND` del ESP32-S3.
2.  **Módulo MOSFET de 4 Canales (Cargas 12V):**
    - `GPIO 4` (PWM) del ESP32-S3 $\rightarrow$ Pin de Entrada `IN1` (Control Canal 1 - Luces Zona A).
    - `GPIO 5` (PWM) del ESP32-S3 $\rightarrow$ Pin de Entrada `IN2` (Control Canal 2 - Luces Zona B).
    - `GPIO 6` (PWM) del ESP32-S3 $\rightarrow$ Pin de Entrada `IN3` (Control Canal 3 - Luces Zona C).
    - `GPIO 7` del ESP32-S3 $\rightarrow$ Pin de Entrada `IN4` (Control Canal 4 - Bomba de Agua Seaflo).
3.  **Módulo Optoacoplador PC817 (Aislamiento Señales):**
    - `GPIO 15` del ESP32-S3 $\rightarrow$ Pin de Entrada `IN1` (Control Canal 1 - Inversor Victron).
    - `GPIO 16` del ESP32-S3 $\rightarrow$ Pin de Entrada `IN2` (Control Canal 2 - Calefactor Diésel).
4.  **Medidor de Batería Principal 12V:**
    - Batería 12V $\rightarrow$ Resistencia 100k Ohm $\rightarrow$ Pin `GPIO 1` (ADC1_CH0) del ESP32-S3 $\rightarrow$ Resistencia 27k Ohm $\rightarrow$ `GND`.
      - _Nota:_ Este divisor de tensión reduce los 12V-15V de la batería a una escala segura de 0V-3.1V para la lectura del ADC del ESP32-S3.

### Unidad Remota (Emisor)

1.  **Alimentación:**
    - Portapilas 2x AA (~3V) $\rightarrow$ Pin `3V3` del ESP32 y pin `GND`.
2.  **Pulsadores (Wake-up):**
    - Botón 1 $\rightarrow$ Conectado entre `GPIO 12` y `GND`. Resistencia pull-up de 10k Ohm externa desde el botón a `3V3`.
    - Botón 2 $\rightarrow$ Conectado entre `GPIO 13` y `GND`. Resistencia pull-up de 10k Ohm externa a `3V3`.
3.  **Medidor de Batería:**
    - Batería (+3V) $\rightarrow$ Resistencia 100k $\rightarrow$ `GPIO 34` (ADC) $\rightarrow$ Resistencia 100k $\rightarrow$ `GND`.

---

## 3. Plan de Pruebas de 3 Pasos

Una vez comprados los componentes, realizaremos las siguientes pruebas para validar el prototipo:

- [ ] **1. Prueba de Comunicación Base:** Subir los códigos actuales y verificar en el Monitor Serial del Central que, al presionar un botón del remoto, el mensaje llega y se visualiza la información de botón y voltaje.
- [ ] **2. Prueba de Consumo (Deep Sleep):** Conectar un multímetro en serie con la batería del remoto para validar que consume microamperios ($\mu$A) mientras duerme y solo sube a miliamperios (mA) por una fracción de segundo al presionar el botón.
- [ ] **3. Prueba de Potencia y Aislamiento:** Conectar el MOSFET y el optoacoplador para validar que el ESP32 Central efectivamente enciende el circuito de luces de 12V y cierra el circuito del optoacoplador del inversor de forma segura.
