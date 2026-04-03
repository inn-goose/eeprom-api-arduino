---
name: Performance bottleneck analysis
description: Detailed breakdown of where time is spent in read/write operations — ArduinoJson is the dominant cost
type: project
---

## Per-page cost breakdown (AT28C256 read, 64-byte pages)

| Cost | MEGA (164ms/page) | DUE (109ms/page) |
|---|---|---|
| ArduinoJson parse+serialize+heap | ~102ms (62%) | ~53ms (48%) |
| Serial wire (329 bytes at 115200) | ~29ms (17%) | ~29ms (26%) |
| Python 50ms poll sleep (avg) | ~25ms (15%) | ~25ms (23%) |
| GPIO (64 × digitalWrite/Read) | ~8ms (5%) | ~3ms (3%) |

## Key insight
The dominant cost is ArduinoJson on the microcontroller, NOT the serial wire. Each page requires heap-allocating DynamicJsonDocuments, building a JsonArray of 64 elements, serializing to decimal strings, then clear+garbageCollect. On MEGA's 16MHz AVR this is ~100ms per page.

## Baud rate increase alone doesn't help much
921600 baud + 5ms poll with JSON-RPC: MEGA drops from 84s → 61s (only 1.4x). The ArduinoJson CPU cost doesn't change with baud rate.

## Binary protocol projections (with fixed Python polling)
- MEGA read AT28C256: 84s → ~8s (10x)
- DUE read AT28C256: 56s → ~5s (11x)
- MEGA write AT28C256: 305s → ~23s (13x)
- DUE write AT28C256: 85s → ~7s (12x)

Wire bytes per page: JSON ~323 vs binary ~69 (4.7x ratio). The 10x total speedup comes from eliminating BOTH wire overhead AND ArduinoJson CPU cost.

**Why:** User is considering binary protocol migration (already uses binary in another project).
**How to apply:** When discussing performance or protocol changes, reference these numbers. The ArduinoJson overhead is the key insight — it's CPU-bound, not I/O-bound.
