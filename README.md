# VanNOW: Sistema de Relés Inalámbricos para Camper Van

Este proyecto (**VanNOW**) contiene el diseño de hardware (como código) y el firmware para un sistema de conmutación de relés y transistores inteligente en una camper van, controlado de forma inalámbrica por ESP32 mediante el protocolo autónomo ESP-NOW.

---

## Estructura del Repositorio

*   **`docs/`**: Contiene la documentación del proyecto, decisiones de arquitectura y especificaciones eléctricas.
    *   [`propuesta_proyecto.md`](file:///Users/daniel/Projects/vannow/docs/propuesta_proyecto.md): Propuesta inicial aprobada.
*   **`firmware/`**: Código fuente C++ para los microcontroladores (gestión con PlatformIO).
    *   **`central/`**: Firmware del receptor en el gabinete eléctrico central. Controla MOSFETs, PROFETs y optoacopladores.
        *   [`platformio.ini`](file:///Users/daniel/Projects/vannow/firmware/central/platformio.ini)
        *   [`src/main.cpp`](file:///Users/daniel/Projects/vannow/firmware/central/src/main.cpp)
    *   **`remote/`**: Firmware para los paneles de pulsadores inalámbricos autónomos. Utiliza Deep Sleep profundo y envía comandos al central.
        *   [`platformio.ini`](file:///Users/daniel/Projects/vannow/firmware/remote/platformio.ini)
        *   [`src/main.cpp`](file:///Users/daniel/Projects/vannow/firmware/remote/src/main.cpp)
*   **`hardware/`**: Código de diseño de circuito impreso (PCB) utilizando la plataforma de Hardware-as-Code **Atopile**.
    *   **`central_pcb/`**:
        *   [`ato.yaml`](file:///Users/daniel/Projects/vannow/hardware/central_pcb/ato.yaml)
        *   [`central.ato`](file:///Users/daniel/Projects/vannow/hardware/central_pcb/central.ato): Esquema y conexiones de la placa central.

---

## Cómo Empezar

### 1. Firmware (PlatformIO)

Para cargar el software en tus módulos ESP32:

1.  Abre la carpeta `firmware/central/` o `firmware/remote/` en VS Code con la extensión **PlatformIO** instalada.
2.  En el módulo **Central**, compila y sube el código. Abre el monitor serial a `115200 baudios` y copia la **Dirección MAC** del dispositivo.
3.  En el archivo [`src/main.cpp` del módulo remoto](file:///Users/daniel/Projects/vannow/firmware/remote/src/main.cpp#L17), reemplaza la dirección MAC en `centralMacAddress` con la dirección que copiaste del central:
    ```cpp
    uint8_t centralMacAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    ```
4.  Carga el código en el módulo remoto.
5.  ¡Listo! Al presionar los pulsadores (conectados a los pines indicados), el remoto transmitirá la señal de conmutación al central.

### 2. Hardware (Atopile)

Para compilar el circuito y generar los archivos de KiCad:

1.  Asegúrate de tener instalado Python y Atopile (`pip install atopile`).
2.  Abre una terminal en `hardware/central_pcb/`.
3.  Ejecuta el comando para compilar:
    ```bash
    ato build
    ```
4.  Esto validará las conexiones eléctricas y generará los esquemáticos y la netlist en formato compatible con **KiCad**.
