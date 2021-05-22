#include <LinearInterpolator.h>

#include "lcd.h"

// Analogue inputs
#define TEMP_INPUT A7
#define SPEED_INPUT A6
#define KEY_INPUT A5

#define KEY_PRESHIFT 7
#define K_INC 4
#define K_DEC 3
#define K_FWD 2
#define K_REV 1

// Digital pin/pulse outputs
#define LED_READY 4
#define HTR_ON 5
#define MTR_FWD 6
#define MTR_REV 7

// Port C 0..3 for tristate selectors (segments), 4 as "balance"
#define LCD_SELECT PORTC
#define LCD_SELECT_DDR DDRC
// Port B 0..5 for digital selectors (half-digits)
#define LCD_PLANES PORTB
#define LCD_PLANES_DDR DDRB

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

byte lcdState[4]; // See lcd.h

// Update the LCD into the next time state
// It rotates through 8 states, each one + then - on each segment set in turn
// Note that there is also a ballast line in the segment output!
// The data from the state array is _the state of the backplanes_
void updateLCDState() {
  static byte t_state = 0;
  byte select = t_state >> 1;
  
  byte data = lcdState[select]; // This data feeds the 6 half-digit backplanes
  byte selectbit = (1 << select);
  LCD_PLANES = (t_state & 1) ? data : ~data;      // Set backplane polarity
  LCD_SELECT = (t_state & 1) ? 0b10000 : selectbit; // Set segment polarity (ballast line opposite)
  
  LCD_PLANES_DDR = 0b00111111;                  // Output on backplanes
  LCD_SELECT_DDR = selectbit + 0b00010000;    // Output on segment select and ballast line

  // Advance to the next state (modulo 8)
  t_state = (t_state + 1) & 0x7;
}

void setLCDState(lcd_glyph dig1, lcd_glyph dig2, lcd_glyph dig3) {
  for(int i=0; i<4; ++i) {
    lcdState[i] = ((dig1 & DIG_MASK) << DIG_1) | ((dig2 & DIG_MASK) << DIG_2) | ((dig3 & DIG_MASK) << DIG_3);
    dig1 >>= 2;
    dig2 >>= 2;
    dig3 >>= 2;
  }
}

void setLCDString(const char* string) {
  lcd_glyph buf[3] = { BLANK, BLANK, BLANK };
  for(int i=0; i<3 && string[i]; ++i) buf[i] = lcd_char(string[i]);
  setLCDState(buf[0], buf[1], buf[2]);
}

void setLCDNumeric(int number) {
  if(number == 0) setLCDState(BLANK, BLANK, numeral[0]);
  else if(number > 999) setLCDState(DASH, DASH, DASH);
  else if(number < -99) setLCDState(DASH, DASH, DASH);
  else {
    bool neg = (number < 0);
    if(neg) number = -number;

    byte d0=0, d1=0;
    if(number >= 800) { number -= 800; d0 += 8; }
    if(number >= 400) { number -= 400; d0 += 4; }
    if(number >= 200) { number -= 200; d0 += 2; }
    byte n = number;
    if(n >= 100) { n -= 100; d0 += 1; }
    if(n >=  80) { n -= 80; d1 += 8; }
    if(n >=  40) { n -= 40; d1 += 4; }
    if(n >=  20) { n -= 20; d1 += 2; }
    if(n >=  10) { n -= 10; d1 += 1; }
    byte d2 = n;

    setLCDState(
      neg ? DASH : (d0 ? numeral[d0] : BLANK),
      (d1 || d0) ? numeral[d1] : BLANK,
      numeral[d2]
    );
  }
}

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
