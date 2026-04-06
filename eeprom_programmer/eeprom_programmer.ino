#include "board_wiring.h"
#include "eeprom_programmer_lib.h"
#include "binary_protocol.h"

using namespace EepromProgrammerLibrary;
using namespace BoardWiring;
using namespace BinaryProtocol;


// EEPROM Programmer

// specify the wiring type here
// * DIP24
// * DIP28
// static EepromProgrammer eeprom_programmer(BoardWiringType::DIP24);
static EepromProgrammer eeprom_programmer(BoardWiringType::DIP28);


// Binary Protocol Processor

static BinaryProtocolBoard binary_board(command_handler);

void command_handler(uint8_t cmd, uint8_t seq, const uint8_t* payload, uint16_t payload_len) {
  if (cmd == CMD_INIT_CHIP) {
    // payload: chip_name (null-terminated string)
    if (payload_len < 1) {
      binary_board.send_error(cmd, seq, ErrorCode::CHIP_NOT_SUPPORTED, "empty chip name");
      return;
    }
    const char* chip_name = (const char*)payload;

    ErrorCode code = eeprom_programmer.init_chip(String(chip_name));
    if (code != ErrorCode::SUCCESS) {
      char buf[50];
      snprintf(buf, sizeof(buf), "init failed: %d", code);
      binary_board.send_error(cmd, seq, code, buf);
      return;
    }

    // response payload: memory_size as uint32 LE (explicit serialization)
    uint32_t mem_size = eeprom_programmer.get_memory_size_bytes();
    uint8_t mem_size_le[4] = {
      (uint8_t)(mem_size & 0xFF),
      (uint8_t)((mem_size >> 8) & 0xFF),
      (uint8_t)((mem_size >> 16) & 0xFF),
      (uint8_t)((mem_size >> 24) & 0xFF),
    };
    binary_board.send_response(cmd, seq, mem_size_le, 4);

  } else if (cmd == CMD_SET_READ_MODE) {
    // payload: page_size as uint16 LE
    if (payload_len < 2) {
      binary_board.send_error(cmd, seq, ErrorCode::INVALID_PAGE_SIZE, "missing page_size");
      return;
    }
    uint16_t page_size = payload[0] | ((uint16_t)payload[1] << 8);

    ErrorCode code = eeprom_programmer.set_read_mode(page_size);
    if (code != ErrorCode::SUCCESS) {
      char buf[50];
      snprintf(buf, sizeof(buf), "set_read_mode failed: %d", code);
      binary_board.send_error(cmd, seq, code, buf);
      return;
    }

    binary_board.send_response(cmd, seq, nullptr, 0);

  } else if (cmd == CMD_READ_PAGE) {
    // payload: page_no as uint16 LE
    if (payload_len < 2) {
      binary_board.send_error(cmd, seq, ErrorCode::INVALID_PAGE_NO, "missing page_no");
      return;
    }
    uint16_t page_no = payload[0] | ((uint16_t)payload[1] << 8);

    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    uint8_t buffer[page_size];

    ErrorCode code = eeprom_programmer.read_page(page_no, buffer);
    if (code != ErrorCode::SUCCESS) {
      char buf[50];
      snprintf(buf, sizeof(buf), "read_page %d failed: %d", page_no, code);
      binary_board.send_error(cmd, seq, code, buf);
      return;
    }

    binary_board.send_response(cmd, seq, buffer, page_size);

  } else if (cmd == CMD_SET_WRITE_MODE) {
    // payload: page_size as uint16 LE
    if (payload_len < 2) {
      binary_board.send_error(cmd, seq, ErrorCode::INVALID_PAGE_SIZE, "missing page_size");
      return;
    }
    uint16_t page_size = payload[0] | ((uint16_t)payload[1] << 8);

    ErrorCode code = eeprom_programmer.set_write_mode(page_size);
    if (code != ErrorCode::SUCCESS) {
      char buf[50];
      snprintf(buf, sizeof(buf), "set_write_mode failed: %d", code);
      binary_board.send_error(cmd, seq, code, buf);
      return;
    }

    binary_board.send_response(cmd, seq, nullptr, 0);

  } else if (cmd == CMD_WRITE_PAGE) {
    // payload: page_no(uint16 LE) + data(uint8[N])
    if (payload_len < 3) {
      binary_board.send_error(cmd, seq, ErrorCode::INVALID_PAGE_NO, "missing page_no or data");
      return;
    }
    uint16_t page_no = payload[0] | ((uint16_t)payload[1] << 8);
    const uint8_t* data = payload + 2;
    uint16_t data_len = payload_len - 2;

    ErrorCode code = eeprom_programmer.write_page(page_no, data, data_len);
    if (code != ErrorCode::SUCCESS) {
      char buf[50];
      snprintf(buf, sizeof(buf), "write_page %d failed: %d", page_no, code);
      binary_board.send_error(cmd, seq, code, buf);
      return;
    }

    binary_board.send_response(cmd, seq, nullptr, 0);

  } else if (cmd == CMD_GET_READ_PERF) {
    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    unsigned int timings[page_size];
    eeprom_programmer.get_read_byte_usec_for_page(timings, page_size);

    // serialize as uint16 LE array (explicit byte order)
    uint8_t timings_le[page_size * 2];
    for (size_t i = 0; i < page_size; i++) {
      timings_le[i * 2] = (uint8_t)(timings[i] & 0xFF);
      timings_le[i * 2 + 1] = (uint8_t)((timings[i] >> 8) & 0xFF);
    }
    binary_board.send_response(cmd, seq, timings_le, page_size * 2);

  } else if (cmd == CMD_GET_WRITE_PERF) {
    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    unsigned int timings[page_size];
    eeprom_programmer.get_write_byte_usec_for_page(timings, page_size);

    // serialize as uint16 LE array (explicit byte order)
    uint8_t timings_le[page_size * 2];
    for (size_t i = 0; i < page_size; i++) {
      timings_le[i * 2] = (uint8_t)(timings[i] & 0xFF);
      timings_le[i * 2 + 1] = (uint8_t)((timings[i] >> 8) & 0xFF);
    }
    binary_board.send_response(cmd, seq, timings_le, page_size * 2);

  } else {
    // protocol-level error: not from EepromProgrammer ErrorCode enum
    binary_board.send_error(cmd, seq, 0xFFFF, "unknown command");
  }
}


// Arduino

void setup() {
  // protocol board
  binary_board.init();
  // eeprom programmer
  eeprom_programmer.init_programmer();
  // send boot frame
  binary_board.send_boot(
    (uint8_t)eeprom_programmer.get_board_wiring_type(),
    (uint8_t)eeprom_programmer.get_max_page_size());
}

void loop() {
  binary_board.loop();
}
