import struct

import serial


class BinaryProtocolClientError(Exception):
    pass


# Frame format:
#   [0xAA] [0x55] [LEN_L] [LEN_H] [BODY...] [CRC_L] [CRC_H]
# BODY = CMD(1) | SEQ(1) | PAYLOAD(0..N)
# CRC-16/CCITT over BODY bytes. All multi-byte integers: little-endian.

SYNC_BYTE_1 = 0xAA
SYNC_BYTE_2 = 0x55

# Commands (request)
CMD_INIT_CHIP      = 0x01
CMD_SET_READ_MODE  = 0x02
CMD_READ_PAGE      = 0x03
CMD_SET_WRITE_MODE = 0x04
CMD_WRITE_PAGE     = 0x05
CMD_GET_READ_PERF  = 0x06
CMD_GET_WRITE_PERF = 0x07

RESP_FLAG = 0x80

# Special commands
CMD_BOOT  = 0xFE
CMD_ERROR = 0xFF

# Max body size: matches firmware MAX_SEND_BODY_SIZE (cmd + seq + max_payload)
MAX_BODY_SIZE = 130


def crc16(data: bytes) -> int:
    """CRC-16/CCITT (poly 0x1021, init 0xFFFF), matching firmware implementation."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


class BinaryProtocolClient:

    RESPONSE_TIMEOUT_SEC = 2.0

    def __init__(self, port: str, baudrate: int, init_timeout: float):
        self.port = port
        self.baudrate = baudrate
        self.init_timeout = init_timeout
        self.serial = None
        self._seq = 0

    def init(self):
        """Open serial, wait for BOOT frame from firmware."""
        if self.serial is not None:
            return

        try:
            self.serial = serial.Serial(
                port=self.port, baudrate=self.baudrate, timeout=self.init_timeout)
        except Exception as ex:
            raise BinaryProtocolClientError(
                f"failed to open serial port: {ex}")

        # read BOOT frame (firmware sends on startup after reset)
        cmd, _, payload = self._read_frame(self.init_timeout)
        if cmd is None:
            raise BinaryProtocolClientError("no BOOT frame received")
        if cmd != CMD_BOOT:
            raise BinaryProtocolClientError(
                f"expected BOOT frame (0xFE), got 0x{cmd:02X}")

        # payload: version(1) + wiring_type(1) + max_page_size(1)
        if len(payload) < 3:
            raise BinaryProtocolClientError(
                f"BOOT payload too short: {len(payload)}")

        return {
            "version": payload[0],
            "board_wiring_type": payload[1],
            "max_page_size": payload[2],
        }

    def send_command(self, cmd: int, payload: bytes = b"") -> bytes:
        """Send a command and return the response payload."""
        seq = self._seq
        self._seq = (self._seq + 1) & 0xFF

        self._send_frame(cmd, seq, payload)

        resp_cmd, resp_seq, resp_payload = self._read_frame(self.RESPONSE_TIMEOUT_SEC)
        if resp_cmd is None:
            raise BinaryProtocolClientError(
                f"no response for cmd 0x{cmd:02X}, seq {seq}")

        if resp_cmd == CMD_ERROR:
            self._handle_error(resp_payload)

        expected_resp = cmd | RESP_FLAG
        if resp_cmd != expected_resp:
            raise BinaryProtocolClientError(
                f"unexpected response cmd 0x{resp_cmd:02X}, expected 0x{expected_resp:02X}")

        if resp_seq != seq:
            raise BinaryProtocolClientError(
                f"seq mismatch: expected {seq}, got {resp_seq}")

        return resp_payload

    def _send_frame(self, cmd: int, seq: int, payload: bytes):
        body = bytes([cmd, seq]) + payload
        body_len = len(body)
        crc = crc16(body)

        frame = (
            bytes([SYNC_BYTE_1, SYNC_BYTE_2])
            + struct.pack("<H", body_len)
            + body
            + struct.pack("<H", crc)
        )

        written = self.serial.write(frame)
        if written != len(frame):
            raise BinaryProtocolClientError(
                f"short write: {written}/{len(frame)} bytes")
        self.serial.flush()

    def _read_frame(self, timeout: float):
        """Read a complete frame. Returns (cmd, seq, payload) or (None, None, None) on timeout."""
        prev_timeout = self.serial.timeout
        self.serial.timeout = timeout

        try:
            # sync — state machine matching firmware
            state = 'SYNC1'
            while True:
                b = self.serial.read(1)
                if len(b) == 0:
                    return None, None, None
                if state == 'SYNC1':
                    if b[0] == SYNC_BYTE_1:
                        state = 'SYNC2'
                elif state == 'SYNC2':
                    if b[0] == SYNC_BYTE_2:
                        break
                    elif b[0] != SYNC_BYTE_1:
                        state = 'SYNC1'
                    # else: another 0xAA, stay in SYNC2

            # use caller's timeout for remainder of frame too
            # length
            len_bytes = self.serial.read(2)
            if len(len_bytes) < 2:
                return None, None, None
            body_len = struct.unpack("<H", len_bytes)[0]

            if body_len < 2 or body_len > MAX_BODY_SIZE:
                return None, None, None

            # body
            body = self.serial.read(body_len)
            if len(body) < body_len:
                return None, None, None

            # crc
            crc_bytes = self.serial.read(2)
            if len(crc_bytes) < 2:
                return None, None, None
            received_crc = struct.unpack("<H", crc_bytes)[0]

            computed_crc = crc16(body)
            if received_crc != computed_crc:
                return None, None, None

            cmd = body[0]
            seq = body[1]
            payload = body[2:]
            return cmd, seq, bytes(payload)

        finally:
            self.serial.timeout = prev_timeout

    def _handle_error(self, payload: bytes):
        """Parse ERROR frame payload and raise."""
        if len(payload) < 3:
            raise BinaryProtocolClientError("error response with no details")

        original_cmd = payload[0]
        error_code = struct.unpack("<H", payload[1:3])[0]
        message = payload[3:].split(b"\x00")[0].decode("ascii", errors="replace")

        raise BinaryProtocolClientError(
            f"cmd 0x{original_cmd:02X} error {error_code}: {message}")
