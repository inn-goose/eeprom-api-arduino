---
name: JSON-RPC vs binary protocol tradeoffs
description: Why JSON-RPC is still valuable despite 10x performance penalty, and hybrid approach option
type: project
---

## Why JSON-RPC is still useful
1. **Serial Monitor debugging** — type raw JSON to test wiring, new chips, diagnose issues. No tooling needed. Critical for hardware dev.
2. **Self-describing errors** — "Failed to init AT28C256 chip with error: 12" vs an opcode.
3. **Minimal client code** — Python client is ~130 lines of json.dumps/json.loads.
4. **Development velocity** — new RPC method = string comparison + handler. No byte-level spec.
5. **Blog series** — companion posts use JSON-RPC in Serial Monitor as teaching tool.
6. **Use frequency** — EEPROM programming is occasional, not a tight loop.

## Hybrid approach (recommended if migrating)
Keep JSON-RPC for control commands (init_chip, set_read_mode, set_write_mode — called once, debuggability matters) and use binary only for bulk transfer (read_page, write_page — called hundreds of times, speed matters).

## User context
User already uses binary protocol in another project and it works well. They're seriously considering the migration.

**Why:** Captures the full analysis so future conversations about protocol changes have context.
**How to apply:** If user decides to implement binary protocol, start with the hybrid approach. Don't lose the Serial Monitor debugging capability for init/config commands.
