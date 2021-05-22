#include "lcd.h"
#include "hardware.h"

static byte lcdState[4]; // See lcd.h

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
