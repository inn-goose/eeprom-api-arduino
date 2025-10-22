#include "eeprom_wiring.h"
#include "eeprom_api.h"
#include "serial_json_rpc.h"

using namespace EepromApiLibrary;
using namespace SerialJsonRpcLibrary;


// EEPROM API

static EepromApi eeprom_api(
  // address
  EEPROM_ADDRESS_PINS,
  // data
  EEPROM_DATA_PINS,
  // management
  EEPROM_CHIP_ENABLE_PIN,
  EEPROM_OUTPUT_ENABLE_PIN,
  EEPROM_WRITE_ENABLE_PIN,
  // status
  EEPROM_READY_BUSY_OUTPUT_PIN);


// Serial JSON RPC Processor

static SerialJsonRpcBoard rpc_board(rpc_processor);

void rpc_processor(int request_id, const String &method, const String params[], int params_size) {
  if (method == "init_read") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "params_size != 1");
      return;
    }

    String eeprom_type = params[0];

    eeprom_api.readInit();

    rpc_board.send_result_string(request_id, "OK. Initialized");

  } else if (method == "read_page") {
    if (params_size != 2) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "params_size != 2");
      return;
    }

    const int page_size_bytes = atoi(params[0].c_str());
    const int page_no = atoi(params[1].c_str());

    uint8_t buffer[page_size_bytes];
    const int start = page_size_bytes * page_no;
    for (int i = 0; i < page_size_bytes; i ++) {
      buffer[i] = eeprom_api.readData(start + i);
    }

    rpc_board.send_result_bytes(request_id, buffer, page_size_bytes);

  } else {
    rpc_board.send_error(request_id, -32601, "Method not found", method.c_str());
  }
}


// Arduino

void setup() {
  // rpc board
  rpc_board.init();
  // eeprom api
  eeprom_api.init();
}

void loop() {
  rpc_board.loop();
}
