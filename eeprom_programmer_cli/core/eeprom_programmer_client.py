import struct

from binary_protocol.client import (
    BinaryProtocolClient, BinaryProtocolClientError,
    CMD_INIT_CHIP, CMD_SET_READ_MODE, CMD_READ_PAGE,
    CMD_SET_WRITE_MODE, CMD_WRITE_PAGE, CMD_GET_READ_PERF, CMD_GET_WRITE_PERF,
)


class EepromProgrammerClientError(Exception):
    pass


class EepromProgrammerClient:
    _READ_PAGE_SIZE = 64
    _WRITE_PAGE_SIZE = 64

    def __init__(self, port: str, baudrate: int, init_timeout: int):
        self._connect_programmer(port, baudrate, init_timeout)

    def _connect_programmer(self, port: str, baudrate: int, init_timeout: int):
        self._client = BinaryProtocolClient(
            port=port, baudrate=baudrate, init_timeout=float(init_timeout))

        boot_info = self._client.init()
        if not boot_info:
            raise EepromProgrammerClientError("failed to connect programmer, no BOOT frame")

        self.programmer_settings = {
            "board_wiring_type": boot_info["board_wiring_type"],
            "max_page_size": boot_info["max_page_size"],
        }
        print(f"programmer_settings: {self.programmer_settings}")

        if self.programmer_settings["board_wiring_type"] == 0:
            raise EepromProgrammerClientError("invalid board_wiring_type")

    def init_chip(self, chip_type: str):
        try:
            payload = self._client.send_command(
                CMD_INIT_CHIP, chip_type.encode("ascii") + b"\x00")
        except BinaryProtocolClientError as ex:
            raise EepromProgrammerClientError(
                f"failed to init {chip_type} chip with: {ex}")

        if len(payload) < 4:
            raise EepromProgrammerClientError(
                f"empty chip settings for {chip_type}")

        memory_size = struct.unpack("<I", payload[:4])[0]
        self.chip_settings = {
            "memory_size": memory_size,
        }
        print(f"chip settings: {self.chip_settings}")

    def _set_read_mode(self, page_size: int):
        try:
            self._client.send_command(
                CMD_SET_READ_MODE, struct.pack("<H", page_size))
            print(f"set_read_mode: READ mode is ON for {page_size} bytes pages")
        except BinaryProtocolClientError as ex:
            raise EepromProgrammerClientError(
                f"failed to set READ mode with: {ex}")

    def read_data(self, collect_performance: bool = False) -> bytes:
        page_size = self._READ_PAGE_SIZE
        memory_size = self.chip_settings["memory_size"]
        pages_total = int(memory_size / page_size)

        # set READ mode
        self._set_read_mode(page_size)

        if collect_performance:
            read_performance = []

        output_data = bytearray()
        for page_no in range(pages_total):
            resp = self._client.send_command(
                CMD_READ_PAGE, struct.pack("<H", page_no))
            if len(resp) != page_size:
                raise EepromProgrammerClientError(
                    f"read_page {page_no}: expected {page_size} bytes, got {len(resp)}")
            output_data.extend(resp)
            if collect_performance:
                perf_resp = self._client.send_command(CMD_GET_READ_PERF)
                read_performance.extend(struct.unpack(f"<{len(perf_resp)//2}H", perf_resp))

        if collect_performance:
            print("AVG read time {:.2f} us".format(sum(read_performance) / len(read_performance)))

        return bytes(output_data)

    def _set_write_mode(self, page_size: int):
        try:
            self._client.send_command(
                CMD_SET_WRITE_MODE, struct.pack("<H", page_size))
            print(f"set_write_mode: WRITE mode is ON for {page_size} bytes pages")
        except BinaryProtocolClientError as ex:
            raise EepromProgrammerClientError(
                f"failed to set WRITE mode with: {ex}")

    def write_data(self, input_data: bytes, collect_performance: bool = False):
        page_size = self._WRITE_PAGE_SIZE
        pages_total = int(len(input_data) / page_size)
        # last page
        if len(input_data) > pages_total * page_size:
            pages_total += 1

        # set WRITE mode
        self._set_write_mode(page_size)

        if collect_performance:
            write_performance = []

        for page_no in range(pages_total):
            address = page_no * page_size
            page_data = input_data[address:(address+page_size)]
            self._client.send_command(
                CMD_WRITE_PAGE, struct.pack("<H", page_no) + page_data)
            if collect_performance:
                perf_resp = self._client.send_command(CMD_GET_WRITE_PERF)
                write_performance.extend(struct.unpack(f"<{len(perf_resp)//2}H", perf_resp))

        if collect_performance:
            print("AVG write time {:.2f} us".format(sum(write_performance) / len(write_performance)))

    def erase_data(self, erase_pattern: int, collect_performance: bool = False):
        if erase_pattern < 0 or erase_pattern > 255:
            raise EepromProgrammerClientError(
                f"failed to erase data, invalid pattern: {erase_pattern}")

        memory_size = self.chip_settings["memory_size"]
        input_data = bytes([erase_pattern] * memory_size)

        self.write_data(input_data, collect_performance)
