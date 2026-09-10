# Ultra-Low-Power Wake-Up Architectures: LP-GPIO & Diode-OR Matrices

Battery-powered remote switches (e.g., powered by 2x AA batteries for multi-year lifespan) require sub-$20\,\mu\text{A}$ deep sleep currents. This guide covers power domain partitioning, pin limitations, and hardware Diode-OR wakeup circuits for ESP32 microcontrollers.

---

## 1. Power Domain Partitioning: Low-Power (LP) vs. High-Power (HP)

```
ESP32-C6 Power Domain Architecture:
+-------------------------------------------------------------+
| ALWAYS-ON (AON) / LOW-POWER (LP) DOMAIN (~15 uA Deep Sleep)  |
|   - LP Core (RISC-V)                                        |
|   - LP Memory                                               |
|   - LP-GPIOs (GPIO 0 through GPIO 7)                        |
|   - Wakeup Controller (EXT0 / EXT1)                         |
+-------------------------------------------------------------+
                               | (Power Gated in Deep Sleep)
                               v
+-------------------------------------------------------------+
| HIGH-POWER (HP) DOMAIN (Powered OFF in Deep Sleep, 0 mA)    |
|   - Main HP Core (160 MHz RISC-V)                           |
|   - HP SRAM                                                 |
|   - Wi-Fi 6 / Bluetooth 5 (LE) Subsystems                   |
|   - HP-GPIOs (GPIO 8 through GPIO 23)                       |
+-------------------------------------------------------------+
```

### 1.1 The Seeed Studio XIAO ESP32-C6 Header Trap
The Seeed Studio XIAO ESP32-C6 header exposes only 11 pins:

| Header Pin | ESP32-C6 GPIO | Power Domain | Deep Sleep Wakeup Capable? | Notes |
| :---: | :---: | :---: | :---: | :--- |
| **D0** | GPIO 0 | **LP Domain** | **YES** (`EXT1`) | Available on header |
| **D1** | GPIO 1 | **LP Domain** | **YES** (`EXT1`) | Available on header |
| **D2** | GPIO 2 | **LP Domain** | **YES** (`EXT1`) | Available on header |
| **D3** | GPIO 21 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D4** | GPIO 22 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D5** | GPIO 23 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D6** | GPIO 16 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D7** | GPIO 17 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D8** | GPIO 19 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D9** | GPIO 20 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |
| **D10** | GPIO 9 | **HP Domain** | ❌ **NO** | Cannot wake chip from Deep Sleep |

> [!WARNING]
> On the XIAO ESP32-C6:
> - **GPIO 3** is consumed internally by Seeed for the RF Wi-Fi switch (`WIFI_ENABLE`).
> - **GPIO 6** is only an unpopulated SMD test pad on the bottom of the PCB.
> - **Only 3 LP-GPIOs (D0, D1, D2)** are accessible on the physical header.
> - Connecting a 4th switch directly to **D3 (GPIO 21)** will **fail silently** in deep sleep—the remote will never wake up when the 4th switch is pressed.

---

## 2. Hardware Diode-OR Wakeup Matrix

If a project strictly requires 4 physical pushbuttons while retaining single-digit microamp deep sleep:

```
4-Button Diode-OR Wakeup Circuit:
                  +3.3V
                    |
               [R_pull: 47k - 100k]
                    |
                    +---------------------------------------------> LP_GPIO0 (Wakeup Interrupt Pin)
                    |               |               |               |
                  [D1]            [D2]            [D3]            [D4]  (BAT54C / BAS70-04)
                    |               |               |               |
                    +---------------+---------------+---------------+
                    |               |               |               |
                   SW1             SW2             SW3             SW4  (Momentary Tactile Switches)
                    |               |               |               |
                    +---------------+---------------+---------------+
                    |               |               |               |
                   GND             GND             GND             GND
                    |               |               |               |
                    +--------+      +--------+      +--------+      +--------+
                             |               |               |               |
                             v               v               v               v
                         HP_GPIO21       HP_GPIO22       HP_GPIO23       HP_GPIO16
                        (Header D3)     (Header D4)     (Header D5)     (Header D6)
```

