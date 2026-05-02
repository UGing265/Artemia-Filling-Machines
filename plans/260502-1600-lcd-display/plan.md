---
title: "Add LCD 16x4 I2C Display to Arduino Code"
description: "Add LCD 16x4 I2C (PCF8574) display showing pump speed, motor rotation count, and pump volume"
status: completed
priority: P2
effort: 2h
branch: main
tags: [arduino, lcd, embedded]
created: 2026-05-02
---

# LCD Display Implementation Plan

## Overview

Add LCD 16x4 I2C display to show pump speed (PWM 0-255), motor rotation count, and estimated pump output volume. Track stepper steps for rotation counting, track pump runtime for volume estimation.

## Display Format (16x4)

| Line | Content |
|------|---------|
| 1 | Speed: XXX |
| 2 | Motor: XXXX |
| 3 | PumpVol: XXXX |
| 4 | (reserved/spare) |

## Components

- LCD: I2C PCF8574 backpack, address 0x27, 16 cols x 4 rows
- Library: LiquidCrystal_I2C
- Variables: motorSteps (step counter), pumpVolume (estimated ml), currentSpeed

## Phases

| # | Phase | Status | Effort | Link |
|---|-------|--------|--------|------|
| 1 | Add LCD library + init | Pending | 30m | [phase-01](./phase-01-lcd-init.md) |
| 2 | Add LCD display logic | Pending | 1h | [phase-02](./phase-02-lcd-display.md) |
| 3 | Verify compile | Pending | 30m | [phase-03](./phase-03-verify.md) |

## Dependencies

- Original stepper+pump code unchanged
- LiquidCrystal_I2C library required

## Notes

- LCD 16x4 has 4 rows (vs typical 16x2 which has 2 rows)
- Pump volume estimated from runtime × flow rate constant