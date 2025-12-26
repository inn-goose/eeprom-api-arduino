#ifndef __chip_wiring_h__
#define __chip_wiring_h__

#include "board_wiring.h"

using namespace BoardWiring;

namespace ChipWiring {

enum ChipType : int {
  // DIP28
  AT28C64 = 100,
  AT28C256 = 101,
  // DIP24
  AT28C04 = 200,
  AT28C16 = 201,
  // unknown
  UNKNOWN = 10000
};

ChipType chip_name_to_type(const String& chip_name) {
  String _chip_name = chip_name;
  _chip_name.toUpperCase();
  if (_chip_name == "AT28C64") {
    return ChipType::AT28C64;
  } else if (_chip_name == "AT28C256") {
    return ChipType::AT28C256;
  } else if (_chip_name == "AT28C04") {
    return ChipType::AT28C04;
  } else if (_chip_name == "AT28C16") {
    return ChipType::AT28C16;
  }
  return ChipType::UNKNOWN;
}


// ========================================
// Chip WIRING
// ========================================

// AT28C64 / DIP28

// 1  -- | !BSY  VCC |-- 28
// 2  -- | A12   !WE |-- 27
// 3  -- | A7     NC |-- 26
// 4  -- | A6     A8 |-- 25
// 5  -- | A5     A9 |-- 24
// 6  -- | A4    A11 |-- 23
// 7  -- | A3    !OE |-- 22
// 8  -- | A2    A10 |-- 21
// 9  -- | A1    !CE |-- 20
// 10 -- | A0    IO7 |-- 19
// 11 -- | IO0   IO6 |-- 18
// 12 -- | IO1   IO5 |-- 17
// 13 -- | IO2   IO4 |-- 16
// 14 -- | GND   IO3 |-- 15

namespace AT28C64_Wiring {
static const BoardWiringType BOARD_WIRING_TYPE = BoardWiringType::DIP28;
static const size_t ADDRESS_BUS_SIZE = 13;  // A0-A12
static const PIN_NO ADDRESS_BUS_PINS[ADDRESS_BUS_SIZE] = { 10, 9, 8, 7, 6, 5, 4, 3, 25, 24, 21, 23, 2 };
static const size_t DATA_BUS_SIZE = 8;
static const PIN_NO DATA_BUS_PINS[DATA_BUS_SIZE] = { 11, 12, 13, 15, 16, 17, 18, 19 };
static const size_t MANAGEMENT_SIZE = 4;
static const PIN_NO MANAGEMENT_PINS[MANAGEMENT_SIZE] = { 20, 22, 27, 1 };  // !CE, !OE, !WE, !BSY
}


// AT28C256 / DIP28

// 1  -- | A14   VCC |-- 28
// 2  -- | A12   !WE |-- 27
// 3  -- | A7    A13 |-- 26
// 4  -- | A6     A8 |-- 25
// 5  -- | A5     A9 |-- 24
// 6  -- | A4    A11 |-- 23
// 7  -- | A3    !OE |-- 22
// 8  -- | A2    A10 |-- 21
// 9  -- | A1    !CE |-- 20
// 10 -- | A0    IO7 |-- 19
// 11 -- | IO0   IO6 |-- 18
// 12 -- | IO1   IO5 |-- 17
// 13 -- | IO2   IO4 |-- 16
// 14 -- | GND   IO3 |-- 15

namespace AT28C256_Wiring {
static const BoardWiringType BOARD_WIRING_TYPE = BoardWiringType::DIP28;
static const size_t ADDRESS_BUS_SIZE = 15;  // A0-A14
static const PIN_NO ADDRESS_BUS_PINS[ADDRESS_BUS_SIZE] = { 10, 9, 8, 7, 6, 5, 4, 3, 25, 24, 21, 23, 2, 26, 1 };
static const size_t DATA_BUS_SIZE = 8;
static const PIN_NO DATA_BUS_PINS[DATA_BUS_SIZE] = { 11, 12, 13, 15, 16, 17, 18, 19 };
static const size_t MANAGEMENT_SIZE = 3;
static const PIN_NO MANAGEMENT_PINS[MANAGEMENT_SIZE] = { 20, 22, 27 };  // !CE, !OE, !WE
};


// AT28C04 / DIP24

// 1  -- | A7    VCC |-- 24
// 2  -- | A6     A8 |-- 23
// 3  -- | A5     NC |-- 22
// 4  -- | A4    !WE |-- 21
// 5  -- | A3    !OE |-- 20
// 6  -- | A2     NC |-- 19
// 7  -- | A1    !CE |-- 18
// 8  -- | A0    IO7 |-- 17
// 9  -- | IO0   IO6 |-- 16
// 10 -- | IO1   IO5 |-- 15
// 11 -- | IO2   IO4 |-- 14
// 12 -- | GND   IO3 |-- 13

namespace AT28C04_Wiring {
static const BoardWiringType BOARD_WIRING_TYPE = BoardWiringType::DIP24;
static const size_t ADDRESS_BUS_SIZE = 9;  // A0-A8
static const PIN_NO ADDRESS_BUS_PINS[ADDRESS_BUS_SIZE] = { 8, 7, 6, 5, 4, 3, 2, 1, 23 };
static const size_t DATA_BUS_SIZE = 8;
static const PIN_NO DATA_BUS_PINS[DATA_BUS_SIZE] = { 9, 10, 11, 13, 14, 15, 16, 17 };
static const size_t MANAGEMENT_SIZE = 3;
static const PIN_NO MANAGEMENT_PINS[MANAGEMENT_SIZE] = { 18, 20, 21 };  // !CE, !OE, !WE
};


// AT28C16 / DIP24

// 1  -- | A7    VCC |-- 24
// 2  -- | A6     A8 |-- 23
// 3  -- | A5     A9 |-- 22
// 4  -- | A4    !WE |-- 21
// 5  -- | A3    !OE |-- 20
// 6  -- | A2    A10 |-- 19
// 7  -- | A1    !CE |-- 18
// 8  -- | A0    IO7 |-- 17
// 9  -- | IO0   IO6 |-- 16
// 10 -- | IO1   IO5 |-- 15
// 11 -- | IO2   IO4 |-- 14
// 12 -- | GND   IO3 |-- 13

namespace AT28C16_Wiring {
static const BoardWiringType BOARD_WIRING_TYPE = BoardWiringType::DIP24;
static const size_t ADDRESS_BUS_SIZE = 11;  // A0-A10
static const PIN_NO ADDRESS_BUS_PINS[ADDRESS_BUS_SIZE] = { 8, 7, 6, 5, 4, 3, 2, 1, 23, 22, 19 };
static const size_t DATA_BUS_SIZE = 8;
static const PIN_NO DATA_BUS_PINS[DATA_BUS_SIZE] = { 9, 10, 11, 13, 14, 15, 16, 17 };
static const size_t MANAGEMENT_SIZE = 3;
static const PIN_NO MANAGEMENT_PINS[MANAGEMENT_SIZE] = { 18, 20, 21 };  // !CE, !OE, !WE
};


class ChipWiringController {
public:
  static const size_t MAX_BOARD_BUS_SIZE = 28;    // DIP28
  static const size_t MAX_ADDRESS_BUS_SIZE = 15;  // AT28C256: A0 to A14
  static const size_t MAX_DATA_BUS_SIZE = 8;      // AT28C256: I/O0 to I/O7
  static const size_t MAX_MANAGEMENT_SIZE = 4;    // CE, OE, WE, BSY

