#ifndef __binary_protocol_h__
#define __binary_protocol_h__

namespace BinaryProtocol {

// Protocol version
static const uint8_t PROTOCOL_VERSION = 1;

// Frame format:
//   [0xAA] [0x55] [LEN_L] [LEN_H] [BODY...] [CRC_L] [CRC_H]
// BODY = CMD(1) | SEQ(1) | PAYLOAD(0..N)
// CRC-16/CCITT (poly 0x1021, init 0xFFFF) over BODY bytes
// All multi-byte integers: little-endian

static const uint8_t SYNC_BYTE_1 = 0xAA;
static const uint8_t SYNC_BYTE_2 = 0x55;

// Commands (request)
static const uint8_t CMD_INIT_CHIP      = 0x01;
static const uint8_t CMD_SET_READ_MODE  = 0x02;
static const uint8_t CMD_READ_PAGE      = 0x03;
static const uint8_t CMD_SET_WRITE_MODE = 0x04;
static const uint8_t CMD_WRITE_PAGE     = 0x05;
static const uint8_t CMD_GET_READ_PERF  = 0x06;
static const uint8_t CMD_GET_WRITE_PERF = 0x07;

// Response = request | 0x80
static const uint8_t RESP_FLAG = 0x80;

// Special commands
static const uint8_t CMD_BOOT  = 0xFE;
static const uint8_t CMD_ERROR = 0xFF;

// Buffer sizes
// Receive body: cmd(1) + seq(1) + page_no(2) + data(64) = 68
static const uint16_t MAX_BODY_SIZE = 68;
// Send frame: sync(2) + len(2) + cmd(1) + seq(1) + max_payload(128) + crc(2) = 136
// Max payload: perf timings 64 x uint16 = 128
static const uint16_t FRAME_HEADER_SIZE = 4;  // sync(2) + len(2)
static const uint16_t FRAME_TRAILER_SIZE = 2; // crc(2)
static const uint16_t MAX_SEND_FRAME_SIZE = 136;


// CRC-16/CCITT (poly 0x1021, init 0xFFFF), bit-by-bit — no lookup table
static uint16_t crc16(const uint8_t* data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }
  return crc;
}


// Receiver state machine
enum RxState : uint8_t {
  WAIT_SYNC1,
  WAIT_SYNC2,
  WAIT_LEN_L,
  WAIT_LEN_H,
  WAIT_BODY,
  WAIT_CRC_L,
  WAIT_CRC_H
};


class BinaryProtocolBoard {

  // cmd, seq, payload, payload_len
  using CommandHandler = void (*)(uint8_t cmd, uint8_t seq, const uint8_t* payload, uint16_t payload_len);

public:
  BinaryProtocolBoard(CommandHandler handler)
    : _handler(handler),
      _state(WAIT_SYNC1),
      _body_pos(0),
      _body_len(0),
      _crc_lo(0) {}

  void init() {
    Serial.begin(_BAUDRATE);
  }

  void loop() {
    while (Serial.available()) {
      uint8_t c = (uint8_t)Serial.read();

      switch (_state) {
        case WAIT_SYNC1:
          if (c == SYNC_BYTE_1) _state = WAIT_SYNC2;
          break;

        case WAIT_SYNC2:
          if (c == SYNC_BYTE_2) {
            _state = WAIT_LEN_L;
          } else if (c == SYNC_BYTE_1) {
            _state = WAIT_SYNC2;  // 0xAA again — could be start of new sync
          } else {
            _state = WAIT_SYNC1;
          }
          break;

        case WAIT_LEN_L:
          _body_len = c;
          _state = WAIT_LEN_H;
          break;

        case WAIT_LEN_H:
          _body_len |= (uint16_t)c << 8;
          if (_body_len < 2 || _body_len > MAX_BODY_SIZE) {
            _state = WAIT_SYNC1;
          } else {
            _body_pos = 0;
            _state = WAIT_BODY;
          }
          break;

        case WAIT_BODY:
          _body_buf[_body_pos++] = c;
          if (_body_pos >= _body_len) {
            _state = WAIT_CRC_L;
          }
          break;

        case WAIT_CRC_L:
          _crc_lo = c;
          _state = WAIT_CRC_H;
          break;

        case WAIT_CRC_H: {
          uint16_t received_crc = _crc_lo | ((uint16_t)c << 8);
          uint16_t computed_crc = crc16(_body_buf, _body_len);
          if (received_crc == computed_crc) {
            uint8_t cmd = _body_buf[0];
            uint8_t seq = _body_buf[1];
            _handler(cmd, seq, _body_buf + 2, _body_len - 2);
          }
          // bad CRC: silently drop, host will timeout and retry
          _state = WAIT_SYNC1;
          break;
        }
      }
    }
  }

