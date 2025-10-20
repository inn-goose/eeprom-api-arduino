#ifndef __main_loop_h__
#define __main_loop_h__

#include "lcd_2004_i2c.h"
#include "eeprom_wiring.h"
#include "eeprom_28c64_api.h"

using namespace Lcd2004I2C;
using namespace EepromApiLibrary;


// helper functions

String bitsToString(bool* bitsArray, const int arraySize) {
  String result = "";
  for (int i = 0; i < arraySize; i++) {
    result += bitsArray[i];
  }
  return result;
}

void setBuiltinLed(int led_color) {
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  if (led_color == LEDR || led_color == LEDG || led_color == LEDB) {
    digitalWrite(led_color, LOW);
  }
}


// main loop

// EEPROM

const String EEPROM_CHIP_NAME = "EEPROM 28C64";

Eeprom28C64Api eeprom28C64Api(
  // address
  EEPROM_ADDRESS_PINS,
  // data
  EEPROM_DATA_PINS,
  // management
  EEPROM_CHIP_ENABLE_PIN,
  EEPROM_OUTPUT_ENABLE_PIN,
  EEPROM_WRITE_ENABLE_PIN,
  // status
  EEPROM_READY_BUSY_OUTPUT_PIN
);


enum OperationType {
  OPERATION_PRE_READ,
  OPERATION_READ,
  OPERATION_POST_READ,
  OPERATION_PRE_WRITE,
  OPERATION_WRITE,
  OPERATION_POST_WRITE,
};


OperationType currentOperation = OperationType::OPERATION_PRE_READ;
bool initReadOperation = false;
bool initWriteOperation = false;
bool clearLcd = false;

// read operation global vars
const int READ_OPERATION_CYCLES_TOTAL = 10;
int readOperationCycle = 0;

// write operation global vars
const int WRITE_OPERATION_CYCLES_TOTAL = 10;
int writeOperationCycle = 0;


void initMainLoop() {
  // LCD
  lcdInit();
  lcdPrint(EEPROM_CHIP_NAME);

  // eeprom
  eeprom28C64Api.init();
}


void toggleNextOperation() {
  // on-button-press interrupt
  // DO NOT RUN ANY HEAVY OPERATIONS
  switch (currentOperation) {
    case OperationType::OPERATION_PRE_READ:
      // initiate read operation
      currentOperation = OperationType::OPERATION_READ;
      initReadOperation = true;
      clearLcd = true;
      break;
    case OperationType::OPERATION_READ:
      // do nothing during the read cycle
      break;
    case OperationType::OPERATION_POST_READ:
      currentOperation = OperationType::OPERATION_PRE_WRITE;
      clearLcd = true;
      break;
    case OperationType::OPERATION_PRE_WRITE:
      // initiate write operation
      currentOperation = OperationType::OPERATION_WRITE;
      initWriteOperation = true;
      clearLcd = true;
      break;
    case OperationType::OPERATION_WRITE:
      // do nothing during the write cycle
      break;
    case OperationType::OPERATION_POST_WRITE:
      currentOperation = OperationType::OPERATION_PRE_READ;
      clearLcd = true;
      break;
    default:
      currentOperation = OperationType::OPERATION_PRE_READ;
      break;
  }
}


String getAddressStr(const uint64_t address) {
  static const int arraySize = Eeprom28C64Api::addressBusSize();
  bool bAddress[arraySize];
  Eeprom28C64Api::uint64ToBits((uint64_t)address, bAddress, arraySize);
  return "addr: b" + bitsToString(bAddress, arraySize);
}

String getDataStr(const uint64_t data) {
  static const int arraySize = Eeprom28C64Api::dataBusSize();
  bool bData[arraySize];
  Eeprom28C64Api::uint64ToBits((uint64_t)data, bData, arraySize);
  return "data: b" + bitsToString(bData, arraySize);
}


void processReadOperationCycle() {
  // read
  uint16_t address = (uint16_t)readOperationCycle;
  uint8_t data = eeprom28C64Api.readData(address);

  // update LCD
  String operationStr = "op: read " + String(readOperationCycle) + "/" + String(READ_OPERATION_CYCLES_TOTAL);
  lcdPrint(EEPROM_CHIP_NAME, operationStr, getAddressStr(address), getDataStr(data));

  // iterate through the cycles
  readOperationCycle += 1;
  // finish the cycle
  if (readOperationCycle > READ_OPERATION_CYCLES_TOTAL) {
    currentOperation = OperationType::OPERATION_POST_READ;
    clearLcd = true;
    setBuiltinLed(-1);
  }
}


void processWriteOperationCycle() {
  // write
  uint16_t address = (uint16_t)writeOperationCycle;
  uint8_t data = (uint8_t)writeOperationCycle;
  eeprom28C64Api.writeData(address, data);

  // update LCD
  String operationStr = "op: write " + String(writeOperationCycle) + "/" + String(WRITE_OPERATION_CYCLES_TOTAL);
  lcdPrint(EEPROM_CHIP_NAME, operationStr, getAddressStr(address), getDataStr(data));

  // iterate through the cycles
  writeOperationCycle += 1;
  // finish the cycle
  if (writeOperationCycle > WRITE_OPERATION_CYCLES_TOTAL) {
    currentOperation = OperationType::OPERATION_POST_WRITE;
    clearLcd = true;
    setBuiltinLed(-1);
  }
}


void mainLoop() {
  // init operations

  if (initReadOperation) {
    initReadOperation = false;
    readOperationCycle = 0;
    eeprom28C64Api.readInit();
    setBuiltinLed(LEDG);
  }

  if (initWriteOperation) {
    initWriteOperation = false;
    writeOperationCycle = 0;
    eeprom28C64Api.writeInit();
    setBuiltinLed(LEDR);
  }

  if (clearLcd) {
    clearLcd = false;
    lcdClear();
  }

  // main loop

  switch (currentOperation) {
    case OperationType::OPERATION_PRE_READ:
      lcdPrint(EEPROM_CHIP_NAME, "op: pre-read", "waiting for read", "press the button");
      break;
    case OperationType::OPERATION_READ:
      processReadOperationCycle();
      break;
    case OperationType::OPERATION_POST_READ:
      lcdPrint(EEPROM_CHIP_NAME, "op: post-read", "reading is done", "press the button");
      break;
    case OperationType::OPERATION_PRE_WRITE:
      lcdPrint(EEPROM_CHIP_NAME, "op: pre-write", "waiting for write", "press the button");
      break;
    case OperationType::OPERATION_WRITE:
      processWriteOperationCycle();
      break;
    case OperationType::OPERATION_POST_WRITE:
      lcdPrint(EEPROM_CHIP_NAME, "op: post-write", "writing is done", "press the button");
      break;
  }
}

#endif  // !__main_loop_h__
