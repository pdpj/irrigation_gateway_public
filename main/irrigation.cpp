#include "irrigation.h"

bool shouldStartIrrigation(const IrrigationConfig& config, const SensorReading& reading) {
  if (!config.respectThresholds) {
    return true;
  }
  return reading.soilMoisturePercent < config.startThreshold;
}

bool shouldStopIrrigation(const IrrigationConfig& config, const SensorReading& reading, uint32_t currentDurationMs) {
  // Safety net - always the first thing checked, never bypassable.
  if (currentDurationMs >= MAX_IRRIGATION_DURATION_MS) {
    return true;
  }

  if (!config.respectThresholds) {
    return currentDurationMs >= config.fixedDurationMs;
  }

  return reading.soilMoisturePercent > config.stopThreshold;
}

bool isIntervalDue(const IrrigationConfig& config, time_t nowEpoch, time_t lastStartedEpoch) {
  return (nowEpoch - lastStartedEpoch) >= (time_t)config.intervalSeconds;
}

bool isScheduleDue(const IrrigationConfig& config, uint8_t currentWday, uint8_t currentHour, uint8_t currentMinute) {
  bool dayMatches = config.scheduleDaysMask & (1 << currentWday);   // is today one of the chosen days?
  bool timeMatches = (currentHour == config.scheduleHour) && (currentMinute == config.scheduleMinute);
  return dayMatches && timeMatches;
}

// --- Actuation task + thread-safe state access ---

// { trigger, respectThresholds, startThreshold, stopThreshold, fixedDurationMs, intervalSeconds, scheduleDaysMask, scheduleHour, scheduleMinute }
static IrrigationConfig currentConfig = { SCHEDULE, false, 20.0f, 80.0f, 5000, 120, 0x7F, 16, 35 }; // TEST Step 14: every day (0x7F) at HH:MM, stays on for 5s
static bool valveOn = false;                       // is the valve/pump on right now?
static uint32_t startedAtMs = 0;                   // millis() of when the current irrigation started (0 = inactive)
static time_t lastStartedEpoch = 0;                 // REAL time (epoch) of when the current/last irrigation started
static bool timeSynced = false;                     // only true after irrigationSyncTime() succeeds
static int lastScheduleFiredDayKey = -1;             // year*1000+day-of-year of the last time SCHEDULE fired
static bool manualRequestOn = false;                // raw MANUAL trigger request, via irrigationSetManual()
static SemaphoreHandle_t irrigationMutex = NULL;    // protects the variables above

