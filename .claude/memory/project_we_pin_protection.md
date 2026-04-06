---
name: WE pin protection — deep research for blog post on EEPROM corruption during Arduino reset
description: Comprehensive analysis with datasheet specs, circuit details, root cause, and community data — ready for blog post
type: project
---

## Problem Summary

Opening a serial connection to Arduino DUE triggers a board reset. During the ~3-second reset window, GPIO transients corrupt EEPROM data at address 0x0000 (written to 0xFF). No passive pull-up resistor configuration tested can prevent this.

Blog post with oscilloscope measurements: https://goose.sh/blog/eeprom-programmer-5-data-corruption/

---

## Test Results (2026-04-06, Arduino DUE + AT28C64, Programming Port)

| Configuration | Corruption? | Notes |
|---|---|---|
| No protection | Yes — addr 0x0000 | Multiple bytes possible |
| 10K on !WE to VCC | Yes — addr 0x0000 | Same as no protection |
| 1K on !WE + 10K on !CE,!OE to VCC | Yes — addr 0x0000 | No *further* corruption on subsequent reconnects |
| Direct jumper !WE→VCC | **No** | Must swap jumper between write/read |

---

## AT28C64 Datasheet Specs (Verified from doc0001H)

### Write Cycle Initiation (Page 11)
"A low pulse on the WE or CE input with OE high and CE or WE low (respectively) initiates a byte write. The address location is latched on the falling edge of WE (or CE); the new data is latched on the rising edge."

### Operating Modes (Page 12, Table)
| Mode | !CE | !OE | !WE |
|---|---|---|---|
| Read | VIL | VIL | VIH |
| Write | VIL | VIH | VIL |
| Standby / Write Inhibit | VIH | X | X |
| Write Inhibit | X | X | VIH |
| Write Inhibit | X | VIL | X |

**Key: ANY ONE of (!OE LOW, !CE HIGH, !WE HIGH) inhibits writes.** But during reset, all three float simultaneously.

### DC Characteristics (Page 12)
- **V_IL (Input Low)**: max **0.8V**
- **V_IH (Input High)**: min **2.0V**
- **V_OL (Output Low)**: max 0.45V (at I_OL = 2.1 mA)
- **V_OH (Output High)**: min 2.4V (at I_OH = -400 µA)
- **I_LI (Input Load Current)**: 10 µA max (VIN = 0V to VCC+1V)

**Forbidden zone: 0.8V–2.0V** — undefined behavior, most dangerous.

### AC Write Characteristics (Page 14)
- **tWP (Write Pulse Width min)**: **100 ns** — only 100 nanoseconds to trigger a write!
- **tAS (Address Setup Time)**: 10 ns
- **tAH (Address Hold Time)**: 50 ns
- **tDS (Data Setup Time)**: 50 ns
- **tDH (Data Hold Time)**: 10 ns
- **tCS, tCH (CE to WE setup/hold)**: 0 ns
- **tWC (Write Cycle Time)**: 1 ms (AT28C64), 200 µs (AT28C64E)
- **tDB (Time to Device Busy)**: 50 ns

### Write Protection Features (Page 11)
1. **VCC sense**: writes inhibited if VCC < 3.8V (typical, NOT guaranteed)
2. **Power-on delay (tPUW)**: 5 ms typical after VCC reaches 3.8V (NOT guaranteed)
3. **Write inhibit logic**: any one of (!OE LOW, !CE HIGH, !WE HIGH) inhibits writes
4. **No SDP**: AT28C64 does NOT have Software Data Protection (AT28C64B does)
5. **Noise filter**: NOT documented in AT28C64 datasheet (AT28C64B has 15 ns typical)

### AT28C64 vs AT28C64B Key Differences
| Feature | AT28C64 | AT28C64B |
|---|---|---|
| SDP | No | Yes (not enabled by default) |
| Page write | No | Yes (64 bytes) |
| RDY/!BUSY pin | Yes (pin 1) | No (pin 1 = NC) |
| tWC | 200 µs (E variant) / 1 ms | 10 ms max |
| Noise filter | Not documented | 15 ns typical |

Datasheets:
- AT28C64: https://ww1.microchip.com/downloads/en/devicedoc/doc0001h.pdf
- AT28C64B: https://ww1.microchip.com/downloads/en/DeviceDoc/doc0270.pdf

---

## Arduino DUE Reset Circuit (Programming Port)

### How Reset Triggers (from PeterVH blog + Quentin Santos article)

The Programming Port uses **ATmega16U2** as USB-to-serial bridge:

