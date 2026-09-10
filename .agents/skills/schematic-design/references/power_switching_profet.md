# High-Side Power Switching: Smart Switches (PROFETs) & Slew Rate Limits

Automotive body electronics and van conversions require switching positive rails (High-Side Switching) to prevent accidental ground-fault short circuits. This guide details design rules for Infineon PROFET+ (BTS series) smart power switches.

---

## 1. PROFET Architecture vs. Discrete MOSFETs

```
Infineon PROFET Smart Switch Internal Architecture:
                 +---------------- V_S (+12V Battery)
                 |
           +-----+-----+
           |  Charge   |
           |   Pump    |
           +-----+-----+
                 |
IN (GPIO) ---> [ Gate  ] ---> [ N-MOS Power DMOS ] ---> OUT (To 12V Load)
               [ Logic ]      [      Switch      ]
                 |                    |
DEN (GPIO) --> [Current] <------------+ (Current Mirror Ratio: k_ILIS = 8200)
               [ Mirror]
                 |
                 +--------------------> IS (Current / Fault Diagnostics)
                                              |
                                             [R_IS] 1.2k
                                              |
                                             GND
```

### 1.1 Why PROFETs Replace Discrete MOSFETs in Automotive Design
1. **Autonomous Fault Protection:** Built-in hardware current limiting, short-to-ground protection, and thermal shutdown ($150^\circ\text{C}-175^\circ\text{C}$) latch-off in $<10\,\mu\text{s}$ without MCU intervention.
2. **Inductive Load Demagnetization:** Active Zener clamp across Drain-Source ($V_{DS(AZ)} \approx 41\,\text{V}$) absorbs the inductive flyback energy of motors and relays safely without requiring external freewheeling diodes.
3. **Controlled Slew Rates:** Internal gate driver controls $dV/dt$ to eliminate ringing and comply with CISPR 25 Class 5 radiated emissions.

---

## 2. The Physics of Slew Rates & PWM Frequency Limits

### 2.1 Switching Times of BTS5008-1EKB
According to the Infineon BTS5008-1EKB datasheet:
- Turn-on delay + rise time ($t_{ON}$): **Typ 150 µs, Max 250 µs**
- Turn-off delay + fall time ($t_{OFF}$): **Typ 155 µs, Max 250 µs**
- Total transition time: $t_{trans} = t_{ON} + t_{OFF} \le 500\,\mu\text{s}$

### 2.2 Why 5 kHz PWM Causes Thermal Runaway
If driven with standard microcontroller 5 kHz PWM ($T_{PWM} = 200\,\mu\text{s}$):
$$T_{PWM} = 200\,\mu\text{s} < t_{trans} = 500\,\mu\text{s}$$
The device is permanently trapped transitioning through its linear (resistive) region.

Switching power dissipation per cycle:
$$P_{SW} = f_{PWM} \cdot V_S \cdot I_L \cdot \left(\frac{t_{rise} + t_{fall}}{2}\right)$$
For $V_S = 13.5\,\text{V}$, $I_L = 5\,\text{A}$, $t_{rise} = t_{fall} = 50\,\mu\text{s}$:
- **At 200 Hz:**
  $$P_{SW} = 200 \times 13.5 \times 5 \times 50\times 10^{-6} = 0.675\,\text{W} \quad (\text{Safe, easy PCB cooling})$$
- **At 5000 Hz:**
  $$P_{SW} = 5000 \times 13.5 \times 5 \times 50\times 10^{-6} = 16.88\,\text{W}!$$
  With standard thermal resistance $R_{\theta JA} \approx 35^\circ\text{C/W}$:
  $$\Delta T = 16.88\,\text{W} \times 35^\circ\text{C/W} = 590^\circ\text{C} \implies \text{DESTRUCTION}$$

> [!IMPORTANT]
> **Mandatory Rule:** Set PWM frequency for PROFET switches between **100 Hz and 400 Hz** (nominally **200 Hz** for automotive cabin lighting and dimming).

---

## 3. Current Sensing (IS) & Microcontroller ADC Protection

### 3.1 Sense Resistor Calculation
The IS pin mirrors load current:
$$I_S = \frac{I_L}{k_{ILIS}}$$
For BTS5008-1EKB, nominal $k_{ILIS} = 8200$. For a maximum load current $I_L = 15\,\text{A}$:
$$I_S = \frac{15\,\text{A}}{8200} = 1.83\,\text{mA}$$
To map to the linear range of an ESP32 ADC ($V_{ADC} \approx 2.2\,\text{V}$):
$$R_{IS} = \frac{2.2\,\text{V}}{1.83\,\text{mA}} \approx 1.2\,\text{k}\Omega$$

### 3.2 Overvoltage Fault Clamp Protection
During a fault condition (short circuit, thermal shutdown, or open load), the PROFET drives a constant fault current into the IS pin:
$$I_{S(FAULT)} \approx 4.5\,\text{mA} - 7.0\,\text{mA}$$
Across $R_{IS} = 1.2\,\text{k}\Omega$:
$$V_{IS(FAULT)} = 7.0\,\text{mA} \times 1.2\,\text{k}\Omega = 8.4\,\text{V}$$
Because $8.4\,\text{V}$ exceeds the ESP32 absolute maximum rating ($3.6\,\text{V}$), an unprotected ADC pin will be destroyed.

```
PROFET IS Pin Microcontroller Protection Circuit:
  PROFET IS Pin ---+---[ R_series: 2.2k ]---+---> ESP32 ADC (GPIO)
                   |                        |
                 [R_IS] 1.2k            [D_clamp] BAT54S (Dual Schottky to 3.3V / GND)
                   |                        |
                  GND                      GND
```

1. **Dual Schottky Clamp (BAT54S):** Clamps voltages $>3.6\,\text{V}$ to the 3.3V rail and $<-0.3\,\text{V}$ to GND.
2. **Series Resistor ($R_{series} \ge 2.2\,\text{k}\Omega$):** Restricts the maximum current injected into the 3.3V rail to $<1.5\,\text{mA}$.
3. **Diagnostic Enable (DEN):** When `DEN = LOW`, the IS pin is high-impedance. Firmware must assert `DEN = HIGH` before taking an ADC measurement.
