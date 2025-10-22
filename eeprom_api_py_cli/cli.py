from enum import Enum

import argparse
import sys

from serial_json_rpc import client


class Method(Enum):
    READ = "read"

    def __str__(self):
        return self.value


def to_ascii(val):
    if (val >= 33 and val <= 126) or val == 20:
        return chr(val)
    return '.'


def execute_method(json_rpc_client: client.SerialJsonRpcClient, method: str) -> str:
    if method == Method.READ:
        resp = json_rpc_client.send_request("init_read", ["AT28C64"])
        # print("init_read:", resp)

        memory_size = 8192
        page_size = 16
        pages_total = int(memory_size / page_size)

        for i in range(pages_total):
            resp = json_rpc_client.send_request("read_page", [16, i])
            address = page_size * i
            hex = "".join([f"{r:02x}" + (" " if (n % 2 == 1) else "")
                          for (n, r) in enumerate(resp)])
            ascii = "".join([to_ascii(r) for r in resp])
            print(f"{address:08x}: {hex}{ascii}")

        return "OK"

    else:
        raise Exception(f"unknown method: {method}")


def cli() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('port', type=str)
    parser.add_argument('--baudrate', type=int, default=115200)
    parser.add_argument('--init-timeout', type=int, default=3)
    parser.add_argument('method', type=Method, choices=list(Method))
    args = parser.parse_args()

    # init
    json_rpc_client = client.SerialJsonRpcClient(
        port=args.port, baudrate=args.baudrate, init_timeout=float(args.init_timeout))
    init_result = json_rpc_client.init()
    if init_result is not None:
        print(f"init: {init_result}")

    # execute
    try:
        result = execute_method(json_rpc_client, Method(args.method))
        print(f"{args.method}: {result}")
        return 0
    except Exception as ex:
        print(f"failed to execute {args.method} method with: {str(ex)}")
        return 1


if __name__ == '__main__':
    sys.exit(cli())
