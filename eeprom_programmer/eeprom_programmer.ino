#include "board_wiring.h"
#include "eeprom_programmer_lib.h"
#include "serial_json_rpc_lib.h"

using namespace EepromProgrammerLibrary;
using namespace BoardWiring;
using namespace SerialJsonRpcLibrary;


// EEPROM Programmer

static EepromProgrammer eeprom_programmer(BoardWiringType::DIP28);


// Serial JSON RPC Processor

static SerialJsonRpcBoard rpc_board(rpc_processor);

void rpc_processor(int request_id, const String &method, const String params[], int params_size) {
  if (method == "init_chip") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected: (chip_type)");
      return;
    }
    String chip_type = params[0];

    ErrorCode code = eeprom_programmer.init_chip(chip_type);
    if (code != ErrorCode::SUCCESS) {
      const size_t error_data_buf_size = 50;
      char error_data_buf[error_data_buf_size];
      snprintf(error_data_buf, error_data_buf_size, "Failed to init %s chip with error: %d", chip_type.c_str(), code);
      rpc_board.send_error(request_id, -32010, "Service error", error_data_buf);
      return;
    }

    int32_t chip_settings[] = {
      eeprom_programmer.get_memory_size_bytes(),
    };
    rpc_board.send_result_ints(request_id, chip_settings, sizeof(chip_settings) / sizeof(chip_settings[0]));

  } else if (method == "set_read_mode") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected: (read_page_size_bytes)");
      return;
    }
    const int read_page_size_bytes = atoi(params[0].c_str());

    ErrorCode code = eeprom_programmer.set_read_mode(read_page_size_bytes);
    if (code != ErrorCode::SUCCESS) {
      const size_t error_data_buf_size = 70;
      char error_data_buf[error_data_buf_size];
      snprintf(error_data_buf, error_data_buf_size, "Failed to set READ mode for page size %d with error: %d", read_page_size_bytes, code);
      rpc_board.send_error(request_id, -32020, "Service error", error_data_buf);
      return;
    }

    const size_t result_buf_size = 50;
    char result_buf[result_buf_size];
    snprintf(result_buf, result_buf_size, "READ mode is ON for %d bytes pages", read_page_size_bytes);
    rpc_board.send_result_string(request_id, result_buf);

  } else if (method == "read_page") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected: (page_no)");
      return;
    }
    const int page_no = atoi(params[0].c_str());

    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    uint8_t buffer[page_size];

    ErrorCode code = eeprom_programmer.read_page(page_no, buffer);
    if (code != ErrorCode::SUCCESS) {
      const size_t error_data_buf_size = 70;
      char error_data_buf[error_data_buf_size];
      snprintf(error_data_buf, error_data_buf_size, "Failed to READ page %d with error: %d", page_no, code);
      rpc_board.send_error(request_id, -32021, "Service error", error_data_buf);
      return;
    }

    rpc_board.send_result_bytes(request_id, buffer, page_size);

  } else if (method == "set_write_mode") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected: (write_page_size_bytes)");
      return;
    }
    const int write_page_size_bytes = atoi(params[0].c_str());

    ErrorCode code = eeprom_programmer.set_write_mode(write_page_size_bytes);
    if (code != ErrorCode::SUCCESS) {
      const size_t error_data_buf_size = 70;
      char error_data_buf[error_data_buf_size];
      snprintf(error_data_buf, error_data_buf_size, "Failed to set WRITE mode for page size %d with error: %d", write_page_size_bytes, code);
      rpc_board.send_error(request_id, -32030, "Service error", error_data_buf);
      return;
    }

    const size_t result_buf_size = 50;
    char result_buf[result_buf_size];
    snprintf(result_buf, result_buf_size, "WRITE mode is ON for %d bytes pages", write_page_size_bytes);
    rpc_board.send_result_string(request_id, result_buf);

  } else if (method == "write_page") {
    if (params_size != 2) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected: (page_no, bytes_to_write)");
      return;
    }
    const int page_no = atoi(params[0].c_str());

    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    uint8_t buffer[page_size];
    const size_t json_array_size = SerialJsonRpcBoard::json_array_to_byte_array(params[1], buffer, page_size);

    ErrorCode code = eeprom_programmer.write_page(page_no, buffer, json_array_size);
    if (code != ErrorCode::SUCCESS) {
      const size_t error_data_buf_size = 70;
      char error_data_buf[error_data_buf_size];
      snprintf(error_data_buf, error_data_buf_size, "Failed to WRITE page %d with error: %d", page_no, code);
      rpc_board.send_error(request_id, -32031, "Service error", error_data_buf);
      return;
    }

    const size_t result_buf_size = 50;
    char result_buf[result_buf_size];
    snprintf(result_buf, result_buf_size, "WRITE success. %d bytes written", json_array_size);
    rpc_board.send_result_string(request_id, result_buf);

  } else if (method == "get_write_perf") {
    const size_t page_size = eeprom_programmer.get_page_size_bytes();
    unsigned long wait_time_for_page[page_size];
    eeprom_programmer.get_write_op_wait_time_usec_for_page(wait_time_for_page, page_size);

    rpc_board.send_result_ints(request_id, wait_time_for_page, page_size);

  } else {
    rpc_board.send_error(request_id, -32601, "Method not found", method.c_str());
  }
}


// Arduino

void setup() {
  // rpc board
  rpc_board.init();
  // eeprom programmer
  eeprom_programmer.init_programmer();
  // eeprom programmer settings
  int32_t programmer_settings[] = {
    eeprom_programmer.get_max_page_size(),
  };
  rpc_board.send_result_ints(0, programmer_settings, sizeof(programmer_settings) / sizeof(programmer_settings[0]));
}

void loop() {
  rpc_board.loop();
}
