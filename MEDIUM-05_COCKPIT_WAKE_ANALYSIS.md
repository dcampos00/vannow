# Cockpit Wake Wiring Analysis (MEDIUM-05)

**Date:** 2026-09-16  
**Investigation:** Cross-verification of cockpit deep-sleep wake architecture  
**Verdict:** ✅ **OK AS DESIGNED** – Hardware, firmware, and documentation are consistent

---

## Investigation Summary

The cockpit remote uses a **diode-OR wake bus architecture** where all 6 rocker switches OR together to wake the ESP32-C6 on a single LP-GPIO (GPIO 0 / D0). After wake, the firmware reads the individual switch sense pins to determine which switch(es) changed state.

---

## Hardware Design (cockpit.ato)

### Diode-OR Wake Bus

**File:** `hardware/cockpit-pcb/cockpit.ato`

```
Line 131: wakeup_line ~ mcu.LP_GPIO0   # Wake bus connected to GPIO 0 (D0)

Lines 140-154: All 6 switches connected to diode-OR wake bus
    ch1 = new SwitchInputChannel
    ...
    ch6 = new SwitchInputChannel
    
    wakeup_line ~ ch1.wakeup_bus
    wakeup_line ~ ch2.wakeup_bus
    ...
    wakeup_line ~ ch6.wakeup_bus
```

### SwitchInputChannel Architecture

**File:** `hardware/cockpit-pcb/cockpit.ato` lines 96-116

Each switch channel has:
- **100k pull-up** to 3.3V (keeps switch input HIGH when open)
- **100nF debounce capacitor** to GND
- **1N4148W diode:** Anode → wake bus, Cathode → switch input
- **Sense connection** to MCU GPIO

**Operation:**
1. Switch open: input is HIGH (pulled up), diode reverse-biased, wake bus unaffected
2. Switch closes: input pulled to GND, diode conducts, pulls wake bus LOW → wakes MCU on GPIO 0
3. After wake: firmware reads all 6 sense GPIOs to identify which switch(es) changed

### GPIO Mapping

| Switch | Sense GPIO | Terminal | Function |
|--------|------------|----------|----------|
| SW1 | GPIO 1 (D1) | Term1.1 | Exterior Driving / Aux Lights |
| SW2 | GPIO 2 (D2) | Term1.3 | Orion-XS DC-DC Remote Enable |
| SW3 | GPIO 21 (D3) | Term2.1 | Cabin Interior Lights |
| SW4 | GPIO 22 (D4) | Term2.3 | Water Pump |
| SW5 | GPIO 23 (D5) | Term3.1 | Inverter (Multiplus II) |
| SW6 | GPIO 16 (D6) | Term3.3 | Master Driving Mode / Aux |

**Wake Bus:** GPIO 0 (D0) with 47k pull-up to 3.3V (line 134-137)

---

## Firmware Configuration (main.cpp)

**File:** `firmware/remote/src/main.cpp`

### Cockpit Wake Mask

```cpp
Line 80: #elif (CONFIG_PANEL_TYPE == PANEL_TYPE_COCKPIT)
Lines 81-86: 6x ButtonHandler on GPIOs 1, 2, 21, 22, 23, 16 (latching mode)
Line 89: const uint8_t wakeupPins[] = {0};  // Only GPIO 0 in EXT1 wake mask
```

### Wake Sequence

```cpp
Line 126: pinMode(0, INPUT_PULLUP);  // Configure GPIO 0 pull-up before sleep

Lines 179-186: Loop after wake
    for (int i = 0; i < NUM_BUTTONS; i++) {
        ActionType action;
        if (buttons[i]->checkEvent(action)) {  // Read each switch
            powerManager.feed();
            remoteSender.send(action, i, 0);
            indicateAction(action, i);
        }
    }
```

---

## Documentation Verification

### SKILL.md (Authoritative Firmware Guide)

**File:** `.agents/skills/esp32-firmware-engineering/SKILL.md` line 61

```
Cockpit: `CONFIG_PANEL_TYPE=5`, latching, wake on GPIO 0 only.
```

**Gesture Table (line 58):**
```
| Cockpit rocker | `Click` on **each edge** (`InputMode::Latching`) | Toggle. Never emit `StartHold` |
```

### Deep Sleep Reference

**File:** `.agents/skills/esp32-firmware-engineering/references/esp32c6_deep_sleep_wakeups.md`

**Line 29-31:**
```
All tactile/rocker lines diode-OR to D0 (GPIO 0), 47 kΩ pull-up to 3.3 V. 
Anode on the wake bus, cathode on the sense pin (pulled to GND when pressed). 
Sense GPIOs identify which switch after wake.

Entrance also wakes on encoder A/B (GPIO 1, 2). Cockpit wakes on GPIO 0 only.
```

---

## Comparison: Entrance vs Cockpit

| Feature | Entrance Remote | Cockpit Remote |
|---------|-----------------|----------------|
| **Switch Type** | Momentary tactile (spring return) | Latching rocker (stays in position) |
| **Wake Sources** | GPIO 0 (diode-OR) + GPIO 1, 2 (encoder rotation) | GPIO 0 (diode-OR) only |
| **Wake Mask** | `{0, 1, 2}` | `{0}` |
| **Sense GPIOs** | 21, 22, 23, 16, 17, 19 (buttons) + 1, 2, 20 (encoder) | 1, 2, 21, 22, 23, 16 (switches) |
| **Diode-OR** | 6 buttons + encoder SW → GPIO 0 | 6 switches → GPIO 0 |
| **Input Mode** | `InputMode::Momentary` (default) | `InputMode::Latching` |
| **Gestures** | Click, DoubleClick, Hold | Click on each edge (press AND release) |

---

## Why GPIO 1 & 2 Are NOT in Cockpit Wake Mask

