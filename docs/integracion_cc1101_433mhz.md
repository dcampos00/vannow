# Integración del Módulo de Radiofrecuencia CC1101 (433 MHz)

Este documento detalla cómo la integración del transceptor de sub-GHz **Texas Instruments CC1101** en la frecuencia de **433 MHz** puede resolver los problemas de interferencia por barreras metálicas (jaula de Faraday) en la van y abrir la puerta al uso de interruptores comerciales de bajo costo.

---

## 1. ¿Por qué usar 433 MHz (CC1101) en lugar de 2.4 GHz (ESP-NOW)?

El uso de ondas de radio en la banda Sub-GHz (433 MHz) ofrece ventajas físicas cruciales en entornos automotrices y campers:

*   **Mayor penetración en metal:** La longitud de onda a 433 MHz es de aproximadamente **70 cm**, mientras que a 2.4 GHz (Wi-Fi) es de solo **12.5 cm**. Las ondas más largas "esquivan" y atraviesan las barreras metálicas, paneles de aislamiento de aluminio y gabinetes del vehículo con una atenuación drásticamente menor.
*   **Compatibilidad con interruptores comerciales:** La gran mayoría de los interruptores inalámbricos económicos del mercado (de pared, llaveros, botones) operan en 433 MHz mediante modulación simple (ASK/OOK). Al integrar el CC1101 en el ESP32 Central, puedes **escuchar y decodificar estos interruptores directamente**, eliminando la necesidad de fabricar tus propios módulos remotos.
*   **Consumo ultra bajo:** La transmisión a 433 MHz requiere mucha menos energía que el Wi-Fi o ESP-NOW, prolongando aún más la vida de las baterías en los mandos emisores.

---

## 2. Dos Estrategias de Arquitectura con CC1101

### Estrategia A: Interruptores Comerciales $\rightarrow$ Central ESP32 + CC1101 (¡RECOMENDADO!)
*   **Cómo funciona:** Compras interruptores inalámbricos de 433 MHz comerciales (ej. de pared, con aspecto residencial). En el gabinete eléctrico, tu **ESP32 Central** tiene conectado un **módulo CC1101** configurado en modo receptor.
*   **Ventajas:**
    *   **Cero código en remotos:** No diseñas ni programas los remotos. Usas hardware comercial ya certificado y estético.
    *   **Bajo costo:** Los interruptores de 433 MHz de pared cuestan entre $4 y $7 USD.
    *   **Larga duración:** Sus baterías de litio (CR2032) duran entre 2 y 3 años.
*   **Desventajas:** No hay confirmación de entrega (ACK) bidireccional nativa (el interruptor emite a ciegas, aunque a 4 metros y en 433 MHz la tasa de éxito es cercana al 100%).

### Estrategia B: Remoto a Medida (ESP32 + CC1101) $\rightarrow$ Central a Medida (ESP32 + CC1101)
*   **Cómo funciona:** Utilizas el microcontrolador Seeed Studio XIAO y un módulo CC1101 tanto en el emisor como en el receptor, creando tu propio protocolo RF bidireccional.
*   **Ventajas:** Permite confirmación de entrega (el central responde "recibido" y el remoto enciende un LED verde de éxito).
*   **Desventajas:** Aumenta la complejidad del hardware en ambos lados y requiere mayor tiempo de programación SPI.

---

## 3. Conexión del CC1101 al Seeed Studio XIAO (SPI)

El módulo CC1101 se comunica por el bus SPI. A continuación se muestra la conexión para un XIAO:

```
 [ Módulo CC1101 ]           [ XIAO ESP32-C6 ]
       VCC      ----------->      3V3
       GND      ----------->      GND
       CSN      ----------->      D1  (Chip Select)
       SCK      ----------->      D8  (SPI SCK)
       MOSI     ----------->      D10 (SPI MOSI)
       MISO     ----------->      D9  (SPI MISO)
       GD0      ----------->      D2  (Pin de Interrupción / Rx)
```
*\*Nota: Verifica que el módulo CC1101 que adquieras sea el modelo de 3.3V (la gran mayoría lo son).*

---

## 4. Software y Librerías para Programación

Para manejar el CC1101 en PlatformIO/Arduino de forma sencilla, existen librerías maduras:

1.  **`ELECHOUSE_CC1101` (de SmartSparks):** La librería más robusta para configurar los registros internos del CC1101 (frecuencia exacta, potencia de salida, modulación ASK/OOK o FSK).
2.  **`rc-switch`:** Ideal para la **Estrategia A**. Esta librería se integra con el CC1101 y permite decodificar automáticamente los códigos de 24 bits enviados por casi cualquier interruptor inalámbrico comercial de 433 MHz del mercado.

### Código de Ejemplo de Recepción (Central):
```cpp
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
    Serial.begin(115200);
    
    // Inicializar CC1101 en la frecuencia de 433.92 MHz
    if (ELECHOUSE_cc1101.getCC1101()) {
        Serial.println("CC1101 conectado correctamente.");
    } else {
        Serial.println("Error de conexión con CC1101.");
    }
    
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRx(); // Configurar en modo receptor
    
    // Asociar el pin GDO0 del CC1101 (conectado a D2 del XIAO) a rc-switch
    mySwitch.enableReceive(2); // D2 es el pin de interrupción
    Serial.println("Esperando señales RF 433MHz...");
}

void loop() {
    if (mySwitch.available()) {
        long value = mySwitch.getReceivedValue();
        if (value == 0) {
            Serial.println("Error de codificación.");
        } else {
            Serial.printf("Señal Recibida: %ld | Protocolo: %d\n", value, mySwitch.getReceivedProtocol());
            
            // Lógica de conmutación basada en el código único del control
            if (value == 1234567) { // Reemplazar con el código de tu interruptor
                // Toggle Luces Zona 1
            }
        }
        mySwitch.resetAvailable();
    }
}
```
