// TODO: https://docs.arduino.cc/learn/contributions/arduino-creating-library-guide/

#ifndef __eeprom_28c64_api_h__
#define __eeprom_28c64_api_h__

#define _EEPROM_DEBUG_LOGGING
#ifdef _EEPROM_DEBUG_LOGGING
#define _debugPrint(log) Serial.print(log)
#define _debugPrintln(log) Serial.println(log)
#else
#define _debugPrint(log)
#define _debugPrintln(log)
#endif  // _EEPROM_DEBUG_LOGGING

namespace EepromApiLibrary {

// 28C64 API

class Eeprom28C64Api {
public:
  Eeprom28C64Api(
    // address
    const uint8_t* addressPins,
    // data
    const uint8_t* dataPins,
    // control
    const uint8_t chipEnablePin,
    const uint8_t outputEnablePin,
    const uint8_t writeEnablePin,
    // status
    const uint8_t readyBusyOutputPin);

  void init();

  void readInit();
  uint8_t readData(const uint16_t address);

  void writeInit();
  void writeData(const uint16_t address, const uint8_t data);

  // bit operations
  // Most Significant Bit First ordering
  // { 0,0,0,0,0,0,0,1 } == 1
  // { 1,0,0,0,0,0,0,0 } == 128

  static void uint64ToBits(uint64_t value, bool* bitsArray, const int arraySize) {
    // reverse order
    for (int i = 0; i < arraySize; i++) {
      bitsArray[i] = bitRead(value, arraySize - 1 - i);
    }
  }

  static uint64_t bitsToUint64(bool* bitsArray, const int arraySize) {
    uint64_t value = 0;
    for (int i = 0; i < arraySize; i++) {
      value = (value << 1) | bitsArray[i];
    }
    return value;
  }

  static int addressBusSize() {
    return _EEPROM_28C64_ADDR_BUS_SIZE;
  }
  static int dataBusSize() {
    return _EEPROM_28C64_DATA_BUS_SIZE;
  }

private:
  static const int _EEPROM_28C64_ADDR_BUS_SIZE = 13;
  static const int _EEPROM_28C64_DATA_BUS_SIZE = 8;

  enum _DataPinsMode {
    DATA_PINS_READ,
    DATA_PINS_WRITE,
  };
  void _changeDataPinsMode(const _DataPinsMode mode);

  void _chipEnable(const bool ehable);
  void _outputEnable(const bool ehable);
  void _writeEnable(const bool ehable);

  // PINS
  // address
  uint8_t _addressPins[_EEPROM_28C64_ADDR_BUS_SIZE];
  // data
  uint8_t _dataPins[_EEPROM_28C64_DATA_BUS_SIZE];
  // control
  uint8_t _chipEnablePin;    // !CE
  uint8_t _outputEnablePin;  // !OE
  uint8_t _writeEnablePin;   // !WE
  // status
  uint8_t _readyBusyOutputPin;  // READY / !BUSY
};

Eeprom28C64Api::Eeprom28C64Api(
  // address
  const uint8_t* addressPins,
  // data
  const uint8_t* dataPins,
  // control
  const uint8_t chipEnablePin,
  const uint8_t outputEnablePin,
  const uint8_t writeEnablePin,
  // status
  const uint8_t readyBusyOutputPin) {
  // address
  memcpy(_addressPins, addressPins, _EEPROM_28C64_ADDR_BUS_SIZE);
  // data
  memcpy(_dataPins, dataPins, _EEPROM_28C64_DATA_BUS_SIZE);
  // control
  _chipEnablePin = chipEnablePin;
  _outputEnablePin = outputEnablePin;
  _writeEnablePin = writeEnablePin;
  // status
  _readyBusyOutputPin = readyBusyOutputPin;
}

void Eeprom28C64Api::_changeDataPinsMode(const Eeprom28C64Api::_DataPinsMode mode) {
  if (mode == Eeprom28C64Api::_DataPinsMode::DATA_PINS_READ) {
    for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
      pinMode(_dataPins[i], INPUT);
    }

  } else if (mode == Eeprom28C64Api::_DataPinsMode::DATA_PINS_WRITE) {
    for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
      pinMode(_dataPins[i], OUTPUT);
    }
  }
}

