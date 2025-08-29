#include "eeprom_wiring.h"
#include "eeprom_28c64_api.h"

using namespace EepromApiLibrary;


// helper functions

void setBuiltinLed(int led_color) {
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  if (led_color == LEDR || led_color == LEDG || led_color == LEDB) {
    digitalWrite(led_color, LOW);
  }
}


String bitsToString(bool* bitsArray, const int arraySize) {
  String result = "";
  for (int i = 0; i < arraySize; i++) {
    result += bitsArray[i];
  }
  return result;
}

String getAddressStr(const uint64_t address) {
  static const int arraySize = Eeprom28C64Api::addressBusSize();
  bool bAddress[arraySize];
  Eeprom28C64Api::uint64ToBits((uint64_t)address, bAddress, arraySize);
  return "b" + bitsToString(bAddress, arraySize);
}

String getDataStr(const uint64_t data) {
  static const int arraySize = Eeprom28C64Api::dataBusSize();
  bool bData[arraySize];
  Eeprom28C64Api::uint64ToBits((uint64_t)data, bData, arraySize);
  return "b" + bitsToString(bData, arraySize);
}


// EEPROM 28C64

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
  EEPROM_READY_BUSY_OUTPUT_PIN);


void setup() {
  Serial.begin(57600);
  // eeprom
  eeprom28C64Api.init();
}


// execute the loop code only once
bool executed_once = false;

const int ADDRESS_SPACE_SIZE = 8192;
const int USABLE_ADDRESS_OFFSET = 0;
const int USABLE_ADDRESS_SPACE_SIZE = 8;
uint8_t writeValues[ADDRESS_SPACE_SIZE];
uint8_t readValues[ADDRESS_SPACE_SIZE];
bool damagedCells[ADDRESS_SPACE_SIZE];
int damagedCellsTotal = 0;

void loop() {
  if (executed_once) {
    return;
  }
  executed_once = true;

  delay(1000);

  // WARMUP READ
  setBuiltinLed(LEDB);
  for (int i = 0; i < USABLE_ADDRESS_SPACE_SIZE; i++) {
    uint16_t address = (uint16_t)(USABLE_ADDRESS_OFFSET + i);
    uint8_t data = eeprom28C64Api.readData(address);
    Serial.println("R1 [" + String(address) + "] addr: " + getAddressStr(address) + " | data: " + getDataStr(data));
  }
  setBuiltinLed(-1);

  // WRITE
  setBuiltinLed(LEDR);
  int write_total_micros = 0;
  for (int i = 0; i < USABLE_ADDRESS_SPACE_SIZE; i++) {
    uint16_t address = (uint16_t)(USABLE_ADDRESS_OFFSET + i);
    // uint8_t data = (uint8_t)random(1, 255);
    uint8_t data = (uint8_t)i;
    writeValues[i] = data;
    int write_start = micros();
    eeprom28C64Api.writeData(address, data);
    write_total_micros += micros() - write_start;
    Serial.println("W [" + String(address) + "] addr: " + getAddressStr(address) + " | data: " + getDataStr(data));
  }
  setBuiltinLed(-1);

  // READ

  setBuiltinLed(LEDB);
  int read_total_micros = 0;
  for (int i = 0; i < USABLE_ADDRESS_SPACE_SIZE; i++) {
    uint16_t address = (uint16_t)(USABLE_ADDRESS_OFFSET + i);
    int read_start = micros();
    uint8_t data = eeprom28C64Api.readData(address);
    read_total_micros += micros() - read_start;
    readValues[i] = data;
    Serial.println("R2 [" + String(address) + "] addr: " + getAddressStr(address) + " | data: " + getDataStr(data));
  }
  setBuiltinLed(-1);

  // check damaged cells
  for (int i = 0; i < USABLE_ADDRESS_SPACE_SIZE; i++) {
    if (writeValues[i] != readValues[i]) {
      damagedCells[i] = true;
      damagedCellsTotal += 1;
      // Serial.println(getAddressStr(i) + " | W: " + getDataStr(writeValues[i]) + " | R: " + getDataStr(readValues[i]));
    }
    Serial.println(getAddressStr(i) + " | W: " + getDataStr(writeValues[i]) + " | R: " + getDataStr(readValues[i]));
  }

  Serial.println("W | T: " + String(write_total_micros) + " | C: " + String(write_total_micros / USABLE_ADDRESS_SPACE_SIZE));
  Serial.println("R | T: " + String(read_total_micros) + " | C: " + String(read_total_micros / USABLE_ADDRESS_SPACE_SIZE));
  if (damagedCellsTotal) {
    Serial.println("damagedCellsTotal: " + String(damagedCellsTotal));
  }
}
