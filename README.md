# EEPROM API for Arduino

## TLDR

🚧 WIP 🚧

[EEPROM Read and Write Operations with Arduino](https://goose.sh/blog/eeprom-read-and-write-operations/)

[EEPROM API Performance with Arduino](https://goose.sh/blog/eeprom-api-performance/)

[Debugging the EEPROM API](https://goose.sh/blog/debugging-eeprom-api/)


## EEPROM API Board

set pins layout in `eeprom_wiring.h`


## EEPROM API python CLI

Uses the [Serial JSON RPC](https://github.com/inn-goose/serial-json-rpc-arduino) interface.

### init

```
pip3 install virtualenv

cd eeprom_api_py_cli/

PATH=${PATH}:~/Library/Python/3.9/bin/ ./sh/init.sh

source venv/bin/activate

deactivate
```

### usage

```
python -m serial.tools.list_ports
...
/dev/cu.usbmodem2101

PYTHONPATH=./:$PYTHONPATH python3 ./cli.py /dev/cu.usbmodem2101 led_on

PYTHONPATH=./:$PYTHONPATH python3 ./cli.py /dev/cu.usbmodem2101 led_off
```
