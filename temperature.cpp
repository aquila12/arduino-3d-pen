// Temperature sensing

#include "Arduino.h"

#include "temperature.h"
#include "hardware.h"

#include <LinearInterpolator.h>

/*
 * Parameters:
 * R_0 100000
 * T_0 298
 * R_L 992
 * Beta  3950
 */
static int thermistor_points[] = { 323, 281, 253, 232, 215, 199, 186, 173, 160, 147, 134, 120, 102, 78, 17 };
static LinearInterpolator temp_curve(120, 6, 14, thermistor_points);

int temperature = UNINITIALIZED_TEMP_READING;
int temperature_setpoint = DEFAULT_TEMP_SETPOINT;

void readTemperature() {
  int result = temp_curve.interpolate(analogRead(TEMP_INPUT), temperature);
  if(result == LININT_TOO_LOW) temperature = MAX_TEMP_READING;
  if(result == LININT_TOO_HIGH) temperature = -MIN_TEMP_READING;
}
