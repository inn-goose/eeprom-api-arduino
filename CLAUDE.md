# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An Arduino-based EEPROM programmer for AT28Cxx family chips (AT28C04, AT28C16, AT28C64, AT28C256). Two components:

1. **Arduino firmware** (`eeprom_programmer/`) — C++ sketch, validated on Arduino Mega and Due
2. **Python CLI** (`eeprom_programmer_cli/`) — host-side tool communicating over Serial JSON-RPC

The CLI interface is modeled after `minipro` (XGecu programmer). A companion blog series documents the development: [goose.sh/tags/eeprom-programmer](https://goose.sh/tags/eeprom-programmer/).

## Architecture

### Firmware (`eeprom_programmer/`)

All logic lives in header files — no `.cpp` files. The `.ino` includes everything.

- `eeprom_programmer.ino` — main sketch; wires JSON-RPC methods (`init_chip`, `set_read_mode`, `read_page`, `set_write_mode`, `write_page`, `get_read_perf`, `get_write_perf`) to the `EepromProgrammer` class
- `eeprom_programmer_lib.h` — core `EepromProgrammer` class: address/data bus GPIO, read/write byte operations, write completion polling
- `chip_wiring.h` — per-chip pin mappings using **DIP pin numbers** (1-24 or 1-28) and `ChipWiringController`
- `board_wiring.h` — maps **DIP pin positions to Arduino GPIO numbers** for DIP24 and DIP28 socket layouts
- `serial_json_rpc_lib.h` — JSON-RPC 2.0 protocol handler over Serial, vendored locally (origin: `inn-goose/serial-json-rpc-arduino`). Depends on ArduinoJson library. 350-byte receive buffer, 115200 baud.

**Two-level pin mapping** (non-obvious): chip wiring tables define which DIP pin number corresponds to each logical function (A0, A1, IO0, !CE, etc.). The board wiring table then resolves DIP pin positions to physical Arduino GPIO numbers. Adding a new chip means defining its DIP-pin-to-function mapping in `chip_wiring.h`; changing the Arduino board means updating the DIP-to-GPIO table in `board_wiring.h`.

**Wiring type** is a compile-time choice in `eeprom_programmer.ino` — toggle between `BoardWiringType::DIP24` (AT28C04, AT28C16) and `BoardWiringType::DIP28` (AT28C64, AT28C256). Must match the physical wiring and requires recompilation.

### CLI (`eeprom_programmer_cli/`)

- `cli.py` — argparse entry point with `--read`, `--write`, `--erase`, `--verify` commands
- `core/eeprom_programmer_client.py` — `EepromProgrammerClient`: wraps JSON-RPC calls, handles 64-byte page read/write
- `serial_json_rpc/client.py` — `SerialJsonRpcClient`: pyserial connection, JSON-RPC request/response framing

**Data flow**: CLI sends JSON-RPC over serial -> firmware parses and dispatches -> firmware manipulates EEPROM via GPIO -> JSON-RPC response back. The architecture follows a "smart client, simple board" pattern — the CLI holds business logic while the firmware exposes only basic page read/write primitives.

### Backup versions (`bak/`)

Contains three earlier firmware iterations showing the project's evolution:
- `eeprom_api__1__rw_ops/` — LCD UI + button control, 9600 baud, 10-cycle demos
- `eeprom_api__2__performance/` — autonomous performance testing, 57600 baud, RDY/!BUSY polling
- `eeprom_api__3__debugging/` — xxd-style hex dumps, 115200 baud, full address space testing

The `xxd_arduino.txt` and `xxd_xgecu.txt` are hex dumps for verifying read integrity against a reference XGecu programmer.

## Arduino Platform Differences

The firmware uses **no conditional compilation** (`#ifdef`). A single codebase compiles for all platforms. Platform-specific behavior:

- **AT28C256 page write** (`_can_write_pages` flag in `eeprom_programmer_lib.h`): unconditionally enabled for AT28C256. Page write requires each successive byte written within 150 us (tBLC). Arduino MEGA's `digitalWrite` takes ~400 us per byte — too slow, will cause silent data corruption. Arduino DUE at 84 MHz achieves ~110 us (within spec). There is no runtime platform detection; the comment warns "doesn't work on Arduino MEGA / enable if use Arduino DUE only" but the flag is always set.
- **Arduino Giga**: commit 908064f fixed type casting issues (`int32_t` vs `int`/`long` on ARM) to allow compilation, but Giga is not a validated platform.
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
./eeprom_programmer_cli/cli.py <port> -p <chip> --write <file>                  # erase + write file to chip
./eeprom_programmer_cli/cli.py <port> -p <chip> --write <file> --skip-erase     # write without erase
./eeprom_programmer_cli/cli.py <port> -p <chip> --erase --erase-pattern FF      # erase with pattern
./eeprom_programmer_cli/cli.py <port> -p <chip> --verify <file>                 # read and compare to file
```

Add `--collect-performance` to any operation for timing data.

## Key Design Details

- **!WE pin safety**: during read operations, the EEPROM's `!WE` pin MUST be physically connected to VCC via jumper wire. Arduino's serial reset causes voltage transients (~2V, lasting ~3 sec) that can trigger unintended writes. Remove the jumper only for write/erase operations.
- **Write polling strategies**: AT28C64 has a dedicated RDY/!BUSY pin (faster, simpler polling). Other chips use data polling (read-back loop until written value matches). Both achieve ~400-450 us write time. Controlled by `_polling()` method which auto-selects based on pin availability.
- **Page size**: both read and write use 64-byte pages (`_MAX_PAGE_SIZE = 64`). This is also the AT28C256's hardware page buffer size.
- **JSON-RPC buffer**: 350 bytes max per message, tuned for Arduino UNO R3 memory constraints. Messages delimited by `\n`.
- **Test binaries** (`test_bin/`): two variants per chip — raw data (`64_the_red_migration.bin`) and padded to full chip size with 0xFF (`64_the_red_migration_AT28C64_ff.bin`). The number prefix corresponds to the chip model (4=AT28C04, 16=AT28C16, 64=AT28C64, 256=AT28C256). The `--verify` command requires the padded `_ff` variant.
