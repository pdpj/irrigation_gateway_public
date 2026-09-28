# Roadmap

Three work tracks, treated as independent micro-tasks.

## Firmware / RTOS

- [x] Step 1 — project setup, `arduino:esp32:nano_nora` board
- [x] Step 2 — asynchronous HTTP server up
- [x] Step 3 — LED control via `POST /led`
- [x] Step 4 — simulated sensors + `GET /api/status`
- [x] Step 5 — web page with polling (auto-refresh every 2s)
- [ ] Step 6 — irrigation schedule endpoint (`GET`/`POST /api/agenda`)
- [ ] Step 7 — automatic schedule-based triggering logic
- [ ] Step 8 — real sensor integration (replace simulated reading)
- [x] Step 9 — irrigation modes: data model + safety timeout (see `docs/irrigation-modes.md`)
- [x] Step 10 — irrigation modes: pure decision logic (thresholds/hysteresis)
- [x] Step 11 — irrigation modes: `irrigationTask` (drives the valve/pump)
- [x] Step 12 — irrigation modes: real Manual trigger (replace LED placeholder)
- [x] Step 13 — irrigation modes: Interval trigger
- [x] Step 14 — irrigation modes: Schedule trigger (requires NTP)
- [ ] Step 15 — irrigation modes: pure Threshold + "respect sensors" toggle
- [ ] Step 16 — irrigation modes: REST API + UI (`/api/irrigation`)

## Hardware / Electronics

- [x] BOM and wiring diagram v1 (pump)
- [x] SPICE validation of the MOSFET + flyback topology (v1)
- [x] BOM and wiring diagram v2 (12V valve)
- [ ] SPICE validation of the topology resized for 12V/0.42A
- [ ] Power architecture decision (single supply + buck converter vs. two separate supplies)
- [ ] Breadboard assembly (v1)
- [ ] Configurable PCB layout (optional footprints for v1/v2)

## Web / Remote access

- [x] Preliminary architecture decision (server-on-ESP32 vs. broker) — see `docs/architecture.md`
- [ ] Prototype Architecture B with a free service (e.g. Adafruit IO)
- [ ] Define UI requirements for non-technical use (see ADR 0004)
- [ ] Own backend (if needed) — configuration and history storage
- [ ] Final web interface, accessible remotely
