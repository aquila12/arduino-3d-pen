#include <LinearInterpolator.h>
#include "lcd.h"
#include "hardware.h"

// Key numbering
#define K_INC 4
#define K_DEC 3
#define K_FWD 2
#define K_REV 1

// Other definitions
#define IDLE_DELAY 25

/*** INPUT FUNCTIONS ***/

// Temperature sensing
/*
 * Parameters:
 * R_0 100000
 * T_0 298
 * R_L 992
 * Beta  3950
 */
int thermistor_points[] = { 323, 281, 253, 232, 215, 199, 186, 173, 160, 147, 134, 120, 102, 78, 17 };
LinearInterpolator temp_curve(120, 6, 14, thermistor_points);

int temperature = -100;
void readTemperature() {
  int result = temp_curve.interpolate(analogRead(TEMP_INPUT), temperature);
  if(result == LININT_TOO_LOW) temperature = 999;
  if(result == LININT_TOO_HIGH) temperature = -99;
}

// Slider linearisation (loaded variable resistor.....why)
/*
 * Parameters:
 * R_0 9000
 * Full-scale reading 0..100
 * R_L 1000
 * Assumed Linear; note this curve can be combined directly to whatever we want in terms of speeds...
 */
int slider_points[] = { -1, 0, 2, 4, 7, 12, 21, 39, 104 };
LinearInterpolator speed_curve(-100, 7, 8, slider_points);

int speed = 100;
void readSlider() {
  int result = speed_curve.interpolate(analogRead(SPEED_INPUT), speed);
  if(result == LININT_TOO_LOW || speed < 0) speed = 0;
  if(result == LININT_TOO_HIGH || speed > 100) speed = 100;
}

// Return key press information 0-4
unsigned long keydown_t = 0;
byte key = 0;
bool key_changed = false;
void readKeys() {
  static byte last_key = 0;
  int inputValue = analogRead(KEY_INPUT);
  inputValue >>= KEY_PRESHIFT;
  ++inputValue;
  inputValue >>= 1;
  byte new_key = 4 - inputValue;
  if(new_key == last_key) {
    if(key != new_key) {
      keydown_t = millis();
      key_changed = true;
      key = new_key;
    }
  }
  else last_key = new_key;
}

bool key_event() { bool k = key_changed; key_changed = false; return k; }
unsigned long keydown_ms() { return key ? millis() - keydown_t : 0; }

/*** OUTPUT FUNCTIONS ***/
void setHeater(int pwm) {
  analogWrite(HTR_ON, pwm);
}

void setMotorForwards(int pwm) {
  digitalWrite(MTR_REV, 0);
  analogWrite(MTR_FWD, pwm);
}

void setMotorReverse() {
  analogWrite(MTR_FWD, 0);
  digitalWrite(MTR_REV, 1);
}

void resetOutputs() {
  pinMode(HTR_ON, OUTPUT);
  pinMode(MTR_FWD, OUTPUT);
  pinMode(MTR_REV, OUTPUT);

  setMotorForwards(0);
  setHeater(0); 
}

void setLEDGreen() { pinMode(LED_READY, OUTPUT); digitalWrite(LED_READY, 1); };
void setLEDRed() { pinMode(LED_READY, OUTPUT); digitalWrite(LED_READY, 0); };
void setLEDOff() { pinMode(LED_READY, INPUT); };

/*** PROGRAM ***/

// Timer interrupt setup code
// Amanda Ghassaei
// https://www.instructables.com/id/Arduino-Timer-Interrup
void setupTimer2() {
  cli();
  //set timer2 interrupt at 1kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 1khz increments
  OCR2A = 249;// = (16*10^6) / (1000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 64 prescaler
  TCCR2B |= (1 << CS22);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei();
}

ISR(TIMER2_COMPA_vect) { // 1ms timer callback
  updateLCDState();
}

void setup() {
  setupTimer2();
  
  resetOutputs();
  setLEDOff();
}

void yield(int ms) {
  unsigned long end_time = millis() + ms;
  do {
    readKeys();
    readTemperature();
    delay(IDLE_DELAY);
  } while(millis() < end_time);
}

void doSplash() {
  setLCDString("3D");
  yield(1000);
  setLEDRed(); yield(500);
  setLEDGreen(); yield(500);
  setLEDOff(); yield(1000);
}

void doIdleTemp() {
  if(temperature > 30) setLEDRed();
  else setLEDOff();
  
  setLCDNumeric(temperature);
  yield(1000);
}

void loop() {
  yield(0);

  doSplash();

  while(true) {
    doIdleTemp();
  }
}
