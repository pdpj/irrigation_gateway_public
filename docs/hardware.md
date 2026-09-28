# Hardware

KiCad project in `Hardware/irrigation_project/` (schematic, SPICE simulation, layout).

## v1 — Reservoir + pump (5V)

Scenario: a 6L jug as reservoir, 5V submersible DC pump, driven via an N-channel MOSFET.

**Validated topology**: MOSFET (gate driven by a GPIO through a series resistor + pull-down) + Schottky flyback protection diode in parallel with the pump.

**Electrical validation (SPICE, before any purchase)**:
- Inrush current and supply sag test (ESP32 brownout risk) — VIN voltage kept within a safe margin with the simulated values.
- Drain voltage spike on turn-off (energy stored in the motor's inductance) clamped by the flyback diode to ~5.7V — well below the MOSFET's limit (~30V).
- Post-clamping RLC ringing identified and characterized as damped/benign.

See [ADR 0002](decisions/0002-mosfet-flyback-diode.md) for the topology's rationale.

Estimated cost: ~€35–50.

## v2 — Real garden + solenoid valve (12V)

Scenario: a tap/pressurized water line, a normally-closed solenoid valve instead of a pump.

Reuses the same MOSFET+diode topology from v1, resized for 12V/0.42A. SPICE simulation **not yet** repeated with the new values — pending before manufacturing any board.

ESP32 power supply decision (single 12V supply + buck converter, vs. two separate supplies) still open.

Estimated additional cost: ~€22–39.

## Sensors

- **Soil moisture** — capacitive (avoids electrode corrosion vs. resistive), 3.3V-safe analog output.
- **Air temperature/humidity** — DHT22. See [ADR 0003](decisions/0003-sensor-dht22.md) for why it was chosen.

Neither is physically connected yet — the firmware reads simulated values (`docs/api.md`).
