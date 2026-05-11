---
title: "Split Calibration & Prioritize Pot Time"
description: "Move calibration to calibration.h and ensure pot time always overrides calibration"
status: pending
priority: P1
effort: 2h
branch: main
tags: [calibration, arduino, refactor]
created: 2026-05-10
---

# Plan: Calibration Split + Pot Priority

## Context
User wants:
1. Move all calibration code to separate `calibration.h` file
2. Ensure manual potentiometer (A2) time always takes priority over any calibrated flow rate values

## Problem Analysis
Current issue: Pump duration seems controlled by calibrated flowRateCalibrated (auto-stop at 5ml) rather than pot-set time. User wants pot time to be the primary control.

## Phases

### Phase 1: Extract Calibration to Header
- Create `calibration.h` with all calibration logic
- Remove calibration code from main `.ino`
- Add `#include "calibration.h"`

### Phase 2: Fix Pot Time Priority
- Ensure `capturedProcessTime` from pot always controls pump duration
- Auto-stop at 5ml should be secondary (only if pot time allows)
- Add debug output to verify pot reading

### Phase 3: Verify
- Compile and test pump duration follows pot setting

## Related Files
- `ArtemiaCode/ArtemiaCode.ino` - main sketch
- `ArtemiaCode/calibration.h` - NEW

## Dependencies
None - standalone refactoring task