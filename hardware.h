/* Hardware connections */

// Analogue inputs
#define TEMP_INPUT A7
#define SPEED_INPUT A6
#define KEY_INPUT A5

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
