#include "sensor_utils.h"
#include <Arduino.h>
#include <EEPROM.h>

bool testMode = true; // Режим тестирования без датчиков

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensors(&oneWire);
Servo servo;

void initSensors() {
  if (testMode) {
    Serial.println("Running in TEST MODE - no sensors required");
    return;
  }
  
  Serial.println("Initializing sensors...");
  
  // Инициализация датчиков температуры
  tempSensors.begin();
  
  // Настройка пинов
  pinMode(FLOW_SENSOR_PIN, INPUT);
  pinMode(LEAK_SENSOR_PIN, INPUT_PULLUP);
  servo.attach(SERVO_PIN);
  
  Serial.println("Sensors initialized");
}

void readSensorData(SensorData &data) {
  if (testMode) {
    // Тестовые данные
    static unsigned long lastChange = 0;
    static float tempDirection = 0.1;
    
    if (millis() - lastChange > 2000) {
      tempDirection = -tempDirection;
      lastChange = millis();
    }
    
    data.tempCube = 75.0 + tempDirection * 5;
    data.tempColumn = 78.5 + tempDirection * 0.5;
    data.tempTsa = 25.0;
    data.tempCoolant = 30.0;
    data.flowRate = 1.5 + (millis() % 1000 - 500) * 0.001;
    data.leakDetected = false;
  } else {
    // Режим работы с датчиками
    tempSensors.requestTemperatures();
    data.tempCube = tempSensors.getTempCByIndex(0);
    data.tempColumn = tempSensors.getTempCByIndex(1);
    data.tempTsa = tempSensors.getTempCByIndex(2);
    data.tempCoolant = tempSensors.getTempCByIndex(3);
    
    // Чтение протока (заглушка - нужна калибровка)
    data.flowRate = pulseIn(FLOW_SENSOR_PIN, HIGH) / 1000.0;
    
    // Датчик протечки
    data.leakDetected = !digitalRead(LEAK_SENSOR_PIN);
  }
}

float calculateStrength(float columnTemp) {
  // Расчет крепости по температуре колонны
  // Формула: strength = (78.4 - columnTemp) * 4.0 + 95.0
  float strength = 95.0 - (78.4 - columnTemp) * 4.0;
  
  // Ограничение диапазона
  if (strength < 0) return 0;
  if (strength > 100) return 100;
  return strength;
}

void calibrateServo(int mode, SensorData &data) {
  int angle = servo.read();
  int targetFlow = (mode == 0) ? 1 : 25;
  
  // В тестовом режиме эмулируем калибровку
  if (testMode) {
    angle = (mode == 0) ? 30 : 60;
    data.flowRate = targetFlow;
    delay(1000);
  } else {
    // Реальная калибровка
    while(fabs(data.flowRate - targetFlow) > 0.5) {
      if(data.flowRate < targetFlow) {
        angle = min(89, angle + 1);
      } else {
        angle = max(0, angle - 1);
      }
      servo.write(angle);
      delay(1000);
      readSensorData(data);
    }
  }
  
  if(mode == 0) {
    EEPROM.put(HEAD_ANGLE_ADDR, angle);
  } else {
    EEPROM.put(BODY_ANGLE_ADDR, angle);
  }
  EEPROM.commit();
}