#include "keys.h"
#include "hardware.h"

byte key = 0;
bool key_changed = false;
static time_ms key_change_t = 0;

#define KEY_PRESHIFT 7

static byte readKeyValue() {
  int inputValue = analogRead(KEY_INPUT);
  inputValue >>= KEY_PRESHIFT;
  ++inputValue;
  inputValue >>= 1;
  return 4 - inputValue;
}

void readKeys() {
  static byte last_key = 0;
  byte new_key = readKeyValue();
  
  if(new_key == last_key) {
    if(key != new_key) {
      key_change_t = current_time();
      key_changed = true;
      key = new_key;
    }
  }
  else last_key = new_key;
}

time_ms keydown_ms() { return key ? current_time() - key_change_t : 0; }
