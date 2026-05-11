---
title: "Photoelectric Sensor Counting + 5ml Auto-Stop Pump"
description: "Add tube counting via photoelectric sensor with reset button, and auto-stop pump at 5ml with LCD display"
status: completed
priority: P2
effort: 2h
branch: main
tags: [arduino, photoelectric-sensor, pump-control, lcd]
created: 2026-05-02
---

# Photoelectric Sensor Counting + 5ml Auto-Stop Pump Plan

## Requirements

1. **Photoelectric sensor** - count +1 each time a tube passes detected
2. **Push button** - reset tube counter to 0 when pressed
3. **5ml auto-stop pump** - stop pump automatically when pumpVolume >= 5ml
4. **LCD display** - show tube count + pump volume (4th row)

## Architecture

- Photoelectric sensor: digital input pin (use interrupt for edge detection)
- Reset button: digital input pin with pull-down resistor (or internal pullup)
- Pump control: existing pumpDir + pumpPWM pins
- Volume tracking: existing pumpVolume variable (calculated from runtime × FLOW_RATE)
- LCD: existing 16x4 I2C display (rows 0-3)

## Display Format (16x4)

| Line | Content |
|------|---------|
| 0 | Speed: XXX |
| 1 | Motor: XXXX |
| 2 | PumpVol: X.XX ml |
| 3 | Tubes: XX |

## Phases

| # | Phase | Status | Effort | Link |
|---|-------|--------|--------|------|
| 1 | Photoelectric sensor + tube counter | pending | 1h | [phase-01](./phase-01-sensor-counter.md) |
| 2 | 5ml auto-stop pump + LCD tube count | pending | 1h | [phase-02](./phase-02-auto-stop.md) |

## Dependencies

- Existing: LCD 16x4 I2C, pump L298N, potentiometer
- New: Photoelectric sensor (D10/Y+), push button (A0/Abort)
- Library: none (arduino built-in)

## Risk Assessment

- Sensor bounce: use proper debouncing (software delay or hardware RC)
- Flow rate calibration: FLOW_RATE constant may need tuning for accuracy
- 5ml precision: depends on pump consistency and calibration

## Unresolved Questions

- What pin for photoelectric sensor? Will assign default (D7)
- What pin for reset button? Will assign default (D6)
- Debounce delay: 50ms typical for mechanical button