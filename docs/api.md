# REST API

Served by the ESP32 itself (`AsyncWebServer`, port 80). All data responses are JSON.

## `GET /`

Dashboard HTML — cards with sensor readings (auto-refreshed every 2s via polling) and irrigation control buttons.

## `POST /led`

Turns the irrigation valve/pump on or off (trigger MANUAL). Route name and JSON field (`ledState`) kept for backward compatibility with the dashboard — this no longer controls a literal LED, see `docs/irrigation-modes.md`.

**Request body**
```json
{ "state": true }
```

**Response — 200 OK**
```json
{ "ledState": true }
```

**Response — 400 Bad Request** (invalid JSON)
```json
{ "error": "Invalid JSON" }
```

## `POST /api/sensors/refresh`

Requests an immediate sensor sample, without waiting for the normal sampling interval. Returns right away — it doesn't wait for the reading to complete; poll `GET /api/status` afterward for the fresh value.

**Response — 202 Accepted**
```json
{ "status": "refresh requested" }
```

## `GET /api/status`

Current system state — sensor readings (currently simulated — see `docs/hardware.md`) and valve status.

**Response — 200 OK**
```json
{
  "soilMoisturePercent": 42.5,
  "airTemperatureC": 23.4,
  "airHumidityPercent": 58.0,
  "sensorAgeMs": 1234,
  "ledState": false,
  "uptimeMs": 12345
}
```

`sensorAgeMs` — how long ago (ms) the returned reading was sampled, relative to when the ESP32 answered the request; used by the dashboard to compute the real sampling time.
`uptimeMs` — time since the last boot (`millis()`); useful to detect unexpected restarts (e.g. brownout).

## Planned

- `GET /api/irrigation` / `POST /api/irrigation` — read and configure the irrigation mode (trigger type, thresholds, interval, schedule). See `docs/irrigation-modes.md` and Step 16 in `docs/roadmap.md`.
