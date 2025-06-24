#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

class PIDController {
public:
  PIDController(float Kp, float Ki, float Kd, int min, int max)
    : Kp(Kp), Ki(Ki), Kd(Kd), minVal(min), maxVal(max) {}
  
  int compute(float setpoint, float input);
  void reset();

private:
  float Kp, Ki, Kd;
  float integral = 0;
  float prevError = 0;
  int minVal, maxVal;
};

#endif