from enum import Enum

import argparse
import sys

from serial_json_rpc import client


class Method(Enum):
    READ = "read"

    def __str__(self):
        return self.value


def execute_method(json_rpc_client: client.SerialJsonRpcClient, method: str) -> str:
    if method == Method.READ:
        resp = json_rpc_client.send_request("init_read", ["AT28C64"])
        print("init_read:", resp)

        for i in range(4):
            resp = json_rpc_client.send_request("read_page", [16, i])
            print(f"read_page [{i}]: {resp}")

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
