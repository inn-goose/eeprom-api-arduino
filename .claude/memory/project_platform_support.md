---
name: Arduino platform support
description: Which Arduino boards are supported, tested, and unsupported for the EEPROM programmer
type: project
---

- **MEGA**: validated, primary development platform. 16MHz AVR. Pins 22-53 used.
- **DUE**: validated. 84MHz ARM. ~30% faster reads, page write works for AT28C256.
- **Giga**: compiles (commit 908064f fixed ARM type casting). Shares MEGA pin layout. Had serial issues — user needs to re-verify.
- **UNO**: explicitly **not supported**. Lacks pins 22-53. The "works fine for UNO R3" comment in serial_json_rpc_lib.h refers to the JSON-RPC library's memory footprint in isolation, not EEPROM programmer compatibility.

**Why:** User corrected incorrect assumption that UNO was supported. MEGA is the minimum supported board.
**How to apply:** Never suggest UNO as a target platform. MEGA and DUE are the safe recommendations.
