---
title: "Phase 2: Abort Button Stop/Reset Fix"
description: "Fix abort button to stop pump immediately and reset state when pressed during pumping"
status: pending
owner: claude
priority: P2
created: 2026-05-02
---

## Context Links

- Parent: [plan.md](../plan.md)
- Deps: [phase-01-calibration.md](./phase-01-calibration.md)
- Code: `ArtemiaCode/ArtemiaCode.ino`

## Overview

Fix abort button to stop pump immediately and reset all counters when pressed during pumping or idle.

### Current Problem

- Reset button only resets `tubeCount`
- Doesn't stop pump if running
- Doesn't reset pump volume/runtime

### Fix Behavior

| State | Abort Button Action |
|-------|---------------------|
| PUMPING | Stop pump, reset pumpRuntime/pumpVolume/tubeCount to 0 |
| IDLE | Reset tubeCount to 0 (existing behavior) |
| CALIBRATING | Cancel calibration, return to IDLE |
| CALIB_INPUT | Cancel calibration, return to IDLE |

## Implementation Steps

### 1. Add abortCheck() function for centralized handling

```cpp
void abortAll() {
  // Stop pump if running
  analogWrite(pumpPWM, 0);
  digitalWrite(pumpDir, LOW);

  // Reset pump state
  pumpRuntime = 0;
  pumpVolume = 0.0;
  pumpRunning = false;
  currentSpeed = 0;

  // Reset tube count
  tubeCount = 0;

  // Exit any special mode
  systemState = IDLE;

  // Show abort message
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("!! ABORTED !!");
  delay(1000);
  displayUpdate();
}
```

### 2. Replace direct button check with abortCheck()

Current code:
```cpp
if (digitalRead(resetBtnPin) == LOW) {
  tubeCount = 0;
  delay(DEBOUNCE_BUTTON);
}
```

New code:
```cpp
if (digitalRead(resetBtnPin) == LOW) {
  delay(DEBOUNCE_BUTTON);  // Debounce first
  if (digitalRead(resetBtnPin) == LOW) {  // Still pressed = real press
    abortAll();
    // Skip rest of loop to avoid re-triggering
    return;
  }
}
```

### 3. Integration with pump running block

In the pump running block, check abort button continuously:

```cpp
// Inside pump running block (delay loop):
int pumpSpeed = readPotSpeed();
currentSpeed = pumpSpeed;
pumpRunning = true;
pumpStartTime = millis();
digitalWrite(pumpDir, HIGH);
analogWrite(pumpPWM, pumpSpeed);
displayUpdate();

unsigned long pumpLoopStart = millis();
while (pumpRunning) {
  // Check abort every iteration
  if (digitalRead(resetBtnPin) == LOW) {
    abortAll();
    return;
  }

  // Check volume auto-stop
  float currentVolume = (millis() - pumpStartTime) / 1000.0 * flowRateCalibrated;
  if (currentVolume >= TARGET_VOLUME) {
    analogWrite(pumpPWM, 0);
    pumpRuntime += millis() - pumpStartTime;
    pumpVolume = pumpRuntime / 1000.0 * flowRateCalibrated;
    pumpRunning = false;
    currentSpeed = 0;
    displayUpdate();
    return;
  }

  // Check if delay elapsed
  if (millis() - pumpLoopStart >= 2000) {
    analogWrite(pumpPWM, 0);
    pumpRuntime += millis() - pumpStartTime;
    pumpVolume = pumpRuntime / 1000.0 * flowRateCalibrated;
    pumpRunning = false;
    currentSpeed = 0;
    displayUpdate();
    return;
  }

  displayUpdate();
}
```

## Todo List

- [ ] Add abortAll() function
- [ ] Add abort check in pump running loop
- [ ] Add abort check in loop() main section
- [ ] Verify abort works in all states

## Success Criteria

- Pressing abort button while pumping stops pump immediately
- All counters (pumpRuntime, pumpVolume, tubeCount) reset to 0
- "ABORTED" message shows briefly on LCD
- Pressing abort in IDLE still just resets tubeCount
- No double-triggering from button bounce

## Risk Assessment

- Button bounce: debounce delay before checking second time
- State leakage: ensure all states cleared on abort
- Return in loop: prevents further processing after abort
