#define MIN_TEMP_READING -99
#define MAX_TEMP_READING 999
#define UNINITIALIZED_TEMP_READING -100

#define DEFAULT_TEMP_SETPOINT 100

extern int temperature;  //°C
extern int temperature_setpoint;
void readTemperature();
