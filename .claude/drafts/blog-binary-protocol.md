---
date: 2026-XX-XX
###
title: "EEPROM Programmer: Migrating from JSON-RPC to Binary Protocol"
###
description: "Replacing JSON-RPC + ArduinoJson with a binary serial protocol: 2.7x faster full write cycle, 4.4x faster reads, 31% less flash, zero external dependencies."
summary: "Replacing JSON-RPC + ArduinoJson with a binary serial protocol: 2.7x faster full write cycle, 4.4x faster reads, 31% less flash, zero external dependencies."
###
tags: [eeprom-programmer, performance, arduino]
series: ["EEPROM Programmer"]
series_order: 9
---

{{< alert "fire" >}}
During read operations with the EEPROM Programmer, the chip's `!WE` pin **MUST** be connected to `VCC` using a jumper wire to disable the write mode. Otherwise, invoking the CLI may corrupt data on the chip due to Arduino's internal behavior. [Details](/blog/eeprom-programmer-5-data-corruption/)
{{< /alert >}}


## TLDR

The EEPROM Programmer's serial protocol was migrated from JSON-RPC (with ArduinoJson dependency) to a lightweight binary protocol. On Arduino DUE + AT28C64: reads are **4.4x faster** (6.96s → 1.58s), full write cycles are **2.7x faster** (33.74s → 12.70s), flash usage dropped **31%** (16KB saved), and the only external firmware dependency was eliminated. Nine bugs were caught during iterative code review before hardware testing.


## Motivation

