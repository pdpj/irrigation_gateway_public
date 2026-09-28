#pragma once
#include <Arduino.h>

// Sensor reading. Currently simulated (fixed/representative values), just to
// validate the API contract before the real hardware arrives. Once the
// capacitive soil sensor and the DHT22 are wired up, only the body of these
// functions changes - nothing else in the firmware needs to know the difference.
//
// From this point on these three functions are only called from inside
// sensorsTask (below) - never directly by the HTTP handlers, which run in the
// AsyncWebServer's context, potentially at the same time.
float readSoilMoisture();
float readAirTemperature();
float readAirHumidity();

// --- Sampling task + thread-safe state access ---
//
// The most recent reading lives in a struct protected by a mutex.
// sensorsGetLatest() is the only safe way for the rest of the firmware to
// read that state - any other direct access would reintroduce the race
// condition between sensorsTask (which writes) and the HTTP handlers (which
// would read).

struct SensorReading {
  float soilMoisturePercent;
  float airTemperatureC;
  float airHumidityPercent;
  uint32_t sampledAtMs;   // millis() of when this reading was sampled
};

// Creates the mutex and starts sensorsTask, pinned to core 0. Call once, in
// setup() - before or after WiFi.begin() doesn't matter, the task is independent.
void sensorsTaskStart();

// Thread-safe copy of the last known reading. Blocks for at most 50ms waiting
// for the mutex; if it can't get it in time, returns a zeroed struct.
SensorReading sensorsGetLatest();

// Wakes sensorsTask up immediately, without waiting for the rest of the
// normal sampling interval. Safe to call from any HTTP handler - it just does
// xTaskNotifyGive() on the task handle, it doesn't touch currentState
// directly. Non-blocking: it only "wakes" the task, it doesn't wait for it to finish.
void sensorsRequestRefresh();
