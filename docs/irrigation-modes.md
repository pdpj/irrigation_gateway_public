# Irrigation modes

Planning document for the firmware's next phase: moving from manual
irrigation (already implemented) to the various automatic triggering
modes. This is the architecture sketch written before implementation
started, to keep the thread across sessions.

## The two axes of the problem

The initial brainstorm had 6 separate "modes", but in practice they're
the combination of two orthogonal axes.

**Axis 1 — what triggers irrigation (trigger):**

- **Manual** — the user turns it on/off directly (already implemented).
- **Schedule** — one or more fixed times of day (e.g. 07:00 and 19:00).
  Requires real, synced time (NTP), since the ESP32 has no battery-backed
  clock of its own.
- **Interval** — every X hours, from a reference point (`millis()` since
  the last irrigation — no real clock needed).
- **Pure threshold** — crossing the moisture threshold itself IS the
  trigger, no time involved at all.

**Axis 2 — what controls duration, once triggered** (only applies to
Schedule and Interval; Manual is controlled by hand and pure Threshold
already IS the threshold):

- **Respect sensors** (toggle on): starts only if
  `soilMoisture < startThreshold`, and stops when
  `soilMoisture > stopThreshold` (not on a fixed duration).
- **Ignore sensors — "dumb" mode** (toggle off): irrigates for a fixed
  duration, regardless of what the sensors say.

Consequence: the "mixed rule" and "dumb irrigation" items from the
original brainstorm aren't separate modes — they're Schedule/Interval
with the "respect sensors" toggle on or off, respectively.


## Data model (draft)

```cpp
enum TriggerType { MANUAL, SCHEDULE, INTERVAL, THRESHOLD };

struct IrrigationConfig {
  TriggerType trigger;
  bool respectThresholds;      // only relevant for SCHEDULE and INTERVAL
  float startThreshold;        // soil moisture % below which it starts
  float stopThreshold;         // soil moisture % above which it stops
  uint32_t fixedDurationMs;    // used when respectThresholds == false
  // + mode-specific fields: schedule list (SCHEDULE) or interval in
  // hours (INTERVAL) — defined once we reach those steps
};
```

## Safety timeout

Regardless of mode, irrigation must never run indefinitely — if a sensor
fails and gets stuck returning a low value, or some electronic component
fails, the valve/pump has to turn itself off after a maximum duration
(`MAX_IRRIGATION_DURATION_MS`). This is a safety net that sits *on top
of* any mode logic — never bypassable through configuration.

## Implementation order

Built in layers, each one introducing only one new concept:

1. **Data model + safety timeout** — the `enum`/`struct` above, plus the
   maximum-duration constant. Pure C, no RTOS or hardware.
2. **Pure decision logic** — a `decideIrrigation(config, reading,
   irrigationActive)` function that returns start/stop/keep. Testable in
   isolation, no concurrency.
3. **`irrigationTask`** — a new FreeRTOS task that uses
   `sensorsGetLatest()`, runs the logic from step 2, and turns the
   pump/valve pin on/off, with the safety timeout always active.
4. **Real Manual trigger** — the LED placeholder starts actually
   controlling the valve/pump via `irrigationTask`.
5. **Interval trigger** — the simplest of the time-based ones, just
   `millis()`.
6. **Schedule trigger** — the most complex: needs real-time NTP sync.
7. **Pure Threshold + "respect sensors" toggle** — combines the pieces
   already built in the previous steps.
8. **REST API + UI** — expose everything on `/api/irrigation` and the
   dashboard, last, once the logic is solid.
