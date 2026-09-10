# Automotive 12V DC Power Entry & Transient Protection

Vehicular 12V electrical systems (nominal 13.8V, charging up to 14.4V) present one of the harshest electrical environments in electronics design. Circuitry must withstand continuous voltage fluctuations, inductive kicks, and alternator load dump surges governed by **ISO 7637-2:2011** and **ISO 16750-2:2012**.

---

## 1. Power Entry Block Diagram

```
+12V VEHICLE BATTERY
        |
     [ FUSE ] (15A - 30A Fast Blow)
        |
        v
+-------+-----------------------------------------------------------+
| REVERSE POLARITY PROTECTION (P-MOSFET)                            |
|       +---[ Source ]---[ P-FET ]---[ Drain ]----+----> +12V_PROT  |
|       |                  |                    |                   |
|      [Z1] 15V Zener     [R_G] 100k            |                   |
|       |                  |                    |                   |
|       +--------+---------+                    |                   |
|                |                              |                   |
|               [R1] 10k                        |                   |
|                |                              |                   |
+----------------+------------------------------+                   |
                 |                                                  |
                GND                                                 v
+-------------------------------------------------------------------+
| TRANSIENT SUPPRESSION (TVS)                                       |
|       +12V_PROT ---+---> To PROFETs (BTS5008-1EKB)                |
|                    |                                              |
|                  [TVS] Automotive DO-218AB (SM8S24A)              |
|                    |   (V_RWM = 24V, V_C = 38.9V)                 |
|                   GND                                             |
+--------------------+----------------------------------------------+
                     |
                     v
+-------------------------------------------------------------------+
| LC PI-FILTER & MIDDLEBROOK DAMPING (CISPR 25)                     |
|       +12V_PROT ---[ L1 10uH ]---+------------+---> BUCK VIN      |
|                                  |            |     (MP1584EN)    |
|                                [C_in1]      [C_d] 47uF (Elec)     |
|                                10uF X7R       |                   |
|                                (50V)        [R_d] 1.0 Ohm         |
|                                  |            |                   |
|                                 GND          GND                  |
+-------------------------------------------------------------------+
```

---

## 2. Reverse Polarity Protection

### 2.1 Comparison: Schottky Diode vs. P-MOSFET

| Parameter | Series Schottky Diode (e.g., MBR1045) | High-Side P-MOSFET (e.g., DMP3010LK3) |
| :--- | :--- | :--- |
| **Forward Drop ($V_f$)** | $0.4\,\text{V} - 0.7\,\text{V}$ | $I_L \cdot R_{DS(\text{on})} \approx 0.05\,\text{V}$ |
| **Power Dissipation at 10A** | $5.0\,\text{W} - 7.0\,\text{W}$ (Hot, needs heatsink) | $0.5\,\text{W} - 1.0\,\text{W}$ ($R_{DS(\text{on})} = 8\,\text{m}\Omega$) |
| **High Temperature Leakage** | Severe risk of thermal runaway ($I_R > 30\,\text{mA}$ at $125^\circ\text{C}$) | Completely safe; channel held off by gate oxide |

### 2.2 P-MOSFET Implementation & Gate Oxide Protection
- **Orientation:** Connect **Source to Input ($V_{IN}$)** and **Drain to Protected Rail ($V_{PROT}$)**.
- **Normal Operation:** When $+12\,\text{V}$ is connected, current flows through the P-FET body diode, pulling Source to $11.3\,\text{V}$. Resistor $R_1$ pulls the Gate toward GND. $V_{GS}$ drops to $-11.3\,\text{V}$, driving the MOSFET into full saturation and shorting out the body diode ($V_{DS} \approx 0\,\text{V}$).
- **Reverse Connection:** When $-12\,\text{V}$ is connected to $V_{IN}$, Source is at $-12\,\text{V}$ while Gate is at $0\,\text{V}$ via $R_1$. Thus $V_{GS} > 0\,\text{V}$. The MOSFET channel remains off, and the body diode is reverse-biased, blocking all reverse current.
- **Critical Gate Breakdown Rule ($V_{GS,\max}$):**
  - MOSFET gate oxide ruptures at $V_{GS} > 20\,\text{V}$.
  - During automotive load dump surges ($+35\,\text{V}$ to $+40\,\text{V}$), grounding the Gate directly would apply $V_{GS} = -35\,\text{V}$, destroying the MOSFET instantly.
  - **Mandatory Protection:** Place a **15V Zener diode ($Z_1$)** between Gate and Source, and a $10\,\text{k}\Omega$ resistor ($R_1$) from Gate to GND. The Zener clamps $V_{GS}$ safely to $-15\,\text{V}$.