static void irrigationTask(void* parameter) {
  for (;;) {
    SensorReading reading = sensorsGetLatest();     // most recent sensor reading
    uint32_t now = millis();                        // "now" in millis() - duration/timeout (short term)
    time_t nowEpoch;                                 // "now" in real time - only valid if timeSynced
    time(&nowEpoch);

    if (xSemaphoreTake(irrigationMutex, portMAX_DELAY) == pdTRUE) {
      bool wasValveOn = valveOn;                                          // state BEFORE deciding anything
      uint32_t currentDurationMs = wasValveOn ? (now - startedAtMs) : 0; // how long it's been on (0 if it isn't)

      if (currentConfig.trigger == MANUAL) {
        if (wasValveOn && currentDurationMs >= MAX_IRRIGATION_DURATION_MS) {
          manualRequestOn = false;   // timeout: forces the REQUEST to false, not just the valve
        }
        valveOn = manualRequestOn;    // manual doesn't use thresholds - follows the request directly
      } else if (currentConfig.trigger == INTERVAL) {
        // only considers starting if the time is already synced - without
        // that nowEpoch/lastStartedEpoch don't mean anything (epoch 0 = 1970)
        bool intervalDue = timeSynced && isIntervalDue(currentConfig, nowEpoch, lastStartedEpoch);
        if (!wasValveOn && intervalDue && shouldStartIrrigation(currentConfig, reading)) {
          valveOn = true;             // interval elapsed and sensors allow it (or we don't respect sensors)
        } else if (wasValveOn && shouldStopIrrigation(currentConfig, reading, currentDurationMs)) {
          valveOn = false;            // was on and the stop condition was met
        }
      } else if (currentConfig.trigger == SCHEDULE) {
        bool scheduleDue = false;            // by default, today isn't the scheduled time
        int todayKey = -1;                    // only meaningful if timeSynced/getLocalTime succeed
        struct tm timeinfo;
        // timeout 0 - only READS the time already synced by irrigationSyncTime(), never blocks the task waiting for NTP
        if (timeSynced && getLocalTime(&timeinfo, 0)) {
          todayKey = timeinfo.tm_year * 1000 + timeinfo.tm_yday;   // identifies today, unambiguous across years
          bool alarmTime = isScheduleDue(currentConfig, timeinfo.tm_wday, timeinfo.tm_hour, timeinfo.tm_min);
          scheduleDue = alarmTime && (todayKey != lastScheduleFiredDayKey);   // only true the FIRST time it matches today
        }

        if (!wasValveOn && scheduleDue && shouldStartIrrigation(currentConfig, reading)) {
          valveOn = true;                       // scheduled time hit, hadn't fired yet today
          lastScheduleFiredDayKey = todayKey;    // marks "fired today" - only now, only when it actually turns on
        } else if (wasValveOn && shouldStopIrrigation(currentConfig, reading, currentDurationMs)) {
          valveOn = false;            // was on and the stop condition was met
        }
      } else {
        // Step 15 (pure THRESHOLD) not wired up here yet.
        if (!wasValveOn && shouldStartIrrigation(currentConfig, reading)) {
          valveOn = true;             // was off and the start condition was met
        } else if (wasValveOn && shouldStopIrrigation(currentConfig, reading, currentDurationMs)) {
          valveOn = false;            // was on and the stop condition was met
        }
      }

      if (valveOn && !wasValveOn) {
        startedAtMs = now;         // off -> on transition: marks the start (millis(), for duration/timeout)
        lastStartedEpoch = nowEpoch; // also marks the start in real time - INTERVAL counts from here
      } else if (!valveOn) {
        startedAtMs = 0;     // it's off (already was, or just became): reset
      }

      digitalWrite(IRRIGATION_VALVE_PIN, valveOn ? HIGH : LOW);   // the only place in the firmware that writes to this pin
      xSemaphoreGive(irrigationMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(500));   // check interval - doesn't need to be as fast as the sensors
  }
}

void irrigationTaskStart() {
  irrigationMutex = xSemaphoreCreateMutex();   // create the mutex before the task can run

  pinMode(IRRIGATION_VALVE_PIN, OUTPUT);       // configure the pin as output
  digitalWrite(IRRIGATION_VALVE_PIN, LOW);     // initial state: off

  xTaskCreatePinnedToCore(
    irrigationTask, "IrrigationTask",
    4096,           // stack (words)
    NULL,           // no parameter
    1,              // priority
    NULL,           // handle discarded - nothing needs to notify this task for now
    0               // core 0 - same core as sensorsTask
  );
}

void irrigationSyncTime() {
  configTzTime(IRRIGATION_TZ, "pool.ntp.org");   // requests NTP sync (doesn't block by itself)

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {   // waits up to 10s for the NTP response
    Serial.println("Failed to sync time via NTP - INTERVAL trigger stays disabled.");
    return;   // timeSynced stays false - irrigationTask never fires INTERVAL
  }

  time_t nowEpoch;
  time(&nowEpoch);   // "now", in seconds since 1970, with the timezone already applied

  // Anchor: finds the last time INTERVAL_ANCHOR_HOUR:00 hit (today, or
  // yesterday if that hour hasn't arrived yet today). 24h is a multiple of
  // INTERVAL_ANCHOR_HOUR*3600, so any interval that's a multiple of 24h
  // (e.g. 48h) always lands back on this same hour, with no need to re-anchor.
  timeinfo.tm_hour = INTERVAL_ANCHOR_HOUR;
  timeinfo.tm_min = 0;
  timeinfo.tm_sec = 0;
  time_t anchorEpoch = mktime(&timeinfo);   // converts the (now modified) struct tm back to epoch
  if (anchorEpoch > nowEpoch) {
    anchorEpoch -= 24UL * 60UL * 60UL;   // today's hour hasn't arrived yet - use yesterday's
  }

  if (xSemaphoreTake(irrigationMutex, portMAX_DELAY) == pdTRUE) {
    lastStartedEpoch = anchorEpoch;   // the next INTERVAL counts from here
    timeSynced = true;                 // only now can the INTERVAL trigger fire
    xSemaphoreGive(irrigationMutex);
  }

  Serial.print("Time synced, current epoch: ");
  Serial.println((long)nowEpoch);
}

void irrigationSetManual(bool turnOn) {
  if (xSemaphoreTake(irrigationMutex, portMAX_DELAY) == pdTRUE) {
    manualRequestOn = turnOn;   // only updates the request - the task decides the pin, on its own cycle
    xSemaphoreGive(irrigationMutex);
  }
}

// Not called by anything in the firmware yet - ready for Step 16 (API).
IrrigationStatus irrigationGetStatus() {
  IrrigationStatus status = { false, 0 };   // value returned if the mutex doesn't come in time
  if (xSemaphoreTake(irrigationMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    status.valveOn = valveOn;
    status.startedAtMs = startedAtMs;
    xSemaphoreGive(irrigationMutex);
  }
  return status;
}
