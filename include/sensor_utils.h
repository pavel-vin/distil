#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Конфигурация пинов
#define HEATER_RELAY_PIN 2
#define SERVO_PIN 3
#define BUZZER_PIN 4
#define FLOW_SENSOR_PIN 5
#define LEAK_SENSOR_PIN 6
#define ONE_WIRE_BUS 15

// Адреса EEPROM
#define HEAD_ANGLE_ADDR 0
#define BODY_ANGLE_ADDR 4
#define WIFI_SSID_ADDR 100
#define WIFI_PASS_ADDR 132

struct SensorData {
  float tempCube;
  float tempColumn;
  float tempTsa;
  float tempCoolant;
  float flowRate;
  bool leakDetected;
};

enum SystemMode {
  MODE_IDLE,
  MODE_DISTILLATION_WASH,
  MODE_RECTIFICATION_HEATING,
  MODE_RECTIFICATION_HEADS,
  MODE_RECTIFICATION_BODY,
  MODE_DISTILLATION_AUTO,
  MODE_DISTILLATION_MANUAL,
  MODE_CALIBRATION,
  MODE_EMERGENCY_STOP
};

struct SystemState {
  SystemMode currentMode;
  float baseTemp;
  float tempTolerance;
  int servoAngle;
  bool heaterOn;
  unsigned long timerStart;
};

extern bool testMode;

void initSensors();
void readSensorData(SensorData &data);
float calculateStrength(float columnTemp);
void calibrateServo(int mode, SensorData &data);

extern OneWire oneWire;
extern DallasTemperature tempSensors;
extern Servo servo;

#endif