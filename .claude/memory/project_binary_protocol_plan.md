---
name: Binary serial protocol migration plan
description: Protocol spec, 6-commit incremental plan, and blog post datapoints for replacing JSON-RPC + ArduinoJson
type: project
---

## Goal

Replace JSON-RPC + ArduinoJson with a binary serial protocol. Eliminates the only external firmware dependency, reduces RAM usage by 63%, and yields projected ~10x speedup for bulk operations.

**Why:** ArduinoJson is 62% of per-page read time on MEGA (~102ms of 164ms). It consumes ~6KB flash + ~1KB RAM. Has v6→v7 breaking change. Binary protocol eliminates all of this.

## Baseline Performance (DUE + AT28C64 + ArduinoJson v7.4.2)

Measured 2026-04-06:

| Operation | Time |
|---|---|
| Read 8KB | 6.96s |
| Erase 8KB | 13.85s |
| Write only 8KB | 12.91s |
| Write + in-session verify | 19.88s |
| Full (erase + write + verify) | 33.74s |

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
- Single Serial.write() per frame — full frame built in contiguous buffer, no flush()
- Receive buffer: 68 bytes (vs current 350). Send buffer: 136 bytes (full frame).

### Board State Machine

```
States: SYNC1 → SYNC2 → LEN_L → LEN_H → BODY (N bytes) → CRC_L → CRC_H → dispatch
```

### Python Host

Blocking `serial.read(n)` with timeout — eliminates the 50ms poll sleep.

## Implementation Progress

### Step 1: DONE — `binary_protocol.h` created, included in .ino but not wired

Bugs found and fixed during review:
- **WAIT_SYNC2 missed valid sync**: `0xAA 0xAA 0x55` failed to sync because second 0xAA reset to WAIT_SYNC1. Fixed: stay in WAIT_SYNC2 on 0xAA.
- **Multiple Serial.write() calls per frame**: 7 calls per frame, each with function call overhead and potential separate USB packets. Fixed: build full frame in contiguous `_send_buf`, single Serial.write().
- **No bounds check in _send_frame**: buffer overflow if payload exceeded 128 bytes. Fixed: early return if frame_len > MAX_SEND_FRAME_SIZE.
- **Serial.flush() blocking on hot path**: ~6ms per page of CPU blocking while TX drains. Removed — TX drains asynchronously via hardware.

Known acceptable limitations:
- No receive timeout (stuck state machine on partial frames) — acceptable for USB serial
- VLA in send_error — GCC extension, consistent with codebase style
- CRC doesn't cover LEN field — corruption detected indirectly via CRC mismatch
- Boot frame fire-and-forget — same as JSON-RPC, handled by host init timeout

### Step 2: DONE — `binary_protocol/client.py` created with unit tests, not wired

Bugs found and fixed during review:
- **SEQ not validated in send_command**: stale responses from previous commands accepted silently. Two consecutive same-type commands could return wrong data. Fixed: compare resp_seq to expected seq, raise on mismatch.
- **No upper bound on body_len in _read_frame**: corrupted LEN field could cause 64KB read/allocation. Fixed: cap at MAX_BODY_SIZE (130).
- **Sync detection was fragile nested while loops**: replaced with clean state machine matching firmware pattern. 12 lines vs 25, same behavior.
- **Timeout semantics mixed**: removed hardcoded RESPONSE_TIMEOUT_SEC override after sync — now uses caller's timeout throughout.

31 unit tests covering: CRC (7), frame building (3), frame parsing (10), send_command (8), CRC cross-validation (3).

### Remaining Steps

3. Switchover: wire binary protocol in `.ino` + `eeprom_programmer_client.py` (atomic)
4. Delete `serial_json_rpc_lib.h` and `serial_json_rpc/` directory
5. Update README with new architecture + performance comparison

CLAUDE.md updated with every commit.

## Files

| File | Action |
|---|---|
| `eeprom_programmer/binary_protocol.h` | CREATE — DONE |
| `eeprom_programmer/eeprom_programmer.ino` | MODIFY (include added) — DONE |
| `eeprom_programmer_cli/binary_protocol/__init__.py` | CREATE — DONE |
| `eeprom_programmer_cli/binary_protocol/client.py` | CREATE — DONE |
| `eeprom_programmer_cli/binary_protocol/test_client.py` | CREATE — DONE |
| `eeprom_programmer_cli/core/eeprom_programmer_client.py` | MODIFY |
| `eeprom_programmer/serial_json_rpc_lib.h` | DELETE |
| `eeprom_programmer_cli/serial_json_rpc/` | DELETE |

## Verification

1. Compile for Arduino DUE — no ArduinoJson dependency
2. Unit-test CRC on both sides
3. Full erase → write → in-session-verify cycle on AT28C64
4. Compare performance to baseline

**How to apply:** This memory tracks migration progress. Check step status before continuing work.
