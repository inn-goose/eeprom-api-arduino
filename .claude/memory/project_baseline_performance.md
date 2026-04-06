---
name: Baseline performance measurements (pre-migration)
description: Isolated operation timings for DUE + AT28C64 (8KB) with JSON-RPC + ArduinoJson v7 — measured 2026-04-06
type: project
---

## Baseline: DUE + AT28C64 (8KB) + JSON-RPC + ArduinoJson v7.4.2

Measured 2026-04-06 with `64_the_red_migration.bin` (7593 bytes).

| # | Operation | Time |
|---|---|---|
| 1 | Read | 6.96s |
| 2 | Erase | 13.85s |
| 3 | Write only | 12.91s |
| 5 | Write + in-session verify | 19.88s (12.87 + 7.01) |
| 6 | Full (erase + write + verify) | 33.74s (13.86 + 12.91 + 6.97) |

Standalone verify was skipped — it's identical to a read, and switching !WE jumper mid-session isn't possible for in-session operations.

**Why:** Pre-migration baseline for comparing binary protocol performance.
**How to apply:** Compare post-migration timings against these numbers to measure actual speedup.
