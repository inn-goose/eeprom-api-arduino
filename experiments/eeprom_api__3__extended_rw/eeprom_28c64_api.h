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

  static String addressBin(const uint16_t address) {
    bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
    _addressToBitsArray(address, bAddress);
    String result = "";
    for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
      // print in reverse order, since the printed A0 should be the last bit
      result += bAddress[_EEPROM_28C64_ADDR_BUS_SIZE - 1 - i] ? 1 : 0;
    }
    return result;
  }

  static String addressHex(const uint16_t address) {
    char buf[8];
    sprintf(buf, "%08x", address);
    return String(buf);
  }

  static String dataHex(const uint8_t data) {
    char buf[2];
    sprintf(buf, "%02x", data);
    return String(buf);
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

  // bit operations
  // Most Significant Bit First ordering
  // { 0,0,0,0,0,0,0,1 } == 1
  // { 1,0,0,0,0,0,0,0 } == 128

  static void _addressToBitsArray(uint16_t address, bool* bitsArray) {
    // Ensure address is within 13-bit range (0 to 8191)
    address &= 0x1FFF;
    for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; ++i) {
      // MSB order
      // bitsArray[_EEPROM_28C64_ADDR_BUS_SIZE - 1 - i] = (address >> i) & 1;
      // LSB order
      bitsArray[i] = (address >> i) & 1;
    }
  }

  static void _dataToBitsArray(uint8_t data, bool* bitsArray) {
    // MSB order
    for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
      bitsArray[_EEPROM_28C64_DATA_BUS_SIZE - 1 - i] = bitRead(data, i);
    }
  }

  static uint8_t _bitsArrayToData(bool* bitsArray) {
    // MSB order
    uint8_t data = 0;
    for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
      data = (data << 1) | bitsArray[_EEPROM_28C64_DATA_BUS_SIZE - 1 - i];
    }
    return data;
  }
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
  // set control pins
  pinMode(_chipEnablePin, OUTPUT);
  pinMode(_outputEnablePin, OUTPUT);
  pinMode(_writeEnablePin, OUTPUT);
  // disable all control pins
  digitalWrite(_chipEnablePin, HIGH);
  digitalWrite(_outputEnablePin, HIGH);
  digitalWrite(_writeEnablePin, HIGH);

  // address
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    pinMode(_addressPins[i], OUTPUT);
  }
  // reset address
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], 0);
  }

  _changeDataPinsMode(_DataPinsMode::DATA_PINS_READ);

  // status pin / open drain
  pinMode(_readyBusyOutputPin, OUTPUT);
  digitalWrite(_readyBusyOutputPin, LOW);
}

void Eeprom28C64Api::readInit() {
  _readState = true;
  // initial READ waveforms state
  digitalWrite(_chipEnablePin, HIGH);    // off
  digitalWrite(_outputEnablePin, HIGH);  // off
  digitalWrite(_writeEnablePin, HIGH);   // not in use
  // switch data pins to READ mode
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_READ);

  // status pin / NC
  pinMode(_readyBusyOutputPin, OUTPUT);
  digitalWrite(_readyBusyOutputPin, LOW);
}

uint8_t Eeprom28C64Api::readData(const uint16_t address) {
  if (!_readState) {
    return 0;
  }

  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];

  // (0) prepare inputs
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  _addressToBitsArray(address, bAddress);

  // (1) set address
  _debugPrint("(API) R [" + String(address) + "] | addr[LSB]: b");
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], bAddress[i]);
    _debugPrint(bAddress[i]);
  }

  // (2) chip enable
  digitalWrite(_chipEnablePin, LOW);

  // (3) output enable
  digitalWrite(_outputEnablePin, LOW);

  // (4) !OE to Output Delay (delta between OE and data ready) == 100 ns MAX
  delayMicroseconds(1);  // arduino cannot delay in ns, only us

  // (5) read data
  _debugPrint(" | data[LSB]: b");
  for (int i = 0; i < _EEPROM_28C64_DATA_BUS_SIZE; i++) {
    bData[i] = digitalRead(_dataPins[i]) == HIGH ? 1 : 0;
    _debugPrint(bData[i]);
  }
  _debugPrintln();

  // if (address == 3968) delay(1000000);

  // (6) output disable
  digitalWrite(_outputEnablePin, HIGH);

  // (7) chip disable
  digitalWrite(_chipEnablePin, HIGH);

  // (8) reset address
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], 0);
  }

  return _bitsArrayToData(bData);
}

void Eeprom28C64Api::writeInit() {
  _writeState = true;

  // initial WRITE waveforms state (!WE controlled)
  digitalWrite(_chipEnablePin, HIGH);    // off
  digitalWrite(_outputEnablePin, HIGH);  // not in use
  digitalWrite(_writeEnablePin, HIGH);   // off

  // switch data pins to WRITE mode
  _changeDataPinsMode(_DataPinsMode::DATA_PINS_WRITE);

  // status pin / read status / open drain
  pinMode(_readyBusyOutputPin, INPUT_PULLUP);
}

void Eeprom28C64Api::writeData(const uint16_t address, const uint8_t data) {
  if (!_writeState) {
    return;
  }

  // (0) prepare inputs
  // convert address to bits
  bool bAddress[_EEPROM_28C64_ADDR_BUS_SIZE];
  _addressToBitsArray(address, bAddress);
  // convert data to bits
  bool bData[_EEPROM_28C64_DATA_BUS_SIZE];
  _dataToBitsArray(data, bData);

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
  int currBusyState = digitalRead(_readyBusyOutputPin);

  int busyStateStart = micros();

  // wait until !BUSY state switches to READY state (1 ms MAX)
  // or just wait for the Write Cycle Time MAX
  static const int totalDelayMsec = 1.4 * 1000;  // 1 ms is not enough
  if (currBusyState == LOW) {
    // device is in !BUSY state
    // use the READY/!BUSY pin status to wait for the Write Cycle End
    int prevBusyState = currBusyState;
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

  // (8) reset address
  for (int i = 0; i < _EEPROM_28C64_ADDR_BUS_SIZE; i++) {
    digitalWrite(_addressPins[i], 0);
  }

  _busyStateUsec = micros() - busyStateStart;
}

}  // EepromApiLibrary

#endif  // !__eeprom_28c64_api_h__