#!/usr/bin/env python3

import argparse
import sys
import time

from core.eeprom_programmer_client import EepromProgrammerClient


class CliError(Exception):
    pass


def connect_programmer(port: str, baudrate: int, init_timeout: int):
    print(f"connect programmer: {port}")

    ts = time.time()

    programmer = EepromProgrammerClient(
        port=port, baudrate=baudrate, init_timeout=float(init_timeout))

    elapsed = time.time() - ts
    print(f"connect programmer: DONE, {elapsed:.02f} sec")

    return programmer


def init_device(programmer: EepromProgrammerClient, device: str):
    print(f"init device: {device}")

    try:
        programmer.init_chip(device)
    except Exception as ex:
        raise CliError(f"init device: failed, {str(ex)}")

    print(f"init device: DONE")


def read(programmer: EepromProgrammerClient, filename: str, collect_performance: bool):
    if not filename:
        raise CliError("filename is empty")

    print(f"read operation: {filename}")

    ts = time.time()

    try:
        output_data = programmer.read_data(collect_performance)
    except Exception as ex:
        raise CliError(f"read operation: failed, {str(ex)}")

    with open(filename, "wb") as f:
        f.write(output_data)

    elapsed = time.time() - ts
    print(f"read operation: DONE, {elapsed:.02f} sec")


def write(programmer: EepromProgrammerClient, filename: str, erase_pattern_str: str, skip_erase: bool,
          skip_verify: bool, collect_performance: bool):
    print(f"write operation: {filename}")

    if not filename:
        raise CliError("filename is empty")

    input_data = None
    with open(filename, "rb") as f:
        input_data = bytes(f.read())
    if not input_data:
        raise CliError("data source is empty")

    memory_size = programmer.chip_settings["memory_size"]
    if len(input_data) > memory_size:
        raise CliError("data size is bigger than the chip memory size")
    if len(input_data) < memory_size:
        print(
            f"WARNING incorrect data size: {len(input_data)} / chip memory size is {memory_size}")

    # resolve erase pattern for padding
    erase_pattern = 0xFF
    if erase_pattern_str:
        erase_pattern = int(erase_pattern_str, 16)

    if not skip_erase:
        erase(programmer, erase_pattern_str, collect_performance)

    print("write operation: started")

    ts = time.time()

    try:
        programmer.write_data(input_data, collect_performance)
    except Exception as ex:
        raise CliError(f"write operation: failed, {str(ex)}")

    elapsed = time.time() - ts
    print(f"write operation: DONE, {elapsed:.02f} sec")

    if not skip_verify:
        # pad input data to full chip size with erase pattern
        if len(input_data) < memory_size:
            expected_data = input_data + bytes([erase_pattern] * (memory_size - len(input_data)))
        else:
            expected_data = input_data
        verify_data(programmer, expected_data, collect_performance)


def verify_data(programmer: EepromProgrammerClient, expected_data: bytes, collect_performance: bool):
    """Read chip contents and compare against expected data (in-session, no reconnect)."""
    print("verify operation: started")

    ts = time.time()

    try:
        actual_data = programmer.read_data(collect_performance)
    except Exception as ex:
        raise CliError(f"verify operation: read failed, {str(ex)}")

    if expected_data != actual_data:
        # find first mismatch for diagnostics
        for i in range(min(len(expected_data), len(actual_data))):
            if expected_data[i] != actual_data[i]:
                print(f"verify operation: first mismatch at address 0x{i:04X} "
                      f"(expected 0x{expected_data[i]:02X}, got 0x{actual_data[i]:02X})")
                break
        raise CliError("verify operation: failed, mismatched data")

    elapsed = time.time() - ts
    print(f"verify operation: DONE, {elapsed:.02f} sec")


def verify(programmer: EepromProgrammerClient, filename: str, collect_performance: bool):
    print(f"verify operation: {filename}")

    if not filename:
        raise CliError("filename is empty")

    input_data = None
    with open(filename, "rb") as f:
        input_data = bytes(f.read())
    if not input_data:
        raise CliError("data source is empty")

    memory_size = programmer.chip_settings["memory_size"]
    if len(input_data) > memory_size:
        raise CliError("data size is bigger than the chip memory size")
    if len(input_data) < memory_size:
        raise CliError(
            f"incorrect data size: {len(input_data)} / chip memory size is {memory_size}")

    verify_data(programmer, input_data, collect_performance)


def erase(programmer: EepromProgrammerClient, erase_pattern_str: str, collect_performance: bool):
    print("erase operation")

    erase_pattern = 255  # FF
    if erase_pattern_str:
        try:
            erase_pattern = int(erase_pattern_str, 16)
            if erase_pattern < 0 or erase_pattern > 255:
                raise CliError(
                    f"invalid erase pattern {erase_pattern_str}, should be within [0, 255] range")
        except Exception:
            raise CliError(
                f"invalid erase pattern {erase_pattern_str}, should be a HEX value")

    print(f"erase pattern: 0x{erase_pattern:02X}")

    ts = time.time()

    try:
        programmer.erase_data(erase_pattern, collect_performance)
    except Exception as ex:
        raise CliError(f"erase operation: failed, {str(ex)}")

    elapsed = time.time() - ts
    print(f"erase operation: DONE, {elapsed:.02f} sec")


def cli() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port", type=str, metavar="<port>",
                        help="Specify the USP port address for the serial connection")
    parser.add_argument("--baudrate", type=int, default=115200,
                        metavar="<baud>", help="Set the serial connection speed")
    parser.add_argument("--init-timeout", type=int, default=3, metavar="<sec>",
                        help="Set the MAX Arduino's reset-on-connect timeout in seconds")
    parser.add_argument("-l", "--list", action="store_true",
                        help="List all supported devices")
    parser.add_argument("-p", "--device", type=str, required=False,
                        metavar="<device>", help="Specify the device type, like AT28C64")
    parser.add_argument("-r", "--read", type=str, required=False, metavar="<filename>",
                        help="Read from the device and write the contents to this file")
    parser.add_argument("-m", "--verify", type=str, required=False,
                        metavar="<filename>", help="Verify memory in the device against this file")
    parser.add_argument("-w", "--write", type=str, required=False,
                        metavar="<filename>", help="Write to the device using this file")
    parser.add_argument("-e", "--skip-erase",
                        action="store_true", help="Do NOT erase the device")
    parser.add_argument("-v", "--skip-verify",
                        action="store_true", help="Do NOT verify after write")
    parser.add_argument("-E", "--erase", action="store_true",
                        help="Just erase the device")
    parser.add_argument("--erase-pattern", type=str, required=False, metavar="<hex>",
                        help="Specify the erase pattern, like CC or AA, default: FF")
    parser.add_argument("--collect-performance", action="store_true",
                        help="Collect the operation performance")
    args = parser.parse_args()

    if args.list:
        raise CliError("LIST operation is not supported")

    if not args.device:
        raise CliError("-p/--device argument is required")

    # connect
    programmer = connect_programmer(
        args.port, args.baudrate, args.init_timeout)

    # init chip
    init_device(programmer, args.device)

    if args.read is not None:
        read(programmer, args.read, args.collect_performance)

    elif args.write is not None:
        write(programmer, args.write, args.erase_pattern, args.skip_erase,
              args.skip_verify, args.collect_performance)

    elif args.verify is not None:
        verify(programmer, args.verify, args.collect_performance)

    elif args.erase:
        erase(programmer, args.erase_pattern, args.collect_performance)

    else:
        raise CliError("unknown operation")


if __name__ == '__main__':
    try:
        cli()
    except Exception as ex:
        print(ex)
        sys.exit(1)
