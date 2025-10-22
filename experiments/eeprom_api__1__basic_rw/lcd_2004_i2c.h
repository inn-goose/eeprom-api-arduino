#ifndef __lcd_2004_i2c_h__
#define __lcd_2004_i2c_h__

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

namespace Lcd2004I2C {

// use I2C scanner to find the address
LiquidCrystal_I2C _lcd(0x27, 20, 4);

const String _EMPTY_LCD_LINE = "                    ";  // 20 spaces

void lcdInit() {
  _lcd.init();
  _lcd.backlight();
  _lcd.home();
}

void lcdPrint(String line1, String line2 = "", String line3 = "", String line4 = "") {
  _lcd.setCursor(0, 0);
  _lcd.print(line1);
  _lcd.setCursor(0, 1);
  _lcd.print(line2);
  _lcd.setCursor(0, 2);
  _lcd.print(line3);
  _lcd.setCursor(0, 3);
  _lcd.print(line4);
  _lcd.display();
}

void lcdClear() {
  lcdPrint(_EMPTY_LCD_LINE, _EMPTY_LCD_LINE, _EMPTY_LCD_LINE, _EMPTY_LCD_LINE);
}

}  // Lcd2004I2C

#endif  // !__lcd_2004_i2c_h__