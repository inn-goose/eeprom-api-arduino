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
  ErrorCode read_byte(const uint32_t address, uint8_t& byte);

  // write
  ErrorCode set_write_mode(const uint32_t page_size_bytes);
  ErrorCode write_page(const int page_no, const uint8_t* bytes, const size_t bytes_size);
  ErrorCode write_byte(const uint32_t address, const uint8_t data);

  // debugging
  unsigned long get_write_op_wait_time_usec() {
    return _write_op_wait_time_usec;
  }

  void get_write_op_wait_time_usec_for_page(unsigned long* wait_time_for_page, const size_t buffer_size) {
    if (buffer_size <= 0 || buffer_size > _MAX_PAGE_SIZE) {
      return;
    }
    for (int i; i < buffer_size; i++) {
      wait_time_for_page[i] = _write_op_wait_time_usec_for_page[i];
    }
  }

  int get_write_op_wait_cycles() {
    return _write_op_wait_cycles;
  }

  // helpers
  static String address_to_binary_string(const uint32_t address, const size_t address_bus_size) {
    bool b_address[address_bus_size];
    _addressToBitsArray(address, b_address, address_bus_size);
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

  // tune this constant if write is not working
  // if the waiting is insufficient, data propagation may be incomplete
  // AT28C64 write time is about 400 us
  // AT28C256 write time is about 6000 us
  static const unsigned int _WRITE_SUCCESS_WAITING_TIME_USEC = int(20.0 * 1000);

  enum _DataBusMode {
    READ,
    WRITE,
  };
  void _setAddressBusMode();
  void _setDataBusMode(const _DataBusMode mode);
  void _writeAddress(const uint32_t address);
  uint8_t _readData();
  void _writeData(const uint8_t data);

  void _rdy_busy_polling(const unsigned long write_op_start_usec, const uint8_t data);
  void _data_polling(const unsigned long write_op_start_usec, const uint8_t data);

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

  // modes
  uint32_t _memory_size_bytes;
  uint32_t _page_size_bytes;
  bool _read_mode;
  bool _write_mode;

  // debugging
  unsigned long _write_op_wait_time_usec;
  unsigned long _write_op_wait_time_usec_for_page[_MAX_PAGE_SIZE];
  int _write_op_wait_cycles;

  // bit operations
  // Most Significant Bit First ordering
  // { 0,0,0,0,0,0,0,1 } == 1
  // { 1,0,0,0,0,0,0,0 } == 128

  static void _addressToBitsArray(uint32_t address, bool* b_address, const size_t address_bus_size) {
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

  static void _dataToBitsArray(uint8_t data, bool* b_data, const size_t data_bus_size) {
    // MSB order
    for (int i = 0; i < data_bus_size; i++) {
      // MSP order
      // b_data[data_bus_size - 1 - i] = bitRead(data, i);
      // LSB order
      b_data[i] = (data >> i) & 1;
    }
  }

  static uint8_t _bitsArrayToData(const bool* b_data, const size_t data_bus_size) {
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

  // mode
  _memory_size_bytes = 0;
  _page_size_bytes = 0;
  _read_mode = false;
  _write_mode = false;

  // performance
  _write_op_wait_time_usec = 0;
  for (int i = 0; i < _MAX_PAGE_SIZE; i++) {
    _write_op_wait_time_usec_for_page[i] = 0;
  }
  _write_op_wait_cycles = -1;
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

ErrorCode EepromProgrammer::init_chip(const String& chip_type) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (_chip_ready) {
    return ErrorCode::CHIP_ALREADY_INITIALIZED;
  }

  _chip_wiring_controller.set_chip_type(str_to_chip_type(chip_type));
  if (_chip_wiring_controller.get_chip_type() == ChipType::UNKNOWN) {
    return ErrorCode::CHIP_NOT_SUPPORTED;
  }

  // address bus
  _address_bus_size = _chip_wiring_controller.get_address_bus_pins(_address_bus_pins, ChipWiringController::MAX_ADDRESS_BUS_SIZE);
  if (_address_bus_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  _memory_size_bytes = (uint32_t)(1) << _address_bus_size;

  _setAddressBusMode();
  // reset address
  _writeAddress(0);

  // data bus
  _data_bus_size = _chip_wiring_controller.get_data_bus_pins(_data_bus_pins, ChipWiringController::MAX_DATA_BUS_SIZE);
  if (_data_bus_size <= 0) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }

  _setDataBusMode(_DataBusMode::READ);

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
  digitalWrite(_chip_enable_pin, HIGH);
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
  digitalWrite(_chip_enable_pin, HIGH);    // off
  digitalWrite(_output_enable_pin, HIGH);  // off
  digitalWrite(_write_enable_pin, HIGH);   // not in use
  // switch data pins to READ mode
  _setDataBusMode(_DataBusMode::READ);

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
    uint8_t byte = -1;
    ErrorCode code = read_byte(start_address + i, byte);
    if (code != ErrorCode::SUCCESS) {
      return ErrorCode::READ_FAILED;
    }
    bytes[i] = byte;
  }

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::read_byte(const uint32_t address, uint8_t& byte) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (!_read_mode) {
    return ErrorCode::READ_MODE_DISABLED;
  }
  if (address < 0 || address >= _memory_size_bytes) {
    return ErrorCode::INVALID_ADDRESS;
  }

  // (1) set address
  _writeAddress(address);

  // (2) chip enable
  digitalWrite(_chip_enable_pin, LOW);

  // (3) output enable
  digitalWrite(_output_enable_pin, LOW);

  // (4) !OE to Output Delay (delta between OE and data ready) == 100 ns MAX
  delayMicroseconds(1);  // arduino cannot delay in ns, only us

  // (5) read data
  byte = _readData();

  // (6) output disable
  digitalWrite(_output_enable_pin, HIGH);

  // (7) chip disable
  digitalWrite(_chip_enable_pin, HIGH);

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
  digitalWrite(_chip_enable_pin, HIGH);    // off
  digitalWrite(_output_enable_pin, HIGH);  // not in use
  digitalWrite(_write_enable_pin, HIGH);   // off

  // switch data pins to WRITE mode
  _setDataBusMode(_DataBusMode::WRITE);

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
    ErrorCode code = write_byte(start_address + i, bytes[i]);
    if (code != ErrorCode::SUCCESS) {
      return ErrorCode::WRITE_FAILED;
    }
    _write_op_wait_time_usec_for_page[i] = _write_op_wait_time_usec;
  }

  return ErrorCode::SUCCESS;
}

