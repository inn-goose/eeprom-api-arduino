# EEPROM API for Arduino

## TLDR

🚧 WIP 🚧

[EEPROM Read and Write Operations with Arduino](https://goose.sh/blog/eeprom-read-and-write-operations/)

[EEPROM API Performance with Arduino](https://goose.sh/blog/eeprom-api-performance/)

[Debugging the EEPROM API](https://goose.sh/blog/debugging-eeprom-api/)


## EEPROM API Board

set pins layout in `eeprom_wiring.h`

API interface:
```cpp
// initializes the pinout 
void init_read(String &eeprom_type);

// reads one page of data, returns raw bytes
byte[] read_page(int page_size_bytes, int page_no);
```


## EEPROM API python CLI

Uses the [Serial JSON RPC](https://github.com/inn-goose/serial-json-rpc-arduino) interface.

### init

```commandline
pip3 install virtualenv

cd eeprom_api_py_cli/

PATH=${PATH}:~/Library/Python/3.9/bin/ ./sh/init.sh

source venv/bin/activate

deactivate
```

### usage

```commandline
python -m serial.tools.list_ports
...
/dev/cu.usbmodem2101

PYTHONPATH=./:$PYTHONPATH python3 ./cli.py /dev/cu.usbmodem2101 read
```
