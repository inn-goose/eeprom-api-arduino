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
- No `while (!Serial)` for native USB boards
- 350-byte buffer margin tight for write_page
- `json_array_to_byte_array` undersizes its JsonDocument

## ArduinoJson version (discovered 2026-04-06)
- Code uses `DynamicJsonDocument` (ArduinoJson v6 API)
- **v6.21.5 does NOT work** — `DynamicJsonDocument(serial_read_buffer_pos)` undersizes allocation, causes `NoMemory` on every request
- **v7.4.2 WORKS** — provides compatibility shim for `DynamicJsonDocument`, handles allocation correctly
- User's Arduino IDE uses v7.4.2; this is the version that was always used in practice

## Baseline performance (DUE + AT28C64 + ArduinoJson v7.4.2)
- Read 8KB: 13.90s
- Write 8KB: 20.72s (byte-by-byte with RDY/!BUSY polling)
- Erase 8KB: 20.74s
- Verify: 13.90s (same as read)

## 10K resistor on !WE (tested 2026-04-06)
- Reddit user nib85 suggested replacing !WE jumper with 10K pull-up resistor
- **Does NOT fully work** — address 0x0000 still gets corrupted to 0xFF on Arduino serial reset
- Jumper wire approach (swap between write/read) remains the working solution

**Why:** Track what was fixed and what remains for future sessions.
**How to apply:** If user asks about remaining bugs or wants to continue fixing, reference the "not yet fixed" list. Use ArduinoJson v7.4.2 for compilation until binary protocol migration removes the dependency entirely.
