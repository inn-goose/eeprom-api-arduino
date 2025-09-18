#ifndef __eeprom_wiring_h__
#define __eeprom_wiring_h__

// ========================================
// !!! REDEFINE FOR YOUR WIRING SCHEME
// ========================================

// PIN -- !BSY -- |    | -- VCC  -- VCC
// PIN --  A12 -- |    | -- !WE  -- PIN
// PIN --   A7 -- |    | -- NC   -- –––
// PIN --   A6 -- |    | -- A8   -- PIN
// PIN --   A5 -- |    | -- A9   -- PIN
// PIN --   A4 -- |    | -- A11  -- PIN
// PIN --   A3 -- |    | -- !OE  -- PIN
// PIN --   A2 -- |    | -- A10  -- PIN
// PIN --   A1 -- |    | -- !CE  -- PIN
// PIN --   A0 -- |    | -- IO7  -- PIN
// PIN --  IO0 -- |    | -- IO6  -- PIN
// PIN --  IO1 -- |    | -- IO5  -- PIN
// PIN --  IO2 -- |    | -- IO4  -- PIN
// GND --  GND -- |    | -- IO3  -- PIN

// wire pins from 0 to N
// const uint8_t EEPROM_ADDRESS_PINS[] = { A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12 };
// const uint8_t EEPROM_DATA_PINS[] = { IO0, IO1, IO2, IO3, IO4, IO5, IO6, IO7 };
// const uint8_t EEPROM_CHIP_ENABLE_PIN = CE;
// const uint8_t EEPROM_OUTPUT_ENABLE_PIN = OE;
// const uint8_t EEPROM_WRITE_ENABLE_PIN = WE;
// const uint8_t EEPROM_READY_BUSY_OUTPUT_PIN = BSY;

// ========================================
// MEGA
// ========================================

// 29 -- !BSY -- |    | -- VCC  -- VCC
// 31 --  A12 -- |    | -- !WE  -- 22
// 33 --   A7 -- |    | -- NC   -- 24
// 35 --   A6 -- |    | -- A8   -- 26
// 37 --   A5 -- |    | -- A9   -- 28
// 39 --   A4 -- |    | -- A11  -- 30
// 41 --   A3 -- |    | -- !OE  -- 32
// 43 --   A2 -- |    | -- A10  -- 34
// 45 --   A1 -- |    | -- !CE  -- 36
// 47 --   A0 -- |    | -- IO7  -- 38
// 49 --  IO0 -- |    | -- IO6  -- 40
// 51 --  IO1 -- |    | -- IO5  -- 42
// 53 --  IO2 -- |    | -- IO4  -- 44
// GND -- GND -- |    | -- IO3  -- 46

const uint8_t EEPROM_ADDRESS_PINS[] = { 47, 45, 43, 41, 39, 37, 35, 33, 26, 28, 34, 30, 31 };
const uint8_t EEPROM_DATA_PINS[] = { 49, 51, 53, 46, 44, 42, 40, 38 };
const uint8_t EEPROM_CHIP_ENABLE_PIN = 36;
const uint8_t EEPROM_OUTPUT_ENABLE_PIN = 32;
const uint8_t EEPROM_WRITE_ENABLE_PIN = 22;
const uint8_t EEPROM_READY_BUSY_OUTPUT_PIN = 29;

// // ========================================
// // MEGA
// // ========================================

// //  66 -- !BSY -- |    | -- VCC  -- VCC
// //  65 --  A12 -- |    | -- !WE  -- 22
// //  64 --   A7 -- |    | -- NC   -- 24
// //  63 --   A6 -- |    | -- A8   -- 26
// //  62 --   A5 -- |    | -- A9   -- 28
// //  61 --   A4 -- |    | -- A11  -- 30
// //  60 --   A3 -- |    | -- !OE  -- 32
// //  59 --   A2 -- |    | -- A10  -- 34
// //  58 --   A1 -- |    | -- !CE  -- 36
// //  57 --   A0 -- |    | -- IO7  -- 38
// //  56 --  IO0 -- |    | -- IO6  -- 40
// //  55 --  IO1 -- |    | -- IO5  -- 42
// //  54 --  IO2 -- |    | -- IO4  -- 44
// // GND --  GND -- |    | -- IO3  -- 46

// const uint8_t EEPROM_ADDRESS_PINS[] = { 57, 58, 59, 60, 61, 62, 63, 64, 26, 28, 34, 30, 65 };
// const uint8_t EEPROM_DATA_PINS[] = { 56, 55, 54, 46, 44, 42, 40, 38 };
// const uint8_t EEPROM_CHIP_ENABLE_PIN = 36;
// const uint8_t EEPROM_OUTPUT_ENABLE_PIN = 32;
// const uint8_t EEPROM_WRITE_ENABLE_PIN = 22;
// const uint8_t EEPROM_READY_BUSY_OUTPUT_PIN = 66;

#endif  // !__eeprom_wiring_h__