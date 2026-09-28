# ADR 0001 — ESP32 as HTTP server vs. client of a remote broker

## Context

The system needs to expose control (turning actuators on/off) and sensor readings to a client (browser). The gateway may, in the future, need access from outside the local network (use by non-technical third parties, remotely) and/or need to run on battery.

## Options considered

**A — ESP32 as its own web server** (`AsyncWebServer`, pull model: the client asks, the ESP32 answers).
**B — ESP32 as a client that publishes data to an external broker/backend** (push model: the ESP32 initiates communication; the backend, publicly accessible, serves the client).

## Decision

Use Architecture A for the current project phase.

## Rationale

The network in use is local, the gateway is always powered from an outlet, and the priority right now is to validate firmware and hardware quickly — A is simpler to implement and debug, with no extra infrastructure.

B remains planned as an evolution, not discarded: it solves two problems simultaneously that A doesn't handle well —

1. **Remote access** — A requires additional router configuration (port forwarding — a security risk, exposing a simple HTTP server without robust authentication to the internet — or a VPN/reverse tunnel) to be reachable from outside the local network. B is publicly accessible from the backend side by design, requiring nothing on the home network side.
2. **Power consumption** — keeping an HTTP server up requires the WiFi radio always ready to receive a connection (tens of mA even idle). That's incompatible with deep sleep (µA), ruling out battery operation. In B, the ESP32 controls when it wakes up and talks, and can sleep the rest of the time.

## Consequences

Migrating to B requires an extra, always-on infrastructure component (a hosted backend — cloud or a dedicated device). Before migrating, it's worth prototyping with a free service (e.g. Adafruit IO) to validate the push model without building a custom backend.

## When to revisit

As soon as either of these becomes a real requirement: access from outside the local network, or battery operation (e.g. a remote sensor with no outlet nearby).