  // Send a response frame: cmd byte = request_cmd | RESP_FLAG
  void send_response(uint8_t request_cmd, uint8_t seq, const uint8_t* payload, uint16_t payload_len) {
    _send_frame(request_cmd | RESP_FLAG, seq, payload, payload_len);
  }

  // Send an error frame
  void send_error(uint8_t original_cmd, uint8_t seq, uint16_t error_code, const char* message) {
    // error payload: original_cmd(1) + error_code(2) + message(null-terminated)
    uint16_t msg_len = strlen(message) + 1;
    uint8_t payload[1 + 2 + msg_len];
    payload[0] = original_cmd;
    payload[1] = (uint8_t)(error_code & 0xFF);
    payload[2] = (uint8_t)(error_code >> 8);
    memcpy(payload + 3, message, msg_len);

    _send_frame(CMD_ERROR, seq, payload, 1 + 2 + msg_len);
  }

  // Send the BOOT frame (called once from setup())
  void send_boot(uint8_t wiring_type, uint8_t max_page_size) {
    uint8_t payload[] = { PROTOCOL_VERSION, wiring_type, max_page_size };
    _send_frame(CMD_BOOT, 0, payload, sizeof(payload));
  }

private:
  static const unsigned long _BAUDRATE = 115200;

  CommandHandler _handler;
  RxState _state;

  uint8_t _body_buf[MAX_BODY_SIZE];
  uint16_t _body_pos;
  uint16_t _body_len;
  uint8_t _crc_lo;

  uint8_t _send_buf[MAX_SEND_FRAME_SIZE];

  // Build complete frame in _send_buf and send with a single Serial.write()
  void _send_frame(uint8_t cmd, uint8_t seq, const uint8_t* payload, uint16_t payload_len) {
    uint16_t body_len = 2 + payload_len;
    uint16_t frame_len = FRAME_HEADER_SIZE + body_len + FRAME_TRAILER_SIZE;
    if (frame_len > MAX_SEND_FRAME_SIZE) return;

    // header: sync + len
    _send_buf[0] = SYNC_BYTE_1;
    _send_buf[1] = SYNC_BYTE_2;
    _send_buf[2] = (uint8_t)(body_len & 0xFF);
    _send_buf[3] = (uint8_t)(body_len >> 8);

    // body: cmd + seq + payload
    uint8_t* body = _send_buf + FRAME_HEADER_SIZE;
    body[0] = cmd;
    body[1] = seq;
    if (payload_len > 0 && payload != nullptr) {
      memcpy(body + 2, payload, payload_len);
    }

    // trailer: crc over body
    uint16_t crc = crc16(body, body_len);
    uint16_t crc_offset = FRAME_HEADER_SIZE + body_len;
    _send_buf[crc_offset] = (uint8_t)(crc & 0xFF);
    _send_buf[crc_offset + 1] = (uint8_t)(crc >> 8);

    // single write — no flush(), TX drains asynchronously via hardware
    Serial.write(_send_buf, frame_len);
  }
};

}  // namespace BinaryProtocol

#endif  // !__binary_protocol_h__
