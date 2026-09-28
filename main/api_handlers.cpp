#include <Arduino.h>
#include <ArduinoJson.h>
#include "api_handlers.h"
#include "sensors.h"
#include "irrigation.h"

// Route /led — actually controls the valve/pump today (irrigationTask,
// MANUAL trigger). Route name and JSON field ("ledState") kept for
// compatibility with the existing dashboard, even though it's no longer an
// isolated test LED - see docs/irrigation-modes.md.
void handleLedPost(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  bool turnOn = doc["state"] | false;   // "state" kept - it's the field the dashboard already sends
  irrigationSetManual(turnOn);          // only requests it - irrigationTask is what drives the pin, on its own cycle (every 500ms)

  JsonDocument response;
  response["ledState"] = turnOn;   // reflects the REQUEST - the real pin converges at most 500ms later
  String out;
  serializeJson(response, out);
  request->send(200, "application/json", out);
}

void handleSensorsRefreshPost(AsyncWebServerRequest* request) {
  // Only requests the sample - doesn't wait for it to finish. sensorsTask
  // wakes up, samples, and updates currentState asynchronously; the next GET
  // /api/status is what reads that fresh value.
  sensorsRequestRefresh();
  request->send(202, "application/json", "{\"status\":\"refresh requested\"}");
}

void handleStatusGet(AsyncWebServerRequest* request) {
  // Reads the latest reading via sensorsGetLatest() — a mutex-protected
  // copy, never calling readSoilMoisture()/etc directly here (those can only
  // be called from inside sensorsTask, see sensors.cpp).
  SensorReading reading = sensorsGetLatest();

  JsonDocument response;
  response["soilMoisturePercent"]  = reading.soilMoisturePercent;
  response["airTemperatureC"]      = reading.airTemperatureC;
  response["airHumidityPercent"]   = reading.airHumidityPercent;
  response["sensorAgeMs"]  = millis() - reading.sampledAtMs;

  IrrigationStatus irrigationStatus = irrigationGetStatus();
  response["ledState"]  = irrigationStatus.valveOn;   // name kept for compatibility - reflects the valve, not a LED
  response["uptimeMs"]  = millis();

  String out;
  serializeJson(response, out);
  request->send(200, "application/json", out);
}
