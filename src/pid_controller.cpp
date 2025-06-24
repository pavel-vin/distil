#include "pid_controller.h"
#include <Arduino.h>

int PIDController::compute(float setpoint, float input) {
  float error = setpoint - input;
  integral += error;
  float derivative = error - prevError;
  prevError = error;
  
  float output = Kp * error + Ki * integral + Kd * derivative;
  return constrain(static_cast<int>(output), minVal, maxVal);
}

void PIDController::reset() {
  integral = 0;
  prevError = 0;
}