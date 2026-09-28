#include "sensors.h"

float readSoilMoisture() {
  return 42.5;   // % — example value, "moderately moist" soil
}

float readAirTemperature() {
  return 23.4;   // degrees Celsius
}

float readAirHumidity() {
  return 58.0;   // %
}

// --- Sampling task + thread-safe state access ---

static SensorReading currentState = {0, 0, 0, 0};
static SemaphoreHandle_t stateMutex = NULL;
static TaskHandle_t sensorsTaskHandle = NULL;   // kept for sensorsRequestRefresh()

static const TickType_t SAMPLING_INTERVAL = pdMS_TO_TICKS(10000); // 10s

static void sensorsTask(void* parameter) {
  for (;;) {
    SensorReading newReading;
    newReading.soilMoisturePercent = readSoilMoisture();
    newReading.airTemperatureC     = readAirTemperature();
    newReading.airHumidityPercent  = readAirHumidity();
    newReading.sampledAtMs         = millis();

    if (xSemaphoreTake(stateMutex, portMAX_DELAY) == pdTRUE) {
      currentState = newReading;
      xSemaphoreGive(stateMutex);
    }

    Serial.printf("[sensorsTask/core%d] soil=%.1f%% temp=%.1fC airHum=%.1f%%\n",
                  xPortGetCoreID(), newReading.soilMoisturePercent, newReading.airTemperatureC, newReading.airHumidityPercent);

    // Waits for the normal interval OR an xTaskNotifyGive() coming from
    // sensorsRequestRefresh() - whichever comes first. pdTRUE clears the
    // pending notification count on exit (binary-semaphore-like behavior).
    ulTaskNotifyTake(pdTRUE, SAMPLING_INTERVAL);
  }
}

void sensorsTaskStart() {
  stateMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    sensorsTask, "SensorsTask",
    4096,
    NULL,
    1,
    &sensorsTaskHandle,   // kept, so it can be notified by sensorsRequestRefresh()
    0   // core 0 — separate from the core where loop()/Arduino runs by default (core 1)
  );
}

SensorReading sensorsGetLatest() {
  SensorReading copy = {0, 0, 0, 0};
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    copy = currentState;
    xSemaphoreGive(stateMutex);
  }
  return copy;
}

void sensorsRequestRefresh() {
  if (sensorsTaskHandle != NULL) {
    xTaskNotifyGive(sensorsTaskHandle);
  }
}
