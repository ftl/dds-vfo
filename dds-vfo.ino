/*
 * Copyright (c) Florian Thienel/DL3NEY.
 * 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>

#define PIN_ROT_A 2
#define PIN_ROT_B 3
#define PIN_ROT_BTN 14

#define PIN_DDS_W_CLK 8
#define PIN_DDS_FQ_UD 9
#define PIN_DDS_DATA 10
#define PIN_DDS_RESET 11

#define MIN_HERTZ 14000000
#define MAX_HERTZ 14350000

#define DDS_CLOCK 125000000
#define DDS_ADJUST -200
#define DDS_SHIFT 10700000

#define HERTZ_ADR 0
#define STEP_SIZE_ADR 4

const char DIR[] = {0, 0, -1, 1};
const int32_t STEP_SIZE[] = {10, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 100000, 1000000};
const String STEP_NAME[] = {",01", ",05", ",10", ",25", ",50", "1,0", "2,5", "5,0", " 10", "100", " 1M"};
const int STEPS = sizeof(STEP_SIZE) / sizeof(int32_t);

LiquidCrystal lcd(12, 13, 7, 6, 5, 4);

int32_t hertz = MIN_HERTZ;
int32_t visibleHertz = hertz;
byte stepSizeIndex = 0;
byte visibleStepSizeIndex = stepSizeIndex;

int32_t inputHertz = 0;

void setup() {
  pinMode(PIN_ROT_A, INPUT);
  pinMode(PIN_ROT_B, INPUT);
  pinMode(PIN_ROT_BTN, INPUT);

  digitalWrite(PIN_ROT_A, HIGH);
  digitalWrite(PIN_ROT_B, HIGH);
  digitalWrite(PIN_ROT_BTN, HIGH);

  attachInterrupt(0, rotTurned, RISING);

  lcd.begin(16, 2);

  pinMode(PIN_DDS_FQ_UD, OUTPUT);
  pinMode(PIN_DDS_W_CLK, OUTPUT);
  pinMode(PIN_DDS_DATA, OUTPUT);
  pinMode(PIN_DDS_RESET, OUTPUT);

  resetDDS();

  Serial.begin(9600);
  delay(100);

  setFrequency(loadFrequency());
  setStepSize(loadStepSize());

  sendFrequencyToDDS();

  showFrequency();
  showStepSize();
}

void loop() {
  handleSerialInput();
  
  handleFrequencyChange();
  handleStepSizeChange();
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = (char) Serial.read();
    if (c == 'F') {
      changeFrequency(STEP_SIZE[stepSizeIndex]);
    } else if (c == 'f') {
      changeFrequency(-1 * STEP_SIZE[stepSizeIndex]);
    } else if (c == 'S') {
      changeStepSize(1);
    } else if (c == 's') {
      changeStepSize(-1);
    } else if (c == '?') {
      showFrequency();
    } else if ((c == '\n' || c == '\r') && inputHertz > 0) {
      setFrequency(inputHertz);
      inputHertz = 0;
    } else if (isDigit(c)) {
      inputHertz = inputHertz * 10 + digit(c);
    }
  }
}

bool isDigit(char c) {
  int value = digit(c);
  return value >= 0 && value <= 9;
}

int digit(char c) {
  return (int)c - '0';
}

void handleFrequencyChange() {
  if (visibleHertz != hertz) {
    visibleHertz = hertz;
    sendFrequencyToDDS();
    storeFrequency();
    showFrequency();
  }
}

void handleStepSizeChange() {
  if (visibleStepSizeIndex != stepSizeIndex) {
    visibleStepSizeIndex = stepSizeIndex;
    storeStepSize();
    showStepSize();
  }
}

void rotTurned() {
  byte turn = digitalRead(PIN_ROT_A) * 2 + digitalRead(PIN_ROT_B);
  boolean pressed = !digitalRead(PIN_ROT_BTN);

  if (pressed) {
    changeStepSize(DIR[turn]);
  } else {
    changeFrequency(DIR[turn] * STEP_SIZE[stepSizeIndex]);
  }
}

void changeFrequency(int32_t delta) {
  setFrequency(hertz + delta);
}

void setFrequency(int32_t h) {
  hertz = constrain(h, MIN_HERTZ, MAX_HERTZ);
}

int32_t loadFrequency() {
  int32_t h;
  EEPROM.get(HERTZ_ADR, h);
  return h;
}

void storeFrequency() {
  EEPROM.put(HERTZ_ADR, visibleHertz);
}

void showFrequency() {
  String f = String(visibleHertz, DEC);
  String t = f.substring(0, 2) + "." + f.substring(2, 5) + "," + f.substring(5, 7) + "kHz";

  Serial.println(visibleHertz, DEC);
  lcd.setCursor(0, 0);
  lcd.print(t);
}

void sendFrequencyToDDS() {
  uint32_t effectiveFrequency = visibleHertz + DDS_SHIFT;
  
  uint32_t tuningWord = (effectiveFrequency * pow(2, 32)) / (DDS_CLOCK + DDS_ADJUST);

  pulseHigh(PIN_DDS_W_CLK);
  pulseHigh(PIN_DDS_FQ_UD);
  for (int i = 0; i < 4; i += 1, tuningWord >>= 8) {
    shiftOut(PIN_DDS_DATA, PIN_DDS_W_CLK, LSBFIRST, tuningWord);
  }
  shiftOut(PIN_DDS_DATA, PIN_DDS_W_CLK, LSBFIRST, 0x00);
  digitalWrite(PIN_DDS_FQ_UD, HIGH);
}

void resetDDS() {
  digitalWrite(PIN_DDS_W_CLK, LOW);
  digitalWrite(PIN_DDS_FQ_UD, LOW);

  pulseHigh(PIN_DDS_RESET);
  digitalWrite(PIN_DDS_DATA, LOW);
  pulseHigh(PIN_DDS_W_CLK);
  pulseHigh(PIN_DDS_FQ_UD);
}

void pulseHigh(byte pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pin, LOW);
  delayMicroseconds(5);
}

void changeStepSize(char delta) {
  setStepSize(stepSizeIndex + delta);
}

void setStepSize(char i) {
  stepSizeIndex = constrain(i, 0, STEPS - 1);
}

int loadStepSize() {
  int s;
  EEPROM.get(STEP_SIZE_ADR, s);
  return s;
}

void storeStepSize() {
  EEPROM.put(STEP_SIZE_ADR, visibleStepSizeIndex);
}

void showStepSize() {
  String t = STEP_NAME[visibleStepSizeIndex];

  lcd.setCursor(13, 0);
  lcd.print(t);
}

