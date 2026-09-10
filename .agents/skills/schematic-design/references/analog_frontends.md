# Analog Front-Ends: Battery Telemetry & ADC Protection

Precision analog sensing on microcontrollers requires careful impedance matching and protection against high-voltage transients. This guide details design rules for measuring battery voltages using SAR ADCs (such as ESP32-S3 and ESP32-C6).

---

## 1. High-Precision Battery Divider Front-End

```
High-Precision Battery Telemetry Front-End:
                                                         3.3V Rail
                                                            ^
                                                            |
                                                        [ D_clamp: BAT54S ]
                                                            |
 V_BAT (+12V) ---[ R_high: 100k ]---+---[ R_series: 1k ]----+---> ESP32 ADC (GPIO1)
                                    |                       |
                             [ R_low: 18k ]            [ C_f: 100nF ]
                                    |                       |
                                   GND                     GND
```

---

## 2. Divider Sizing & Thévenin Source Impedance

### 2.1 Voltage Ratio
To scale an automotive battery voltage ($V_{BAT,\max} = 18.0\,\text{V}$) into the linear dynamic range of the ESP32 ADC ($V_{ADC} \le 2.45\,\text{V}$ with 11 dB attenuation):
$$K = \frac{R_{low}}{R_{high} + R_{low}} = \frac{18\,\text{k}\Omega}{100\,\text{k}\Omega + 18\,\text{k}\Omega} = 0.1525$$
At nominal float charge ($V_{BAT} = 14.4\,\text{V}$):
$$V_{ADC} = 14.4 \times 0.1525 = 2.20\,\text{V}$$

### 2.2 Thévenin Resistance
$$R_{Th} = R_{high} \parallel R_{low} = \frac{100\,\text{k}\Omega \times 18\,\text{k}\Omega}{118\,\text{k}\Omega} = 15.25\,\text{k}\Omega$$

---

## 3. SAR ADC Switched-Capacitor Settling & The Reservoir Capacitor

ESP32 SAR ADCs use a switched-capacitor track-and-hold circuit:
- Sampling capacitor: $C_{sample} \approx 2\,\text{pF} - 5\,\text{pF}$
- Internal switch resistance: $R_{sw} \approx 1\,\text{k}\Omega - 2\,\text{k}\Omega$
- Sampling window: $t_{sample} \approx 1\,\mu\text{s} - 2\,\mu\text{s}$

### 3.1 The Sampling Error
During the $1\,\mu\text{s}$ sampling window, $C_{sample}$ must charge to within $\frac{1}{2}\,\text{LSB}$ ($0.012\%$) of the input voltage:
$$e^{-t_{sample} / \tau} \le \frac{1}{8192} \implies t_{sample} \ge 9.0 \cdot \tau$$
$$\tau = (R_{source} + R_{sw}) \cdot C_{sample}$$
If $R_{source} = 15.25\,\text{k}\Omega$, $\tau = (15.25\,\text{k}\Omega + 1.5\,\text{k}\Omega) \times 5\,\text{pF} = 83.7\,\text{ns} \implies 9\tau \approx 753\,\text{ns}$.
While close, any multiplexer switching transient or parasitic trace capacitance causes gain error and cross-channel crosstalk.

### 3.2 The Reservoir Capacitor Solution
Placing a **$100\,\text{nF}$ ceramic capacitor ($C_f$)** directly at the ADC pin:
1. **Charge Reservoir:** $C_f = 100\,\text{nF} \gg 8192 \cdot C_{sample} = 41\,\text{nF}$.
   $C_{sample}$ draws its sampling charge directly from $C_f$ in nanoseconds. The voltage droop on $C_f$ is negligible:
   $$\Delta V = V_{in} \cdot \frac{C_{sample}}{C_f + C_{sample}} = 2.2\,\text{V} \times \frac{5\,\text{pF}}{100\,\text{nF}} = 0.11\,\text{mV} \quad (< 0.15\,\text{LSB})$$
2. **Anti-Aliasing Filter:**
   $$f_c = \frac{1}{2\pi \cdot R_{Th} \cdot C_f} = \frac{1}{2\pi \times 15.25\,\text{k}\Omega \times 100\,\text{nF}} \approx 104.3\,\text{Hz}$$
   This completely suppresses 1 kHz – 3 kHz alternator ripple and high-frequency noise from buck converters.

---

## 4. Why Low-Voltage Zener Diodes Ruin ADC Accuracy

### 4.1 Quantum Tunneling vs. Avalanche Breakdown
- **Diodes with $V_Z > 5.6\,\text{V}$ (Avalanche):** Sharp knee, sub-nanoamp leakage ($I_R < 1\,\text{nA}$) below breakdown.
- **Diodes with $V_Z < 5.1\,\text{V}$ (Zener):** Dominated by quantum tunneling across an ultra-thin barrier. They have a **soft, rounded knee** with massive sub-breakdown leakage.

### 4.2 Quantitative Failure Case: 3.3V Zener (`BZX84-C3V3`)
At a normal measurement level ($V_{ADC} \approx 2.1\,\text{V} - 2.3\,\text{V}$), the BZX84-C3V3 leaks **$30\,\mu\text{A}$ to $60\,\mu\text{A}$**.

Across $R_{Th} = 15.25\,\text{k}\Omega$:
$$\Delta V_{ADC\_drop} = 50\,\mu\text{A} \times 15.25\,\text{k}\Omega = 0.7625\,\text{V}$$
Reflected back to the 12V battery reading ($K^{-1} = 6.556$):
$$\Delta V_{BAT\_error} = 0.7625\,\text{V} \times 6.556 \approx \mathbf{5.0\,\text{V}!}$$

> [!CAUTION]
> A 3.3V Zener diode will cause a healthy 14.4V battery to read as **$\approx 9.4\,\text{V}$**, falsely triggering low-voltage cutoffs. Because Zener leakage varies exponentially with ambient temperature, software calibration is mathematically impossible. **Never place low-voltage Zeners on ADC inputs.**

---

## 5. Correct Rail Clamping (BAT54S)

To protect the ADC against continuous overvoltage ($V_{BAT} > 24\,\text{V}$):
1. Use a dual Schottky diode (**BAT54S**) connected between GND and the 3.3V power rail.
2. Forward voltage $V_f \approx 0.32\,\text{V}$ clamps positive transients to $3.3\,\text{V} + 0.32\,\text{V} = 3.62\,\text{V}$, strictly within the ESP32 absolute maximum rating ($V_{DD} + 0.3\,\text{V}$).
3. Reverse leakage of BAT54 is $<100\,\text{nA}$ at $25^\circ\text{C}$, introducing an error of $<1.5\,\text{mV}$ ($<2\,\text{LSB}$).
4. Series resistor $R_{series} = 1\,\text{k}\Omega$ limits fault current injected into the 3.3V rail.
