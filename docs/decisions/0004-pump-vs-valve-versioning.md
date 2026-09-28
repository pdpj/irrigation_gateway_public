# ADR 0004 — Two product versions: pump (v1) vs. valve (v2)

## Context

The project covers two real usage scenarios: potted plants with their own reservoir, and a flower bed/garden connected to the house's water supply. They're physically different — one generates flow, the other controls an already-existing flow.

## Options considered

- **A single version**, picking between pump or valve up front.
- **Two versions documented in parallel (v1/v2)**, with v1 as the initial validation target (simpler, cheaper) and v2 as a planned extension.

## Decision

Two versions, with v1 (pump, reservoir) as the implementation priority, and v2 (valve, water supply) as the next phase, documented but not implemented first.

## Rationale

v1 is simpler and cheaper (~€35–50 vs. an additional ~€22–39 for v2) and uses lower voltage (5V vs. 12V), reducing risk while the firmware and drive-electronics fundamentals are still being validated. v2 reuses the same drive topology validated in v1 (MOSFET + flyback diode), just resized — it's not a from-scratch project, it's a natural extension.

## Consequences

The hardware is being designed from the start to support both versions on a single configurable PCB (optional footprints for the 5V/pump path and the 12V/valve path, power jumper) instead of two separate boards — see `docs/hardware.md`.