The [EEPROM Programmer](https://github.com/inn-goose/eeprom-programmer) uses a serial connection between the Arduino board and a Python CLI to read and write EEPROM chips. The original implementation used [JSON-RPC 2.0](/blog/eeprom-programmer-4-serial-json-rpc-api/) over serial, backed by the [ArduinoJson](https://arduinojson.org/) library.

### What JSON-RPC Gave Us

- **Serial Monitor debugging**: type raw JSON in Arduino Serial Monitor to test wiring and diagnose issues — no tooling needed. Critical during early hardware development when the CLI didn't exist yet.
- **Self-describing errors**: `"Failed to init AT28C256 chip with error: 12"` vs an opcode. When debugging new chip support, readable error messages saved significant time.
- **Minimal client code**: the Python client was ~130 lines of `json.dumps`/`json.loads`. No protocol spec, no byte-level framing.
- **Development velocity**: adding a new RPC method = one string comparison in the handler + one `send_request` call in Python. The [blog series](/tags/eeprom-programmer/) used JSON-RPC in Serial Monitor as a teaching tool.

### Where It Hurts

Per-page cost breakdown (AT28C256 read, 64-byte pages):

| Cost | MEGA (164ms/page) | DUE (109ms/page) |
|---|---|---|
| ArduinoJson parse+serialize+heap | ~102ms (62%) | ~53ms (48%) |
| Serial wire (329 bytes at 115200) | ~29ms (17%) | ~29ms (26%) |
| Python 50ms poll sleep (avg) | ~25ms (15%) | ~25ms (23%) |
| GPIO (64 × digitalWrite/Read) | ~8ms (5%) | ~3ms (3%) |

The dominant cost is **ArduinoJson on the microcontroller**, not the serial wire. Each page requires heap-allocating `DynamicJsonDocument`s, building a `JsonArray` of 64 elements, serializing to decimal-string JSON, then `clear()` + `garbageCollect()`. On MEGA's 16MHz AVR with software heap, this is ~100ms per page. Increasing baud rate alone barely helps (921600 baud + 5ms poll: MEGA drops from 84s → 61s, only 1.4x).

Additional concerns:
- ArduinoJson v6 → v7 breaking change (`DynamicJsonDocument` removed entirely)
- ~6KB flash + ~1KB RAM consumed by ArduinoJson alone
- 350-byte receive buffer needed for JSON parsing
- ArduinoJson was the only external firmware dependency


## Measurements: Before

**Setup**: Arduino DUE + AT28C64 (8KB), ArduinoJson v7.4.2, 115200 baud

| Operation | Time |
|---|---|
| Read 8KB | 6.96s |
| Erase 8KB | 13.85s |
| Write only 8KB | 12.91s |
| Write + in-session verify | 19.88s (12.87 + 7.01) |
| **Full (erase + write + verify)** | **33.74s** (13.86 + 12.91 + 6.97) |

Firmware flash: 50,936 bytes (9% of DUE capacity).

Wire bytes per 64-byte page:
- READ_PAGE: request ~55B, response ~323B (total ~378B)
- WRITE_PAGE: request ~320B, response ~80B (total ~400B)

Per-byte timings (`--collect-performance`):
- AVG read: 31.34 μs/byte
- AVG write: 539.56 μs/byte (erase), 546.18 μs/byte (write)

> Note: measurements use in-session write+verify (no serial reconnect between operations) to avoid [data corruption from Arduino's reset behavior](/blog/eeprom-programmer-5-data-corruption/).


## The Decision

### Options Considered

1. **Optimize JSON-RPC**: increase baud rate, reduce poll sleep — projections showed only ~1.4x improvement (MEGA: 84s → 61s). ArduinoJson CPU cost dominates, not wire time.
2. **Hybrid approach**: keep JSON-RPC for control commands (init_chip, set_mode — called once), binary only for bulk transfer (read_page, write_page — called hundreds of times). Reasonable, but still requires ArduinoJson for the control path.
3. **Full binary protocol**: eliminate ArduinoJson entirely. More work upfront, but removes the only external dependency and gives maximum performance.

### Why Full Binary

ArduinoJson is too heavy for a microcontroller that just needs to shuttle 64-byte pages between serial and GPIO. The library was designed for general-purpose JSON manipulation — heap allocation, tree traversal, polymorphic types — none of which this protocol needs. The v6 → v7 breaking change (`DynamicJsonDocument` removed, `JsonDocument` redesigned) forced a migration decision: update to v7 or replace entirely. Given that ArduinoJson accounted for 62% of per-page time and was the sole external dependency, replacement made more sense than update.


## Protocol Design

### Frame Format

```
[0xAA] [0x55] [LEN_L] [LEN_H] [BODY...] [CRC_L] [CRC_H]
```

- **Sync word**: `0xAA 0x55` — classic alternating-bit pattern, same approach as STK500v2 and AVR bootloaders
- **LEN**: uint16 LE — byte count of BODY only
- **BODY**: `CMD(1) | SEQ(1) | PAYLOAD(0..N)`
- **CRC-16/CCITT**: poly `0x1021`, init `0xFFFF`, computed over BODY bytes
- All multi-byte integers: **little-endian** (native on both AVR and ARM)

### Design Decisions

| Decision | Rationale |
|---|---|
| Length-prefix over COBS/SLIP | Zero encoding overhead on the 64-byte hot path |
| CRC-16/CCITT | Industry standard (HDLC, X.25, Bluetooth), catches all single/double-bit errors and burst errors ≤16 bits |
| Bit-by-bit CRC (no lookup table) | Saves 256 bytes flash, negligible speed difference at 115200 baud |
| SEQ byte | Detects stale responses after timeout/retry — prevents silent data corruption |
| Little-endian | Native on both AVR and ARM — zero-cost serialization |
| Single Serial.write() per frame | Full frame built in contiguous buffer, no flush() — avoids per-call overhead and separate USB packets |

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

### Wire Bytes Comparison

| Message | Binary | JSON-RPC | Reduction |
|---|---|---|---|
| READ_PAGE request | 10B | ~55B | 5.5x |
| READ_PAGE response (64B) | 72B | ~323B | 4.5x |
| WRITE_PAGE request (64B) | 74B | ~320B | 4.3x |
| WRITE_PAGE response | 8B | ~80B | 10x |


## Migration Strategy

The migration was done in incremental commits to keep diffs small and reviewable:

1. **Add firmware protocol handler** (`binary_protocol.h`): new file, included in `.ino` but not wired — purely additive, existing behavior unchanged
2. **Add Python protocol client** (`binary_protocol/client.py`): new file with 32 unit tests, not wired — purely additive
3. **Switchover**: rewire both firmware `.ino` and Python `eeprom_programmer_client.py` to use binary protocol — the only commit that changes behavior
4. **Delete old JSON-RPC files**: dead code removal
5. **Update documentation**: README, CLAUDE.md

Commits 1-2 added all new code without changing any behavior. Commit 3 was the only risky one, but small because all new code was already in place and tested. Each step was compiled, uploaded, and validated on hardware before committing.


## Measurements: After

**Setup**: Arduino DUE + AT28C64 (8KB), binary protocol, 115200 baud

### Without `--collect-performance`

| Operation | Before | After | Speedup |
|---|---|---|---|
| Read 8KB | 6.96s | 1.58s | **4.4x** |
| Erase 8KB | 13.85s | 5.77s | **2.4x** |
| Write only 8KB | 12.91s | 5.35s | **2.4x** |
| **Full (erase + write + verify)** | **33.74s** | **12.70s** | **2.7x** |

### With `--collect-performance`

| Operation | Before | After | Speedup |
|---|---|---|---|
| Erase 8KB | 20.74s | 7.87s | **2.6x** |
| Write 8KB | 19.35s | 7.32s | **2.6x** |
| Verify (read) 8KB | 13.86s | 3.68s | **3.8x** |
| **Full cycle** | **53.95s** | **18.87s** | **2.9x** |

Per-byte timings are identical (same EEPROM, same GPIO):
- AVG read: 31.34 → 30.00 μs/byte
- AVG write: 539.56 → 539.03 μs/byte

### Why Not 10x?

The original 10x projection was for MEGA (ArduinoJson ~100ms/page). On DUE it was ~53ms/page — less overhead to remove. Write operations have a hardware floor: EEPROM write polling takes ~400-540μs/byte regardless of protocol. For 8KB that's ~3.3s minimum. The protocol improvement is ~4x on serial overhead, but the hardware floor masks it:

- **Read**: near-zero hardware time → total speedup shows full protocol improvement (4.4x)
- **Write**: ~3.3s hardware floor out of 5.35s → protocol improvement visible but diluted (2.4x)

### Firmware Size

| | Before | After | Saved |
|---|---|---|---|
| Flash | 50,936 B (9%) | 34,936 B (6%) | **16,000 B (-31%)** |
| RAM | TODO | TODO | TODO |

ArduinoJson library dependency eliminated entirely.


## Bugs Found During Review

The iterative review process (write → challenge → fix → re-challenge) caught nine bugs before they reached hardware testing:

### Firmware (`binary_protocol.h`)

1. **WAIT_SYNC2 missed valid sync**: `0xAA 0xAA 0x55` failed to sync — the second `0xAA` reset to WAIT_SYNC1 instead of staying in WAIT_SYNC2. Any stray `0xAA` byte before a valid frame would cause the frame to be missed.
2. **Multiple Serial.write() calls per frame**: 7 individual calls per frame, each with function call overhead and potentially triggering separate USB packets on DUE's Programming Port. Fixed: build complete frame in a contiguous buffer, single Serial.write() call.
3. **No bounds check in _send_frame**: a payload exceeding 128 bytes would overflow `_send_buf` via memcpy. Fixed: early return if frame exceeds `MAX_SEND_FRAME_SIZE`.
4. **Serial.flush() blocking on hot path**: `flush()` waits for TX buffer to drain — ~6ms of CPU blocking per page at 115200 baud. Over 128 pages (8KB read), that's ~800ms wasted. Removed: TX drains asynchronously via hardware UART.

### Python Client (`binary_protocol/client.py`)

5. **SEQ not validated in send_command**: stale responses from previous commands accepted silently. Two consecutive READ_PAGE commands with different page numbers could return the wrong page's data — silent data corruption.
6. **Error frame bypassed SEQ check**: `_handle_error()` raised an exception before the SEQ validation ran. A stale error from a previous command was accepted as the current command's error.
7. **No upper bound on body_len in _read_frame**: a corrupted LEN field of 0xFFFF would cause `serial.read(65535)` — 64KB allocation and a long blocking read. Fixed: cap at `MAX_BODY_SIZE`.
8. **Sync detection was fragile nested loops**: 25 lines of nested while loops with a shared `b3` variable. Replaced with a clean 12-line state machine matching the firmware's pattern.

### Integration (`.ino` ↔ `client.py`)

9. **Unknown command used error_code=0**: the firmware sent error code 0 for unrecognized commands, which is the same as `ErrorCode::SUCCESS`. Changed to 0xFFFF to avoid confusion.


## Summary

The migration from JSON-RPC to a binary protocol delivered a 2.7x speedup on full write cycles (4.4x on reads), a 31% reduction in flash usage, and eliminated the only external firmware dependency. The iterative review process — write code, challenge it, fix bugs, re-challenge — caught 9 bugs before hardware testing, including a silent data corruption bug (SEQ not validated) and a sync detection bug that would have caused intermittent frame drops. The protocol is simple (223 lines of C++, 203 lines of Python, 32 unit tests) and the incremental migration strategy kept risk contained to a single commit.