ErrorCode EepromProgrammer::write_byte(const uint32_t address, const uint8_t data) {
  if (!_pins_initialized) {
    return ErrorCode::PINS_NOT_INITIALIZED;
  }
  if (!_chip_ready) {
    return ErrorCode::CHIP_NOT_INITIALIZED;
  }
  if (!_write_mode) {
    return ErrorCode::WRITE_MODE_DISABLED;
  }
  if (address < 0 || address >= _memory_size_bytes) {
    return ErrorCode::INVALID_ADDRESS;
  }

  // (1) set address
  _writeAddress(address);

  // (2) chip enable
  digitalWrite(_chip_enable_pin, LOW);

  // (3) wrtie enable
  digitalWrite(_write_enable_pin, LOW);

  // (4) write data
  _writeData(data);

  // (5) wrtie disable (initiates the data flush)
  digitalWrite(_write_enable_pin, HIGH);

  // (6) polling
  const unsigned long write_op_start_usec = micros();
  _write_op_wait_time_usec = 0;
  _write_op_wait_cycles = -1;
  if (_rdy_busy_pin > 0) {
    _rdy_busy_polling(write_op_start_usec, data);
  } else {
    _data_polling(write_op_start_usec, data);
  }
  _write_op_wait_time_usec = micros() - write_op_start_usec;

  // (7) chip disable
  digitalWrite(_chip_enable_pin, HIGH);

  return ErrorCode::SUCCESS;
}

void EepromProgrammer::_setAddressBusMode() {
  for (int i = 0; i < _address_bus_size; i++) {
    pinMode(_address_bus_pins[i], OUTPUT);
  }
}

