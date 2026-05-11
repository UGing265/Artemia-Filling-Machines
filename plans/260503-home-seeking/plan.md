---
title: "NEMA Homing Sequence with Limit Switch"
description: "Add home-seeking behavior on startup: slow 360° rotation, detect Z-limit switch, 10s wait + LCD activation, or timeout error message"
status: pending
priority: P1
effort: 2h
branch: main
tags: [feature, motor-control, arduino]
created: 2026-05-03
---

# NEMA Homing Sequence with Limit Switch

## Overview

Implement home-seeking behavior on system startup:
- NEMA motor rotates 360° slowly to find home position
- Limit switch connected to Z pin on CNC Shield V3
- If switch pressed → motor stops, wait 10s, LCD shows "ACTIVE", pump activates
- If home not found after 2 full rotations → LCD shows error message to power cycle

## Phases

| # | Phase | Status | Effort | Link |
|---|-------|--------|--------|------|
| 1 | Homing Implementation | Pending | 1h | [phase-01](./phase-01-homing.md) |
| 2 | Testing | Pending | 1h | - |

## Dependencies

- Requires `ArtemiaCode.ino` modification
- Uses existing LCD, stepper, and limit switch infrastructure

## Hardware Pinout

| Component | Pin | CNC Shield V3 |
|-----------|-----|---------------|
| NEMA Stepper (Step) | D4 | Z+ |
| NEMA Stepper (Dir) | D7 | Z- |
| Limit Switch (Z) | D9 | Z-Limit |

## Timing

- Slow rotation speed: ~3000µs pulse width (slower than current 3000+100)
- 2 rotations timeout = ~8 seconds at slow speed
- Home found → 10s wait → LCD "ACTIVE" → proceed to normal operation