  ChipWiringController(const BoardWiringType board_wiring_type)
    : _board_wiring_type(board_wiring_type) {}

  inline BoardWiringType get_board_wiring_type() {
    return _board_wiring_type;
  }

  inline void set_chip_type(const ChipType chip_type) {
    if (_get_chip_board_wiring_type(chip_type) != _board_wiring_type) {
      _chip_type = ChipType::UNKNOWN;
      return;
    }
    _chip_type = chip_type;
  }

  inline ChipType get_chip_type() {
    return _chip_type;
  }

  inline BoardWiringType _get_chip_board_wiring_type(const ChipType chip_type) {
    switch (chip_type) {
      case ChipType::AT28C64:
        return AT28C64_Wiring::BOARD_WIRING_TYPE;
      case ChipType::AT28C256:
        return AT28C256_Wiring::BOARD_WIRING_TYPE;
      case ChipType::AT28C04:
        return AT28C04_Wiring::BOARD_WIRING_TYPE;
      case ChipType::AT28C16:
        return AT28C16_Wiring::BOARD_WIRING_TYPE;
      default:
        return BoardWiringType::UNKNOWN;
    }
  }

  size_t get_board_bus_pins(PIN_NO* pins_array, const size_t array_size) {
    size_t board_bus_size = 0;
    const PIN_NO* board_bus_pins = 0;

    switch (_board_wiring_type) {
      case BoardWiringType::DIP28:
        board_bus_size = 28;
        board_bus_pins = DIP28_WIRING;
        break;
      case BoardWiringType::DIP24:
        board_bus_size = 24;
        board_bus_pins = DIP24_WIRING;
        break;
      default:
        break;
    }

    if (board_bus_size == 0 || board_bus_pins == 0) {
      return 0;
    }
    if (array_size < board_bus_size) {
      return 0;
    }

    // memcpu
    for (size_t i = 0; i < board_bus_size; i++) {
      pins_array[i] = board_bus_pins[i];
    }
    return board_bus_size;
  }

