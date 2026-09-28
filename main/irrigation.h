#pragma once
#include <Arduino.h>
#include <time.h>       // time_t, struct tm, mktime() - for the INTERVAL trigger (real time, not millis())
#include "sensors.h"    // SensorReading

// Data model for the irrigation modes. See docs/irrigation-modes.md for the
// full design (the trigger/threshold axes) before touching this.

// What triggers irrigation. THRESHOLD is the only case where crossing the
// threshold itself replaces the clock as the trigger - for the other three,
// "when" and "whether to respect sensors" are independent decisions (see
// respectThresholds).
enum TriggerType {
  MANUAL,
  SCHEDULE,
  INTERVAL,
  THRESHOLD
};

struct IrrigationConfig {
  TriggerType trigger;

  // Only relevant for SCHEDULE and INTERVAL (MANUAL is controlled by hand;
  // THRESHOLD already IS the threshold, there's no "respect or not" to decide).
  //   true  -> starts only if soilMoisturePercent < startThreshold,
  //            and stops when soilMoisturePercent > stopThreshold
  //            (not on a fixed duration)
  //   false -> "dumb" mode: irrigates for fixedDurationMs, regardless
  //            of what the sensors say
  bool respectThresholds;

  float startThreshold;       // soil moisture % below which it starts
  float stopThreshold;        // soil moisture % above which it stops
  uint32_t fixedDurationMs;   // used when respectThresholds == false

  uint32_t intervalSeconds;   // only for trigger == INTERVAL - period between waterings, in SECONDS
                               // (not ms: we compare against time_t, which is always in seconds)

  // Only for trigger == SCHEDULE - phone-alarm style: days of the week + a fixed time.
  uint8_t scheduleDaysMask;   // bit 0 = Sunday, bit 1 = Monday, ... bit 6 = Saturday (same index as tm_wday)
  uint8_t scheduleHour;       // 0-23
  uint8_t scheduleMinute;     // 0-59
};

// --- Safety timeout ---
//
// A safety net valid for ANY mode, on top of all the IrrigationConfig
// logic - never bypassable through user configuration. If the valve/pump
// stays on longer than this, irrigationTask turns it off by itself,
// regardless of trigger or thresholds. Protects against a faulty sensor
// (stuck returning a low value) or a failure in another electronic
// component (a stuck relay, etc.) - without this, such a fault would
// irrigate indefinitely.
// static const uint32_t MAX_IRRIGATION_DURATION_MS = 5UL * 60UL * 1000UL; // 5 minutes for large areas
static const uint32_t MAX_IRRIGATION_DURATION_MS = 30UL * 1000UL; // 30 seconds for potted plants

// --- Timezone (NTP) ---
//
// Passed to configTzTime() in irrigationSyncTime() (see main.ino, called
// after WiFi connects). String in POSIX TZ format - the "M3.5.0/1,M10.5.0"
// part tells the ESP32 WHEN to switch daylight saving time on its own.
static const char* IRRIGATION_TZ = "WET0WEST,M3.5.0/1,M10.5.0";   // Portugal, with automatic DST
// Brazil hasn't observed daylight saving time since 2019 - fixed offset, simpler:
// static const char* IRRIGATION_TZ = "<-03>3";   // Brasilia (swap for the line above)

// What hour of the day (0-23) the INTERVAL trigger is "anchored" to - see
// irrigationSyncTime() and isIntervalDue() in irrigation.cpp.
static const uint8_t INTERVAL_ANCHOR_HOUR = 5;

// --- Pure decision logic (no RTOS, no hardware - just calculation) ---
//
// Deliberately split into two functions: "start" and "stop" use different
// thresholds (startThreshold vs stopThreshold) - that's exactly what
// prevents the valve from oscillating on/off/on/off when soil moisture
// hovers near a single boundary value.

