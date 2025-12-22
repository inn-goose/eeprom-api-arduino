# EEPROM Programmer

> [!IMPORTANT]
> This project concerns **external** EEPROM chips, not the built-in Arduino EEPROM memory.

## TLDR

🚧 WIP 🚧


## Wiring Diagram

![Wiring Diagram: Arduino Mega and AT28C64](diagram/arduino-mega-at28c64.png)

## EEPROM Programmer on Arduino

set pins layout in `eeprom_wiring.h`

Programmer interface:
```cpp
// initialize chip pinout 
ErrorCode init_chip(const String& chip_type);

// set read mode
ErrorCode set_read_mode(const int page_size_bytes);

// read one page
ErrorCode read_page(const int page_no, uint8_t* bytes);

// set write mode
ErrorCode set_write_mode(const int page_size_bytes);

// write one page
ErrorCode write_page(const int page_no, const uint8_t* bytes);
```

### JSON-RPC API

Arduino IDE's *Serial Monitor* on `115200` baud

`init_chip(chip_type: str)`

```json
{"jsonrpc":"2.0", "id":0, "method": "init_chip", "params": ["AT28C64"]}
```

`set_read_mode(page_size_bytes: int)`

```json
{"jsonrpc":"2.0", "id":0, "method": "set_read_mode", "params": [4]}
```

`read_page(page_no: int)`

```json
{"jsonrpc":"2.0", "id":0, "method": "read_page", "params": [0]}
```

`set_write_mode(page_size_bytes: int)`

```json
{"jsonrpc":"2.0", "id":0, "method": "set_write_mode", "params": [4]}
```

`write_page(page_no: int, data: array[int])`

```json
{"jsonrpc":"2.0", "id":0, "method": "write_page","params": [0, [127, 127, 127, 127]]}
```

#### Write Operation Sequence

```json
{"jsonrpc":"2.0", "id":0, "method": "init_chip", "params": ["AT28C64"]}
{"jsonrpc":"2.0", "id":0, "method": "set_write_mode", "params": [4]}
{"jsonrpc":"2.0", "id":0, "method": "write_page","params": [0, [120, 130, 140, 150]]}
{"jsonrpc":"2.0", "id":0, "method": "write_page","params": [50, [20, 30, 40, 50]]}
{"jsonrpc":"2.0", "id":0, "method": "get_write_perf","params": []}
```

#### Read Operation Sequence

```json
{"jsonrpc":"2.0", "id":0, "method": "init_chip", "params": ["AT28C64"]}
{"jsonrpc":"2.0", "id":0, "method": "set_read_mode", "params": [4]}
{"jsonrpc":"2.0", "id":0, "method": "read_page", "params": [0]}
{"jsonrpc":"2.0", "id":0, "method": "read_page", "params": [50]}
```


## EEPROM Programmer python CLI

