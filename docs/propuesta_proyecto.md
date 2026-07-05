# Propuesta de Diseño: VanNOW - Sistema de Relés Inalámbricos para Camper Van

Este documento define la propuesta inicial para el diseño e implementación del sistema **VanNOW** (control de relés centralizado y distribuido para una casa rodante), utilizando microcontroladores ESP32 y el protocolo ESP-NOW.

---

## 1. Descripción del Sistema y Arquitectura

El sistema centraliza el control de potencia en el gabinete eléctrico de la van. Elimina la necesidad de pasar cables de control o de potencia hacia múltiples interruptores físicos en las paredes, utilizando comunicación inalámbrica de corto alcance.

### Componentes de la Arquitectura
1.  **Unidad Central (Gabinete Eléctrico):**
    *   Un **ESP32 Central** conectado a la batería de servicio (12V) mediante un convertidor reductor (Buck) de grado automotriz para protegerlo de picos de tensión.
    *   Una placa de potencia con **transistores MOSFET / Smart High-Side Switches (PROFETs)** para conmutar las cargas de 12V.
    *   Entrada para relés mecánicos de expansión (para cargas de 110V AC en el futuro).
2.  **Módulos Remotos (Pulsadores inalámbricos):**
    *   Nodos alimentados por baterías (**2x AA o similar**) con un **ESP32** o **ESP8266** de ultra bajo consumo.
    *   **Pulsadores físicos de pared** (momentáneos).
    *   Funcionamiento mediante **Deep Sleep**: el microcontrolador permanece apagado y solo se activa por interrupción física al presionar un botón, transmite el comando y vuelve a dormir en menos de 150 ms.
3.  **Protocolo de Comunicación (Autónomo):**
    *   **ESP-NOW**: Protocolo sin conexión directa de Espressif. Funciona en 2.4 GHz, no requiere router Wi-Fi, es sumamente rápido (latencia < 50ms) y es altamente confiable para entornos camper.

```mermaid
graph TD
    %% Módulos Remotos (Batería)
    subgraph Módulos Remotos (Inalámbricos - Batería 2x AA)
        R1[Panel Entrada: Pulsadores Físicos]
        R2[Panel Cama: Pulsadores Físicos]
        R3[Panel Cabina: Pulsadores Físicos]
    end

    %% Gabinete Eléctrico
    subgraph Gabinete Eléctrico Central (Alimentación 12V Van)
        Buck[Regulador Buck 12V -> 5V/3.3V]
        ESP_C[ESP32 Central]
        Power_Stage[Placa MOSFETs / PROFETs]
        Redundancy[Interruptores Manuales ON-OFF-AUTO]
        Fuses[Fusibles de Protección]

        Buck --> ESP_C
        ESP_C -->|Señal de Control 3.3V| Power_Stage
        Power_Stage --> Redundancy
        Redundancy --> Fuses
    end

    %% Cargas 12V
    subgraph Cargas Controladas (12V DC)
        L1[Luces Zona Común - 4 Zonas]
        L2[Bomba de Agua]
        Out12[Salidas Auxiliares 12V]
        Fan[Ventilador de Techo]
        Inv[Inversor Multiplus II - Remote Pin]
        DCDC[Cargadores DC-DC - Remote Pin]
    end

    %% Conexiones
    R1 -->|ESP-NOW| ESP_C
    R2 -->|ESP-NOW| ESP_C
    R3 -->|ESP-NOW| ESP_C

    Fuses --> L1
    Fuses --> L2
    Fuses --> Out12
    Fuses --> Fan
    Fuses --> Inv
    Fuses --> DCDC
```

---

## 2. Especificación de Cargas y Control Sugerido

Basado en tus requerimientos, se definen los canales de control y su método de conexión:

| Canal | Carga | Corriente Estimada | Método de Control / Conmutación | Calibre Cable Sugerido (AWG)* |
| :--- | :--- | :--- | :--- | :--- |
| **1 - 4** | Luces LED (4 Zonas) | 1A - 3A por zona | MOSFET Canal-P (Permite atenuación PWM) | **14 AWG** (Evita caída de tensión en tramos largos) |
| **5** | Bomba de Agua | 5A - 10A | Smart High-Side Switch (PROFET) | **12 AWG** |
| **6 - 8** | Salidas 12V Auxiliares (3 tomas) | 10A por toma | Smart High-Side Switch (PROFET) | **12 AWG** |
| **9** | Ventilador de Techo | 3A - 5A | MOSFET Canal-P / PROFET | **14 AWG** |
| **10** | Inversor (Multiplus II) | Señal (< 50mA) | Puerto "Remote Switch" (Optoacoplador) | **20 AWG** (Cable de señal simple) |
| **11** | Cargadores DC-DC | Señal (< 50mA) | Puente de encendido remoto (Optoacoplador) | **20 AWG** (Cable de señal simple) |
| **12** | Aire Acondicionado | Control de Potencia | Relé de estado sólido / Contactor (Futuro 110V) | Depende del consumo del A/C |

