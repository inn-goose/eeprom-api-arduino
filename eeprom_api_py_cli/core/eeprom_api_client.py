import sys

from serial_json_rpc import client


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def read_data(json_rpc_client: client.SerialJsonRpcClient, eeprom_type: str, dump_file_path: str) -> int:
    if not eeprom_type:
        eprint("failed to read data, eeprom_type is unknown")
        return 1

    if not dump_file_path:
        eprint("failed to read data, dump_file_path is empty")
        return 1
    
    # init READ mode
    try:
        json_rpc_client.send_request("init_read", [eeprom_type])
    except Exception as ex:
        eprint(f"failed to init READ mode for the board with: {ex}")
        return 1
    
    memory_size = 8192
    page_size = 64
    pages_total = int(memory_size / page_size)

    data = []
    for i in range(pages_total):
        resp = json_rpc_client.send_request("read_page", [page_size, i])
        data += resp

    with open(dump_file_path, "wb") as f:
        f.write(bytes(data))

    return 0


def write_data(json_rpc_client: client.SerialJsonRpcClient, eeprom_type: str, dump_file_path: str) -> int:
    eprint("unsupported MODE")
    return 1
