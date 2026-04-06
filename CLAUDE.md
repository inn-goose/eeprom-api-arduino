# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An Arduino-based EEPROM programmer for AT28Cxx family chips (AT28C04, AT28C16, AT28C64, AT28C256). Two components:

1. **Arduino firmware** (`eeprom_programmer/`) — C++ sketch, validated on Arduino Mega and Due. Arduino Giga compiles but had serial issues (needs verification).
2. **Python CLI** (`eeprom_programmer_cli/`) — host-side tool communicating over Serial JSON-RPC

The CLI interface is modeled after `minipro` (XGecu programmer). A companion blog series documents the development: [goose.sh/tags/eeprom-programmer](https://goose.sh/tags/eeprom-programmer/).

## Architecture

### Firmware (`eeprom_programmer/`)

All logic lives in header files — no `.cpp` files. The `.ino` includes everything.

- `eeprom_programmer.ino` — main sketch; wires binary protocol commands (`init_chip`, `set_read_mode`, `read_page`, `set_write_mode`, `write_page`, `get_read_perf`, `get_write_perf`) to the `EepromProgrammer` class
- `eeprom_programmer_lib.h` — core `EepromProgrammer` class: address/data bus GPIO, read/write byte operations, write completion polling
- `chip_wiring.h` — per-chip pin mappings using **DIP pin numbers** (1-24 or 1-28) and `ChipWiringController`
- `board_wiring.h` — maps **DIP pin positions to Arduino GPIO numbers** for DIP24 and DIP28 socket layouts
- `binary_protocol.h` — binary serial protocol handler. Frame format: `[0xAA][0x55][LEN_L][LEN_H][BODY...][CRC_L][CRC_H]`, CRC-16/CCITT, state machine receiver. No external dependencies. 68-byte receive buffer, 136-byte send buffer.
- `serial_json_rpc_lib.h` — **DEAD CODE, pending deletion.** Former JSON-RPC 2.0 protocol handler. Replaced by `binary_protocol.h`.

**Two-level pin mapping** (non-obvious): chip wiring tables define which DIP pin number corresponds to each logical function (A0, A1, IO0, !CE, etc.). The board wiring table then resolves DIP pin positions to physical Arduino GPIO numbers. Adding a new chip means defining its DIP-pin-to-function mapping in `chip_wiring.h`; changing the Arduino board means updating the DIP-to-GPIO table in `board_wiring.h`.

**Wiring type** is a compile-time choice in `eeprom_programmer.ino` — toggle between `BoardWiringType::DIP24` (AT28C04, AT28C16) and `BoardWiringType::DIP28` (AT28C64, AT28C256). Must match the physical wiring and requires recompilation.

### CLI (`eeprom_programmer_cli/`)

- `cli.py` — argparse entry point with `--read`, `--write`, `--erase`, `--verify` commands
- `core/eeprom_programmer_client.py` — `EepromProgrammerClient`: wraps binary protocol calls, handles 64-byte page read/write
- `binary_protocol/client.py` — `BinaryProtocolClient`: pyserial connection, binary frame building/parsing, CRC-16/CCITT
- `binary_protocol/test_client.py` — 32 unit tests for CRC, frame building, frame parsing, send_command, cross-validation
- `serial_json_rpc/client.py` — **DEAD CODE, pending deletion.** Former JSON-RPC client. Replaced by `binary_protocol/client.py`.

**Data flow**: CLI sends binary frames over serial -> firmware state machine parses and dispatches -> firmware manipulates EEPROM via GPIO -> binary response frame back. The architecture follows a "smart client, simple board" pattern — the CLI holds business logic while the firmware exposes only basic page read/write primitives.

### Backup versions (`bak/`)

Contains three earlier firmware iterations showing the project's evolution:
- `eeprom_api__1__rw_ops/` — LCD UI + button control, 9600 baud, 10-cycle demos
- `eeprom_api__2__performance/` — autonomous performance testing, 57600 baud, RDY/!BUSY polling
- `eeprom_api__3__debugging/` — xxd-style hex dumps, 115200 baud, full address space testing

The `xxd_arduino.txt` and `xxd_xgecu.txt` are hex dumps for verifying read integrity against a reference XGecu programmer.

## Arduino Platform Differences

The firmware uses **no conditional compilation** (`#ifdef`). A single codebase compiles for all platforms. Arduino UNO is **not supported** — it lacks the required pin count (pins 22-53 needed). The minimum supported board is Arduino MEGA. Platform-specific behavior:

- **AT28C256 page write** (`_can_write_pages` flag in `eeprom_programmer_lib.h`): unconditionally enabled for AT28C256. Page write requires each successive byte written within 150 us (tBLC). Arduino MEGA's `digitalWrite` takes ~400 us per byte — too slow, will cause silent data corruption. Arduino DUE at 84 MHz achieves ~110 us (within spec). There is no runtime platform detection; the comment warns "doesn't work on Arduino MEGA / enable if use Arduino DUE only" but the flag is always set.
- **Arduino Giga**: commit 908064f fixed type casting issues (`int32_t` vs `int`/`long` on ARM) to allow compilation. Giga shares the MEGA pin layout (22-53) but had serial issues — not fully validated.
- **Board wiring**: `board_wiring.h` uses pins 22-53 (MEGA-compatible extended header). MEGA, DUE, and Giga share this pin layout.
- **Performance**: DUE is ~30% faster than MEGA for reads; write speedup is larger due to page write support.

## Build and Setup

### Arduino firmware
Compile and upload via Arduino IDE. Requires [ArduinoJson](https://arduinojson.org/) library. Serial baud rate: 115200.

Before compiling, set the wiring type in `eeprom_programmer.ino`:
```cpp
static EepromProgrammer eeprom_programmer(BoardWiringType::DIP24);  // AT28C04, AT28C16
// static EepromProgrammer eeprom_programmer(BoardWiringType::DIP28);  // AT28C64, AT28C256
```

### Python CLI
```bash
pip3 install virtualenv
# specify your python version (X.Y part only)
PATH=${PATH}:~/Library/Python/3.9/bin/ ./env/init.sh
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH
```

Only dependency: `pyserial==3.4` (in `env/requirements_cli.txt`).

### CLI usage
```bash
python3 -m serial.tools.list_ports                                              # find serial port
./eeprom_programmer_cli/cli.py <port> -p <chip> --read <file>                   # read chip to file
./eeprom_programmer_cli/cli.py <port> -p <chip> --write <file>                  # erase + write + verify
./eeprom_programmer_cli/cli.py <port> -p <chip> --write <file> --skip-verify    # erase + write without verify
./eeprom_programmer_cli/cli.py <port> -p <chip> --write <file> --skip-erase     # write + verify without erase
./eeprom_programmer_cli/cli.py <port> -p <chip> --erase --erase-pattern FF      # erase with pattern
./eeprom_programmer_cli/cli.py <port> -p <chip> --verify <file>                 # read and compare to file
```

Add `--collect-performance` to any operation for timing data.

## Key Design Details

- **!WE pin safety and data corruption**: during read operations, the EEPROM's `!WE` pin MUST be physically connected to VCC via jumper wire. Arduino's serial reset causes voltage transients that trigger unintended writes. Remove the jumper only for write/erase operations. Details in [blog post](https://goose.sh/blog/eeprom-programmer-5-data-corruption/).

  **Root cause (DUE)**: The SAM3X8E has ESD protection diodes on every GPIO pin (clamped to VDDIO). During reset, the 3.3V VDDIO rail ramps from 0V, and the ESD diode clamps GPIO pins to VDDIO+0.3V ≈ 0.3V — well below the AT28C64's V_IL threshold (0.8V). The EEPROM interprets this as a valid LOW on !WE. Combined with the AT28C64's 100ns minimum write pulse width (tWP, datasheet page 14), a write to address 0x0000 occurs during the reset window. The 5V USB rail powers up before the 3.3V regulator stabilizes, creating a window where the EEPROM is operational but all control pins are clamped LOW by ESD diodes. No passive pull-up resistor (tested 10K and 1K on !WE, 10K on !CE/!OE) can overcome this because it's a diode clamp, not a resistive divider.

  **DUE Programming Port specifics**: The ATmega16U2 drives NRST LOW for ~200ms directly through a 10K series resistor (no RC pulse capacitor like UNO/MEGA). The cap-on-RESET trick does not work on DUE. Total boot time with undefined pin states: ~3-4 seconds.

  **Workaround for automated verify**: use in-session write+verify (CLI `--write` with verify, no serial reconnect between operations) to avoid the reset-induced corruption.
- **Write polling strategies**: AT28C64 has a dedicated RDY/!BUSY pin (faster, simpler polling). Other chips use data polling (read-back loop until written value matches). Both achieve ~400-450 us write time. Controlled by `_polling()` method which auto-selects based on pin availability.
- **Page size**: both read and write use 64-byte pages (`_MAX_PAGE_SIZE = 64`). This is also the AT28C256's hardware page buffer size.
- **JSON-RPC buffer**: 350 bytes max per message. The comment "works fine for UNO R3" in `serial_json_rpc_lib.h` refers to the JSON-RPC library's memory footprint in isolation, not EEPROM programmer compatibility. Messages delimited by `\n`.
- **Test binaries** (`test_bin/`): two variants per chip — raw data (`64_the_red_migration.bin`) and padded to full chip size with 0xFF (`64_the_red_migration_AT28C64_ff.bin`). The number prefix corresponds to the chip model (4=AT28C04, 16=AT28C16, 64=AT28C64, 256=AT28C256). The `--verify` command requires the padded `_ff` variant.

## Potential Refactoring

### Firmware performance

- **`_data_polling` toggles entire data bus twice per byte**: switches all 8 data pins to READ mode and back to WRITE mode for each poll cycle. Each switch does 8x `pinMode` + `digitalWrite` calls. Affects AT28C04, AT28C16, AT28C256-on-MEGA (chips without RDY/!BUSY pin).

### Serial protocol: binary protocol performance

The binary protocol replaced JSON-RPC + ArduinoJson, eliminating the only external firmware dependency.

**Performance (DUE + AT28C64, 8KB):**

| Operation | JSON-RPC | Binary | Speedup |
|---|---|---|---|
| Read | 6.96s | 1.58s | 4.4x |
| Erase | 13.85s | 5.77s | 2.4x |
| Write only | 12.91s | 5.35s | 2.4x |
| Full (erase+write+verify) | 33.74s | 12.70s | 2.7x |

Write speedup is limited by EEPROM hardware write time (~400-540μs/byte) — a fixed floor regardless of protocol. Read is almost pure protocol overhead, hence the higher speedup.

**Flash: 50,936 → 34,936 bytes (-31%)** — ArduinoJson eliminated.

Wire bytes per 64-byte page: JSON ~378B vs binary ~82B (read), JSON ~400B vs binary ~82B (write).

**Remaining serial characteristics:**
- **Fully synchronous, one round-trip per page**: no pipelining. Each page is send-request → wait → receive-response → next.
- **No `while (!Serial)` for native USB boards**: `binary_protocol.h` calls `Serial.begin()` without waiting for USB enumeration. On DUE/Giga native USB port, the BOOT frame sent in `setup()` could be lost. Works in practice due to the 3-second `init_timeout` on the Python side.