  size_t get_address_bus_pins(PIN_NO* pins_array, const size_t array_size) {
    const PIN_NO* dip_wiring_mapping = 0;
    size_t address_bus_size = 0;
    const PIN_NO* address_bus_pins = 0;

    switch (_board_wiring_type) {
      case BoardWiringType::DIP28:
        dip_wiring_mapping = DIP28_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C64:
            address_bus_size = AT28C64_Wiring::ADDRESS_BUS_SIZE;
            address_bus_pins = AT28C64_Wiring::ADDRESS_BUS_PINS;
            break;
          case ChipType::AT28C256:
            address_bus_size = AT28C256_Wiring::ADDRESS_BUS_SIZE;
            address_bus_pins = AT28C256_Wiring::ADDRESS_BUS_PINS;
            break;
          default:
            break;
        }
        break;
      case BoardWiringType::DIP24:
        dip_wiring_mapping = DIP24_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C04:
            address_bus_size = AT28C04_Wiring::ADDRESS_BUS_SIZE;
            address_bus_pins = AT28C04_Wiring::ADDRESS_BUS_PINS;
            break;
          case ChipType::AT28C16:
            address_bus_size = AT28C16_Wiring::ADDRESS_BUS_SIZE;
            address_bus_pins = AT28C16_Wiring::ADDRESS_BUS_PINS;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    if (dip_wiring_mapping == 0) {
      return 0;
    }
    if (address_bus_size == 0 || address_bus_pins == 0) {
      return 0;
    }
    if (array_size < address_bus_size) {
      return 0;
    }

    // reset
    for (size_t i = 0; i < MAX_ADDRESS_BUS_SIZE; i++) {
      pins_array[i] = 0;  // NC
    }

    // mapping
    for (size_t i = 0; i < address_bus_size; i++) {
      // mapping starts from 0, but PIN numbers start from 1 for convenience
      pins_array[i] = dip_wiring_mapping[address_bus_pins[i] - 1];
    }
    return address_bus_size;
  }

