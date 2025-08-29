#ifndef __eeprom_wiring_h__
#define __eeprom_wiring_h__

// REDEFINE FOR YOUR WIRING SCHEME
const uint8_t EEPROM_ADDRESS_PINS[] = { 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66 };
const uint8_t EEPROM_DATA_PINS[] = { 22, 23, 24, 25, 26, 27, 28, 29 };
const uint8_t EEPROM_CHIP_ENABLE_PIN = 10;
const uint8_t EEPROM_OUTPUT_ENABLE_PIN = 11;
const uint8_t EEPROM_WRITE_ENABLE_PIN = 12;
const uint8_t EEPROM_READY_BUSY_OUTPUT_PIN = 13;

#endif  // !__eeprom_wiring_h__