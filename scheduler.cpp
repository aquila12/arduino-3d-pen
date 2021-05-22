#include "scheduler.h"

#include "Arduino.h"
#include <util/atomic.h>

// Timer interrupt setup code
// Amanda Ghassaei
// https://www.instructables.com/id/Arduino-Timer-Interrup
void setupScheduler() {
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

static volatile time_ms _current_time = 0;

extern void ms_tick();
void ms_tick(); // weak

ISR(TIMER2_COMPA_vect) { // 1ms timer callback
  ++_current_time;
  ms_tick();
}

time_ms current_time() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return _current_time; };
}

void delay_ms(unsigned long ms) {
  time_ms end_t = (current_time() + ms);
  while(current_time() < end_t);
}