1. Host opens serial port → DTR goes low
2. ATmega16U2 firmware detects DTR change via `EVENT_CDC_Device_ControLineStateChanged()`
3. At 1200 baud: pulses **ERASE** HIGH then **NRST** LOW (erase+reset for upload)
4. At other baud rates: pulses **NRST** LOW only (reset without erase)
5. NRST held LOW for **~200 ms** (set by `ResetTimer = 30` ticks in firmware)
6. 16U2 releases pin to high-impedance; SAM3X8E internal ~100 kΩ pull-up restores NRST

### Critical Circuit Differences from UNO/MEGA

| Feature | UNO/MEGA | DUE |
|---|---|---|
| DTR to RESET | Via 100nF series capacitor (brief pulse) | Direct drive via 10K series resistor (200ms hold) |
| Reset pull-up | External 10K to 5V | Internal ~100 kΩ to 3.3V |
| Protection diode | Diode from RESET to 5V | None |
| Buffer between 16U2 and RESET | N/A (capacitor-coupled) | None — direct connection |
| Cap-on-RESET trick works? | Yes (20µF absorbs brief pulse) | **No** (200ms fully drains any cap) |
| GPIO voltage during reset | 5V system, pins float to ~2V | **3.3V system**, pins float with 100kΩ pull-up to 3.3V |

### Total Boot Sequence Timing
Measured by forum users:
- **Phase 1** (~200ms): NRST asserted LOW → all PIO pins forced to reset state
- **Phase 2** (~100ms): Hardware startup, SAM-BA ROM boot check
- **Phase 3** (~2-3s): Arduino bootloader + sketch init → pins "flap" HIGH/LOW
- **Total**: ~3-4 seconds of undefined pin states

Source: https://petervanhoyweghen.wordpress.com/2013/05/04/disabling-auto-reset-on-the-due/
Source: https://qsantos.fr/2025/05/01/arduino-automatic-reset/

---

## SAM3X8E GPIO Characteristics

### PIO Default State After Reset (Datasheet Section 31)
- **PIO_PSR** = 0xFFFFFFFF → all pins controlled by PIO (not peripherals)
- **PIO_OSR** = 0x00000000 → all pins configured as **inputs** (high-impedance)
- **PIO_PUSR** = 0x00000000 → internal pull-ups **enabled** (0 = enabled in this register)
- Internal pull-up resistance: **~100 kΩ** to VDDIO (3.3V)
- Pull-up current at 3.3V: ~33 µA — essentially floating for EEPROM purposes

### GPIO Drive Strength (Datasheet Table 45)
- **Source current (drive HIGH)**: 3 mA or 15 mA (pin-dependent; pins 22-53 = 3 mA)
- **Sink current (drive LOW)**: 6 mA or 9 mA (pin-dependent)
- **Operating voltage (VDDIO)**: 3.3V
- **Total DC output**: 130 mA across all pins

### ESD Protection Structure
Every SAM3X8E GPIO pad has ESD protection diodes:
- **Upper diode**: pin to VDDIO (clamps at VDDIO + ~0.3V)
- **Lower diode**: pin to GND (clamps at GND - ~0.3V)
- These are inherent to the CMOS I/O cell design

### SAM-BA Bootloader
Only configures UART0 (PA8/PA9) and USB pins. Does **NOT** touch GPIO pins 22-53. All EEPROM-connected pins remain in PIO reset state throughout boot.

Source: https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-11057-32-bit-Cortex-M3-Microcontroller-SAM3X-SAM3A_Datasheet.pdf
Source: https://forum.arduino.cc/t/due-default-pin-state-on-reset-power-on/377523
Source: https://forum.arduino.cc/t/behavior-of-pins-during-reset-state/640285

---

## Root Cause Analysis — Why Pull-Up Resistors Fail on DUE

### Cause 1: ESD Protection Diode Clamping During Power Transitions (Primary)

During reset, the 3.3V VDDIO rail ramps from 0V→3.3V. While VDDIO is low:
- Upper ESD diode conducts: clamps GPIO pin to VDDIO + 0.3V
- If VDDIO = 0V → pin clamped to ~0.3V
- External 1K pull-up to 5V pushes current through ESD diode into unpowered VDDIO rail
- **Pin voltage sits at ~0.3V** — well below EEPROM's 0.8V V_IL
- EEPROM sees valid LOW on !WE

This happens even with the strongest pull-up because it's not a resistive divider — it's a **diode clamp**. The pin cannot exceed VDDIO + 0.3V regardless of external pull-up strength.