// Decides whether irrigation should START, given that the trigger
// (schedule, interval, manual, or the threshold itself) has already
// fired/is active.
//   respectThresholds == false -> always starts (the "when" decision was
//     entirely up to the trigger, this is the "dumb" mode)
//   respectThresholds == true  -> only starts if the soil is dry enough
//     (soilMoisturePercent < startThreshold)
bool shouldStartIrrigation(const IrrigationConfig& config, const SensorReading& reading);

// Decides whether an irrigation already in progress should STOP.
//   The safety timeout (MAX_IRRIGATION_DURATION_MS) always takes priority,
//      regardless of mode - never bypassable through configuration.
//   respectThresholds == false -> stops exactly after fixedDurationMs
//   respectThresholds == true  -> stops once the soil is already moist
//      enough (soilMoisturePercent > stopThreshold)
bool shouldStopIrrigation(const IrrigationConfig& config, const SensorReading& reading, uint32_t currentDurationMs);

// Decides whether enough real time has passed since the last irrigation for
// the INTERVAL trigger to fire. Pure (doesn't call time() internally) so it
// can be tested with plain numbers - irrigationTask is what reads the real
// clock.
//   nowEpoch          -> "now", in seconds since 1970 (time(NULL))
//   lastStartedEpoch   -> when the last irrigation started, same unit
bool isIntervalDue(const IrrigationConfig& config, time_t nowEpoch, time_t lastStartedEpoch);

// Decides whether NOW is the time set in the SCHEDULE trigger's "alarm".
// Pure - knows nothing about "already fired today" (that's irrigationTask's
// responsibility, see lastScheduleFiredDayKey in irrigation.cpp).
//   currentWday   -> day of the week now (tm_wday: 0=Sunday ... 6=Saturday)
//   currentHour   -> hour now (tm_hour: 0-23)
//   currentMinute -> minute now (tm_min: 0-59)
bool isScheduleDue(const IrrigationConfig& config, uint8_t currentWday, uint8_t currentHour, uint8_t currentMinute);

// --- Valve/pump pin ---
//
// For now it's the same pin as LED_BUILTIN, because there's no irrigation
// hardware assembled yet (just the board, no MOSFET or valve). Swap for a
// dedicated GPIO once the real hardware is connected - see
// docs/decisions/0002-mosfet-flyback-diode.md.
static const uint8_t IRRIGATION_VALVE_PIN = LED_BUILTIN;

// --- Actuation task + thread-safe state access ---
//
// Just as sensorsTask is the sole owner of currentState, irrigationTask is
// the only thing in the firmware that writes to the valve/pump pin. Nothing
// else - no HTTP handler, no other task - should call digitalWrite() on
// that pin directly.

struct IrrigationStatus {
  bool valveOn;             // is the valve/pump on right now?
  uint32_t startedAtMs;    // millis() of when the current irrigation started (0 if inactive)
};

// Creates the mutex and starts irrigationTask, pinned to core 0. Call once,
// in setup(), BEFORE WiFi connects (doesn't depend on the network).
void irrigationTaskStart();

// Asks NTP for the real time and anchors the INTERVAL trigger to
// INTERVAL_ANCHOR_HOUR. Call once, in setup(), AFTER WiFi connects (needs
// the network). Until this function runs successfully, the INTERVAL trigger
// stays disabled (never fires) - see the timeSynced flag in irrigation.cpp.
void irrigationSyncTime();

// Requests turning irrigation on/off manually (MANUAL trigger). Safe to call
// from any HTTP handler - it only updates the shared request, inside the
// mutex; irrigationTask, on its own cycle, is what actually drives the pin.
// The safety timeout always has the final word: if it fires, the request is
// forced to false and irrigation has to be requested on again.
void irrigationSetManual(bool turnOn);

// Thread-safe copy of the current irrigation state (for the dashboard/API,
// Step 16). Blocks for at most 50ms waiting for the mutex; if it can't get
// it in time, returns a zeroed struct (equivalent to "inactive").
IrrigationStatus irrigationGetStatus();
