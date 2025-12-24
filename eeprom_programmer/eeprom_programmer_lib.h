// TODO: https://docs.arduino.cc/learn/contributions/arduino-creating-library-guide/

#ifndef __eeprom_programmer_lib_h__
#define __eeprom_programmer_lib_h__

#include <limits.h>

#include "chip_wiring.h"

using namespace ChipWiring;

namespace EepromProgrammerLibrary {

// Error Codes

enum ErrorCode : short {
  SUCCESS = 0,
  // connection and pins
  INVALID_BOARD_WIRING_TYPE = 11,
  PINS_NOT_INITIALIZED = 12,
  // chip
  CHIP_NOT_SUPPORTED = 21,
  CHIP_ALREADY_INITIALIZED = 22,
  CHIP_NOT_INITIALIZED = 23,
  // address
  INVALID_PAGE_SIZE = 31,
  INVALID_PAGE_NO = 32,
  INVALID_ADDRESS = 33,
  // read
  READ_MODE_DISABLED = 41,
  READ_FAILED = 42,
  // write
  WRITE_MODE_DISABLED = 51,
  WRITE_FAILED = 52,
  // unknown
  UNKNOWN_ERROR = SHRT_MAX
};


// EEPROM Programmer

class EepromProgrammer {
public:
  EepromProgrammer(const BoardWiringType board_wiring_type);

  // init
  ErrorCode init_programmer();
  ErrorCode init_chip(const String& chip_type);

  // settings
  inline uint32_t get_memory_size_bytes() {
    return _memory_size_bytes;
  }
  inline uint32_t get_page_size_bytes() {
    return _page_size_bytes;
  }
  inline uint32_t get_max_page_size() {
    return _MAX_PAGE_SIZE;
  }

  // read
  ErrorCode set_read_mode(const uint32_t page_size_bytes);
  ErrorCode read_page(const int page_no, uint8_t* bytes);

  // write
  ErrorCode set_write_mode(const uint32_t page_size_bytes);
  ErrorCode write_page(const int page_no, const uint8_t* bytes, const size_t bytes_size);

  // debugging
  void get_read_byte_usec_for_page(unsigned int* read_byte_usec_for_page, const size_t buffer_size) {
    if (buffer_size <= 0 || buffer_size > _MAX_PAGE_SIZE) {
      return;
    }
    for (int i; i < buffer_size; i++) {
      read_byte_usec_for_page[i] = _read_byte_usec_for_page[i];
    }
  }

  void get_write_byte_usec_for_page(unsigned int* write_byte_usec_for_page, const size_t buffer_size) {
    if (buffer_size <= 0 || buffer_size > _MAX_PAGE_SIZE) {
      return;
    }
    for (int i; i < buffer_size; i++) {
      write_byte_usec_for_page[i] = _write_byte_usec_for_page[i];
    }
  }

  // helpers
  static String address_to_binary_string(const uint32_t address, const size_t address_bus_size) {
    bool b_address[address_bus_size];
    _address_to_bits_array(address, b_address, address_bus_size);
    String result = "";
    for (int i = 0; i < address_bus_size; i++) {
      // print in reverse order, since the printed A0 should be the last bit
      result += b_address[address_bus_size - 1 - i] ? 1 : 0;
    }
    return result;
  }

  static String address_to_hex_string(const uint32_t address) {
    char buf[8];
    sprintf(buf, "%08x", address);
    return String(buf);
  }

  static String data_to_hex_string(const uint8_t data) {
    char buf[2];
    sprintf(buf, "%02x", data);
    return String(buf);
  }

private:
  static const uint32_t _MAX_PAGE_SIZE = 64;

  ErrorCode _read_byte(const uint32_t address, uint8_t& byte);
  ErrorCode _write_byte(const uint32_t address, const uint8_t data);

  enum _DataBusMode {
    READ,
    WRITE,
  };
  void _set_address_bus_mode();
  void _set_data_bus_mode(const _DataBusMode mode);
  void _write_address(const uint32_t address);
  uint8_t _read_data();
  void _write_data(const uint8_t data);

