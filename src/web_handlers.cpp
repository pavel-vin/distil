#include "web_handlers.h"
#include "pid_controller.h"
#include <SPIFFS.h>
#include <EEPROM.h>

extern PIDController headPID;
extern PIDController bodyPID;
extern bool testMode;

void initWebServer(AsyncWebServer &server, SystemState &systemState, SensorData &sensorData) {
  // Веб-страницы
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  // API endpoints
  server.on("/sensors", HTTP_GET, [&systemState, &sensorData](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"cube\":" + String(sensorData.tempCube) + ",";
    json += "\"column\":" + String(sensorData.tempColumn) + ",";
    json += "\"tsa\":" + String(sensorData.tempTsa) + ",";
    json += "\"coolant\":" + String(sensorData.tempCoolant) + ",";
    json += "\"flow\":" + String(sensorData.flowRate) + ",";
    json += "\"leak\":" + String(sensorData.leakDetected) + ",";
    json += "\"strength\":" + String(calculateStrength(sensorData.tempColumn)) + ",";
    json += "\"mode\":" + String(systemState.currentMode) + ",";
    json += "\"testMode\":" + String(testMode);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/setmode", HTTP_POST, [&systemState, &sensorData](AsyncWebServerRequest *request){
    if(request->hasParam("mode", true)) {
      int mode = request->getParam("mode", true)->value().toInt();
      systemState.currentMode = static_cast<SystemMode>(mode);
      
      if(mode == MODE_RECTIFICATION_BODY) {
        systemState.baseTemp = sensorData.tempColumn;
        bodyPID.reset();
      }
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    int headAngle = 30;
    int bodyAngle = 60;
    
    EEPROM.get(HEAD_ANGLE_ADDR, headAngle);
    EEPROM.get(BODY_ANGLE_ADDR, bodyAngle);
    
    String json = "{";
    json += "\"headAngle\":" + String(headAngle) + ",";
    json += "\"bodyAngle\":" + String(bodyAngle);
    json += "}";
    request->send(200, "application/json", json);
  });
  
  server.on("/settest", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("enable", true)) {
      int enable = request->getParam("enable", true)->value().toInt();
      testMode = (enable == 1);
      request->send(200, "text/plain", "Test mode " + String(testMode ? "enabled" : "disabled"));
    }
  });
}

void initCalibrationEndpoints(AsyncWebServer &server, SystemState &systemState, SensorData &sensorData) {
  server.on("/calibrate/heads", HTTP_POST, [&](AsyncWebServerRequest *request){
    systemState.currentMode = MODE_CALIBRATION;
    calibrateServo(0, sensorData);
    request->send(200, "text/plain", "Heads calibrated");
  });
  
  server.on("/calibrate/body", HTTP_POST, [&](AsyncWebServerRequest *request){
    systemState.currentMode = MODE_CALIBRATION;
    calibrateServo(1, sensorData);
    request->send(200, "text/plain", "Body calibrated");
  });
}