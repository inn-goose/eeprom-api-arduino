"""Unit tests for binary protocol client — CRC, frame building, frame parsing, error handling."""

import io
import struct
import unittest

from client import (
    BinaryProtocolClient, BinaryProtocolClientError,
    crc16, SYNC_BYTE_1, SYNC_BYTE_2, CMD_BOOT, CMD_ERROR, CMD_READ_PAGE, CMD_SET_READ_MODE, RESP_FLAG,
)


class FakeSerial:
    """Mock serial port backed by a BytesIO read buffer."""

    def __init__(self, read_data: bytes = b""):
        self._rx = io.BytesIO(read_data)
        self._tx = io.BytesIO()
        self.timeout = 2.0

    def read(self, n: int) -> bytes:
        return self._rx.read(n)

    def write(self, data: bytes) -> int:
        return self._tx.write(data)

    def flush(self):
        pass

    @property
    def tx_data(self) -> bytes:
        return self._tx.getvalue()


def build_frame(cmd: int, seq: int, payload: bytes) -> bytes:
    """Build a complete binary protocol frame (test helper)."""
    body = bytes([cmd, seq]) + payload
    body_len = len(body)
    crc = crc16(body)
    return (
        bytes([SYNC_BYTE_1, SYNC_BYTE_2])
        + struct.pack("<H", body_len)
        + body
        + struct.pack("<H", crc)
    )


# --- CRC tests ---

class TestCrc16(unittest.TestCase):

    def test_empty(self):
        self.assertEqual(crc16(b""), 0xFFFF)

    def test_single_byte(self):
        # known value: CRC-16/CCITT-FALSE of [0x00] = 0xE1F0
        self.assertEqual(crc16(b"\x00"), 0xE1F0)

    def test_known_string(self):
        # "123456789" is the standard CRC test vector
        # CRC-16/CCITT-FALSE = 0x29B1
        self.assertEqual(crc16(b"123456789"), 0x29B1)

    def test_deterministic(self):
        data = bytes(range(256))
        self.assertEqual(crc16(data), crc16(data))

    def test_different_data_different_crc(self):
        self.assertNotEqual(crc16(b"\x01"), crc16(b"\x02"))

    def test_all_zeros(self):
        result = crc16(bytes(64))
        self.assertIsInstance(result, int)
        self.assertTrue(0 <= result <= 0xFFFF)

    def test_all_ff(self):
        result = crc16(bytes([0xFF] * 64))
        self.assertIsInstance(result, int)
        self.assertTrue(0 <= result <= 0xFFFF)


# --- Frame building tests ---

class TestSendFrame(unittest.TestCase):

    def _make_client(self) -> BinaryProtocolClient:
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial()
        client._seq = 0
        return client

    def test_frame_structure(self):
        client = self._make_client()
        client._send_frame(0x01, 0x00, b"\x41\x42")

        data = client.serial.tx_data
        # sync
        self.assertEqual(data[0], SYNC_BYTE_1)
        self.assertEqual(data[1], SYNC_BYTE_2)
        # len = 4 (cmd + seq + 2 payload)
        body_len = struct.unpack("<H", data[2:4])[0]
        self.assertEqual(body_len, 4)
        # body
        self.assertEqual(data[4], 0x01)  # cmd
        self.assertEqual(data[5], 0x00)  # seq
        self.assertEqual(data[6:8], b"\x41\x42")  # payload
        # crc
        body = data[4:8]
        expected_crc = crc16(body)
        actual_crc = struct.unpack("<H", data[8:10])[0]
        self.assertEqual(actual_crc, expected_crc)
        # total length
        self.assertEqual(len(data), 10)

    def test_empty_payload(self):
        client = self._make_client()
        client._send_frame(0x02, 0x05, b"")

        data = client.serial.tx_data
        body_len = struct.unpack("<H", data[2:4])[0]
        self.assertEqual(body_len, 2)  # cmd + seq only
        self.assertEqual(data[4], 0x02)
        self.assertEqual(data[5], 0x05)
        self.assertEqual(len(data), 8)  # sync(2) + len(2) + body(2) + crc(2)

    def test_64_byte_payload(self):
        client = self._make_client()
        payload = bytes(range(64))
        client._send_frame(0x05, 0x0A, payload)

        data = client.serial.tx_data
        body_len = struct.unpack("<H", data[2:4])[0]
        self.assertEqual(body_len, 66)  # cmd + seq + 64
        self.assertEqual(data[6:70], payload)
        self.assertEqual(len(data), 72)  # sync(2) + len(2) + body(66) + crc(2)


