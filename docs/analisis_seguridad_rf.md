# Análisis de Seguridad: 433 MHz vs. ESP-NOW (2.4 GHz)

Este documento analiza las implicaciones de seguridad inalámbrica, vulnerabilidades (especialmente ataques de replay y sniffing) y estrategias de mitigación al utilizar la frecuencia de 433 MHz en comparación con ESP-NOW.

---

## 1. Comparativa de Seguridad Inalámbrica

| Característica | 433 MHz Comercial (ASK/OOK - rc-switch) | ESP-NOW (2.4 GHz) | 433 MHz con Código Custom (CC1101 + AES) |
| :--- | :--- | :--- | :--- |
| **Encriptación** | **Ninguna (Por defecto)** | Sí (Soporta WPA2 / AES-CCMP de forma nativa) | Sí (Se puede implementar AES-128 en software) |
| **Tipo de Código** | Estático (Siempre envía el mismo ID de 24 bits) | Dinámico / Encriptado | Dinámico (Rolling Code o cifrado por software) |
| **Vulnerabilidad a Sniffing** | **Muy Alta** (Cualquiera con un receptor de $2 puede leer el código) | Muy Baja (Tráfico encriptado) | Baja (El tráfico se puede leer pero no descifrar) |
| **Ataque de Replay (Reenvío)** | **Muy Alta** (Un Flipper Zero o HackRF puede grabar y retransmitir el código) | Inmune (Usa números de secuencia y encriptación) | Inmune (Si se implementa Rolling Code) |
| **Interferencias / Falsos disparos**| Media (Es una banda pública muy saturada) | Muy Baja | Baja |

---

## 2. El Riesgo Real en una Camper Van

Si utilizas la **Estrategia A** (Central ESP32 + CC1101 y pulsadores comerciales de 433 MHz sin encriptar), el código transmitido es estático. Esto introduce vulnerabilidades específicas según la carga controlada:

### Cargas de Bajo Riesgo (Luces, Ventilador)
*   **Impacto:** **Mínimo**. Si alguien clona el código de tus luces, lo peor que puede pasar es que las encienda o apague desde el exterior de la van a modo de broma. No representa un riesgo de seguridad física.

### Cargas de Alto Riesgo (Bomba de Agua, Inversor Multiplus II)
*   **Impacto:** **Grave**.
    *   **Bomba de Agua:** Si un atacante clona el código y enciende la bomba de agua de forma remota mientras estás fuera de la van, podría vaciar tu tanque de agua limpia, inundar la van (si dejaste un grifo mal cerrado) o **quemar el motor de la bomba** al hacerla funcionar en seco por horas.
    *   **Inversor:** Podrían encender el inversor a distancia, consumiendo y drenando tu batería de servicio silenciosamente.

---

## 3. Decisión de Diseño: Descarte de Switches Comerciales de 433 MHz

**Bajo esta evaluación de riesgos, se descarta oficialmente el uso de interruptores comerciales de 433 MHz (código estático) para controlar cualquier elemento de la van.** 

El riesgo de activación no deseada de la bomba de agua o del inversor (con consecuencias de inundación o descarga total de baterías) supera los beneficios de simplicidad de dicha solución.

---

## 4. Alternativas de Reemplazo Seguras

Para mantener la seguridad criptográfica del sistema, nos enfocaremos en las siguientes dos rutas:

### Ruta A: Remotos a Medida con ESP-NOW Encriptado (Nuestra Ruta Activa)
*   **Hardware:** Módulo central y remotos basados en **Seeed Studio XIAO ESP32-C6**.
*   **Seguridad:** Implementación de cifrado nativo ESP-NOW (AES-CCMP) o encriptación simétrica ligera por software con clave precompartida (PSK) en la memoria flash.
*   **Penetración de señal:** Mitigado mediante el uso de una variante del ESP32 central con **antena externa** llevada fuera del gabinete de chapa metálica.

### Ruta B: Interruptores Comerciales Encriptados (Zigbee/BLE) + Servidor
*   **Hardware:** Pulsadores inalámbricos **Zigbee (Aqara/IKEA)** o **Bluetooth (Shelly BLU)** comerciales, comunicados con una Raspberry Pi/Home Assistant.
*   **Seguridad:** Cifrado nativo AES-128 de grado industrial. Los códigos cambian y están protegidos contra replay attacks de forma nativa.
*   **Desventaja:** Requiere mantener encendida la Raspberry Pi y el receptor 24/7 (consumo de ~8-10 Ah/día).
