# ADR 0003 — Air temperature/humidity sensor: DHT22

## Context

The gateway needs to measure air temperature and humidity, in addition to soil moisture, to decide when to irrigate.

## Options considered

- **DHT22** — proprietary single-wire protocol, timing-critical, requires error handling (`NaN` readings are common) and a minimum ~2s interval between readings. Lower cost.
- **SHT31** — I2C, standard protocol, more robust against read errors, easier to integrate alongside other sensors on the same bus. Higher cost.

## Decision

Keep the DHT22.

## Rationale

The initial choice was a "common default" in maker projects, without a rigorous comparison between the two at the time. When questioned, the comparison was made and the SHT31 is technically superior (I2C more robust than the DHT22's proprietary protocol). The decision to keep the DHT22 was made consciously, based on per-unit cost, not for lack of a known better alternative.

## Consequences

The firmware needs to explicitly handle DHT22 read failures (`NaN` returns) and respect the minimum sampling interval — this isn't implemented yet in the current simulated reading and needs to go in when the real sensor is integrated.
