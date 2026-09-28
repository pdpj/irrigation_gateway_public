# Irrigation Gateway

Automated irrigation gateway built on the Arduino Nano ESP32 (ESP32-S3) — a learning and portfolio project covering embedded firmware, power electronics, and a REST API for remote control.

## Why this project

My parents travel a lot, and every trip means finding someone to water their plants — sometimes for over a month at a time, when they come visit Europe. That's the actual problem behind this project: an irrigation system that keeps the plants alive on its own while they're away.

It's being built in phases:

- **v1** — potted plants, fed from a water reservoir (the current hardware track: 5V pump).
- **v2** — larger areas fed from mains water, like a lawn or garden (12V valve).

It's also a deliberate learning project, split into three tracks that converge on the same product:

- **Firmware / RTOS** — Arduino Nano ESP32 running on FreeRTOS, an asynchronous HTTP server, REST API.
- **Hardware / Electronics** — hands-on with KiCad end to end (schematic capture, SPICE simulation, PCB layout), validating design choices before any purchase — e.g. the pump driver circuit (MOSFET switching an inductive load, with flyback protection against voltage spikes).
- **Web / Remote access** — a control interface accessible from a browser, evolving from a page served by the ESP32 itself toward a real remote-access architecture.

## Current status

| Track | Status |
|---|---|
| Firmware | HTTP server up and running. Irrigation modes implemented: Manual, Interval and Schedule (via NTP, with timezone and daylight saving time handled) — pure Threshold and the configuration API (`/api/irrigation`) are still missing. Safety timeout always active, independent of the mode, as a fail-safe against a faulty sensor or electronic failure. Sensors are still simulated (fixed values) — see `docs/hardware.md`. Full design of the modes in [`docs/irrigation-modes.md`](docs/irrigation-modes.md) |
| Hardware | v1 (5V pump) validated by SPICE simulation (inrush + flyback). v2 (12V valve) designed, simulation pending |
| Web / Remote access | Under decision — see [`docs/architecture.md`](docs/architecture.md) |

Full roadmap in [`docs/roadmap.md`](docs/roadmap.md).

## Quick start

1. Copy `main/config.example.h` to `main/config.h` and fill in `WIFI_SSID`/`WIFI_PASSWORD` (`config.h` is not versioned — see `.gitignore`).
2. Open `main/main.ino` in the Arduino IDE (or CLI) with the `Arduino Nano ESP32` board selected.
3. Build and upload.
4. Read the IP printed on Serial (115200 baud) — NTP sync is also reported there.
5. Open `http://<IP>/` in the browser — dashboard with sensor readings (simulated) and manual irrigation control (`Turn ON` / `Turn OFF`).
6. Test the API directly: `curl http://<IP>/api/status`.

Full API reference in [`docs/api.md`](docs/api.md).

## Repository structure

```
irrigation_gateway/
├── main/
│   ├── main.ino             # setup(): starts the tasks, connects WiFi/NTP, defines the HTTP routes
│   ├── config.example.h     # template for config.h (WiFi) - copy and fill in, not versioned
│   ├── sensors.h / .cpp     # sensorsTask - periodic sensor reading (currently simulated)
│   ├── irrigation.h / .cpp  # irrigationTask - irrigation-mode logic + valve/pump actuation
│   ├── api_handlers.h / .cpp # REST route handlers
│   └── web_page.h            # dashboard HTML/CSS/JS embedded in the firmware
├── Hardware/                 # KiCad project (schematic, SPICE simulation, layout)
└── docs/
    ├── architecture.md       # system architecture, local-server vs remote-broker decision
    ├── hardware.md            # BOM, electrical schematic, simulation results
    ├── api.md                 # REST endpoint reference
    ├── irrigation-modes.md    # design of the 4 irrigation modes (trigger x "respect sensors")
    ├── roadmap.md             # the 3 tracks, in detail
    └── decisions/             # ADRs - justified engineering decisions
```

## AI assistance

This project is developed with Claude (Anthropic) as a pair-programming and learning partner: architecture trade-offs, RTOS design decisions and debugging are worked through in conversation first, with the implementation following from those decisions.