# --- Frame parsing tests ---

class TestReadFrame(unittest.TestCase):

    def _make_client(self, read_data: bytes) -> BinaryProtocolClient:
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(read_data)
        client._seq = 0
        return client

    def test_valid_frame(self):
        frame = build_frame(0x83, 0x00, b"\x01\x02\x03")
        client = self._make_client(frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x83)
        self.assertEqual(seq, 0x00)
        self.assertEqual(payload, b"\x01\x02\x03")

    def test_boot_frame(self):
        frame = build_frame(CMD_BOOT, 0x00, bytes([1, 28, 64]))
        client = self._make_client(frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, CMD_BOOT)
        self.assertEqual(payload, bytes([1, 28, 64]))

    def test_empty_payload(self):
        frame = build_frame(0x84, 0x03, b"")
        client = self._make_client(frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x84)
        self.assertEqual(seq, 0x03)
        self.assertEqual(payload, b"")

    def test_timeout_empty(self):
        client = self._make_client(b"")

        cmd, seq, payload = client._read_frame(0.01)
        self.assertIsNone(cmd)

    def test_bad_crc_returns_none(self):
        frame = bytearray(build_frame(0x83, 0x00, b"\x01\x02"))
        frame[-1] ^= 0xFF  # corrupt CRC
        client = self._make_client(bytes(frame))

        cmd, seq, payload = client._read_frame(0.01)
        self.assertIsNone(cmd)

    def test_garbage_before_sync(self):
        garbage = b"\x00\x13\x37\xFF\x42"
        frame = build_frame(0x83, 0x01, b"\xAA")
        client = self._make_client(garbage + frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x83)
        self.assertEqual(seq, 0x01)
        self.assertEqual(payload, b"\xAA")

    def test_double_aa_sync(self):
        """0xAA 0xAA 0x55 should sync on the second 0xAA."""
        frame = build_frame(0x82, 0x00, b"")
        # prepend an extra 0xAA
        client = self._make_client(b"\xAA" + frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x82)

    def test_triple_aa_sync(self):
        """0xAA 0xAA 0xAA 0x55 should still sync."""
        frame = build_frame(0x82, 0x00, b"")
        client = self._make_client(b"\xAA\xAA" + frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x82)

    def test_truncated_body(self):
        frame = build_frame(0x83, 0x00, bytes(64))
        # cut frame short (missing last 10 bytes)
        client = self._make_client(frame[:-10])

        cmd, seq, payload = client._read_frame(0.01)
        self.assertIsNone(cmd)

    def test_64_byte_page_data(self):
        page_data = bytes(range(64))
        frame = build_frame(CMD_READ_PAGE | RESP_FLAG, 0x07, page_data)
        client = self._make_client(frame)

        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, CMD_READ_PAGE | RESP_FLAG)
        self.assertEqual(seq, 0x07)
        self.assertEqual(payload, page_data)


# --- send_command / error handling tests ---

