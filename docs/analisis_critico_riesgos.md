# Análisis Crítico, Factores de Riesgo y Plan de Mitigación

Este documento evalúa los riesgos del proyecto de conmutación inalámbrica a medida por ESP32 y detalla cómo mitigar cada uno de ellos para lograr un sistema seguro, confiable y mantenible.

---

## 1. Puntos Críticos y Estrategias de Mitigación

### 1.1. Seguridad y Riesgo de Incendio (Líneas de Potencia de 12V)
*   **El Riesgo:** Diseñar una PCB casera con pistas de cobre que transportan corrientes de hasta 10A continuos puede generar calor excesivo o fallas catastróficas por sobrecorriente o soldadura defectuosa.
*   **Mitigación Propuesta:** 
    *   **Separar Lógica y Potencia:** Diseñaremos la PCB del ESP32 central únicamente para **señales de control de bajo voltaje (3.3V / 5V)** con consumos inferiores a 0.2A. Esta placa será 100% segura y libre de calor.
    *   **Módulos de Potencia Ensamblados de Fábrica:** Utilizaremos módulos MOSFET multicanal o bloques de relés de estado sólido (SSR) de grado industrial/automotriz ya ensamblados y certificados, con pistas gruesas y disipadores de calor de fábrica. La placa central ESP32 solo enviará la señal digital a estos bloques de potencia mediante cables de señal delgados.

### 1.2. Mantenimiento y "Efecto Caja Negra" (Resalabilidad)
*   **El Riesgo:** Ser el único capaz de reparar el sistema si falla a mitad de un viaje, o la imposibilidad de vender la van debido a una electrónica indescifrable para un tercero.
*   **Mitigación Propuesta:**
    *   **Código Abierto:** Almacenar el código en un repositorio público (GitHub) con documentación clara.
    *   **Redundancia Física Total (ON-OFF-AUTO):** Instalaremos interruptores manuales de bypass de 3 posiciones (de palanca de grado industrial) en el gabinete.
        *   Si el ESP32 o el firmware fallan, cambias el interruptor a **ON** y la luz/bomba funciona directamente con 12V. Si el MOSFET falla en corto, lo cambias a **OFF**. Esto garantiza un 100% de operatividad manual en emergencias.

### 1.3. Vibración y Desgaste Mecánico en la Van
*   **El Riesgo:** Las soldaduras frías y las borneras de tornillo baratas se aflojan con la vibración de la carretera.
*   **Mitigación Propuesta:**
    *   **Montaje Antivibración:** Alojar la central en un gabinete montado sobre soportes amortiguadores de goma (**silentblocks**) o espuma de alta densidad.
    *   **Conectores Resistentes:** Usar borneras de presión con resorte (tipo WAGO o bornes push-in para PCB) en lugar de tornillos. Los bornes de resorte ejercen una presión constante que no se ve afectada por la vibración.
    *   **Fijación Química:** Aplicar silicona neutra o cola caliente en los conectores clave y componentes pesados para evitar fatiga de material en las soldaduras.

### 1.4. Interferencia y Pérdida de Señal (Jaula de Faraday)
*   **El Riesgo:** El metal del chasis y del gabinete eléctrico bloquea la señal inalámbrica de 2.4 GHz de ESP-NOW a pesar de la corta distancia (4 metros).
*   **Mitigación Propuesta:**
    *   **Antena Externa:** Utilizaremos una variante del chip ESP32 con conector **IPEX/U.FL** para antena externa. Llevaremos un cable coaxial delgado fuera del gabinete eléctrico metálico a una pequeña antena oculta detrás de un panel de madera.
    *   **Protocolo de Reintentos Rápido:** Configurar ESP-NOW para realizar hasta 5 reintentos automáticos si no recibe la confirmación (ACK) del central en menos de 10 ms.

---

## 2. Alternativas Comerciales Sugeridas

Si decides no fabricar el sistema a medida, aquí tienes los detalles de las alternativas comerciales maduras y dónde buscarlas:

### A. Receptores de Relés por Radiofrecuencia (RF 433 MHz)
Sistemas "llave en mano" y pre-certificados para conmutación inalámbrica de 12V. Utilizan frecuencias de 433 MHz, que tienen una penetración en metal muy superior al Wi-Fi.
*   **Marca Recomendada:** **eMylo 12V RF Switch** (o similares de las marcas *eMylo*, *QIACHIP* o *Solidrun*).
*   **Cómo buscar en Amazon:** 
    *   *Término:* `eMylo 12V RF relay switch 433MHz`
    *   *Término:* `Interruptor inalambrico 12V 4 canales`
*   **Características:** Incluyen el receptor de 12V (con carcasa plástica e ignífuga) y pulsadores inalámbricos de pared estéticos que funcionan con pilas por años. Cero código.

### B. Ecosistema Home Assistant / Venus OS + Interruptores Inteligentes
Si corres **Venus OS** (Victron) o **Home Assistant** en una Raspberry Pi central y quieres interruptores comerciales bonitos para pegar en la pared:
*   **Opción Bluetooth (BLE):** **Shelly BLU Button 1**
    *   Pulsador pequeño, elegante, con batería CR2032 de 2 años de duración. Emite señales Bluetooth que la Raspberry Pi o un ESP32 proxy reciben para apagar luces o cambiar el estado del inversor Multiplus II.
*   **Opción Zigbee (Altamente Recomendada):** **Aqara Wireless Mini Switch** o **IKEA RODRET / TRÅDFRI Dimmer**
    *   *Requisito:* Requieren un receptor Zigbee USB (como el *Sonoff Zigbee 3.0 USB Dongle Plus*, aprox. $15 USD) conectado a la Raspberry Pi de la van.
    *   *Ventaja:* Tienen una estética impecable que simula un interruptor de pared residencial, soportan clicks múltiples y regulan el brillo de las luces nativamente a través de Home Assistant.