---

## 3. Transient Voltage Suppression (TVS) for ISO 7637-2 / ISO 16750-2

Vehicular transient standards:
- **ISO 7637-2 Pulse 1 (Inductive kick):** $-100\,\text{V}$ to $-150\,\text{V}$, $2\,\text{ms}$ duration.
- **ISO 7637-2 Pulse 2a (Harness inductance):** $+37\,\text{V}$ to $+55\,\text{V}$, $50\,\mu\text{s}$ duration.
- **ISO 16750-2 Pulse 5b (Suppressed Load Dump):** Alternator voltage surge clamped to $35\,\text{V}$, $40 - 400\,\text{ms}$ duration.

### 3.1 TVS Selection Criteria
1. **Reverse Standoff Voltage ($V_{RWM}$):** Must be $\ge 24\,\text{V}$ to prevent conduction during emergency 24V jump-start events.
2. **Maximum Clamping Voltage ($V_C$):** Must be strictly below the maximum breakdown voltage of downstream silicon:
   - Infineon PROFET BTS5008-1EKB active clamp: $V_{S(AZ)} = 41\,\text{V}$. The external TVS **must clamp below 39V** to prevent the PROFET from absorbing the vehicle's entire load dump energy.
3. **Recommended Component:**
   - **SM8S24A** or **SLD8S24A** (DO-218AB package, $6600\,\text{W}$ peak pulse power, $V_{RWM} = 24\,\text{V}$, $V_{BR} = 26.7\,\text{V} - 29.5\,\text{V}$, $V_C = 38.9\,\text{V}$ at peak surge).

---

## 4. Buck Converter $\pi$-Filter & Middlebrook Stability Criterion

Switching buck regulators behave as **constant-power loads**, which exhibit a negative incremental input resistance:
$$R_{in\_neg} = -\frac{V_{IN}^2 \cdot \eta}{P_{OUT}}$$
For $V_{IN} = 12\,\text{V}$, $P_{OUT} = 15\,\text{W}$ (5V at 3A), $\eta = 0.85$:
$$R_{in\_neg} = -\frac{144 \times 0.85}{15} = -8.16\,\Omega$$

### 4.1 Filter Damping Requirement
An undamped LC $\pi$-filter with ceramic MLCC capacitors has an impedance peak:
$$Z_{out,peak} = Q \cdot \sqrt{\frac{L}{C_{in}}}$$
High-$Q$ ceramic capacitors can produce $Z_{out,peak} > 40\,\Omega$. When $\|Z_{out,filter}\| \ge \|R_{in\_neg}\|$, the circuit oscillates, resulting in output ripple, audible whining, or buck regulator shutdown.

### 4.2 Solution: RC Parallel Damping
Add an electrolytic damping capacitor $C_d$ with series damping resistance $R_d$:
$$C_d \ge 4 \cdot C_{in2} \approx 47\,\mu\text{F}$$
$$R_d \approx \sqrt{\frac{L}{C_{in2}}} \approx 1.0\,\Omega$$
This clamps the maximum filter output impedance to $R_d \approx 1\,\Omega \ll 8.16\,\Omega$, ensuring unconditional stability.

---

## 5. MLCC Ceramic Capacitor DC Bias Derating

Class II MLCC dielectrics (X5R, X7R) experience drastic capacitance loss under DC bias voltage:
- A $10\,\mu\text{F}$, 25V 0805 X7R capacitor loses **$60\% - 75\%$** of its capacitance at 14V DC (actual effective capacitance $C_{eff} \approx 2.5\,\mu\text{F} - 3.5\,\mu\text{F}$).
- **Design Rule:** Always specify capacitors rated for **$\ge 50\,\text{V}$** on 12V/14V automotive lines, and choose 1206 or 1210 package sizes for bulk filtering.