**Question:** GPIO 1 and GPIO 2 are LP-GPIOs (can wake), and they are connected to switches SW1 and SW2. Why aren't they in the wake mask?

**Answer:** By design, cockpit uses **diode-OR architecture** for uniform wake behavior:

1. **Hardware:** All 6 switches diode-OR to GPIO 0. SW1 and SW2 sense pins are also on GPIO 1 and 2.
2. **Firmware:** Only GPIO 0 in wake mask ensures **any** switch can wake (not just SW1/SW2).
3. **Benefit:** Symmetry and consistency – all switches have equal wake capability.
4. **Trade-off:** If GPIO 0 diode-OR bus were to fail (open circuit), SW1 and SW2 could still wake independently if added to the mask. But this adds complexity and asymmetry.

**Design choice:** Diode-OR simplicity over redundant wake pins.

---

## Edge Case Analysis

### What if GPIO 0 Diode-OR Bus Fails?

**Scenario:** 47k pull-up resistor fails open, or PCB trace to GPIO 0 is damaged.

**Impact:**
- Device cannot wake from deep sleep (all switches ineffective)
- **Mitigation:** Add GPIO 1 and GPIO 2 to wake mask as backup

**Proposed Resilient Wake Mask:**
```cpp
const uint8_t wakeupPins[] = {0, 1, 2};  // GPIO 0 primary, GPIO 1/2 backup
```

**Trade-off:**
- ✅ **Pro:** SW1 and SW2 can wake even if GPIO 0 bus fails
- ❌ **Con:** Asymmetric behavior (SW1/SW2 preferred over SW3-6)
- ❌ **Con:** Adds complexity to failure modes

**Recommendation:** Keep current design (`{0}` only) unless field failures indicate GPIO 0 bus reliability issues.

---

## Cross-Domain Consistency Matrix

| Domain | GPIO 0 Wake Bus | SW1-SW6 Sense GPIOs | Documentation |
|--------|-----------------|---------------------|---------------|
| **Hardware (cockpit.ato)** | ✅ Line 131: `wakeup_line ~ mcu.LP_GPIO0` | ✅ Lines 156-161: GPIO 1, 2, 21, 22, 23, 16 | — |
| **Firmware (main.cpp)** | ✅ Line 89: `wakeupPins[] = {0}` | ✅ Lines 81-86: GPIO 1, 2, 21, 22, 23, 16 | — |
| **SKILL.md** | — | — | ✅ Line 61: "wake on GPIO 0 only" |
| **esp32c6_deep_sleep_wakeups.md** | ✅ Line 29: "diode-OR to D0 (GPIO 0)" | ✅ Line 29: "Sense GPIOs identify which switch after wake" | ✅ Line 31: "Cockpit wakes on GPIO 0 only" |

**Result:** All domains are consistent. No mismatches found.

---

## Functional Verification

### Test Case 1: Any Switch Can Wake

**Setup:** Cockpit in deep sleep  
**Action:** Press SW3 (GPIO 21, connected to Term2.1)  
**Expected:**
1. SW3 pulls GPIO 21 to GND
2. Diode D3 conducts, pulls GPIO 0 wake bus LOW
3. ESP32-C6 wakes on GPIO 0 EXT1 interrupt
4. Firmware reads GPIO 21 = LOW, identifies SW3 pressed
5. Emits `Click` action, button index 2

**Result:** ✅ Hardware and firmware support this

### Test Case 2: Multiple Simultaneous Switches

**Setup:** Cockpit in deep sleep  
**Action:** Press SW1 and SW4 simultaneously  
**Expected:**
1. Both switches pull GPIO 0 wake bus LOW via diodes
2. ESP32-C6 wakes on GPIO 0
3. Firmware reads GPIO 1 = LOW and GPIO 22 = LOW
4. Emits `Click` for button 0 (SW1) and button 3 (SW4)

**Result:** ✅ Diode-OR architecture supports this

### Test Case 3: Latching Edge Detection

**Setup:** Cockpit awake, SW1 in OFF position  
**Action:** Flip SW1 to ON  
**Expected:**
1. GPIO 1 transitions HIGH → LOW
2. `ButtonHandler::checkEvent()` detects edge
3. Emits `Click` action, button index 0

**Action:** Flip SW1 back to OFF (while still awake)  
**Expected:**
1. GPIO 1 transitions LOW → HIGH
2. `ButtonHandler::checkEvent()` detects edge
3. Emits another `Click` action, button index 0

**Result:** ✅ `InputMode::Latching` emits `Click` on both edges (lines 31-62 of ButtonHandler.cpp)

---

## Verdict

**✅ OK AS DESIGNED**

The cockpit remote wake architecture is **correctly implemented** across hardware, firmware, and documentation:

1. **Hardware:** 6 switches diode-OR to GPIO 0 wake bus (47k pull-up). Sense on GPIO 1, 2, 21, 22, 23, 16.
2. **Firmware:** EXT1 wake mask = `{0}`. After wake, reads all 6 sense GPIOs to identify switch(es).
3. **Documentation:** SKILL.md and deep sleep reference explicitly state "wake on GPIO 0 only."
4. **Consistency:** All domains align. No bugs found.

**Design rationale:**
- Diode-OR provides **uniform wake capability** for all 6 switches
- Latching switches emit `Click` on **both edges** (press AND release)
- Simple, symmetric, and consistent with entrance remote architecture

**No firmware or hardware changes required.**

---

## Recommendation

**CLOSE MEDIUM-05** as resolved.

Update audit findings to reflect that cockpit wake design is verified correct. Remove from "Remaining Work" section.

---

**Investigation Complete ✓**  
**No bugs or design flaws found ✓**  
**Hardware, firmware, and documentation are consistent ✓**
