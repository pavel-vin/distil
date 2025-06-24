#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sensor_utils.h"
#include "web_handlers.h"
#include "pid_controller.h"

// Глобальные объекты
SystemState systemState;
SensorData sensorData;
AsyncWebServer server(80);
PIDController headPID(0.5, 0.01, 0.1, 0, 89);
PIDController bodyPID(0.7, 0.02, 0.2, 0, 89);

// Очереди FreeRTOS
QueueHandle_t sensorQueue;
TaskHandle_t safetyTaskHandle;
TaskHandle_t controlTaskHandle;

// Задача чтения датчиков
void taskSensors(void *pvParams) {
  while(1) {
    readSensorData(sensorData);
    if (xQueueSend(sensorQueue, &sensorData, portMAX_DELAY) != pdPASS) {
      Serial.println("Failed to send sensor data to queue");
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Задача безопасности
void taskSafety(void *pvParams) {
  SensorData data;
  
  while(1) {
    if(xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
      bool emergency = false;
      
      // Проверка аварийных условий (только в реальном режиме)
      if (!testMode) {
        if(data.leakDetected) {
          emergency = true;
        }
        else if(data.tempCoolant > 50.0 && 
                (systemState.currentMode == MODE_DISTILLATION_WASH || 
                 systemState.currentMode == MODE_DISTILLATION_MANUAL)) {
          emergency = true;
        }
        else if(data.tempCoolant > 60.0) {
          emergency = true;
        }
        else if(systemState.currentMode == MODE_RECTIFICATION_HEATING && 
               (millis() - systemState.timerStart > 300000)) {
          emergency = true;
        }
      }
      
      if(emergency) {
        systemState.currentMode = MODE_EMERGENCY_STOP;
        digitalWrite(HEATER_RELAY_PIN, LOW);
        servo.write(0);
        if (!testMode) tone(BUZZER_PIN, 1000);
      }
    }
  }
}

// Задача управления
void taskControl(void *pvParams) {
  SensorData data;
  
  while(1) {
    if(xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
      switch(systemState.currentMode) {
        case MODE_DISTILLATION_WASH: {
          // Для дистилляции браги используем только температуру куба
          if(data.tempCube > 98.0) {
            digitalWrite(HEATER_RELAY_PIN, LOW);
            systemState.currentMode = MODE_IDLE;
          }
          break;
        }
          
        case MODE_RECTIFICATION_HEATING: {
          if(data.tempCube > 50.0) {
            if (!testMode) tone(BUZZER_PIN, 500, 1000);
            systemState.timerStart = millis();
          }
          break;
        }
          
        case MODE_RECTIFICATION_HEADS: {
          int headAngle = 30; // Значение по умолчанию
          EEPROM.get(HEAD_ANGLE_ADDR, headAngle);
          servo.write(headAngle);
          
          // В тестовом режиме эмулируем поток
          if (testMode) {
            data.flowRate = 1.5 + (millis() % 1000 - 500) * 0.001;
          }
          
          int newAngle = headPID.compute(1.5, data.flowRate);
          servo.write(newAngle);
          break;
        }
          
        case MODE_RECTIFICATION_BODY: {
          if(systemState.baseTemp == 0.0) {
            systemState.baseTemp = data.tempColumn;
            bodyPID.reset();
          }
          
          // В тестовом режиме эмулируем изменение температуры
          if (testMode) {
            data.tempColumn = systemState.baseTemp + (millis() % 2000 - 1000) * 0.01;
          }
          
          int newAngle = bodyPID.compute(systemState.baseTemp, data.tempColumn);
          servo.write(newAngle);
          break;
        }
          
        case MODE_EMERGENCY_STOP: {
          if(testMode || (!data.leakDetected && data.tempCoolant < 40.0)) {
            if (!testMode) noTone(BUZZER_PIN);
            systemState.currentMode = MODE_IDLE;
          }
          break;
        }
          
        default:
          break;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Инициализация EEPROM
  EEPROM.begin(512);
  
  // Инициализация SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
  } else {
    Serial.println("SPIFFS Mount Success");
  }
  
  // Инициализация состояния системы
  systemState.currentMode = MODE_IDLE;
  systemState.baseTemp = 0.0;
  systemState.tempTolerance = 0.2;
  systemState.servoAngle = 0;
  systemState.heaterOn = false;
  systemState.timerStart = 0;
  
  // Инициализация оборудования
  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  initSensors();
  
  // Попытка подключения к WiFi
  String ssid = "your_SSID";
  String password = "your_PASSWORD";
  
  // Попробуем прочитать сохраненные учетные данные
  EEPROM.get(WIFI_SSID_ADDR, ssid);
  EEPROM.get(WIFI_PASS_ADDR, password);
  
  if (ssid.length() > 0) {
    Serial.println("Trying to connect to WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 15) {
      delay(1000);
      Serial.print(".");
      wifiAttempts++;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    // Режим точки доступа
    Serial.println("\nFailed to connect. Starting AP mode...");
    WiFi.softAP("DistillerAP", "distill123");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Специальная страница для настройки WiFi
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = "<h1>WiFi Setup</h1>"
                   "<form action='/savewifi' method='POST'>"
                   "SSID: <input type='text' name='ssid'><br>"
                   "Password: <input type='password' name='password'><br>"
                   "<input type='submit' value='Save'>"
                   "</form>";
      request->send(200, "text/html", html);
    });
    
    server.on("/savewifi", HTTP_POST, [](AsyncWebServerRequest *request){
      if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        
        // Сохраняем в EEPROM
        EEPROM.put(WIFI_SSID_ADDR, ssid);
        EEPROM.put(WIFI_PASS_ADDR, password);
        EEPROM.commit();
        
        request->send(200, "text/plain", "Settings saved. Restarting...");
        delay(1000);
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });
  }

  // Инициализация веб-сервера
  initWebServer(server, systemState, sensorData);
  initCalibrationEndpoints(server, systemState, sensorData);
  server.begin();
  
  // Создание очереди
  sensorQueue = xQueueCreate(10, sizeof(SensorData));
  if (sensorQueue == NULL) {
    Serial.println("Failed to create sensor queue");
  }
  
  // Создание задач
  xTaskCreate(taskSensors, "Sensors", 4096, NULL, 1, NULL);
  xTaskCreate(taskSafety, "Safety", 4096, NULL, 3, &safetyTaskHandle);
  xTaskCreate(taskControl, "Control", 8192, NULL, 2, &controlTaskHandle);
  
  Serial.println("System initialization complete");
}

void loop() {
  vTaskDelete(NULL);
}