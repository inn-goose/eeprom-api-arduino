---
name: Blog post — binary protocol migration
description: Datapoints and structure for blog post about migrating from JSON-RPC to binary serial protocol
type: project
---

## Blog post about migrating from JSON-RPC to binary serial protocol

**Why:** User plans to write a post on goose.sh documenting this refactoring. Datapoints must be collected during implementation.

### Agreed structure

1. TL;DR
2. Motivation: what JSON-RPC gave us (debugging, Serial Monitor, dev velocity) and where it hurts (ArduinoJson is 62% of cost)
3. Measurements "before" (detailed, with per-page breakdown)
4. The decision: options considered → why full binary (not hybrid, not just optimizing JSON)
5. Protocol design (frame format, CRC, commands — the technical core)
6. Migration strategy: incremental commits, how we kept things testable
7. Measurements "after" (same operations, side-by-side comparison)
8. Summary + what's next

Note: jumper/corruption issue is a sidebar in the measurements section (explains measurement methodology — in-session verify needed to avoid reconnect corruption).

### "Before" datapoints (DUE + AT28C64 + ArduinoJson v7.4.2)

- Read 8KB: 13.90s
- Write+Verify 8KB: 55.4s (erase 20.73s + write 20.80s + verify 13.89s)
- Per-page cost breakdown: ArduinoJson 62%, serial wire 17%, Python poll sleep 15%, GPIO 5%
- Wire bytes per page: READ_PAGE req ~55B / resp ~323B, WRITE_PAGE req ~320B / resp ~80B
- Firmware flash size: TODO (capture before any code changes)
- Firmware RAM usage: TODO (capture before any code changes)
- Per-byte usec timings from --collect-performance: TODO (save raw output)

### "After" datapoints (to collect during commit 4 verification)

- Read 8KB time: TODO
- Write+Verify 8KB time: TODO
- Firmware flash size without ArduinoJson: TODO
- Firmware RAM usage without ArduinoJson: TODO
- Per-byte usec timings: TODO
- Wire bytes per page: READ_PAGE req 10B / resp 72B, WRITE_PAGE req 74B / resp 8B

**How to apply:** Collect TODO items during implementation. Save raw CLI and compile outputs.
