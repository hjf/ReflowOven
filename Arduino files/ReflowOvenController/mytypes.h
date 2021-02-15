#ifndef MYTYPES_H
#define MYTYPES_H
#include <stdint.h>
#include <PID_v1.h>

typedef struct thermocouple_reading_s {
  float cold_junction_temperature;
  float thermocouple_temperature;
  uint8_t fault;
} thermocouple_reading_t;

extern thermocouple_reading_t tc_reading;

typedef struct profile_s {
  int soak_target;
  int soak_time;
  int reflow_target;
  int reflow_time;
  int cooling_target;
} profile_t;

typedef enum oven_status_enum {
  IDLE = 0,
  SOAK = 1,
  REFLOW = 2,
  COOLING = 3,
  BAKING = 4
} oven_status_t;

extern oven_status_t oven_status;
extern profile_t current_profile;
extern double Setpoint, Input, Output;
extern double Kp,  Ki , Kd ;
extern bool tuningsChanged;
extern int32_t remaining_seconds;
#endif
