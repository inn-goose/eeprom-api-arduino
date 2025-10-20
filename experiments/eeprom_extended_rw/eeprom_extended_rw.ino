#include "eeprom_wiring.h"
#include "eeprom_28c64_api.h"

using namespace EepromApiLibrary;


// helper functions

enum LedColor { NO_LED_,
                LED_RED_,
                LED_GREEN_,
                LED_BLUE_ };

void setBuiltinLed(LedColor led_color) {
#ifdef LEDR
  // reverse logic. HIGH == off, LOW == on
  switch (led_color) {
    case LedColor::LED_RED_:
      digitalWrite(LEDR, LOW);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      break;
    case LedColor::LED_GREEN_:
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, LOW);
      digitalWrite(LEDB, HIGH);
      break;
    case LedColor::LED_BLUE_:
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, LOW);
      break;
    case LedColor::NO_LED_:
    default:
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      break;
  }
#else
  switch (led_color) {
    case LedColor::LED_RED_:
    case LedColor::LED_GREEN_:
    case LedColor::LED_BLUE_:
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    default:
      digitalWrite(LED_BUILTIN, LOW);
      break;
  }
#endif
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
  Serial.begin(115200);
  // Serial.begin(9600);
  // LED
#ifndef LEDR
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
  // eeprom
  eeprom28C64Api.init();
  // NC to 0
  pinMode(24, OUTPUT);
  digitalWrite(24, LOW);
}


const int ADDRESS_SPACE_SIZE = 8192;
const int USABLE_ADDRESS_OFFSET = 0;
const int USABLE_ADDRESS_SPACE_SIZE = 8192;
const int BUFFER_SIZE = 16;
// uint8_t writeValues[ADDRESS_SPACE_SIZE];
// int busyStateUsec[ADDRESS_SPACE_SIZE];
// uint8_t readValues[ADDRESS_SPACE_SIZE];
uint8_t readBuffer[BUFFER_SIZE];
// bool damagedCells[ADDRESS_SPACE_SIZE];
// int damagedCellsTotal = 0;

bool op_init = true;
// int op_write = -1;
int op_read = -1;
bool op_verify = false;

// int write_total_micros = 0;
int read_total_micros = 0;

void loop() {
  if (op_init) {
    delay(1000);
    op_init = false;
    op_read = 0;
  }

  // if (op_write >= 0) {
  //   // init
  //   if (op_write == 0) {
  //     setBuiltinLed(LedColor::LED_RED_);
  //     eeprom28C64Api.writeInit();
  //   }

  //   uint16_t address = (uint16_t)(USABLE_ADDRESS_OFFSET + op_write);
  //   uint8_t data = (uint8_t)random(1, 255);
  //   // uint8_t data = (uint8_t)op_write;

  //   writeValues[USABLE_ADDRESS_OFFSET + op_write] = data;

  //   int write_start = micros();
  //   eeprom28C64Api.writeData(address, data);
  //   write_total_micros += micros() - write_start;

  //   busyStateUsec[USABLE_ADDRESS_OFFSET + op_write] = eeprom28C64Api.busyStateUsec();

  //   Serial.println("W [" + String(address) + "] addr: " + getAddressStrBin(address) + " | data: " + getDataStrHex(data));

  //   op_write += 1;
  //   // deinit
  //   if (op_write > USABLE_ADDRESS_SPACE_SIZE) {
  //     setBuiltinLed(LedColor::NO_LED_);
  //     op_write = -1;
  //     op_read = 0;
  //     Serial.flush();
  //     delay(500);
  //     return;
  //   }
  // }

  if (op_read >= 0) {
    // init
    if (op_read == 0) {
      setBuiltinLed(LedColor::LED_BLUE_);
      eeprom28C64Api.readInit();
    }

    uint16_t address = (uint16_t)(USABLE_ADDRESS_OFFSET + op_read);

    int read_start = micros();
    uint8_t data = eeprom28C64Api.readData(address);
    read_total_micros += micros() - read_start;
    // readValues[address] = data;

    // Serial.println("R [" + String(address) + "] addr: " + Eeprom28C64Api::addressHex(address) + " | data: " + Eeprom28C64Api::dataHex(data));

    int bufferIndex = op_read % BUFFER_SIZE;
    if (bufferIndex == 0 && op_read != 0) {
      String dataHex;
      String dataAscii;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        dataHex += Eeprom28C64Api::dataHex(readBuffer[i]);
        if (i % 2 == 1) {
          dataHex += " ";
        }
        if ((readBuffer[i] >= 33 && readBuffer[i] <= 126) || readBuffer[i] == 20 /*space*/) {
          dataAscii += String((char)readBuffer[i]);
        } else {
          dataAscii += ".";
        }
      }
      uint16_t startAddress = address - BUFFER_SIZE;
      // Serial.println(Eeprom28C64Api::addressHex(startAddress) + " [" + Eeprom28C64Api::addressBin(startAddress) + "]" + ": " + dataHex + " " + dataAscii);
      Serial.println(Eeprom28C64Api::addressHex(startAddress) + ": " + dataHex + " " + dataAscii);
    }

    readBuffer[bufferIndex] = data;

    op_read += 1;
    // deinit
    if (op_read > USABLE_ADDRESS_SPACE_SIZE) {
      setBuiltinLed(LedColor::NO_LED_);
      op_read = -1;
      op_verify = true;
      Serial.flush();
      delay(500);
      return;
    }
  }

  if (op_verify) {
    op_verify = false;

    // check damaged cells
    // for (int i = USABLE_ADDRESS_OFFSET; i < USABLE_ADDRESS_OFFSET + USABLE_ADDRESS_SPACE_SIZE; i++) {
    //   if (writeValues[i] != readValues[i]) {
    //     damagedCells[i] = true;
    //     damagedCellsTotal += 1;
    //     Serial.println(getAddressStrBin(i) + " | W: " + getDataStrHex(writeValues[i]) + " | R: " + getDataStrHex(readValues[i]));
    //   }
    // }

    // int busyStateUsecTotal = 0;
    // int busyStateUsecMax = 0;
    // for (int i = USABLE_ADDRESS_OFFSET; i < USABLE_ADDRESS_OFFSET + USABLE_ADDRESS_SPACE_SIZE; i++) {
    //   busyStateUsecTotal += busyStateUsec[i];
    //   busyStateUsecMax = max(busyStateUsecMax, busyStateUsec[i]);
    // }
    // Serial.println("B | T: " + String(busyStateUsecTotal) + " us | AVG: " + String(busyStateUsecTotal / USABLE_ADDRESS_SPACE_SIZE) + " us | MAX: " + String(busyStateUsecMax) + " us");

    // Serial.println("W | T: " + String(write_total_micros) + " us | AVG: " + String(write_total_micros / USABLE_ADDRESS_SPACE_SIZE) + " us");
    Serial.println("R | T: " + String(read_total_micros) + " us | AVG: " + String(read_total_micros / USABLE_ADDRESS_SPACE_SIZE) + " us");
    // if (damagedCellsTotal) {
    //   Serial.println("VERIFY: damagedCellsTotal: " + String(damagedCellsTotal));
    // } else {
    //   Serial.println("VERIFY: OK");
    // }

    Serial.flush();
    delay(500);
    return;
  }
}