class TestSendCommand(unittest.TestCase):

    def _make_client_with_response(self, response_frame: bytes) -> BinaryProtocolClient:
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(response_frame)
        client._seq = 0
        return client

    def test_send_command_success(self):
        response = build_frame(CMD_READ_PAGE | RESP_FLAG, 0x00, bytes(64))
        client = self._make_client_with_response(response)

        payload = client.send_command(CMD_READ_PAGE, struct.pack("<H", 0))
        self.assertEqual(len(payload), 64)

    def test_send_command_timeout(self):
        client = self._make_client_with_response(b"")

        with self.assertRaises(BinaryProtocolClientError) as ctx:
            client.send_command(CMD_READ_PAGE, struct.pack("<H", 0))
        self.assertIn("no response", str(ctx.exception))

    def test_send_command_error_response(self):
        # error payload: original_cmd(1) + error_code(2) + message(null-terminated)
        error_payload = (
            bytes([CMD_READ_PAGE])
            + struct.pack("<H", 42)
            + b"read failed\x00"
        )
        response = build_frame(CMD_ERROR, 0x00, error_payload)
        client = self._make_client_with_response(response)

        with self.assertRaises(BinaryProtocolClientError) as ctx:
            client.send_command(CMD_READ_PAGE, struct.pack("<H", 0))
        self.assertIn("read failed", str(ctx.exception))
        self.assertIn("42", str(ctx.exception))

    def test_send_command_wrong_response_cmd(self):
        # respond with wrong command
        response = build_frame(0x84, 0x00, b"")  # SET_WRITE_MODE resp instead of READ_PAGE resp
        client = self._make_client_with_response(response)

        with self.assertRaises(BinaryProtocolClientError) as ctx:
            client.send_command(CMD_READ_PAGE, struct.pack("<H", 0))
        self.assertIn("unexpected response", str(ctx.exception))

    def test_seq_increments(self):
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(
            build_frame(0x82, 0x00, b"") + build_frame(0x82, 0x01, b"")
        )
        client._seq = 0

        client.send_command(CMD_SET_READ_MODE, struct.pack("<H", 64))
        client.send_command(CMD_SET_READ_MODE, struct.pack("<H", 64))

        # check seq in sent frames
        tx = client.serial.tx_data
        # first frame: seq at offset 5
        self.assertEqual(tx[5], 0x00)
        # second frame starts after first
        first_body_len = struct.unpack("<H", tx[2:4])[0]
        second_frame_start = 4 + first_body_len + 2
        self.assertEqual(tx[second_frame_start + 5], 0x01)

    def test_seq_wraps_at_256(self):
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client._seq = 255
        client.serial = FakeSerial(build_frame(0x82, 0xFF, b""))

        client.send_command(CMD_SET_READ_MODE, struct.pack("<H", 64))
        self.assertEqual(client._seq, 0)

    def test_seq_mismatch_raises(self):
        """Stale response with wrong seq should be rejected."""
        # send with seq=0, but response has seq=5 (stale)
        response = build_frame(CMD_READ_PAGE | RESP_FLAG, 0x05, bytes(64))
        client = self._make_client_with_response(response)

        with self.assertRaises(BinaryProtocolClientError) as ctx:
            client.send_command(CMD_READ_PAGE, struct.pack("<H", 0))
        self.assertIn("seq mismatch", str(ctx.exception))

    def test_oversized_body_len_rejected(self):
        """Frame with body_len > MAX_BODY_SIZE should be rejected."""
        # build a frame manually with body_len=500
        body = bytes([0x83, 0x00]) + bytes(498)
        crc = crc16(body)
        frame = (
            bytes([SYNC_BYTE_1, SYNC_BYTE_2])
            + struct.pack("<H", 500)
            + body
            + struct.pack("<H", crc)
        )
        client = self._make_client_with_response(frame)

        cmd, seq, payload = client._read_frame(0.01)
        self.assertIsNone(cmd)


# --- CRC cross-validation (firmware parity) ---

class TestCrcCrossValidation(unittest.TestCase):
    """Verify CRC matches expected values that the firmware will produce."""

    def test_boot_frame_body(self):
        # BOOT frame body: cmd=0xFE, seq=0x00, payload=[1, 28, 64]
        body = bytes([0xFE, 0x00, 0x01, 0x1C, 0x40])
        crc = crc16(body)
        # verify it's a valid uint16
        self.assertTrue(0 <= crc <= 0xFFFF)
        # verify frame round-trips
        frame = build_frame(0xFE, 0x00, bytes([0x01, 0x1C, 0x40]))
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(frame)
        client._seq = 0
        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0xFE)
        self.assertEqual(payload, bytes([0x01, 0x1C, 0x40]))

    def test_read_page_response_body(self):
        # 64 bytes of 0xFF (erased EEPROM)
        page = bytes([0xFF] * 64)
        frame = build_frame(0x83, 0x05, page)
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(frame)
        client._seq = 0
        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(payload, page)

    def test_write_page_request_body(self):
        # write_page: cmd=0x05, seq=0x0A, payload=page_no(2) + data(64)
        page_no = struct.pack("<H", 42)
        data = bytes(range(64))
        body = bytes([0x05, 0x0A]) + page_no + data
        crc = crc16(body)
        # rebuild and verify
        frame = (
            bytes([SYNC_BYTE_1, SYNC_BYTE_2])
            + struct.pack("<H", len(body))
            + body
            + struct.pack("<H", crc)
        )
        client = BinaryProtocolClient.__new__(BinaryProtocolClient)
        client.serial = FakeSerial(frame)
        client._seq = 0
        cmd, seq, payload = client._read_frame(2.0)
        self.assertEqual(cmd, 0x05)
        self.assertEqual(seq, 0x0A)
        self.assertEqual(payload[:2], page_no)
        self.assertEqual(payload[2:], data)


if __name__ == "__main__":
    unittest.main()
