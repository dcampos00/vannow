# Cross-Domain Co-Design Verification Matrix

Authoritative 24-channel mapping after the 2026-09-11 logic audit. The 11-channel PROFET carrier (`VanCentralControllerPROFET`) is a legacy breadboard-scale design and must not be flashed with this firmware table.

---

## 1. Central Controller 24-Channel Alignment Matrix

| System Channel | Atopile Net / Stage | DevKit GPIO | Firmware (`DEFAULT_24CH_CONFIG`) | Type | Notes |
| :--- | :--- | :---: | :--- | :--- | :--- |
| **Ch 0** Zone 1 lights | `led_zone1` | 12 | `Lights Zone 1` | Dimmable 200 Hz | |
| **Ch 1** Zone 2 lights | `led_zone2` | 13 | `Lights Zone 2` | Dimmable 200 Hz | |
| **Ch 2** Zone 3 lights | `led_zone3` | 14 | `Lights Zone 3` | Dimmable 200 Hz | |
| **Ch 3** Zone 4 lights | `led_zone4` | 15 | `Lights Zone 4` | Dimmable 200 Hz | |
| **Ch 4** Water pump | `pump` + 1N5408 | 4 | `Water Pump` | Digital, 10 min auto-off, no restore | Shower via DoubleClick / first StartHold |
| **Ch 5** Exterior driver | `ext_driver` | 5 | `Exterior Driver Light` | Digital | Cockpit SW1 |
| **Ch 6** Exterior passenger | `ext_passenger` | 6 | `Exterior Passenger Light` | Digital | |
| **Ch 7** Lightbar | `lightbar` | 7 | `Aux Lightbar / Roof` | Digital | |
| **Ch 8** Maxxair 12 V | `fan_pwr` + 1N5408 | 17 | `Maxxair Fan Power` | Digital | Cockpit SW6 |
| **Ch 9** Aux 1 boiler | `aux_pwr1` | 8 | `Aux Power 1 (Boiler)` | Digital | |
| **Ch 10** Aux 2 grey valve | `aux_pwr2` | 9 | `Aux Power 2 (Grey Valve)` | Digital | no restore |
| **Ch 11** Aux 3 tank heat | `aux_pwr3` | 10 | `Aux Power 3 (Tank Heat)` | Digital | no restore |
| **Ch 12** Aux 4 sockets | `aux_pwr4` | 11 | `Aux Power 4 (Aux Sockets)` | Digital | |
| **Ch 13** Aux 5 awning | `aux_pwr5` | 18 | `Aux Power 5 (Awning)` | Digital | |
| **Ch 14** MultiPlus remote | `inverter_opto` | 21 | `Inverter (Multiplus II)` | Digital | Cockpit SW5 |
| **Ch 15** Orion-XS #1 | `orion1_opto` | 38 | `DC-DC Orion-XS #1` | Digital | Shares DevKit v1.1 RGB LED |
| **Ch 16** Orion-XS #2 | `orion2_opto` | 39 | `DC-DC Orion-XS #2` | Digital | |
| **Ch 17** SmartSolar | `mppt_opto` | 40 | `SmartSolar MPPT` | Digital | |
| **Ch 18** Diesel heater | `heater_opto` | 41 | `Diesel Heater` | Digital | thermostat contact only |
| **Ch 19** Fridge | `fridge_opto` | 42 | `12V Fridge Compressor` | Digital | |
| **Ch 20** Maxxair keypad | `fan_pulse_opto` | 47 | `Maxxair Keypad Pulse` | Pulse 250 ms | |
| **Ch 21** Alarm | `aux_sig1_opto` | 48 | `Aux Signal 1 (Alarm)` | Digital | Shares DevKit v1.0 RGB LED |
| **Ch 22** LPG valve | `aux_sig2_opto` | 2 | `Aux Signal 2 (LPG Valve)` | Digital | no restore |
| **Ch 23** Generator start | `aux_sig3_opto` | 16 | `Aux Signal 3 (Gen Start)` | Pulse 500 ms | |
| **ADC** cabin battery | `voltage_sensor` | 1 | `BATTERY_ADC_PIN = 1` | 100k/18k + BAT54S clamp | |

