# Análisis de Eficiencia Energética: Sistema a Medida vs. Plataformas Comerciales

Este análisis técnico calcula y compara el consumo de energía en estado de reposo (standby) de tu propuesta original (basada en ESP-NOW directo) frente a las alternativas basadas en servidores como Home Assistant o VanPi.

---

## 1. Cálculos de Consumo: Tu Propuesta (ESP-NOW Directo)

Tu propuesta de comunicación directa punto a punto sin router Wi-Fi es el diseño de menor consumo posible para control inalámbrico.

### Unidad Central (XIAO ESP32-C6) - Siempre Encendido (RX)
*   Al no requerir estar asociado a un router Wi-Fi ni escuchar beacons de red constantemente, el ESP32-C6 opera en un modo de radio extremadamente ligero.
*   **Consumo promedio del ESP32-C6 (RX activo):** ~35 mA a 3.3V $\approx$ **0.11 Watts**.
*   **Pérdida por conversión Buck (Eficiencia 85% de 12V a 5V/3.3V):** El consumo a la batería de 12V es de ~11 mA.
*   **Consumo diario:** 
    $$\text{0.11W} \times 24\text{h} = 2.64\text{Wh por día}$$
    $$\text{En Ampere-hora (Ah) a 12V:} \approx \mathbf{0.22\text{ Ah/día}}$$

### Módulo Remoto (XIAO ESP32-C6) - Deep Sleep (TX)
*   **Modo Sleep (99.9% del tiempo):** ~15 $\mu$A.
*   **Modo Activo (durante la transmisión, ~150 ms):** ~100 mA.
*   **Consumo diario:** Prácticamente despreciable. Una batería AA estándar (2000 mAh) durará varios años.

### **Total Consumo en Reposo del Sistema:** ~0.15 W ($\approx$ **0.25 Ah/día** de tu batería de 12V).

---

## 2. Cálculos de Consumo: Alternativas (Home Assistant / VanPi)

Para que Home Assistant o VanPi funcionen, se requiere una infraestructura de red encendida de forma permanente en la van.

### Servidor Central (Raspberry Pi 4 o Pi 5) - Siempre Encendido
*   Una Raspberry Pi corriendo el sistema operativo y gestionando la base de datos consume de media entre 3.5W y 5W en reposo (sin periféricos exigentes).
*   **Consumo diario:**
    $$\text{4W} \times 24\text{h} = 96\text{Wh por día}$$
    $$\text{En Ah a 12V:} \approx \mathbf{8\text{ Ah/día}}$$

### Router Wi-Fi de 12V (Requerido para la Red de la Van)
*   Un router de viaje de 12V (como los comunes GL.iNet) consume en promedio entre 3W y 5W.
*   **Consumo diario:**
    $$\text{4W} \times 24\text{h} = 96\text{Wh por día}$$
    $$\text{En Ah a 12V:} \approx \mathbf{8\text{ Ah/día}}$$

### Receptores ESP32 (ESPHome)
*   Al requerir mantener una conexión Wi-Fi activa y enlazada al router las 24 horas, el ESP32 consume unos 80-100 mA de media.
*   **Consumo diario por nodo:** $\approx$ **2 Ah/día**.

### **Total Consumo en Reposo del Sistema:** ~10W a 15W ($\approx$ **20 a 30 Ah/día** de tu batería de 12V).

---

## 3. Comparación de Impacto en la Batería de la Van

Si tu van está estacionada sin recibir sol (paneles solares bloqueados por sombra o clima de invierno) y tienes una batería común de servicio de Litio (LiFePO4) de **100 Ah**:

| Métrica de Impacto | Tu Propuesta (ESP-NOW) | Sistema Servidor (Home Assistant / Pi) |
| :--- | :--- | :--- |
| **Potencia en Standby** | **0.15 Watts** | ~10 Watts (66 veces más) |
| **Consumo Diario (a 12V)** | **0.25 Ah / día** | ~20 Ah / día |
| **Días hasta agotar la batería** | **400 días** (Más de un año) | **5 días** |
| **Necesidad de apagado total** | No es necesario apagarlo nunca. | Debes apagar el servidor al guardar la van. |

---

## 4. Conclusión Técnica

**No, los sistemas basados en servidores no son más eficientes. Tu propuesta a medida con ESP-NOW es radicalmente más eficiente (consume casi 100 veces menos energía en reposo).**

*   **¿Cuándo se justifican Home Assistant o VanPi?** Únicamente si ya tienes planeado tener un router Wi-Fi 4G/5G encendido 24/7 y una minicomputadora activa en la van para trabajar en ruta o ver televisión vía streaming. En ese escenario, el consumo ya está "asumido".
*   **¿Cuándo es superior tu propuesta?** Si buscas un sistema **totalmente autónomo, de encendido instantáneo e inmune a cortes de red**, que no represente un drenaje silencioso sobre tus baterías cuando la van esté estacionada por días o semanas.
