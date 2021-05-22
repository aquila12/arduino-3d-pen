#include "scheduler.h"

#include "lcd.h"
#include "keys.h"
#include "temperature.h"
#include "hardware.h"

#include <LinearInterpolator.h>

#define IDLE_DELAY 25
#define AUTOREPEAT_DELAY 500
#define AUTOREPEAT_EVERY 50

/*** INPUT FUNCTIONS ***/

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

void ms_tick() {
  updateLCDState();
}

void yield(int ms) {
  time_ms end_time = current_time() + ms;
  do {
    readKeys();
    readTemperature();
    delay_ms(IDLE_DELAY);
  } while(current_time() < end_time);
}

void doSplash() {
  setLCDString("3D"); yield(1000);
  setLEDRed(); yield(500);
  setLEDGreen(); yield(500);
  setLEDOff(); yield(1000);
}

void doWait() {
  
}

void setup() {
  TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz
  
  setupScheduler();
  
  resetOutputs();
  doSplash();
}

void doIdle() {  
  static time_ms display_revert_time = 0;
  static bool enable_heater = false;

  yield(AUTOREPEAT_EVERY);

  if(key) {
    // "Key presses"
    if(key_changed || keydown_ms() > AUTOREPEAT_DELAY) {
      if(key == K_INC || key == K_DEC) {
        temperature_setpoint += (key == K_INC) ? 1 : -1;
        setLCDNumeric(temperature_setpoint);
        display_revert_time = current_time() + 2000;
      }
    }

    if(key == K_FWD) { enable_heater = true; }
    if(key == K_REV) { enable_heater = false; }
  }

  key_changed = false;

  if(enable_heater) {
    int dt = temperature - temperature_setpoint;
    if(dt > 15) enable_heater = false;
    else if(dt > 0) setHeater(0);
    else if(dt > -5) setHeater(32);
    else setHeater(255);
  }
  
  if(!enable_heater) setHeater(0);

  if(current_time() > display_revert_time) setLCDNumeric(temperature);
}

void loop() {
  doIdle();
}