*\*Nota sobre Calibre de Cables (AWG): Para sistemas de 12V en vans, el calibre se calcula para limitar la caída de tensión a menos del 3% en la distancia total (ida y vuelta del cable).*

---

## 3. Análisis de Fiabilidad y Mitigación de Riesgos

La principal preocupación en sistemas electrónicos camper es la confiabilidad física del hardware y la prevención de fallas graves (como luces o bombas que se queden encendidas permanentemente debido a un fallo del chip).

### Riesgos y Estrategias de Redundancia

1.  **Falla del MOSFET en Cortocircuito (Permanentemente Encendido):**
    *   *Riesgo:* Los transistores MOSFET tradicionales suelen fallar cortocircuitándose entre Drenador y Fuente, lo que dejaría la luz o la bomba permanentemente encendidas.
    *   *Mitigación 1 - Smart Switches (PROFETs):* Utilizaremos interruptores inteligentes de alta potencia (como la familia **Infineon PROFET**). Tienen protección térmica, protección contra sobrecorriente, limitación de corriente integrada y apagado automático en cortocircuito. Son mucho más robustos que un MOSFET estándar.
    *   *Mitigación 2 - Interruptores ON-OFF-AUTO:* Para cargas críticas (Luces principales, Bomba de agua), colocaremos un interruptor físico manual de 3 posiciones en el gabinete eléctrico:
        *   **AUTO:** El circuito es controlado por el ESP32 y sus MOSFETs.
        *   **OFF (Apagado Manual):** Desconexión total física de la carga, impidiendo que se quede encendida si el MOSFET falla.
        *   **ON (Bypass Manual):** Conexión directa a la batería de 12V, permitiendo encender la carga manualmente si el ESP32 fallara o se congelara.

2.  **Pérdida de Paquetes Inalámbricos (ESP-NOW):**
    *   *Riesgo:* Presionar un pulsador remoto y que no se registre la acción debido a interferencias.
    *   *Mitigación:* ESP-NOW soporta confirmación de entrega (**ACK**). Cuando el módulo remoto envía el comando, el ESP32 Central responde inmediatamente con un ACK. Si el remoto no recibe el ACK, reintenta automáticamente hasta 5 veces. Podemos agregar un pequeño LED bicolor en el pulsador remoto que destelle verde si se confirmó la acción, o rojo si falló la comunicación.

3.  **Descarga de Baterías en Remotos:**
    *   *Riesgo:* Quedarse sin batería en los interruptores remotos.
    *   *Mitigación:* Con 2 baterías AA y una corriente de deep sleep de ~15uA, los remotos pueden operar más de 2 años sin cambio de baterías. El ESP32 central monitoreará el nivel de voltaje enviado por los remotos en cada pulsación y podrá alertar cuando la batería baje de 2.2V.

---

## 4. Diseño del Circuito usando Código (Hardware-as-Code)

Implementaremos el diseño del esquemático utilizando **Atopile (`.ato`)** o **SKiDL (Python)**. 

### Propuesta Atopile
Escribiremos el diseño lógico del hardware definiendo componentes de la siguiente manera:
```ato
import ESP32_S3 from "parts/esp32.ato"
import PROFET_ITS4140 from "parts/profet.ato"
import BuckRegulator from "parts/buck.ato"

module CentralController:
    # 1. Alimentación
    buck = new BuckRegulator
    buck.input_voltage = 12V to 15V
    buck.output_voltage = 5V

    # 2. MCU
    mcu = new ESP32_S3
    buck.v_out ~ mcu.power_in

    # 3. Canales de Potencia (Bomba de Agua)
    bomba_driver = new PROFET_ITS4140
    mcu.gpio12 ~ bomba_driver.control_pin
    bomba_driver.v_in ~ buck.v_in # Conectado a 12V directo
```

Este enfoque nos permitirá compilar a KiCad, generar la lista de materiales (BOM) automáticamente y asegurar que las conexiones cumplen con las restricciones eléctricas de diseño de manera automática.