### Cause 2: Power Sequencing (5V Before 3.3V)

On DUE, the 5V USB rail powers up before the on-board 3.3V regulator stabilizes:
- External 1K pull-up to 5V is active immediately (from USB power)
- SAM3X8E VDDIO is still at 0V → ESD diodes clamp all GPIO pins to ~0.3V
- EEPROM (powered by 5V) is already operational
- Window of vulnerability exists during every power-on/reset

### Cause 3: 3.3V vs 5V Voltage Domain Mismatch

Even when VDDIO is stable at 3.3V:
- GPIO pin with internal 100kΩ pull-up reaches 3.3V — above EEPROM V_IH (2.0V), safe
- But during transients, if VDDIO sags to ~1.5V, pin reaches ~1.5V
- 1.5V is in the EEPROM's forbidden zone (0.8V–2.0V)
- External 1K to 5V helps via resistive divider, but fast transients couple through parasitic capacitance

### Cause 4: Parasitic Capacitance + 100ns Write Sensitivity

- GPIO pad capacitance: ~5-10 pF
- PCB trace + EEPROM input: ~10-15 pF
- Total: ~20 pF
- With 1K pull-up: τ = R×C = 1K × 20pF = **20 ns**
- A 20 ns voltage dip can pull !WE below 0.8V
- AT28C64 needs only **100 ns** of !WE LOW to write
- Multiple sub-100ns glitches during 3-second reset window → writes happen

### Cause 5: All Control Pins Float Simultaneously

During reset, !CE, !WE, !OE are ALL in undefined state:
- Write requires: !CE LOW + !WE LOW + !OE HIGH
- With three pins floating independently over 3 seconds
- Address bus defaults to 0x0000 (all pins LOW via weak pull-downs or floating)
- Data bus floats HIGH (internal pull-ups → 0xFF)
- Result: 0xFF written to address 0x0000

### Why Direct Jumper Works

Jumper wire = **zero-impedance DC connection to 5V**:
- No resistor → no RC time constant
- No GPIO pad in the circuit → no ESD diodes involved
- Pin is physically at 5V regardless of VDDIO state
- Only risk: short circuit if Arduino drives LOW, but during reset pin is high-impedance

---

## Solutions

### Working Now
- **Jumper wire swap**: remove !WE jumper for write, reconnect for read. 100% reliable.

### For Binary Protocol Migration
- **In-session verify**: write + read-back in same serial session (no reconnect = no reset = no corruption)

### Alternative Hardware Approaches
1. **Use Native USB Port** — no reset on serial open (except 1200 baud). Requires `while (!Serial)` in setup(). Eliminates the problem entirely.
2. **Disable auto-reset**: 1K resistor between DUE RESET pin and 3.3V (overpowers 16U2's 10K series resistor). Sketch upload then requires manual reset button press.
3. **Enable SDP on AT28C64B** — Software Data Protection prevents writes without 3-byte unlock sequence. Only works on "B" variant.
4. **Buffer IC** (74HC125/245) between Arduino GPIO and EEPROM control pins. Overkill.

---

## Community Sources

### Reddit
- **nib85** (r/beneater): 10K on !WE + !CE + !OE — "I've been using that in my designs and have never had an issue" (likely MEGA, not DUE)
- **lordmonoxide** (r/beneater master list, 314 pts): 10K standard for pull-ups, warns about EEPROM output fluctuations
- **vegansgetsick** (r/beneater): "Transfer from USB through TX/RX and then to EEPROM can corrupt. You have to calculate checksum"

### GitHub
- crmaykish/AT28C-EEPROM-Programmer-Arduino: "Don't forget the 10kohm pullup resistor on the WE pin"
- TomNisbet/TommyPROM: documents AT28C256 SDP issues

### Arduino Forum
- DUE pins "go HIGH for a fraction of a second" on power-on and serial connect
- Pins exhibit two HIGH/LOW cycles during ~3-4 second boot sequence
- Standard recommendation: 10K pull-downs on output pins to prevent floating

### Blogs
- PeterVH: DUE reset is 200ms direct drive (not RC pulse like UNO). Cap trick fails. 1K to 3.3V on RESET works to disable auto-reset.
- Quentin Santos: detailed analysis of Arduino auto-reset mechanism, capacitor/resistor circuit
- bread80.com: AT28C256 SDP timing issues with Arduino + shift registers

**How to apply:** This analysis is ready to be used as source material for a blog post. The hypothesis is supported by datasheet specs, circuit analysis, and experimental results.
