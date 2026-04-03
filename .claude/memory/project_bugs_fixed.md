---
name: Bugs found and fixed (2026-04-03 session)
description: Bugs identified and fixed in eeprom_programmer_lib.h and serial_json_rpc_lib.h during code review
type: project
---

## Fixed (committed as f9615f9)
1. **Uninitialized loop vars** — `for (int i;` → `for (int i = 0;` in `get_read_byte_usec_for_page` and `get_write_byte_usec_for_page` (lines 84, 93)
2. **Buffer overflow in `address_to_hex_string`** — `char buf[8]` → `char buf[8 + 1]` (8 hex chars + null terminator)
3. **Buffer overflow in `data_to_hex_string`** — `char buf[2]` → `char buf[2 + 1]` (2 hex chars + null terminator)
4. **Dead code `address < 0` on uint32_t** — removed from `_read_byte` and `_write_byte`
5. **Wrong array size for `_current_data`** — `MAX_ADDRESS_BUS_SIZE` (15) → `MAX_DATA_BUS_SIZE` (8). Worked by accident since 15 > 8.
6. **`size_t` returning -1** — `json_array_to_byte_array` returned -1 on error (wraps to UINT_MAX). Changed to 0.

## Identified but NOT yet fixed (in Potential Refactoring section of CLAUDE.md)
- `pow(2, n)` in hot loop (`_address_to_bits_array`) — should be bit shift
- `micros()` overflow in polling loops — unsafe addition pattern
- `_data_polling` toggles entire data bus twice per byte
- ArduinoJson v7 removed `DynamicJsonDocument`
- No `while (!Serial)` for native USB boards
- 350-byte buffer margin tight for write_page
- `json_array_to_byte_array` undersizes its JsonDocument

**Why:** Track what was fixed and what remains for future sessions.
**How to apply:** If user asks about remaining bugs or wants to continue fixing, reference the "not yet fixed" list.
