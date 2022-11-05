#include "scheduler.h"

#include "lcd.h"
#include "keys.h"
#include "temperature.h"
#include "hardware.h"

#include <LinearInterpolator.h>

#define IDLE_DELAY 25
#define AUTOREPEAT_DELAY 500
#define AUTOREPEAT_EVERY 50
#define LED_FLASH_DELAY (led_flash ? 667 : 333)
#define OVERTEMP_LIMIT 15
#define PID_THRESHOLD 5

/*** INPUT FUNCTIONS ***/

// Slider linearisation (loaded variable resistor.....why)
/*
 * Parameters:
 * R_0 9000
 * Full-scale reading 0..255
 * R_L 1000
 * Assumed Linear; note this curve can be combined directly to whatever we want in terms of speeds...
 */
int slider_points[] = { 16, 21, 26, 33, 45, 66, 107, 255 };
LinearInterpolator speed_curve(25, 7, 7, slider_points);

int speed = 0;
void readSlider() {
  int result = speed_curve.interpolate(analogRead(SPEED_INPUT), speed);
  if(result == LININT_TOO_LOW || speed < 16) speed = 0;
  if(result == LININT_TOO_HIGH || speed > 255) speed = 255;
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

enum led_state { LED_OFF, LED_G_ON, LED_R_ON, LED_G_FLASH, LED_R_FLASH };
void setLED(led_state state, bool flash_state = true) {
  if(!flash_state && (state == LED_G_FLASH || state == LED_R_FLASH)) state = LED_OFF;
  
  switch(state) {
    case LED_OFF: pinMode(LED_READY, INPUT); break;
    case LED_G_ON:
    case LED_G_FLASH: pinMode(LED_READY, OUTPUT); digitalWrite(LED_READY, 1); break;
    case LED_R_ON:
    case LED_R_FLASH: pinMode(LED_READY, OUTPUT); digitalWrite(LED_READY, 0); break;
  }
}

inline int clamp(int x, int x_min, int x_max) { return (x < x_min) ? x_min : ((x > x_max) ? x_max : x); };

/*** PROGRAM ***/

void ms_tick() {
  updateLCDState();
}

void yield(int ms) {
  time_ms end_time = current_time() + ms;
  do {
    readKeys();
    readSlider();
    readTemperature();
    delay_ms(IDLE_DELAY);
  } while(current_time() < end_time);
}

void doSplash() {
  setLCDString("3D"); yield(1000);
  setLED(LED_R_ON); yield(500);
  setLED(LED_G_ON); yield(500);
  setLED(LED_OFF); yield(1000);
}

void setup() {
  TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz
  
  setupScheduler();
  
  resetOutputs();
  doSplash();
}

void loop() {  
  static time_ms display_revert_time = 0;
  static time_ms led_flash_time = 0;
  static time_ms motor_reverse_time = 0;
  static bool led_flash = true;
  static led_state led = LED_OFF;
  static bool enable_heater = false;
  static bool enable_motor = false;

  yield(AUTOREPEAT_EVERY);

  time_ms now = current_time();

  if(key) {
    // "Key presses"
    if(key_changed || keydown_ms() > AUTOREPEAT_DELAY) {
      if(key == K_INC || key == K_DEC) {
        temperature_setpoint += (key == K_INC) ? 1 : -1;
        setLCDNumeric(temperature_setpoint);
        display_revert_time = now + 2000;
      }
    }

    if(key == K_FWD) { enable_heater = true; }
    if(key == K_REV && keydown_ms() > 1000) { enable_heater = false; }
  }

  key_changed = false;

  // TODO: Implement a PID loop that does the job when switching between extrude / not
  if(enable_heater) {
    int dt = temperature - temperature_setpoint;
    static int dt_last = dt;
    static int i = 0;
    int p, d;

    if(dt > OVERTEMP_LIMIT) {
      // Trip to prevent thermal runaway
      enable_heater = false;
    } else if(dt > PID_THRESHOLD) {
      setHeater(0);
      enable_motor = true;
      led = LED_R_FLASH;
    } else if(dt < -PID_THRESHOLD) {
      setHeater(255);
      enable_motor = false;
      led = LED_R_ON;
    } else {
      // PID Control
      p = dt;
      i = clamp(i + dt, -511, 511);
      d = dt - dt_last;

      int sum = (256 * p) + (4 * i) + (8192 * d);
      int htr = clamp(256 - (sum >> 2), 0, 255);
      setHeater(htr);
      // setLCDNumeric(htr); display_revert_time = now + 1000;
      enable_motor = true;
      led = LED_G_ON;
    }
    
    dt_last = dt;
  }
  
  if(!enable_heater) {
    setHeater(0);
    led = (temperature < 50) ? LED_G_FLASH : LED_OFF;
  }
  
  if(enable_motor) {
    if(key == K_FWD) setMotorForwards(speed);
    else if(now < motor_reverse_time) setMotorReverse();
    else if(key == K_REV) { setMotorReverse(); motor_reverse_time = now + 1000; }
    else setMotorForwards(0);
  } else setMotorForwards(0);

  if(now > display_revert_time) setLCDNumeric(temperature);
  
  if(now > led_flash_time) {
    led_flash = !led_flash;
    led_flash_time = now + LED_FLASH_DELAY;
  }

  setLED(led, led_flash);
}
