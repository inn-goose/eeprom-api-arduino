---
name: Binary serial protocol migration plan
description: Protocol spec, 6-commit incremental plan, and blog post datapoints for replacing JSON-RPC + ArduinoJson
type: project
---

## Goal

Replace JSON-RPC + ArduinoJson with a binary serial protocol. Eliminates the only external firmware dependency, reduces RAM usage by 63%, and yields projected ~10x speedup for bulk operations.

**Why:** ArduinoJson is 62% of per-page read time on MEGA (~102ms of 164ms). It consumes ~6KB flash + ~1KB RAM. Has v6→v7 breaking change. Binary protocol eliminates all of this.

## Baseline Performance (DUE + AT28C64 + ArduinoJson v7.4.2)

| Operation | Time |
|---|---|
| Read 8KB | 13.90s |
| Write 8KB | 20.72s |
| Erase 8KB | 20.74s |

## Protocol Design

### Frame Format

```
[0xAA] [0x55] [LEN_L] [LEN_H] [BODY...] [CRC_L] [CRC_H]
```

- Sync: 0xAA 0x55
- LEN: uint16 LE, byte count of BODY only
- BODY: CMD(1) | SEQ(1) | PAYLOAD(0..N)
- CRC-16/CCITT (poly 0x1021, init 0xFFFF) over BODY bytes
- All multi-byte integers: little-endian (native on AVR and ARM)

### Commands

| CMD | Name | Direction | Payload |
|-----|------|-----------|---------|
| 0x01 | INIT_CHIP | req | chip_name (null-terminated) |
| 0x02 | SET_READ_MODE | req | page_size: uint16 |
| 0x03 | READ_PAGE | req | page_no: uint16 |
| 0x04 | SET_WRITE_MODE | req | page_size: uint16 |
| 0x05 | WRITE_PAGE | req | page_no: uint16 + data: uint8[N] |
| 0x06 | GET_READ_PERF | req | (none) |
| 0x07 | GET_WRITE_PERF | req | (none) |
| 0x81 | INIT_CHIP | resp | memory_size: uint32 |
| 0x82 | SET_READ_MODE | resp | (ACK, no payload) |
| 0x83 | READ_PAGE | resp | data: uint8[N] |
| 0x84 | SET_WRITE_MODE | resp | (ACK, no payload) |
| 0x85 | WRITE_PAGE | resp | (ACK, no payload) |
| 0x86 | GET_READ_PERF | resp | timings: uint16[N] |
| 0x87 | GET_WRITE_PERF | resp | timings: uint16[N] |
| 0xFE | BOOT | special | version: uint8, wiring_type: uint8, max_page_size: uint8 |
| 0xFF | ERROR | special | original_cmd: uint8, error_code: uint16, message: null-terminated |

### Key Design Decisions

- Length-prefix (not COBS/SLIP) — zero encoding overhead on payload
- CRC-16/CCITT bit-by-bit — no lookup table, saves 256B flash
- SEQ byte — detects stale responses after timeout/retry
- Little-endian — native on both AVR and ARM
- Receive buffer: 68 bytes (vs current 350). Send buffer: 130 bytes.

### Board State Machine

```
States: SYNC1 → SYNC2 → LEN_L → LEN_H → BODY (N bytes) → CRC_L → CRC_H → dispatch
```

### Python Host

Blocking `serial.read(n)` with timeout — eliminates the 50ms poll sleep.

## Implementation Steps (incremental, 6 commits)

1. Create `eeprom_programmer/binary_protocol.h` (additive, not wired)
2. Create `eeprom_programmer_cli/binary_protocol/client.py` (additive, not wired)
3. Switchover: wire binary protocol in `.ino` + `eeprom_programmer_client.py` (atomic)
4. Delete `serial_json_rpc_lib.h` and `serial_json_rpc/` directory
5. Update README with new architecture + performance comparison

CLAUDE.md updated with every commit.

## Files

| File | Action |
|---|---|
| `eeprom_programmer/binary_protocol.h` | CREATE |
| `eeprom_programmer/eeprom_programmer.ino` | MODIFY |
| `eeprom_programmer_cli/binary_protocol/__init__.py` | CREATE |
| `eeprom_programmer_cli/binary_protocol/client.py` | CREATE |
| `eeprom_programmer_cli/core/eeprom_programmer_client.py` | MODIFY |
| `eeprom_programmer/serial_json_rpc_lib.h` | DELETE |
| `eeprom_programmer_cli/serial_json_rpc/` | DELETE |
| `eeprom_programmer/eeprom_programmer_lib.h` | MODIFY (pow, micros fixes) |

## Verification

1. Compile for Arduino DUE — no ArduinoJson dependency
2. Unit-test CRC on both sides
3. Full erase → write → in-session-verify cycle on AT28C64
4. Compare performance to baseline

**How to apply:** Full plan file at ~/.claude/plans/tender-weaving-nebula.md. This memory is a summary for future sessions.
