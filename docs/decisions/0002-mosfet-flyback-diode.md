# ADR 0002 — Drive topology: MOSFET + gate resistor + flyback diode

## Context

The pump (and later the valve) are inductive loads drawing more current than an ESP32 GPIO can supply directly. A safe drive circuit is needed, and the inductive load generates a voltage spike when switched off that can destroy the switching element if left untreated.

## Options considered

- **Electromechanical relay** — galvanic isolation, but mechanical wear per cycle, slower, no practical PWM, and has the same flyback problem in the relay's own coil.
- **Logic-level N-channel MOSFET** — fast switching, no mechanical wear, allows PWM (future intensity control), but needs external protection against the inductive spike.
- **SSR (solid-state relay)** — isolation without mechanical wear, but higher cost and overkill for the current involved here.

## Decision

N-channel MOSFET (e.g. AO3400), with:
- A series gate resistor (limits gate charge/discharge current, controls switching speed).
- A gate pull-down resistor (guarantees an off state by default during boot/reset, before the GPIO takes control).
- A Schottky diode (e.g. 1N5819) in parallel with the load, cathode to the positive rail — conducts only during the reverse spike when switching off, giving the energy stored in the motor/coil's inductance a path to dissipate without destroying the MOSFET.

## Rationale

A MOSFET is the right choice for this case (low/medium current, no need for galvanic isolation, future interest in PWM for intensity control). The real risk — the inductive voltage spike on turn-off — is well known and treatable with a freewheeling diode, standard practice when driving inductive loads with a MOSFET.

## Validation

Topology validated by SPICE simulation (ngspice, via KiCad) before any component purchase:
- VIN sag during inrush kept within a safe margin against brownout.
- Drain voltage spike on turn-off, without protection: high enough to be a concern. With the diode: clamped to ~5.7V, well below the MOSFET's rating (~30V).

## Consequences

The same topology is reused for the 12V valve (v2), with resized values — see `docs/hardware.md`. The simulation needs to be repeated with the new values before manufacturing any board (still pending).
