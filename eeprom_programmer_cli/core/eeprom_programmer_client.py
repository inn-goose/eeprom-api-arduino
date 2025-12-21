from serial_json_rpc.client import SerialJsonRpcClient


class EepromProgrammerClientError(Exception):
    pass


class EepromProgrammerClient:
    _READ_PAGE_SIZE = 64
    _WRITE_PAGE_SIZE = 64

    def __init__(self, port: str, baudrate: int, init_timeout: int):
        self._connect_programmer(port, baudrate, init_timeout)

    def _connect_programmer(self, port: str, baudrate: int, init_timeout: int):
        self._json_rpc_client = SerialJsonRpcClient(
            port=port, baudrate=baudrate, init_timeout=float(init_timeout))

        programmer_settings = self._json_rpc_client.init()
        if not programmer_settings:
            raise EepromProgrammerClientError("failed to connect programmer, empty settings returned")

        self.programmer_settings = {
            "max_page_size": programmer_settings[0],
        }
        print(f"programmer_settings: {self.programmer_settings}")

    def init_chip(self, chip_type: str):
        try:
            chip_settings = self._json_rpc_client.send_request("init_chip", [chip_type])
        except Exception as ex:
            raise EepromProgrammerClientError(
                f"failed to init {chip_type} chip with: {ex}")
        if not chip_settings:
            raise EepromProgrammerClientError(
                f"empty chip settings for {chip_type}")
        self.chip_settings = {
            "memory_size": chip_settings[0],
        }
        print(f"chip settings: {self.chip_settings}")

    def _set_read_mode(self, page_size: int):
        try:
            res = self._json_rpc_client.send_request("set_read_mode", [page_size])
            print(f"set_read_mode: {res}")
        except Exception as ex:
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

        output_data = []
        for page_no in range(pages_total):
            resp = self._json_rpc_client.send_request("read_page", [page_no])
            output_data += resp
            if collect_performance:
                read_performance.extend(self._json_rpc_client.send_request("get_read_perf", None))

        if collect_performance:
            print("AVG read time {:.2f} ms".format(sum(read_performance) / len(read_performance)))

        return bytes(output_data)

    def _set_write_mode(self, page_size: int):
        try:
            res = self._json_rpc_client.send_request("set_write_mode", [page_size])
            print(f"set_write_mode: {res}")
        except Exception as ex:
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

        # convert bytes to array
        input_data = [b for b in input_data]

        if collect_performance:
            write_performance = []

        for page_no in range(pages_total):
            address = page_no * page_size
            page_data = input_data[address:(address+page_size)]
            self._json_rpc_client.send_request("write_page", [page_no, page_data])
            if collect_performance:
                write_performance.extend(self._json_rpc_client.send_request("get_write_perf", None))

        if collect_performance:
            print("AVG write time {:.2f} ms".format(sum(write_performance) / len(write_performance)))

    def erase_data(self, erase_pattern: int, collect_performance: bool = False):
        if erase_pattern < 0 or erase_pattern > 255:
            raise EepromProgrammerClientError(
                f"failed to erase data, invalid pattern: {erase_pattern}")

        memory_size = self.chip_settings["memory_size"]
        input_data = bytes([erase_pattern] * memory_size)

        self.write_data(input_data, collect_performance)
