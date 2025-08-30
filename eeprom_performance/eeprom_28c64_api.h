// TODO: https://docs.arduino.cc/learn/contributions/arduino-creating-library-guide/

#ifndef __eeprom_28c64_api_h__
#define __eeprom_28c64_api_h__

// #define _EEPROM_DEBUG_LOGGING
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

  int busyStateUsec() {
    return _busyStateUsec;
  }

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

  // inner state
  bool _readState;
  bool _writeState;
  int _busyStateUsec;
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

  // inner state
  _readState = false;
  _writeState = false;
  _busyStateUsec = 0;
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
  // status pin / open drain
  pinMode(_readyBusyOutputPin, INPUT_PULLUP);

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
  _readState = true;
  // initial READ waveforms state
  digitalWrite(_chipEnablePin, HIGH);    // off
  digitalWrite(_outputEnablePin, HIGH);  // off
  digitalWrite(_writeEnablePin, HIGH);   // not in use
  // switch data pins to READ mode
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_READ);
}

uint8_t Eeprom28C64Api::readData(const uint16_t address) {
  if (!_readState) {
    return 0;
  }

  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];

  // (0) prepare inputs
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  uint64ToBits((uint64_t)address, bAddress, _EEPROM_28C64_ADDR_BUS_SIZE);

  // (1) set address
  _debugPrint("(API) R [" + String(address) + "] | addr: b");
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], bAddress[i]);
    _debugPrint(bAddress[i]);
  }

  // (2) chip enable
  digitalWrite(_chipEnablePin, LOW);

  // (3) output enable
  digitalWrite(_outputEnablePin, LOW);

  // (4) read data
  _debugPrint(" | data: b");
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    bData[i] = digitalRead(_dataPins[i]) ? 1 : 0;
    _debugPrint(bData[i]);
  }
  _debugPrintln();

  // (5) output disable
  digitalWrite(_outputEnablePin, HIGH);

  // (6) chip disable
  digitalWrite(_chipEnablePin, HIGH);

  return (uint8_t)bitsToUint64(bData, _EEPROM_28C64_DATA_BUS_SIZE);
}

void Eeprom28C64Api::writeInit() {
  _writeState = true;
  // initial WRITE waveforms state (!WE controlled)
  digitalWrite(_chipEnablePin, HIGH);    // off
  digitalWrite(_outputEnablePin, HIGH);  // not in use
  digitalWrite(_writeEnablePin, HIGH);   // off
  // switch data pins to WRITE mode
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_WRITE);
}

void Eeprom28C64Api::writeData(const uint16_t address, const uint8_t data) {
  if (!_writeState) {
    return;
  }

  // (0) prepare inputs
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  uint64ToBits((uint64_t)address, bAddress, _EEPROM_28C64_ADDR_BUS_SIZE);
  // convert data to bits
  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];
  uint64ToBits((uint64_t)data, bData, _EEPROM_28C64_DATA_BUS_SIZE);

  // (1) set address
  _debugPrint("(API) W [" + String(address) + "] | addr: b");
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], bAddress[i]);
    _debugPrint(bAddress[i]);
  }

  // (2) chip enable
  digitalWrite(_chipEnablePin, LOW);

  // (3) wrtie enable
  digitalWrite(_writeEnablePin, LOW);

  // (4) write data
  _debugPrint(" | data: b");
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    digitalWrite(_dataPins[i], bData[i]);
    _debugPrint(bData[i]);
  }
  _debugPrintln();

  // (5) wrtie disable (initiates the data flush)
  digitalWrite(_writeEnablePin, HIGH);

  // (6) chip disable
  digitalWrite(_chipEnablePin, HIGH);

  // (7) wait until device switches to !BUSY state
  // Time to Device Busy (delta between WE and !BUSY) == 50 ms MAX (spec)
  delayMicroseconds(1);  // arduino cannot delay in ns, only us
  PinStatus currBusyState = digitalRead(_readyBusyOutputPin);

  int busyStateStart = micros();

  // wait until !BUSY state switches to READY state (1 ms MAX)
  // or just wait for the Write Cycle Time MAX
  static const int totalDelayMsec = 1.4 * 1000;  // 1 ms is not enough
  if (currBusyState == LOW) {
    // device is in !BUSY state
    // use the READY/!BUSY pin status to wait for the Write Cycle End
    PinStatus prevBusyState = currBusyState;
    static const int attemptDelayMsec = 200;
    for (int i = 0; i < totalDelayMsec / attemptDelayMsec; i++) {
      delayMicroseconds(attemptDelayMsec);
      prevBusyState = currBusyState;
      currBusyState = digitalRead(_readyBusyOutputPin);
      if (prevBusyState == LOW && currBusyState == HIGH) {  // rising edge
        break;
      }
    }
  } else {
    // device not in !BUSY state
    // use generic delay
    delayMicroseconds(totalDelayMsec);
  }

  _busyStateUsec = micros() - busyStateStart;
}

}  // EepromApiLibrary

#endif  // !__eeprom_28c64_api_h__