# Evaluación de Costos: Sistema a Medida vs. Ecosistema VanPi (Pekaway)

Este análisis evalúa el costo económico estimado para construir tu propio sistema de control a medida en comparación con la adquisición y puesta en marcha del ecosistema VanPi (Pekaway), que es la solución comercial/open-source madura equivalente.

---

## 1. Estimación de Costos: Tu Sistema a Medida (DIY)

Calculamos el costo para armar una unidad central de 12 canales (con PCA9685 y PROFETs) y 2 paneles de interruptores remotos inalámbricos.

### A. Unidad Central (Gabinete)
*   **Microcontrolador:** 1x Seeed Studio XIAO ESP32-C6 $\rightarrow$ **$6 USD**
*   **Multiplexor I2C:** 1x PCA9685 (Módulo o chip) $\rightarrow$ **$3 USD**
*   **Regulador de Tensión:** 1x Convertidor Buck automotriz (12V a 5V) $\rightarrow$ **$3 USD**
*   **Etapa de Potencia:** 5x PROFETs (ej. BTS50085) + 5x MOSFETs de Canal P + optoacopladores $\rightarrow$ **$18 USD**
*   **Redundancia manual:** 5x Interruptores de palanca 3 posiciones (ON-OFF-AUTO) $\rightarrow$ **$10 USD**
*   **Caja y Borneras:** Fusibles, borneras de tornillo y caja plástica $\rightarrow$ **$15 USD**
*   **Fabricación de PCB + Envío (JLCPCB):** 5 placas de circuito impreso $\rightarrow$ **$20 USD**
*   **Ensamblaje SMT en fábrica (Opcional pero recomendado):** $\rightarrow$ **$25 USD**
*   *Subtotal Central:* **$100 USD**

### B. Módulos Remotos (2 Unidades)
*   **Microcontroladores:** 2x Seeed Studio XIAO ESP32-C6 $\rightarrow$ **$12 USD**
*   **Placas PCB:** Fabricadas en el mismo lote o por separado $\rightarrow$ **$10 USD**
*   **Componentes:** 8x Pulsadores táctiles + resistencias + portapilas AA $\rightarrow$ **$10 USD**
*   **Carcasas:** Impresión 3D (costo de filamento) $\rightarrow$ **$5 USD**
*   *Subtotal Remotos:* **$37 USD**

### **Costo Total Estimado del Sistema a Medida:** $\approx$ **$137 USD** (aprox. **125 EUR**).

---

## 2. Estimación de Costos: Ecosistema VanPi (Pekaway)

VanPi es un sistema basado en Raspberry Pi. Pekaway comercializa el hardware pre-ensamblado, aunque los esquemáticos son abiertos. Analizamos ambas opciones de adquisición:

### Opción A: Comprando el Hardware oficial de Pekaway (Plug & Play)
*   **VanPi Relay Board (Shield central de potencia):** $\rightarrow$ **129 EUR**
*   **Raspberry Pi 4 (2GB/4GB) + Fuente + Tarjeta SD:** (Cerebro del sistema) $\rightarrow$ **80 EUR**
*   **Pantalla Táctil Oficial VanPi de 7" (Opcional, muy común):** $\rightarrow$ **89 EUR**
*   **2x Módulos Remotos inalámbricos oficiales (IoT Bridge):** $\rightarrow$ **50 EUR** (25 EUR c/u)
*   *Costo Total Oficial:* $\approx$ **348 EUR** (aprox. **$380 USD**).

### Opción B: Construyendo el Hardware de VanPi tú mismo (DIY Pekaway)
*   **Fabricación de la PCB VanPi Relay Board + componentes sueltos:** $\rightarrow$ **$50 USD**
*   **Raspberry Pi 4 (2GB/4GB) + Accesorios:** $\rightarrow$ **$80 USD**
*   **2x Módulos Remotos (DIY ESP32):** $\rightarrow$ **$20 USD**
*   *Costo Total DIY VanPi:* $\approx$ **$150 USD** (aprox. **140 EUR**). *Nota: No incluye pantalla.*

---

## 3. Tabla Comparativa de Costos y Valor

| Característica | Tu Sistema a Medida (DIY) | VanPi Oficial (Pekaway) | VanPi Hazlo Tú Mismo (DIY) |
| :--- | :--- | :--- | :--- |
| **Costo Económico** | **Bajo (~125 EUR)** | Alto (~350 EUR) | Medio (~150 EUR) |
| **Consumo en Reposo** | **Casi Nulo (~0.25 Ah/día)** | Muy Alto (~20 Ah/día) | Muy Alto (~20 Ah/día) |
| **Esfuerzo de Construcción** | Alto (Diseño completo de PCB) | Muy Bajo (Ensamblar placas) | Alto (Fabricar Shield y Nodos) |
| **Facilidad de Integración** | A medida exacta de tu van | Genérica (adaptar a su estándar) | Genérica |
| **Garantía y Soporte** | Propia (tú conoces cada cable) | Comercial por Pekaway | Soporte de comunidad |
| **Interfaz de Usuario** | Pulsadores y control móvil simple | Pantalla táctil y Web App avanzada | Web App avanzada (en móvil) |

---

## 4. Conclusión y Recomendación

*   **El sistema a medida (nuestra ruta) es entre 2 y 3 veces más barato** que comprar el sistema oficial de VanPi y consume 100 veces menos energía.
*   **¿Cuándo elegir VanPi?** Si deseas una interfaz gráfica táctil en la pared de tu van de forma inmediata, no te importa el consumo de 20 Ah/día de la Raspberry Pi, y prefieres pagar más para no tener que diseñar la PCB de potencia central.
*   **¿Cuándo elegir el sistema a medida?** Si valoras la eficiencia energética absoluta, el control total de los costos, un tamaño de placa optimizado para tu gabinete y quieres aprender el proceso completo de diseño de hardware-as-code con Atopile.
