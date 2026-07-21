---
title: Fix Reset Button and PUMP_WAIT Stuck Issue
description: Two bugs in ArtemiaCode.ino: reset button not working and PUMP_WAIT stuck when tube removed
status: completed
priority: high
effort: 2h
branch: main
tags: [bugfix, reset-button, pump-wait, artemia]
created: 2026-05-17
completed: 2026-05-17
---

# Plan: Fix Reset Button and PUMP_WAIT Stuck Issue

## Summary

Two bugs in ArtemiaCode.ino:
1. **Reset button doesn't work** - button checked only in unused `runPumpCycle()`, not in main handle sequence
2. **PUMP_WAIT stuck when tube removed** - timer resets in loop causing infinite 0.x second waiting

## Implementation Summary

Both issues were fixed in `ArtemiaCode.ino`:

1. **Reset button**: Added button monitoring at the start of `loop()` before the handle sequence. The code checks `digitalRead(resetBtnPin)`, debounces, and calls `abortAll()` when pressed.

2. **PUMP_WAIT stuck**: Changed the sensor check (lines 230-231) to transition to `ROTATING` state immediately when the tube disappears during `PUMP_WAIT`, instead of resetting the timer endlessly.

---

## Issue 1: Reset Button Not Working

### Root Cause

Button check exists only in `runPumpCycle()` (never called). Main loop has zero button monitoring.

### Fix

Add button monitoring at start of `loop()` before line 190:
```cpp
// Check reset button (external pull-up circuit)
bool btnState = digitalRead(resetBtnPin);
if (btnState == LOW && lastAbortBtnState == HIGH) {
  delay(DEBOUNCE_BUTTON);
  if (digitalRead(resetBtnPin) == LOW) {
    abortAll();
    return;
  }
}
if (btnState == HIGH && lastAbortBtnState == LOW) {
  lastAbortBtnState = HIGH;
}
```

---

## Issue 2: PUMP_WAIT Stuck When Tube Removed

### Root Cause

PUMP_WAIT lines 230-231:
```cpp
if (digitalRead(sensorPin) == LOW) {
  stateStartTime = millis();  // Reset timer - wait for stable tube
}
```

When tube removed, sensor goes LOW → timer resets. If sensor bounces, timer resets infinitely at 0.x seconds → never reaches 3s → stuck forever.

**User behavior:** During 1s PUMP_WAIT delay, user removes tube → sensor goes LOW → code resets timer → user keeps tube out → sensor stays LOW → timer resets again and again → stuck.

### Fix

Change lines 230-231. If tube disappears during PUMP_WAIT, go to ROTATING immediately instead of resetting timer:

```cpp
// OLD (lines 230-231):
if (digitalRead(sensorPin) == LOW) {
  stateStartTime = millis();  // Reset timer - wait for stable tube
}

// NEW:
if (digitalRead(sensorPin) == LOW) {
  // Tube disappeared during PUMP_WAIT - give up and rotate back
  handleState = ROTATING;
  stateStartTime = millis();
  break;
}
```

---

## Files to Modify

| File | Change |
|------|--------|
| `ArtemiaCode.ino` | Add button check at start of `loop()` (before line 190) |
| `ArtemiaCode.ino` | Change PUMP_WAIT sensor check to abort instead of timer reset (lines 230-231) |

---

## Success Criteria

1. **Reset button works** - pressing A0 resets tubeCount to 0 from any state
2. **PUMP_WAIT abort** - if tube removed during PUMP_WAIT, machine goes back to ROTATING within 1 loop cycle (no stuck timer)
3. **Normal flow intact** - if tube stays present, PUMP_WAIT completes normally after 3s

---

## Implementation Order

1. Add button monitoring in `loop()` — enables reset testing
2. Fix PUMP_WAIT sensor check — prevents stuck behavior
3. Compile check