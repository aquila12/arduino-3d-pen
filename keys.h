#include "Arduino.h"
#include "scheduler.h"

// Key numbering
#define K_INC 4
#define K_DEC 3
#define K_FWD 2
#define K_REV 1

extern byte key;
extern bool key_changed;  // Must be cleared manually!

void readKeys();
time_ms keydown_ms(); // Checks how long a key has been held (0 if none held)