  size_t get_data_bus_pins(PIN_NO* pins_array, const size_t array_size) {
    const PIN_NO* dip_wiring_mapping = 0;
    size_t data_bus_size = 0;
    const PIN_NO* data_bus_pins = 0;

    switch (_board_wiring_type) {
      case BoardWiringType::DIP28:
        dip_wiring_mapping = DIP28_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C64:
            data_bus_size = AT28C64_Wiring::DATA_BUS_SIZE;
            data_bus_pins = AT28C64_Wiring::DATA_BUS_PINS;
            break;
          case ChipType::AT28C256:
            data_bus_size = AT28C256_Wiring::DATA_BUS_SIZE;
            data_bus_pins = AT28C256_Wiring::DATA_BUS_PINS;
            break;
          default:
            break;
        }
        break;
      case BoardWiringType::DIP24:
        dip_wiring_mapping = DIP24_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C04:
            data_bus_size = AT28C04_Wiring::DATA_BUS_SIZE;
            data_bus_pins = AT28C04_Wiring::DATA_BUS_PINS;
            break;
          case ChipType::AT28C16:
            data_bus_size = AT28C16_Wiring::DATA_BUS_SIZE;
            data_bus_pins = AT28C16_Wiring::DATA_BUS_PINS;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    if (dip_wiring_mapping == 0) {
      return 0;
    }
    if (data_bus_size == 0 || data_bus_pins == 0) {
      return 0;
    }
    if (array_size < data_bus_size) {
      return 0;
    }

    // reset
    for (size_t i = 0; i < MAX_DATA_BUS_SIZE; i++) {
      pins_array[i] = 0;  // NC
    }

    // mapping
    for (size_t i = 0; i < data_bus_size; i++) {
      // mapping starts from 0, but PIN numbers start from 1 for convenience
      pins_array[i] = dip_wiring_mapping[data_bus_pins[i] - 1];
    }
    return data_bus_size;
  }

  size_t get_management_pins(PIN_NO* pins_array, const size_t array_size) {
    const PIN_NO* dip_wiring_mapping = 0;
    size_t management_size = 0;
    const PIN_NO* management_pins = 0;

    switch (_board_wiring_type) {
      case BoardWiringType::DIP28:
        dip_wiring_mapping = DIP28_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C64:
            management_size = AT28C64_Wiring::MANAGEMENT_SIZE;
            management_pins = AT28C64_Wiring::MANAGEMENT_PINS;
            break;
          case ChipType::AT28C256:
            management_size = AT28C256_Wiring::MANAGEMENT_SIZE;
            management_pins = AT28C256_Wiring::MANAGEMENT_PINS;
            break;
          default:
            break;
        }
        break;
      case BoardWiringType::DIP24:
        dip_wiring_mapping = DIP24_WIRING;
        switch (_chip_type) {
          case ChipType::AT28C04:
            management_size = AT28C04_Wiring::MANAGEMENT_SIZE;
            management_pins = AT28C04_Wiring::MANAGEMENT_PINS;
            break;
          case ChipType::AT28C16:
            management_size = AT28C16_Wiring::MANAGEMENT_SIZE;
            management_pins = AT28C16_Wiring::MANAGEMENT_PINS;
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }

    if (dip_wiring_mapping == 0) {
      return 0;
    }
    if (management_size == 0 || management_pins == 0) {
      return 0;
    }
    if (array_size < management_size) {
      return 0;
    }

    // reset
    for (size_t i = 0; i < MAX_MANAGEMENT_SIZE; i++) {
      pins_array[i] = 0;  // NC
    }

    // mapping
    for (size_t i = 0; i < management_size; i++) {
      // mapping starts from 0, but PIN numbers start from 1 for convenience
      pins_array[i] = dip_wiring_mapping[management_pins[i] - 1];
    }
    return management_size;
  }

private:
  BoardWiringType _board_wiring_type;
  ChipType _chip_type;
};

}  // ChipWiring

#endif  // !__chip_wiring_h__