void Eeprom28C64Api::init() {
  // status pin
  pinMode(_readyBusyOutputPin, INPUT);

  // control pins
  pinMode(_chipEnablePin, OUTPUT);
  pinMode(_outputEnablePin, OUTPUT);
  pinMode(_writeEnablePin, OUTPUT);

  // address
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    pinMode(_addressPins[i], OUTPUT);
  }

  _changeDataPinsMode(_DataPinsMode::DATA_PINS_READ);

  // disable all control pins
  digitalWrite(_chipEnablePin, HIGH);
  digitalWrite(_outputEnablePin, HIGH);
  digitalWrite(_writeEnablePin, HIGH);

  // set address to 0
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], 0);
  }
}

void Eeprom28C64Api::readInit() {
  _chipEnable(false);
  _outputEnable(false);
  _writeEnable(false);
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_READ);
}

uint8_t Eeprom28C64Api::readData(const uint16_t address) {
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  uint64ToBits((uint64_t)address, bAddress, _EEPROM_28C64_ADDR_BUS_SIZE);

  // set address
  _debugPrint("(API) R [" + String(address) + "] | addr: b");
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], bAddress[i]);
    _debugPrint(bAddress[i]);
  }
  delay(1);

  // chip enable
  _chipEnable(true);
  delay(1);

  // output enable
  _outputEnable(true);
  delay(1);

  // wait for the data propagation
  delay(10);

  // read output by address
  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];
  _debugPrint("| data: b");
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    bData[i] = digitalRead(_dataPins[i]) ? 1 : 0;
    _debugPrint(bData[i]);
  }
  _debugPrintln();
  delay(1);

  // chip disable
  _chipEnable(false);
  delay(1);

  // output disable
  _outputEnable(false);
  delay(1);

  return (uint8_t)bitsToUint64(bData, _EEPROM_28C64_DATA_BUS_SIZE);
}

void Eeprom28C64Api::writeInit() {
  _chipEnable(false);
  _outputEnable(true);
  _writeEnable(false);
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_WRITE);
}

void Eeprom28C64Api::writeData(const uint16_t address, const uint8_t data) {
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  uint64ToBits((uint64_t)address, bAddress, _EEPROM_28C64_ADDR_BUS_SIZE);

  // convert data to bits
  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];
  uint64ToBits((uint64_t)data, bData, _EEPROM_28C64_DATA_BUS_SIZE);

  // output disable
  _outputEnable(false);
  delay(1);

  // set address
  _debugPrint("(API) W [" + String(address) + "] | addr: b");
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], bAddress[i]);
    _debugPrint(bAddress[i]);
  }
  delay(1);

  // chip enable
  _chipEnable(true);
  delay(1);

  // set data
  _debugPrint("| data: b");
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    digitalWrite(_dataPins[i], bData[i]);
    _debugPrint(bData[i]);
  }
  _debugPrintln();
  delay(1);

  // wrtie enable (initiates the write op)
  _writeEnable(true);
  delay(1);

  // wrtie disable
  _writeEnable(false);
  delay(1);

  // chip disable
  _chipEnable(false);
  delay(1);

  // output enable
  _outputEnable(true);
  delay(1);
}

void Eeprom28C64Api::_chipEnable(const bool enable) {
  if (enable) {
    digitalWrite(_chipEnablePin, LOW);
  } else {
    digitalWrite(_chipEnablePin, HIGH);
  }
}

void Eeprom28C64Api::_outputEnable(const bool enable) {
  if (enable) {
    digitalWrite(_outputEnablePin, LOW);
  } else {
    digitalWrite(_outputEnablePin, HIGH);
  }
}

void Eeprom28C64Api::_writeEnable(const bool enable) {
  if (enable) {
    digitalWrite(_writeEnablePin, LOW);
  } else {
    digitalWrite(_writeEnablePin, HIGH);
  }
}

}  // EepromApiLibrary

#endif  // !__eeprom_28c64_api_h__