void EepromProgrammer::_setDataBusMode(const EepromProgrammer::_DataBusMode mode) {
  if (mode == EepromProgrammer::_DataBusMode::READ) {
    for (int i = 0; i < _data_bus_size; i++) {
      pinMode(_data_bus_pins[i], INPUT_PULLUP);
    }

  } else if (mode == EepromProgrammer::_DataBusMode::WRITE) {
    for (int i = 0; i < _data_bus_size; i++) {
      pinMode(_data_bus_pins[i], OUTPUT);
    }
  }
}

void EepromProgrammer::_writeAddress(const uint32_t address) {
  const size_t c_address_bus_size = _address_bus_size;
  bool b_address[c_address_bus_size];
  _addressToBitsArray(address, b_address, c_address_bus_size);
  for (int i = 0; i < c_address_bus_size; i++) {
    digitalWrite(_address_bus_pins[i], b_address[i]);
  }
}

uint8_t EepromProgrammer::_readData() {
  const size_t c_data_bus_size = _data_bus_size;
  bool b_data[c_data_bus_size];
  for (int i = 0; i < c_data_bus_size; i++) {
    b_data[i] = digitalRead(_data_bus_pins[i]) == HIGH ? 1 : 0;
  }
  return _bitsArrayToData(b_data, c_data_bus_size);
}

void EepromProgrammer::_writeData(const uint8_t data) {
  const size_t c_data_bus_size = _data_bus_size;
  bool b_data[c_data_bus_size];
  _dataToBitsArray(data, b_data, c_data_bus_size);
  for (int i = 0; i < c_data_bus_size; i++) {
    digitalWrite(_data_bus_pins[i], b_data[i]);
  }
}

void EepromProgrammer::_rdy_busy_polling(const unsigned long write_op_start_usec, const uint8_t data) {
  // wait until device switches to !BUSY state, if chip has the RDY/!BUSY pin
  // Time to Device Busy (delta between WE and !BUSY) == 50 ms MAX (spec)
  delayMicroseconds(1);  // arduino cannot delay in ns, only us
  int currBusyState = digitalRead(_rdy_busy_pin);

  // wait until !BUSY state switches to READY state (1 ms MAX)
  // or just wait for the Write Cycle Time MAX
  if (currBusyState == LOW) {
    // device is in !BUSY state
    // use the READY/!BUSY pin status to wait for the Write Cycle End
    _write_op_wait_cycles = 0;
    const unsigned int delay_usec = 100;

    int prevBusyState = currBusyState;
    while (write_op_start_usec + _WRITE_SUCCESS_WAITING_TIME_USEC > micros()) {
      delayMicroseconds(delay_usec);
      _write_op_wait_cycles += 1;

      prevBusyState = currBusyState;
      currBusyState = digitalRead(_rdy_busy_pin);
      if (prevBusyState == LOW && currBusyState == HIGH) {  // rising edge
        break;
      }
    }
  } else {
    // device not in !BUSY state
    // use generic delay
    delayMicroseconds(_WRITE_SUCCESS_WAITING_TIME_USEC);
  }
}

void EepromProgrammer::_data_polling(const unsigned long write_op_start_usec, const uint8_t data) {
  // use !DATA polling, if chip doesn't have the RDY/!BUSY pin
  // following the data poll waveforms, the data is read in a loop until the value matches the one written
  // during the write procedure, the data pins remain in a metastable state.
  _setDataBusMode(_DataBusMode::READ);

  _write_op_wait_cycles = 0;
  const unsigned int delay_usec = 50;

  while (write_op_start_usec + _WRITE_SUCCESS_WAITING_TIME_USEC > micros()) {
    delayMicroseconds(delay_usec);
    _write_op_wait_cycles += 1;

    // !DATA polling waveforms require to switch !CE and !OE for every attempt
    digitalWrite(_chip_enable_pin, LOW);
    digitalWrite(_output_enable_pin, LOW);
    // !OE to Output Delay (delta between OE and data ready) == 100 ns MAX
    delayMicroseconds(1);  // arduino cannot delay in ns, only us
    uint8_t read_result = _readData();
    digitalWrite(_output_enable_pin, HIGH);
    digitalWrite(_chip_enable_pin, HIGH);
    if (read_result == data) {
      break;
    }
  }

  _setDataBusMode(_DataBusMode::WRITE);
}


}  // EepromProgrammerLibrary

#endif  // !__eeprom_programmer_lib_h__