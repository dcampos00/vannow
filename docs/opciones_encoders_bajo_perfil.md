# Opciones de Encoders Rotativos de Bajo Perfil (5 mm)

Para lograr un panel de control ultraplano en las paredes de tu van (con una elevación final de unos 5 mm respecto al panel de madera o acrílico), debemos seleccionar componentes con montaje superficial (SMD) o del tipo rueda lateral.

A continuación se analizan las mejores opciones del mercado que cumplen con esta restricción de espacio e integran el botón pulsador:

---

## 1. Opciones de Componentes Recomendadas

### Opción A: Encoders Tipo "Dial" o Rueda Lateral (Ej. Panasonic EVQ-WGD001)
Son encoders diseñados para montarse de forma horizontal en la PCB, de modo que solo sobresale una pequeña rueda plana (como la rueda de volumen de los antiguos reproductores de MP3 o cámaras).
*   **Dimensiones:** El grosor del componente sobre la PCB es de apenas **3.4 mm a 4 mm**.
*   **Funcionamiento:** Giras la perilla plana desde el borde y la presionas hacia adentro para activar el interruptor integrado (click).
*   **Estética:** Ideal si quieres un panel moderno donde las perillas no sobresalgan hacia el frente, sino que estén empotradas en una ranura.

### Opción B: Encoders Rotativos SMD de Eje Corto (Ej. Alps Alpine EC12E / Bourns PEC11L)
Son encoders verticales tradicionales pero optimizados para montaje superficial (SMD), eliminando el casquillo de rosca alto de los modelos estándar.
*   **Cómo lograr los 5 mm:** Debes buscar modelos con **longitud de eje (L) de 9.5 mm o 10 mm**.
*   **Montaje empotrado:** Si tu panel de madera o plástico tiene un espesor de 3 mm a 4 mm, y el encoder se monta directamente en la PCB detrás del panel, solo sobresaldrán unos 5 mm o 6 mm de eje.
*   **Perillas personalizadas (Knobs):** Puedes usar una perilla de perfil ultra-bajo de aluminio (de 10 mm de diámetro y 5 mm de altura) que abrace el eje por completo y quede casi al ras del panel frontal.

### Opción C: Encoders de Eje Hueco (Hollow Shaft Encoders - Ej. Alps EC10E Series)
Estos encoders no tienen un eje que sobresalga hacia arriba; en su lugar, tienen un orificio central engranado en el cual encajas un dial plano o perilla giratoria.
*   **Altura del cuerpo:** Solo **4 mm** de grosor sobre la PCB.
*   **Ventaja:** Permiten diseñar una rueda de control completamente plana sobre el panel (como el dial de control de un termostato Nest o una vitrocerámica).

---

## 2. Comparativa de Encoders para Prototipado y PCB Final

| Parámetro | Alps EC11 Standard (Descartado) | Alps EC12E SMD (Eje 9mm) | Panasonic EVQ-WGD (Rueda) | Alps EC10E (Eje Hueco) |
| :--- | :--- | :--- | :--- | :--- |
| **Altura del Cuerpo** | 6.5 mm (cuerpo) + 15mm (eje) | 5.5 mm (cuerpo) + 9mm (eje) | **3.4 mm (total)** | **4.0 mm (total)** |
| **Tipo de Montaje** | Trough-hole (Agujero pasante) | SMD (Montaje superficial) | SMD | SMD |
| **Botón Pulsador** | Sí | Sí | Sí | Sí |
| **Facilidad de Prototipado** | Muy Fácil (compatible con protoboard) | Media (requiere placa adaptadora) | Difícil (SMD muy pequeño) | Media (SMD) |
| **Perfil Final Expuesto** | > 15 mm | **~5 mm (con panel y perilla corta)** | **~3 mm** | **~2 mm (dial plano)** |

---

## 3. Estrategia Sugerida para tu Diseño

Para tus pruebas y la implementación final, te sugiero la siguiente ruta:

1.  **Para el Prototipo:** 
    Usa un encoder **EC11 estándar** (con eje largo) en tu protoboard. El software, la lógica de interrupciones del ESP32-C6 y la comunicación inalámbrica por ESP-NOW son **exactamente idénticos** sin importar el tamaño físico del encoder. Esto te evitará tener que soldar componentes SMD diminutos durante las pruebas de código.
2.  **Para el Diseño de PCB Final (en Atopile):**
    Diseñaremos la placa del remoto usando la huella (footprint) del **Alps EC12E de 9mm** o el **Panasonic EVQ-WGD** para montaje superficial.
3.  **Montaje Mecánico:**
    Montaremos la PCB directamente atornillada detrás de la placa de pared de la van, de modo que solo la perilla ultraplana de 5 mm de altura sobresalga a través del orificio del panel.
