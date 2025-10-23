import argparse
import sys

from core import eeprom_api_client
from serial_json_rpc import client


def cli() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port", type=str)
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--init-timeout", type=int, default=3)
    parser.add_argument("-p", "--device", type=str, required=True)
    parser.add_argument("-r", "--read", type=str, required=False)
    parser.add_argument("-w", "--write", type=str, required=False)
    args = parser.parse_args()

    # init
    json_rpc_client = client.SerialJsonRpcClient(
        port=args.port, baudrate=args.baudrate, init_timeout=float(args.init_timeout))
    init_result = json_rpc_client.init()
    if init_result is not None:
        print(f"init: {init_result}")

    if args.read is not None:
        try:
            return eeprom_api_client.read_data(json_rpc_client, args.device, args.read)
        except Exception as ex:
            print(f"failed to read data with: {str(ex)}")
            return 1
    elif args.write is not None:
        try:
            return eeprom_api_client.read_data(json_rpc_client, args.device, args.write)
        except Exception as ex:
            print(f"failed to write data with: {str(ex)}")
            return 1
    else:
        print("unknown mode")


if __name__ == '__main__':
    sys.exit(cli())
