# Proyectos Existentes y Soluciones Maduras para Camper Vans

Esta evaluación recopila y analiza los proyectos de código abierto más consolidados en el sector camper que resuelven exactamente la necesidad de centralizar el control de potencia, gestionar el inversor Victron y permitir control distribuido desde la cabina sin necesidad de diseñar todo desde cero.

---

## 1. VanPi (Ecosistema Pekaway) - La Solución Específica más Madura
**VanPi** es un proyecto alemán de código abierto desarrollado específicamente para la gestión eléctrica de casas rodantes.
*   **Cómo funciona:** Combina una unidad central basada en **Raspberry Pi** (que actúa como servidor local y centralizador) con **nodos ESP32** satélites para el control de relés y sensores.
*   **Características clave:**
    *   **Integración con Victron:** Se conecta de forma nativa a cargadores, shunts e inversores Victron (a través de VE.Direct por USB o RJ45) permitiendo encender/apagar el inversor y leer telemetría en tiempo real.
    *   **Control de Potencia:** Cuenta con placas de relés y dimmers MOSFET ya diseñadas y listas para fabricar o comprar.
    *   **Interfaz:** Dashboard web responsive integrado y soporte para pantallas táctiles de 7 pulgadas en la cabina o entrada.
*   **Veredicto:** Si quieres un ecosistema "llave en mano" pensado al 100% para furgonetas, es la alternativa más completa. Puedes usar su software libre y fabricar tu propio hardware o comprar sus placas prediseñadas.

---

## 2. Home Assistant + ESPHome (El Estándar DIY)
La gran mayoría de los instaladores camper avanzados optan por este combo debido a su inmensa flexibilidad y madurez.
*   **Cómo funciona:** Un mini PC o Raspberry Pi corre **Home Assistant** (servidor local sin internet) y los módulos ESP32 del gabinete se programan usando **ESPHome** (configuración declarativa en YAML).
*   **Por qué destaca en tu caso de uso:**
    *   **Interruptores comerciales listos para usar:** En lugar de fabricar controles remotos con baterías y C++, puedes comprar interruptores inalámbricos **Zigbee** o **Bluetooth** (de marcas como Aqara, IKEA, Tuya o Sonoff) por $5 a $10 USD. Ya vienen con un diseño estético impecable para paredes, funcionan con una pila de botón durante 2 años y se emparejan con Home Assistant con un par de clicks.
    *   **Control total del Multiplus II:** Home Assistant se conecta al inversor mediante **Modbus TCP** (a través del Cerbo GX o una Raspberry Pi con Venus OS) permitiendo cambiar el estado del inversor ("On", "Off", "Solo Cargador") desde cualquier panel físico o el móvil.
*   **Veredicto:** Es la ruta que requiere menos código y ofrece el acabado final más profesional, ya que te evitas fabricar físicamente la carcasa y electrónica de los interruptores remotos.

---

## 3. Victron Venus OS en Raspberry Pi (La Solución Oficial)
Victron Energy mantiene su sistema operativo de control (Venus OS) como **código abierto**. Puedes instalarlo gratis en una Raspberry Pi en lugar de comprar el costoso dispositivo físico Cerbo GX.
*   **Cómo funciona:** Conectas el inversor Multiplus II a la Raspberry Pi usando un cable USB-MK3. La Pi se convierte en el cerebro oficial de Victron.
*   **Control de cargas externas:** Venus OS permite configurar los pines GPIO de la Raspberry Pi (o tarjetas de relés USB conectadas a ella) como interruptores para la bomba de agua, nevera, etc.
*   **Veredicto:** Si tu prioridad número uno es controlar y monitorizar de forma segura y robusta el inversor Multiplus II y tus baterías de litio, esta es la base más confiable y oficial.

---

## 4. Comparativa de Rutas a Seguir

| Criterio | Ruta 1: Desarrollo a Medida (Nuestro plan actual) | Ruta 2: VanPi / Pekaway | Ruta 3: Home Assistant + ESPHome (Híbrido) |
| :--- | :--- | :--- | :--- |
| **Tiempo de desarrollo** | Alto (requiere diseñar PCB y código) | Medio (placas e interfaces prediseñadas) | **Bajo** (configuración visual y lógica YAML) |
| **Costo de Hardware** | Bajo (ESP32 y transistores sueltos) | Medio-Alto (comprar o mandar a hacer placas) | Bajo (aprovecha pulsadores Zigbee comerciales) |
| **Estética de los interruptores** | DIY (requiere imprimir carcasas 3D) | Pantalla táctil o Web App | **Premium** (interruptores comerciales de pared listos) |
| **Integración con Victron** | Media (requiere programar protocolo VE.Direct) | **Nativa y madura** | **Nativa y madura** (vía Modbus o Venus OS) |
| **Autonomía (Sin Router)** | **Excelente** (ESP-NOW directo) | Regular (crea su propio punto de acceso Wi-Fi) | Regular (requiere red Wi-Fi local dentro de la van) |

---

## Recomendación Final

Si sientes que el diseño completo desde cero es redundante, te sugiero evaluar la **Ruta 3 (Home Assistant + ESPHome + Botones Zigbee/BLE comerciales)**:
1.  **Gabinete Central:** Conservamos la placa ESP32 + MOSFETs/PROFETs controlados localmente. En lugar de C++ personalizado, usamos **ESPHome**, lo que reduce el software del central a un archivo de configuración de 50 líneas.
2.  **Control del Inversor:** Conectamos el Multiplus II a Home Assistant.
3.  **Remotos:** Descartamos fabricar placas para los pulsadores. Compras pulsadores inalámbricos comerciales (como los botones redondos Aqara o los interruptores planos de IKEA) que pegas en cualquier pared sin un solo cable.