> [!CAUTION]
> During read operations with the EEPROM Programmer, the chip's `!WE` pin **MUST** be connected to `VCC` using a jumper wire to disable the write mode. Otherwise, invoking the CLI may corrupt data on the chip due to Arduino's internal behavior. [Details](https://goose.sh/blog/eeprom-programmer-5-data-corruption/)

Uses the [Serial JSON RPC](https://github.com/inn-goose/serial-json-rpc-arduino) interface.

### init CLI

```bash
pip3 install virtualenv

PATH=${PATH}:~/Library/Python/3.9/bin/ ./env/init.sh

source venv/bin/activate

deactivate
```

### Usage

```bash
source venv/bin/activate

python3 -m serial.tools.list_ports
...
/dev/cu.usbmodem2101
```

#### prepare data

```bash
mkdir ./tmp

# get a "real" EEPROM dump
curl -LJ --output tmp/zenith_zt1_eeprom.bin "https://github.com/misterblack1/zenith_zt1/raw/refs/heads/main/444-187%20U114%20ROM%202732.bin"
```

#### read

```bash
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH

# read data to file
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --read tmp/dump_eeprom.bin

# convert to HEX
xxd tmp/dump_eeprom.bin > tmp/dump_eeprom.hex
```

#### erase

```bash
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH

# erase with FF pattern
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --erase --erase-pattern FF
```

#### write

```bash
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH

# write data from file
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration.bin

# skip erase
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration.bin --skip-erase

# custom erase pattern
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration.bin --erase-pattern CC
```

#### verify

```bash
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH

# read data and verify it against the file
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --verify test_bin/64_the_red_migration_AT28C64_ff.bin
```


## XGecu Programmer as a Reference

Use the [`minipro`](https://formulae.brew.sh/formula/minipro) utility to perform read and write operations with the XGecu programmer

```bash
brew install minipro

# write the "real" dump to the chip
minipro --device AT28C64 -s -u --write test_bin/64_the_red_migration.bin
minipro --device AT28C256 -s -u --write test_bin/256_the_geometry_of_flight.bin

# read the data
minipro --device AT28C64 -u --read tmp/dump_xgecu.bin
minipro --device AT28C256 -u --read tmp/dump_xgecu.bin

# convert to HEX
xxd tmp/dump_xgecu.bin > tmp/dump_xgecu.hex

# compare
vimdiff tmp/dump_eeprom.hex tmp/dump_xgecu.hex
```



## Tests

### prepare

```bash
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH
```

### erase

```bash
# >> remove jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --erase --erase-pattern BB --collect-performance

# >> set jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --read tmp/dump_eeprom.bin

xxd tmp/dump_eeprom.bin | less
```

### write and read

```bash
# >> remove jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration.bin --erase-pattern BB --collect-performance

# >> set jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --read tmp/dump_eeprom.bin

xxd tmp/dump_eeprom.bin > tmp/dump_eeprom.hex
vimdiff tmp/dump_eeprom.hex tmp/64_the_red_migration.hex
```

### verify

```bash
# >> remove jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration.bin --collect-performance

# >> set jumper wire
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --verify test_bin/64_the_red_migration_AT28C64_ff.bin
```




## Performance

```
source venv/bin/activate
export PYTHONPATH=./eeprom_programmer_cli/:$PYTHONPATH

# AT28C04
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C04 --erase --erase-pattern 11 --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C04 --read tmp/dump_eeprom.bin --collect-performance && xxd tmp/dump_eeprom.bin | less
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C04 --write test_bin/4_journey_of_luna_AT28C04_ff.bin --skip-erase --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C04 --verify test_bin/4_journey_of_luna_AT28C04_ff.bin --collect-performance

# AT28C16
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C16 --erase --erase-pattern 11 --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C16 --read tmp/dump_eeprom.bin --collect-performance && xxd tmp/dump_eeprom.bin | less
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C16 --write test_bin/16_geese_in_space_AT28C16_ff.bin --skip-erase --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C16 --verify test_bin/16_geese_in_space_AT28C16_ff.bin --collect-performance

# AT28C64
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --erase --erase-pattern 11 --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --read tmp/dump_eeprom.bin --collect-performance && xxd tmp/dump_eeprom.bin | less
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --write test_bin/64_the_red_migration_AT28C64_ff.bin --skip-erase --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C64 --verify test_bin/64_the_red_migration_AT28C64_ff.bin --collect-performance

# AT28C256
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C256 --erase --erase-pattern 11 --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C256 --read tmp/dump_eeprom.bin --collect-performance && xxd tmp/dump_eeprom.bin | less
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C256 --write test_bin/256_the_geometry_of_flight_AT28C256_ff.bin --skip-erase --collect-performance
./eeprom_programmer_cli/cli.py /dev/cu.usbmodem2101 -p AT28C256 --verify test_bin/256_the_geometry_of_flight_AT28C256_ff.bin --collect-performance
```

| Chip | Size | Byte Read | Total Read | Byte Write | Write |
| -- | :--: | :--: | :--: | :--: | :--: |
| AT28C04 | 512 | 450 ms | 1.5 sec | 950 ms | 2 sec |
| AT28C16 | 2048 | 450 ms | 6 sec | 3800 ms | 14 sec |
| AT28C64 | 8192 | 400 ms | 20 sec | 4100 ms | 55 sec |
| AT28C256 | 32768 | 500 ms | 85 sec | 6500 ms | 300 sec |


## Datasheets

AT28C64
https://ww1.microchip.com/downloads/en/devicedoc/doc0001H.pdf

AT28C256
https://ww1.microchip.com/downloads/en/DeviceDoc/doc0006.pdf

AT28C04
https://www.tvsat.com.pl/pdf/A/at28c04_atm.pdf

AT28C16
http://cva.stanford.edu/classes/cs99s/datasheets/at28c16.pdf