  void _polling(const uint8_t data);
  void _rdy_busy_polling();
  void _data_polling(const uint8_t data);

  // wiring controller
  ChipWiringController _chip_wiring_controller;

  // PINS
  // address bus
  PIN_NO _address_bus_pins[ChipWiringController::MAX_ADDRESS_BUS_SIZE];
  size_t _address_bus_size;
  // data bus
  PIN_NO _data_bus_pins[ChipWiringController::MAX_DATA_BUS_SIZE];
  size_t _data_bus_size;
  // management
  PIN_NO _chip_enable_pin;    // !CE
  PIN_NO _output_enable_pin;  // !OE
  PIN_NO _write_enable_pin;   // !WE
  PIN_NO _rdy_busy_pin;       // RDY / !BUSY

  // inner
  bool _pins_initialized;
  bool _chip_ready;

  // chip settings
  unsigned int _write_polling_time_usec;
  bool _can_write_pages;

  // optimizations
  bool _current_address[ChipWiringController::MAX_ADDRESS_BUS_SIZE];
  bool _current_data[ChipWiringController::MAX_ADDRESS_BUS_SIZE];

  // modes
  uint32_t _memory_size_bytes;
  uint32_t _page_size_bytes;
  bool _read_mode;
  bool _write_mode;

  // debugging
  unsigned int _read_byte_usec_for_page[_MAX_PAGE_SIZE];
  unsigned int _write_byte_usec_for_page[_MAX_PAGE_SIZE];

  // bit operations
  // Most Significant Bit First ordering
  // { 0,0,0,0,0,0,0,1 } == 1
  // { 1,0,0,0,0,0,0,0 } == 128
  // Less Significant Bit First ordering
  // { 1,0,0,0,0,0,0,0 } == 1
  // { 0,0,0,0,0,0,0,1 } == 128

  static void _address_to_bits_array(uint32_t address, bool* b_address, const size_t address_bus_size) {
    // ensure address is within the memory size range
    const uint32_t memory_size_bytes = pow(2, address_bus_size);
    if (address >= memory_size_bytes) {
      return;
    }
    for (int i = 0; i < address_bus_size; ++i) {
      // MSB order
      // b_address[address_bus_size - 1 - i] = (address >> i) & 1;
      // LSB order
      b_address[i] = (address >> i) & 1;
    }
  }

  static void _data_to_bits_array(uint8_t data, bool* b_data, const size_t data_bus_size) {
    // MSB order
    for (int i = 0; i < data_bus_size; i++) {
      // MSP order
      // b_data[data_bus_size - 1 - i] = bitRead(data, i);
      // LSB order
      b_data[i] = (data >> i) & 1;
    }
  }

