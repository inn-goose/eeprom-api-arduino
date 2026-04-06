---
date: 2026-XX-XX
###
title: "EEPROM Programmer: Migrating from JSON-RPC to Binary Protocol"
###
description: "TODO"
summary: "TODO"
###
tags: [eeprom-programmer, performance, arduino]
series: ["EEPROM Programmer"]
series_order: 9
---

{{< alert "fire" >}}
During read operations with the EEPROM Programmer, the chip's `!WE` pin **MUST** be connected to `VCC` using a jumper wire to disable the write mode. Otherwise, invoking the CLI may corrupt data on the chip due to Arduino's internal behavior. [Details](/blog/eeprom-programmer-5-data-corruption/)
{{< /alert >}}


## TLDR

TODO: 3-4 sentences. The EEPROM Programmer's serial protocol was migrated from JSON-RPC (with ArduinoJson dependency) to a lightweight binary protocol. Result: ~Xx faster read/write, ArduinoJson dependency eliminated, XX KB flash and XX bytes RAM saved.


## Motivation

The [EEPROM Programmer](https://github.com/inn-goose/eeprom-programmer) uses a serial connection between the Arduino board and a Python CLI to read and write EEPROM chips. The original implementation used [JSON-RPC 2.0](/blog/eeprom-programmer-4-serial-json-rpc-api/) over serial, backed by the [ArduinoJson](https://arduinojson.org/) library.

### What JSON-RPC Gave Us

TODO: expand each point with 1-2 sentences

- **Serial Monitor debugging**: type raw JSON in Arduino Serial Monitor to test wiring and diagnose issues — no tooling needed
- **Self-describing errors**: `"Failed to init AT28C256 chip with error: 12"` vs an opcode
- **Minimal client code**: Python client was ~130 lines of `json.dumps`/`json.loads`
- **Development velocity**: adding a new RPC method = string comparison + handler

### Where It Hurts

TODO: expand with real numbers

Per-page cost breakdown (DUE + AT28C64, 64-byte pages):

| Cost | Time | % |
|---|---|---|
| ArduinoJson parse+serialize+heap | TODO ms | 62% |
| Serial wire (329 bytes at 115200) | ~29ms | 17% |
| Python 50ms poll sleep (avg) | ~25ms | 15% |
| GPIO (64 x digitalWrite/Read) | TODO ms | 5% |

Additional concerns:
- ArduinoJson v6 → v7 breaking change (`DynamicJsonDocument` removed)
- ~6KB flash + ~1KB RAM consumed by ArduinoJson alone
- 350-byte receive buffer for JSON parsing
- The only external firmware dependency


## Measurements: Before

**Setup**: Arduino DUE + AT28C64 (8KB), ArduinoJson v7.4.2, 115200 baud

| Operation | Time |
|---|---|
| Read 8KB | 13.90s |
| Erase 8KB | 20.73s |
| Write 8KB | 20.80s |
| Verify 8KB | 13.89s |
| **Write + Verify (full cycle)** | **55.4s** |

Firmware size:
- Flash: TODO bytes (XX% of DUE capacity)
- RAM: TODO bytes

Wire bytes per 64-byte page:
- READ_PAGE: request ~55B, response ~323B (total ~378B)
- WRITE_PAGE: request ~320B, response ~80B (total ~400B)

TODO: per-byte usec timings from `--collect-performance`, maybe a chart

> Note: measurements use in-session write+verify (no serial reconnect between operations) to avoid [data corruption from Arduino's reset behavior](/blog/eeprom-programmer-5-data-corruption/).


## The Decision

TODO: describe the options considered and why full binary was chosen over hybrid or JSON optimization

### Options Considered

1. **Optimize JSON-RPC**: increase baud rate, reduce poll sleep — projections showed only ~1.4x improvement (MEGA: 84s → 61s). ArduinoJson CPU cost dominates, not wire time.
2. **Hybrid approach**: keep JSON-RPC for control commands (init_chip, set_mode — called once), binary only for bulk transfer (read_page, write_page — called hundreds of times). Reasonable, but still requires ArduinoJson for control path.
3. **Full binary protocol**: eliminate ArduinoJson entirely. More work upfront, but removes the only external dependency and gives maximum performance.

### Why Full Binary

TODO: expand reasoning — ArduinoJson is too heavy for a microcontroller that just needs to shuttle bytes. The v6→v7 migration pain sealed the decision.


## Protocol Design

TODO: describe the binary protocol frame format, design decisions

### Frame Format

```
[0xAA] [0x55] [LEN_L] [LEN_H] [BODY...] [CRC_L] [CRC_H]
```

- **Sync word**: `0xAA 0x55` — classic alternating-bit pattern, same approach as STK500v2 and AVR bootloaders
- **LEN**: uint16 LE — byte count of BODY only
- **BODY**: `CMD(1) | SEQ(1) | PAYLOAD(0..N)`
- **CRC-16/CCITT**: poly `0x1021`, init `0xFFFF`, computed over BODY bytes

### Design Decisions

| Decision | Rationale |
|---|---|
| Length-prefix over COBS/SLIP | Zero encoding overhead on the 64-byte hot path |
| CRC-16/CCITT | Industry standard (HDLC, X.25, Bluetooth), catches all single/double-bit errors |
| Bit-by-bit CRC (no lookup table) | Saves 256 bytes flash, negligible speed difference |
| SEQ byte | Detects stale responses after timeout/retry |
| Little-endian | Native on both AVR and ARM — zero-cost |

### Commands

TODO: command table

### Wire Bytes Comparison

| Message | Binary | JSON-RPC | Reduction |
|---|---|---|---|
| READ_PAGE request | 10B | ~55B | 5.5x |
| READ_PAGE response (64B) | 72B | ~323B | 4.5x |
| WRITE_PAGE request (64B) | 74B | ~320B | 4.3x |
| WRITE_PAGE response | 8B | ~80B | 10x |


## Migration Strategy

TODO: describe the incremental approach — 6 commits, only 1 changes behavior

The migration was done in 6 incremental commits to keep diffs small and reviewable:

1. **Add firmware protocol handler** (`binary_protocol.h`): new file, not wired yet — purely additive
2. **Add Python protocol client** (`binary_protocol/client.py`): new file, not wired yet — purely additive
3. **Switchover**: rewire both firmware and Python to use binary protocol — the only commit that changes behavior
4. **Delete old JSON-RPC files**: dead code removal
5. **Update documentation**

TODO: highlight what made this safe — commits 1-2 add code without changing behavior, commit 3 is the only risky one but small because all new code was already in place


## Measurements: After

**Setup**: Arduino DUE + AT28C64 (8KB), binary protocol, 115200 baud

| Operation | Before | After | Speedup |
|---|---|---|---|
| Read 8KB | 13.90s | TODO | TODO |
| Erase 8KB | 20.73s | TODO | TODO |
| Write 8KB | 20.80s | TODO | TODO |
| Verify 8KB | 13.89s | TODO | TODO |
| **Write + Verify** | **55.4s** | **TODO** | **TODO** |

Firmware size:
- Flash: TODO bytes (saved TODO bytes / TODO%)
- RAM: TODO bytes (saved TODO bytes / TODO%)

TODO: per-byte usec timings comparison, maybe a chart


## Summary

TODO: 3-5 sentences wrapping up the migration results and what's next
