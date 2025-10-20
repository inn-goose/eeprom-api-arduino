#include <ArduinoJson.h>

namespace SerialJsonRpcLibrary {

class SerialJsonRpcBoard {

  // request_id, method, params[], params_size
  using RpcProcessor = void (*)(int, const String&, const String[], int);

public:
  SerialJsonRpcBoard(RpcProcessor rpc_processor);

  void init();
  void loop();

  void send_result(int id, const char* result_message);
  void send_error(int id, int error_code, const char* error_message, const char* error_data);

  String convert_bytes_array_to_json(const uint8_t* buffer, int buffer_size);

private:
  // default baudrate
  static const unsigned long _DEFAULT_BAUDRATE = 115200;

  // balance between the protocol throughput and the board memory limit
  // works fine for UNO R3
  static const int _JSON_RPC_BUFFER_SIZE = 350;

  // use \n for simiplicity to use both py-client and Arduino Serial Monitor
  static const char _END_OF_JSON_RPC_MESSAGE = '\n';

  void SerialJsonRpcBoard::_process_request(JsonDocument& request);

  int baudrate;

  RpcProcessor rpc_processor_callback;

  char serial_read_buffer[_JSON_RPC_BUFFER_SIZE];
  int serial_read_buffer_pos;
};

SerialJsonRpcBoard::SerialJsonRpcBoard(RpcProcessor rpc_processor)
  : rpc_processor_callback(rpc_processor), serial_read_buffer_pos(0) {}

void SerialJsonRpcBoard::init() {
  Serial.begin(_DEFAULT_BAUDRATE);
}

void SerialJsonRpcBoard::loop() {
  // read data by char if any
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == _END_OF_JSON_RPC_MESSAGE) {
      DynamicJsonDocument request(serial_read_buffer_pos);
      DeserializationError deserialization_error = deserializeJson(request, serial_read_buffer, serial_read_buffer_pos);
      if (deserialization_error) {
        const char* error_data = deserialization_error.c_str();
        send_error(0, -32700, "Parse error", error_data);
      } else {
        _process_request(request);
      }
      request.clear();
      request.garbageCollect();
      serial_read_buffer_pos = 0;
      return;
    }

    // buffer overflow
    if (serial_read_buffer_pos >= _JSON_RPC_BUFFER_SIZE) {
      send_error(0, -32600, "Invalid Request", "JSON RPC message is to large");
      serial_read_buffer_pos = 0;
      return;
    }

    // read next char
    serial_read_buffer[serial_read_buffer_pos++] = c;
  }
}

void SerialJsonRpcBoard::send_result(int id, const char* result_message) {
  // {"jsonrpc":"2.0","id":-,"result":""}
  // base lenght is 36
  // +10 for ID (max signed 32 len)
  // 46 in total
  DynamicJsonDocument response(46 + strlen(result_message));
  response["jsonrpc"] = "2.0";
  response["id"] = id;

  response["result"] = result_message;

  serializeJson(response, Serial);
  response.clear();
  response.garbageCollect();

  Serial.write(_END_OF_JSON_RPC_MESSAGE);
  Serial.flush();
}

void SerialJsonRpcBoard::send_error(int id, int error_code, const char* error_message, const char* error_data) {
  // {"jsonrpc":"2.0","id":-,"error":{"code":-,"message":"","data":""}}
  // base lenght is 66
  // +10 for ID (max signed 32 len)
  // +10 for error_code
  // 86 in total
  DynamicJsonDocument response(86 + strlen(error_message) + strlen(error_data));
  response["jsonrpc"] = "2.0";
  response["id"] = id;

  JsonObject error = response.createNestedObject("error");
  error["code"] = error_code;
  error["message"] = error_message;
  if (error_data != 0) {
    error["data"] = error_data;
  }

  serializeJson(response, Serial);
  response.clear();
  response.garbageCollect();

  Serial.write(_END_OF_JSON_RPC_MESSAGE);
  Serial.flush();
}

String SerialJsonRpcBoard::convert_bytes_array_to_json(const uint8_t* buffer, int buffer_size) {
  String json_data = "[";
  for (int i = 0; i < buffer_size; i ++) {
    json_data += String(buffer[i]) + ",";
  }
  json_data.setCharAt(json_data.length() - 1, ']');
  return json_data;
}

void SerialJsonRpcBoard::_process_request(JsonDocument& request) {
  // validata JSON RPC format
  if (!request.containsKey("jsonrpc") || strcmp(request["jsonrpc"], "2.0") != 0) {
    send_error(0, -32600, "Invalid Request", "Invalid protocol version");
    return;
  }

  int request_id = request.containsKey("id") ? request["id"].as<int>() : 0;

  const String method = request["method"] | "";
  JsonVariant params = request["params"];

  if (!params.is<JsonArray>()) {
    send_error(request_id, -32602, "Invalid params", "Array expected");
    return;
  }

  // convert JsonArray to const String[]
  JsonArray params_json_array = params.as<JsonArray>();
  const size_t params_size = params_json_array.size();
  const String params_array[params_size];
  for (size_t i = 0; i < params_size; i++) {
    params_array[i] = params_json_array[i].as<String>();
  }

  rpc_processor_callback(request_id, method, params_array, params_size);
}

}