  static uint8_t _bits_array_to_data(const bool* b_data, const size_t data_bus_size) {
    // MSB order
    uint8_t data = 0;
    for (int i = 0; i < data_bus_size; i++) {
      data = (data << 1) | b_data[data_bus_size - 1 - i];
    }
    return data;
  }
};

EepromProgrammer::EepromProgrammer(const BoardWiringType board_wiring_type)
  : _chip_wiring_controller(board_wiring_type) {

  // inner
  _pins_initialized = false;
  _chip_ready = false;

  // pins
  _address_bus_size = 0;
  _data_bus_size = 0;

  // chip settings
  _write_polling_time_usec = 0;
  _can_write_pages = false;

  // mode
  _memory_size_bytes = 0;
  _page_size_bytes = 0;
  _read_mode = false;
  _write_mode = false;

  // performance
  for (int i = 0; i < _MAX_PAGE_SIZE; i++) {
    _read_byte_usec_for_page[i] = 0;
    _write_byte_usec_for_page[i] = 0;
  }
}

ErrorCode EepromProgrammer::init_programmer() {
  PIN_NO board_bus_pins[ChipWiringController::MAX_BOARD_BUS_SIZE];
  const size_t board_bus_size = _chip_wiring_controller.get_board_bus_pins(board_bus_pins, ChipWiringController::MAX_BOARD_BUS_SIZE);
  if (board_bus_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }

  // set all pins as NC
  for (size_t i = 0; i < board_bus_size; i++) {
    const PIN_NO pin_no = board_bus_pins[i];
    if (pin_no == 0) {  // VCC or GND
      continue;
    }
    pinMode(pin_no, INPUT_PULLUP);
    // pinMode(pin_no, OUTPUT);
    // digitalWrite(pin_no, LOW);
  }

  _pins_initialized = true;

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::init_chip(const String& chip_name) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (_chip_ready) {
    return ErrorCode::CHIP_ALREADY_INITIALIZED;
  }

  _chip_wiring_controller.set_chip_type(chip_name_to_type(chip_name));
  ChipType chip_type = _chip_wiring_controller.get_chip_type();
  if (chip_type == ChipType::UNKNOWN) {
    return ErrorCode::CHIP_NOT_SUPPORTED;
  }

  // address bus
  _address_bus_size = _chip_wiring_controller.get_address_bus_pins(_address_bus_pins, ChipWiringController::MAX_ADDRESS_BUS_SIZE);
  if (_address_bus_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  _memory_size_bytes = (uint32_t)(1) << _address_bus_size;

  _set_address_bus_mode();

  // data bus
  _data_bus_size = _chip_wiring_controller.get_data_bus_pins(_data_bus_pins, ChipWiringController::MAX_DATA_BUS_SIZE);
  if (_data_bus_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }

  _set_data_bus_mode(_DataBusMode::READ);

  // management pins
  // !CE, !OE, !WE, [!BSY]
  PIN_NO management_pins[ChipWiringController::MAX_MANAGEMENT_SIZE];
  const size_t management_size = _chip_wiring_controller.get_management_pins(management_pins, ChipWiringController::MAX_MANAGEMENT_SIZE);
  if (management_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }

  // !CE
  _chip_enable_pin = management_pins[0];
  pinMode(_chip_enable_pin, OUTPUT);
  digitalWrite(_chip_enable_pin, LOW);  // always ON
  // !OE
  _output_enable_pin = management_pins[1];
  pinMode(_output_enable_pin, OUTPUT);
  digitalWrite(_output_enable_pin, HIGH);
  // !WE
  _write_enable_pin = management_pins[2];
  pinMode(_write_enable_pin, OUTPUT);
  digitalWrite(_write_enable_pin, HIGH);
  // RDY/!BUSY
  _rdy_busy_pin = management_pins[3];
  if (_rdy_busy_pin > 0) {
    // open drain
    pinMode(_rdy_busy_pin, INPUT_PULLUP);
  }

  // chip settings
  // tune this constant if write is not working
  // if the waiting is insufficient, data propagation may be incomplete
  // AT28C04 write time is about 950 us
  // AT28C16 write time is about 3800 us
  // AT28C64 write time is about 4100 us
  // AT28C256 write time is about 6500 us
  _write_polling_time_usec = 20000;
  switch (chip_type) {
    case ChipType::AT28C04:
      break;
    case ChipType::AT28C16:
      break;
    case ChipType::AT28C64:
      break;
    case ChipType::AT28C256:
      _can_write_pages = true;
      break;
    default:
      break;
  }

  _chip_ready = true;

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::set_read_mode(const uint32_t page_size_bytes) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (page_size_bytes < 1 || page_size_bytes > _MAX_PAGE_SIZE) {
    return ErrorCode::INVALID_PAGE_SIZE;
  }
  _read_mode = true;
  _page_size_bytes = page_size_bytes;

  // initial READ waveforms state
  digitalWrite(_output_enable_pin, HIGH);  // off
  digitalWrite(_write_enable_pin, HIGH);   // not in use
  // switch data pins to READ mode
  _set_data_bus_mode(_DataBusMode::READ);

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::read_page(const int page_no, uint8_t* bytes) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (!_read_mode) {
    return ErrorCode::READ_MODE_DISABLED;
  }
  const uint32_t max_page_no = _memory_size_bytes / _page_size_bytes;
  if (page_no < 0 || page_no >= max_page_no) {
    return ErrorCode::INVALID_PAGE_NO;
  }

  const uint32_t start_address = page_no * _page_size_bytes;
  for (int i = 0; i < _page_size_bytes; i++) {
    const unsigned long read_byte_start_usec = micros();
    uint8_t byte = -1;
    ErrorCode code = _read_byte(start_address + i, byte);
    if (code != ErrorCode::SUCCESS) {
      return ErrorCode::READ_FAILED;
    }
    bytes[i] = byte;
    _read_byte_usec_for_page[i] = (unsigned int)(micros() - read_byte_start_usec);
  }

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::set_write_mode(const uint32_t page_size_bytes) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (page_size_bytes < 1 || page_size_bytes > _MAX_PAGE_SIZE) {
    return ErrorCode::INVALID_PAGE_SIZE;
  }
  _write_mode = true;
  _page_size_bytes = page_size_bytes;

  // initial WRITE waveforms state (!WE controlled)
  digitalWrite(_output_enable_pin, HIGH);  // not in use
  digitalWrite(_write_enable_pin, HIGH);   // off

  // switch data pins to WRITE mode
  _set_data_bus_mode(_DataBusMode::WRITE);

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::write_page(const int page_no, const uint8_t* bytes, const size_t bytes_size) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (!_write_mode) {
    return ErrorCode::WRITE_MODE_DISABLED;
  }
  if (bytes_size <= 0 || bytes_size > _page_size_bytes) {
    return ErrorCode::INVALID_PAGE_SIZE;
  }
  const uint32_t max_page_no = _memory_size_bytes / _page_size_bytes;
  if (page_no < 0 || page_no >= max_page_no) {
    return ErrorCode::INVALID_PAGE_NO;
  }

  const uint32_t start_address = page_no * _page_size_bytes;
  for (int i = 0; i < bytes_size; i++) {
    const unsigned long write_byte_start_usec = micros();
    ErrorCode code = _write_byte(start_address + i, bytes[i]);
    if (code != ErrorCode::SUCCESS) {
      return ErrorCode::WRITE_FAILED;
    }
    // poll only last byte for the page write mode
    if (!_can_write_pages || (_can_write_pages && i == bytes_size - 1)) {
      _polling(bytes[i]);
    }
    _write_byte_usec_for_page[i] = (unsigned int)(micros() - write_byte_start_usec);
  }

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::_read_byte(const uint32_t address, uint8_t& byte) {
  if (address < 0 || address >= _memory_size_bytes) {
    return ErrorCode::INVALID_ADDRESS;
  }

  // (1) set address
  _write_address(address);

  // (2) output enable
  digitalWrite(_output_enable_pin, LOW);

  // (3) !OE to Output Delay (delta between OE and data ready) == 100 ns MAX
  delayMicroseconds(1);  // arduino cannot delay in ns, only us

  // (4) read data
  byte = _read_data();

  // (5) output disable
  digitalWrite(_output_enable_pin, HIGH);

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::_write_byte(const uint32_t address, const uint8_t data) {
  if (address < 0 || address >= _memory_size_bytes) {
    return ErrorCode::INVALID_ADDRESS;
  }

  // (1) set address
  _write_address(address);

  // (2) set data
  _write_data(data);

  // (3) wrtie enable
  digitalWrite(_write_enable_pin, LOW);

  // (4) wrtie disable (initiates the data flush)
  digitalWrite(_write_enable_pin, HIGH);

  return ErrorCode::SUCCESS;
}

void EepromProgrammer::_set_address_bus_mode() {
  for (int i = 0; i < _address_bus_size; i++) {
    pinMode(_address_bus_pins[i], OUTPUT);
    digitalWrite(_address_bus_pins[i], 0);
    _current_address[i] = 0;
  }
}

void EepromProgrammer::_set_data_bus_mode(const EepromProgrammer::_DataBusMode mode) {
  if (mode == EepromProgrammer::_DataBusMode::READ) {
    for (int i = 0; i < _data_bus_size; i++) {
      pinMode(_data_bus_pins[i], INPUT_PULLUP);
    }

  } else if (mode == EepromProgrammer::_DataBusMode::WRITE) {
    for (int i = 0; i < _data_bus_size; i++) {
      pinMode(_data_bus_pins[i], OUTPUT);
      digitalWrite(_data_bus_pins[i], 0);
      _current_data[i] = 0;
    }
  }
}

void EepromProgrammer::_write_address(const uint32_t address) {
  const size_t c_address_bus_size = _address_bus_size;
  bool b_address[c_address_bus_size];
  _address_to_bits_array(address, b_address, c_address_bus_size);
  for (int i = 0; i < c_address_bus_size; i++) {
    if (_current_address[i] != b_address[i]) {
      digitalWrite(_address_bus_pins[i], b_address[i]);
      _current_address[i] = b_address[i];
    }
  }
}

uint8_t EepromProgrammer::_read_data() {
  const size_t c_data_bus_size = _data_bus_size;
  bool b_data[c_data_bus_size];
  for (int i = 0; i < c_data_bus_size; i++) {
    b_data[i] = digitalRead(_data_bus_pins[i]) == HIGH ? 1 : 0;
  }
  return _bits_array_to_data(b_data, c_data_bus_size);
}

void EepromProgrammer::_write_data(const uint8_t data) {
  const size_t c_data_bus_size = _data_bus_size;
  bool b_data[c_data_bus_size];
  _data_to_bits_array(data, b_data, c_data_bus_size);
  for (int i = 0; i < c_data_bus_size; i++) {
    if (_current_data[i] != b_data[i]) {
      digitalWrite(_data_bus_pins[i], b_data[i]);
      _current_data[i] = b_data[i];
    }
  }
}

void EepromProgrammer::_polling(const uint8_t data) {
  if (_rdy_busy_pin > 0) {
    _rdy_busy_polling();
  } else {
    _data_polling(data);
  }
}

void EepromProgrammer::_rdy_busy_polling() {
  const unsigned long polling_start_usec = micros();

  // wait until device switches to !BUSY state, if chip has the RDY/!BUSY pin
  // Time to Device Busy (delta between WE and !BUSY) == 50 ms MAX (spec)
  delayMicroseconds(1);  // arduino cannot delay in ns, only us
  int currBusyState = digitalRead(_rdy_busy_pin);

  // wait until !BUSY state switches to READY state (1 ms MAX)
  // or just wait for the Write Cycle Time MAX
  if (currBusyState == LOW) {
    // device is in !BUSY state
    // use the READY/!BUSY pin status to wait for the Write Cycle End
    const unsigned int delay_usec = 100;

    int prevBusyState = currBusyState;
    while (polling_start_usec + _write_polling_time_usec > micros()) {
      delayMicroseconds(delay_usec);

      prevBusyState = currBusyState;
      currBusyState = digitalRead(_rdy_busy_pin);
      if (prevBusyState == LOW && currBusyState == HIGH) {  // rising edge
        break;
      }
    }
  } else {
    // device not in !BUSY state
    // use generic delay
    delayMicroseconds(_write_polling_time_usec);
  }
}

void EepromProgrammer::_data_polling(const uint8_t data) {
  const unsigned long polling_start_usec = micros();

  // use !DATA polling, if chip doesn't have the RDY/!BUSY pin
  // following the data poll waveforms, the data is read in a loop until the value matches the one written
  // during the write procedure, the data pins remain in a metastable state.
  _set_data_bus_mode(_DataBusMode::READ);

  const unsigned int delay_usec = 50;

  while (polling_start_usec + _write_polling_time_usec > micros()) {
    delayMicroseconds(delay_usec);

    // !DATA polling waveforms require to switch !CE and !OE for every attempt
    digitalWrite(_output_enable_pin, LOW);
    // !OE to Output Delay (delta between OE and data ready) == 100 ns MAX
    delayMicroseconds(1);  // arduino cannot delay in ns, only us
    uint8_t read_result = _read_data();
    digitalWrite(_output_enable_pin, HIGH);
    if (read_result == data) {
      break;
    }
  }

  _set_data_bus_mode(_DataBusMode::WRITE);
}


}  // EepromProgrammerLibrary

#endif  // !__eeprom_programmer_lib_h__