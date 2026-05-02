---
title: "Pump Calibration + Abort Button Fix"
description: "Add flow rate calibration mode and fix abort button to stop/reset pump"
status: pending
priority: P2
effort: 1h
branch: main
tags: [arduino, calibration, pump-control, bugfix]
created: 2026-05-02
---

# Pump Calibration + Abort Button Fix Plan

## Overview

Two fixes/improvements:
1. **Calibration mode**: Let user run pump for fixed time, input actual volume to auto-calculate FLOW_RATE
2. **Abort button fix**: When pressed, immediately stop pump and reset tube counter

## Requirements

### 1. Calibration Mode

- Enter calibration via long-press (>3s) on reset button during IDLE state
- During calibration:
  - Pump runs for 5 seconds at full speed
  - Display shows "CALIBRATING..."
  - After 5s, pump stops, display shows "ENTER ML:"
- User turns potentiometer to set measured volume (shown on LCD)
- Short-press to save and exit, long-press to cancel
- New FLOW_RATE saved to EEPROM so it persists across reboots

### 2. Abort Button Fix

- Current problem: reset button only resets `tubeCount`, not pump
- Fix: when Abort button pressed while pump running:
  - Stop pump immediately (analogWrite 0)
  - Reset `pumpRuntime`, `pumpVolume` to 0
  - Reset `tubeCount` to 0
  - Show "ABORTED" briefly on LCD
- When Abort pressed while IDLE: just reset tubeCount (existing behavior)

## Architecture

### State Machine

```
IDLE → (long-press 3s) → CALIBRATING → (5s elapsed) → CALIB_INPUT → (short-press) → IDLE
                   ↘ (long-press) ↗                              ↗ (long-press) ↗

IDLE → (pump running) → PUMPING → (done OR volume>=5ml OR abort) → IDLE
                  (abort button) → STOP_ABORT → IDLE
```

### New Variables

```cpp
// Calibration
float flowRateCalibrated = 0.5;  // loaded from EEPROM
const int CALIB_DURATION = 5000; // ms
const int CALIB_LONG_PRESS = 3000; // ms
bool calibMode = false;
unsigned long calibStartTime = 0;

// State enum
enum SystemState { IDLE, PUMPING, CALIBRATING, CALIB_INPUT };
SystemState systemState = IDLE;
```

### EEPROM Layout

| Address | Description |
|---------|-------------|
| 0 | 'A' magic byte (valid flag) |
| 1-4 | FLOW_RATE float |

## Display Format

### Calibration States

| State | Row 0 | Row 1 | Row 2 | Row 3 |
|-------|-------|-------|-------|-------|
| IDLE | Speed: XXX | Motor: XXXX | PumpVol: X.XX ml | Tubes: XX |
| CALIBRATING | CALIBRATING... | Time: Xs | Vol: X.XX ml | |
| CALIB_INPUT | ENTER ML: X.X | FLOW_RATE: X.XX | <-POT-> | SAVE=BTN |
| ABORTED | !! ABORTED !! | PumpVol: 0.00 ml | Tubes: 0 | (auto-return) |

## Phases

| # | Phase | Status | Effort | Link |
|---|-------|--------|--------|------|
| 1 | Calibration mode with EEPROM | pending | 30m | [phase-01](./phase-01-calibration.md) |
| 2 | Abort button stop/reset fix | pending | 30m | [phase-02](./phase-02-abort-fix.md) |

## Dependencies

- Existing: LCD 16x4 I2C, pump L298N, potentiometer, photoelectric sensor, reset button
- EEPROM library: `<EEPROM.h>` (Arduino built-in)
- No new hardware needed

## Risk Assessment

- EEPROM write wear: only write during calibration (user-initiated), negligible
- Calibration accuracy: depends on user measurement precision
- State machine complexity: keep simple, only 4 states

## Unresolved Questions

- None — requirements are clear from user description
