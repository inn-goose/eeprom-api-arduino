#include "main_loop.h"


#define ACTION_BUTTON_PIN 17

void onActionButtonPress() {
  toggleNextOperation();
}


void setup() {
  Serial.begin(9600);

  // action button
  pinMode(ACTION_BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ACTION_BUTTON_PIN), onActionButtonPress, RISING);

  // main loop
  initMainLoop();
}


void loop() {
  mainLoop();
  delay(500);
}
