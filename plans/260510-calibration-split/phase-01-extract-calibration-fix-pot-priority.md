# Phase 1: Extract Calibration to Header + Fix Pot Priority

## Context
- Parent plan: `plans/260510-calibration-split/plan.md`
- Related docs: `docs/operation-rules.md`

## Overview
- Date: 2026-05-10
- Priority: P1
- Status: pending
- Description: Move calibration logic to calibration.h, ensure pot time takes priority over calibration for pump duration

## Key Insights

### Issue 1: Pump timeout not following pot
Current code has TWO stop conditions for pump:
1. **Time-based** (capturedProcessTime from pot A2): `millis() - stateStartTime >= capturedProcessTime * 1000`
2. **Volume-based** (flowRateCalibrated): `currentVolume >= TARGET_VOLUME` (5ml)

**Problem**: If flowRateCalibrated is high, volume check triggers first (~5s), ignoring pot time.

**Fix**: Make pot time the primary controller. Volume auto-stop should only trigger if pot time is > actual fill time needed.

### Issue 2: Calibration code mixed in main sketch
Calibration functions and variables scattered in main .ino make it hard to maintain.

## Requirements
1. Extract calibration to `calibration.h`
2. Pot time (A2) must always control pump duration
3. Volume auto-stop (5ml) is secondary - only stops early if volume reached before pot time expires
4. Debug output to LCD to verify pot reading

## Architecture

### New File: `calibration.h`
```cpp
#ifndef CALIBRATION_H
#define CALIBRATION_H

// Calibration constants
#define CALIB_DURATION 5000       // 5 seconds
#define CALIB_LONG_PRESS 3000     // 3 seconds to enter calib
#define CALIB_SAVE_ADDR 0
#define EEPROM_MAGIC 'A'
#define EEPROM_DEFAULT_RATE 0.5

// Global calibration variables (declared extern for .ino access)
extern unsigned long calibStartTime;
extern float calibInputVolume;
extern float flowRateCalibrated;
extern bool calibBtnWasHigh;
extern unsigned long calibPressStart;

// Functions
void loadFlowRate();
void saveFlowRate();
void handleCalibrationState(LiquidCrystal_I2C &lcd, int speedPotPin, int resetBtnPin);

#endif
```

### Modified: `PUMP_FILL` logic
```cpp
// Check timeout FIRST (pot time is primary)
if (millis() - stateStartTime >= (unsigned long)capturedProcessTime * 1000UL) {
  // Timeout - stop pump
  // ...stop pump, count tube...
}
// Only check volume auto-stop AFTER confirming pot time hasn't expired
else {
  float currentVolume = (millis() - pumpStartTime) / 1000.0 * flowRateCalibrated;
  if (currentVolume >= TARGET_VOLUME) {
    // Volume reached before timeout - stop pump
    // ...stop pump, count tube...
  }
}
```

## Related Code Files
- `ArtemiaCode/ArtemiaCode.ino` - modify (remove calibration, fix PUMP_FILL)
- `ArtemiaCode/calibration.h` - create (all calibration logic)

## Implementation Steps

### Step 1: Create `calibration.h`
Create header with:
- Calibration constants
- Extern variable declarations
- Function prototypes

### Step 2: Move calibration functions from .ino to .h
Move to inside calibration.h (or create calibration.cpp):
- `loadFlowRate()`
- `saveFlowRate()`
- Calibration button handling (loop section lines 251-271)
- `handleStateMachine()` for calibration

### Step 3: Modify `ArtemiaCode.ino`
- Add `#include "calibration.h"` at top
- Remove lines 31-41 (calibration constants/variables)
- Remove lines 88-99 (loadFlowRate, saveFlowRate)
- Remove lines 251-271 (calibration button handling)
- Remove handleStateMachine() function (lines 471-583)
- Add `pinMode(timePot, INPUT);` in setup if missing

### Step 4: Fix PUMP_FILL logic
Ensure timeout check happens BEFORE volume check. Pot time must always be primary.

## Todo List
- [ ] Create calibration.h with all calibration code
- [ ] Modify ArtemiaCode.ino to remove calibration code
- [ ] Add include for calibration.h
- [ ] Fix PUMP_FILL to prioritize pot time over volume auto-stop
- [ ] Add debug LCD output for pot value reading

## Success Criteria
1. Calibration mode still works (long-press 3s to enter)
2. Pump duration follows pot setting (2-20s) not calibration
3. Volume auto-stop (5ml) only triggers if pot time allows
4. LCD shows correct pot reading

## Risk Assessment
- **Risk**: Breaking calibration functionality during move
- **Mitigation**: Keep backup of current .ino, test calibration mode after changes

## Security Considerations
None - offline Arduino project

## Next Steps
Test pump duration with pot set to max (20s) - should run full 20s not 5-7s