### 2.1 Principle of Operation
1. **Quiescent Deep Sleep:** Switches are open. The line connected to `LP_GPIO0` is pulled up to 3.3V via $R_{pullup}$. Diodes D1–D4 are reverse biased. Current consumption is strictly diode reverse leakage ($<400\,\text{nA}$).
2. **Button Press:** When any button (e.g., SW2) is pressed to GND, Diode D2 is forward biased, pulling `LP_GPIO0` down to $V_f \approx 0.3\,\text{V} < V_{IL} (0.8\,\text{V})$. The LP core detects an `EXT1` low transition and boots the chip in $<2\,\text{ms}$.
3. **Pin Sensing:** Once awake, firmware initializes the HP domain, activates internal pull-ups on HP_GPIOs (D3–D6), and reads the digital inputs to identify which specific button is pressed.

### 2.2 Sizing the Pull-Up Resistor for Automotive Temperatures
In hot vehicle cabins ($70^\circ\text{C} - 85^\circ\text{C}$), Schottky reverse leakage increases:
- Using `BAS70-04` diodes ($I_R < 2\,\mu\text{A}$ at $85^\circ\text{C}$):
  $$\Delta V_{leakage} = 4 \times 2\,\mu\text{A} \times R_{pullup}$$
- To prevent false wakeups, $\Delta V_{leakage}$ must stay below $0.4\,\text{V}$:
  $$R_{pullup} \le \frac{0.4\,\text{V}}{8\,\mu\text{A}} = 50\,\text{k}\Omega$$
- **Design Rule:** Use $R_{pullup} = 47\,\text{k}\Omega$ to $100\,\text{k}\Omega$ for automotive grade reliability.

---

## 3. 7-Button Binary Diode Matrix (Exclusive D0, D1, D2)

If only the 3 LP-GPIO pins (`D0`, `D1`, `D2`) are used without requiring any HP pins:

```
7-Button Binary Diode Matrix:
                     +3.3V
                       |
               [Pull-ups 47k-100k]
               +-------+-------+
               |       |       |
              D0      D1      D2   (LP-GPIO Pins on ESP32-C6)
               |       |       |
SW1 (Code 1) --|>|-----+-------+----[ SW1 ]---> GND  (D0 = LOW)
               |       |       |
SW2 (Code 2) --+-------|>|-----+----[ SW2 ]---> GND  (D1 = LOW)
               |       |       |
SW3 (Code 3) --|>|-----+       |
               +-------|>|-----+----[ SW3 ]---> GND  (D0 & D1 = LOW)
               |       |       |
SW4 (Code 4) --+-------+-------|>|--[ SW4 ]---> GND  (D2 = LOW)
               |       |       |
SW5 (Code 5) --|>|-----+       |
               +-------+-------|>|--[ SW5 ]---> GND  (D0 & D2 = LOW)
               |       |       |
SW6 (Code 6) --+-------|>|-----+
               +-------+-------|>|--[ SW6 ]---> GND  (D1 & D2 = LOW)
               |       |       |
SW7 (Code 7) --|>|-----+       |
               +-------|>|-----+
               +-------+-------|>|--[ SW7 ]---> GND  (D0, D1 & D2 = LOW)
```

### 3.1 Characteristics
- **Button Capacity:** 7 independent tactile pushbuttons.
- **Wakeup:** `ESP_EXT1_WAKEUP_ANY_LOW` triggers on any button press since every button pulls at least one pin to GND.
- **Standby Power:** 0 µA added. Diodes are reverse biased in sleep.
- **Decoding Latency:** $<1\,\mu\text{s}$ (3-bit register read: `(!d0 ? 1 : 0) | (!d1 ? 2 : 0) | (!d2 ? 4 : 0)`).

---

## 4. Expansion Architecture: Row $\times$ Column Matrix (Up to 16 Buttons)

For large 12- to 16-key panels:
1. **Shared Wakeup Line:** Connect all button contact nodes through individual low-leakage Schottky diodes to **D0** (LP-GPIO 0).
2. **Matrix Lines:** Connect rows and columns to available HP header pins (e.g. Rows: D3, D4, D5, D6; Columns: D7, D8, D9, D10).
3. **Sequence:**
   - Button pressed $\implies$ D0 pulled LOW $\implies$ ESP32-C6 wakes from Deep Sleep.
   - Firmware drives rows LOW sequentially and samples columns in $\approx 40\,\mu\text{s}$.
   - Protocol payload and central firmware remain 100% identical.