PCB: `hardware/central-pcb/layouts/profet_24ch/` — 180 mm × 110 mm, M3 holes at (±85 mm, ±50 mm) / 170 × 100 mm pitch. Enclosure: `central_24ch_*` 205 × 135 mm.

---

## 2. Entrance Remote Alignment Matrix

| Control | Atopile | XIAO pin | Firmware | Wake | Central mapping |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Diode-OR bus | `wakeup_line` | GPIO 0 (D0) | pull-up only | LP EXT1 | — |
| Encoder A | `ENC_A` | GPIO 1 (D1) | `EncoderHandler` pin A | LP EXT1 | button 7 `EncoderTurn` → focused zone |
| Encoder B | `ENC_B` | GPIO 2 (D2) | `EncoderHandler` pin B | LP EXT1 | button 7 |
| Encoder SW | `ENC_SW` | GPIO 20 (D9) | `EncoderHandler` pin SW **only** | via Diode-OR | button 7 `Click` → 100% boost |
| Zone 1 | `btn_zone1` | GPIO 21 (D3) | button 0 | via Diode-OR | Ch 0 |
| Zone 2 | `btn_zone2` | GPIO 22 (D4) | button 1 | via Diode-OR | Ch 1 |
| Zone 3 | `btn_zone3` | GPIO 23 (D5) | button 2 | via Diode-OR | Ch 2 |
| Zone 4 | `btn_zone4` | GPIO 16 (D6) | button 3 | via Diode-OR | Ch 3 |
| Water pump | `btn_pump` | GPIO 17 (D7) | button 4 | via Diode-OR | Ch 4 |
| Night shutdown | `btn_night` | GPIO 19 (D8) | button 5 | via Diode-OR | `TurnOffAllLights` |
| Status LED | `status_led` | **GPIO 18 (D10)** | `STATUS_LED_PIN 18` | — | — |

Build: `pio run -d firmware/remote -e seeed_xiao_esp32c6` (`REMOTE_ID=1`).

---

## 3. Cockpit Hub Alignment Matrix

| Switch | Atopile | XIAO pin | Firmware (latching edges) | Central mapping |
| :--- | :--- | :--- | :--- | :--- |
| Wake bus | `wakeup_line` | GPIO 0 (D0) | pull-up | — |
| SW1 | `ch1` | GPIO 1 (D1) | button 0 | Ch 5 exterior |
| SW2 | `ch2` | GPIO 2 (D2) | button 1 | Ch 15 Orion #1 |
| SW3 | `ch3` | GPIO 21 (D3) | button 2 | Ch 0 Zone 1 |
| SW4 | `ch4` | GPIO 22 (D4) | button 3 | Ch 4 pump |
| SW5 | `ch5` | GPIO 23 (D5) | button 4 | Ch 14 inverter |
| SW6 | `ch6` | GPIO 16 (D6) | button 5 | Ch 8 Maxxair power |
| Status LED | `status_led` | GPIO 18 (D10) | `STATUS_LED_PIN 18` | — |

Build: `pio run -d firmware/remote -e cockpit_xiao_esp32c6` (`REMOTE_ID=3`). Inputs are **latching**: Click on each edge, no `StartHold`.

---

## 4. Pre-Sync Audit Checklist

- [ ] Pin change in `central.ato` → `SystemController.cpp` `DEFAULT_24CH_CONFIG` + this matrix + `pio test -e native`
- [ ] Pin change in `entrance_remote.ato` / `cockpit.ato` → `firmware/remote/src/main.cpp`
- [ ] PCB outline change → `generate_enclosures.py` standoff pitch
- [ ] Do not flash 24-channel firmware onto the 11-channel PROFET carrier
