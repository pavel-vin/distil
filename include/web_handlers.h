#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "sensor_utils.h"

void initWebServer(AsyncWebServer &server, SystemState &systemState, SensorData &sensorData);
void initCalibrationEndpoints(AsyncWebServer &server, SystemState &systemState, SensorData &sensorData);

#endif